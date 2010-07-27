/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2010 AlliedModders LLC.  All rights reserved.
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

#include "sm_globals.h"
#include "sourcemod.h"
#include "sourcemm_api.h"
#include "PlayerManager.h"
#include "HalfLife2.h"
#include <IGameConfigs.h>
#include "sm_stringutil.h"
#include "logic_bridge.h"

enum PropType
{
	Prop_Send = 0,
	Prop_Data
};

enum PropFieldType
{
	PropField_Unsupported,		/**< The type is unsupported. */
	PropField_Integer,			/**< Valid for SendProp and Data fields */
	PropField_Float,			/**< Valid for SendProp and Data fields */
	PropField_Entity,			/**< Valid for Data fields only (SendProp shows as int) */
	PropField_Vector,			/**< Valid for SendProp and Data fields */
	PropField_String,			/**< Valid for SendProp and Data fields */
	PropField_String_T,			/**< Valid for Data fields.  Read only! */
};

inline edict_t *BaseEntityToEdict(CBaseEntity *pEntity)
{
	IServerUnknown *pUnk = (IServerUnknown *)pEntity;
	IServerNetworkable *pNet = pUnk->GetNetworkable();

	if (!pNet)
	{
		return NULL;
	}

	return pNet->GetEdict();
}

inline edict_t *GetEdict(cell_t num)
{
	edict_t *pEdict;
	if (!IndexToAThings(num, NULL, &pEdict))
	{
		return NULL;
	}
	
	return pEdict;
}

inline CBaseEntity *GetEntity(cell_t num)
{
	CBaseEntity *pEntity;
	if (!IndexToAThings(num, &pEntity, NULL))
	{
		return NULL;
	}

	return pEntity;
}

/* Given an entity reference or index, fill out a CBaseEntity and/or edict.
   If lookup is successful, returns true and writes back the two parameters.
   If lookup fails, returns false and doesn't touch the params.  */
bool IndexToAThings(cell_t num, CBaseEntity **pEntData, edict_t **pEdictData)
{
	CBaseEntity *pEntity = g_HL2.ReferenceToEntity(num);

	if (!pEntity)
	{
		return false;
	}

	int index = g_HL2.ReferenceToIndex(num);
	if (index > 0 && index <= g_Players.GetMaxClients())
	{
		CPlayer *pPlayer = g_Players.GetPlayerByIndex(index);
		if (!pPlayer || !pPlayer->IsConnected())
		{
			return false;
		}
	}

	if (pEntData)
	{
		*pEntData = pEntity;
	}

	if (pEdictData)
	{
		edict_t *pEdict = ::BaseEntityToEdict(pEntity);
		if (!pEdict || pEdict->IsFree())
		{
			pEdict = NULL;
		}

		*pEdictData = pEdict;
	}

	return true;
}
/*
  <predcrab> AThings is like guaranteed r+ by dvander though
  <predcrab> he wont even read the rest of the patch to nit stuff
  <Fyren> Like I wasn't going to r? you anyway!
  <predcrab> he still sees!
  <predcrab> he's everywhere
*/

class VEmptyClass {};
datamap_t *VGetDataDescMap(CBaseEntity *pThisPtr, int offset)
{
	void **this_ptr = *reinterpret_cast<void ***>(&pThisPtr);
	void **vtable = *reinterpret_cast<void ***>(pThisPtr);
	void *vfunc = vtable[offset];

	union
	{
		datamap_t *(VEmptyClass::*mfpnew)();
#ifndef PLATFORM_POSIX
		void *addr;
	} u;
	u.addr = vfunc;
#else
		struct  
		{
			void *addr;
			intptr_t adjustor;
		} s;
	} u;
	u.s.addr = vfunc;
	u.s.adjustor = 0;
#endif

	return (datamap_t *)(reinterpret_cast<VEmptyClass *>(this_ptr)->*u.mfpnew)();
}

inline datamap_t *CBaseEntity_GetDataDescMap(CBaseEntity *pEntity)
{
	int offset;

	if (!g_pGameConf->GetOffset("GetDataDescMap", &offset) || !offset)
	{
		return NULL;
	}

	return VGetDataDescMap(pEntity, offset);
}

/* :TODO: Move this! */
datamap_t *CHalfLife2::GetDataMap(CBaseEntity *pEntity)
{
	return CBaseEntity_GetDataDescMap(pEntity);
}

static cell_t GetMaxEntities(IPluginContext *pContext, const cell_t *params)
{
	return gpGlobals->maxEntities;
}

static cell_t CreateEdict(IPluginContext *pContext, const cell_t *params)
{
	edict_t *pEdict = engine->CreateEdict();

	if (!pEdict)
	{
		return 0;
	}

	return IndexOfEdict(pEdict);
}

static cell_t RemoveEdict(IPluginContext *pContext, const cell_t *params)
{
	edict_t *pEdict = GetEdict(params[1]);

	if (!pEdict)
	{
		return pContext->ThrowNativeError("Edict %d (%d) is not a valid edict", g_HL2.ReferenceToIndex(params[1]), params[1]);
	}

	engine->RemoveEdict(pEdict);

	return 1;
}

static cell_t IsValidEdict(IPluginContext *pContext, const cell_t *params)
{
	edict_t *pEdict = GetEdict(params[1]);

	if (!pEdict)
	{
		return 0;
	}

	/* Shouldn't be necessary... not sure though */
	return pEdict->IsFree() ? 0 : 1;
}

static cell_t IsValidEntity(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity = g_HL2.ReferenceToEntity(params[1]);

	return (pEntity != NULL) ? 1 : 0;
}

static cell_t IsEntNetworkable(IPluginContext *pContext, const cell_t *params)
{
	edict_t *pEdict = GetEdict(params[1]);

	if (!pEdict)
	{
		return 0;
	}

	return (pEdict->GetNetworkable() != NULL) ? 1 : 0;
}

