/**
 * vim: set ts=4 :
 * ===============================================================
 * SourceMod SDK Tools Extension
 * Copyright (C) 2004-2007 AlliedModders LLC. All rights reserved.
 * ===============================================================
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Version: $Id$
 */

#include <sh_string.h>
#include "extension.h"
#include "vcallbuilder.h"
#include "vnatives.h"

List<ValveCall *> g_RegCalls;

inline void InitPass(ValvePassInfo &info, ValveType vtype, PassType type, unsigned int flags, unsigned int decflags=0)
{
	info.decflags = decflags;
	info.encflags = 0;
	info.flags = flags;
	info.type = type;
	info.vtype = vtype;
}

#define START_CALL() \
	unsigned char *vptr = pCall->stk_get();

#define FINISH_CALL_SIMPLE(vret) \
	pCall->call->Execute(vptr, vret); \
	pCall->stk_put(vptr);

#define ENCODE_VALVE_PARAM(num, which, vnum) \
	if (EncodeValveParam(pContext, \
			params[num], \
			pCall, \
			&pCall->which[vnum], \
			vptr) \
		== Data_Fail) \
	{ \
		return 0; \
	}

#define DECODE_VALVE_PARAM(num, which, vnum) \
	if (DecodeValveParam(pContext, \
			params[num], \
			pCall, \
			&pCall->which[vnum], \
			vptr) \
		== Data_Fail) \
	{ \
		return 0; \
	}

bool CreateBaseCall(const char *name,
						  ValveCallType vcalltype,
						  const ValvePassInfo *retinfo,
						  const ValvePassInfo params[],
						  unsigned int numParams,
						  ValveCall **vaddr)
{
	int offset;
	ValveCall *call;
	if (g_pGameConf->GetOffset(name, &offset))
	{
		call = CreateValveVCall(offset, vcalltype, retinfo, params, numParams);
		if (call)
		{
			g_RegCalls.push_back(call);
		}
		*vaddr = call;
		return true;
	} else {
		void *addr;
		if (g_pGameConf->GetMemSig(name, &addr))
		{
			call = CreateValveCall(addr, vcalltype, retinfo, params, numParams);
			if (call)
			{
				g_RegCalls.push_back(call);
			}
			*vaddr = call;
			return true;
		}
	}
	return false;
}


static cell_t RemovePlayerItem(IPluginContext *pContext, const cell_t *params)
{
	static ValveCall *pCall = NULL;
	if (!pCall)
	{
		ValvePassInfo pass[2];
		InitPass(pass[0], Valve_CBaseEntity, PassType_Basic, 0);
		InitPass(pass[1], Valve_Bool, PassType_Basic, 0);
		if (!CreateBaseCall("RemovePlayerItem", ValveCall_Player, &pass[1], &pass[0], 1, &pCall))
		{
			return pContext->ThrowNativeError("\"RemovePlayerItem\" not supported by this mod");
		} else if (!pCall) {
			return pContext->ThrowNativeError("\"RemovePlayerItem\" wrapper failed to initialized");
		}
	}

	bool ret;
	START_CALL();
	DECODE_VALVE_PARAM(1, thisinfo, 0);
	DECODE_VALVE_PARAM(2, vparams, 0);
	FINISH_CALL_SIMPLE(&ret);
	return ret ? 1 : 0;
}

static cell_t GiveNamedItem(IPluginContext *pContext, const cell_t *params)
{
	static ValveCall *pCall = NULL;
	if (!pCall)
	{
		ValvePassInfo pass[3];
		InitPass(pass[0], Valve_String, PassType_Basic, PASSFLAG_BYVAL);
		InitPass(pass[1], Valve_POD, PassType_Basic, PASSFLAG_BYVAL);
		InitPass(pass[2], Valve_CBaseEntity, PassType_Basic, PASSFLAG_BYVAL);
		if (!CreateBaseCall("GiveNamedItem", ValveCall_Player, &pass[2], pass, 2, &pCall))
		{
			return pContext->ThrowNativeError("\"GiveNamedItem\" not supported by this mod");
		} else if (!pCall) {
			return pContext->ThrowNativeError("\"GiveNamedItem\" wrapper failed to initialized");
		}
	}

	CBaseEntity *pEntity = NULL;
	START_CALL();
	DECODE_VALVE_PARAM(1, thisinfo, 0);
	DECODE_VALVE_PARAM(2, vparams, 0);
	DECODE_VALVE_PARAM(3, vparams, 1);
	FINISH_CALL_SIMPLE(&pEntity);

	if (pEntity == NULL)
	{
		return -1;
	}

	edict_t *pEdict = gameents->BaseEntityToEdict(pEntity);
	if (!pEdict)
	{
		return -1;
	}

	return engine->IndexOfEdict(pEdict);
}

