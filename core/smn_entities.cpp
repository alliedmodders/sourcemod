/**
 * ===============================================================
 * SourceMod (C)2004-2007 AlliedModders LLC.  All rights reserved.
 * ===============================================================
 *
 * This file is not open source and may not be copied without explicit
 * written permission of AlliedModders LLC.  This file may not be redistributed 
 * in whole or significant part.
 * For information, see LICENSE.txt or http://www.sourcemod.net/license.php
 *
 * Version: $Id$
 */

#include "sm_globals.h"
#include "sourcemod.h"
#include "sourcemm_api.h"
#include "server_class.h"
#include "PlayerManager.h"
#include "HalfLife2.h"

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
		pEdict->StateChanged(offset);
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
		pEdict->StateChanged(offset);
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
		pEdict->StateChanged(offset);
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
		pEdict->StateChanged(offset);
	}

	return 1;
}

IChangeInfoAccessor *CBaseEdict::GetChangeAccessor()
{
	return engine->GetChangeAccessor( (const edict_t *)this );
}

CSharedEdictChangeInfo *g_pSharedChangeInfo = NULL;

static cell_t ChangeEdictState(IPluginContext *pContext, const cell_t *params)
{
	edict_t *pEdict = GetEdict(params[1]);

	if (!pEdict)
	{
		return pContext->ThrowNativeError("Edict %d is invalid", params[1]);
	}

	if (!g_pSharedChangeInfo)
	{
		g_pSharedChangeInfo = engine->GetSharedEdictChangeInfo();
	}

	if (params[2])
	{
		pEdict->StateChanged(params[2]);
	} else {
		pEdict->StateChanged();
	}

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
	{NULL,						NULL}
};
