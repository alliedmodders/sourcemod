/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod SDKTools Extension
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

#include "smsdk_ext.h"
#include "extension.h"
#include "vdecoder.h"
#include "vcallbuilder.h"

#include <IMemoryPointer.h>

using namespace SourceMod;
using namespace SourcePawn;

class ForeignMemoryPointer : public IMemoryPointer
{
public:
	ForeignMemoryPointer(void* ptr) : m_ptr(ptr)
	{
	}

	virtual void Delete() override
	{
		delete this;
	}

	virtual void* Get() override
	{
		return m_ptr;
	}

	virtual cell_t GetSize() override
	{
		return 0;
	}
protected:
	void* m_ptr;
};

/**
 * For object pointers, the data looks like this instead:
 * 4 bytes: POINTER TO LATER
 * + bytes: Object internal data
 *
 * We use the virtual stack as extra fake stack space and create a temp object.
 * If these objects had destructors, we'd need to fake destroy toom of course.
 * Of course, BinTools only reads the first four bytes and passes the pointer.
 */

size_t ValveParamToBinParam(ValveType type, 
						  PassType pass,
						  unsigned int flags,
						  PassInfo *info,
						  bool &needs_extra)
{
	needs_extra = false;
	switch (type)
	{
	case Valve_Vector:
		{
			size_t mySize = sizeof(Vector *);
			if (pass == PassType_Basic)
			{
				if (flags & PASSFLAG_BYREF)
				{
					return 0;
				}
				info->type = PassType_Basic;
				info->flags = flags;
				info->size = sizeof(Vector *);
				mySize = sizeof(Vector);
				needs_extra = true;
			} else if (pass == PassType_Object) {
				info->type = PassType_Object;
				info->flags = flags | PASSFLAG_OASSIGNOP | PASSFLAG_OCTOR;
				info->size = sizeof(Vector);
				info->fields[0] = info->fields[1] = info->fields[2] = ObjectField::Float;
				info->numFields = 3;
			} else {
				return 0;
			}
			return mySize;
		}
	case Valve_QAngle:
		{
			size_t mySize = sizeof(QAngle *);
			if (pass == PassType_Basic)
			{
				if (flags & PASSFLAG_BYREF)
				{
					return 0;
				}
				info->type = PassType_Basic;
				info->flags = flags;
				info->size = sizeof(QAngle *);
				mySize = sizeof(QAngle);
				needs_extra = true;
			} else if (pass == PassType_Object) {
				info->type = PassType_Object;
				info->flags = flags | PASSFLAG_OASSIGNOP | PASSFLAG_OCTOR;
				info->size = sizeof(QAngle);
				info->fields[0] = info->fields[1] = info->fields[2] = ObjectField::Float;
				info->numFields = 3;
			} else {
				return 0;
			}
			return mySize;
		}
	case Valve_CBaseEntity:
	case Valve_CBasePlayer:
	case Valve_Edict:
	case Valve_String:
	case Valve_Object:
		{
			if (pass != PassType_Basic || (flags & PASSFLAG_BYREF))
			{
				return 0;
			}
			info->type = PassType_Basic;
			info->flags = flags;
			info->size = sizeof(void *);
			return sizeof(void *);
		}
	case Valve_POD:
		{
			info->type = PassType_Basic;
			info->flags = flags;
			if (flags & PASSFLAG_ASPOINTER)
			{
				needs_extra = true;
				info->size = sizeof(int *);
				return sizeof(int *) + sizeof(int);
			} else {
				info->size = sizeof(int);
				return sizeof(int);
			}
		}
	case Valve_Bool:
		{
			info->type = PassType_Basic;
			info->flags = flags;
			if (flags & PASSFLAG_ASPOINTER)
			{
				needs_extra = true;
				info->size = sizeof(bool *);
				return sizeof(bool *) + sizeof(bool);
			} else {
				info->size = sizeof(bool);
				return sizeof(bool);
			}
		}
	case Valve_Float:
		{
			info->flags = flags;
			if (flags & PASSFLAG_ASPOINTER)
			{
				needs_extra = true;
				info->type = PassType_Basic;
				info->size = sizeof(float *);
				return sizeof(float *) + sizeof(float);
			} else {
				info->type = PassType_Float;
				info->size = sizeof(float);
				return sizeof(float);
			}
		}
	case Valve_MemoryPointer:
		{
			info->flags = flags;
			if (flags & PASSFLAG_ASPOINTER)
			{
				needs_extra = true;
				info->type = PassType_Basic;
				info->size = sizeof(void**);
				return sizeof(void**) + sizeof(void*);
			} else {
				info->type = PassType_Basic;
				info->size = sizeof(void*);
				return sizeof(void*);
			}
		}
	}

	return 0;
}

