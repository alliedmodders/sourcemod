/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */

#ifndef _INCLUDE_SOURCEMOD_HANDLESYSTEM_H_
#define _INCLUDE_SOURCEMOD_HANDLESYSTEM_H_

#include <stdio.h>

#include <memory>

#include <amtl/am-string.h>
#include <amtl/am-function.h>
#include <IHandleSys.h>
#include <sm_namehashset.h>
#include "common_logic.h"

#define HANDLESYS_HANDLE_BITS   20
#define HANDLESYS_MAX_HANDLES		((1 << HANDLESYS_HANDLE_BITS) - 1)
#define HANDLESYS_MAX_TYPES			(1<<9)
#define HANDLESYS_MAX_SUBTYPES		0xF
#define HANDLESYS_SUBTYPE_MASK		0xF
#define HANDLESYS_TYPEARRAY_SIZE	(HANDLESYS_MAX_TYPES * (HANDLESYS_MAX_SUBTYPES + 1))
#define HANDLESYS_SERIAL_BITS		(32 - HANDLESYS_HANDLE_BITS)
#define HANDLESYS_MAX_SERIALS		(1 << HANDLESYS_SERIAL_BITS)
#define HANDLESYS_SERIAL_MASK		(((1 << HANDLESYS_SERIAL_BITS) - 1) << HANDLESYS_HANDLE_BITS)
#define HANDLESYS_HANDLE_MASK		((1 << HANDLESYS_HANDLE_BITS) - 1)
#define HANDLESYS_WARN_USAGE		100000

#define HANDLESYS_MEMUSAGE_MIN_VERSION		3

/**
 *   The QHandle is a nasty structure that compacts the handle system into a big vector.
 * The members of the vector each encapsulate one Handle, however, they also act as nodes
 * in an inlined linked list and an inlined vector.
 *
 *   The first of these lists is the 'freeID' list.  Each node from 1 to N (where N
 * is the number of free nodes) has a 'freeID' that specifies a free Handle ID.  This
 * is a quick hack to get around allocating a second base vector.
 *
 *   The second vector is the identity linked list.  An identity has its own handle, so
 * these handles are used as sentinel nodes for index linking.  They point to the first and last
 * index into the handle array.  Each subsequent Handle who is owned by that identity is mapped into
 * that list.  This lets owning identities be unloaded in O(n) time.
 *
 *   Eventually, there may be a third list for type chains.
 */

enum HandleSet
{
	HandleSet_None = 0,
	HandleSet_Used,			/* The Handle is in use */
	HandleSet_Freed,		/* The "master" Handle of a clone chain is freed */
	HandleSet_Identity,		/* The Handle is a special identity */
};

struct QHandle
{
	HandleType_t type;			/* Handle type */
	void *object;				/* Unmaintained object pointer */
	IdentityToken_t *owner;		/* Identity of object which owns this */
	unsigned int serial;		/* Serial no. for sanity checking */
	unsigned int refcount;		/* Reference count for safe destruction */
	unsigned int clone;			/* If non-zero, this is our cloned parent index */
	HandleSet set;				/* Information about the handle's state */
	bool access_special;		/* Whether or not access rules are special or type-derived */
	bool is_destroying;			/* Whether or not the handle is being destroyed */
	HandleAccess sec;			/* Security rules */
	time_t timestamp;			/* Creation timestamp */
	/* The following variables are unrelated to the Handle array, and used 
	 * as an inlined chain of information */
	unsigned int freeID;		/* ID of a free handle in the free handle chain */
	/* Indexes into the handle array for owner membership.
	 * For identity roots, these are treated as the head/tail. */
	unsigned int ch_prev;		/* chained previous handle */
	unsigned int ch_next;		/* chained next handle */
};

struct QHandleType
{
	IHandleTypeDispatch *dispatch;
	unsigned int freeID;
	unsigned int children;
	TypeAccess typeSec;
	HandleAccess hndlSec;
	unsigned int opened;
	std::unique_ptr<std::string> name;

	static inline bool matches(const char *key, const QHandleType *type)
	{
		return type->name && type->name->compare(key) == 0;
	}
	static inline uint32_t hash(const detail::CharsAndLength &key)
	{
		return key.hash();
	}
};

typedef ke::Function<void(const char *)> HandleReporter;