static cell_t GetEntityCount(IPluginContext *pContext, const cell_t *params)
{
	return engine->GetEntityCount();
}

static cell_t GetEdictFlags(IPluginContext *pContext, const cell_t *params)
{
	edict_t *pEdict = GetEdict(params[1]);

	if (!pEdict)
	{
		return pContext->ThrowNativeError("Invalid edict (%d - %d)", g_HL2.ReferenceToIndex(params[1]), params[1]);
	}

	return pEdict->m_fStateFlags;
}

static cell_t SetEdictFlags(IPluginContext *pContext, const cell_t *params)
{
	edict_t *pEdict = GetEdict(params[1]);

	if (!pEdict)
	{
		return pContext->ThrowNativeError("Invalid edict (%d - %d)", g_HL2.ReferenceToIndex(params[1]), params[1]);
	}

	pEdict->m_fStateFlags = params[2];

	return 1;
}

static cell_t GetEdictClassname(IPluginContext *pContext, const cell_t *params)
{
	edict_t *pEdict = GetEdict(params[1]);

	if (!pEdict)
	{
		return pContext->ThrowNativeError("Invalid edict (%d - %d)", g_HL2.ReferenceToIndex(params[1]), params[1]);
	}

	const char *cls = pEdict->GetClassName();

	if (!cls || cls[0] == '\0')
	{
		return 0;
	}

	pContext->StringToLocal(params[2], params[3], cls);

	return 1;
}

static cell_t GetEntityNetClass(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity = g_HL2.ReferenceToEntity(params[1]);

	IServerUnknown *pUnk = (IServerUnknown *)pEntity;

	if (!pEntity)
	{
		return pContext->ThrowNativeError("Invalid entity (%d - %d)", g_HL2.ReferenceToIndex(params[1]), params[1]);
	}

	IServerNetworkable *pNet = pUnk->GetNetworkable();
	if (!pNet)
	{
		return 0;
	}

	ServerClass *pClass = pNet->GetServerClass();

	pContext->StringToLocal(params[2], params[3], pClass->GetName());

	return 1;
}

static cell_t GetEntData(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity = GetEntity(params[1]);

	if (!pEntity)
	{
		return pContext->ThrowNativeError("Entity %d (%d) is invalid", g_HL2.ReferenceToIndex(params[1]), params[1]);
	}

	int offset = params[2];
	if (offset <= 0 || offset > 32768)
	{
		return pContext->ThrowNativeError("Offset %d is invalid", offset);
	}

	switch (params[3])
	{
	case 4:
		return *(int *)((uint8_t *)pEntity + offset);
	case 2:
		return *(short *)((uint8_t *)pEntity + offset);
	case 1:
		return *((uint8_t *)pEntity + offset);
	default:
		return pContext->ThrowNativeError("Integer size %d is invalid", params[3]);
	}
}

static cell_t SetEntData(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity;
	edict_t *pEdict;

	if (!IndexToAThings(params[1], &pEntity, &pEdict))
	{
		return pContext->ThrowNativeError("Entity %d (%d) is invalid", g_HL2.ReferenceToIndex(params[1]), params[1]);
	}

	int offset = params[2];
	if (offset <= 0 || offset > 32768)
	{
		return pContext->ThrowNativeError("Offset %d is invalid", offset);
	}

	if (params[5] && (pEdict != NULL))
	{
		g_HL2.SetEdictStateChanged(pEdict, offset);
	}

	switch (params[4])
	{
	case 4:
		{
			*(int *)((uint8_t *)pEntity + offset) = params[3];
			break;
		}
	case 2:
		{
			*(short *)((uint8_t *)pEntity + offset) = params[3];
			break;
		}
	case 1:
		{
			*((uint8_t *)pEntity + offset) = params[3];
			break;
		}
	default:
		return pContext->ThrowNativeError("Integer size %d is invalid", params[4]);
	}

	return 1;
}

static cell_t GetEntDataFloat(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity = GetEntity(params[1]);

	if (!pEntity)
	{
		return pContext->ThrowNativeError("Entity %d (%d) is invalid", g_HL2.ReferenceToIndex(params[1]), params[1]);
	}

	int offset = params[2];
	if (offset <= 0 || offset > 32768)
	{
		return pContext->ThrowNativeError("Offset %d is invalid", offset);
	}

	float f = *(float *)((uint8_t *)pEntity + offset);

	return sp_ftoc(f);
}

static cell_t SetEntDataFloat(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity;
	edict_t *pEdict;

	if (!IndexToAThings(params[1], &pEntity, &pEdict))
	{
		return pContext->ThrowNativeError("Entity %d (%d) is invalid", g_HL2.ReferenceToIndex(params[1]), params[1]);
	}

	int offset = params[2];
	if (offset <= 0 || offset > 32768)
	{
		return pContext->ThrowNativeError("Offset %d is invalid", offset);
	}

	*(float *)((uint8_t *)pEntity + offset) = sp_ctof(params[3]);

	if (params[4] && (pEdict != NULL))
	{
		g_HL2.SetEdictStateChanged(pEdict, offset);
	}

	return 1;
}

static cell_t GetEntDataVector(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity = GetEntity(params[1]);

	if (!pEntity)
	{
		return pContext->ThrowNativeError("Entity %d (%d) is invalid", g_HL2.ReferenceToIndex(params[1]), params[1]);
	}

	int offset = params[2];
	if (offset <= 0 || offset > 32768)
	{
		return pContext->ThrowNativeError("Offset %d is invalid", offset);
	}

	Vector *v = (Vector *)((uint8_t *)pEntity + offset);

	cell_t *vec;
	pContext->LocalToPhysAddr(params[3], &vec);

	vec[0] = sp_ftoc(v->x);
	vec[1] = sp_ftoc(v->y);
	vec[2] = sp_ftoc(v->z);

	return 1;
}

