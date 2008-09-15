/**
 * vim: set ts=4 :
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

#include "HandleSys.h"
#include "ShareSys.h"
#include "PluginSys.h"
#include "ExtensionSys.h"
#include "Logger.h"
#include <assert.h>
#include <string.h>
#include "sm_stringutil.h"

HandleSystem g_HandleSys;

QHandle *ignore_handle;

inline HandleType_t TypeParent(HandleType_t type)
{
	return (type & ~HANDLESYS_SUBTYPE_MASK);
}

inline HandleError IdentityHandle(IdentityToken_t *token, unsigned int *index)
{
	return g_HandleSys.GetHandle(token->ident, g_ShareSys.GetIdentRoot(), &ignore_handle, index);
}

HandleSystem::HandleSystem()
{
	m_Handles = new QHandle[HANDLESYS_MAX_HANDLES + 1];
	memset(m_Handles, 0, sizeof(QHandle) * (HANDLESYS_MAX_HANDLES + 1));

	m_Types = new QHandleType[HANDLESYS_TYPEARRAY_SIZE];
	memset(m_Types, 0, sizeof(QHandleType) * HANDLESYS_TYPEARRAY_SIZE);

	m_TypeLookup = sm_trie_create();
	m_strtab = new BaseStringTable(512);

	m_TypeTail = 0;
}

HandleSystem::~HandleSystem()
{
	delete [] m_Handles;
	delete [] m_Types;
	sm_trie_destroy(m_TypeLookup);
	delete m_strtab;
}


HandleType_t HandleSystem::CreateType(const char *name, 
									  IHandleTypeDispatch *dispatch, 
									  HandleType_t parent, 
									  const TypeAccess *typeAccess, 
									  const HandleAccess *hndlAccess, 
									  IdentityToken_t *ident,
									  HandleError *err)
{
	if (!dispatch)
	{
		if (err)
		{
			*err = HandleError_Parameter;
		}
		return 0;
	}

	if (typeAccess && typeAccess->hsVersion > SMINTERFACE_HANDLESYSTEM_VERSION)
	{
		if (err)
		{
			*err = HandleError_Version;
		}
		return 0;
	}

	if (hndlAccess && hndlAccess->hsVersion > SMINTERFACE_HANDLESYSTEM_VERSION)
	{
		if (err)
		{
			*err = HandleError_Version;
		}
		return 0;
	}

	bool isChild = false;

	if (parent != 0)
	{
		isChild = true;
		if (parent & HANDLESYS_SUBTYPE_MASK)
		{
			if (err)
			{
				*err = HandleError_NoInherit;
			}
			return 0;
		}
		if (parent >= HANDLESYS_TYPEARRAY_SIZE
			|| m_Types[parent].dispatch == NULL)
		{
			if (err)
			{
				*err = HandleError_Parameter;
			}
			return 0;
		}
		if (m_Types[parent].typeSec.access[HTypeAccess_Inherit] == false
			&& (m_Types[parent].typeSec.ident != ident))
		{
			if (err)
			{
				*err = HandleError_Access;
			}
			return 0;
		}
	}

	if (name && name[0] != '\0')
	{
		if (sm_trie_retrieve(m_TypeLookup, name, NULL))
		{
			if (err)
			{
				*err = HandleError_Parameter;
			}
			return 0;
		}
	}

	unsigned int index;

	if (isChild)
	{
		QHandleType *pParent = &m_Types[parent];
		if (pParent->children >= HANDLESYS_MAX_SUBTYPES)
		{
			if (err)
			{
				*err = HandleError_Limit;
			}
			return 0;
		}
		index = 0;
		for (unsigned int i=1; i<=HANDLESYS_MAX_SUBTYPES; i++)
		{
			if (m_Types[parent + i].dispatch == NULL)
			{
				index = parent + i;
				break;
			}
		}
		if (!index)
		{
			if (err)
			{
				*err = HandleError_Limit;
			}
			return 0;
		}
		pParent->children++;
	} else {
		if (m_FreeTypes == 0)
		{
			/* Reserve another index */
			if (m_TypeTail >= HANDLESYS_TYPEARRAY_SIZE)
			{
				if (err)
				{
					*err = HandleError_Limit;
				}
				return 0;
			} else {
				m_TypeTail += (HANDLESYS_MAX_SUBTYPES + 1);
				index = m_TypeTail;
			}
		} else {
			/* The "free array" is compacted into the normal array for easiness */
			index = m_Types[m_FreeTypes--].freeID;
		}
	}

	QHandleType *pType = &m_Types[index];
	
	pType->dispatch = dispatch;
	if (name && name[0] != '\0')
	{
		pType->nameIdx = m_strtab->AddString(name);
		sm_trie_insert(m_TypeLookup, name, (void *)pType);
	} else {
		pType->nameIdx = -1;
	}

	pType->opened = 0;

	if (typeAccess)
	{
		pType->typeSec = *typeAccess;
	} else {
		InitAccessDefaults(&pType->typeSec, NULL);
		pType->typeSec.ident = ident;
	}

	if (hndlAccess)
	{
		pType->hndlSec = *hndlAccess;
	} else {
		InitAccessDefaults(NULL, &pType->hndlSec);
	}

	if (!isChild)
	{
		pType->children = 0;
	}

	return index;
}

