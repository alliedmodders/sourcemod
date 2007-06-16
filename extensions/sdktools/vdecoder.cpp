#include "smsdk_ext.h"
#include "extension.h"
#include "vdecoder.h"

using namespace SourceMod;
using namespace SourcePawn;

/**
 * For object pointers, the data looks like this instead:
 * 4 bytes: POINTER TO NEXT FOUR BYTES
 * + bytes: Object internal data
 *
 * We use the virtual stack as extra fake stack space and create a temp object.
 * If these objects had destructors, we'd need to fake destroy toom of course.
 * Of course, BinTools only reads the first four bytes and passes the pointer.
 */

size_t ValveParamToBinParam(ValveType type, 
						  PassType pass,
						  unsigned int flags,
						  PassInfo *info)
{
	switch (type)
	{
	case Valve_Vector:
		{
			size_t mySize = sizeof(Vector *);
			if (pass == PassType_Basic)
			{
				if (info->flags & PASSFLAG_BYREF)
				{
					return 0;
				}
				info->type = PassType_Basic;
				info->flags = flags;
				info->size = sizeof(Vector *);
				mySize += sizeof(Vector);
			} else if (pass == PassType_Object) {
				info->type = PassType_Object;
				info->flags = flags | PASSFLAG_OASSIGNOP | PASSFLAG_OCTOR;
				info->size = sizeof(Vector);
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
				if (info->flags & PASSFLAG_BYREF)
				{
					return 0;
				}
				info->type = PassType_Basic;
				info->flags = flags;
				info->size = sizeof(QAngle *);
				mySize += sizeof(QAngle);
			} else if (pass == PassType_Object) {
				info->type = PassType_Object;
				info->flags = flags | PASSFLAG_OASSIGNOP | PASSFLAG_OCTOR;
				info->size = sizeof(QAngle);
			} else {
				return 0;
			}
			return mySize;
		}
	case Valve_CBaseEntity:
	case Valve_CBasePlayer:
	case Valve_POD:
	case Valve_Edict:
	case Valve_String:
		{
			if (pass != PassType_Basic
				|| (info->flags & PASSFLAG_BYREF))
			{
				return 0;
			}
			info->type = PassType_Basic;
			info->flags = flags;
			info->size = sizeof(void *);
			return sizeof(void *);
		}
	case Valve_Float:
		{
			info->type = PassType_Float;
			info->flags = flags;
			info->size = sizeof(float);
			return sizeof(float);
		}
	}

	return 0;
}

DataStatus EncodeValveParam(IPluginContext *pContext,
							cell_t param, 
							ValveType type, 
							PassType pass, 
							const void *buffer)
{
	switch (type)
	{
	case Valve_Vector:
		{
			Vector *v = NULL;

			if (pass == PassType_Basic)
			{
				v = *(Vector **)((unsigned char *)buffer + sizeof(Vector *));
			} else if (pass == PassType_Object) {
				v = (Vector *)buffer;
			}

			cell_t *addr;
			pContext->LocalToPhysAddr(param, &addr);

			addr[0] = v->x;
			addr[1] = v->y;
			addr[2] = v->z;

			return Data_Okay;
		}
	case Valve_QAngle:
		{
			QAngle *q = NULL;

			if (pass == PassType_Basic)
			{
				q = *(QAngle **)((unsigned char *)buffer + sizeof(QAngle *));
			} else if (pass == PassType_Object) {
				q = (QAngle *)buffer;
			}

			cell_t *addr;
			pContext->LocalToPhysAddr(param, &addr);

			addr[0] = q->x;
			addr[1] = q->y;
			addr[2] = q->z;

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
				edict_t *pEdict = gameents->BaseEntityToEdict(pEntity);
				*addr = engine->IndexOfEdict(pEdict);
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
				*addr = engine->IndexOfEdict(pEdict);
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

			*addr = *(cell_t *)buffer;

			return Data_Okay;
		}
	}

	return Data_Fail;
}

