/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod SDKTools Extension
 * Copyright (C) 2004-2007 AlliedModders LLC.  All rights reserved.
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

#include <stdlib.h>
#include <sh_string.h>
#include "extension.h"
#include "vcallbuilder.h"
#include "vnatives.h"
#include "vhelpers.h"
#include "vglobals.h"
#include "CellRecipientFilter.h"

List<ValveCall *> g_RegCalls;
List<ICallWrapper *> g_CallWraps;

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
		InitPass(pass[0], Valve_CBaseEntity, PassType_Basic, PASSFLAG_BYVAL);
		InitPass(pass[1], Valve_Bool, PassType_Basic, PASSFLAG_BYVAL);
		if (!CreateBaseCall("RemovePlayerItem", ValveCall_Player, &pass[1], pass, 1, &pCall))
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

static cell_t IgniteEntity(IPluginContext *pContext, const cell_t *params)
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

static cell_t ExtinguishEntity(IPluginContext *pContext, const cell_t *params)
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

static cell_t TeleportEntity(IPluginContext *pContext, const cell_t *params)
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
	int style = params[1];
	if (style >= MAX_LIGHTSTYLES)
	{
		return pContext->ThrowNativeError("Light style %d is invalid (range: 0-%d)", style, MAX_LIGHTSTYLES - 1);
	}

	if (g_lightstyle[style] == NULL)
	{
		/* We allocate and never free this because the Engine wants to hold onto it :\
		 * in theory we could hook light style and know whether we're supposed to free 
		 * this or not on shutdown, but for ~4K of memory MAX, it doesn't seem worth it yet.
		 * So, it's a :TODO:!
		 */
		g_lightstyle[style] = new String();
	}

	char *str;
	pContext->LocalToString(params[2], &str);

	g_lightstyle[style]->assign(str);

	engine->LightStyle(style, g_lightstyle[style]->c_str());

	return 1;
}

static cell_t SlapPlayer(IPluginContext *pContext, const cell_t *params)
{
	static bool s_slap_supported = false;
	static bool s_slap_setup = false;
	static ICallWrapper *s_teleport = NULL;
	static int s_health_offs = 0;
	static int s_sound_count = 0;
	static int s_frag_offs = 0;

	if (!s_slap_setup)
	{
		int tries = 0;

		s_slap_setup = true;

		if (IsTeleportSupported())
		{
			tries++;
		}
		if (IsGetVelocitySupported())
		{
			tries++;
		}

		/* Setup health */
		if (g_pGameConf->GetOffset("m_iHealth", &s_health_offs) && s_health_offs)
		{
			tries++;
		}

		if (tries == 3)
		{
			s_slap_supported = true;

			const char *key;
			if ((key = g_pGameConf->GetKeyValue("SlapSoundCount")) != NULL)
			{
				s_sound_count = atoi(key);
			}
		}
	}

	if (!s_slap_supported)
	{
		return pContext->ThrowNativeError("This function is not supported on this mod");
	}

	/* First check if the client is valid */
	int client = params[1];
	IGamePlayer *player = playerhelpers->GetGamePlayer(client);
	if (!player)
	{
		return pContext->ThrowNativeError("Client %d is not valid", client);
	} else if (!player->IsInGame()) {
		return pContext->ThrowNativeError("Client %d is not in game", client);
	}

	edict_t *pEdict = player->GetEdict();
	CBaseEntity *pEntity = pEdict->GetUnknown()->GetBaseEntity();

	/* See if we should be taking away health */
	bool should_slay = false;
	if (params[2])
	{
		int *health = (int *)((char *)pEntity + s_health_offs);
		if (*health - params[2] <= 0)
		{
			*health = 1;
			should_slay = true;
		} else {
			*health -= params[2];
		}
	}

	/* Teleport in a random direction - thank you, Mani!*/
	Vector velocity;
	GetVelocity(pEntity, &velocity, NULL);
	velocity.x += ((rand() % 180) + 50) * (((rand() % 2) == 1) ?  -1 : 1);
	velocity.y += ((rand() % 180) + 50) * (((rand() % 2) == 1) ?  -1 : 1);
	velocity.z += rand() % 200 + 100;
	Teleport(pEntity, NULL, NULL, &velocity);

	/* Play a random sound */
	if (params[3] && s_sound_count > 0)
	{
		char name[48];
		const char *sound_name;
		cell_t player_list[256], total_players = 0;
		int maxClients = playerhelpers->GetMaxClients();

		int r = (rand() % s_sound_count) + 1;
		snprintf(name, sizeof(name), "SlapSound%d", r);

		if ((sound_name = g_pGameConf->GetKeyValue(name)) != NULL)
		{
			IGamePlayer *other;
			for (int i=1; i<=maxClients; i++)
			{
				other = playerhelpers->GetGamePlayer(i);
				if (other->IsInGame())
				{
					player_list[total_players++] = i;
				}
			}

			const Vector & pos = pEdict->GetCollideable()->GetCollisionOrigin();
			CellRecipientFilter rf;
			rf.SetToReliable(true);
			rf.Initialize(player_list, total_players);
			engsound->EmitSound(rf, client, CHAN_AUTO, sound_name, VOL_NORM, ATTN_NORM, 0, PITCH_NORM, &pos);
		}
	}

	if (!s_frag_offs)
	{
		const char *frag_prop = g_pGameConf->GetKeyValue("m_iFrags");
		if (frag_prop)
		{
			datamap_t *pMap = gamehelpers->GetDataMap(pEntity);
			typedescription_t *pType = gamehelpers->FindInDataMap(pMap, frag_prop);
			if (pType != NULL)
			{
				s_frag_offs = pType->fieldOffset[TD_OFFSET_NORMAL];
			}
		}
		if (!s_frag_offs)
		{
			s_frag_offs = -1;
		}
	}

	int old_frags = 0;
	if (s_frag_offs > 0)
	{
		old_frags = *(int *)((char *)pEntity + s_frag_offs);
	}
	
	/* Force suicide */
	if (should_slay)
	{
		pluginhelpers->ClientCommand(pEdict, "kill\n");
	}

	if (s_frag_offs > 0)
	{
		*(int *)((char *)pEntity + s_frag_offs) = old_frags;
	}

	return 1;
}

