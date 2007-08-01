/**
 * vim: set ts=4 :
 * ================================================================
 * SourceMod
 * Copyright (C) 2004-2007 AlliedModders LLC.  All rights reserved.
 * ================================================================
 *
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License, 
 * version 3.0, as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to 
 * link the code of this program (as well as its derivative works) to 
 * "Half-Life 2," the "Source Engine," the "SourcePawn JIT," and any 
 * Game MODs that run on software by the Valve Corporation.  You must 
 * obey the GNU General Public License in all respects for all other 
 * code used. Additionally, AlliedModders LLC grants this exception 
 * to all derivative works. AlliedModders LLC defines further 
 * exceptions, found in LICENSE.txt (as of this writing, version 
 * JULY-31-2007), or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */

#include "sm_globals.h"
#include "sourcemod.h"
#include "sourcemm_api.h"
#include "server_class.h"
#include "PlayerManager.h"
#include "HalfLife2.h"
#include "GameConfigs.h"
#include "sm_stringutil.h"

enum PropType
{
	Prop_Send = 0,
	Prop_Data
};

inline edict_t *GetEdict(cell_t num)
{
	edict_t *pEdict = engine->PEntityOfEntIndex(num);
	if (!pEdict || pEdict->IsFree())
	{
		return NULL;
	}
	if (num > 0 && num < g_Players.GetMaxClients())
	{
		CPlayer *pPlayer = g_Players.GetPlayerByIndex(num);
		if (!pPlayer || !pPlayer->IsConnected())
		{
			return NULL;
		}
	}
	return pEdict;
}

inline edict_t *GetEntity(cell_t num, CBaseEntity **pData)
{
	edict_t *pEdict = engine->PEntityOfEntIndex(num);
	if (!pEdict || pEdict->IsFree())
	{
		return NULL;
	}
	if (num > 0 && num < g_Players.GetMaxClients())
	{
		CPlayer *pPlayer = g_Players.GetPlayerByIndex(num);
		if (!pPlayer || !pPlayer->IsConnected())
		{
			return NULL;
		}
	}
	IServerUnknown *pUnk;
	if ((pUnk=pEdict->GetUnknown()) == NULL)
	{
		return NULL;
	}
	*pData = pUnk->GetBaseEntity();
	return pEdict;
}

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

	return engine->IndexOfEdict(pEdict);
}

static cell_t RemoveEdict(IPluginContext *pContext, const cell_t *params)
{
	edict_t *pEdict = engine->PEntityOfEntIndex(params[1]);

	if (!pEdict)
	{
		return pContext->ThrowNativeError("Edict %d is not a valid edict", params[1]);
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
	edict_t *pEdict = GetEdict(params[1]);

	if (!pEdict)
	{
		return 0;
	}

	IServerUnknown *pUnknown = pEdict->GetUnknown();
	if (!pUnknown)
	{
		return 0;
	}

	CBaseEntity *pEntity = pUnknown->GetBaseEntity();
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
		return pContext->ThrowNativeError("Invalid edict (%d)", params[1]);
	}

	return pEdict->m_fStateFlags;
}

static cell_t SetEdictFlags(IPluginContext *pContext, const cell_t *params)
{
	edict_t *pEdict = GetEdict(params[1]);

	if (!pEdict)
	{
		return pContext->ThrowNativeError("Invalid edict (%d)", params[1]);
	}

	pEdict->m_fStateFlags = params[2];

	return 1;
}