bool HandleSystem::FindHandleType(const char *name, HandleType_t *type)
{
	QHandleType *_type;

	if (!sm_trie_retrieve(m_TypeLookup, name, (void **)&_type))
	{
		return false;
	}

	unsigned int offset = _type - m_Types;

	if (type)
	{
		*type = offset;
	}

	return true;
}

HandleError HandleSystem::TryAllocHandle(unsigned int *handle)
{
	if (m_FreeHandles == 0)
	{
		if (m_HandleTail >= HANDLESYS_MAX_HANDLES)
		{
			return HandleError_Limit;;
		}
		*handle = ++m_HandleTail;
	}
	else
	{
		*handle = m_Handles[m_FreeHandles--].freeID;
	}

	return HandleError_None;
}

HandleError HandleSystem::MakePrimHandle(HandleType_t type, 
						   QHandle **in_pHandle, 
						   unsigned int *in_index, 
						   Handle_t *in_handle,
						   IdentityToken_t *owner,
						   bool identity)
{
	HandleError err;
	unsigned int owner_index = 0;

	if (owner && (IdentityHandle(owner, &owner_index) != HandleError_None))
	{
		return HandleError_Identity;
	}

	unsigned int handle;
	if ((err = TryAllocHandle(&handle)) != HandleError_None)
	{
		if (!TryAndFreeSomeHandles()
			|| (err = TryAllocHandle(&handle)) != HandleError_None)
		{
			return err;
		}
	}

	QHandle *pHandle = &m_Handles[handle];
	
	assert(pHandle->set == false);

	if (++m_HSerial >= HANDLESYS_MAX_SERIALS)
	{
		m_HSerial = 1;
	}

	/* Set essential information */
	pHandle->set = identity ? HandleSet_Identity : HandleSet_Used;
	pHandle->refcount = 1;
	pHandle->type = type;
	pHandle->serial = m_HSerial;
	pHandle->owner = owner;
	pHandle->ch_next = 0;
	pHandle->access_special = false;
	pHandle->is_destroying = false;

	/* Create the hash value */
	Handle_t hash = pHandle->serial;
	hash <<= 16;
	hash |= handle;

	/* Add a reference count to the type */
	m_Types[type].opened++;

	/* Output */
	*in_pHandle = pHandle;
	*in_index = handle;
	*in_handle = hash;

	/* Decode the identity token 
	 * For now, we don't allow nested ownership
	 */
	if (owner && !identity)
	{
		QHandle *pIdentity = &m_Handles[owner_index];
		if (pIdentity->ch_prev == 0)
		{
			pIdentity->ch_prev = handle;
			pIdentity->ch_next = handle;
			pHandle->ch_prev = 0;
		}
		else
		{
			/* Link previous node to us (forward) */
			m_Handles[pIdentity->ch_next].ch_next = handle;
			/* Link us to previous node (backwards) */
			pHandle->ch_prev = pIdentity->ch_next;
			/* Set new tail */
			pIdentity->ch_next = handle;
		}
		pIdentity->refcount++;
	}
	else
	{
		pHandle->ch_prev = 0;
	}

	return HandleError_None;
}

