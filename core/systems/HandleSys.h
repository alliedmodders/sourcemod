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

struct QHandle
{
	HandleType_t type;			/* Handle type */
	void *object;				/* Unmaintained object pointer */
	unsigned int freeID;		/* ID of a free handle in the free handle chain */
	IdentityToken_t source;		/* Identity of object which owns this */
	unsigned int serial;		/* Serial no. for sanity checking */
	unsigned int refcount;		/* Reference count for safe destruction */
	Handle_t clone;				/* If non-zero, this is our cloned parent */
	bool set;					/* Whether or not this handle is set */
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
public:
	HandleSystem();
	~HandleSystem();
public: //IHandleSystem
	HandleType_t CreateType(const char *name, IHandleTypeDispatch *dispatch);
	HandleType_t CreateTypeEx(const char *name,	
							  IHandleTypeDispatch *dispatch, 
							  HandleType_t parent, 
							  const HandleSecurity *security);
	HandleType_t CreateChildType(const char *name, HandleType_t parent, IHandleTypeDispatch *dispatch);
	bool RemoveType(HandleType_t type, IdentityToken_t ident);
	bool FindHandleType(const char *name, HandleType_t *type);
	Handle_t CreateHandle(HandleType_t type, 
							void *object, 
							IdentityToken_t source, 
							IdentityToken_t ident);
	Handle_t CreateScriptHandle(HandleType_t type, void *object, sp_context_t *ctx, IdentityToken_t ident);
	HandleError FreeHandle(Handle_t handle, IdentityToken_t ident);
	HandleError CloneHandle(Handle_t handle, Handle_t *newhandle, IdentityToken_t source, IdentityToken_t ident);
	HandleError ReadHandle(Handle_t handle, HandleType_t type, IdentityToken_t ident, void **object);
private:
	/**
	 * Decodes a handle with sanity and security checking.
	 */
	HandleError GetHandle(Handle_t handle, 
						  IdentityToken_t ident, 
						  QHandle **pHandle, 
						  unsigned int *index,
						  HandleAccessRight access);

	/**
	 * Creates a basic handle and sets its reference count to 1.
	 * Does not do any type or security checking.
	 */
	HandleError MakePrimHandle(HandleType_t type, QHandle **pHandle, unsigned int *index, HandleType_t *handle);

	/**
	 * Frees a primitive handle.  Does no object freeing, only reference count and bookkeepping.
	 */
	void ReleasePrimHandle(unsigned int index);
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