static cell_t SetEntDataVector(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity;
	edict_t *pEdict;

	if (!IndexToAThings(params[1], &pEntity, &pEdict))
	{
		return pContext->ThrowNativeError("Entity %d (%d) is invalid", g_HL2.ReferenceToIndex(params[1]), params[1]);
	}

	int offset = params[2];
	if (offset <= 0 || offset > 32768)
	{
		return pContext->ThrowNativeError("Offset %d is invalid", offset);
	}

	Vector *v = (Vector *)((uint8_t *)pEntity + offset);

	cell_t *vec;
	pContext->LocalToPhysAddr(params[3], &vec);

	v->x = sp_ctof(vec[0]);
	v->y = sp_ctof(vec[1]);
	v->z = sp_ctof(vec[2]);

	if (params[4] && (pEdict != NULL))
	{
		g_HL2.SetEdictStateChanged(pEdict, offset);
	}

	return 1;
}

/* THIS GUY IS DEPRECATED. */
static cell_t GetEntDataEnt(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity = GetEntity(params[1]);

	if (!pEntity)
	{
		return pContext->ThrowNativeError("Entity %d (%d) is invalid", g_HL2.ReferenceToIndex(params[1]), params[1]);
	}

	int offset = params[2];
	if (offset <= 0 || offset > 32768)
	{
		return pContext->ThrowNativeError("Offset %d is invalid", offset);
	}

	CBaseHandle &hndl = *(CBaseHandle *)((uint8_t *)pEntity + offset);

	if (!hndl.IsValid())
	{
		return 0;
	}

	int ref = g_HL2.IndexToReference(hndl.GetEntryIndex());
	return g_HL2.ReferenceToBCompatRef(ref);
}

int CheckBaseHandle(CBaseHandle &hndl)
{
	if (!hndl.IsValid())
	{
		return -1;
	}

	int index = hndl.GetEntryIndex();

	edict_t *pStoredEdict;
	CBaseEntity *pStoredEntity;

	if (!IndexToAThings(index, &pStoredEntity, &pStoredEdict))
	{
		return -1;
	}

	if (pStoredEdict == NULL || pStoredEntity == NULL)
	{
		return -1;
	}

	IServerEntity *pSE = pStoredEdict->GetIServerEntity();

	if (pSE == NULL)
	{
		return -1;
	}

	if (pSE->GetRefEHandle() != hndl)
	{
		return -1;
	}

	return index;
}

static cell_t GetEntDataEnt2(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity = GetEntity(params[1]);

	if (pEntity == NULL)
	{
		return pContext->ThrowNativeError("Entity %d (%d) is invalid", g_HL2.ReferenceToIndex(params[1]), params[1]);
	}

	int offset = params[2];
	if (offset <= 0 || offset > 32768)
	{
		return pContext->ThrowNativeError("Offset %d is invalid", offset);
	}

	CBaseHandle &hndl = *(CBaseHandle *)((uint8_t *)pEntity + offset);

	int ref = g_HL2.IndexToReference(hndl.GetEntryIndex());
	return g_HL2.ReferenceToBCompatRef(ref);
}

/* THIS GUY IS DEPRECATED. */
static cell_t SetEntDataEnt(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity;
	edict_t *pEdict;

	if (!IndexToAThings(params[1], &pEntity, &pEdict))
	{
		return pContext->ThrowNativeError("Entity %d (%d) is invalid", g_HL2.ReferenceToIndex(params[1]), params[1]);
	}

	int offset = params[2];
	if (offset <= 0 || offset > 32768)
	{
		return pContext->ThrowNativeError("Offset %d is invalid", offset);
	}

	CBaseHandle &hndl = *(CBaseHandle *)((uint8_t *)pEntity + offset);

	if (params[3] == 0 || (unsigned)params[3] == INVALID_EHANDLE_INDEX)
	{
		hndl.Set(NULL);
	} else {
		CBaseEntity *pOther = GetEntity(params[3]);

		if (!pOther)
		{
			return pContext->ThrowNativeError("Entity %d (%d) is invalid", g_HL2.ReferenceToIndex(params[3]), params[3]);
		}

		IHandleEntity *pHandleEnt = (IHandleEntity *)pOther;
		hndl.Set(pHandleEnt);
	}

	if (params[4] && (pEdict != NULL))
	{
		g_HL2.SetEdictStateChanged(pEdict, offset);
	}

	return 1;
}

static cell_t SetEntDataEnt2(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity;
	edict_t *pEdict;

	if (!IndexToAThings(params[1], &pEntity, &pEdict))
	{
		return pContext->ThrowNativeError("Entity %d (%d) is invalid", g_HL2.ReferenceToIndex(params[1]), params[1]);
	}

	int offset = params[2];
	if (offset <= 0 || offset > 32768)
	{
		return pContext->ThrowNativeError("Offset %d is invalid", offset);
	}

	CBaseHandle &hndl = *(CBaseHandle *)((uint8_t *)pEntity + offset);

	if ((unsigned)params[3] == INVALID_EHANDLE_INDEX)
	{
		hndl.Set(NULL);
	}
	else
	{
		CBaseEntity *pOther = GetEntity(params[3]);

		if (!pOther)
		{
			return pContext->ThrowNativeError("Entity %d (%d) is invalid", g_HL2.ReferenceToIndex(params[3]), params[3]);
		}

		IHandleEntity *pHandleEnt = (IHandleEntity *)pOther;
		hndl.Set(pHandleEnt);
	}

	if (params[4] && (pEdict != NULL))
	{
		g_HL2.SetEdictStateChanged(pEdict, offset);
	}

	return 1;
}

static cell_t ChangeEdictState(IPluginContext *pContext, const cell_t *params)
{
	edict_t *pEdict = GetEdict(params[1]);

	if (!pEdict)
	{
		return pContext->ThrowNativeError("Edict %d (%d) is invalid", g_HL2.ReferenceToIndex(params[1]), params[1]);
	}

	g_HL2.SetEdictStateChanged(pEdict, params[2]);

	return 1;
}

