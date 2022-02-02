/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod SDKTools Extension
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

#include <stdlib.h>
#include <sh_string.h>
#include "extension.h"
#include "vcallbuilder.h"
#include "vnatives.h"
#include "vhelpers.h"
#include "vglobals.h"
#include "CellRecipientFilter.h"
#include <inetchannel.h>
#include <iclient.h>
#include "iserver.h"
#include "am-string.h"
#include <sm_argbuffer.h>

SourceHook::List<ValveCall *> g_RegCalls;
SourceHook::List<ICallWrapper *> g_CallWraps;

#define ENTINDEX_TO_CBASEENTITY(ref, buffer) \
	buffer = gamehelpers->ReferenceToEntity(ref); \
	if (!buffer) \
	{ \
		return pContext->ThrowNativeError("Entity %d (%d) is not a CBaseEntity", gamehelpers->ReferenceToIndex(ref), ref); \
	}

#define SET_VECTOR(addr, vec) \
	addr[0] = sp_ftoc(vec.x); \
	addr[1] = sp_ftoc(vec.y); \
	addr[2] = sp_ftoc(vec.z); 

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
		void *addr = NULL;
		if (g_pGameConf->GetMemSig(name, &addr) && addr != NULL)
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
			return pContext->ThrowNativeError("\"RemovePlayerItem\" wrapper failed to initialize");
		}
	}

	bool ret;
	START_CALL();
	DECODE_VALVE_PARAM(1, thisinfo, 0);
	DECODE_VALVE_PARAM(2, vparams, 0);
	FINISH_CALL_SIMPLE(&ret);
	return ret ? 1 : 0;
}

#if SOURCE_ENGINE == SE_CSGO
class CEconItemView;
static cell_t GiveNamedItem(IPluginContext *pContext, const cell_t *params)
{
	if (g_SdkTools.ShouldFollowCSGOServerGuidelines())
	{
		char *pWeaponName;
		pContext->LocalToString(params[2], &pWeaponName);

		// Don't allow knives other than weapon_knife,  weapon_knifegg, and weapon_knife_t.
		// Others follow pattern weapon_knife_*
		size_t len = strlen(pWeaponName);
		if (len >= 14 && strnicmp(pWeaponName, "weapon_knife_", 13) == 0 && !(pWeaponName[13] == 't' && pWeaponName[14] == '\0'))
		{
			return pContext->ThrowNativeError("Blocked giving of %s due to core.cfg option FollowCSGOServerGuidelines", pWeaponName);
		}
	}

	static ValveCall *pCall = NULL;
	if (!pCall)
	{
		ValvePassInfo pass[6];
		InitPass(pass[0], Valve_String, PassType_Basic, PASSFLAG_BYVAL);
		InitPass(pass[1], Valve_POD, PassType_Basic, PASSFLAG_BYVAL);
		InitPass(pass[2], Valve_Object, PassType_Basic, PASSFLAG_BYVAL);
		InitPass(pass[3], Valve_Bool, PassType_Basic, PASSFLAG_BYVAL);
		InitPass(pass[4], Valve_Vector, PassType_Basic, PASSFLAG_BYVAL);
		InitPass(pass[5], Valve_CBaseEntity, PassType_Basic, PASSFLAG_BYVAL);
		if (!CreateBaseCall("GiveNamedItem", ValveCall_Player, &pass[5], pass, 5, &pCall))
		{
			return pContext->ThrowNativeError("\"GiveNamedItem\" not supported by this mod");
		} else if (!pCall) {
			return pContext->ThrowNativeError("\"GiveNamedItem\" wrapper failed to initialize");
		}
	}

	CBaseEntity *pEntity = NULL;
	START_CALL();
	DECODE_VALVE_PARAM(1, thisinfo, 0);
	DECODE_VALVE_PARAM(2, vparams, 0);
	DECODE_VALVE_PARAM(3, vparams, 1);
	*(CEconItemView **)(vptr + pCall->vparams[2].offset) = NULL;
	*(bool *)(vptr + pCall->vparams[3].offset) = false;
	*(void **)(vptr + pCall->vparams[4].offset) = NULL;
	FINISH_CALL_SIMPLE(&pEntity);

	return gamehelpers->EntityToBCompatRef(pEntity);
}
#elif SOURCE_ENGINE == SE_BLADE
class CEconItemView;
static cell_t GiveNamedItem(IPluginContext *pContext, const cell_t *params)
{
	static ValveCall *pCall = NULL;
	if (!pCall)
	{
		ValvePassInfo pass[5];
		InitPass(pass[0], Valve_String, PassType_Basic, PASSFLAG_BYVAL);
		InitPass(pass[1], Valve_POD, PassType_Basic, PASSFLAG_BYVAL);
		InitPass(pass[2], Valve_Object, PassType_Basic, PASSFLAG_BYVAL);
		InitPass(pass[3], Valve_Bool, PassType_Basic, PASSFLAG_BYVAL);
		InitPass(pass[4], Valve_CBaseEntity, PassType_Basic, PASSFLAG_BYVAL);
		if (!CreateBaseCall("GiveNamedItem", ValveCall_Player, &pass[4], pass, 4, &pCall))
		{
			return pContext->ThrowNativeError("\"GiveNamedItem\" not supported by this mod");
		} else if (!pCall) {
			return pContext->ThrowNativeError("\"GiveNamedItem\" wrapper failed to initialize");
		}
	}

	CBaseEntity *pEntity = NULL;
	START_CALL();
	DECODE_VALVE_PARAM(1, thisinfo, 0);
	DECODE_VALVE_PARAM(2, vparams, 0);
	DECODE_VALVE_PARAM(3, vparams, 1);
	*(CEconItemView **)(vptr + pCall->vparams[2].offset) = NULL;
	*(bool *)(vptr + pCall->vparams[3].offset) = false;
	FINISH_CALL_SIMPLE(&pEntity);

	return gamehelpers->EntityToBCompatRef(pEntity);
}
#elif SOURCE_ENGINE == SE_BMS
// CBaseEntity	*GiveNamedItem( const char *szName, int iSubType = 0, int iPrimaryAmmo = -1, int iSecondaryAmmo = -1 )
static cell_t GiveNamedItem(IPluginContext *pContext, const cell_t *params)
{
	static ValveCall *pCall = NULL;
	if (!pCall)
	{
		ValvePassInfo pass[5];
		InitPass(pass[0], Valve_String, PassType_Basic, PASSFLAG_BYVAL);
		InitPass(pass[1], Valve_POD, PassType_Basic, PASSFLAG_BYVAL);
		InitPass(pass[2], Valve_POD, PassType_Basic, PASSFLAG_BYVAL);
		InitPass(pass[3], Valve_POD, PassType_Basic, PASSFLAG_BYVAL);
		InitPass(pass[4], Valve_CBaseEntity, PassType_Basic, PASSFLAG_BYVAL);
		if (!CreateBaseCall("GiveNamedItem", ValveCall_Player, &pass[4], pass, 4, &pCall))
		{
			return pContext->ThrowNativeError("\"GiveNamedItem\" not supported by this mod");
		} else if (!pCall) {
			return pContext->ThrowNativeError("\"GiveNamedItem\" wrapper failed to initialize");
		}
	}

	CBaseEntity *pEntity = NULL;
	START_CALL();
	DECODE_VALVE_PARAM(1, thisinfo, 0);
	DECODE_VALVE_PARAM(2, vparams, 0);
	DECODE_VALVE_PARAM(3, vparams, 1);
	*(int *)(vptr + pCall->vparams[2].offset) = -1;
	*(int *)(vptr + pCall->vparams[3].offset) = -1;
	FINISH_CALL_SIMPLE(&pEntity);

	return gamehelpers->EntityToBCompatRef(pEntity);
}
#elif SOURCE_ENGINE == SE_LEFT4DEAD2
// CBaseEntity *CTerrorPlayer::GiveNamedItem( const char *pchName, int iSubType, bool bForce, CBaseEntity *pUnk)
static cell_t GiveNamedItem(IPluginContext *pContext, const cell_t *params)
{
	static ValveCall *pCall = NULL;
	if (!pCall)
	{
		ValvePassInfo pass[5];
		InitPass(pass[0], Valve_String, PassType_Basic, PASSFLAG_BYVAL);
		InitPass(pass[1], Valve_POD, PassType_Basic, PASSFLAG_BYVAL);
		InitPass(pass[2], Valve_Bool, PassType_Basic, PASSFLAG_BYVAL);
		InitPass(pass[3], Valve_CBaseEntity, PassType_Basic, PASSFLAG_BYVAL);
		InitPass(pass[4], Valve_CBaseEntity, PassType_Basic, PASSFLAG_BYVAL);
		
		if (!CreateBaseCall("GiveNamedItem", ValveCall_Player, &pass[4], pass, 4, &pCall)) {
			return pContext->ThrowNativeError("\"GiveNamedItem\" not supported by this mod");
		} else if (!pCall) {
			return pContext->ThrowNativeError("\"GiveNamedItem\" wrapper failed to initialize");
		}
	}

	CBaseEntity *pEntity = NULL;
	START_CALL();
	DECODE_VALVE_PARAM(1, thisinfo, 0);
	DECODE_VALVE_PARAM(2, vparams, 0);
	DECODE_VALVE_PARAM(3, vparams, 1);
	*(bool *)(vptr + pCall->vparams[2].offset) = false;
	*(void **)(vptr + pCall->vparams[3].offset) = NULL;
	FINISH_CALL_SIMPLE(&pEntity);
	
	return gamehelpers->EntityToBCompatRef(pEntity);
}
#else
// CBaseEntity	*GiveNamedItem( const char *szName, int iSubType = 0 )
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
			return pContext->ThrowNativeError("\"GiveNamedItem\" wrapper failed to initialize");
		}
	}

	CBaseEntity *pEntity = NULL;
	START_CALL();
	DECODE_VALVE_PARAM(1, thisinfo, 0);
	DECODE_VALVE_PARAM(2, vparams, 0);
	DECODE_VALVE_PARAM(3, vparams, 1);
	FINISH_CALL_SIMPLE(&pEntity);

	return gamehelpers->EntityToBCompatRef(pEntity);
}
#endif

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
			return pContext->ThrowNativeError("\"Weapon_GetSlot\" wrapper failed to initialize");
		}
	}

	CBaseEntity *pEntity;
	START_CALL();
	DECODE_VALVE_PARAM(1, thisinfo, 0);
	DECODE_VALVE_PARAM(2, vparams, 0);
	FINISH_CALL_SIMPLE(&pEntity);

	return gamehelpers->EntityToBCompatRef(pEntity);
}