void HandleSystem::SetTypeSecurityOwner(HandleType_t type, IdentityToken_t *pToken)
{
	if (!type
		|| type >= HANDLESYS_TYPEARRAY_SIZE
		|| m_Types[type].dispatch == NULL)
	{
		return;
	}

	m_Types[type].typeSec.ident = pToken;
}

Handle_t HandleSystem::CreateHandleInt(HandleType_t type, 
									   void *object, 
									   const HandleSecurity *pSec,
									   HandleError *err, 
									   const HandleAccess *pAccess,
									   bool identity)
{
	IdentityToken_t *ident;
	IdentityToken_t *owner;

	if (pSec)
	{
		ident = pSec->pIdentity;
		owner = pSec->pOwner;
	} else {
		ident = NULL;
		owner = NULL;
	}

	if (!type 
		|| type >= HANDLESYS_TYPEARRAY_SIZE
		|| m_Types[type].dispatch == NULL)
	{
		if (err)
		{
			*err = HandleError_Parameter;
		}
		return 0;
	}

	/* Check to see if we're allowed to create this handle type */
	QHandleType *pType = &m_Types[type];
	if (!pType->typeSec.access[HTypeAccess_Create]
	&& (!pType->typeSec.ident
		|| pType->typeSec.ident != ident))
	{
		if (err)
		{
			*err = HandleError_Access;
		}
		return 0;
	}

	unsigned int index;
	Handle_t handle;
	QHandle *pHandle;
	HandleError _err;

	if ((_err=MakePrimHandle(type, &pHandle, &index, &handle, owner, identity)) != HandleError_None)
	{
		if (err)
		{
			*err = _err;
		}
		return 0;
	}

	if (pAccess)
	{
		pHandle->access_special = true;
		pHandle->sec = *pAccess;
	}

	pHandle->object = object;
	pHandle->clone = 0;

	return handle;
}

Handle_t HandleSystem::CreateHandleEx(HandleType_t type, void *object, const HandleSecurity *pSec, const HandleAccess *pAccess, HandleError *err)
{
	return CreateHandleInt(type, object, pSec, err, pAccess, false);
}

Handle_t HandleSystem::CreateHandle(HandleType_t type, void *object, IdentityToken_t *owner, IdentityToken_t *ident, HandleError *err)
{
	HandleSecurity sec;

	sec.pIdentity = ident;
	sec.pOwner = owner;

	return CreateHandleEx(type, object, &sec, NULL, err);
}

bool HandleSystem::TypeCheck(HandleType_t intype, HandleType_t outtype)
{
	/* Check the type inheritance */
	if (intype & HANDLESYS_SUBTYPE_MASK)
	{
		if (intype != outtype
			&& (TypeParent(intype) != TypeParent(outtype)))
		{
			return false;
		}
	} else {
		if (intype != outtype)
		{
			return false;
		}
	}

	return true;
}

HandleError HandleSystem::GetHandle(Handle_t handle,
									IdentityToken_t *ident, 
									QHandle **in_pHandle, 
									unsigned int *in_index,
									bool ignoreFree)
{
	unsigned int serial = (handle >> 16);
	unsigned int index = (handle & HANDLESYS_HANDLE_MASK);

	if (index == 0 || index == 0xFFFF)
	{
		return HandleError_Index;
	}

	QHandle *pHandle = &m_Handles[index];

	if (!pHandle->set
		|| (pHandle->set == HandleSet_Freed && !ignoreFree))
	{
		return HandleError_Freed;
	} else if (pHandle->set == HandleSet_Identity
			   && ident != g_ShareSys.GetIdentRoot())
	{
		/* Only IdentityHandle() can read this! */
		return HandleError_Identity;
	}
	if (pHandle->serial != serial)
	{
		return HandleError_Changed;
	}

	*in_pHandle = pHandle;
	*in_index = index;

	return HandleError_None;
}

