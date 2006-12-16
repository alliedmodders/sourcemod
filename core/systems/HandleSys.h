#ifndef _INCLUDE_SOURCEMOD_HANDLESYSTEM_H_
#define _INCLUDE_SOURCEMOD_HANDLESYSTEM_H_

#include <IHandleSys.h>
#include "sm_globals.h"
#include "sm_trie.h"
#include "sourcemod.h"
#include "sm_memtable.h"

#define HANDLESYS_MAX_HANDLES		(1<<16)
#define HANDLESYS_MAX_TYPES			(1<<9)
#define HANDLESYS_MAX_SUBTYPES		0xF
#define HANDLESYS_SUBTYPE_MASK		0xF
#define HANDLESYS_TYPEARRAY_SIZE	(HANDLESYS_MAX_TYPES * (HANDLESYS_MAX_SUBTYPES + 1))
#define HANDLESYS_MAX_SERIALS		0xFFFF
#define HANDLESYS_SERIAL_MASK		0xFFFF0000
#define HANDLESYS_HANDLE_MASK		0x0000FFFF

struct QHandle
{
	HandleType_t type;
	void *object;
	unsigned int freeID;
	IdentityToken_t source;
	bool set;
	unsigned int serial;
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
	HandleError DestroyHandle(Handle_t handle, IdentityToken_t ident);
	HandleError ReadHandle(Handle_t handle, HandleType_t type, IdentityToken_t ident, void **object);
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