#if SOURCE_ENGINE != SE_DARKMESSIAH
static cell_t IgniteEntity(IPluginContext *pContext, const cell_t *params)
{
	static ValveCall *pCall = NULL;
	if (!pCall)
	{
#if SOURCE_ENGINE == SE_SDK2013
		if (!strcmp(g_pSM->GetGameFolderName(), "nmrih"))
		{
			ValvePassInfo pass[6];
			InitPass(pass[0], Valve_Float, PassType_Float, PASSFLAG_BYVAL);
			InitPass(pass[1], Valve_Bool, PassType_Basic, PASSFLAG_BYVAL);
			InitPass(pass[2], Valve_Float, PassType_Float, PASSFLAG_BYVAL);
			InitPass(pass[3], Valve_Bool, PassType_Basic, PASSFLAG_BYVAL);
			InitPass(pass[4], Valve_POD, PassType_Basic, PASSFLAG_BYVAL);
			InitPass(pass[5], Valve_POD, PassType_Basic, PASSFLAG_BYVAL);
			if (!CreateBaseCall("Ignite", ValveCall_Entity, NULL, pass, 6, &pCall))
			{
				return pContext->ThrowNativeError("\"Ignite\" not supported by this mod");
			}
			else if (!pCall) {
				return pContext->ThrowNativeError("\"Ignite\" wrapper failed to initialize");
			}
		}
		else
#endif // SDK2013
		{
			ValvePassInfo pass[4];
			InitPass(pass[0], Valve_Float, PassType_Float, PASSFLAG_BYVAL);
			InitPass(pass[1], Valve_Bool, PassType_Basic, PASSFLAG_BYVAL);
			InitPass(pass[2], Valve_Float, PassType_Float, PASSFLAG_BYVAL);
			InitPass(pass[3], Valve_Bool, PassType_Basic, PASSFLAG_BYVAL);
			if (!CreateBaseCall("Ignite", ValveCall_Entity, NULL, pass, 4, &pCall))
			{
				return pContext->ThrowNativeError("\"Ignite\" not supported by this mod");
			}
			else if (!pCall) {
				return pContext->ThrowNativeError("\"Ignite\" wrapper failed to initialize");
			}
		}
	}

	START_CALL();
	DECODE_VALVE_PARAM(1, thisinfo, 0);
	DECODE_VALVE_PARAM(2, vparams, 0);
	DECODE_VALVE_PARAM(3, vparams, 1);
	DECODE_VALVE_PARAM(4, vparams, 2);
	DECODE_VALVE_PARAM(5, vparams, 3);

#if SOURCE_ENGINE == SE_SDK2013
	if (!strcmp(g_pSM->GetGameFolderName(), "nmrih"))
	{
		*(int *) (vptr + pCall->vparams[4].offset) = 0;
		*(int *) (vptr + pCall->vparams[5].offset) = 0;
	}
#endif // SDK2013

	FINISH_CALL_SIMPLE(NULL);

	return 1;
}
#else
/* Dark Messiah specific version */
static cell_t IgniteEntity(IPluginContext *pContext, const cell_t *params)
{
	static ValveCall *pCall = NULL;
	if (!pCall)
	{
		ValvePassInfo pass[6];
		InitPass(pass[0], Valve_Float, PassType_Float, PASSFLAG_BYVAL);
		InitPass(pass[1], Valve_Bool, PassType_Basic, PASSFLAG_BYVAL);
		InitPass(pass[2], Valve_Float, PassType_Float, PASSFLAG_BYVAL);
		InitPass(pass[3], Valve_Bool, PassType_Basic, PASSFLAG_BYVAL);
		InitPass(pass[4], Valve_POD, PassType_Basic, PASSFLAG_BYVAL);
		InitPass(pass[5], Valve_POD, PassType_Basic, PASSFLAG_BYVAL);
		if (!CreateBaseCall("Ignite", ValveCall_Entity, NULL, pass, 6, &pCall))
		{
			return pContext->ThrowNativeError("\"Ignite\" not supported by this mod");
		} else if (!pCall) {
			return pContext->ThrowNativeError("\"Ignite\" wrapper failed to initialize");
		}
	}

	START_CALL();
	DECODE_VALVE_PARAM(1, thisinfo, 0);
	DECODE_VALVE_PARAM(2, vparams, 0);
	DECODE_VALVE_PARAM(3, vparams, 1);
	DECODE_VALVE_PARAM(4, vparams, 2);
	DECODE_VALVE_PARAM(5, vparams, 3);
	/* Not sure what these params do, but they appear to be the default values */
	*(int *)(vptr + pCall->vparams[4].offset) = 3;
	*(int *)(vptr + pCall->vparams[5].offset) = 0;
	FINISH_CALL_SIMPLE(NULL);

	return 1;
}
#endif