DataStatus EncodeValveParam(IPluginContext *pContext,
							cell_t param, 
							const ValveCall *pCall,
							const ValvePassInfo *data,
							const void *_buffer)
{
	const void *buffer = (const unsigned char *)_buffer + data->offset;
	switch (data->vtype)
	{
	case Valve_Vector:
		{
			Vector *v = NULL;

			if (data->type == PassType_Basic)
			{
				v = *(Vector **)buffer;
			} else if (data->type == PassType_Object) {
				v = (Vector *)buffer;
			}

			cell_t *addr;
			pContext->LocalToPhysAddr(param, &addr);

			addr[0] = sp_ftoc(v->x);
			addr[1] = sp_ftoc(v->y);
			addr[2] = sp_ftoc(v->z);

			return Data_Okay;
		}
	case Valve_QAngle:
		{
			QAngle *q = NULL;

			if (data->type == PassType_Basic)
			{
				q = *(QAngle **)buffer;
			} else if (data->type == PassType_Object) {
				q = (QAngle *)buffer;
			}

			cell_t *addr;
			pContext->LocalToPhysAddr(param, &addr);

			addr[0] = sp_ftoc(q->x);
			addr[1] = sp_ftoc(q->y);
			addr[2] = sp_ftoc(q->z);

			return Data_Okay;
		}
	case Valve_CBaseEntity:
	case Valve_CBasePlayer:
		{
			cell_t *addr;
			pContext->LocalToPhysAddr(param, &addr);

			CBaseEntity *pEntity = *(CBaseEntity **)buffer;
			if (pEntity)
			{
				*addr = gamehelpers->EntityToBCompatRef(pEntity);
			} else {
				*addr = -1;
			}

			return Data_Okay;
		}
	case Valve_Edict:
		{
			cell_t *addr;
			pContext->LocalToPhysAddr(param, &addr);

			edict_t *pEdict = *(edict_t **)buffer;
			if (pEdict)
			{
				*addr = IndexOfEdict(pEdict);
			} else {
				*addr = -1;
			}

			return Data_Okay;
		}
	case Valve_POD:
	case Valve_Float:
		{
			cell_t *addr;
			pContext->LocalToPhysAddr(param, &addr);

			if (data->flags & PASSFLAG_ASPOINTER)
			{
				buffer = *(cell_t **)buffer;
			}

			*addr = *(cell_t *)buffer;

			return Data_Okay;
		}
	case Valve_Bool:
		{
			cell_t *addr;
			pContext->LocalToPhysAddr(param, &addr);

			if (data->flags & PASSFLAG_ASPOINTER)
			{
				buffer = *(bool **)buffer;
			}

			*addr = *(bool *)buffer ? 1 : 0;

			return Data_Okay;
		}
	case Valve_MemoryPointer:
		{
			cell_t *addr;
			pContext->LocalToPhysAddr(param, &addr);

			if (data->flags & PASSFLAG_ASPOINTER)
			{
				buffer = *(void ***)buffer;
			}

			auto ptr = *(void **)buffer;

			if (ptr == nullptr)
			{
				*addr = 0;
				return Data_Okay;
			}

			HandleError err = HandleError_None;
			Handle_t hndl = handlesys->CreateHandle(g_MemPtrHandle, new ForeignMemoryPointer(ptr), pContext->GetIdentity(), myself->GetIdentity(), &err);

			if (err != HandleError_None)
			{
				pContext->ThrowNativeError("Failed to create MemoryPointer while decoding (error: %d)", err);
				return Data_Fail;
			}

			*addr = hndl;

			return Data_Okay;
		}
	}

	return Data_Fail;
}