static cell_t FindSendPropOffs(IPluginContext *pContext, const cell_t *params)
{
	char *cls, *prop;
	pContext->LocalToString(params[1], &cls);
	pContext->LocalToString(params[2], &prop);

	SendProp *pSend = g_HL2.FindInSendTable(cls, prop);

	if (!pSend)
	{
		return -1;
	}

	return pSend->GetOffset();
}

static cell_t FindSendPropInfo(IPluginContext *pContext, const cell_t *params)
{
	char *cls, *prop;
	sm_sendprop_info_t info;
	cell_t *pType, *pBits, *pLocal;

	pContext->LocalToString(params[1], &cls);
	pContext->LocalToString(params[2], &prop);

	if (!g_HL2.FindSendPropInfo(cls, prop, &info))
	{
		return -1;
	}

	pContext->LocalToPhysAddr(params[3], &pType);
	pContext->LocalToPhysAddr(params[4], &pBits);
	pContext->LocalToPhysAddr(params[5], &pLocal);

	switch (info.prop->GetType())
	{
	case DPT_Int:
		{
			*pType = PropField_Integer;
			break;
		}
	case DPT_Float:
		{
			*pType = PropField_Float;
			break;
		}
	case DPT_String:
		{
			*pType = PropField_String;
			break;
		}
	case DPT_Vector:
		{
			*pType = PropField_Vector;
			break;
		}
	default:
		{
			*pType = PropField_Unsupported;
			break;
		}
	}

	*pBits = info.prop->m_nBits;
	*pLocal = info.prop->GetOffset();

	return info.actual_offset;
}

static cell_t FindDataMapOffs(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity;
	datamap_t *pMap;
	typedescription_t *td;
	char *offset;
	pEntity = GetEntity(params[1]);

	if (!pEntity)
	{
		return pContext->ThrowNativeError("Entity %d (%d) is invalid", g_HL2.ReferenceToIndex(params[1]), params[1]);
	}

	if ((pMap=CBaseEntity_GetDataDescMap(pEntity)) == NULL)
	{
		return pContext->ThrowNativeError("Unable to retrieve GetDataDescMap offset");
	}

	pContext->LocalToString(params[2], &offset);
	if ((td=g_HL2.FindInDataMap(pMap, offset)) == NULL)
	{
		return -1;
	}

	if (params[0] == 4)
	{
		cell_t *pType, *pSize;

		pContext->LocalToPhysAddr(params[3], &pType);
		pContext->LocalToPhysAddr(params[4], &pSize);

		switch (td->fieldType)
		{
		case FIELD_TICK:
		case FIELD_MODELINDEX:
		case FIELD_MATERIALINDEX:
		case FIELD_INTEGER:
		case FIELD_COLOR32:
			{
				*pType = PropField_Integer;
				*pSize = 32;
				break;
			}
		case FIELD_VECTOR:
		case FIELD_POSITION_VECTOR:
			{
				*pType = PropField_Vector;
				*pSize = 12;
				break;
			}
		case FIELD_SHORT:
			{
				*pType = PropField_Integer;
				*pSize = 16;
				break;
			}
		case FIELD_BOOLEAN:
			{
				*pType = PropField_Integer;
				*pSize = 1;
				break;
			}
		case FIELD_CHARACTER:
			{
				if (td->fieldSize == 1)
				{
					*pType = PropField_Integer;
					*pSize = 8;
				}
				else
				{
					*pType = PropField_String;
					*pSize = 8 * td->fieldSize;
				}
				break;
			}
		case FIELD_MODELNAME:
		case FIELD_SOUNDNAME:
		case FIELD_STRING:
			{
				*pSize = sizeof(string_t);
				*pType = PropField_String_T;
				break;
			}
		case FIELD_FLOAT:
		case FIELD_TIME:
			{
				*pType = PropField_Float;
				*pSize = 32;
				break;
			}
		case FIELD_EHANDLE:
			{
				*pType = PropField_Entity;
				*pSize = 32;
				break;
			}
		default:
			{
				*pType = PropField_Unsupported;
				*pSize = 0;
			}
		}
	}

	return GetTypeDescOffs(td);
}

static cell_t GetEntDataString(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity = GetEntity(params[1]);

	if (!pEntity)
	{
		return pContext->ThrowNativeError("Entity %d (%d) is invalid", g_HL2.ReferenceToIndex(params[1]), params[1]);
	}

	int offset = params[2];
	if (offset < 0 || offset > 32768)
	{
		return pContext->ThrowNativeError("Offset %d is invalid", offset);
	}

	size_t len;
	char *src = (char *)((uint8_t *)pEntity + offset);
	pContext->StringToLocalUTF8(params[3], params[4], src, &len);

	return len;
}

static cell_t SetEntDataString(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity;
	edict_t *pEdict;

	if (!IndexToAThings(params[1], &pEntity, &pEdict))
	{
		return pContext->ThrowNativeError("Entity %d (%d) is invalid", g_HL2.ReferenceToIndex(params[1]), params[1]);
	}

	int offset = params[2];
	if (offset < 0 || offset > 32768)
	{
		return pContext->ThrowNativeError("Offset %d is invalid", offset);
	}

	char *src;
	char *dest = (char *)((uint8_t *)pEntity + offset);

	pContext->LocalToString(params[3], &src);
	size_t len = strncopy(dest, src, params[4]);

	if (params[5] && (pEdict != NULL))
	{
		g_HL2.SetEdictStateChanged(pEdict, offset);
	}

	return len;
}

#define FIND_PROP_DATA(td) \
	datamap_t *pMap; \
	if ((pMap = CBaseEntity_GetDataDescMap(pEntity)) == NULL) \
	{ \
		return pContext->ThrowNativeError("Could not retrieve datamap"); \
	} \
	if ((td = g_HL2.FindInDataMap(pMap, prop)) == NULL) \
	{ \
		return pContext->ThrowNativeError("Property \"%s\" not found (entity %d/%s)", \
			prop, \
			params[1], \
			class_name); \
	}