static cell_t ExtinguishEntity(IPluginContext *pContext, const cell_t *params)
{
	static ValveCall *pCall = NULL;
	if (!pCall)
	{
		if (!CreateBaseCall("Extinguish", ValveCall_Entity, NULL, NULL, 0, &pCall))
		{
			return pContext->ThrowNativeError("\"Extinguish\" not supported by this mod");
		} else if (!pCall) {
			return pContext->ThrowNativeError("\"Extinguish\" wrapper failed to initialize");
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
			return pContext->ThrowNativeError("\"Teleport\" wrapper failed to initialize");
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

#if SOURCE_ENGINE >= SE_ORANGEBOX
static cell_t ForcePlayerSuicide(IPluginContext *pContext, const cell_t *params)
{
	static ValveCall *pCall = NULL;
	if (!pCall)
	{
		ValvePassInfo pass[2];
		InitPass(pass[0], Valve_Bool, PassType_Basic, PASSFLAG_BYVAL);
		InitPass(pass[1], Valve_Bool, PassType_Basic, PASSFLAG_BYVAL);
		if (!CreateBaseCall("CommitSuicide", ValveCall_Player, NULL, pass, 2, &pCall))
		{
			return pContext->ThrowNativeError("\"CommitSuicide\" not supported by this mod");
		}
		else if (!pCall)
		{
			return pContext->ThrowNativeError("\"CommitSuicide\" wrapper failed to initialize");
		}
	}

	START_CALL();
	DECODE_VALVE_PARAM(1, thisinfo, 0);
	*(bool *)(vptr + pCall->vparams[0].offset) = false;
	*(bool *)(vptr + pCall->vparams[1].offset) = false;
	FINISH_CALL_SIMPLE(NULL);

	return 1;
}
#else
static cell_t ForcePlayerSuicide(IPluginContext *pContext, const cell_t *params)
{
	static ValveCall *pCall = NULL;
	if (!pCall)
	{
		if (!CreateBaseCall("CommitSuicide", ValveCall_Player, NULL, NULL, 0, &pCall))
		{
			return pContext->ThrowNativeError("\"CommitSuicide\" not supported by this mod");
		} else if (!pCall) {
			return pContext->ThrowNativeError("\"CommitSuicide\" wrapper failed to initialize");
		}
	}

	START_CALL();
	DECODE_VALVE_PARAM(1, thisinfo, 0);
	FINISH_CALL_SIMPLE(NULL);

	return 1;
}
#endif

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

	edict_t *pEdict = PEntityOfEntIndex(gamehelpers->ReferenceToIndex(params[2]));
	if (!pEdict || pEdict->IsFree())
	{
		return pContext->ThrowNativeError("Entity %d is not valid", params[2]);
	}

	engine->SetView(player->GetEdict(), pEdict);

	return 1;
}

static SourceHook::String *g_lightstyle[MAX_LIGHTSTYLES] = {NULL};
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
		g_lightstyle[style] = new SourceHook::String();
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
	IGamePlayer *player = playerhelpers->GetGamePlayer(params[1]);
	if (!player)
	{
		return pContext->ThrowNativeError("Client %d is not valid", params[1]);
	} else if (!player->IsInGame()) {
		return pContext->ThrowNativeError("Client %d is not in game", params[1]);
	}

	edict_t *pEdict = player->GetEdict();
	CBaseEntity *pEntity = pEdict->GetUnknown()->GetBaseEntity();

	/* See if we should be taking away health */
	bool should_slay = false;
	if (params[2])
	{
#if SOURCE_ENGINE != SE_DARKMESSIAH
		int *health = (int *)((char *)pEntity + s_health_offs);

		if (*health - params[2] <= 0)
		{
			*health = 1;
			should_slay = true;
		} else {
			*health -= params[2];
		}
#else
		float *health = (float *)((char *)pEntity + s_health_offs);

		if (*health - (float)params[2] <= 0)
		{
			*health = 1.0f;
			should_slay = true;
		} else {
			*health -= (float)params[2];
		}
#endif
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
		cell_t player_list[SM_MAXPLAYERS], total_players = 0;
		int maxClients = playerhelpers->GetMaxClients();

		int r = (rand() % s_sound_count) + 1;
		ke::SafeSprintf(name, sizeof(name), "SlapSound%d", r);

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
#if SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_HL2DM || SOURCE_ENGINE == SE_DODS || SOURCE_ENGINE == SE_SDK2013 \
	|| SOURCE_ENGINE == SE_BMS || SOURCE_ENGINE == SE_TF2
			engsound->EmitSound(rf, params[1], CHAN_AUTO, sound_name, VOL_NORM, ATTN_NORM, 0, PITCH_NORM, 0, &pos);
#elif SOURCE_ENGINE < SE_PORTAL2
			engsound->EmitSound(rf, params[1], CHAN_AUTO, sound_name, VOL_NORM, ATTN_NORM, 0, PITCH_NORM, &pos);
#else
			engsound->EmitSound(rf, params[1], CHAN_AUTO, sound_name, -1, sound_name, VOL_NORM, ATTN_NORM, 0, 0, PITCH_NORM, &pos);
#endif
		}
	}

	if (!s_frag_offs)
	{
		const char *frag_prop = g_pGameConf->GetKeyValue("m_iFrags");
		if (frag_prop)
		{
			datamap_t *pMap = gamehelpers->GetDataMap(pEntity);
			sm_datatable_info_t info;
			if (gamehelpers->FindDataMapInfo(pMap, frag_prop, &info))
			{
				s_frag_offs = info.actual_offset;
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
	IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(params[1]);

	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Invalid client index %d", params[1]);
	}
	else if (!pPlayer->IsInGame())
	{
		return pContext->ThrowNativeError("Client %d is not in game", params[1]);
	}

	edict_t *pEdict = pPlayer->GetEdict();
	CBaseEntity *pEntity = pEdict->GetUnknown() ? pEdict->GetUnknown()->GetBaseEntity() : NULL;

	/* We always set the angles for backwards compatibility -- 
	 * The original function had no return value.
	 */
	QAngle angles;
	bool got_angles = false;

	if (pEntity != NULL)
	{
		got_angles = GetEyeAngles(pEntity, &angles);
	}
	
	cell_t *addr;
	pContext->LocalToPhysAddr(params[2], &addr);
	addr[0] = sp_ftoc(angles.x);
	addr[1] = sp_ftoc(angles.y);
	addr[2] = sp_ftoc(angles.z);

	return got_angles ? 1 : 0;
}

#if SOURCE_ENGINE >= SE_ORANGEBOX
static cell_t NativeFindEntityByClassname(IPluginContext *pContext, const cell_t *params)
{
	char *searchname;
	CBaseEntity *pEntity;

	if (params[1] == -1)
	{
		pEntity = (CBaseEntity *)servertools->FirstEntity();
	}
	else
	{
		pEntity = gamehelpers->ReferenceToEntity(params[1]);
		if (!pEntity)
		{
			return pContext->ThrowNativeError("Entity %d (%d) is invalid",
				gamehelpers->ReferenceToIndex(params[1]),
				params[1]);
		}
		pEntity = (CBaseEntity *)servertools->NextEntity(pEntity);
	}

	// it's tough to find a good ent these days
	if (!pEntity)
	{
		return -1;
	}

	pContext->LocalToString(params[2], &searchname);

	const char *classname;
	int lastletterpos;

	static int offset = -1;
	if (offset == -1)
	{
		sm_datatable_info_t info;
		if (!gamehelpers->FindDataMapInfo(gamehelpers->GetDataMap(pEntity), "m_iClassname", &info))
		{
			return -1;
		}
		
		offset = info.actual_offset;
	}

	string_t s;

	while (pEntity)
	{
		if ((s = *(string_t *)((uint8_t *)pEntity + offset)) == NULL_STRING)
		{
			pEntity = (CBaseEntity *)servertools->NextEntity(pEntity);
			continue;
		}

		classname = STRING(s);

		lastletterpos = strlen(searchname) - 1;
		if (searchname[lastletterpos] == '*')
		{
			if (strncasecmp(searchname, classname, lastletterpos) == 0)
			{
				return gamehelpers->EntityToBCompatRef(pEntity);
			}
		}
		else if (strcasecmp(searchname, classname) == 0)
		{
			return gamehelpers->EntityToBCompatRef(pEntity);
		}

		pEntity = (CBaseEntity *)servertools->NextEntity(pEntity);
	}

	return -1;
}
#endif // >= ORANGEBOX && != TF2

static cell_t FindEntityByClassname(IPluginContext *pContext, const cell_t *params)
{
#if SOURCE_ENGINE == SE_TF2        \
	|| SOURCE_ENGINE == SE_DODS    \
	|| SOURCE_ENGINE == SE_HL2DM   \
	|| SOURCE_ENGINE == SE_CSS     \
	|| SOURCE_ENGINE == SE_BMS     \
	|| SOURCE_ENGINE == SE_SDK2013 \
	|| SOURCE_ENGINE == SE_BLADE   \
	|| SOURCE_ENGINE == SE_NUCLEARDAWN

	static bool bHasServerTools3 = !!g_SMAPI->GetServerFactory(false)("VSERVERTOOLS003", nullptr);
	if (bHasServerTools3)
	{
		CBaseEntity *pStartEnt = NULL;
		if (params[1] != -1)
		{
			pStartEnt = gamehelpers->ReferenceToEntity(params[1]);
			if (!pStartEnt)
			{
				return pContext->ThrowNativeError("Entity %d (%d) is invalid",
					gamehelpers->ReferenceToIndex(params[1]),
					params[1]);
			}
		}

		char *searchname;
		pContext->LocalToString(params[2], &searchname);

		CBaseEntity *pEntity = servertools->FindEntityByClassname(pStartEnt, searchname);
		return gamehelpers->EntityToBCompatRef(pEntity);
	}
#endif

	static ValveCall *pCall = NULL;
	static bool bProbablyNoFEBC = false;

#if SOURCE_ENGINE >= SE_ORANGEBOX
	if (bProbablyNoFEBC)
	{
		return NativeFindEntityByClassname(pContext, params);
	}
#endif // >= SE_ORANGEBOX

	if (!pCall)
	{
		ValvePassInfo pass[3];
		InitPass(pass[0], Valve_CBaseEntity, PassType_Basic, PASSFLAG_BYVAL, VDECODE_FLAG_ALLOWNULL|VDECODE_FLAG_ALLOWWORLD);
		InitPass(pass[1], Valve_String, PassType_Basic, PASSFLAG_BYVAL);
		InitPass(pass[2], Valve_CBaseEntity, PassType_Basic, PASSFLAG_BYVAL);

		char error[256];
		error[0] = '\0';
		if (!CreateBaseCall("FindEntityByClassname", ValveCall_EntityList, &pass[2], pass, 2, &pCall))
		{
			g_pSM->Format(error, sizeof(error), "\"FindEntityByClassname\" not supported by this mod");
		} else if (!pCall) {
			g_pSM->Format(error, sizeof(error), "\"FindEntityByClassname\" wrapper failed to initialize");
		}

		if (error[0] != '\0')
		{
#if SOURCE_ENGINE >= SE_ORANGEBOX
			if (!bProbablyNoFEBC)
			{
				bProbablyNoFEBC = true;

				// CreateBaseCall above abstracts all of the gamedata logic, but we need to know if the key was even found.
				// We don't want to log an error if key isn't present (knowing falling back to native method), only throw
				// error if signature/symbol was not found.
				void *dummy;
				if (g_pGameConf->GetMemSig("FindEntityByClassname", &dummy))
				{
					g_pSM->LogError(myself, "%s, falling back to IServerTools method.", error);
				}
			}
			return NativeFindEntityByClassname(pContext, params);
#else
			return pContext->ThrowNativeError("%s", error);
#endif // >= ORANGEBOX
		}
	}

	CBaseEntity *pEntity;
	START_CALL();
	*(void **)vptr = g_EntList; 
	DECODE_VALVE_PARAM(1, vparams, 0);
	DECODE_VALVE_PARAM(2, vparams, 1);
	FINISH_CALL_SIMPLE(&pEntity);

	return gamehelpers->EntityToBCompatRef(pEntity);
}

#if SOURCE_ENGINE >= SE_ORANGEBOX
static cell_t CreateEntityByName(IPluginContext *pContext, const cell_t *params)
{
	if (!g_pSM->IsMapRunning())
	{
		return pContext->ThrowNativeError("Cannot create new entity when no map is running");
	}

	char *classname;
	pContext->LocalToString(params[1], &classname);
#if SOURCE_ENGINE != SE_CSGO && SOURCE_ENGINE != SE_BLADE
	CBaseEntity *pEntity = (CBaseEntity *)servertools->CreateEntityByName(classname);
#else
	CBaseEntity *pEntity = (CBaseEntity *)servertools->CreateItemEntityByName(classname);

	if(!pEntity)
	{
		pEntity = (CBaseEntity *)servertools->CreateEntityByName(classname);
	}
#endif
	return gamehelpers->EntityToBCompatRef(pEntity);
}
#else
static cell_t CreateEntityByName(IPluginContext *pContext, const cell_t *params)
{
	if (!g_pSM->IsMapRunning())
	{
		return pContext->ThrowNativeError("Cannot create new entity when no map is running");
	}

	static ValveCall *pCall = NULL;
	if (!pCall)
	{
		ValvePassInfo pass[3];
		InitPass(pass[0], Valve_String, PassType_Basic, PASSFLAG_BYVAL);
		InitPass(pass[1], Valve_POD, PassType_Basic, PASSFLAG_BYVAL);
		InitPass(pass[2], Valve_CBaseEntity, PassType_Basic, PASSFLAG_BYVAL);
		if (!CreateBaseCall("CreateEntityByName", ValveCall_Static, &pass[2], pass, 2, &pCall))
		{
			return pContext->ThrowNativeError("\"CreateEntityByName\" not supported by this mod");
		} else if (!pCall) {
			return pContext->ThrowNativeError("\"CreateEntityByName\" wrapper failed to initialize");
		}
	}

	CBaseEntity *pEntity = NULL;
	START_CALL();
	DECODE_VALVE_PARAM(1, vparams, 0);
	DECODE_VALVE_PARAM(2, vparams, 1);
	FINISH_CALL_SIMPLE(&pEntity);

	return gamehelpers->EntityToBCompatRef(pEntity);
}
#endif

#if SOURCE_ENGINE >= SE_ORANGEBOX
static cell_t DispatchSpawn(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(params[1]);
	if (!pEntity)
	{
		return pContext->ThrowNativeError("Entity %d (%d) is invalid", gamehelpers->ReferenceToIndex(params[1]), params[1]);
	}

	servertools->DispatchSpawn(pEntity);

	return 1;
}
#else
static cell_t DispatchSpawn(IPluginContext *pContext, const cell_t *params)
{
	static ValveCall *pCall = NULL;
	if (!pCall)
	{
		ValvePassInfo pass[2];
		InitPass(pass[0], Valve_CBaseEntity, PassType_Basic, PASSFLAG_BYVAL);
		InitPass(pass[1], Valve_POD, PassType_Basic, PASSFLAG_BYVAL);
		if (!CreateBaseCall("DispatchSpawn", ValveCall_Static, &pass[1], pass, 1, &pCall))
		{
			return pContext->ThrowNativeError("\"DispatchSpawn\" not supported by this mod");
		} else if (!pCall) {
			return pContext->ThrowNativeError("\"DispatchSpawn\" wrapper failed to initialize");
		}
	}

	int ret;
	START_CALL();
	DECODE_VALVE_PARAM(1, vparams, 0);
	FINISH_CALL_SIMPLE(&ret);

	return (ret == -1) ? 0 : 1;
}
#endif

#if SOURCE_ENGINE >= SE_ORANGEBOX
static cell_t DispatchKeyValue(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(params[1]);
	if (!pEntity)
	{
		return pContext->ThrowNativeError("Entity %d (%d) is invalid", gamehelpers->ReferenceToIndex(params[1]), params[1]);
	}

	char *key;
	char *value;
	pContext->LocalToString(params[2], &key);
	pContext->LocalToString(params[3], &value);

	return (servertools->SetKeyValue(pEntity, key, value) ? 1 : 0);
}
#else
static cell_t DispatchKeyValue(IPluginContext *pContext, const cell_t *params)
{
	static ValveCall *pCall = NULL;
	if (!pCall)
	{
		ValvePassInfo pass[3];
		InitPass(pass[0], Valve_String, PassType_Basic, PASSFLAG_BYVAL);
		InitPass(pass[1], Valve_String, PassType_Basic, PASSFLAG_BYVAL);
		InitPass(pass[2], Valve_Bool, PassType_Basic, PASSFLAG_BYVAL);
		if (!CreateBaseCall("DispatchKeyValue", ValveCall_Entity, &pass[2], pass, 2, &pCall))
		{
			return pContext->ThrowNativeError("\"DispatchKeyValue\" not supported by this mod");
		} else if (!pCall) {
			return pContext->ThrowNativeError("\"DispatchKeyValue\" wrapper failed to initialize");
		}
	}

	bool ret;
	START_CALL();
	DECODE_VALVE_PARAM(1, thisinfo, 0);
	DECODE_VALVE_PARAM(2, vparams, 0);
	DECODE_VALVE_PARAM(3, vparams, 1);
	FINISH_CALL_SIMPLE(&ret);

	return ret ? 1 : 0;
}
#endif

#if SOURCE_ENGINE >= SE_ORANGEBOX
static cell_t DispatchKeyValueFloat(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(params[1]);
	if (!pEntity)
	{
		return pContext->ThrowNativeError("Entity %d (%d) is invalid", gamehelpers->ReferenceToIndex(params[1]), params[1]);
	}

	char *key;
	pContext->LocalToString(params[2], &key);
	
	float value = sp_ctof(params[3]);

	return (servertools->SetKeyValue(pEntity, key, value) ? 1 : 0);
}
#else
static cell_t DispatchKeyValueFloat(IPluginContext *pContext, const cell_t *params)
{
	static ValveCall *pCall = NULL;
	if (!pCall)
	{
		ValvePassInfo pass[3];
		InitPass(pass[0], Valve_String, PassType_Basic, PASSFLAG_BYVAL);
		InitPass(pass[1], Valve_Float, PassType_Float, PASSFLAG_BYVAL);
		InitPass(pass[2], Valve_Bool, PassType_Basic, PASSFLAG_BYVAL);
		if (!CreateBaseCall("DispatchKeyValueFloat", ValveCall_Entity, &pass[2], pass, 2, &pCall))
		{
			return pContext->ThrowNativeError("\"DispatchKeyValueFloat\" not supported by this mod");
		} else if (!pCall) {
			return pContext->ThrowNativeError("\"DispatchKeyValueFloat\" wrapper failed to initialize");
		}
	}

	bool ret;
	START_CALL();
	DECODE_VALVE_PARAM(1, thisinfo, 0);
	DECODE_VALVE_PARAM(2, vparams, 0);
	DECODE_VALVE_PARAM(3, vparams, 1);
	FINISH_CALL_SIMPLE(&ret);

	return ret ? 1 : 0;
}
#endif

#if SOURCE_ENGINE >= SE_ORANGEBOX
static cell_t DispatchKeyValueVector(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(params[1]);
	if (!pEntity)
	{
		return pContext->ThrowNativeError("Entity %d (%d) is invalid", gamehelpers->ReferenceToIndex(params[1]), params[1]);
	}

	char *key;
	pContext->LocalToString(params[2], &key);
	
	cell_t *vec;
	pContext->LocalToPhysAddr(params[3], &vec);

	const Vector *v = new Vector(
		sp_ctof(vec[0]),
		sp_ctof(vec[1]),
		sp_ctof(vec[2]));

	return (servertools->SetKeyValue(pEntity, key, *v) ? 1 : 0);
}
#else
static cell_t DispatchKeyValueVector(IPluginContext *pContext, const cell_t *params)
{
	static ValveCall *pCall = NULL;
	if (!pCall)
	{
		ValvePassInfo pass[3];
		InitPass(pass[0], Valve_String, PassType_Basic, PASSFLAG_BYVAL);
#if SOURCE_ENGINE >= SE_ORANGEBOX
		InitPass(pass[1], Valve_Vector, PassType_Basic, PASSFLAG_BYVAL);
#else
		InitPass(pass[1], Valve_Vector, PassType_Object, PASSFLAG_BYVAL|PASSFLAG_OCTOR|PASSFLAG_OASSIGNOP);
#endif
		InitPass(pass[2], Valve_Bool, PassType_Basic, PASSFLAG_BYVAL);
		if (!CreateBaseCall("DispatchKeyValueVector", ValveCall_Entity, &pass[2], pass, 2, &pCall))
		{
			return pContext->ThrowNativeError("\"DispatchKeyValueVector\" not supported by this mod");
		} else if (!pCall) {
			return pContext->ThrowNativeError("\"DispatchKeyValueVector\" wrapper failed to initialize");
		}
	}

	bool ret;
	START_CALL();
	DECODE_VALVE_PARAM(1, thisinfo, 0);
	DECODE_VALVE_PARAM(2, vparams, 0);
	DECODE_VALVE_PARAM(3, vparams, 1);
	FINISH_CALL_SIMPLE(&ret);

	return ret ? 1 : 0;
}
#endif

static cell_t sm_GetClientAimTarget(IPluginContext *pContext, const cell_t *params)
{
	IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(params[1]);

	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Invalid client index %d", params[1]);
	}
	else if (!pPlayer->IsInGame())
	{
		return pContext->ThrowNativeError("Client %d is not in game", params[1]);
	}

	return GetClientAimTarget(pPlayer->GetEdict(), params[2] ? true : false);
}

static cell_t sm_SetEntityModel(IPluginContext *pContext, const cell_t *params)
{
	static ValveCall *pCall = NULL;
	if (!pCall)
	{
		ValvePassInfo pass[1];
		InitPass(pass[0], Valve_String, PassType_Basic, PASSFLAG_BYVAL);
		if (!CreateBaseCall("SetEntityModel", ValveCall_Entity, NULL, pass, 1, &pCall))
		{
			return pContext->ThrowNativeError("\"SetEntityModel\" not supported by this mod");
		} else if (!pCall) {
			return pContext->ThrowNativeError("\"SetEntityModel\" wrapper failed to initialize");
		}
	}

	START_CALL();
	DECODE_VALVE_PARAM(1, thisinfo, 0);
	DECODE_VALVE_PARAM(2, vparams, 0);
	FINISH_CALL_SIMPLE(NULL);

	return 1;
}

static cell_t GetPlayerDecalFile(IPluginContext *pContext, const cell_t *params)
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

	player_info_t info;
	char *buffer;

	if (!GetPlayerInfo(params[1], &info) || !info.customFiles[0])
	{
		return 0;
	}

	pContext->LocalToString(params[2], &buffer);
	Q_binarytohex((byte *)&info.customFiles[0], sizeof(info.customFiles[0]), buffer, params[3]);

	return 1;
}