DataStatus DecodeValveParam(IPluginContext *pContext,
					  cell_t param,
					  const ValveCall *pCall,
					  const ValvePassInfo *data,
					  void *_buffer)
{
	void *buffer = (unsigned char *)_buffer + data->offset;
	switch (data->vtype)
	{
	case Valve_Vector:
		{
			cell_t *addr;
			pContext->LocalToPhysAddr(param, &addr);

			unsigned char *mem = (unsigned char *)buffer;
			if (data->type == PassType_Basic)
			{
				/* Store the object in the next N bytes, and store
				 * a pointer to that object right beforehand.
				 */
				Vector **realPtr = (Vector **)buffer;

				if (addr == pContext->GetNullRef(SP_NULL_VECTOR))
				{
					if (data->decflags & VDECODE_FLAG_ALLOWNULL)
					{
						*realPtr = NULL;
						return Data_Okay;
					} else {
						pContext->ThrowNativeError("NULL not allowed");
						return Data_Fail;
					}
				} else {
					mem = (unsigned char *)_buffer + pCall->stackEnd + data->obj_offset;
					*realPtr = (Vector *)mem;
				}
			}

			/* Use placement new to initialize the object cleanly
			 * This has no destructor so we don't need to do 
			 * DestroyValveParam() or something :]
			 */
			Vector *v = new (mem) Vector(
				sp_ctof(addr[0]),
				sp_ctof(addr[1]),
				sp_ctof(addr[2]));

			return Data_Okay;
		}
	case Valve_QAngle:
		{
			cell_t *addr;
			int err;
			err = pContext->LocalToPhysAddr(param, &addr);

			unsigned char *mem = (unsigned char *)buffer;
			if (data->type == PassType_Basic)
			{
				/* Store the object in the next N bytes, and store
				 * a pointer to that object right beforehand.
				 */
				QAngle **realPtr = (QAngle **)buffer;

				if (addr == pContext->GetNullRef(SP_NULL_VECTOR))
				{
					if (!(data->decflags & VDECODE_FLAG_ALLOWNULL))
					{
						pContext->ThrowNativeError("NULL not allowed");
						return Data_Fail;
					} else {
						*realPtr = NULL;
						return Data_Okay;
					}
				} else {
					mem = (unsigned char *)_buffer + pCall->stackEnd + data->obj_offset;
					*realPtr = (QAngle *)mem;
				}
			}

			if (err != SP_ERROR_NONE)
			{
				pContext->ThrowNativeErrorEx(err, "Could not read plugin data");
				return Data_Fail;
			}

			/* Use placement new to initialize the object cleanly
			 * This has no destructor so we don't need to do 
			 * DestroyValveParam() or something :]
			 */
			QAngle *v = new (mem) QAngle(
				sp_ctof(addr[0]),
				sp_ctof(addr[1]),
				sp_ctof(addr[2]));

			return Data_Okay;
		}
	case Valve_CBasePlayer:
		{
			CBaseEntity *pEntity = NULL;
			if (data->decflags & VDECODE_FLAG_BYREF)
			{
				cell_t *addr;
				pContext->LocalToPhysAddr(param, &addr);
				param = *addr;
			}
			int index = gamehelpers->ReferenceToIndex(param);
			if ((unsigned)index == INVALID_EHANDLE_INDEX && param != -1)
			{
				return Data_Fail;
			}
			if (index >= 1 && index <= playerhelpers->GetMaxClients())
			{
				IGamePlayer *player = playerhelpers->GetGamePlayer(index);

				if(!player->IsConnected()) {
					pContext->ThrowNativeError("Client %d is not connected", param);
					return Data_Fail;
				}

				if(!(data->decflags & VDECODE_FLAG_ALLOWNOTINGAME) && !player->IsInGame()) {
					pContext->ThrowNativeError("Client %d is not in game", param);
					return Data_Fail;
				}

				pEntity = gamehelpers->ReferenceToEntity(param);
			} else if (param == -1) {
				if (data->decflags & VDECODE_FLAG_ALLOWNULL)
				{
					pEntity = NULL;
				} else {
					pContext->ThrowNativeError("NULL not allowed");
					return Data_Fail;
				}
			} else {
				pContext->ThrowNativeError("Entity index %d is not a valid client", param);
				return Data_Fail;
			}
			
			CBaseEntity **ebuf = (CBaseEntity **)buffer;
			*ebuf = pEntity;

			return Data_Okay;
		}
	case Valve_CBaseEntity:
		{
			CBaseEntity *pEntity = NULL;
			if (data->decflags & VDECODE_FLAG_BYREF)
			{
				cell_t *addr;
				pContext->LocalToPhysAddr(param, &addr);
				param = *addr;
			}
			int index = gamehelpers->ReferenceToIndex(param);
			if ((unsigned)index == INVALID_EHANDLE_INDEX && param != -1)
			{
				return Data_Fail;
			}
			if (index >= 1 && index <= playerhelpers->GetMaxClients())
			{
				IGamePlayer *player = playerhelpers->GetGamePlayer(index);

				if(!player->IsConnected()) {
					pContext->ThrowNativeError("Client %d is not connected", param);
					return Data_Fail;
				}

				if(!(data->decflags & VDECODE_FLAG_ALLOWNOTINGAME) && !player->IsInGame()) {
					pContext->ThrowNativeError("Client %d is not in game", param);
					return Data_Fail;
				}

				pEntity = gamehelpers->ReferenceToEntity(param);
			} else if (param == -1) {
				if (data->decflags & VDECODE_FLAG_ALLOWNULL)
				{
					pEntity = NULL;
				} else {
					pContext->ThrowNativeError("NULL not allowed");
					return Data_Fail;
				}
			} else if (index == 0) {
				if (data->decflags & VDECODE_FLAG_ALLOWWORLD)
				{
					pEntity = gamehelpers->ReferenceToEntity(0);
				} else {
					pContext->ThrowNativeError("World not allowed");
					return Data_Fail;
				}
			} else {
				pEntity = gamehelpers->ReferenceToEntity(param);
				if (!pEntity)
				{
					pContext->ThrowNativeError("Entity %d is not valid", param);
					return Data_Fail;
				}
			}

			CBaseEntity **ebuf = (CBaseEntity **)buffer;
			*ebuf = pEntity;

			return Data_Okay;
		}
	case Valve_Edict:
		{
			edict_t *pEdict;
			if (data->decflags & VDECODE_FLAG_BYREF)
			{
				cell_t *addr;
				pContext->LocalToPhysAddr(param, &addr);
				param = *addr;
			}
			if (param >= 1 && param <= playerhelpers->GetMaxClients())
			{
				IGamePlayer *player = playerhelpers->GetGamePlayer(param);

				if(!player->IsConnected()) {
					pContext->ThrowNativeError("Client %d is not connected", param);
					return Data_Fail;
				}

				if(!(data->decflags & VDECODE_FLAG_ALLOWNOTINGAME) && !player->IsInGame()) {
					pContext->ThrowNativeError("Client %d is not in game", param);
					return Data_Fail;
				}

				pEdict = player->GetEdict();
			} else if (param == -1) {
				if (data->decflags & VDECODE_FLAG_ALLOWNULL)
				{
					pEdict = NULL;
				} else {
					pContext->ThrowNativeError("NULL not allowed");
					return Data_Fail;
				}
			} else if (param == 0) {
				if (data->decflags & VDECODE_FLAG_ALLOWWORLD)
				{
					pEdict = PEntityOfEntIndex(0);
				} else {
					pContext->ThrowNativeError("World not allowed");
					return Data_Fail;
				}
			} else {
				pEdict = PEntityOfEntIndex(param);
				if (!pEdict || pEdict->IsFree())
				{
					pContext->ThrowNativeError("Entity %d is not valid or is freed", param);
					return Data_Fail;
				}
			}

			edict_t **ebuf = (edict_t **)buffer;
			*ebuf = pEdict;

			return Data_Okay;
		}
	case Valve_POD:
	case Valve_Float:
		{
			if (data->decflags & VDECODE_FLAG_BYREF)
			{
				cell_t *addr;
				pContext->LocalToPhysAddr(param, &addr);
				param = *addr;
			}
			if (data->flags & PASSFLAG_ASPOINTER)
			{
				*(void **)buffer = (unsigned char *)_buffer + pCall->stackEnd + data->obj_offset;
				buffer = *(void **)buffer;
			}
			*(cell_t *)buffer = param;
			return Data_Okay;
		}
	case Valve_Bool:
		{
			if (data->decflags & VDECODE_FLAG_BYREF)
			{
				cell_t *addr;
				pContext->LocalToPhysAddr(param, &addr);
				param = *addr;
			}
			if (data->flags & PASSFLAG_ASPOINTER)
			{
				*(bool **)buffer = (bool *)((unsigned char *)_buffer + pCall->stackEnd + data->obj_offset);
				buffer = *(bool **)buffer;
			}
			*(bool *)buffer = param ? true : false;
			return Data_Okay;
		}
	case Valve_String:
		{
			char *addr;
			pContext->LocalToString(param, &addr);
			*(char **)buffer = addr;
			return Data_Okay;
		}
	case Valve_MemoryPointer:
		{
			IMemoryPointer* ptr = nullptr;

			HandleSecurity security;
			security.pIdentity = myself->GetIdentity();
			security.pOwner = pContext->GetIdentity();

			cell_t* addr;
			pContext->LocalToPhysAddr(param, &addr);
			Handle_t hndl = (Handle_t)*addr;

			if (hndl == 0)
			{
				if (data->decflags & VDECODE_FLAG_ALLOWNULL)
				{
					*(void **)buffer = nullptr;
					return Data_Okay;
				}
				pContext->ThrowNativeError("Null/Invalid Handle MemoryPointer isn't allowed");
				return Data_Fail;
			}

			HandleError err = HandleError_None;
			if ((err = handlesys->ReadHandle(hndl, g_MemPtrHandle, &security, (void **)&ptr)) != HandleError_None)
			{
				pContext->ThrowNativeError("Could not read MemoryPointer Handle %x (error %d)", hndl, err);
				return Data_Fail;
			}

			*(void **)buffer = ptr->Get();
			return Data_Okay;
		}
	}

	return Data_Fail;
}