#define FIND_PROP_SEND(pProp) \
	IServerUnknown *pUnk = (IServerUnknown *)pEntity; \
	IServerNetworkable *pNet = pUnk->GetNetworkable(); \
	if (!pNet) \
	{ \
		return pContext->ThrowNativeError("Edict %d (%d) is not networkable", g_HL2.ReferenceToIndex(params[1]), params[1]); \
	} \
	if (!g_HL2.FindSendPropInfo(pNet->GetServerClass()->GetName(), prop, pProp)) \
	{ \
		return pContext->ThrowNativeError("Property \"%s\" not found (entity %d/%s)", \
			prop, \
			params[1], \
			class_name); \
	}

inline int MatchFieldAsInteger(int field_type)
{
	switch (field_type)
	{
	case FIELD_TICK:
	case FIELD_MODELINDEX:
	case FIELD_MATERIALINDEX:
	case FIELD_INTEGER:
	case FIELD_COLOR32:
		return 32;
	case FIELD_SHORT:
		return 16;
	case FIELD_CHARACTER:
		return 8;
	case FIELD_BOOLEAN:
		return 1;
	default:
		return 0;
	}

	return 0;
}

static cell_t GetEntProp(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity;
	char *prop;
	int offset;
	const char *class_name;
	edict_t *pEdict;
	int bit_count;

	if (!IndexToAThings(params[1], &pEntity, &pEdict))
	{
		return pContext->ThrowNativeError("Entity %d (%d) is invalid", g_HL2.ReferenceToIndex(params[1]), params[1]);
	}

	/* TODO: Find a way to lookup classname without having an edict - Is this a guaranteed prop? */
	if (!pEdict || (class_name = pEdict->GetClassName()) == NULL)
	{
		class_name = "";
	}

	pContext->LocalToString(params[3], &prop);

	switch (params[2])
	{
	case Prop_Data:
		{
			typedescription_t *td;

			FIND_PROP_DATA(td);

			if ((bit_count = MatchFieldAsInteger(td->fieldType)) == 0)
			{
				return pContext->ThrowNativeError("Data field %s is not an integer (%d)", 
					prop,
					td->fieldType);
			}

			offset = GetTypeDescOffs(td);
			break;
		}
	case Prop_Send:
		{
			sm_sendprop_info_t info;

			FIND_PROP_SEND(&info);

			if (info.prop->GetType() != DPT_Int)
			{
				return pContext->ThrowNativeError("SendProp %s is not an integer ([%d,%d] != %d)",
					prop,
					info.prop->GetType(),
					info.prop->m_nBits,
					DPT_Int);
			}

			bit_count = info.prop->m_nBits;
			offset = info.actual_offset;
			break;
		}
	default:
		{
			return pContext->ThrowNativeError("Invalid Property type %d", params[2]);
		}
	}

	if (bit_count < 1)
	{
		bit_count = params[4] * 8;
	}

	if (bit_count >= 17)
	{
		return *(int32_t *)((uint8_t *)pEntity + offset);
	}
	else if (bit_count >= 9)
	{
		return *(int16_t *)((uint8_t *)pEntity + offset);
	}
	else if (bit_count >= 2)
	{
		return *(int8_t *)((uint8_t *)pEntity + offset);
	}
	else
	{
		return *(bool *)((uint8_t *)pEntity + offset) ? 1 : 0;
	}

	return 0;
}

static cell_t SetEntProp(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity;
	char *prop;
	int offset;
	const char *class_name;
	edict_t *pEdict;
	int bit_count;

	if (!IndexToAThings(params[1], &pEntity, &pEdict))
	{
		return pContext->ThrowNativeError("Entity %d (%d) is invalid", g_HL2.ReferenceToIndex(params[1]), params[1]);
	}

	if (!pEdict || (class_name = pEdict->GetClassName()) == NULL)
	{
		class_name = "";
	}

	pContext->LocalToString(params[3], &prop);

	switch (params[2])
	{
	case Prop_Data:
		{
			typedescription_t *td;

			FIND_PROP_DATA(td);

			if ((bit_count = MatchFieldAsInteger(td->fieldType)) == 0)
			{
				return pContext->ThrowNativeError("Data field %s is not an integer (%d)", 
					prop,
					td->fieldType);
			}

			offset = GetTypeDescOffs(td);
			break;
		}
	case Prop_Send:
		{
			sm_sendprop_info_t info;

			FIND_PROP_SEND(&info);

			if (info.prop->GetType() != DPT_Int)
			{
				return pContext->ThrowNativeError("SendProp %s is not an integer ([%d,%d] != %d)",
					prop,
					info.prop->GetType(),
					info.prop->m_nBits,
					DPT_Int);
			}

			bit_count = info.prop->m_nBits;
			offset = info.actual_offset;
			break;
		}
	default:
		{
			return pContext->ThrowNativeError("Invalid Property type %d", params[2]);
		}
	}

	if (bit_count < 1)
	{
		bit_count = params[5] * 8;
	}

	if (bit_count >= 17)
	{
		*(int32_t *)((uint8_t *)pEntity + offset) = params[4];
	}
	else if (bit_count >= 9)
	{
		*(int16_t *)((uint8_t *)pEntity + offset) = (int16_t)params[4];
	}
	else if (bit_count >= 2)
	{
		*(int8_t *)((uint8_t *)pEntity + offset) = (int8_t)params[4];
	}
	else
	{
		*(bool *)((uint8_t *)pEntity + offset) = params[4] ? true : false;
	}

	return 0;
}