static cell_t GetPlayerJingleFile(IPluginContext *pContext, const cell_t *params)
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

	player_info_t info;
	char *buffer;

	if (!GetPlayerInfo(params[1], &info) || !info.customFiles[1])
	{
		return 0;
	}

	pContext->LocalToString(params[2], &buffer);
	Q_binarytohex((byte *)&info.customFiles[1], sizeof(info.customFiles[1]), buffer, params[3]);

	return 1;
}

static cell_t GetServerNetStats(IPluginContext *pContext, const cell_t *params)
{
	if (iserver == NULL)
	{
		return pContext->ThrowNativeError("IServer interface not supported, file a bug report.");
	}

	float in, out;
	cell_t *pIn, *pOut;

	pContext->LocalToPhysAddr(params[1], &pIn);
	pContext->LocalToPhysAddr(params[2], &pOut);
	iserver->GetNetStats(in, out);

	*pIn = sp_ftoc(in);
	*pOut = sp_ftoc(out);

	return 1;
}

static cell_t WeaponEquip(IPluginContext *pContext, const cell_t *params)
{
	static ValveCall *pCall = NULL;
	if (!pCall)
	{
		ValvePassInfo pass[1];
		InitPass(pass[0], Valve_CBaseEntity, PassType_Basic, PASSFLAG_BYVAL);
		if (!CreateBaseCall("WeaponEquip", ValveCall_Player, NULL, pass, 1, &pCall))
		{
			return pContext->ThrowNativeError("\"WeaponEquip\" not supported by this mod");
		} else if (!pCall) {
			return pContext->ThrowNativeError("\"WeaponEquip\" wrapper failed to initialize");
		}
	}

	START_CALL();
	DECODE_VALVE_PARAM(1, thisinfo, 0);
	DECODE_VALVE_PARAM(2, vparams, 0);
	FINISH_CALL_SIMPLE(NULL);

	return 1;
}

