#ifndef _INCLUDE_SOURCEMOD_HANDLESYSTEM_H_
#define _INCLUDE_SOURCEMOD_HANDLESYSTEM_H_

#include <IHandleSys.h>
#include "sm_globals.h"
#include "sm_trie.h"
#include "sourcemod.h"
#include "sm_memtable.h"

#define HANDLESYS_MAX_HANDLES		(1<<14)
#define HANDLESYS_MAX_TYPES			(1<<9)
#define HANDLESYS_MAX_SUBTYPES		0xF
#define HANDLESYS_SUBTYPE_MASK		0xF
#define HANDLESYS_TYPEARRAY_SIZE	(HANDLESYS_MAX_TYPES * (HANDLESYS_MAX_SUBTYPES + 1))
#define HANDLESYS_MAX_SERIALS		0xFFFF
#define HANDLESYS_SERIAL_MASK		0xFFFF0000
#define HANDLESYS_HANDLE_MASK		0x0000FFFF

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
 * index into the handle array.  Each subsequent Handle who is owned by that indentity is mapped into
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
	Handle_t clone;				/* If non-zero, this is our cloned parent */
	HandleSet set;				/* Information about the handle's state */
	/* The following variables are unrelated to the Handle array, and used 
	 * as an inlined chain of information */
	unsigned int freeID;		/* ID of a free handle in the free handle chain */
	/* Indexes into the handle array for owner membership.
	 * For identity roots, these are treated as the head/tail. */
	unsigned int ch_prev;		/* chained previous handle or HEAD */
	unsigned int ch_next;		/* chained next handle or TAIL */
};

struct QHandleType
{
	IHandleTypeDispatch *dispatch;
	unsigned int freeID;
	unsigned int children;
	HandleSecurity sec;
	unsigned int opened;
	int nameIdx;
};

class HandleSystem : 
	public IHandleSys
{
	friend HandleError IdentityHandle(IdentityToken_t *token, unsigned int *index);
	friend class ShareSystem;
public:
	HandleSystem();
	~HandleSystem();
public: //IHandleSystem
	HandleType_t CreateType(const char *name, IHandleTypeDispatch *dispatch);
	HandleType_t CreateTypeEx(const char *name,	
							  IHandleTypeDispatch *dispatch, 
							  HandleType_t parent, 
							  const HandleSecurity *security,
							  IdentityToken_t *ident);
	HandleType_t CreateChildType(const char *name, HandleType_t parent, IHandleTypeDispatch *dispatch);
	bool RemoveType(HandleType_t type, IdentityToken_t *ident);
	bool FindHandleType(const char *name, HandleType_t *type);
	Handle_t CreateHandle(HandleType_t type, 
							void *object, 
							IdentityToken_t *source, 
							IdentityToken_t *ident);
	Handle_t CreateScriptHandle(HandleType_t type, void *object, IPluginContext *pContext, IdentityToken_t *ident);
	HandleError FreeHandle(Handle_t handle, IdentityToken_t *owner, IdentityToken_t *ident);
	HandleError CloneHandle(Handle_t handle, Handle_t *newhandle, IdentityToken_t *source, IdentityToken_t *ident);
	HandleError ReadHandle(Handle_t handle, HandleType_t type, IdentityToken_t *ident, void **object);
	bool TypeCheck(HandleType_t intype, HandleType_t outtype);
protected:
	/**
	 * Decodes a handle with sanity and security checking.
	 */
	HandleError GetHandle(Handle_t handle, 
						  IdentityToken_t *ident, 
						  QHandle **pHandle, 
						  unsigned int *index,
						  HandleAccessRight access,
						  bool ignoreFree=false);

	/**
	 * Creates a basic handle and sets its reference count to 1.
	 * Does not do any type or security checking.
	 */
	HandleError MakePrimHandle(HandleType_t type, 
							   QHandle **pHandle, 
							   unsigned int *index, 
							   HandleType_t *handle,
							   IdentityToken_t *owner);

	/**
	 * Frees a primitive handle.  Does no object freeing, only reference count, bookkeepping, 
	 * and linked list maintenance.
	 */
	void ReleasePrimHandle(unsigned int index);

	/**
	 * Sets the security owner of a type
	 */
	void SetTypeSecurityOwner(HandleType_t type, IdentityToken_t *pToken);

	/** 
	 * Marks a handle as an identity.
	 * This prevents it from being tampered with by outside stuff
	 */
	void MarkHandleAsIdentity(Handle_t handle);
private:
	QHandle *m_Handles;
	QHandleType *m_Types;
	Trie *m_TypeLookup;
	unsigned int m_TypeTail;
	unsigned int m_FreeTypes;
	unsigned int m_HandleTail;
	unsigned int m_FreeHandles;
	unsigned int m_HSerial;
	BaseStringTable *m_strtab;
};

extern HandleSystem g_HandleSys;

#endif //_INCLUDE_SOURCEMOD_HANDLESYSTEM_H_