class HandleSystem : 
	public IHandleSys
{
	friend HandleError IdentityHandle(IdentityToken_t *token, unsigned int *index);
	friend class ShareSystem;
public:
	HandleSystem();
	~HandleSystem();
public: //IHandleSystem

	HandleType_t CreateType(const char *name,
		IHandleTypeDispatch *dispatch,
		HandleType_t parent,
		const TypeAccess *typeAccess,
		const HandleAccess *hndlAccess,
		IdentityToken_t *ident,
		HandleError *err);

	bool RemoveType(HandleType_t type, IdentityToken_t *ident);

	bool FindHandleType(const char *name, HandleType_t *type);

	Handle_t CreateHandle(HandleType_t type, 
		void *object,
		IdentityToken_t *owner,
		IdentityToken_t *ident,
		HandleError *err);

	HandleError FreeHandle(Handle_t handle, const HandleSecurity *pSecurity);

	HandleError CloneHandle(Handle_t handle, 
		Handle_t *newhandle, 
		IdentityToken_t *newOwner,
		const HandleSecurity *pSecurity);

	HandleError ReadHandle(Handle_t handle, 
		HandleType_t type, 
		const HandleSecurity *pSecurity,
		void **object);

	bool InitAccessDefaults(TypeAccess *pTypeAccess, HandleAccess *pHandleAccess);

	bool TypeCheck(HandleType_t intype, HandleType_t outtype);

	virtual Handle_t CreateHandleEx(HandleType_t type, 
		void *object,
		const HandleSecurity *pSec,
		const HandleAccess *pAccess,
		HandleError *err);

	void Dump(const HandleReporter &reporter);

	/* Bypasses security checks. */
	Handle_t FastCloneHandle(Handle_t hndl);
protected:
	/**
	 * Decodes a handle with sanity and security checking.
	 */
	HandleError GetHandle(Handle_t handle, 
						  IdentityToken_t *ident, 
						  QHandle **pHandle, 
						  unsigned int *index,
						  bool ignoreFree=false);

	Handle_t FastCloneHandle(QHandle *pHandle, unsigned int index);
	void GetHandleUnchecked(Handle_t hndl, QHandle *& pHandle, unsigned int &index);

	/**
	 * Creates a basic handle and sets its reference count to 1.
	 * Does not do any type or security checking.
	 */
	HandleError MakePrimHandle(HandleType_t type, 
							   QHandle **pHandle, 
							   unsigned int *index, 
							   HandleType_t *handle,
							   IdentityToken_t *owner,
							   bool identity=false);

	/**
	 * Frees a primitive handle.  Does no object freeing, only reference count, bookkeeping, 
	 * and linked list maintenance.
	 * If used on an Identity handle, destroys all Handles under that identity.
	 */
	void ReleasePrimHandle(unsigned int index);

	/**
	 * Sets the security owner of a type.
	 */
	void SetTypeSecurityOwner(HandleType_t type, IdentityToken_t *pToken);

	/**
	 * Helper function to check access rights.
	 */
	bool CheckAccess(QHandle *pHandle, HandleAccessRight right, const HandleSecurity *pSecurity);

	/** 
	 * Some wrappers for internal functions, so we can pass indexes instead of encoded handles.
	 */
	HandleError FreeHandle(QHandle *pHandle, unsigned int index);
	void UnlinkHandleFromOwner(QHandle *pHandle, unsigned int index);
	HandleError CloneHandle(QHandle *pHandle, unsigned int index, Handle_t *newhandle, IdentityToken_t *newOwner);
	Handle_t CreateHandleInt(HandleType_t type, void *object, const HandleSecurity *pSec, HandleError *err, const HandleAccess *pAccess, bool identity);

	bool TryAndFreeSomeHandles();
	HandleError TryAllocHandle(unsigned int *handle);
private:
	QHandle *m_Handles;
	QHandleType *m_Types;
	NameHashSet<QHandleType *> m_TypeLookup;
	unsigned int m_TypeTail;
	unsigned int m_FreeTypes;
	unsigned int m_HandleTail;
	unsigned int m_FreeHandles;
	unsigned int m_HSerial;
};

extern HandleSystem g_HandleSys;

struct AutoHandleRooter
{
public:
	AutoHandleRooter(Handle_t hndl)
	{
		if (hndl != BAD_HANDLE)
			this->hndl = g_HandleSys.FastCloneHandle(hndl);
		else
			this->hndl = BAD_HANDLE;
	}

	~AutoHandleRooter()
	{
		if (hndl != BAD_HANDLE)
		{
			HandleSecurity sec(g_pCoreIdent, g_pCoreIdent);
			g_HandleSys.FreeHandle(hndl, &sec);
		}
	}
private:
	Handle_t hndl;
};

#endif //_INCLUDE_SOURCEMOD_HANDLESYSTEM_H_