static cell_t GetPlayerWeaponSlot(IPluginContext *pContext, const cell_t *params)
{
	static ValveCall *pCall = NULL;
	if (!pCall)
	{
		ValvePassInfo pass[2];
		InitPass(pass[0], Valve_POD, PassType_Basic, PASSFLAG_BYVAL);
		InitPass(pass[1], Valve_CBaseEntity, PassType_Basic, PASSFLAG_BYVAL);
		if (!CreateBaseCall("Weapon_GetSlot", ValveCall_Player, &pass[1], pass, 1, &pCall))
		{
			return pContext->ThrowNativeError("\"Weapon_GetSlot\" not supported by this mod");
		} else if (!pCall) {
			return pContext->ThrowNativeError("\"Weapon_GetSlot\" wrapper failed to initialized");
		}
	}

	CBaseEntity *pEntity;
	START_CALL();
	DECODE_VALVE_PARAM(1, thisinfo, 0);
	DECODE_VALVE_PARAM(2, vparams, 0);
	FINISH_CALL_SIMPLE(&pEntity);

	if (pEntity == NULL)
	{
		return -1;
	}

	edict_t *pEdict = gameents->BaseEntityToEdict(pEntity);
	if (!pEdict)
	{
		return -1;
	}

	return engine->IndexOfEdict(pEdict);
}

static cell_t IgnitePlayer(IPluginContext *pContext, const cell_t *params)
{
	static ValveCall *pCall = NULL;
	if (!pCall)
	{
		ValvePassInfo pass[4];
		InitPass(pass[0], Valve_Float, PassType_Float, PASSFLAG_BYVAL);
		InitPass(pass[1], Valve_Bool, PassType_Basic, PASSFLAG_BYVAL);
		InitPass(pass[2], Valve_Float, PassType_Float, PASSFLAG_BYVAL);
		InitPass(pass[3], Valve_Bool, PassType_Basic, PASSFLAG_BYVAL);
		if (!CreateBaseCall("Ignite", ValveCall_Entity, NULL, pass, 4, &pCall))
		{
			return pContext->ThrowNativeError("\"Ignite\" not supported by this mod");
		} else if (!pCall) {
			return pContext->ThrowNativeError("\"Ignite\" wrapper failed to initialized");
		}
	}

	START_CALL();
	DECODE_VALVE_PARAM(1, thisinfo, 0);
	DECODE_VALVE_PARAM(2, vparams, 0);
	DECODE_VALVE_PARAM(3, vparams, 1);
	DECODE_VALVE_PARAM(4, vparams, 2);
	DECODE_VALVE_PARAM(5, vparams, 3);
	FINISH_CALL_SIMPLE(NULL);

	return 1;
}

static cell_t ExtinguishPlayer(IPluginContext *pContext, const cell_t *params)
{
	static ValveCall *pCall = NULL;
	if (!pCall)
	{
		if (!CreateBaseCall("Extinguish", ValveCall_Entity, NULL, NULL, 0, &pCall))
		{
			return pContext->ThrowNativeError("\"Extinguish\" not supported by this mod");
		} else if (!pCall) {
			return pContext->ThrowNativeError("\"Extinguish\" wrapper failed to initialized");
		}
	}

	START_CALL();
	DECODE_VALVE_PARAM(1, thisinfo, 0);
	FINISH_CALL_SIMPLE(NULL);

	return 1;
}

