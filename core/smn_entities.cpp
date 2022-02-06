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

// These values need to mirror the values in entity_prop_stocks
#define ENTFLAG_ONGROUND		(1 << 0)
#define ENTFLAG_DUCKING			(1 << 1)
#define ENTFLAG_WATERJUMP		(1 << 2)
#define ENTFLAG_ONTRAIN			(1 << 3)
#define ENTFLAG_INRAIN			(1 << 4)
#define ENTFLAG_FROZEN			(1 << 5)
#define ENTFLAG_ATCONTROLS		(1 << 6)
#define ENTFLAG_CLIENT			(1 << 7)
#define ENTFLAG_FAKECLIENT		(1 << 8)
#define ENTFLAG_INWATER			(1 << 9)
#define ENTFLAG_FLY				(1 << 10)
#define ENTFLAG_SWIM			(1 << 11)
#define ENTFLAG_CONVEYOR		(1 << 12)
#define ENTFLAG_NPC				(1 << 13)
#define ENTFLAG_GODMODE			(1 << 14)
#define ENTFLAG_NOTARGET		(1 << 15)
#define ENTFLAG_AIMTARGET		(1 << 16)
#define ENTFLAG_PARTIALGROUND	(1 << 17)
#define ENTFLAG_STATICPROP		(1 << 18)
#define ENTFLAG_GRAPHED			(1 << 19)
#define ENTFLAG_GRENADE			(1 << 20)
#define ENTFLAG_STEPMOVEMENT	(1 << 21)
#define ENTFLAG_DONTTOUCH		(1 << 22)
#define ENTFLAG_BASEVELOCITY	(1 << 23)
#define ENTFLAG_WORLDBRUSH		(1 << 24)
#define ENTFLAG_OBJECT			(1 << 25)
#define ENTFLAG_KILLME			(1 << 26)
#define ENTFLAG_ONFIRE			(1 << 27)
#define ENTFLAG_DISSOLVING		(1 << 28)
#define ENTFLAG_TRANSRAGDOLL	(1 << 29)
#define ENTFLAG_UNBLOCKABLE_BY_PLAYER	(1 << 30)
#define ENTFLAG_FREEZING		(1 << 31)
#define ENTFLAG_EP2V_UNKNOWN1	(1 << 31)

// Not defined in the sdk as we have no clue what it is
#define FL_EP2V_UNKNOWN			(1 << 2)

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
	PropField_Variant,			/**< Valid for variants/any. (User must know type) */
};

// From game/server/variant_t.h, same on all supported games.
class variant_t
{
public:
	union
	{
		bool bVal;
		string_t iszVal;
		int iVal;
		float flVal;
		float vecVal[3];
		color32 rgbaVal;
	};
	
	CBaseHandle eVal;
	fieldtype_t fieldType;
};

// From game/server/baseentity.h, same on all supported games.
struct inputdata_t
{
	CBaseEntity *pActivator;		// The entity that initially caused this chain of output events.
	CBaseEntity *pCaller;			// The entity that fired this particular output.
	variant_t value;				// The data parameter for this output.
	int nOutputID;					// The unique ID of the output that was fired.
};

inline bool CanSetPropName(const char *pszPropName)
{
#if SOURCE_ENGINE == SE_CSGO
	return g_HL2.CanSetCSGOEntProp(pszPropName);
#else
	return true;
#endif
}

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

static cell_t RemoveEntity(IPluginContext *pContext, const cell_t *params)
{
	auto *pEntity = GetEntity(params[1]);
	if (!pEntity)
	{
		return pContext->ThrowNativeError("Entity %d (%d) is not a valid entity", g_HL2.ReferenceToIndex(params[1]), params[1]);
	}

	// Some games have UTIL_Remove exposed on IServerTools, but for consistence, we'll
	// use this method for all. Results in DeathNotice( this ) being called on parent,
	// and parent being cleared (both if any parent) before UTIL_Remove is called.
	static inputfunc_t fnKillEntity = nullptr;
	if (!fnKillEntity)
	{
		// Get world, as other ents aren't guaranteed to inherit full datadesc (but kill func is same for all)
		CBaseEntity *pGetterEnt = g_HL2.ReferenceToEntity(0);
		if (pGetterEnt == nullptr)
		{
			// If we don't have a world entity yet, we'll have to rely on the given entity. Does this even make sense???
			pGetterEnt = pEntity;
		}

		datamap_t *pMap = g_HL2.GetDataMap(pGetterEnt);

		sm_datatable_info_t info;
		if (!g_HL2.FindDataMapInfo(pMap, "InputKill", &info))
		{
			return pContext->ThrowNativeError("Failed to find Kill input!");
		}

		fnKillEntity = info.prop->inputFunc;
	}

	// Input data is ignored for this. No need to initialize
	static inputdata_t data;

	(pEntity->*fnKillEntity)(data);

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

	const char *cls = g_HL2.GetEntityClassname(pEdict);

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
	CBaseEntity *pHandleEntity = g_HL2.ReferenceToEntity(hndl.GetEntryIndex());

	if (!pHandleEntity || hndl != reinterpret_cast<IHandleEntity *>(pHandleEntity)->GetRefEHandle())
		return -1;

	return g_HL2.EntityToBCompatRef(pHandleEntity);
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

	// Before we added support for DPT_Array props, FindInSendTable would have given us the inner array prop itself.
	// To maintain compatibility with older plugins still using us, pluck out the inner array prop ourselves.
	if (pSend->GetType() == DPT_Array && pSend->GetArrayProp()) {
		return pSend->GetArrayProp()->GetOffset();
	}

	return pSend->GetOffset();
}

static cell_t FindSendPropInfo(IPluginContext *pContext, const cell_t *params)
{
	char *cls, *prop;
	sm_sendprop_info_t info;
	cell_t *pType, *pBits, *pLocal, *pArraySize = nullptr;

	pContext->LocalToString(params[1], &cls);
	pContext->LocalToString(params[2], &prop);

	if (!g_HL2.FindSendPropInfo(cls, prop, &info))
	{
		return -1;
	}

	pContext->LocalToPhysAddr(params[3], &pType);
	pContext->LocalToPhysAddr(params[4], &pBits);
	pContext->LocalToPhysAddr(params[5], &pLocal);

	if (params[0] >= 6) {
		pContext->LocalToPhysAddr(params[6], &pArraySize);
		*pArraySize = 0;
	}

	SendProp *pProp = info.prop;
	unsigned int actual_offset = info.actual_offset;

	// SendPropArray / SendPropArray2
	if (pProp->GetType() == DPT_Array && pProp->GetArrayProp())
	{
		if (pArraySize) {
			// This'll only work for SendPropArray
			*pArraySize = pProp->GetNumElements();
		}

		// Use the type / bits / local offset of the real data prop
		pProp = pProp->GetArrayProp();

		// This is sane as the DPT_Array prop's local offset is always 0
		actual_offset += pProp->GetOffset();
	}

	// Get the local offset now before we might dive into another table
	*pLocal = pProp->GetOffset();

	// SendPropArray3
	SendTable *pTable = pProp->GetDataTable();
	if (pProp->GetType() == DPT_DataTable && pTable && pTable->GetNumProps() > 0)
	{
		if (pArraySize) {
			*pArraySize = pTable->GetNumProps();
		}

		// Use the type / bits of the first data prop
		pProp = pTable->GetProp(0);
	}

	switch (pProp->GetType())
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

	*pBits = pProp->m_nBits;

	return actual_offset;
}