static cell_t GetEdictClassname(IPluginContext *pContext, const cell_t *params)
{
	edict_t *pEdict = GetEdict(params[1]);

	if (!pEdict)
	{
		return pContext->ThrowNativeError("Invalid edict (%d)", params[1]);
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
	edict_t *pEdict = GetEdict(params[1]);

	if (!pEdict)
	{
		return pContext->ThrowNativeError("Invalid edict (%d)", params[1]);
	}

	IServerNetworkable *pNet = pEdict->GetNetworkable();
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
	CBaseEntity *pEntity;
	edict_t *pEdict = GetEntity(params[1], &pEntity);

	if (!pEdict || !pEntity)
	{
		return pContext->ThrowNativeError("Entity %d is invalid", params[1]);
	}

	int offset = params[2];
	if (offset < 0 || offset > 32768)
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
	edict_t *pEdict = GetEntity(params[1], &pEntity);

	if (!pEdict || !pEntity)
	{
		return pContext->ThrowNativeError("Entity %d is invalid", params[1]);
	}

	int offset = params[2];
	if (offset < 0 || offset > 32768)
	{
		return pContext->ThrowNativeError("Offset %d is invalid", offset);
	}

	if (params[5])
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
	CBaseEntity *pEntity;
	edict_t *pEdict = GetEntity(params[1], &pEntity);

	if (!pEdict || !pEntity)
	{
		return pContext->ThrowNativeError("Entity %d is invalid", params[1]);
	}

	int offset = params[2];
	if (offset < 0 || offset > 32768)
	{
		return pContext->ThrowNativeError("Offset %d is invalid", offset);
	}

	float f = *(float *)((uint8_t *)pEntity + offset);

	return sp_ftoc(f);
}

static cell_t SetEntDataFloat(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity;
	edict_t *pEdict = GetEntity(params[1], &pEntity);

	if (!pEdict || !pEntity)
	{
		return pContext->ThrowNativeError("Entity %d is invalid", params[1]);
	}

	int offset = params[2];
	if (offset < 0 || offset > 32768)
	{
		return pContext->ThrowNativeError("Offset %d is invalid", offset);
	}

	*(float *)((uint8_t *)pEntity + offset) = sp_ctof(params[3]);

	if (params[4])
	{
		g_HL2.SetEdictStateChanged(pEdict, offset);
	}

	return 1;
}

static cell_t GetEntDataVector(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity;
	edict_t *pEdict = GetEntity(params[1], &pEntity);

	if (!pEdict || !pEntity)
	{
		return pContext->ThrowNativeError("Entity %d is invalid", params[1]);
	}

	int offset = params[2];
	if (offset < 0 || offset > 32768)
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
	edict_t *pEdict = GetEntity(params[1], &pEntity);

	if (!pEdict || !pEntity)
	{
		return pContext->ThrowNativeError("Entity %d is invalid", params[1]);
	}

	int offset = params[2];
	if (offset < 0 || offset > 32768)
	{
		return pContext->ThrowNativeError("Offset %d is invalid", offset);
	}

	Vector *v = (Vector *)((uint8_t *)pEntity + offset);

	cell_t *vec;
	pContext->LocalToPhysAddr(params[3], &vec);

	v->x = sp_ctof(vec[0]);
	v->y = sp_ctof(vec[1]);
	v->z = sp_ctof(vec[2]);

	if (params[4])
	{
		g_HL2.SetEdictStateChanged(pEdict, offset);
	}

	return 1;
}

static cell_t GetEntDataEnt(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity;
	edict_t *pEdict = GetEntity(params[1], &pEntity);

	if (!pEdict || !pEntity)
	{
		return pContext->ThrowNativeError("Entity %d is invalid", params[1]);
	}

	int offset = params[2];
	if (offset < 0 || offset > 32768)
	{
		return pContext->ThrowNativeError("Offset %d is invalid", offset);
	}

	CBaseHandle &hndl = *(CBaseHandle *)((uint8_t *)pEntity + offset);

	if (!hndl.IsValid())
	{
		return 0;
	}

	/* :TODO: check serial no.? */

	return hndl.GetEntryIndex();
}

static cell_t SetEntDataEnt(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity;
	edict_t *pEdict = GetEntity(params[1], &pEntity);

	if (!pEdict || !pEntity)
	{
		return pContext->ThrowNativeError("Entity %d is invalid", params[1]);
	}

	int offset = params[2];
	if (offset < 0 || offset > 32768)
	{
		return pContext->ThrowNativeError("Offset %d is invalid", offset);
	}

	CBaseHandle &hndl = *(CBaseHandle *)((uint8_t *)pEntity + offset);

	if (params[3] == 0)
	{
		hndl.Set(NULL);
	} else {
		edict_t *pEdict = GetEdict(params[3]);
		if (!pEdict)
		{
			return pContext->ThrowNativeError("Entity %d is invalid", params[3]);
		}
		IServerEntity *pEntOther = pEdict->GetIServerEntity();
		hndl.Set(pEntOther);
	}

	if (params[4])
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
		return pContext->ThrowNativeError("Edict %d is invalid", params[1]);
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

static cell_t FindDataMapOffs(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity;
	datamap_t *pMap;
	typedescription_t *td;
	char *offset;
	edict_t *pEdict = GetEntity(params[1], &pEntity);

	if (!pEdict || !pEntity)
	{
		return pContext->ThrowNativeError("Entity %d is invalid", params[1]);
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

	return td->fieldOffset[TD_OFFSET_NORMAL];
}

static cell_t GetEntDataString(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity;
	edict_t *pEdict = GetEntity(params[1], &pEntity);

	if (!pEdict || !pEntity)
	{
		return pContext->ThrowNativeError("Entity %d is invalid", params[1]);
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
	edict_t *pEdict = GetEntity(params[1], &pEntity);

	if (!pEdict || !pEntity)
	{
		return pContext->ThrowNativeError("Entity %d is invalid", params[1]);
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

	if (params[5])
	{
		g_HL2.SetEdictStateChanged(pEdict, offset);
	}

	return len;
}

static cell_t GetEntPropString(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity;
	char *prop;
	int offset;
	edict_t *pEdict = GetEntity(params[1], &pEntity);

	if (!pEdict || !pEntity)
	{
		return pContext->ThrowNativeError("Entity %d is invalid", params[1]);
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
			offset = td->fieldOffset[TD_OFFSET_NORMAL];
			break;
		}
	case Prop_Send:
		{
			char *prop;
			IServerNetworkable *pNet = pEdict->GetNetworkable();
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
			break;
		}
	default:
		{
			return pContext->ThrowNativeError("Invalid Property type %d", params[2]);
		}
	}

	size_t len;
	char *src = (char *)((uint8_t *)pEntity + offset);
	pContext->StringToLocalUTF8(params[4], params[5], src, &len);

	return len;
}

static cell_t SetEntPropString(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity;
	char *prop;
	int offset;
	int maxlen;
	bool is_sendprop = false;
	edict_t *pEdict = GetEntity(params[1], &pEntity);

	if (!pEdict || !pEntity)
	{
		return pContext->ThrowNativeError("Entity %d is invalid", params[1]);
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
			offset = td->fieldOffset[TD_OFFSET_NORMAL];
			maxlen = td->fieldSize;
			break;
		}
	case Prop_Send:
		{
			char *prop;
			IServerNetworkable *pNet = pEdict->GetNetworkable();
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
			is_sendprop = true;
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

	if (is_sendprop)
	{
		g_HL2.SetEdictStateChanged(pEdict, offset);
	}

	return len;
}

REGISTER_NATIVES(entityNatives)
{
	{"ChangeEdictState",		ChangeEdictState},
	{"CreateEdict",				CreateEdict},
	{"FindSendPropOffs",		FindSendPropOffs},
	{"GetEdictClassname",		GetEdictClassname},
	{"GetEdictFlags",			GetEdictFlags},
	{"GetEntData",				GetEntData},
	{"GetEntDataEnt",			GetEntDataEnt},
	{"GetEntDataFloat",			GetEntDataFloat},
	{"GetEntDataVector",		GetEntDataVector},
	{"GetEntityCount",			GetEntityCount},
	{"GetEntityNetClass",		GetEntityNetClass},
	{"GetMaxEntities",			GetMaxEntities},
	{"IsEntNetworkable",		IsEntNetworkable},
	{"IsValidEdict",			IsValidEdict},
	{"IsValidEntity",			IsValidEntity},
	{"RemoveEdict",				RemoveEdict},
	{"SetEdictFlags",			SetEdictFlags},
	{"SetEntData",				SetEntData},
	{"SetEntDataEnt",			SetEntDataEnt},
	{"SetEntDataFloat",			SetEntDataFloat},
	{"SetEntDataVector",		SetEntDataVector},
	{"FindDataMapOffs",			FindDataMapOffs},
	{"GetEntDataString",		GetEntDataString},
	{"SetEntDataString",		SetEntDataString},
	{"GetEntPropString",		GetEntPropString},
	{"SetEntPropString",		SetEntPropString},
	{NULL,						NULL}
};