static cell_t TeleportPlayer(IPluginContext *pContext, const cell_t *params)
{
	static ValveCall *pCall = NULL;
	if (!pCall)
	{
		ValvePassInfo pass[3];
		InitPass(pass[0], Valve_Vector, PassType_Basic, PASSFLAG_BYVAL, VDECODE_FLAG_ALLOWNULL);
		InitPass(pass[1], Valve_QAngle, PassType_Basic, PASSFLAG_BYVAL, VDECODE_FLAG_ALLOWNULL);
		InitPass(pass[2], Valve_Vector, PassType_Basic, PASSFLAG_BYVAL, VDECODE_FLAG_ALLOWNULL);
		if (!CreateBaseCall("Teleport", ValveCall_Entity, NULL, pass, 3, &pCall))
		{
			return pContext->ThrowNativeError("\"Teleport\" not supported by this mod");
		} else if (!pCall) {
			return pContext->ThrowNativeError("\"Teleport\" wrapper failed to initialized");
		}
	}

	START_CALL();
	DECODE_VALVE_PARAM(1, thisinfo, 0);
	DECODE_VALVE_PARAM(2, vparams, 0);
	DECODE_VALVE_PARAM(3, vparams, 1);
	DECODE_VALVE_PARAM(4, vparams, 2);
	FINISH_CALL_SIMPLE(NULL);

	return 1;
}

static cell_t ForcePlayerSuicide(IPluginContext *pContext, const cell_t *params)
{
	static ValveCall *pCall = NULL;
	if (!pCall)
	{
		if (!CreateBaseCall("CommitSuicide", ValveCall_Player, NULL, NULL, 0, &pCall))
		{
			return pContext->ThrowNativeError("\"CommitSuicide\" not supported by this mod");
		} else if (!pCall) {
			return pContext->ThrowNativeError("\"CommitSuicide\" wrapper failed to initialized");
		}
	}

	START_CALL();
	DECODE_VALVE_PARAM(1, thisinfo, 0);
	FINISH_CALL_SIMPLE(NULL);

	return 1;
}

static cell_t SetClientViewEntity(IPluginContext *pContext, const cell_t *params)
{
	IGamePlayer *player = playerhelpers->GetGamePlayer(params[1]);
	if (player == NULL)
	{
		return pContext->ThrowNativeError("Invalid client index %d", params[1]);
	}
	if (!player->IsInGame())
	{
		return pContext->ThrowNativeError("Client %d is not in game", params[1]);
	}

	edict_t *pEdict = engine->PEntityOfEntIndex(params[2]);
	if (!pEdict || pEdict->IsFree())
	{
		return pContext->ThrowNativeError("Entity %d is not valid", params[2]);
	}

	engine->SetView(player->GetEdict(), pEdict);

	return 1;
}

static String *g_lightstyle[MAX_LIGHTSTYLES] = {NULL};
static cell_t SetLightStyle(IPluginContext *pContext, const cell_t *params)
{
	if (!g_lightstyle)
	{
		/* We allocate and never free this because the Engine wants to hold onto it :\
		 * in theory we could hook light style and know whether we're supposed to free 
		 * this or not on shutdown, but for 4K of memory, it doesn't seem worth it yet.
		 * So, it's a :TODO:!
		 */
	}

	int style = params[1];
	if (style >= MAX_LIGHTSTYLES)
	{
		return pContext->ThrowNativeError("Light style %d is invalid (range: 0-%d)", style, MAX_LIGHTSTYLES - 1);
	}

	if (g_lightstyle[style] == NULL)
	{
		g_lightstyle[style] = new String();
	}

	char *str;
	pContext->LocalToString(params[2], &str);

	g_lightstyle[style]->assign(str);

	engine->LightStyle(style, str);

	return 1;
}

sp_nativeinfo_t g_Natives[] = 
{
	{"ExtinguishPlayer",	ExtinguishPlayer},
	{"ExtinguishEntity",	ExtinguishPlayer},
	{"ForcePlayerSuicide",	ForcePlayerSuicide},
	{"GivePlayerItem",		GiveNamedItem},
	{"GetPlayerWeaponSlot",	GetPlayerWeaponSlot},
	{"IgnitePlayer",		IgnitePlayer},
	{"IgniteEntity",		IgnitePlayer},
	{"RemovePlayerItem",	RemovePlayerItem},
	{"TeleportPlayer",		TeleportPlayer},
	{"TeleportEntity",		TeleportPlayer},
	{"SetClientViewEntity",	SetClientViewEntity},
	{"SetLightStyle",		SetLightStyle},
	{NULL,					NULL},
};