static cell_t GetEntPropFloat(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity;
	char *prop;
	int offset;
	const char *class_name;
	edict_t *pEdict;

	if (!IndexToAThings(params[1], &pEntity, &pEdict))
	{
		return pContext->ThrowNativeError("Entity %d (%d) is invalid", g_HL2.ReferenceToIndex(params[1]), params[1]);
	}

	if (!pEdict || (class_name = pEdict->GetClassName()) == NULL)
	{
		class_name = "";
	}

	pContext->LocalToString(params[3], &prop);

	switch (params[2])
	{
	case Prop_Data:
		{
			typedescription_t *td;

			FIND_PROP_DATA(td);

			if (td->fieldType != FIELD_FLOAT
				&& td->fieldType != FIELD_TIME)
			{
				return pContext->ThrowNativeError("Data field %s is not a float (%d != [%d,%d])", 
					prop,
					td->fieldType,
					FIELD_FLOAT,
					FIELD_TIME);
			}

			offset = GetTypeDescOffs(td);
			break;
		}
	case Prop_Send:
		{
			sm_sendprop_info_t info;

			FIND_PROP_SEND(&info);

			if (info.prop->GetType() != DPT_Float)
			{
				return pContext->ThrowNativeError("SendProp %s is not a float (%d != %d)",
					prop,
					info.prop->GetType(),
					DPT_Float);
			}

			offset = info.actual_offset;
			break;
		}
	default:
		{
			return pContext->ThrowNativeError("Invalid Property type %d", params[2]);
		}
	}

	float val = *(float *)((uint8_t *)pEntity + offset);

	return sp_ftoc(val);
}

static cell_t SetEntPropFloat(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity;
	char *prop;
	int offset;
	const char *class_name;
	edict_t *pEdict;

	if (!IndexToAThings(params[1], &pEntity, &pEdict))
	{
		return pContext->ThrowNativeError("Entity %d (%d) is invalid", g_HL2.ReferenceToIndex(params[1]), params[1]);
	}

	if (!pEdict || (class_name = pEdict->GetClassName()) == NULL)
	{
		class_name = "";
	}

	pContext->LocalToString(params[3], &prop);

	switch (params[2])
	{
	case Prop_Data:
		{
			typedescription_t *td;

			FIND_PROP_DATA(td);

			if (td->fieldType != FIELD_FLOAT
				&& td->fieldType != FIELD_TIME)
			{
				return pContext->ThrowNativeError("Data field %s is not a float (%d != [%d,%d])", 
					prop,
					td->fieldType,
					FIELD_FLOAT,
					FIELD_TIME);
			}

			offset = GetTypeDescOffs(td);
			break;
		}
	case Prop_Send:
		{
			sm_sendprop_info_t info;

			FIND_PROP_SEND(&info);

			if (info.prop->GetType() != DPT_Float)
			{
				return pContext->ThrowNativeError("SendProp %s is not a float (%d != %d)",
					prop,
					info.prop->GetType(),
					DPT_Float);
			}

			offset = info.actual_offset;
			break;
		}
	default:
		{
			return pContext->ThrowNativeError("Invalid Property type %d", params[2]);
		}
	}

	*(float *)((uint8_t *)pEntity + offset) = sp_ctof(params[4]);

	if (params[2] == Prop_Send && (pEdict != NULL))
	{
		g_HL2.SetEdictStateChanged(pEdict, offset);
	}

	return 1;
}

static cell_t GetEntPropEnt(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity;
	char *prop;
	int offset;
	const char *class_name;
	edict_t *pEdict;

	if (!IndexToAThings(params[1], &pEntity, &pEdict))
	{
		return pContext->ThrowNativeError("Entity %d (%d) is invalid", g_HL2.ReferenceToIndex(params[1]), params[1]);
	}

	if (!pEdict || (class_name = pEdict->GetClassName()) == NULL)
	{
		class_name = "";
	}

	pContext->LocalToString(params[3], &prop);

	switch (params[2])
	{
	case Prop_Data:
		{
			typedescription_t *td;

			FIND_PROP_DATA(td);

			if (td->fieldType != FIELD_EHANDLE)
			{
				return pContext->ThrowNativeError("Data field %s is not an entity (%d != %d)", 
					prop,
					td->fieldType,
					FIELD_EHANDLE);
			}

			offset = GetTypeDescOffs(td);
			break;
		}
	case Prop_Send:
		{
			sm_sendprop_info_t info;

			FIND_PROP_SEND(&info);

			if (info.prop->GetType() != DPT_Int)
			{
				return pContext->ThrowNativeError("SendProp %s is not an integer (%d != %d)",
					prop,
					info.prop->GetType(),
					DPT_Int);
			}

			offset = info.actual_offset;
			break;
		}
	default:
		{
			return pContext->ThrowNativeError("Invalid Property type %d", params[2]);
		}
	}

	CBaseHandle &hndl = *(CBaseHandle *)((uint8_t *)pEntity + offset);

	int ref = g_HL2.IndexToReference(hndl.GetEntryIndex());
	return g_HL2.ReferenceToBCompatRef(ref);
}

static cell_t SetEntPropEnt(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity;
	char *prop;
	int offset;
	const char *class_name;
	edict_t *pEdict;

	if (!IndexToAThings(params[1], &pEntity, &pEdict))
	{
		return pContext->ThrowNativeError("Entity %d (%d) is invalid", g_HL2.ReferenceToIndex(params[1]), params[1]);
	}

	if (!pEdict || (class_name = pEdict->GetClassName()) == NULL)
	{
		class_name = "";
	}

	pContext->LocalToString(params[3], &prop);

	switch (params[2])
	{
	case Prop_Data:
		{
			typedescription_t *td;

			FIND_PROP_DATA(td);

			if (td->fieldType != FIELD_EHANDLE)
			{
				return pContext->ThrowNativeError("Data field %s is not an entity (%d != %d)", 
					prop,
					td->fieldType,
					FIELD_EHANDLE);
			}

			offset = GetTypeDescOffs(td);
			break;
		}
	case Prop_Send:
		{
			sm_sendprop_info_t info;

			FIND_PROP_SEND(&info);

			if (info.prop->GetType() != DPT_Int)
			{
				return pContext->ThrowNativeError("SendProp %s is not an integer (%d != %d)",
					prop,
					info.prop->GetType(),
					DPT_Int);
			}

			offset = info.actual_offset;
			break;
		}
	default:
		{
			return pContext->ThrowNativeError("Invalid Property type %d", params[2]);
		}
	}

	CBaseHandle &hndl = *(CBaseHandle *)((uint8_t *)pEntity + offset);

	if (params[4] == -1)
	{
		hndl.Set(NULL);
	}
	else
	{
		CBaseEntity *pOther = GetEntity(params[4]);

		if (!pOther)
		{
			return pContext->ThrowNativeError("Entity %d (%d) is invalid", g_HL2.ReferenceToIndex(params[4]), params[4]);
		}

		IHandleEntity *pHandleEnt = (IHandleEntity *)pOther;
		hndl.Set(pHandleEnt);
	}

	if (params[2] == Prop_Send && (pEdict != NULL))
	{
		g_HL2.SetEdictStateChanged(pEdict, offset);
	}

	return 1;
}