bool HandleSystem::CheckAccess(QHandle *pHandle, HandleAccessRight right, const HandleSecurity *pSecurity)
{
	QHandleType *pType = &m_Types[pHandle->type];
	unsigned int access;

	if (pHandle->access_special)
	{
		access = pHandle->sec.access[right];
	} else {
		access = pType->hndlSec.access[right];
	}

	/* Check if the type's identity matches */
	if (access & HANDLE_RESTRICT_IDENTITY)
	{
		IdentityToken_t *owner = pType->typeSec.ident;
		if (!owner
			|| (!pSecurity || pSecurity->pIdentity != owner))
		{
			return false;
		}
	}

	/* Check if the owner is allowed */
	if (access & HANDLE_RESTRICT_OWNER)
	{
		IdentityToken_t *owner = pHandle->owner;
		if (owner
			&& (!pSecurity || pSecurity->pOwner != owner))
		{
			return false;
		}
	}

	return true;
}

HandleError HandleSystem::CloneHandle(QHandle *pHandle, unsigned int index, Handle_t *newhandle, IdentityToken_t *newOwner)
{
	/* Get a new Handle ID */
	unsigned int new_index;
	QHandle *pNewHandle;
	Handle_t new_handle;
	HandleError err;

	if ((err=MakePrimHandle(pHandle->type, &pNewHandle, &new_index, &new_handle, newOwner)) != HandleError_None)
	{
		return err;
	}

	/* Assign permissions from parent */
	if (pHandle->access_special)
	{
		pNewHandle->access_special = true;
		pNewHandle->sec = pHandle->sec;
	}

	pNewHandle->clone = index;
	pHandle->refcount++;

	*newhandle = new_handle;

	return HandleError_None;
}

HandleError HandleSystem::CloneHandle(Handle_t handle, Handle_t *newhandle, IdentityToken_t *newOwner, const HandleSecurity *pSecurity)
{
	HandleError err;
	QHandle *pHandle;
	unsigned int index;
	IdentityToken_t *ident = pSecurity ? pSecurity->pIdentity : NULL;

	if ((err=GetHandle(handle, ident, &pHandle, &index)) != HandleError_None)
	{
		return err;
	}

	/* Identities cannot be cloned */
	if (pHandle->set == HandleSet_Identity)
	{
		return HandleError_Identity;
	}

	/* Check if the handle can be cloned */
	if (!CheckAccess(pHandle, HandleAccess_Clone, pSecurity))
	{
		return HandleError_Access;
	}

	/* Make sure we're not cloning a clone */
	if (pHandle->clone)
	{
		QHandle *pParent = &m_Handles[pHandle->clone];
		return CloneHandle(pParent, pHandle->clone, newhandle, newOwner);
	}

	return CloneHandle(pHandle, index, newhandle, newOwner);
}

HandleError HandleSystem::FreeHandle(QHandle *pHandle, unsigned int index)
{
	QHandleType *pType = &m_Types[pHandle->type];

	if (pHandle->clone)
	{
		/* If we're a clone, decrease the parent reference count */
		QHandle *pMaster;
		unsigned int master;

		/* Note that if we ever have per-handle security, we would need to re-check
		* the access on this Handle. */
		master = pHandle->clone;
		pMaster = &m_Handles[master];

		/* Release the clone now */
		ReleasePrimHandle(index);

		/* Decrement the master's reference count */
		if (--pMaster->refcount == 0)
		{
			/* Type should be the same but do this anyway... */
			pType = &m_Types[pMaster->type];
			pMaster->is_destroying = true;
			pType->dispatch->OnHandleDestroy(pMaster->type, pMaster->object);
			ReleasePrimHandle(master);
		}
	} else if (pHandle->set == HandleSet_Identity) {
		/* If we're an identity, skip all this stuff!
		 * NOTE: SHARESYS DOES NOT CARE ABOUT THE DESTRUCTOR
		 */
		ReleasePrimHandle(index);
	} else {
		/* Decrement, free if necessary */
		if (--pHandle->refcount == 0)
		{
			pHandle->is_destroying = true;
			pType->dispatch->OnHandleDestroy(pHandle->type, pHandle->object);
			ReleasePrimHandle(index);
		} else {
			/* We must be cloned, so mark ourselves as freed */
			pHandle->set = HandleSet_Freed;
			/* Now, unlink us, so we're not being tracked by the owner */
			if (pHandle->owner)
			{
				UnlinkHandleFromOwner(pHandle, index);
			}
		}
	}

	return HandleError_None;
}