static cell_t ActivateEntity(IPluginContext *pContext, const cell_t *params)
{
	static ValveCall *pCall = NULL;
	if (!pCall)
	{
		if (!CreateBaseCall("Activate", ValveCall_Entity, NULL, NULL, 0, &pCall))
		{
			return pContext->ThrowNativeError("\"Activate\" not supported by this mod");
		}
		else if (!pCall)
		{
			return pContext->ThrowNativeError("\"Activate\" wrapper failed to initialize");
		}
	}

	START_CALL();
	DECODE_VALVE_PARAM(1, thisinfo, 0);
	FINISH_CALL_SIMPLE(NULL);

	return 1;
}

static cell_t SetClientName(IPluginContext *pContext, const cell_t *params)
{
	if (iserver == NULL)
	{
		return pContext->ThrowNativeError("IServer interface not supported, file a bug report.");
	}

	IGamePlayer *player = playerhelpers->GetGamePlayer(params[1]);
	IClient *pClient = iserver->GetClient(params[1] - 1);

	if (player == NULL || pClient == NULL)
	{
		return pContext->ThrowNativeError("Invalid client index %d", params[1]);
	}
	if (!player->IsConnected())
	{
		return pContext->ThrowNativeError("Client %d is not connected", params[1]);
	}

	static ValveCall *pCall = NULL;
	if (!pCall)
	{
		ValvePassInfo params[1];
		InitPass(params[0], Valve_String, PassType_Basic, PASSFLAG_BYVAL);

		if (!CreateBaseCall("SetClientName", ValveCall_Entity, NULL, params, 1, &pCall))
		{
			return pContext->ThrowNativeError("\"SetClientName\" not supported by this mod");
		}
		else if (!pCall)
		{
			return pContext->ThrowNativeError("\"SetClientName\" wrapper failed to initialize");
		}
	}

	// The IClient vtable is +sizeof(void *) from the CBaseClient vtable due to multiple inheritance.
	void *pGameClient = (void *)((intptr_t)pClient - sizeof(void *));

	// Change the name in the engine.
	START_CALL();
	void **ebuf = (void **)vptr;
	*ebuf = pGameClient;
	DECODE_VALVE_PARAM(2, vparams, 0);
	FINISH_CALL_SIMPLE(NULL);

	// Notify the server of the change.
	serverClients->ClientSettingsChanged(player->GetEdict());

	return 1;
}