static void GuessDataPropTypes(typedescription_t *td, cell_t * pSize, cell_t * pType)
{
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
			return;
		}
	case FIELD_VECTOR:
	case FIELD_POSITION_VECTOR:
		{
			*pType = PropField_Vector;
			*pSize = 96;
			return;
		}
	case FIELD_SHORT:
		{
			*pType = PropField_Integer;
			*pSize = 16;
			return;
		}
	case FIELD_BOOLEAN:
		{
			*pType = PropField_Integer;
			*pSize = 1;
			return;
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
			return;
		}
	case FIELD_MODELNAME:
	case FIELD_SOUNDNAME:
	case FIELD_STRING:
		{
			*pSize = sizeof(string_t);
			*pType = PropField_String_T;
			return;
		}
	case FIELD_FLOAT:
	case FIELD_TIME:
		{
			*pType = PropField_Float;
			*pSize = 32;
			return;
		}
	case FIELD_EHANDLE:
		{
			*pType = PropField_Entity;
			*pSize = 32;
			return;
		}
	case FIELD_CUSTOM:
		{
			if ((td->flags & FTYPEDESC_OUTPUT) == FTYPEDESC_OUTPUT)
			{
				*pType = PropField_Variant;
				*pSize = 0;
				return;
			}
		}
	}
			
	*pType = PropField_Unsupported;
	*pSize = 0;
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
	sm_datatable_info_t info;
	if (!g_HL2.FindDataMapInfo(pMap, offset, &info))
	{
		return -1;
	}
	
	td = info.prop;

	if (params[0] == 4)
	{
		cell_t *pType, *pSize;

		pContext->LocalToPhysAddr(params[3], &pType);
		pContext->LocalToPhysAddr(params[4], &pSize);
		
		GuessDataPropTypes(td, pSize, pType);
	}

	return GetTypeDescOffs(td);
}

static cell_t FindDataMapInfo(IPluginContext *pContext, const cell_t *params)
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
	sm_datatable_info_t info;
	if (!g_HL2.FindDataMapInfo(pMap, offset, &info))
	{
		return -1;
	}
	
	td = info.prop;

	if (params[0] >= 4)
	{
		cell_t *pType, *pSize;

		pContext->LocalToPhysAddr(params[3], &pType);
		pContext->LocalToPhysAddr(params[4], &pSize);
		
		GuessDataPropTypes(td, pSize, pType);
		
		if (params[0] == 5)
		{
			cell_t *pLocalOffs;
			
			pContext->LocalToPhysAddr(params[5], &pLocalOffs);
			
			*pLocalOffs = GetTypeDescOffs(info.prop);
		}
	}

	return info.actual_offset;
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
	size_t len = ke::SafeStrcpy(dest, params[4], src);

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
	sm_datatable_info_t info; \
	if (!g_HL2.FindDataMapInfo(pMap, prop, &info)) \
	{ \
		const char *class_name = g_HL2.GetEntityClassname(pEntity); \
		return pContext->ThrowNativeError("Property \"%s\" not found (entity %d/%s)", \
			prop, \
			params[1], \
			((class_name) ? class_name : "")); \
	} \
	td = info.prop;

#define CHECK_SET_PROP_DATA_OFFSET() \
	if (element < 0 || element >= td->fieldSize) \
	{ \
		return pContext->ThrowNativeError("Element %d is out of bounds (Prop %s has %d elements).", \
			element, \
			prop, \
			td->fieldSize); \
	} \
	\
	offset = info.actual_offset + (element * (td->fieldSizeInBytes / td->fieldSize));

#define CHECK_TYPE_VALID_IF_VARIANT(type, typeName) \
	if (td->fieldType == FIELD_CUSTOM && (td->flags & FTYPEDESC_OUTPUT) == FTYPEDESC_OUTPUT) \
	{ \
		auto *pVariant = (variant_t *)((intptr_t)pEntity + offset); \
		if (pVariant->fieldType != type) \
		{ \
			return pContext->ThrowNativeError("Variant value for %s is not a %s (%d)", \
				prop, \
				typeName, \
				pVariant->fieldType); \
		} \
	}

#define SET_TYPE_IF_VARIANT(type) \
	if (td->fieldType == FIELD_CUSTOM && (td->flags & FTYPEDESC_OUTPUT) == FTYPEDESC_OUTPUT) \
	{ \
		auto *pVariant = (variant_t *)((intptr_t)pEntity + offset); \
		pVariant->fieldType = type; \
	}

#define FIND_PROP_SEND(type, type_name) \
	sm_sendprop_info_t info;\
	SendProp *pProp; \
	IServerUnknown *pUnk = (IServerUnknown *)pEntity; \
	IServerNetworkable *pNet = pUnk->GetNetworkable(); \
	if (!pNet) \
	{ \
		return pContext->ThrowNativeError("Edict %d (%d) is not networkable", g_HL2.ReferenceToIndex(params[1]), params[1]); \
	} \
	if (!g_HL2.FindSendPropInfo(pNet->GetServerClass()->GetName(), prop, &info)) \
	{ \
		const char *class_name = g_HL2.GetEntityClassname(pEntity); \
		return pContext->ThrowNativeError("Property \"%s\" not found (entity %d/%s)", \
			prop, \
			params[1], \
			((class_name) ? class_name : "")); \
	} \
	\
	offset = info.actual_offset; \
	pProp = info.prop; \
	bit_count = pProp->m_nBits; \
	\
	switch (pProp->GetType()) \
	{ \
	case type: \
		{ \
			if (element != 0) \
			{ \
				return pContext->ThrowNativeError("SendProp %s is not an array. Element %d is invalid.", \
					prop, \
					element); \
			} \
			break; \
		} \
	case DPT_Array: \
		{ \
			int elementCount = pProp->GetNumElements(); \
			int elementStride = pProp->GetElementStride(); \
			if (element < 0 || element >= elementCount) \
			{ \
				return pContext->ThrowNativeError("Element %d is out of bounds (Prop %s has %d elements).", \
					element, \
					prop, \
					elementCount); \
			} \
			\
			pProp = pProp->GetArrayProp(); \
			if (!pProp) { \
				return pContext->ThrowNativeError("Error looking up ArrayProp for prop %s", \
					prop); \
			} \
			\
			if (pProp->GetType() != type) \
			{ \
				return pContext->ThrowNativeError("SendProp %s type is not " type_name " ([%d,%d] != %d)", \
					prop, \
					pProp->GetType(), \
					pProp->m_nBits, \
					type); \
			} \
			\
			offset += pProp->GetOffset() + (elementStride * element); \
			bit_count = pProp->m_nBits; \
			break; \
		} \
	case DPT_DataTable: \
		{ \
			FIND_PROP_SEND_IN_SENDTABLE(info, pProp, element, type, type_name); \
			\
			offset += pProp->GetOffset(); \
			bit_count = pProp->m_nBits; \
			break; \
		} \
	default: \
		{ \
			return pContext->ThrowNativeError("SendProp %s type is not " type_name " (%d != %d)", \
				prop, \
				pProp->GetType(), \
				type); \
		} \
	} \