HandleError HandleSystem::FreeHandle(Handle_t handle, const HandleSecurity *pSecurity)
{
	unsigned int index;
	QHandle *pHandle;
	HandleError err;
	IdentityToken_t *ident = pSecurity ? pSecurity->pIdentity : NULL;

	if ((err=GetHandle(handle, ident, &pHandle, &index)) != HandleError_None)
	{
		return err;
	}

	if (!CheckAccess(pHandle, HandleAccess_Delete, pSecurity))
	{
		return HandleError_Access;
	}

	if (pHandle->is_destroying)
	{
		/* Someone tried to free this recursively.  
		 * We'll just ignore this safely.
		 */
		return HandleError_None;
	}

	return FreeHandle(pHandle, index);
}

HandleError HandleSystem::ReadHandle(Handle_t handle, HandleType_t type, const HandleSecurity *pSecurity, void **object)
{
	unsigned int index;
	QHandle *pHandle;
	HandleError err;
	IdentityToken_t *ident = pSecurity ? pSecurity->pIdentity : NULL;

	if ((err=GetHandle(handle, ident, &pHandle, &index)) != HandleError_None)
	{
		return err;
	}

	if (!CheckAccess(pHandle, HandleAccess_Read, pSecurity))
	{
		return HandleError_Access;
	}

	/* Check the type inheritance */
	if (pHandle->type & HANDLESYS_SUBTYPE_MASK)
	{
		if (pHandle->type != type
			&& (TypeParent(pHandle->type) != TypeParent(type)))
		{
			return HandleError_Type;
		}
	} else if (type) {
		if (pHandle->type != type)
		{
			return HandleError_Type;
		}
	}

	if (object)
	{
		/* if we're a clone, the rules change - object is ONLY in our reference */
		if (pHandle->clone)
		{
			pHandle = &m_Handles[pHandle->clone];
		}
		*object = pHandle->object;
	}

	return HandleError_None;
}

void HandleSystem::UnlinkHandleFromOwner(QHandle *pHandle, unsigned int index)
{
	/* Unlink us if necessary */
	unsigned int ident_index;
	if (IdentityHandle(pHandle->owner, &ident_index) != HandleError_None)
	{
		/* Uh-oh! */
		assert(pHandle->owner == 0);
		return;
	}
	/* Note that since 0 is an invalid handle, if any of these links are 0,
	* the data can still be set.
	*/
	QHandle *pIdentity = &m_Handles[ident_index];

	/* Unlink case: We're the head AND tail node */
	if (index == pIdentity->ch_prev && index == pIdentity->ch_next)
	{
		pIdentity->ch_prev = 0;
		pIdentity->ch_next = 0;
	}
	/* Unlink case: We're the head node */
	else if (index == pIdentity->ch_prev) {
		/* Link us to the next in the chain */
		pIdentity->ch_prev = pHandle->ch_next;
		/* Patch up the previous link */
		m_Handles[pHandle->ch_next].ch_prev = 0;
	}
	/* Unlink case: We're the tail node */
	else if (index == pIdentity->ch_next) {
		/* Link us to the previous in the chain */
		pIdentity->ch_next = pHandle->ch_prev;
		/* Patch up the next link */
		m_Handles[pHandle->ch_prev].ch_next = 0;
	}
	/* Unlink case: We're in the middle! */
	else {
		/* Patch the forward reference */
		m_Handles[pHandle->ch_next].ch_prev = pHandle->ch_prev;
		/* Patch the backward reference */
		m_Handles[pHandle->ch_prev].ch_next = pHandle->ch_next;
	}

	/* Lastly, decrease the reference count */
	pIdentity->refcount--;
}