static cell_t SetClientInfo(IPluginContext *pContext, const cell_t *params)
{
	if (iserver == NULL)
	{
		return pContext->ThrowNativeError("IServer interface not supported, file a bug report.");
	}

	IGamePlayer *player = playerhelpers->GetGamePlayer(params[1]);
	IClient *pClient = iserver->GetClient(params[1] - 1);

	if (player == NULL || pClient == NULL)
	{
		return pContext->ThrowNativeError("Invalid client index %d", params[1]);
	}
	if (!player->IsConnected())
	{
		return pContext->ThrowNativeError("Client %d is not connected", params[1]);
	}

	static ValveCall *pCall = NULL;
	if (!pCall)
	{
		ValvePassInfo params[2];
		InitPass(params[0], Valve_String, PassType_Basic, PASSFLAG_BYVAL);
		InitPass(params[1], Valve_String, PassType_Basic, PASSFLAG_BYVAL);

		if (!CreateBaseCall("SetUserCvar", ValveCall_Entity, NULL, params, 2, &pCall))
		{
			return pContext->ThrowNativeError("\"SetUserCvar\" not supported by this mod");
		}
		else if (!pCall)
		{
			return pContext->ThrowNativeError("\"SetUserCvar\" wrapper failed to initialize");
		}
	}

/* TODO: Use UpdateUserSettings function for all engines */
#if SOURCE_ENGINE == SE_DARKMESSIAH
	static ValveCall *pUpdateSettings = NULL;
	if (!pUpdateSettings)
	{
		if (!CreateBaseCall("UpdateUserSettings", ValveCall_Entity, NULL, NULL, 0, &pUpdateSettings))
		{
			return pContext->ThrowNativeError("\"SetUserCvar\" not supported by this mod");
		}
		else if (!pUpdateSettings)
		{
			return pContext->ThrowNativeError("\"SetUserCvar\" wrapper failed to initialize");
		}
	}
#else
	static int changedOffset = -1;

	if (changedOffset == -1)
	{	
		if (!g_pGameConf->GetOffset("InfoChanged", &changedOffset))
		{
			return pContext->ThrowNativeError("\"SetUserCvar\" not supported by this mod");
		}
	}
#endif

	unsigned char *CGameClient = (unsigned char *)pClient - sizeof(void *);

	START_CALL();
	/* Not really a CBaseEntity* but this works */
	CBaseEntity **ebuf = (CBaseEntity **)vptr;
	*ebuf = (CBaseEntity *)CGameClient;
	DECODE_VALVE_PARAM(2, vparams, 0);
	DECODE_VALVE_PARAM(3, vparams, 1);
	FINISH_CALL_SIMPLE(NULL);

#if SOURCE_ENGINE == SE_DARKMESSIAH
	unsigned char *args = pUpdateSettings->stk_get();
	*(void **)args = CGameClient;
	pUpdateSettings->call->Execute(args, NULL);
	pUpdateSettings->stk_put(args);
#else
	uint8_t* changed = (uint8_t *)(CGameClient + changedOffset);
	*changed = 1;
#endif

	return 1;
}