static cell_t GetEntPropVector(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity;
	char *prop;
	int offset;
	const char *class_name;
	edict_t *pEdict;
	
	if (!IndexToAThings(params[1], &pEntity, &pEdict))
	{
		return pContext->ThrowNativeError("Entity %d (%d) is invalid", g_HL2.ReferenceToIndex(params[1]), params[1]);
	}

	if (!pEdict || (class_name = pEdict->GetClassName()) == NULL)
	{
		class_name = "";
	}

	pContext->LocalToString(params[3], &prop);

	switch (params[2])
	{
	case Prop_Data:
		{
			typedescription_t *td;
			
			FIND_PROP_DATA(td);

			if (td->fieldType != FIELD_VECTOR
				&& td->fieldType != FIELD_POSITION_VECTOR)
			{
				return pContext->ThrowNativeError("Data field %s is not a vector (%d != [%d,%d])", 
					prop,
					td->fieldType,
					FIELD_VECTOR,
					FIELD_POSITION_VECTOR);
			}

			offset = GetTypeDescOffs(td);
			break;
		}
	case Prop_Send:
		{
			sm_sendprop_info_t info;

			FIND_PROP_SEND(&info);

			if (info.prop->GetType() != DPT_Vector)
			{
				return pContext->ThrowNativeError("SendProp %s is not a vector (%d != %d)",
					prop,
					info.prop->GetType(),
					DPT_Vector);
			}

			offset = info.actual_offset;
			break;
		}
	default:
		{
			return pContext->ThrowNativeError("Invalid Property type %d", params[2]);
		}
	}

	Vector *v = (Vector *)((uint8_t *)pEntity + offset);

	cell_t *vec;
	pContext->LocalToPhysAddr(params[4], &vec);

	vec[0] = sp_ftoc(v->x);
	vec[1] = sp_ftoc(v->y);
	vec[2] = sp_ftoc(v->z);

	return 1;
}

static cell_t SetEntPropVector(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity;
	char *prop;
	int offset;
	const char *class_name;
	edict_t *pEdict;
	
	if (!IndexToAThings(params[1], &pEntity, &pEdict))
	{
		return pContext->ThrowNativeError("Entity %d (%d) is invalid", g_HL2.ReferenceToIndex(params[1]), params[1]);
	}

	if (!pEdict || (class_name = pEdict->GetClassName()) == NULL)
	{
		class_name = "";
	}

	pContext->LocalToString(params[3], &prop);

	switch (params[2])
	{
	case Prop_Data:
		{
			typedescription_t *td;
			
			FIND_PROP_DATA(td);

			if (td->fieldType != FIELD_VECTOR
				&& td->fieldType != FIELD_POSITION_VECTOR)
			{
				return pContext->ThrowNativeError("Data field %s is not a vector (%d != [%d,%d])", 
					prop,
					td->fieldType,
					FIELD_VECTOR,
					FIELD_POSITION_VECTOR);
			}

			offset = GetTypeDescOffs(td);
			break;
		}
	case Prop_Send:
		{
			sm_sendprop_info_t info;

			FIND_PROP_SEND(&info);

			if (info.prop->GetType() != DPT_Vector)
			{
				return pContext->ThrowNativeError("SendProp %s is not a vector (%d != %d)",
					prop,
					info.prop->GetType(),
					DPT_Vector);
			}

			offset = info.actual_offset;
			break;
		}
	default:
		{
			return pContext->ThrowNativeError("Invalid Property type %d", params[2]);
		}
	}

	Vector *v = (Vector *)((uint8_t *)pEntity + offset);

	cell_t *vec;
	pContext->LocalToPhysAddr(params[4], &vec);

	v->x = sp_ctof(vec[0]);
	v->y = sp_ctof(vec[1]);
	v->z = sp_ctof(vec[2]);

	if (params[2] == Prop_Send && (pEdict != NULL))
	{
		g_HL2.SetEdictStateChanged(pEdict, offset);
	}

	return 1;
}