void HandleSystem::ReleasePrimHandle(unsigned int index)
{
	QHandle *pHandle = &m_Handles[index];
	HandleSet set = pHandle->set;

	if (pHandle->owner && (set != HandleSet_Identity))
	{
		UnlinkHandleFromOwner(pHandle, index);
	}

	/* Were we an identity ourself? */
	QHandle *pLocal;
	if (set == HandleSet_Identity)
	{
		/* Extra work to do.  We need to find everything connected to this identity and release it. */
		unsigned int ch_index;
#if defined _DEBUG
		unsigned int old_index = 0;
#endif
		while ((ch_index = pHandle->ch_next) != 0)
		{
			pLocal = &m_Handles[ch_index];
#if defined _DEBUG
			assert(old_index != ch_index);
			assert(pLocal->set == HandleSet_Used);
			old_index = ch_index;
#endif
			FreeHandle(pLocal, ch_index);
		}
	}

	pHandle->set = HandleSet_None;
	m_Types[pHandle->type].opened--;
	m_Handles[++m_FreeHandles].freeID = index;
}

bool HandleSystem::RemoveType(HandleType_t type, IdentityToken_t *ident)
{
	if (type == 0 || type >= HANDLESYS_TYPEARRAY_SIZE)
	{
		return false;
	}

	QHandleType *pType = &m_Types[type];

	if (pType->typeSec.ident
		&& pType->typeSec.ident != ident)
	{
		return false;
	}

	if (pType->dispatch == NULL)
	{
		return false;
	}

	/* Remove children if we have to */
	if (!(type & HANDLESYS_SUBTYPE_MASK))
	{
		QHandleType *childType;
		for (unsigned int i=1; i<=HANDLESYS_MAX_SUBTYPES; i++)
		{
			childType = &m_Types[type + i];
			if (childType->dispatch)
			{
				RemoveType(type + i, childType->typeSec.ident);
			}
		}
		/* Link us into the free chain */
		m_Types[++m_FreeTypes].freeID = type;
	}

	/* Invalidate the type now */
	IHandleTypeDispatch *dispatch = pType->dispatch;
	pType->dispatch = NULL;

	/* Make sure nothing is using this type. */
	if (pType->opened)
	{
		QHandle *pHandle;
		for (unsigned int i=1; i<=m_HandleTail; i++)
		{
			pHandle = &m_Handles[i];
			if (!pHandle->set || pHandle->type != type)
			{
				continue;
			}
			if (pHandle->clone)
			{
				/* Get parent */
				QHandle *pOther = &m_Handles[pHandle->clone];
				if (--pOther->refcount == 0)
				{
					/* Free! */
					dispatch->OnHandleDestroy(type, pOther->object);
					ReleasePrimHandle(pHandle->clone);
				}
				/* Unlink ourselves since we don't have a reference count */
				ReleasePrimHandle(i);
			} else {
				/* If it's not a clone, we still have to check the reference count.
				 * Either way, we'll be destroyed eventually because the handle types do not change.
				 */
				if (--pHandle->refcount == 0)
				{
					/* Free! */
					dispatch->OnHandleDestroy(type, pHandle->object);
					ReleasePrimHandle(i);
				}
			}
			if (pType->opened == 0)
			{
				break;
			}
		}
	}

	/* Remove it from the type cache. */
	if (pType->nameIdx != -1)
	{
		const char *typeName;

		typeName = m_strtab->GetString(pType->nameIdx);
		sm_trie_delete(m_TypeLookup, typeName);
	}

	return true;
}

bool HandleSystem::InitAccessDefaults(TypeAccess *pTypeAccess, HandleAccess *pHandleAccess)
{
	if (pTypeAccess)
	{
		if (pTypeAccess->hsVersion > SMINTERFACE_HANDLESYSTEM_VERSION)
		{
			return false;
		}
		pTypeAccess->access[HTypeAccess_Create] = false;
		pTypeAccess->access[HTypeAccess_Inherit] = false;
		pTypeAccess->ident = NULL;
	}

	if (pHandleAccess)
	{
		if (pHandleAccess->hsVersion > SMINTERFACE_HANDLESYSTEM_VERSION)
		{
			return false;
		}

		pHandleAccess->access[HandleAccess_Clone] = 0;
		pHandleAccess->access[HandleAccess_Delete] = HANDLE_RESTRICT_OWNER;
		pHandleAccess->access[HandleAccess_Read] = HANDLE_RESTRICT_IDENTITY;
	}

	return true;
}