DataStatus DecodeValveParam(IPluginContext *pContext,
					  cell_t param,
					  ValveType vtype,
					  unsigned int vflags,
					  PassType type,
					  void *buffer)
{
	switch (vtype)
	{
	case Valve_Vector:
		{
			cell_t *addr;
			int err;
			err = pContext->LocalToPhysAddr(param, &addr);

			unsigned char *mem = (unsigned char *)buffer;
			if (type == PassType_Basic)
			{
				/* Store the object in the next N bytes, and store
				 * a pointer to that object right beforehand.
				 */
				Vector **realPtr = (Vector **)buffer;

				if (addr == pContext->GetNullRef(SP_NULL_VECTOR))
				{
					if (vflags & VDECODE_FLAG_ALLOWNULL)
					{
						*realPtr = NULL;
						return Data_Okay;
					} else {
						pContext->ThrowNativeError("NULL not allowed");
						return Data_Fail;
					}
				} else {
					mem += sizeof(Vector *);
					*realPtr = (Vector *)mem;
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
			if (type == PassType_Basic)
			{
				/* Store the object in the next N bytes, and store
				 * a pointer to that object right beforehand.
				 */
				QAngle **realPtr = (QAngle **)buffer;

				if (addr == pContext->GetNullRef(SP_NULL_VECTOR))
				{
					if (!(vflags & VDECODE_FLAG_ALLOWNULL))
					{
						pContext->ThrowNativeError("NULL not allowed");
						return Data_Fail;
					} else {
						*realPtr = NULL;
						return Data_Okay;
					}
				} else {
					mem += sizeof(QAngle *);
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
			edict_t *pEdict;
			if (vflags & VDECODE_FLAG_BYREF)
			{
				cell_t *addr;
				pContext->LocalToPhysAddr(param, &addr);
				param = *addr;
			}
			if (param >= 1 && param <= playerhelpers->GetMaxClients())
			{
				IGamePlayer *player = playerhelpers->GetGamePlayer(param);
				if ((vflags & VDECODE_FLAG_ALLOWNOTINGAME)
					&& !player->IsConnected())
				{
					pContext->ThrowNativeError("Client %d is not connected", param);
					return Data_Fail;
				} else if (!player->IsInGame()) {
					pContext->ThrowNativeError("Client %d is not in game", param);
					return Data_Fail;
				}
				pEdict = player->GetEdict();
			} else if (param == -1) {
				if (vflags & VDECODE_FLAG_ALLOWNULL)
				{
					pEdict = NULL;
				} else {
					pContext->ThrowNativeError("NULL not allowed");
					return Data_Fail;
				}
			} else if (param == 0) {
				if (vflags & VDECODE_FLAG_ALLOWWORLD)
				{
					pEdict = engine->PEntityOfEntIndex(0);
				} else {
					pContext->ThrowNativeError("World not allowed");
					return Data_Fail;
				}
			} else {
				pContext->ThrowNativeError("Entity index %d is not a valid client", param);
				return Data_Fail;
			}
			CBaseEntity *pEntity;
			if (pEdict)
			{
				IServerUnknown *pUnknown = pEdict->GetUnknown();
				if (!pUnknown)
				{
					pContext->ThrowNativeError("Entity %d is a not an IServerUnknown");
					return Data_Fail;
				}
				pEntity = pUnknown->GetBaseEntity();
				if (!pEntity)
				{
					pContext->ThrowNativeError("Entity %d is not a CBaseEntity");
					return Data_Fail;
				}
			} else {
				pEdict = NULL;
			}

			CBaseEntity **ebuf = (CBaseEntity **)buffer;
			*ebuf = pEntity;

			return Data_Okay;
		}
	case Valve_CBaseEntity:
		{
			edict_t *pEdict;
			if (vflags & VDECODE_FLAG_BYREF)
			{
				cell_t *addr;
				pContext->LocalToPhysAddr(param, &addr);
				param = *addr;
			}
			if (param >= 1 && param <= playerhelpers->GetMaxClients())
			{
				IGamePlayer *player = playerhelpers->GetGamePlayer(param);
				if ((vflags & VDECODE_FLAG_ALLOWNOTINGAME)
					&& !player->IsConnected())
				{
					pContext->ThrowNativeError("Client %d is not connected", param);
					return Data_Fail;
				} else if (!player->IsInGame()) {
					pContext->ThrowNativeError("Client %d is not in game", param);
					return Data_Fail;
				}
				pEdict = player->GetEdict();
			} else if (param == -1) {
				if (vflags & VDECODE_FLAG_ALLOWNULL)
				{
					pEdict = NULL;
				} else {
					pContext->ThrowNativeError("NULL not allowed");
					return Data_Fail;
				}
			} else if (param == 0) {
				if (vflags & VDECODE_FLAG_ALLOWWORLD)
				{
					pEdict = engine->PEntityOfEntIndex(0);
				} else {
					pContext->ThrowNativeError("World not allowed");
					return Data_Fail;
				}
			} else {
				pEdict = engine->PEntityOfEntIndex(param);
				if (!pEdict || pEdict->IsFree())
				{
					pContext->ThrowNativeError("Entity %d is not valid or is freed", param);
					return Data_Fail;
				}
			}
			CBaseEntity *pEntity;
			if (pEdict)
			{
				IServerUnknown *pUnknown = pEdict->GetUnknown();
				if (!pUnknown)
				{
					pContext->ThrowNativeError("Entity %d is a not an IServerUnknown");
					return Data_Fail;
				}
				pEntity = pUnknown->GetBaseEntity();
				if (!pEntity)
				{
					pContext->ThrowNativeError("Entity %d is not a CBaseEntity");
					return Data_Fail;
				}
			} else {
				pEdict = NULL;
			}

			CBaseEntity **ebuf = (CBaseEntity **)buffer;
			*ebuf = pEntity;

			return Data_Okay;
		}
	case Valve_Edict:
		{
			edict_t *pEdict;
			if (vflags & VDECODE_FLAG_BYREF)
			{
				cell_t *addr;
				pContext->LocalToPhysAddr(param, &addr);
				param = *addr;
			}
			if (param >= 1 && param <= playerhelpers->GetMaxClients())
			{
				IGamePlayer *player = playerhelpers->GetGamePlayer(param);
				if ((vflags & VDECODE_FLAG_ALLOWNOTINGAME)
					&& !player->IsConnected())
				{
					pContext->ThrowNativeError("Client %d is not connected", param);
					return Data_Fail;
				} else if (!player->IsInGame()) {
					pContext->ThrowNativeError("Client %d is not in game", param);
					return Data_Fail;
				}
				pEdict = player->GetEdict();
			} else if (param == -1) {
				if (vflags & VDECODE_FLAG_ALLOWNULL)
				{
					pEdict = NULL;
				} else {
					pContext->ThrowNativeError("NULL not allowed");
					return Data_Fail;
				}
			} else if (param == 0) {
				if (vflags & VDECODE_FLAG_ALLOWWORLD)
				{
					pEdict = engine->PEntityOfEntIndex(0);
				} else {
					pContext->ThrowNativeError("World not allowed");
					return Data_Fail;
				}
			} else {
				pEdict = engine->PEntityOfEntIndex(param);
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
			if (vflags & VDECODE_FLAG_BYREF)
			{
				cell_t *addr;
				pContext->LocalToPhysAddr(param, &addr);
				param = *addr;
			}
			*(cell_t *)buffer = param;
			return Data_Okay;
		}
	case Valve_String:
		{
			char *addr;
			pContext->LocalToString(param, &addr);
			*(char **)buffer = addr;
			return Data_Okay;
		}
	}

	return Data_Fail;
}