static cell_t GetEntPropString(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity;
	char *prop;
	int offset;
	const char *class_name;
	edict_t *pEdict;
	bool bIsStringIndex;
	
	if (!IndexToAThings(params[1], &pEntity, &pEdict))
	{
		return pContext->ThrowNativeError("Entity %d (%d) is invalid", g_HL2.ReferenceToIndex(params[1]), params[1]);
	}

	if (!pEdict || (class_name = pEdict->GetClassName()) == NULL)
	{
		class_name = "";
	}

	pContext->LocalToString(params[3], &prop);

	bIsStringIndex = false;

	switch (params[2])
	{
	case Prop_Data:
		{
			typedescription_t *td;
			
			FIND_PROP_DATA(td);

			if (td->fieldType != FIELD_CHARACTER
				&& td->fieldType != FIELD_STRING
				&& td->fieldType != FIELD_MODELNAME 
				&& td->fieldType != FIELD_SOUNDNAME)
			{
				return pContext->ThrowNativeError("Data field %s is not a string (%d != %d)", 
					prop,
					td->fieldType,
					FIELD_CHARACTER);
			}

			bIsStringIndex = (td->fieldType != FIELD_CHARACTER);

			offset = GetTypeDescOffs(td);
			break;
		}
	case Prop_Send:
		{
			sm_sendprop_info_t info;

			FIND_PROP_SEND(&info);

			if (info.prop->GetType() != DPT_String)
			{
				return pContext->ThrowNativeError("SendProp %s is not a string (%d != %d)",
					prop,
					info.prop->GetType(),
					DPT_String);
			}

			offset = info.actual_offset;
			break;
			break;
		}
	default:
		{
			return pContext->ThrowNativeError("Invalid Property type %d", params[2]);
		}
	}

	size_t len;
	const char *src; 
	
	if (!bIsStringIndex)
	{
		src = (char *)((uint8_t *)pEntity + offset);
	}
	else
	{
		string_t idx;

		idx = *(string_t *)((uint8_t *)pEntity + offset);
		src = (idx == NULL_STRING) ? "" : STRING(idx);
	}

	pContext->StringToLocalUTF8(params[4], params[5], src, &len);

	return len;
}

static cell_t SetEntPropString(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity;
	char *prop;
	int offset;
	int maxlen;
	edict_t *pEdict;

	if (!IndexToAThings(params[1], &pEntity, &pEdict))
	{
		return pContext->ThrowNativeError("Entity %d (%d) is invalid", g_HL2.ReferenceToIndex(params[1]), params[1]);
	}

	switch (params[2])
	{
	case Prop_Data:
		{
			datamap_t *pMap;
			typedescription_t *td;
			if ((pMap=CBaseEntity_GetDataDescMap(pEntity)) == NULL)
			{
				return pContext->ThrowNativeError("Unable to retrieve GetDataDescMap offset");
			}
			pContext->LocalToString(params[3], &prop);
			if ((td=g_HL2.FindInDataMap(pMap, prop)) == NULL)
			{
				return pContext->ThrowNativeError("Property \"%s\" not found for entity %d", prop, params[1]);
			}
			if (td->fieldType != FIELD_CHARACTER)
			{
				return pContext->ThrowNativeError("Property \"%s\" is not a valid string", prop);
			}
			offset = GetTypeDescOffs(td);
			maxlen = td->fieldSize;
			break;
		}
	case Prop_Send:
		{
			char *prop;
			IServerUnknown *pUnk = (IServerUnknown *)pEntity;
			IServerNetworkable *pNet = pUnk->GetNetworkable();
			if (!pNet)
			{
				return pContext->ThrowNativeError("The edict is not networkable");
			}
			pContext->LocalToString(params[3], &prop);
			SendProp *pSend = g_HL2.FindInSendTable(pNet->GetServerClass()->GetName(), prop);
			if (!pSend)
			{
				return pContext->ThrowNativeError("Property \"%s\" not found for entity %d", prop, params[1]);
			}
			if (pSend->GetType() != DPT_String)
			{
				return pContext->ThrowNativeError("Property \"%s\" is not a valid string", prop);
			}
			offset = pSend->GetOffset();
			maxlen = DT_MAX_STRING_BUFFERSIZE;
			break;
		}
	default:
		{
			return pContext->ThrowNativeError("Invalid Property type %d", params[2]);
		}
	}

	char *src;
	char *dest = (char *)((uint8_t *)pEntity + offset);

	pContext->LocalToString(params[4], &src);
	size_t len = strncopy(dest, src, maxlen);

	if (params[2] == Prop_Send && (pEdict != NULL))
	{
		g_HL2.SetEdictStateChanged(pEdict, offset);
	}

	return len;
}

REGISTER_NATIVES(entityNatives)
{
	{"ChangeEdictState",		ChangeEdictState},
	{"CreateEdict",				CreateEdict},
	{"FindDataMapOffs",			FindDataMapOffs},
	{"FindSendPropInfo",		FindSendPropInfo},
	{"FindSendPropOffs",		FindSendPropOffs},
	{"GetEdictClassname",		GetEdictClassname},
	{"GetEdictFlags",			GetEdictFlags},
	{"GetEntData",				GetEntData},
	{"GetEntDataEnt",			GetEntDataEnt},		/** DEPRECATED */
	{"GetEntDataEnt2",			GetEntDataEnt2},
	{"GetEntDataFloat",			GetEntDataFloat},
	{"GetEntDataString",		GetEntDataString},
	{"GetEntDataVector",		GetEntDataVector},
	{"GetEntProp",				GetEntProp},
	{"GetEntPropEnt",			GetEntPropEnt},
	{"GetEntPropFloat",			GetEntPropFloat},
	{"GetEntPropString",		GetEntPropString},
	{"GetEntPropVector",		GetEntPropVector},
	{"GetEntityCount",			GetEntityCount},
	{"GetEntityNetClass",		GetEntityNetClass},
	{"GetMaxEntities",			GetMaxEntities},
	{"IsEntNetworkable",		IsEntNetworkable},
	{"IsValidEdict",			IsValidEdict},
	{"IsValidEntity",			IsValidEntity},
	{"RemoveEdict",				RemoveEdict},
	{"SetEdictFlags",			SetEdictFlags},
	{"SetEntData",				SetEntData},
	{"SetEntDataEnt",			SetEntDataEnt},		/** DEPRECATED */
	{"SetEntDataEnt2",			SetEntDataEnt2},
	{"SetEntDataFloat",			SetEntDataFloat},
	{"SetEntDataVector",		SetEntDataVector},
	{"SetEntDataString",		SetEntDataString},
	{"SetEntProp",				SetEntProp},
	{"SetEntPropEnt",			SetEntPropEnt},
	{"SetEntPropFloat",			SetEntPropFloat},
	{"SetEntPropString",		SetEntPropString},
	{"SetEntPropVector",		SetEntPropVector},
	{NULL,						NULL}
};