static cell_t GetClientEyePosition(IPluginContext *pContext, const cell_t *params)
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

	Vector pos;
	serverClients->ClientEarPosition(player->GetEdict(), &pos);

	cell_t *addr;
	pContext->LocalToPhysAddr(params[2], &addr);
	addr[0] = sp_ftoc(pos.x);
	addr[1] = sp_ftoc(pos.y);
	addr[2] = sp_ftoc(pos.z);

	return 1;
}

static cell_t GetClientEyeAngles(IPluginContext *pContext, const cell_t *params)
{
	static ValveCall *pCall = NULL;
	if (!pCall)
	{
		ValvePassInfo retinfo[1];
		InitPass(retinfo[0], Valve_POD, PassType_Basic, PASSFLAG_BYVAL);
		if (!CreateBaseCall("EyeAngles", ValveCall_Player, retinfo, NULL, 0, &pCall))
		{
			return pContext->ThrowNativeError("\"EyeAngles\" not supported by this mod");
		} else if (!pCall) {
			return pContext->ThrowNativeError("\"EyeAngles\" wrapper failed to initialized");
		}
	}

	QAngle *ang;
	START_CALL();
	DECODE_VALVE_PARAM(1, thisinfo, 0);
	FINISH_CALL_SIMPLE(&ang);
	
	cell_t *addr;
	pContext->LocalToPhysAddr(params[2], &addr);
	addr[0] = sp_ftoc(ang->x);
	addr[1] = sp_ftoc(ang->y);
	addr[2] = sp_ftoc(ang->z);

	return 1;
}

static cell_t FindEntityByClassname(IPluginContext *pContext, const cell_t *params)
{
	static ValveCall *pCall = NULL;
	if (!pCall)
	{
		ValvePassInfo pass[3];
		InitPass(pass[0], Valve_CBaseEntity, PassType_Basic, PASSFLAG_BYVAL, VDECODE_FLAG_ALLOWNULL|VDECODE_FLAG_ALLOWWORLD);
		InitPass(pass[1], Valve_String, PassType_Basic, PASSFLAG_BYVAL);
		InitPass(pass[2], Valve_CBaseEntity, PassType_Basic, PASSFLAG_BYVAL);
		if (!CreateBaseCall("FindEntityByClassname", ValveCall_EntityList, &pass[2], pass, 2, &pCall))
		{
			return pContext->ThrowNativeError("\"FindEntityByClassname\" not supported by this mod");
		} else if (!pCall) {
			return pContext->ThrowNativeError("\"FindEntityByClassname\" wrapper failed to initialized");
		}
	}

	CBaseEntity *pEntity;
	START_CALL();
	*(void **)vptr = g_EntList; 
	DECODE_VALVE_PARAM(1, vparams, 0);
	DECODE_VALVE_PARAM(2, vparams, 1);
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

static cell_t IsPlayerAlive(IPluginContext *pContext, const cell_t *params)
{
	IGamePlayer *player = playerhelpers->GetGamePlayer(params[1]);
	if (player == NULL)
	{
		return pContext->ThrowNativeError("Invalid client index %d", params[1]);
	} else if (!player->IsInGame())
	{
		return pContext->ThrowNativeError("Client %d is not in game", params[1]);
	}

	edict_t *pEdict = player->GetEdict();
	CBaseEntity *pEntity = pEdict->GetUnknown()->GetBaseEntity();

	static int lifeState_off = 0;
	static bool lifeState_setup = false;

	if (!lifeState_setup)
	{
		lifeState_setup = true;
		g_pGameConf->GetOffset("m_lifeState", &lifeState_off);
	}

	if (!lifeState_off)
	{
		IPlayerInfo *info = playerinfomngr->GetPlayerInfo(pEdict);
		if (info)
		{
			return info->IsDead() ? 0 : 1;
		}

		 return pContext->ThrowNativeError("\"IsPlayerAlive\" not supported by this mod");
	}
	return (*((uint8_t *)pEntity + lifeState_off) == LIFE_ALIVE) ? 1: 0;
}

sp_nativeinfo_t g_Natives[] = 
{
	{"ExtinguishPlayer",		ExtinguishEntity},
	{"ExtinguishEntity",		ExtinguishEntity},
	{"ForcePlayerSuicide",		ForcePlayerSuicide},
	{"GivePlayerItem",			GiveNamedItem},
	{"GetPlayerWeaponSlot",		GetPlayerWeaponSlot},
	{"IgnitePlayer",			IgniteEntity},
	{"IgniteEntity",			IgniteEntity},
	{"RemovePlayerItem",		RemovePlayerItem},
	{"TeleportPlayer",			TeleportEntity},
	{"TeleportEntity",			TeleportEntity},
	{"SetClientViewEntity",		SetClientViewEntity},
	{"SetLightStyle",			SetLightStyle},
	{"SlapPlayer",				SlapPlayer},
	{"GetClientEyePosition",	GetClientEyePosition},
	{"GetClientEyeAngles",		GetClientEyeAngles},
	{"FindEntityByClassname",	FindEntityByClassname},
	{"IsPlayerAlive",			IsPlayerAlive},
	{NULL,						NULL},
};
