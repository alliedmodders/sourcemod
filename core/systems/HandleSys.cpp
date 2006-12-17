#include "HandleSys.h"
#include "PluginSys.h"
#include <assert.h>

HandleSystem g_HandleSys;

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

HandleType_t HandleSystem::CreateType(const char *name, IHandleTypeDispatch *dispatch)
{
	return CreateTypeEx(name, dispatch, 0, NULL);
}

HandleType_t HandleSystem::CreateChildType(const char *name, HandleType_t parent, IHandleTypeDispatch *dispatch)
{
	return CreateTypeEx(name, dispatch, parent, NULL);
}

HandleType_t HandleSystem::CreateTypeEx(const char *name, 
										IHandleTypeDispatch *dispatch, 
										HandleType_t parent, 
										const HandleSecurity *security)
{
	if (!dispatch)
	{
		return 0;
	}

	bool isChild = false;

	if (parent != 0)
	{
		isChild = true;
		if (parent & HANDLESYS_SUBTYPE_MASK)
		{
			return 0;
		}
		if (parent >= HANDLESYS_TYPEARRAY_SIZE
			|| m_Types[parent].dispatch != NULL
			|| m_Types[parent].sec.access[HandleAccess_Inherit] == false)
		{
			return 0;
		}
	}

	if (!security)
	{
		if (isChild)
		{
			security = &m_Types[parent].sec;
		} else {
			static HandleSecurity def_h;
			security = &def_h;
		}
	}

	if (security->access[HandleAccess_Create]
		&& sm_trie_retrieve(m_TypeLookup, name, NULL))
	{
		return 0;
	}

	unsigned int index;

	if (isChild)
	{
		QHandleType *pParent = &m_Types[parent];
		if (pParent->children >= HANDLESYS_MAX_SUBTYPES)
		{
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
			return 0;
		}
		pParent->children++;
	} else {
		if (m_FreeTypes == 0)
		{
			/* Reserve another index */
			if (m_TypeTail >= HANDLESYS_TYPEARRAY_SIZE)
			{
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
	pType->sec = *security;
	pType->opened = 0;

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

HandleError HandleSystem::MakePrimHandle(HandleType_t type, 
						   QHandle **in_pHandle, 
						   unsigned int *in_index, 
						   Handle_t *in_handle)
{
	unsigned int handle;
	if (m_FreeHandles == 0)
	{
		if (m_HandleTail >= HANDLESYS_MAX_HANDLES)
		{
			return HandleError_Limit;;
		}
		handle = ++m_HandleTail;
	} else {
		handle = m_Handles[m_FreeHandles--].freeID;
	}

	QHandle *pHandle = &m_Handles[handle];
	
	assert(pHandle->set == false);

	if (++m_HSerial >= HANDLESYS_MAX_SERIALS)
	{
		m_HSerial = 1;
	}

	/* Set essential information */
	pHandle->set = true;
	pHandle->refcount = 1;
	pHandle->type = type;
	pHandle->serial = m_HSerial;

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

	return HandleError_None;
}

Handle_t HandleSystem::CreateHandle(HandleType_t type, void *object, IdentityToken_t source, IdentityToken_t ident)
{
	if (!type 
		|| type >= HANDLESYS_TYPEARRAY_SIZE
		|| m_Types[type].dispatch == NULL)
	{
		return 0;
	}

	/* Check the security of this handle */
	QHandleType *pType = &m_Types[type];
	if (!pType->sec.access[HandleAccess_Create]
		&& pType->sec.owner != ident)
	{
		return 0;
	}

	unsigned int index;
	Handle_t handle;
	QHandle *pHandle;
	HandleError err;

	if ((err=MakePrimHandle(type, &pHandle, &index, &handle)) != HandleError_None)
	{
		return 0;
	}

	pHandle->source = source;
	pHandle->object = object;
	pHandle->clone = 0;

	return handle;
}

Handle_t HandleSystem::CreateScriptHandle(HandleType_t type, 
										  void *object, 
										  sp_context_t *ctx, 
										  IdentityToken_t ident)
{
	IPlugin *pPlugin = g_PluginSys.FindPluginByContext(ctx);

	return CreateHandle(type, object, pPlugin->GetIdentity(), ident);
}

HandleError HandleSystem::GetHandle(Handle_t handle,
									IdentityToken_t ident, 
									QHandle **in_pHandle, 
									unsigned int *in_index,
									HandleAccessRight access)
{
	unsigned int serial = (handle >> 16);
	unsigned int index = (handle & HANDLESYS_HANDLE_MASK);

	if (index == 0 || index == 0xFFFF)
	{
		return HandleError_Index;
	}

	QHandle *pHandle = &m_Handles[index];
	QHandleType *pType = &m_Types[pHandle->type];

	if ((access != HandleAccess_TOTAL)
		&& (!pType->sec.access[access] && pType->sec.owner != ident))
	{
		return HandleError_Access;
	}
	if (!pHandle->set)
	{
		return HandleError_Freed;
	}
	if (pHandle->serial != serial)
	{
		return HandleError_Changed;
	}

	*in_pHandle = pHandle;
	*in_index = index;

	return HandleError_None;
}

HandleError HandleSystem::CloneHandle(Handle_t handle, Handle_t *out_newhandle, IdentityToken_t source, IdentityToken_t ident)
{
	HandleError err;
	QHandle *pHandle;
	unsigned int index;

	if ((err=GetHandle(handle, ident, &pHandle, &index, HandleAccess_Clone)) != HandleError_None)
	{
		return err;
	}

	QHandleType *pType = &m_Types[pHandle->type];

	/* Get a new Handle ID */
	unsigned int new_index;
	QHandle *pNewHandle;
	Handle_t new_handle;

	if ((err=MakePrimHandle(pHandle->type, &pNewHandle, &new_index, &new_handle)) != HandleError_None)
	{
		return err;
	}

	pNewHandle->clone = index;
	pNewHandle->source = source;

	if (out_newhandle)
	{
		*out_newhandle = new_handle;
	}

	return HandleError_None;
}

HandleError HandleSystem::FreeHandle(Handle_t handle, IdentityToken_t ident)
{
	unsigned int index;
	QHandle *pHandle;
	HandleError err;

	if ((err=GetHandle(handle, ident, &pHandle, &index, HandleAccess_Delete)) != HandleError_None)
	{
		return err;
	}

	QHandleType *pType = &m_Types[pHandle->type];

	bool dofree = false;
	if (pHandle->clone)
	{
		/* If we're a clone, there's not a lot to do. */
		if (FreeHandle(pHandle->clone, ident) != HandleError_None)
		{
			assert(false);
		}
		dofree = true;
	} else {
		if (--pHandle->refcount == 0)
		{
			dofree = true;
			pType->dispatch->OnHandleDestroy(pHandle->type, pHandle->object);
		}
	}

	if (dofree)
	{
		ReleasePrimHandle(index);
	}

	return HandleError_None;
}

inline HandleType_t TypeParent(HandleType_t type)
{
	return (type & ~HANDLESYS_SUBTYPE_MASK);
}

HandleError HandleSystem::ReadHandle(Handle_t handle, 
									 HandleType_t type, 
									 IdentityToken_t ident, 
									 void **object)
{
	unsigned int index;
	QHandle *pHandle;
	HandleError err;

	if ((err=GetHandle(handle, ident, &pHandle, &index, HandleAccess_Read)) != HandleError_None)
	{
		return err;
	}

	/* Check the type inheritance */
	if (pHandle->type & HANDLESYS_SUBTYPE_MASK)
	{
		if (pHandle->type != type
			&& (TypeParent(pHandle->type) != TypeParent(type)))
		{
			return HandleError_Type;
		}
	} else {
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

void HandleSystem::ReleasePrimHandle(unsigned int index)
{
	QHandle *pHandle = &m_Handles[index];
	
	pHandle->set = false;
	m_Types[pHandle->type].opened--;
	m_Handles[++m_FreeHandles].freeID = index;
}

bool HandleSystem::RemoveType(HandleType_t type, IdentityToken_t ident)
{
	if (type == 0 || type >= HANDLESYS_TYPEARRAY_SIZE)
	{
		return false;
	}

	QHandleType *pType = &m_Types[type];

	if (pType->sec.owner && pType->sec.owner != ident)
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
				RemoveType(type + i, childType->sec.owner);
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
		for (unsigned int i=1; i<m_HandleTail; i++)
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

	return true;
}