static cell_t GetPlayerResourceEntity(IPluginContext *pContext, const cell_t *params)
{
	if (gamehelpers->GetHandleEntity(g_ResourceEntity) != NULL)
	{
		return g_ResourceEntity.GetEntryIndex();
	}

	return -1;
}

static cell_t GivePlayerAmmo(IPluginContext *pContext, const cell_t *params)
{
	static ValveCall *pCall = NULL;
	if (!pCall)
	{
		ValvePassInfo pass[3];
		InitPass(pass[0], Valve_POD, PassType_Basic, PASSFLAG_BYVAL);
		InitPass(pass[1], Valve_POD, PassType_Basic, PASSFLAG_BYVAL);
		InitPass(pass[2], Valve_Bool, PassType_Basic, PASSFLAG_BYVAL);
		if (!CreateBaseCall("GiveAmmo", ValveCall_Player, &pass[0], pass, 3, &pCall))
		{
			return pContext->ThrowNativeError("\"GiveAmmo\" not supported by this mod");
		} else if (!pCall) {
			return pContext->ThrowNativeError("\"GiveAmmo\" wrapper failed to initialize");
		}
	}

	int ammoGiven;
	START_CALL();
	DECODE_VALVE_PARAM(1, thisinfo, 0);
	DECODE_VALVE_PARAM(2, vparams, 0);
	DECODE_VALVE_PARAM(3, vparams, 1);
	DECODE_VALVE_PARAM(4, vparams, 2);
	FINISH_CALL_SIMPLE(&ammoGiven);

	return ammoGiven;
}

// SetEntityCollisionGroup(int entity, int collisionGroup)
static cell_t SetEntityCollisionGroup(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity;
	ENTINDEX_TO_CBASEENTITY(params[1], pEntity);

	int offsetCollisionGroup = -1;
	// Retrieve m_hOwnerEntity offset
	sm_datatable_info_t offset_data_info;
	datamap_t *offsetMap = gamehelpers->GetDataMap(pEntity);
	if (!offsetMap || !gamehelpers->FindDataMapInfo(offsetMap, "m_CollisionGroup", &offset_data_info))
	{
		return pContext->ThrowNativeError("\"SetEntityCollisionGroup\" Failed to retrieve m_CollisionGroup datamap on entity");
	}
	offsetCollisionGroup = offset_data_info.actual_offset;

	// Reimplementation of CBaseEntity::SetCollisionGroup
	// https://github.com/ValveSoftware/source-sdk-2013/blob/0d8dceea4310fde5706b3ce1c70609d72a38efdf/sp/src/game/shared/baseentity_shared.cpp#L2477L2484
	int *collisionGroup = (int *)((uint8_t *)pEntity + offsetCollisionGroup);
	if ((*collisionGroup) != params[2])
	{
		*collisionGroup = params[2];
		// Returns false if CollisionRulesChanged hack isn't supported for this mod
		if (!CollisionRulesChanged(pEntity))
		{
			return pContext->ThrowNativeError("\"SetEntityCollisionGroup\" unsupported mod");
		}
	}
	return 1;
}

static cell_t EntityCollisionRulesChanged(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity;
	ENTINDEX_TO_CBASEENTITY(params[1], pEntity);
	// Returns false if CollisionRulesChanged hack isn't supported for this mod
	if (!CollisionRulesChanged(pEntity))
	{
		return pContext->ThrowNativeError("\"EntityCollisionRulesChanged\" unsupported mod");
	}
	return 1;
}