bool HandleSystem::TryAndFreeSomeHandles()
{
	IPluginIterator *pl_iter = g_PluginSys.GetPluginIterator();
	IPlugin *highest_owner = NULL;
	unsigned int highest_handle_count = 0;

	/* Search all plugins */
	while (pl_iter->MorePlugins())
	{
		IPlugin *plugin = pl_iter->GetPlugin();
		IdentityToken_t *identity = plugin->GetIdentity();
		unsigned int handle_count = 0;

		if (identity == NULL)
		{
			continue;
		}

		/* Search all handles */
		for (unsigned int i = 1; i <= m_HandleTail; i++)
		{
			if (m_Handles[i].set != HandleSet_Used)
			{
				continue;
			}
			if (m_Handles[i].owner == identity)
			{
				handle_count++;
			}
		}

		if (handle_count > highest_handle_count)
		{
			highest_owner = plugin;
			highest_handle_count = handle_count;
		}

		pl_iter->NextPlugin();
	}

	if (highest_owner == NULL || highest_handle_count == 0)
	{
		return false;
	}

	g_Logger.LogFatal("[SM] MEMORY LEAK DETECTED IN PLUGIN (file \"%s\")", highest_owner->GetFilename());
	g_Logger.LogFatal("[SM] Reloading plugin to free %d handles.", highest_handle_count);
	g_Logger.LogFatal("[SM] Contact the author(s) of this plugin to correct this error.", highest_handle_count);

	highest_owner->GetBaseContext()->ThrowNativeErrorEx(SP_ERROR_MEMACCESS, "Memory leak");

	return g_PluginSys.UnloadPlugin(highest_owner);
}

void HandleSystem::Dump(HANDLE_REPORTER rep)
{
	unsigned int total_size = 0;
	rep("%-10.10s\t%-20.20s\t%-20.20s\t%-10.10s", "Handle", "Owner", "Type", "Memory");
	rep("--------------------------------------------------------------------------");
	for (unsigned int i = 1; i <= m_HandleTail; i++)
	{
		if (m_Handles[i].set != HandleSet_Used)
		{
			continue;
		}
		/* Get the index */
		unsigned int index = (m_Handles[i].serial << 16) | i;
		/* Determine the owner */
		const char *owner = "UNKNOWN";
		if (m_Handles[i].owner)
		{
			IdentityToken_t *pOwner = m_Handles[i].owner;
			if (pOwner == g_pCoreIdent)
			{
				owner = "CORE";
			}
			else if (pOwner == g_PluginSys.GetIdentity())
			{
				owner = "PLUGINSYS";
			}
			else
			{
				CExtension *ext = g_Extensions.GetExtensionFromIdent(pOwner);
				if (ext)
				{
					owner = ext->GetFilename();
				}
				else
				{
					CPlugin *pPlugin = g_PluginSys.GetPluginFromIdentity(pOwner);
					if (pPlugin)
					{
						owner = pPlugin->GetFilename();
					}
				}
			}
		}
		else
		{
			owner = "NONE";
		}
		const char *type = "ANON";
		QHandleType *pType = &m_Types[m_Handles[i].type];
		unsigned int size = 0;
		if (pType->nameIdx != -1)
		{
			type = m_strtab->GetString(pType->nameIdx);
		}
		if (pType->dispatch->GetDispatchVersion() < HANDLESYS_MEMUSAGE_MIN_VERSION
			|| !pType->dispatch->GetHandleApproxSize(m_Handles[i].type, m_Handles[i].object, &size))
		{
			rep("0x%08x\t%-20.20s\t%-20.20s\t%-10.10s", index, owner, type, "-1");
		}
		else
		{
			char buffer[32];
			UTIL_Format(buffer, sizeof(buffer), "%d", size);
			rep("0x%08x\t%-20.20s\t%-20.20s\t%-10.10s", index, owner, type, buffer);
			total_size += size;
		}
	}
	rep("-- Approximately %d bytes of memory are in use by Handles.\n", total_size);
}