#define FIND_PROP_SEND_IN_SENDTABLE(info, pProp, element, type, type_name) \
	SendTable *pTable = pProp->GetDataTable(); \
	if (!pTable) \
	{ \
		return pContext->ThrowNativeError("Error looking up DataTable for prop %s", \
			prop); \
	} \
	\
	int elementCount = pTable->GetNumProps(); \
	if (element < 0 || element >= elementCount) \
	{ \
		return pContext->ThrowNativeError("Element %d is out of bounds (Prop %s has %d elements).", \
			element, \
			prop, \
			elementCount); \
	} \
	\
	pProp = pTable->GetProp(element); \
	if (pProp->GetType() != type) \
	{ \
		return pContext->ThrowNativeError("SendProp %s type is not " type_name " ([%d,%d] != %d)", \
			prop, \
			pProp->GetType(), \
			pProp->m_nBits, \
			type); \
	}


inline int MatchTypeDescAsInteger(_fieldtypes type, int flags)
{
	switch (type)
	{
	case FIELD_TICK:
	case FIELD_MODELINDEX:
	case FIELD_MATERIALINDEX:
	case FIELD_INTEGER:
	case FIELD_COLOR32:
		return 32;
	case FIELD_CUSTOM:
		if ((flags & FTYPEDESC_OUTPUT) == FTYPEDESC_OUTPUT)
		{
			// Variant, read as int32.
			return 32;
		}
		break;
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

static cell_t GetEntPropArraySize(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity;
	char *prop;
	edict_t *pEdict;

	if (!IndexToAThings(params[1], &pEntity, &pEdict))
	{
		return pContext->ThrowNativeError("Entity %d (%d) is invalid", g_HL2.ReferenceToIndex(params[1]), params[1]);
	}

	pContext->LocalToString(params[3], &prop);

	switch (params[2])
	{
	case Prop_Data:
		{
			typedescription_t *td;

			FIND_PROP_DATA(td);

			return td->fieldSize;
		}
	case Prop_Send:
		{
			sm_sendprop_info_t info;
			
			IServerUnknown *pUnk = (IServerUnknown *)pEntity;
			IServerNetworkable *pNet = pUnk->GetNetworkable();
			if (!pNet)
			{
				return pContext->ThrowNativeError("Edict %d (%d) is not networkable", g_HL2.ReferenceToIndex(params[1]), params[1]);
			}
			if (!g_HL2.FindSendPropInfo(pNet->GetServerClass()->GetName(), prop, &info))
			{
				const char *class_name = g_HL2.GetEntityClassname(pEntity);
				return pContext->ThrowNativeError("Property \"%s\" not found (entity %d/%s)",
					prop,
					params[1],
					((class_name) ? class_name : ""));
			}

			if (info.prop->GetType() == DPT_Array)
			{
				return info.prop->GetNumElements();
			}

			if (info.prop->GetType() != DPT_DataTable)
			{
				return 0;
			}

			SendTable *pTable = info.prop->GetDataTable();
			if (!pTable)
			{
				return pContext->ThrowNativeError("Error looking up DataTable for prop %s", prop);
			}
			
			return pTable->GetNumProps();
		}
	default:
		{
			return pContext->ThrowNativeError("Invalid Property type %d", params[2]);
		}
	}

	return 1;
}

static cell_t GetEntProp(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity;
	char *prop;
	int offset;
	edict_t *pEdict;
	int bit_count;
	bool is_unsigned = false;

	int element = 0;
	if (params[0] >= 5)
	{
		element = params[5];
	}

	if (!IndexToAThings(params[1], &pEntity, &pEdict))
	{
		return pContext->ThrowNativeError("Entity %d (%d) is invalid", g_HL2.ReferenceToIndex(params[1]), params[1]);
	}

	pContext->LocalToString(params[3], &prop);

	switch (params[2])
	{
	case Prop_Data:
		{
			typedescription_t *td;

			FIND_PROP_DATA(td);

			if ((bit_count = MatchTypeDescAsInteger(td->fieldType, td->flags)) == 0)
			{
				return pContext->ThrowNativeError("Data field %s is not an integer (%d)", 
					prop,
					td->fieldType);
			}

			CHECK_SET_PROP_DATA_OFFSET();
			
			if (td->fieldType == FIELD_CUSTOM && (td->flags & FTYPEDESC_OUTPUT) == FTYPEDESC_OUTPUT)
			{
				auto *pVariant = (variant_t *)((intptr_t)pEntity + offset);
				if ((bit_count = MatchTypeDescAsInteger(pVariant->fieldType, 0)) == 0)
				{
					return pContext->ThrowNativeError("Variant value for %s is not an integer (%d)",
						prop,
						pVariant->fieldType);
				} \
			}

			break;
		}
	case Prop_Send:
		{
			FIND_PROP_SEND(DPT_Int, "integer");
			is_unsigned = ((pProp->GetFlags() & SPROP_UNSIGNED) == SPROP_UNSIGNED);

			// This isn't in CS:S yet, but will be, doesn't hurt to add now, and will save us a build later
#if SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_HL2DM || SOURCE_ENGINE == SE_DODS \
	|| SOURCE_ENGINE == SE_BMS || SOURCE_ENGINE == SE_SDK2013 || SOURCE_ENGINE == SE_TF2 \
	|| SOURCE_ENGINE == SE_CSGO || SOURCE_ENGINE == SE_BLADE
			if (pProp->GetFlags() & SPROP_VARINT)
			{
				bit_count = sizeof(int) * 8;
			}
#endif
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
		if (is_unsigned)
		{
			return *(uint16_t *)((uint8_t *)pEntity + offset);
		}
		else
		{
			return *(int16_t *)((uint8_t *)pEntity + offset);
		}
	}
	else if (bit_count >= 2)
	{
		if (is_unsigned)
		{
			return *(uint8_t *)((uint8_t *)pEntity + offset);
		}
		else
		{
			return *(int8_t *)((uint8_t *)pEntity + offset);
		}
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
	edict_t *pEdict;
	int bit_count;

	int element = 0;
	if (params[0] >= 6)
	{
		element = params[6];
	}

	if (!IndexToAThings(params[1], &pEntity, &pEdict))
	{
		return pContext->ThrowNativeError("Entity %d (%d) is invalid", g_HL2.ReferenceToIndex(params[1]), params[1]);
	}

	pContext->LocalToString(params[3], &prop);

	switch (params[2])
	{
	case Prop_Data:
		{
			typedescription_t *td;

			FIND_PROP_DATA(td);

			if ((bit_count = MatchTypeDescAsInteger(td->fieldType, td->flags)) == 0)
			{
				return pContext->ThrowNativeError("Data field %s is not an integer (%d)", 
					prop,
					td->fieldType);
			}

			CHECK_SET_PROP_DATA_OFFSET();

			if (td->fieldType == FIELD_CUSTOM && (td->flags & FTYPEDESC_OUTPUT) == FTYPEDESC_OUTPUT)
			{
				auto *pVariant = (variant_t *)((intptr_t)pEntity + offset);
				// These are the only three int-ish types that variants support. If set to valid one that isn't
				// (32-bit) integer, leave it alone. It's probably the intended type.
				if (pVariant->fieldType != FIELD_COLOR32 && pVariant->fieldType != FIELD_BOOLEAN)
				{
					pVariant->fieldType = FIELD_INTEGER;
				}

				bit_count = MatchTypeDescAsInteger(pVariant->fieldType, 0);
			}

			SET_TYPE_IF_VARIANT(FIELD_INTEGER);

			break;
		}
	case Prop_Send:
		{
			FIND_PROP_SEND(DPT_Int, "integer");
			if (!CanSetPropName(prop))
			{
				return pContext->ThrowNativeError("Cannot set %s with \"FollowCSGOServerGuidelines\" option enabled.", prop);
			}

			// This isn't in CS:S yet, but will be, doesn't hurt to add now, and will save us a build later
#if SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_HL2DM || SOURCE_ENGINE == SE_DODS \
	|| SOURCE_ENGINE == SE_BMS || SOURCE_ENGINE == SE_SDK2013 || SOURCE_ENGINE == SE_TF2 \
	|| SOURCE_ENGINE == SE_CSGO || SOURCE_ENGINE == SE_BLADE
			if (pProp->GetFlags() & SPROP_VARINT)
			{
				bit_count = sizeof(int) * 8;
			}
#endif
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
	
	if (params[2] == Prop_Send && (pEdict != NULL))
	{
		g_HL2.SetEdictStateChanged(pEdict, offset);
	}

	return 0;
}

static cell_t GetEntPropFloat(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity;
	char *prop;
	int offset;
	int bit_count;
	edict_t *pEdict;

	int element = 0;
	if (params[0] >= 4)
	{
		element = params[4];
	}

	if (!IndexToAThings(params[1], &pEntity, &pEdict))
	{
		return pContext->ThrowNativeError("Entity %d (%d) is invalid", g_HL2.ReferenceToIndex(params[1]), params[1]);
	}

	pContext->LocalToString(params[3], &prop);

	switch (params[2])
	{
	case Prop_Data:
		{
			typedescription_t *td;

			FIND_PROP_DATA(td);

			if (td->fieldType != FIELD_FLOAT
				&& td->fieldType != FIELD_TIME
				&& (td->fieldType != FIELD_CUSTOM || (td->flags & FTYPEDESC_OUTPUT) != FTYPEDESC_OUTPUT))
			{
				return pContext->ThrowNativeError("Data field %s is not a float (%d != [%d,%d,%d])", 
					prop,
					td->fieldType,
					FIELD_FLOAT,
					FIELD_TIME,
					FIELD_CUSTOM);
			}

			CHECK_SET_PROP_DATA_OFFSET();

			CHECK_TYPE_VALID_IF_VARIANT(FIELD_FLOAT, "float");

			break;
		}
	case Prop_Send:
		{
			FIND_PROP_SEND(DPT_Float, "float");
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
	int bit_count;
	edict_t *pEdict;

	int element = 0;
	if (params[0] >= 5)
	{
		element = params[5];
	}

	if (!IndexToAThings(params[1], &pEntity, &pEdict))
	{
		return pContext->ThrowNativeError("Entity %d (%d) is invalid", g_HL2.ReferenceToIndex(params[1]), params[1]);
	}

	pContext->LocalToString(params[3], &prop);

	switch (params[2])
	{
	case Prop_Data:
		{
			typedescription_t *td;

			FIND_PROP_DATA(td);

			if (td->fieldType != FIELD_FLOAT
				&& td->fieldType != FIELD_TIME
				&& (td->fieldType != FIELD_CUSTOM || (td->flags & FTYPEDESC_OUTPUT) != FTYPEDESC_OUTPUT))
			{
				return pContext->ThrowNativeError("Data field %s is not a float (%d != [%d,%d,%d])", 
					prop,
					td->fieldType,
					FIELD_FLOAT,
					FIELD_TIME,
					FIELD_CUSTOM);
			}

			CHECK_SET_PROP_DATA_OFFSET();

			SET_TYPE_IF_VARIANT(FIELD_FLOAT);

			break;
		}
	case Prop_Send:
		{
			FIND_PROP_SEND(DPT_Float, "float");
			if (!CanSetPropName(prop))
			{
				return pContext->ThrowNativeError("Cannot set %s with \"FollowCSGOServerGuidelines\" option enabled.", prop);
			}
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

enum PropEntType
{
	PropEnt_Unknown,
	PropEnt_Handle,
	PropEnt_Entity,
	PropEnt_Edict,
	PropEnt_Variant,
};

static cell_t GetEntPropEnt(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity;
	char *prop;
	int offset;
	int bit_count;
	edict_t *pEdict;
	PropEntType type = PropEnt_Unknown;

	int element = 0;
	if (params[0] >= 4)
 	{
		element = params[4];
 	}

	if (!IndexToAThings(params[1], &pEntity, &pEdict))
	{
		return pContext->ThrowNativeError("Entity %d (%d) is invalid", g_HL2.ReferenceToIndex(params[1]), params[1]);
	}

	pContext->LocalToString(params[3], &prop);

	switch (params[2])
	{
	case Prop_Data:
		{
			typedescription_t *td;

			FIND_PROP_DATA(td);

			switch (td->fieldType)
			{
			case FIELD_EHANDLE:
				type = PropEnt_Handle;
				break;
			case FIELD_CLASSPTR:
				type = PropEnt_Entity;
				break;
			case FIELD_EDICT:
				type = PropEnt_Edict;
				break;
			case FIELD_CUSTOM:
				if ((td->flags & FTYPEDESC_OUTPUT) == FTYPEDESC_OUTPUT)
				{
					type = PropEnt_Variant;
				}
				break;
			}

			if (type == PropEnt_Unknown)
			{
				return pContext->ThrowNativeError("Data field %s is not an entity nor edict (%d)",
					prop,
					td->fieldType);
			}

			CHECK_SET_PROP_DATA_OFFSET();

			CHECK_TYPE_VALID_IF_VARIANT(FIELD_EHANDLE, "ehandle");

			break;
		}
	case Prop_Send:
		{
			type = PropEnt_Handle;
			FIND_PROP_SEND(DPT_Int, "integer");
			break;
		}
	default:
		{
			return pContext->ThrowNativeError("Invalid Property type %d", params[2]);
		}
	}

	switch (type)
	{
	case PropEnt_Handle:
	case PropEnt_Variant:
		{
			CBaseHandle *hndl;
			if (type == PropEnt_Handle)
			{
				hndl = (CBaseHandle *)((uint8_t *)pEntity + offset);
			}
			else // PropEnt_Variant
			{
				auto *pVariant = (variant_t *)((intptr_t)pEntity + offset);
				hndl = &pVariant->eVal;
			}

			CBaseEntity *pHandleEntity = g_HL2.ReferenceToEntity(hndl->GetEntryIndex());

			if (!pHandleEntity || *hndl != reinterpret_cast<IHandleEntity *>(pHandleEntity)->GetRefEHandle())
				return -1;

			return g_HL2.EntityToBCompatRef(pHandleEntity);
		}
	case PropEnt_Entity:
		{
			CBaseEntity *pPropEntity = *(CBaseEntity **) ((uint8_t *) pEntity + offset);
			return g_HL2.EntityToBCompatRef(pPropEntity);
		}
	case PropEnt_Edict:
		{
			edict_t *pEdict = *(edict_t **) ((uint8_t *) pEntity + offset);
			if (!pEdict || pEdict->IsFree())
				return -1;

			return IndexOfEdict(pEdict);
		}
	}
	
	return -1;
}

static cell_t SetEntPropEnt(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity;
	char *prop;
	int offset;
	int bit_count;
	edict_t *pEdict;
	PropEntType type = PropEnt_Unknown;

	int element = 0;
	if (params[0] >= 5)
 	{
		element = params[5];
 	}

	if (!IndexToAThings(params[1], &pEntity, &pEdict))
	{
		return pContext->ThrowNativeError("Entity %d (%d) is invalid", g_HL2.ReferenceToIndex(params[1]), params[1]);
	}

	pContext->LocalToString(params[3], &prop);

	switch (params[2])
	{
	case Prop_Data:
		{
			typedescription_t *td;

			FIND_PROP_DATA(td);

			switch (td->fieldType)
			{
			case FIELD_EHANDLE:
				type = PropEnt_Handle;
				break;
			case FIELD_CLASSPTR:
				type = PropEnt_Entity;
				break;
			case FIELD_EDICT:
				type = PropEnt_Edict;
				if (!pEdict)
					return pContext->ThrowNativeError("Edict %d is invalid", params[1]);
				break;
			case FIELD_CUSTOM:
				if ((td->flags & FTYPEDESC_OUTPUT) == FTYPEDESC_OUTPUT)
				{
					type = PropEnt_Variant;
				}
				break;				
			}

			if (type == PropEnt_Unknown)
			{
				return pContext->ThrowNativeError("Data field %s is not an entity nor edict (%d)",
					prop,
					td->fieldType);
			}

			CHECK_SET_PROP_DATA_OFFSET();

			SET_TYPE_IF_VARIANT(FIELD_EHANDLE);

			break;
		}
	case Prop_Send:
		{
			type = PropEnt_Handle;
			FIND_PROP_SEND(DPT_Int, "integer");
			break;
		}
	default:
		{
			return pContext->ThrowNativeError("Invalid Property type %d", params[2]);
		}
	}

	CBaseEntity *pOther = GetEntity(params[4]);
	if (!pOther && params[4] != -1)
	{
		return pContext->ThrowNativeError("Entity %d (%d) is invalid", g_HL2.ReferenceToIndex(params[4]), params[4]);
	}

	switch (type)
	{
	case PropEnt_Handle:
	case PropEnt_Variant:
		{
			CBaseHandle *hndl;
			if (type == PropEnt_Handle)
			{
				hndl = (CBaseHandle *)((uint8_t *)pEntity + offset);
			}
			else // PropEnt_Variant
			{
				auto *pVariant = (variant_t *)((intptr_t)pEntity + offset);
				hndl = &pVariant->eVal;
			}

			hndl->Set((IHandleEntity *) pOther);

			if (params[2] == Prop_Send && (pEdict != NULL))
			{
				g_HL2.SetEdictStateChanged(pEdict, offset);
			}
		}

		break;

	case PropEnt_Entity:
		{
			*(CBaseEntity **) ((uint8_t *) pEntity + offset) = pOther;
			break;
		}

	case PropEnt_Edict:
		{
			edict_t *pOtherEdict = NULL;
			if (pOther)
			{
				IServerNetworkable *pNetworkable = ((IServerUnknown *) pOther)->GetNetworkable();
				if (!pNetworkable)
				{
					return pContext->ThrowNativeError("Entity %d (%d) does not have a valid edict", g_HL2.ReferenceToIndex(params[4]), params[4]);
				}

				pOtherEdict = pNetworkable->GetEdict();
				if (!pOtherEdict || pOtherEdict->IsFree())
				{
					return pContext->ThrowNativeError("Entity %d (%d) does not have a valid edict", g_HL2.ReferenceToIndex(params[4]), params[4]);
				}
			}

			*(edict_t **) ((uint8_t *) pEntity + offset) = pOtherEdict;
			break;
		}
	}

	return 1;
}

static cell_t GetEntPropVector(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity;
	char *prop;
	int offset;
	int bit_count;
	edict_t *pEdict;

	int element = 0;
	if (params[0] >= 5)
 	{
		element = params[5];
 	}
	
	if (!IndexToAThings(params[1], &pEntity, &pEdict))
	{
		return pContext->ThrowNativeError("Entity %d (%d) is invalid", g_HL2.ReferenceToIndex(params[1]), params[1]);
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

			CHECK_SET_PROP_DATA_OFFSET();

			if (td->fieldType == FIELD_CUSTOM && (td->flags & FTYPEDESC_OUTPUT) == FTYPEDESC_OUTPUT)
			{
				auto *pVariant = (variant_t *)((intptr_t)pEntity + offset);
				if (pVariant->fieldType != FIELD_VECTOR && pVariant->fieldType != FIELD_POSITION_VECTOR)
				{
					return pContext->ThrowNativeError("Variant value for %s is not vector (%d)",
						prop,
						pVariant->fieldType);
				}
			}

			break;
		}
	case Prop_Send:
		{
			FIND_PROP_SEND(DPT_Vector, "vector");
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
	int bit_count;
	edict_t *pEdict;

	int element = 0;
	if (params[0] >= 5)
 	{
		element = params[5];
 	}
	
	if (!IndexToAThings(params[1], &pEntity, &pEdict))
	{
		return pContext->ThrowNativeError("Entity %d (%d) is invalid", g_HL2.ReferenceToIndex(params[1]), params[1]);
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

			CHECK_SET_PROP_DATA_OFFSET();

			if (td->fieldType == FIELD_CUSTOM && (td->flags & FTYPEDESC_OUTPUT) == FTYPEDESC_OUTPUT)
			{
				auto *pVariant = (variant_t *)((intptr_t)pEntity + offset);
				// Both of these are supported and we don't know which is intended. But, if it's already
				// a pos vector, we probably want to keep that.
				if (pVariant->fieldType != FIELD_POSITION_VECTOR)
				{
					pVariant->fieldType = FIELD_VECTOR;
				}
			}

			break;
		}
	case Prop_Send:
		{
			FIND_PROP_SEND(DPT_Vector, "vector");			
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
	int bit_count;
	edict_t *pEdict;

	int element = 0;
	if (params[0] >= 6)
 	{
		element = params[6];
 	}
	
	if (!IndexToAThings(params[1], &pEntity, &pEdict))
	{
		return pContext->ThrowNativeError("Entity %d (%d) is invalid", g_HL2.ReferenceToIndex(params[1]), params[1]);
	}

	pContext->LocalToString(params[3], &prop);

	const char *src;

	switch (params[2])
	{
	case Prop_Data:
		{
			bool bIsStringIndex = false;
			typedescription_t *td;
			
			FIND_PROP_DATA(td);

			if ((td->fieldType != FIELD_CHARACTER
				&& td->fieldType != FIELD_STRING
				&& td->fieldType != FIELD_MODELNAME 
				&& td->fieldType != FIELD_SOUNDNAME)
				|| (td->fieldType == FIELD_CUSTOM && (td->flags & FTYPEDESC_OUTPUT) != FTYPEDESC_OUTPUT))
			{
				return pContext->ThrowNativeError("Data field %s is not a string (%d != %d)", 
					prop,
					td->fieldType,
					FIELD_CHARACTER);
			}

			bIsStringIndex = (td->fieldType != FIELD_CHARACTER);

			if (element != 0)
			{
				if (bIsStringIndex)
				{
					if (element < 0 || element >= td->fieldSize)
					{
						return pContext->ThrowNativeError("Element %d is out of bounds (Prop %s has %d elements).",
							element,
							prop,
							td->fieldSize);
					}
				}
				else
				{
					return pContext->ThrowNativeError("Prop %s is not an array. Element %d is invalid.",
						prop,
						element);
				}
			}

			offset = info.actual_offset;
			if (bIsStringIndex)
			{
				offset += (element * (td->fieldSizeInBytes / td->fieldSize));

				string_t idx;

				idx = *(string_t *) ((uint8_t *) pEntity + offset);
				src = (idx == NULL_STRING) ? "" : STRING(idx);
			}
			else
			{
				src = (char *) ((uint8_t *) pEntity + offset);
			}
			break;
		}
	case Prop_Send:
		{
			FIND_PROP_SEND(DPT_String, "string");

			if (pProp->GetProxyFn())
			{
				DVariant var;
				pProp->GetProxyFn()(pProp, pEntity, (const void *) ((intptr_t) pEntity + offset), &var, element, params[1]);
				src = var.m_pString;
			}
			else
			{
				src = *(char **) ((uint8_t *) pEntity + offset);
			}
			
			break;
		}
	default:
		{
			return pContext->ThrowNativeError("Invalid Property type %d", params[2]);
		}
	}

	if (src)
	{
		size_t len;
		pContext->StringToLocalUTF8(params[4], params[5], src, &len);
		return len;
	}

	pContext->StringToLocal(params[4], params[5], "");
	return 0;
}

static cell_t SetEntPropString(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity;
	char *prop;
	int offset;
	int maxlen;
	int bit_count;
	edict_t *pEdict;
	bool bIsStringIndex;

	int element = 0;
	if (params[0] >= 5)
	{
		element = params[5];
	}

	if (!IndexToAThings(params[1], &pEntity, &pEdict))
	{
		return pContext->ThrowNativeError("Entity %d (%d) is invalid", g_HL2.ReferenceToIndex(params[1]), params[1]);
	}

	switch (params[2])
	{
	case Prop_Data:
		{
			datamap_t *pMap;
			if ((pMap=CBaseEntity_GetDataDescMap(pEntity)) == NULL)
			{
				return pContext->ThrowNativeError("Unable to retrieve GetDataDescMap offset");
			}
			pContext->LocalToString(params[3], &prop);
			sm_datatable_info_t info;
			if (!g_HL2.FindDataMapInfo(pMap, prop, &info))
			{
				return pContext->ThrowNativeError("Property \"%s\" not found for entity %d", prop, params[1]);
			}
			
			typedescription_t *td = info.prop;
			if ((td->fieldType != FIELD_CHARACTER
				&& td->fieldType != FIELD_STRING
				&& td->fieldType != FIELD_MODELNAME
				&& td->fieldType != FIELD_SOUNDNAME)
				|| (td->fieldType == FIELD_CUSTOM && (td->flags & FTYPEDESC_OUTPUT) != FTYPEDESC_OUTPUT))
			{
				return pContext->ThrowNativeError("Property \"%s\" is not a valid string", prop);
			}

			offset = info.actual_offset;

			bIsStringIndex = (td->fieldType != FIELD_CHARACTER);

			if (element != 0)
			{
				if (bIsStringIndex)
				{
					if (element < 0 || element >= td->fieldSize)
					{
						return pContext->ThrowNativeError("Element %d is out of bounds (Prop %s has %d elements).",
							element,
							prop,
							td->fieldSize);
					}
				}
				else
				{
					return pContext->ThrowNativeError("Prop %s is not an array. Element %d is invalid.",
						prop,
						element);
				}
			}

			if (!CanSetPropName(prop))
			{
				return pContext->ThrowNativeError("Cannot set %s with \"FollowCSGOServerGuidelines\" option enabled.", prop);
			}

			if (bIsStringIndex)
			{
				offset += (element * (td->fieldSizeInBytes / td->fieldSize));
			}
			else
			{
				maxlen = td->fieldSize;
			}

			SET_TYPE_IF_VARIANT(FIELD_STRING);

			break;
		}
	case Prop_Send:
		{
			pContext->LocalToString(params[3], &prop);
			FIND_PROP_SEND(DPT_String, "string");
			if (!CanSetPropName(prop))
			{
				return pContext->ThrowNativeError("Cannot set %s with \"FollowCSGOServerGuidelines\" option enabled.", prop);
			}

			bIsStringIndex = false;
			if (pProp->GetProxyFn())
			{
				DVariant var;
				pProp->GetProxyFn()(pProp, pEntity, (const void *) ((intptr_t) pEntity + offset), &var, element, params[1]);
				if (var.m_pString == ((string_t *) ((intptr_t) pEntity + offset))->ToCStr())
				{
					bIsStringIndex = true;
				}
			}

			// Only used if not string index.
			maxlen = DT_MAX_STRING_BUFFERSIZE;
			
			break;
		}
	default:
		{
			return pContext->ThrowNativeError("Invalid Property type %d", params[2]);
		}
	}

	char *src;
	size_t len;
	pContext->LocalToString(params[4], &src);

	if (bIsStringIndex)
	{
		*(string_t *) ((intptr_t) pEntity + offset) = g_HL2.AllocPooledString(src);
		len = strlen(src);
	}
	else
	{
		char *dest = (char *) ((uint8_t *) pEntity + offset);
		len = ke::SafeStrcpy(dest, maxlen, src);
	}

	if (params[2] == Prop_Send && (pEdict != NULL))
	{
		g_HL2.SetEdictStateChanged(pEdict, offset);
	}

	return len;
}

static int32_t SDKEntFlagToSMEntFlag(int flag)
{
	switch (flag)
	{
		case FL_ONGROUND:
			return ENTFLAG_ONGROUND;
		case FL_DUCKING:
			return ENTFLAG_DUCKING;
		case FL_WATERJUMP:
			return ENTFLAG_WATERJUMP;
		case FL_ONTRAIN:
			return ENTFLAG_ONTRAIN;
		case FL_INRAIN:
			return ENTFLAG_INRAIN;
		case FL_FROZEN:
			return ENTFLAG_FROZEN;
		case FL_ATCONTROLS:
			return ENTFLAG_ATCONTROLS;
		case FL_CLIENT:
			return ENTFLAG_CLIENT;
		case FL_FAKECLIENT:
			return ENTFLAG_FAKECLIENT;
		case FL_INWATER:
			return ENTFLAG_INWATER;
		case FL_FLY:
			return ENTFLAG_FLY;
		case FL_SWIM:
			return ENTFLAG_SWIM;
		case FL_CONVEYOR:
			return ENTFLAG_CONVEYOR;
		case FL_NPC:
			return ENTFLAG_NPC;
		case FL_GODMODE:
			return ENTFLAG_GODMODE;
		case FL_NOTARGET:
			return ENTFLAG_NOTARGET;
		case FL_AIMTARGET:
			return ENTFLAG_AIMTARGET;
		case FL_PARTIALGROUND:
			return ENTFLAG_PARTIALGROUND;
		case FL_STATICPROP:
			return ENTFLAG_STATICPROP;
		case FL_GRAPHED:
			return ENTFLAG_GRAPHED;
		case FL_GRENADE:
			return ENTFLAG_GRENADE;
		case FL_STEPMOVEMENT:
			return ENTFLAG_STEPMOVEMENT;
		case FL_DONTTOUCH:
			return ENTFLAG_DONTTOUCH;
		case FL_BASEVELOCITY:
			return ENTFLAG_BASEVELOCITY;
		case FL_WORLDBRUSH:
			return ENTFLAG_WORLDBRUSH;
		case FL_OBJECT:
			return ENTFLAG_OBJECT;
		case FL_KILLME:
			return ENTFLAG_KILLME;
		case FL_ONFIRE:
			return ENTFLAG_ONFIRE;
		case FL_DISSOLVING:
			return ENTFLAG_DISSOLVING;
		case FL_TRANSRAGDOLL:
			return ENTFLAG_TRANSRAGDOLL;
		case FL_UNBLOCKABLE_BY_PLAYER:
			return ENTFLAG_UNBLOCKABLE_BY_PLAYER;
#if SOURCE_ENGINE == SE_ALIENSWARM
		case FL_FREEZING:
			return ENTFLAG_FREEZING;
#elif SOURCE_ENGINE == SE_HL2DM || SOURCE_ENGINE == SE_DODS || SOURCE_ENGINE == SE_SDK2013 \
	|| SOURCE_ENGINE == SE_BMS || SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_TF2
		case FL_EP2V_UNKNOWN:
			return ENTFLAG_EP2V_UNKNOWN1;
#endif
		default:
			return 0;
	}
}

static int32_t SMEntFlagToSDKEntFlag(int32_t flag)
{
	switch (flag)
	{
		case ENTFLAG_ONGROUND:
			return FL_ONGROUND;
		case ENTFLAG_DUCKING:
			return FL_DUCKING;
		case ENTFLAG_WATERJUMP:
			return FL_WATERJUMP;
		case ENTFLAG_ONTRAIN:
			return FL_ONTRAIN;
		case ENTFLAG_INRAIN:
			return FL_INRAIN;
		case ENTFLAG_FROZEN:
			return FL_FROZEN;
		case ENTFLAG_ATCONTROLS:
			return FL_ATCONTROLS;
		case ENTFLAG_CLIENT:
			return FL_CLIENT;
		case ENTFLAG_FAKECLIENT:
			return FL_FAKECLIENT;
		case ENTFLAG_INWATER:
			return FL_INWATER;
		case ENTFLAG_FLY:
			return FL_FLY;
		case ENTFLAG_SWIM:
			return FL_SWIM;
		case ENTFLAG_CONVEYOR:
			return FL_CONVEYOR;
		case ENTFLAG_NPC:
			return FL_NPC;
		case ENTFLAG_GODMODE:
			return FL_GODMODE;
		case ENTFLAG_NOTARGET:
			return FL_NOTARGET;
		case ENTFLAG_AIMTARGET:
			return FL_AIMTARGET;
		case ENTFLAG_PARTIALGROUND:
			return FL_PARTIALGROUND;
		case ENTFLAG_STATICPROP:
			return FL_STATICPROP;
		case ENTFLAG_GRAPHED:
			return FL_GRAPHED;
		case ENTFLAG_GRENADE:
			return FL_GRENADE;
		case ENTFLAG_STEPMOVEMENT:
			return FL_STEPMOVEMENT;
		case ENTFLAG_DONTTOUCH:
			return FL_DONTTOUCH;
		case ENTFLAG_BASEVELOCITY:
			return FL_BASEVELOCITY;
		case ENTFLAG_WORLDBRUSH:
			return FL_WORLDBRUSH;
		case ENTFLAG_OBJECT:
			return FL_OBJECT;
		case ENTFLAG_KILLME:
			return FL_KILLME;
		case ENTFLAG_ONFIRE:
			return FL_ONFIRE;
		case ENTFLAG_DISSOLVING:
			return FL_DISSOLVING;
		case ENTFLAG_TRANSRAGDOLL:
			return FL_TRANSRAGDOLL;
		case ENTFLAG_UNBLOCKABLE_BY_PLAYER:
			return FL_UNBLOCKABLE_BY_PLAYER;
#if SOURCE_ENGINE == SE_ALIENSWARM
		case ENTFLAG_FREEZING:
			return FL_FREEZING;
#elif SOURCE_ENGINE == SE_HL2DM || SOURCE_ENGINE == SE_DODS || SOURCE_ENGINE == SE_SDK2013 \
	|| SOURCE_ENGINE == SE_BMS || SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_TF2
		case ENTFLAG_EP2V_UNKNOWN1:
			return FL_EP2V_UNKNOWN;
#endif
		default:
			return 0;
	}
}

static cell_t GetEntityFlags(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity = g_HL2.ReferenceToEntity(params[1]);
	if (!pEntity)
	{
		return pContext->ThrowNativeError("Entity %d (%d) is invalid", g_HL2.ReferenceToIndex(params[1]), params[1]);
	}

	const char *prop = g_pGameConf->GetKeyValue("m_fFlags");
	if (!prop)
	{
		return pContext->ThrowNativeError("Could not find m_fFlags prop in gamedata");
	}

	datamap_t *pMap;

	if ((pMap = CBaseEntity_GetDataDescMap(pEntity)) == NULL)
	{
		return pContext->ThrowNativeError("Could not retrieve datamap");
	}
	
	sm_datatable_info_t info;
	if (!g_HL2.FindDataMapInfo(pMap, prop, &info))
	{
		return pContext->ThrowNativeError("Property \"%s\" not found (entity %d)",
			prop,
			params[1]);
	}

	int offset = info.actual_offset;

	int32_t actual_flags = *(int32_t *)((uint8_t *)pEntity + offset);
	int32_t sm_flags = 0;

	for (int32_t i = 0; i < 32; i++)
	{
		int32_t flag = (1U<<i);
		if ((actual_flags & flag) == flag)
		{
			sm_flags |= SDKEntFlagToSMEntFlag(flag);
		}
	}

	return sm_flags;
}

static cell_t SetEntityFlags(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity = g_HL2.ReferenceToEntity(params[1]);
	if (!pEntity)
	{
		return pContext->ThrowNativeError("Entity %d (%d) is invalid", g_HL2.ReferenceToIndex(params[1]), params[1]);
	}

	const char *prop = g_pGameConf->GetKeyValue("m_fFlags");
	if (!prop)
	{
		return pContext->ThrowNativeError("Could not find m_fFlags prop in gamedata");
	}

	datamap_t *pMap;

	if ((pMap = CBaseEntity_GetDataDescMap(pEntity)) == NULL)
	{
		return pContext->ThrowNativeError("Could not retrieve datamap");
	}
	
	sm_datatable_info_t info;
	if (!g_HL2.FindDataMapInfo(pMap, prop, &info))
	{
		return pContext->ThrowNativeError("Property \"%s\" not found (entity %d)",
			prop,
			params[1]);
	}

	int offset = info.actual_offset;

	int32_t sm_flags = params[2];
	int32_t actual_flags = 0;

	for (int32_t i = 0; i < 32; i++)
	{
		int32_t flag = (1U<<i);
		if ((sm_flags & flag) == flag)
		{
			actual_flags |= SMEntFlagToSDKEntFlag(flag);
		}
	}

	*(int32_t *)((uint8_t *)pEntity + offset) = actual_flags;

	return 0;
}

static cell_t GetEntityAddress(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity * pEntity = GetEntity(params[1]);
	if (!pEntity)
	{
		return pContext->ThrowNativeError("Entity %d (%d) is invalid", g_HL2.ReferenceToIndex(params[1]), params[1]);
	}

#ifdef PLATFORM_X86
	return reinterpret_cast<cell_t>(pEntity);
#else
	return g_SourceMod.ToPseudoAddress(pEntity);
#endif
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
	{"GetEntPropArraySize",		GetEntPropArraySize},
	{"GetEntPropEnt",			GetEntPropEnt},
	{"GetEntPropFloat",			GetEntPropFloat},
	{"GetEntPropString",		GetEntPropString},
	{"GetEntPropVector",		GetEntPropVector},
	{"GetEntityCount",			GetEntityCount},
	{"GetEntityFlags",			GetEntityFlags},
	{"GetEntityNetClass",		GetEntityNetClass},
	{"GetMaxEntities",			GetMaxEntities},
	{"IsEntNetworkable",		IsEntNetworkable},
	{"IsValidEdict",			IsValidEdict},
	{"IsValidEntity",			IsValidEntity},
	{"RemoveEntity",			RemoveEntity},
	{"RemoveEdict",				RemoveEdict},
	{"SetEdictFlags",			SetEdictFlags},
	{"SetEntData",				SetEntData},
	{"SetEntDataEnt",			SetEntDataEnt},		/** DEPRECATED */
	{"SetEntDataEnt2",			SetEntDataEnt2},
	{"SetEntDataFloat",			SetEntDataFloat},
	{"SetEntDataVector",		SetEntDataVector},
	{"SetEntDataString",		SetEntDataString},
	{"SetEntityFlags",			SetEntityFlags},
	{"SetEntProp",				SetEntProp},
	{"SetEntPropEnt",			SetEntPropEnt},
	{"SetEntPropFloat",			SetEntPropFloat},
	{"SetEntPropString",		SetEntPropString},
	{"SetEntPropVector",		SetEntPropVector},
	{"GetEntityAddress",		GetEntityAddress},
	{"FindDataMapInfo",		FindDataMapInfo},
	{NULL,						NULL}
};