static cell_t SetEntityOwner(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity;
	ENTINDEX_TO_CBASEENTITY(params[1], pEntity);

	static ICallWrapper *pSetOwnerEntity = NULL;
	if (!pSetOwnerEntity)
	{
		int offset = -1;
		if (!g_pGameConf->GetOffset("SetOwnerEntity", &offset))
		{
			return pContext->ThrowNativeError("\"SetOwnerEntity\" not supported by this mod");
		}

		PassInfo pass[1];
		pass[0].type = PassType_Basic;
		pass[0].flags = PASSFLAG_BYVAL;
		pass[0].size = sizeof(CBaseEntity *);

		if (!(pSetOwnerEntity = g_pBinTools->CreateVCall(offset, 0, 0, nullptr, pass, 1)))
		{
			return pContext->ThrowNativeError("\"SetOwnerEntity\" wrapper failed to initialize");
		}
	}

	CBaseEntity *pNewOwner = gamehelpers->ReferenceToEntity(params[2]);
	ArgBuffer<CBaseEntity *, CBaseEntity *> vstk(pEntity, pNewOwner);
	pSetOwnerEntity->Execute(vstk, nullptr);

	return 1;
}

static cell_t LookupEntityAttachment(IPluginContext* pContext, const cell_t* params)
{
	CBaseEntity* pEntity;
	ENTINDEX_TO_CBASEENTITY(params[1], pEntity);

	ServerClass* pClass = ((IServerUnknown*)pEntity)->GetNetworkable()->GetServerClass();
	if (!FindNestedDataTable(pClass->m_pTable, "DT_BaseAnimating"))
	{
		return pContext->ThrowNativeError("Entity %d (%d) is not a CBaseAnimating", gamehelpers->ReferenceToIndex(params[1]), params[1]);
	}

	static ICallWrapper* pLookupAttachment = NULL;
	if (!pLookupAttachment)
	{
		void* addr;
		if (!g_pGameConf->GetMemSig("LookupAttachment", &addr))
		{
			return pContext->ThrowNativeError("\"LookupAttachment\" not supported by this mod");
		}

		PassInfo retpass;
		retpass.type = PassType_Basic;
		retpass.flags = PASSFLAG_BYVAL;
		retpass.size = sizeof(int);

		PassInfo pass[1];
		pass[0].type = PassType_Basic;
		pass[0].flags = PASSFLAG_BYVAL;
		pass[0].size = sizeof(char*);

		if (!(pLookupAttachment = g_pBinTools->CreateCall(addr, CallConv_ThisCall, &retpass, pass, 1)))
		{
			return pContext->ThrowNativeError("\"LookupAttachment\" wrapper failed to initialize");
		}
	}

	char* buffer;
	pContext->LocalToString(params[2], &buffer);
	ArgBuffer<CBaseEntity*, char*> vstk(pEntity, buffer);

	int ret;
	pLookupAttachment->Execute(vstk, &ret);

	return ret;
}

static cell_t GetEntityAttachment(IPluginContext* pContext, const cell_t* params)
{
	CBaseEntity* pEntity;
	ENTINDEX_TO_CBASEENTITY(params[1], pEntity);

	ServerClass* pClass = ((IServerUnknown*)pEntity)->GetNetworkable()->GetServerClass();
	if (!FindNestedDataTable(pClass->m_pTable, "DT_BaseAnimating"))
	{
		return pContext->ThrowNativeError("Entity %d (%d) is not a CBaseAnimating", gamehelpers->ReferenceToIndex(params[1]), params[1]);
	}

	static ICallWrapper* pGetAttachment = NULL;
	if (!pGetAttachment)
	{
		int offset = -1;
		if (!g_pGameConf->GetOffset("GetAttachment", &offset))
		{
			return pContext->ThrowNativeError("\"GetAttachment\" not supported by this mod");
		}

		PassInfo retpass;
		retpass.type = PassType_Basic;
		retpass.flags = PASSFLAG_BYVAL;
		retpass.size = sizeof(bool);

		PassInfo pass[2];
		pass[0].type = PassType_Basic;
		pass[0].flags = PASSFLAG_BYVAL;
		pass[0].size = sizeof(int);
		pass[1].type = PassType_Basic;
		pass[1].flags = PASSFLAG_BYVAL;
		pass[1].size = sizeof(matrix3x4_t*);

		if (!(pGetAttachment = g_pBinTools->CreateVCall(offset, 0, 0, &retpass, pass, 2)))
		{
			return pContext->ThrowNativeError("\"GetAttachment\" wrapper failed to initialize");
		}
	}

	matrix3x4_t attachmentToWorld;
	ArgBuffer<CBaseEntity*, int, matrix3x4_t*> vstk(pEntity, params[2], &attachmentToWorld);

	bool ret;
	pGetAttachment->Execute(vstk, &ret);

	// GetAttachment returns a matrix3x4_t but plugins can't do anything with this.
	// Convert it to world origin and world angles.
	QAngle absAngles;
	Vector absOrigin;
	MatrixAngles(attachmentToWorld, absAngles, absOrigin);

	cell_t* pOrigin, * pAngles;
	pContext->LocalToPhysAddr(params[3], &pOrigin);
	pContext->LocalToPhysAddr(params[4], &pAngles);

	SET_VECTOR(pOrigin, absOrigin);
	SET_VECTOR(pAngles, absAngles);

	return ret;
}

sp_nativeinfo_t g_Natives[] = 
{
	{"ExtinguishEntity",		ExtinguishEntity},
	{"ForcePlayerSuicide",		ForcePlayerSuicide},
	{"GivePlayerItem",			GiveNamedItem},
	{"GetPlayerWeaponSlot",		GetPlayerWeaponSlot},
	{"IgniteEntity",			IgniteEntity},
	{"RemovePlayerItem",		RemovePlayerItem},
	{"TeleportEntity",			TeleportEntity},
	{"SetClientViewEntity",		SetClientViewEntity},
	{"SetLightStyle",			SetLightStyle},
	{"SlapPlayer",				SlapPlayer},
	{"GetClientEyePosition",	GetClientEyePosition},
	{"GetClientEyeAngles",		GetClientEyeAngles},
	{"FindEntityByClassname",	FindEntityByClassname},
	{"CreateEntityByName",		CreateEntityByName},
	{"DispatchSpawn",			DispatchSpawn},
	{"DispatchKeyValue",		DispatchKeyValue},
	{"DispatchKeyValueFloat",	DispatchKeyValueFloat},
	{"DispatchKeyValueVector",	DispatchKeyValueVector},
	{"GetClientAimTarget",		sm_GetClientAimTarget},
	{"SetEntityModel",			sm_SetEntityModel},
	{"GetPlayerDecalFile",		GetPlayerDecalFile},
	{"GetPlayerJingleFile",		GetPlayerJingleFile},
	{"GetServerNetStats",		GetServerNetStats},
	{"EquipPlayerWeapon",		WeaponEquip},
	{"ActivateEntity",			ActivateEntity},
	{"SetClientInfo",			SetClientInfo},
	{"SetClientName",           SetClientName},
	{"GetPlayerResourceEntity", GetPlayerResourceEntity},
	{"GivePlayerAmmo",		GivePlayerAmmo},
	{"SetEntityCollisionGroup",	SetEntityCollisionGroup},
	{"EntityCollisionRulesChanged",	EntityCollisionRulesChanged},
	{"SetEntityOwner", 				SetEntityOwner},
	{"LookupEntityAttachment", 		LookupEntityAttachment},
	{"GetEntityAttachment", 		GetEntityAttachment},
	{NULL,						NULL},
};
