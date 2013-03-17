/**
 * vim: set ts=4 :
 * =============================================================================
 * Source SDK Hooks Extension
 * Copyright (C) 2010-2012 Nicholas Hastings
 * Copyright (C) 2009-2010 Erik Minekus
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

#include <sourcemod_version.h>
#include "extension.h"
#include "compat_wrappers.h"
#include "macros.h"
#include "natives.h"
#include <sm_platform.h>
#include <const.h>


//#define SDKHOOKSDEBUG

/**
 * Globals
 */

// Order MUST match SDKHookType enum
HookTypeData g_HookTypes[SDKHook_MAXHOOKS] = 
{
//   Hook name                DT required               Supported (always false til later)
	{"EndTouch",              "",                       false},
	{"FireBulletsPost",       "",                       false},
	{"OnTakeDamage",          "",                       false},
	{"OnTakeDamagePost",      "",                       false},
	{"PreThink",              "DT_BasePlayer",          false},
	{"PostThink",             "DT_BasePlayer",          false},
	{"SetTransmit",           "",                       false},
	{"Spawn",                 "",                       false},
	{"StartTouch",            "",                       false},
	{"Think",                 "",                       false},
	{"Touch",                 "",                       false},
	{"TraceAttack",           "",                       false},
	{"TraceAttackPost",       "",                       false},
	{"WeaponCanSwitchTo",     "DT_BaseCombatCharacter", false},
	{"WeaponCanUse",          "DT_BaseCombatCharacter", false},
	{"WeaponDrop",            "DT_BaseCombatCharacter", false},
	{"WeaponEquip",           "DT_BaseCombatCharacter", false},
	{"WeaponSwitch",          "DT_BaseCombatCharacter", false},
	{"ShouldCollide",         "",                       false},
	{"PreThinkPost",          "DT_BasePlayer",          false},
	{"PostThinkPost",         "DT_BasePlayer",          false},
	{"ThinkPost",             "",                       false},
	{"EndTouchPost",          "",                       false},
	{"GroundEntChangedPost",  "",                       false},
	{"SpawnPost",             "",                       false},
	{"StartTouchPost",        "",                       false},
	{"TouchPost",             "",                       false},
	{"VPhysicsUpdate",        "",                       false},
	{"VPhysicsUpdatePost",    "",                       false},
	{"WeaponCanSwitchToPost", "DT_BaseCombatCharacter", false},
	{"WeaponCanUsePost",      "DT_BaseCombatCharacter", false},
	{"WeaponDropPost",        "DT_BaseCombatCharacter", false},
	{"WeaponEquipPost",       "DT_BaseCombatCharacter", false},
	{"WeaponSwitchPost",      "DT_BaseCombatCharacter", false},
	{"Use",                   "",                       false},
	{"UsePost",               "",                       false},
	{"Reload",                "DT_BaseCombatWeapon",    false},
	{"ReloadPost",            "DT_BaseCombatWeapon",    false},
	{"GetMaxHealth",          "",                       false},
};

SDKHooks g_Interface;
SMEXT_LINK(&g_Interface);

CGlobalVars *gpGlobals;
CUtlVector<HookList> g_HookList;

CBitVec<NUM_ENT_ENTRIES> m_EntityExists;

IBinTools *g_pBinTools = NULL;
ICvar *icvar = NULL;

// global hooks and forwards
IForward *g_pOnEntityCreated = NULL;
IForward *g_pOnEntityDestroyed = NULL;

#ifdef GAMEDESC_CAN_CHANGE
int g_hookOnGetGameDescription = 0;
IForward *g_pOnGetGameNameDescription = NULL;
#endif

int g_hookOnGetMapEntitiesString = 0;
int g_hookOnLevelInit = 0;
IForward *g_pOnLevelInit = NULL;

IGameConfig *g_pGameConf = NULL;

char g_szMapEntities[2097152];


/**
 * IServerGameDLL & IVEngineServer Hooks
 */
SH_DECL_HOOK6(IServerGameDLL, LevelInit, SH_NOATTRIB, 0, bool, const char *, const char *, const char *, const char *, bool, bool);
#ifdef GAMEDESC_CAN_CHANGE
SH_DECL_HOOK0(IServerGameDLL, GetGameDescription, SH_NOATTRIB, 0, const char *);
#endif
SH_DECL_HOOK0(IVEngineServer, GetMapEntitiesString, SH_NOATTRIB, 0, const char *);


/**
 * CBaseEntity Hooks
 */
SH_DECL_MANUALHOOK1_void(EndTouch, 0, 0, 0, CBaseEntity *);
SH_DECL_MANUALHOOK1_void(FireBullets, 0, 0, 0, FireBulletsInfo_t const&);
#ifdef GETMAXHEALTH_IS_VIRTUAL
SH_DECL_MANUALHOOK0(GetMaxHealth, 0, 0, 0, int);
#endif
SH_DECL_MANUALHOOK0_void(GroundEntChanged, 0, 0, 0);
SH_DECL_MANUALHOOK1(OnTakeDamage, 0, 0, 0, int, CTakeDamageInfoHack &);
SH_DECL_MANUALHOOK0_void(PreThink, 0, 0, 0);
SH_DECL_MANUALHOOK0_void(PostThink, 0, 0, 0);
SH_DECL_MANUALHOOK0(Reload, 0, 0, 0, bool);
SH_DECL_MANUALHOOK2_void(SetTransmit, 0, 0, 0, CCheckTransmitInfo *, bool);
SH_DECL_MANUALHOOK2(ShouldCollide, 0, 0, 0, bool, int, int);
SH_DECL_MANUALHOOK0_void(Spawn, 0, 0, 0);
SH_DECL_MANUALHOOK1_void(StartTouch, 0, 0, 0, CBaseEntity *);
SH_DECL_MANUALHOOK0_void(Think, 0, 0, 0);
SH_DECL_MANUALHOOK1_void(Touch, 0, 0, 0, CBaseEntity *);
#if SOURCE_ENGINE == SE_ORANGEBOXVALVE || SOURCE_ENGINE == SE_CSS
SH_DECL_MANUALHOOK4_void(TraceAttack, 0, 0, 0, CTakeDamageInfoHack &, const Vector &, CGameTrace *, void *);
#else
SH_DECL_MANUALHOOK3_void(TraceAttack, 0, 0, 0, CTakeDamageInfoHack &, const Vector &, CGameTrace *);
#endif
SH_DECL_MANUALHOOK4_void(Use, 0, 0, 0, CBaseEntity *, CBaseEntity *, USE_TYPE, float);
SH_DECL_MANUALHOOK1_void(VPhysicsUpdate, 0, 0, 0, IPhysicsObject *);
SH_DECL_MANUALHOOK1(Weapon_CanSwitchTo, 0, 0, 0, bool, CBaseCombatWeapon *);
SH_DECL_MANUALHOOK1(Weapon_CanUse, 0, 0, 0, bool, CBaseCombatWeapon *);
SH_DECL_MANUALHOOK3_void(Weapon_Drop, 0, 0, 0, CBaseCombatWeapon *, const Vector *, const Vector *);
SH_DECL_MANUALHOOK1_void(Weapon_Equip, 0, 0, 0, CBaseCombatWeapon *);
SH_DECL_MANUALHOOK2(Weapon_Switch, 0, 0, 0, bool, CBaseCombatWeapon *, int);


/**
 * Forwards
 */
bool SDKHooks::SDK_OnLoad(char *error, size_t maxlength, bool late)
{
	char buffer[256];
	g_pSM->BuildPath(Path_SM, buffer, sizeof(buffer)-1, "/extensions/sdkhooks.ext." PLATFORM_LIB_EXT);
	if (libsys->PathExists(buffer) && libsys->IsPathFile(buffer))
	{
		g_pSM->Format(error, maxlength-1, "SDKHooks 2.x cannot load while old version (sdkhooks.ext." PLATFORM_LIB_EXT ") is still in extensions dir");
		return false;
	}

	g_pSM->BuildPath(Path_SM, buffer, sizeof(buffer)-1, "/gamedata/sdkhooks.games.txt");
	if (libsys->PathExists(buffer) && libsys->IsPathFile(buffer))
	{
		g_pSM->Format(error, maxlength-1, "SDKHooks 2.x cannot load while old gamedata file (sdkhooks.games.txt) is still in gamedata dir");
		return false;
	}

	sharesys->AddDependency(myself, "bintools.ext", true, true);
	sharesys->AddNatives(myself, g_Natives);
	sharesys->RegisterLibrary(myself, "sdkhooks");
	sharesys->AddCapabilityProvider(myself, this, "SDKHook_DmgCustomInOTD");
	sharesys->AddCapabilityProvider(myself, this, "SDKHook_LogicalEntSupport");

	playerhelpers->AddClientListener(&g_Interface);
	
	plsys->AddPluginsListener(&g_Interface);

	g_pOnEntityCreated = forwards->CreateForward("OnEntityCreated", ET_Ignore, 2, NULL, Param_Cell, Param_String);
	g_pOnEntityDestroyed = forwards->CreateForward("OnEntityDestroyed", ET_Ignore, 1, NULL, Param_Cell);
#ifdef GAMEDESC_CAN_CHANGE
	g_pOnGetGameNameDescription = forwards->CreateForward("OnGetGameDescription", ET_Hook, 2, NULL, Param_String);
#endif
	g_pOnLevelInit = forwards->CreateForward("OnLevelInit", ET_Hook, 2, NULL, Param_String, Param_String);

	buffer[0] = '\0';
	if (!gameconfs->LoadGameConfigFile("sdkhooks.games", &g_pGameConf, buffer, sizeof(buffer)))
	{
		if (buffer[0])
		{
			g_pSM->Format(error, maxlength, "Could not read sdkhooks.games gamedata: %s", buffer);
		}
		
		return false;
	}

	void *gEntList = gamehelpers->GetGlobalEntityList();
	if (!gEntList)
	{
		g_pSM->Format(error, maxlength, "Cannot find gEntList pointer");
		return false;
	}

	int offset = -1; /* 65572 */
	if (!g_pGameConf->GetOffset("EntityListeners", &offset))
	{
		g_pSM->Format(error, maxlength, "Cannot find EntityListeners offset");
		return false;
	}

	CUtlVector<IEntityListener *> *pListeners = (CUtlVector<IEntityListener *> *)((intptr_t)gEntList + offset);
	pListeners->AddToTail(this);

	SetupHooks();

#if SOURCE_ENGINE >= SE_ORANGEBOX
	int index;
	CBaseHandle hndl;
	for (IHandleEntity *pEnt = (IHandleEntity *)servertools->FirstEntity(); pEnt; pEnt = (IHandleEntity *)servertools->NextEntity((CBaseEntity *)pEnt))
	{
		hndl = pEnt->GetRefEHandle();
		if (hndl == INVALID_EHANDLE_INDEX || !hndl.IsValid())
			continue;
		
		index = hndl.GetEntryIndex();
		m_EntityExists.Set(index);
	}
#else
	for (int i = 0; i < NUM_ENT_ENTRIES; i++)
	{
		if (gamehelpers->ReferenceToEntity(i) != NULL)
			m_EntityExists.Set(i);
	}
#endif

	return true;
}

inline void HookLevelInit()
{
	assert(g_hookOnLevelInit == 0);
	g_hookOnLevelInit = SH_ADD_HOOK(IServerGameDLL, LevelInit, gamedll, SH_MEMBER(&g_Interface, &SDKHooks::Hook_LevelInit), false);
	assert(g_hookOnGetMapEntitiesString == 0);
	g_hookOnGetMapEntitiesString = SH_ADD_HOOK(IVEngineServer, GetMapEntitiesString, engine, SH_MEMBER(&g_Interface, &SDKHooks::Hook_GetMapEntitiesString), false);
}

#ifdef GAMEDESC_CAN_CHANGE
inline void HookGetGameDescription()
{
	assert(g_hookOnGetGameDescription == 0);
	g_hookOnGetGameDescription = SH_ADD_HOOK(IServerGameDLL, GetGameDescription, gamedll, SH_MEMBER(&g_Interface, &SDKHooks::Hook_GetGameDescription), false);
}
#endif

void SDKHooks::SDK_OnAllLoaded()
{
	SM_GET_LATE_IFACE(BINTOOLS, g_pBinTools);

	if (!g_pBinTools)
	{
		g_pSM->LogError(myself, "Could not find interface: " SMINTERFACE_BINTOOLS_NAME);
		return;
	}

	if (g_pOnLevelInit->GetFunctionCount() > 0)
		HookLevelInit();
#ifdef GAMEDESC_CAN_CHANGE
	if (g_pOnGetGameNameDescription->GetFunctionCount() > 0)
		HookGetGameDescription();
#endif
}

#define KILL_HOOK_IF_ACTIVE(hook) \
	if (hook != 0) \
	{ \
		SH_REMOVE_HOOK_ID(hook); \
	}

void SDKHooks::SDK_OnUnload()
{
	// Remove left over hooks
	HOOKLOOP
		Unhook(i);

	KILL_HOOK_IF_ACTIVE(g_hookOnLevelInit);
	KILL_HOOK_IF_ACTIVE(g_hookOnGetMapEntitiesString);

#ifdef GAMEDESC_CAN_CHANGE
	KILL_HOOK_IF_ACTIVE(g_hookOnGetGameDescription);
#endif

	forwards->ReleaseForward(g_pOnEntityCreated);
	forwards->ReleaseForward(g_pOnEntityDestroyed);
#ifdef GAMEDESC_CAN_CHANGE
	forwards->ReleaseForward(g_pOnGetGameNameDescription);
#endif
	forwards->ReleaseForward(g_pOnLevelInit);

	plsys->RemovePluginsListener(&g_Interface);

	playerhelpers->RemoveClientListener(&g_Interface);

	sharesys->DropCapabilityProvider(myself, this, "SDKHook_DmgCustomInOTD");
	sharesys->DropCapabilityProvider(myself, this, "SDKHook_LogicalEntSupport");

	void *gEntList = gamehelpers->GetGlobalEntityList();
	if (gEntList)
	{
		int offset = -1; /* 65572 */
		if (g_pGameConf->GetOffset("EntityListeners", &offset))
		{
			CUtlVector<IEntityListener *> *pListeners = (CUtlVector<IEntityListener *> *)((intptr_t)gEntList + offset);
			pListeners->FindAndRemove(this);
		}
	}

	gameconfs->CloseGameConfigFile(g_pGameConf);
}

bool SDKHooks::SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	GET_V_IFACE_CURRENT(GetEngineFactory, icvar, ICvar, CVAR_INTERFACE_VERSION);

#if SOURCE_ENGINE >= SE_ORANGEBOX
	g_pCVar = icvar;
#endif
	CONVAR_REGISTER(this);
	
	gpGlobals = ismm->GetCGlobals();

	return true;
}

const char *SDKHooks::GetExtensionVerString()
{
	return SM_VERSION_STRING;
}

const char *SDKHooks::GetExtensionDateString()
{
	return SM_BUILD_TIMESTAMP;
}

void SDKHooks::OnPluginLoaded(IPlugin *plugin)
{
	if (g_pOnLevelInit->GetFunctionCount() > 0 && g_hookOnLevelInit == 0)
		HookLevelInit();

#ifdef GAMEDESC_CAN_CHANGE
	if (g_pOnGetGameNameDescription->GetFunctionCount() > 0 && g_hookOnGetGameDescription == 0)
		HookGetGameDescription();
#endif
}

void SDKHooks::OnPluginUnloaded(IPlugin *plugin)
{
	IPluginContext *plugincontext = plugin->GetBaseContext();
	HOOKLOOP
	{
		if(g_HookList[i].callback->GetParentContext() == plugincontext)
			Unhook(i);
	}

	if (g_pOnLevelInit->GetFunctionCount() == 0)
	{
		KILL_HOOK_IF_ACTIVE(g_hookOnLevelInit);
		KILL_HOOK_IF_ACTIVE(g_hookOnGetMapEntitiesString);
	}

#ifdef GAMEDESC_CAN_CHANGE
	if (g_pOnGetGameNameDescription->GetFunctionCount() == 0)
		KILL_HOOK_IF_ACTIVE(g_hookOnGetGameDescription);
#endif
}

void SDKHooks::OnClientPutInServer(int client)
{
	g_pOnEntityCreated->PushCell(client);

	CBaseEntity *pPlayer = gamehelpers->ReferenceToEntity(client);

	const char * pName = gamehelpers->GetEntityClassname(pPlayer);

	g_pOnEntityCreated->PushString(pName ? pName : "");
	g_pOnEntityCreated->Execute(NULL);

	m_EntityExists.Set(client);
}

bool SDKHooks::RegisterConCommandBase(ConCommandBase *pVar)
{
	/* Always call META_REGCVAR instead of going through the engine. */
	return META_REGCVAR(pVar);
}

FeatureStatus SDKHooks::GetFeatureStatus(FeatureType type, const char *name)
{
	return FeatureStatus_Available;
}

/**
 * Functions
 */
cell_t SDKHooks::Call(int entity, SDKHookType type, int other)
{
	IPluginFunction *callback = NULL;
	cell_t res, ret = Pl_Continue;

	HOOKLOOP
	{
		if(g_HookList[i].entity != entity || g_HookList[i].type != type)
			continue;

		callback = g_HookList[i].callback;
		callback->PushCell(entity);
		if(other > -2)
			callback->PushCell(other);

		callback->Execute(&res);
		if(res > ret)
			ret = res;
	}

	return ret;
}

cell_t SDKHooks::Call(CBaseEntity *pEnt, SDKHookType type, int other)
{
	return Call(gamehelpers->EntityToBCompatRef(pEnt), type, other);
}

cell_t SDKHooks::Call(CBaseEntity *pEnt, SDKHookType type, CBaseEntity *other)
{
	return Call(gamehelpers->EntityToBCompatRef(pEnt), type, gamehelpers->EntityToBCompatRef(other));
}

void SDKHooks::SetupHooks()
{
	int offset;

	//			gamedata          pre    post
	// (pre is not necessarily a prehook, just named without "Post" appeneded)

	CHECKOFFSET(EndTouch,         true,  true);
	CHECKOFFSET(FireBullets,      false, true);
	CHECKOFFSET(GroundEntChanged, false, true);
	CHECKOFFSET(OnTakeDamage,     true,  true);
	CHECKOFFSET(PreThink,         true,  true);
	CHECKOFFSET(PostThink,        true,  true);
	CHECKOFFSET(Reload,           true,  true);
	CHECKOFFSET(SetTransmit,      true,  false);
	CHECKOFFSET(ShouldCollide,    true,  false);
	CHECKOFFSET(Spawn,            true,  true);
	CHECKOFFSET(StartTouch,       true,  true);
	CHECKOFFSET(Think,            true,  true);
	CHECKOFFSET(Touch,            true,  true);
	CHECKOFFSET(TraceAttack,      true,  true);
	CHECKOFFSET(Use,              true,  true);
	CHECKOFFSET_W(CanSwitchTo,    true,  true);
	CHECKOFFSET_W(CanUse,         true,  true);
	CHECKOFFSET_W(Drop,           true,  true);
	CHECKOFFSET_W(Equip,          true,  true);
	CHECKOFFSET_W(Switch,         true,  true);
	CHECKOFFSET(VPhysicsUpdate,   true,  true);

	// this one is in a class all its own -_-
	offset = 0;
	g_pGameConf->GetOffset("GroundEntChanged", &offset);
	if (offset > 0)
	{
		SH_MANUALHOOK_RECONFIGURE(GroundEntChanged, offset, 0, 0);
		g_HookTypes[SDKHook_GroundEntChangedPost].supported = true;
	}

#ifdef GETMAXHEALTH_IS_VIRTUAL
	CHECKOFFSET(GetMaxHealth,	true,	false);
#endif
}

HookReturn SDKHooks::Hook(int entity, SDKHookType type, IPluginFunction *callback)
{
	if(!g_HookTypes[type].supported)
		return HookRet_NotSupported;

	CBaseEntity *pEnt = UTIL_GetCBaseEntity(entity);
	if(!pEnt)
		return HookRet_InvalidEntity;
	if(type < 0 || type >= SDKHook_MAXHOOKS)
		return HookRet_InvalidHookType;

	if (!!strcmp(g_HookTypes[type].dtReq, ""))
	{
		sm_sendprop_info_t spi;
		IServerUnknown *pUnk = (IServerUnknown *)pEnt;

		IServerNetworkable *pNet = pUnk->GetNetworkable();
		if (pNet && !UTIL_FindDataTable(pNet->GetServerClass()->m_pTable, g_HookTypes[type].dtReq, &spi, 0))
			return HookRet_BadEntForHookType;
	}

	bool bHooked = false;
	HOOKLOOP
	{
		if (g_HookList[i].entity == entity && g_HookList[i].type == type)
		{
			bHooked = true;
			break;
		}
	}
	if (!bHooked)
	{
		switch(type)
		{
			case SDKHook_EndTouch:
				SH_ADD_MANUALHOOK(EndTouch, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_EndTouch), false);
				break;
			case SDKHook_EndTouchPost:
				SH_ADD_MANUALHOOK(EndTouch, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_EndTouchPost), true);
				break;
			case SDKHook_FireBulletsPost:
				SH_ADD_MANUALHOOK(FireBullets, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_FireBulletsPost), true);
				break;
#ifdef GETMAXHEALTH_IS_VIRTUAL
			case SDKHook_GetMaxHealth:
				SH_ADD_MANUALHOOK(GetMaxHealth, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_GetMaxHealth), false);
				break;
#endif
			case SDKHook_GroundEntChangedPost:
				SH_ADD_MANUALHOOK(GroundEntChanged, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_GroundEntChangedPost), true);
				break;
			case SDKHook_OnTakeDamage:
				SH_ADD_MANUALHOOK(OnTakeDamage, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_OnTakeDamage), false);
				break;
			case SDKHook_OnTakeDamagePost:
				SH_ADD_MANUALHOOK(OnTakeDamage, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_OnTakeDamagePost), true);
				break;
			case SDKHook_PreThink:
				SH_ADD_MANUALHOOK(PreThink, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_PreThink), false);
				break;
			case SDKHook_PreThinkPost:
				SH_ADD_MANUALHOOK(PreThink, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_PreThinkPost), true);
				break;
			case SDKHook_PostThink:
				SH_ADD_MANUALHOOK(PostThink, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_PostThink), false);
				break;
			case SDKHook_PostThinkPost:
				SH_ADD_MANUALHOOK(PostThink, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_PostThinkPost), true);
				break;
			case SDKHook_Reload:
				SH_ADD_MANUALHOOK(Reload, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_Reload), false);
				break;
			case SDKHook_ReloadPost:
				SH_ADD_MANUALHOOK(Reload, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_ReloadPost), true);
				break;
			case SDKHook_SetTransmit:
				SH_ADD_MANUALHOOK(SetTransmit, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_SetTransmit), false);
				break;
			case SDKHook_Spawn:
				SH_ADD_MANUALHOOK(Spawn, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_Spawn), false);
				break;
			case SDKHook_SpawnPost:
				SH_ADD_MANUALHOOK(Spawn, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_SpawnPost), true);
				break;
			case SDKHook_StartTouch:
				SH_ADD_MANUALHOOK(StartTouch, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_StartTouch), false);
				break;
			case SDKHook_StartTouchPost:
				SH_ADD_MANUALHOOK(StartTouch, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_StartTouchPost), true);
				break;
			case SDKHook_Think:
				SH_ADD_MANUALHOOK(Think, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_Think), false);
				break;
			case SDKHook_ThinkPost:
				SH_ADD_MANUALHOOK(Think, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_ThinkPost), true);
				break;
			case SDKHook_Touch:
				SH_ADD_MANUALHOOK(Touch, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_Touch), false);
				break;
			case SDKHook_TouchPost:
				SH_ADD_MANUALHOOK(Touch, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_TouchPost), true);
				break;
			case SDKHook_TraceAttack:
				SH_ADD_MANUALHOOK(TraceAttack, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_TraceAttack), false);
				break;
			case SDKHook_TraceAttackPost:
				SH_ADD_MANUALHOOK(TraceAttack, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_TraceAttackPost), true);
				break;
			case SDKHook_Use:
				SH_ADD_MANUALHOOK(Use, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_Use), false);
				break;
			case SDKHook_UsePost:
				SH_ADD_MANUALHOOK(Use, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_UsePost), true);
				break;
			case SDKHook_VPhysicsUpdate:
				SH_ADD_MANUALHOOK(VPhysicsUpdate, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_VPhysicsUpdate), false);
				break;
			case SDKHook_VPhysicsUpdatePost:
				SH_ADD_MANUALHOOK(VPhysicsUpdate, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_VPhysicsUpdatePost), true);
				break;
			case SDKHook_WeaponCanSwitchTo:
				SH_ADD_MANUALHOOK(Weapon_CanSwitchTo, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_WeaponCanSwitchTo), false);
				break;
			case SDKHook_WeaponCanSwitchToPost:
				SH_ADD_MANUALHOOK(Weapon_CanSwitchTo, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_WeaponCanSwitchToPost), true);
				break;
			case SDKHook_WeaponCanUse:
				SH_ADD_MANUALHOOK(Weapon_CanUse, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_WeaponCanUse), false);
				break;
			case SDKHook_WeaponCanUsePost:
				SH_ADD_MANUALHOOK(Weapon_CanUse, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_WeaponCanUsePost), true);
				break;
			case SDKHook_WeaponDrop:
				SH_ADD_MANUALHOOK(Weapon_Drop, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_WeaponDrop), false);
				break;
			case SDKHook_WeaponDropPost:
				SH_ADD_MANUALHOOK(Weapon_Drop, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_WeaponDropPost), true);
				break;
			case SDKHook_WeaponEquip:
				SH_ADD_MANUALHOOK(Weapon_Equip, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_WeaponEquip), false);
				break;
			case SDKHook_WeaponEquipPost:
				SH_ADD_MANUALHOOK(Weapon_Equip, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_WeaponEquipPost), true);
				break;
			case SDKHook_WeaponSwitch:
				SH_ADD_MANUALHOOK(Weapon_Switch, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_WeaponSwitch), false);
				break;
			case SDKHook_WeaponSwitchPost:
				SH_ADD_MANUALHOOK(Weapon_Switch, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_WeaponSwitchPost), true);
				break;
			case SDKHook_ShouldCollide:
				SH_ADD_MANUALHOOK(ShouldCollide, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_ShouldCollide), false);
				break;
		}
	}

	// Add hook to hook list
	HookList hook;
	hook.entity = entity;
	hook.type = type;
	hook.callback = callback;
	g_HookList.AddToTail(hook);
#ifdef SDKHOOKSDEBUG
	META_CONPRINTF("DEBUG: Adding to hooklist (ent%d, type%s, cb%d). Total hook count %d\n", entity, g_szHookNames[type], callback, g_HookList.Count());
#endif
	return HookRet_Successful;
}

void SDKHooks::Unhook(int index)
{
	CBaseEntity *pEnt = gamehelpers->ReferenceToEntity(g_HookList[index].entity);
	if(!pEnt)
		return;

	int iHooks = 0;
	HOOKLOOP
	{
		if (g_HookList[i].entity == g_HookList[index].entity && g_HookList[i].type == g_HookList[index].type)
		{
			iHooks++;
#ifdef SDKHOOKSDEBUG
			META_CONPRINTF("DEBUG: Found hook %d on entity %d\n", i, g_HookList[index].entity);
#endif
		}
	}
	if (iHooks == 1)
	{
#ifdef SDKHOOKSDEBUG
		META_CONPRINTF("DEBUG: Removing hook for hooktype %d\n", g_HookList[index].type);
#endif
		switch(g_HookList[index].type)
		{
			case SDKHook_EndTouch:
				SH_REMOVE_MANUALHOOK(EndTouch, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_EndTouch), false);
				break;
			case SDKHook_EndTouchPost:
				SH_REMOVE_MANUALHOOK(EndTouch, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_EndTouchPost), true);
				break;
			case SDKHook_FireBulletsPost:
				SH_REMOVE_MANUALHOOK(FireBullets, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_FireBulletsPost), true);
				break;
#ifdef GETMAXHEALTH_IS_VIRTUAL
			case SDKHook_GetMaxHealth:
				SH_REMOVE_MANUALHOOK(GetMaxHealth, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_GetMaxHealth), false);
				break;
#endif
			case SDKHook_GroundEntChangedPost:
				SH_REMOVE_MANUALHOOK(GroundEntChanged, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_GroundEntChangedPost), true);
				break;
			case SDKHook_OnTakeDamage:
				SH_REMOVE_MANUALHOOK(OnTakeDamage, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_OnTakeDamage), false);
				break;
			case SDKHook_OnTakeDamagePost:
				SH_REMOVE_MANUALHOOK(OnTakeDamage, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_OnTakeDamagePost), true);
				break;
			case SDKHook_PreThink:
				SH_REMOVE_MANUALHOOK(PreThink, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_PreThink), false);
				break;
			case SDKHook_PreThinkPost:
				SH_REMOVE_MANUALHOOK(PreThink, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_PreThinkPost), true);
				break;
			case SDKHook_PostThink:
				SH_REMOVE_MANUALHOOK(PostThink, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_PostThink), false);
				break;
			case SDKHook_PostThinkPost:
				SH_REMOVE_MANUALHOOK(PostThink, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_PostThinkPost), true);
				break;
			case SDKHook_Reload:
				SH_REMOVE_MANUALHOOK(Reload, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_Reload), false);
				break;
			case SDKHook_ReloadPost:
				SH_REMOVE_MANUALHOOK(Reload, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_ReloadPost), true);
				break;
			case SDKHook_SetTransmit:
				SH_REMOVE_MANUALHOOK(SetTransmit, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_SetTransmit), false);
				break;
			case SDKHook_Spawn:
				SH_REMOVE_MANUALHOOK(Spawn, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_Spawn), false);
				break;
			case SDKHook_SpawnPost:
				SH_REMOVE_MANUALHOOK(Spawn, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_SpawnPost), true);
				break;
			case SDKHook_StartTouch:
				SH_REMOVE_MANUALHOOK(StartTouch, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_StartTouch), false);
				break;
			case SDKHook_StartTouchPost:
				SH_REMOVE_MANUALHOOK(StartTouch, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_StartTouchPost), true);
				break;
			case SDKHook_Think:
				SH_REMOVE_MANUALHOOK(Think, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_Think), false);
				break;
			case SDKHook_ThinkPost:
				SH_REMOVE_MANUALHOOK(Think, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_ThinkPost), true);
				break;
			case SDKHook_Touch:
				SH_REMOVE_MANUALHOOK(Touch, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_Touch), false);
				break;
			case SDKHook_TouchPost:
				SH_REMOVE_MANUALHOOK(Touch, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_TouchPost), true);
				break;
			case SDKHook_TraceAttack:
				SH_REMOVE_MANUALHOOK(TraceAttack, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_TraceAttack), false);
				break;
			case SDKHook_TraceAttackPost:
				SH_REMOVE_MANUALHOOK(TraceAttack, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_TraceAttackPost), true);
				break;
			case SDKHook_Use:
				SH_REMOVE_MANUALHOOK(Use, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_Use), false);
				break;
			case SDKHook_UsePost:
				SH_REMOVE_MANUALHOOK(Use, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_UsePost), true);
				break;
			case SDKHook_VPhysicsUpdate:
				SH_REMOVE_MANUALHOOK(VPhysicsUpdate, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_VPhysicsUpdate), false);
				break;
			case SDKHook_VPhysicsUpdatePost:
				SH_REMOVE_MANUALHOOK(VPhysicsUpdate, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_VPhysicsUpdatePost), true);
				break;
			case SDKHook_WeaponCanSwitchTo:
				SH_REMOVE_MANUALHOOK(Weapon_CanSwitchTo, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_WeaponCanSwitchTo), false);
				break;
			case SDKHook_WeaponCanSwitchToPost:
				SH_REMOVE_MANUALHOOK(Weapon_CanSwitchTo, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_WeaponCanSwitchToPost), true);
				break;
			case SDKHook_WeaponCanUse:
				SH_REMOVE_MANUALHOOK(Weapon_CanUse, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_WeaponCanUse), false);
				break;
			case SDKHook_WeaponCanUsePost:
				SH_REMOVE_MANUALHOOK(Weapon_CanUse, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_WeaponCanUsePost), true);
				break;
			case SDKHook_WeaponDrop:
				SH_REMOVE_MANUALHOOK(Weapon_Drop, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_WeaponDrop), false);
				break;
			case SDKHook_WeaponDropPost:
				SH_REMOVE_MANUALHOOK(Weapon_Drop, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_WeaponDropPost), true);
				break;
			case SDKHook_WeaponEquip:
				SH_REMOVE_MANUALHOOK(Weapon_Equip, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_WeaponEquip), false);
				break;
			case SDKHook_WeaponEquipPost:
				SH_REMOVE_MANUALHOOK(Weapon_Equip, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_WeaponEquipPost), true);
				break;
			case SDKHook_WeaponSwitch:
				SH_REMOVE_MANUALHOOK(Weapon_Switch, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_WeaponSwitch), false);
				break;
			case SDKHook_WeaponSwitchPost:
				SH_REMOVE_MANUALHOOK(Weapon_Switch, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_WeaponSwitchPost), true);
				break;
			case SDKHook_ShouldCollide:
				SH_REMOVE_MANUALHOOK(ShouldCollide, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_ShouldCollide), false);
				break;
			default:
				return;
		}
	}
	g_HookList.Remove(index);
}


/**
 * IEntityFactoryDictionary, IServerGameDLL & IVEngineServer Hook Handlers
 */
void SDKHooks::OnEntityCreated(CBaseEntity *pEntity)
{
	// Call OnEntityCreated forward
	int entity = gamehelpers->ReferenceToIndex(gamehelpers->EntityToBCompatRef(pEntity));
	if (m_EntityExists.IsBitSet(entity) || (entity > 0 && entity <= playerhelpers->GetMaxClients()))
	{
		return;
	}

	g_pOnEntityCreated->PushCell(gamehelpers->EntityToBCompatRef(pEntity));

	const char * pName = gamehelpers->GetEntityClassname(pEntity);
	g_pOnEntityCreated->PushString(pName ? pName : "");

	g_pOnEntityCreated->Execute(NULL);

	m_EntityExists.Set(entity);
}

#ifdef GAMEDESC_CAN_CHANGE
const char *SDKHooks::Hook_GetGameDescription()
{
	static char szGameDesc[64];
	cell_t result = Pl_Continue;

	g_pSM->Format(szGameDesc, sizeof(szGameDesc), "%s",
		SH_CALL(gamedll, &IServerGameDLL::GetGameDescription)());

	// Call OnGetGameDescription forward
	g_pOnGetGameNameDescription->PushStringEx(szGameDesc, sizeof(szGameDesc), SM_PARAM_STRING_COPY, SM_PARAM_COPYBACK);
	g_pOnGetGameNameDescription->Execute(&result);

	if(result == Pl_Changed)
		RETURN_META_VALUE(MRES_SUPERCEDE, szGameDesc);

	RETURN_META_VALUE(MRES_IGNORED, NULL);
}
#endif

const char *SDKHooks::Hook_GetMapEntitiesString()
{
	if(g_szMapEntities[0])
		RETURN_META_VALUE(MRES_SUPERCEDE, g_szMapEntities);

	RETURN_META_VALUE(MRES_IGNORED, NULL);
}

bool SDKHooks::Hook_LevelInit(char const *pMapName, char const *pMapEntities, char const *pOldLevel, char const *pLandmarkName, bool loadGame, bool background)
{
	strcpy(g_szMapEntities, pMapEntities);
	cell_t result = Pl_Continue;

	// Call OnLevelInit forward
	g_pOnLevelInit->PushString(pMapName);
	g_pOnLevelInit->PushStringEx(g_szMapEntities, sizeof(g_szMapEntities), SM_PARAM_STRING_COPY, SM_PARAM_COPYBACK);
	g_pOnLevelInit->Execute(&result);

	if(result >= Pl_Handled)
		RETURN_META_VALUE(MRES_SUPERCEDE, false);

	if(result == Pl_Changed)
		RETURN_META_VALUE_NEWPARAMS(MRES_HANDLED, true, &IServerGameDLL::LevelInit, (pMapName, g_szMapEntities, pOldLevel, pLandmarkName, loadGame, background));

	RETURN_META_VALUE(MRES_IGNORED, true);
}


/**
 * CBaseEntity Hook Handlers
 */
void SDKHooks::Hook_EndTouch(CBaseEntity *pOther)
{
	cell_t result = Call(META_IFACEPTR(CBaseEntity), SDKHook_EndTouch, pOther);
	if(result >= Pl_Handled)
		RETURN_META(MRES_SUPERCEDE);

	RETURN_META(MRES_IGNORED);
}

void SDKHooks::Hook_EndTouchPost(CBaseEntity *pOther)
{
	Call(META_IFACEPTR(CBaseEntity), SDKHook_EndTouchPost, pOther);
	RETURN_META(MRES_IGNORED);
}

void SDKHooks::Hook_FireBulletsPost(const FireBulletsInfo_t &info)
{
	int entity = gamehelpers->EntityToBCompatRef(META_IFACEPTR(CBaseEntity));

	IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(entity);
	if(!pPlayer)
		RETURN_META(MRES_IGNORED);

	IPlayerInfo *pInfo = pPlayer->GetPlayerInfo();
	if(!pInfo)
		RETURN_META(MRES_IGNORED);

	const char *weapon = pInfo->GetWeaponName();
	IPluginFunction *callback = NULL;

	HOOKLOOP
	{
		if(g_HookList[i].entity != entity || g_HookList[i].type != SDKHook_FireBulletsPost)
			continue;

		callback = g_HookList[i].callback;
		callback->PushCell(entity);
		callback->PushCell(info.m_iShots);
		callback->PushString(weapon?weapon:"");
		callback->Execute(NULL);
	}

	RETURN_META(MRES_IGNORED);
}

#ifdef GETMAXHEALTH_IS_VIRTUAL
int SDKHooks::Hook_GetMaxHealth()
{
	CBaseEntity *pEntity = META_IFACEPTR(CBaseEntity);
	int original_max = SH_MCALL(pEntity, GetMaxHealth)();
	int entity = gamehelpers->EntityToBCompatRef(pEntity);

	int new_max = original_max;

	cell_t res;
	IPluginFunction *callback = NULL;

	HOOKLOOP
	{
		if (g_HookList[i].entity != entity || g_HookList[i].type != SDKHook_GetMaxHealth)
			continue;

		callback = g_HookList[i].callback;
		callback->PushCell(entity);
		callback->PushCellByRef(&new_max);
		callback->Execute(&res);
	}

	if (res >= Pl_Changed)
		RETURN_META_VALUE(MRES_SUPERCEDE, new_max);

	RETURN_META_VALUE(MRES_IGNORED, original_max);
}
#endif

void SDKHooks::Hook_GroundEntChangedPost()
{
	Call(META_IFACEPTR(CBaseEntity), SDKHook_GroundEntChangedPost);
}

int SDKHooks::Hook_OnTakeDamage(CTakeDamageInfoHack &info)
{
	int entity = gamehelpers->EntityToBCompatRef(META_IFACEPTR(CBaseEntity));
	int attacker = info.GetAttacker();
	int inflictor = info.GetInflictor();
	float damage = info.GetDamage();
	int damagetype = info.GetDamageType();
	int weapon = info.GetWeapon();

	Vector force = info.GetDamageForce();
	cell_t damageForce[3] = {sp_ftoc(force.x), sp_ftoc(force.y), sp_ftoc(force.z)};

	Vector pos = info.GetDamagePosition();
	cell_t damagePosition[3] = {sp_ftoc(pos.x), sp_ftoc(pos.y), sp_ftoc(pos.z)};

	IPluginFunction *callback = NULL;
	cell_t res, ret = Pl_Continue;

	HOOKLOOP
	{
		if(g_HookList[i].entity != entity || g_HookList[i].type != SDKHook_OnTakeDamage)
			continue;

		callback = g_HookList[i].callback;
		callback->PushCell(entity);
		callback->PushCellByRef(&attacker);
		callback->PushCellByRef(&inflictor);
		callback->PushFloatByRef(&damage);
		callback->PushCellByRef(&damagetype);
		callback->PushCellByRef(&weapon);
		callback->PushArray(damageForce, 3, SM_PARAM_COPYBACK);
		callback->PushArray(damagePosition, 3, SM_PARAM_COPYBACK);	
		callback->PushCell(info.GetDamageCustom());
		callback->Execute(&res);

		if(res > ret)
			ret = res;
	}

	if(ret >= Pl_Handled)
		RETURN_META_VALUE(MRES_SUPERCEDE, 1);

	if(ret == Pl_Changed)
	{
		CBaseEntity *pEntAttacker = gamehelpers->ReferenceToEntity(attacker);
		if(!pEntAttacker)
		{
			callback->GetParentContext()->ThrowNativeError("Entity %d for attacker is invalid", attacker);
			RETURN_META_VALUE(MRES_IGNORED, 0);
		}
		CBaseEntity *pEntInflictor = gamehelpers->ReferenceToEntity(inflictor);
		if(!pEntInflictor)
		{
			callback->GetParentContext()->ThrowNativeError("Entity %d for inflictor is invalid", inflictor);
			RETURN_META_VALUE(MRES_IGNORED, 0);
		}

		info.SetAttacker(pEntAttacker);
		info.SetInflictor(pEntInflictor);
		info.SetDamage(damage);
		info.SetDamageType(damagetype);
		info.SetWeapon(gamehelpers->ReferenceToEntity(weapon));
		info.SetDamageForce(
			sp_ctof(damageForce[0]),
			sp_ctof(damageForce[1]),
			sp_ctof(damageForce[2]));
		info.SetDamagePosition(
			sp_ctof(damagePosition[0]),
			sp_ctof(damagePosition[1]),
			sp_ctof(damagePosition[2]));

		RETURN_META_VALUE(MRES_HANDLED, 1); 
	}

	RETURN_META_VALUE(MRES_IGNORED, 0);
}

int SDKHooks::Hook_OnTakeDamagePost(CTakeDamageInfoHack &info)
{
	int entity = gamehelpers->EntityToBCompatRef(META_IFACEPTR(CBaseEntity));
	IPluginFunction *callback = NULL;

	HOOKLOOP
	{
		if(g_HookList[i].entity != entity || g_HookList[i].type != SDKHook_OnTakeDamagePost)
			continue;

		callback = g_HookList[i].callback;
		callback->PushCell(entity);
		callback->PushCell(info.GetAttacker());
		callback->PushCell(info.GetInflictor());
		callback->PushFloat(info.GetDamage());
		callback->PushCell(info.GetDamageType());
		callback->PushCell(info.GetWeapon());

		Vector force = info.GetDamageForce();
		cell_t damageForce[3] = {sp_ftoc(force.x), sp_ftoc(force.y), sp_ftoc(force.z)};
		callback->PushArray(damageForce, 3);

		Vector pos = info.GetDamagePosition();
		cell_t damagePosition[3] = {sp_ftoc(pos.x), sp_ftoc(pos.y), sp_ftoc(pos.z)};
		callback->PushArray(damagePosition, 3);

		callback->PushCell(info.GetDamageCustom());

		callback->Execute(NULL);
	}

	RETURN_META_VALUE(MRES_IGNORED, 0);
}

void SDKHooks::Hook_PreThink()
{
	Call(META_IFACEPTR(CBaseEntity), SDKHook_PreThink);
}

void SDKHooks::Hook_PreThinkPost()
{
	Call(META_IFACEPTR(CBaseEntity), SDKHook_PreThinkPost);
}

void SDKHooks::Hook_PostThink()
{
	Call(META_IFACEPTR(CBaseEntity), SDKHook_PostThink);
}

void SDKHooks::Hook_PostThinkPost()
{
	Call(META_IFACEPTR(CBaseEntity), SDKHook_PostThinkPost);
}

bool SDKHooks::Hook_Reload()
{
	int entity = gamehelpers->EntityToBCompatRef(META_IFACEPTR(CBaseEntity));
	cell_t res;
	IPluginFunction *callback = NULL;

	HOOKLOOP
	{
		if (g_HookList[i].entity != entity || g_HookList[i].type != SDKHook_Reload)
			continue;

		callback = g_HookList[i].callback;
		callback->PushCell(entity);
		callback->Execute(&res);
	}

	if (res >= Pl_Handled)
		RETURN_META_VALUE(MRES_SUPERCEDE, false);

	RETURN_META_VALUE(MRES_IGNORED, true);
}

bool SDKHooks::Hook_ReloadPost()
{
	int entity = gamehelpers->EntityToBCompatRef(META_IFACEPTR(CBaseEntity));
	IPluginFunction *callback = NULL;

	HOOKLOOP
	{
		if (g_HookList[i].entity != entity || g_HookList[i].type != SDKHook_ReloadPost)
			continue;

		callback = g_HookList[i].callback;
		callback->PushCell(entity);
		callback->PushCell(META_RESULT_OVERRIDE_RET(bool) ? 1 : 0);
		callback->Execute(NULL);
	}

	return true;
}

void SDKHooks::Hook_SetTransmit(CCheckTransmitInfo *pInfo, bool bAlways)
{
	cell_t result = Call(META_IFACEPTR(CBaseEntity), SDKHook_SetTransmit, gamehelpers->IndexOfEdict(pInfo->m_pClientEnt));

	if(result >= Pl_Handled)
		RETURN_META(MRES_SUPERCEDE);

	RETURN_META(MRES_IGNORED);
}

bool SDKHooks::Hook_ShouldCollide(int collisionGroup, int contentsMask)
{
	int entity = gamehelpers->EntityToBCompatRef(META_IFACEPTR(CBaseEntity));
	IPluginFunction *callback = NULL;
	bool origRet = ((META_RESULT_STATUS >= MRES_OVERRIDE)?(META_RESULT_OVERRIDE_RET(bool)):(META_RESULT_ORIG_RET(bool)));
	cell_t res;

	HOOKLOOP
	{
		if (g_HookList[i].entity != entity || g_HookList[i].type != SDKHook_ShouldCollide)
			continue;

		callback = g_HookList[i].callback;
		callback->PushCell(entity);
		callback->PushCell(collisionGroup);
		callback->PushCell(contentsMask);
		callback->PushCell(origRet ? 1 : 0);
		callback->Execute(&res);
	}

	bool ret = false;
	if (res != 0)
	{
		ret = true;
	}

	RETURN_META_VALUE(MRES_SUPERCEDE, ret);
}

void SDKHooks::Hook_Spawn()
{
	int entity = gamehelpers->EntityToBCompatRef(META_IFACEPTR(CBaseEntity));
	IPluginFunction *callback = NULL;
	cell_t res;

	HOOKLOOP
	{
		if (g_HookList[i].entity != entity || g_HookList[i].type != SDKHook_Spawn)
			continue;

		callback = g_HookList[i].callback;
		callback->PushCell(entity);
		callback->Execute(&res);
	}

	if (res >= Pl_Handled)
		RETURN_META(MRES_SUPERCEDE);

	RETURN_META(MRES_IGNORED);
}

void SDKHooks::Hook_SpawnPost()
{
	Call(META_IFACEPTR(CBaseEntity), SDKHook_SpawnPost);
}

void SDKHooks::Hook_StartTouch(CBaseEntity *pOther)
{
	cell_t result = Call(META_IFACEPTR(CBaseEntity), SDKHook_StartTouch, pOther);
	if(result >= Pl_Handled)
		RETURN_META(MRES_SUPERCEDE);

	RETURN_META(MRES_IGNORED);
}

void SDKHooks::Hook_StartTouchPost(CBaseEntity *pOther)
{
	Call(META_IFACEPTR(CBaseEntity), SDKHook_StartTouchPost, pOther);
	RETURN_META(MRES_IGNORED);
}

void SDKHooks::Hook_Think()
{
	Call(META_IFACEPTR(CBaseEntity), SDKHook_Think);
}

void SDKHooks::Hook_ThinkPost()
{
	Call(META_IFACEPTR(CBaseEntity), SDKHook_ThinkPost);
}

void SDKHooks::Hook_Touch(CBaseEntity *pOther)
{
	cell_t result = Call(META_IFACEPTR(CBaseEntity), SDKHook_Touch, pOther);

	if(result >= Pl_Handled)
		RETURN_META(MRES_SUPERCEDE);

	RETURN_META(MRES_IGNORED);
}

void SDKHooks::Hook_TouchPost(CBaseEntity *pOther)
{
	Call(META_IFACEPTR(CBaseEntity), SDKHook_TouchPost, pOther);
	RETURN_META(MRES_IGNORED);
}

#if SOURCE_ENGINE == SE_ORANGEBOXVALVE || SOURCE_ENGINE == SE_CSS
void SDKHooks::Hook_TraceAttack(CTakeDamageInfoHack &info, const Vector &vecDir, trace_t *ptr, void *pUnknownJK)
#else
void SDKHooks::Hook_TraceAttack(CTakeDamageInfoHack &info, const Vector &vecDir, trace_t *ptr)
#endif
{
	int entity = gamehelpers->EntityToBCompatRef(META_IFACEPTR(CBaseEntity));
	int attacker = info.GetAttacker();
	int inflictor = info.GetInflictor();
	float damage = info.GetDamage();
	int damagetype = info.GetDamageType();
	int ammotype = info.GetAmmoType();
	IPluginFunction *callback = NULL;
	cell_t res, ret = Pl_Continue;

	HOOKLOOP
	{
		if(g_HookList[i].entity != entity || g_HookList[i].type != SDKHook_TraceAttack)
			continue;

		callback = g_HookList[i].callback;
		callback->PushCell(entity);
		callback->PushCellByRef(&attacker);
		callback->PushCellByRef(&inflictor);
		callback->PushFloatByRef(&damage);
		callback->PushCellByRef(&damagetype);
		callback->PushCellByRef(&ammotype);
		callback->PushCell(ptr->hitbox);
		callback->PushCell(ptr->hitgroup);
		callback->Execute(&res);

		if(res > ret)
			ret = res;
	}

	if(ret >= Pl_Handled)
		RETURN_META(MRES_SUPERCEDE);

	if(ret == Pl_Changed)
	{
		CBaseEntity *pEntAttacker = gamehelpers->ReferenceToEntity(attacker);
		if(!pEntAttacker)
		{
			callback->GetParentContext()->ThrowNativeError("Entity %d for attacker is invalid", attacker);
			RETURN_META(MRES_IGNORED);
		}
		CBaseEntity *pEntInflictor = gamehelpers->ReferenceToEntity(inflictor);
		if(!pEntInflictor)
		{
			callback->GetParentContext()->ThrowNativeError("Entity %d for inflictor is invalid", inflictor);
			RETURN_META(MRES_IGNORED);
		}
		
		info.SetAttacker(gamehelpers->ReferenceToEntity(attacker));
		info.SetInflictor(gamehelpers->ReferenceToEntity(inflictor));
		info.SetDamage(damage);
		info.SetDamageType(damagetype);
		info.SetAmmoType(ammotype);

		RETURN_META(MRES_HANDLED); 
	}

	RETURN_META(MRES_IGNORED);
}

#if SOURCE_ENGINE == SE_ORANGEBOXVALVE || SOURCE_ENGINE == SE_CSS
void SDKHooks::Hook_TraceAttackPost(CTakeDamageInfoHack &info, const Vector &vecDir, trace_t *ptr, void *pUnknownJK)
#else
void SDKHooks::Hook_TraceAttackPost(CTakeDamageInfoHack &info, const Vector &vecDir, trace_t *ptr)
#endif
{
	int entity = gamehelpers->EntityToBCompatRef(META_IFACEPTR(CBaseEntity));
	IPluginFunction *callback = NULL;

	HOOKLOOP
	{
		if(g_HookList[i].entity != entity || g_HookList[i].type != SDKHook_TraceAttackPost)
			continue;

		callback = g_HookList[i].callback;
		callback->PushCell(entity);
		callback->PushCell(info.GetAttacker());
		callback->PushCell(info.GetInflictor());
		callback->PushFloat(info.GetDamage());
		callback->PushCell(info.GetDamageType());
		callback->PushCell(info.GetAmmoType());
		callback->PushCell(ptr->hitbox);
		callback->PushCell(ptr->hitgroup);
		callback->Execute(NULL);
	}

	RETURN_META(MRES_IGNORED);
}

void SDKHooks::Hook_Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	int entity = gamehelpers->EntityToBCompatRef(META_IFACEPTR(CBaseEntity));
	int activator = gamehelpers->EntityToBCompatRef(pActivator);
	int caller = gamehelpers->EntityToBCompatRef(pCaller);

	cell_t ret;
	IPluginFunction *callback = NULL;

	HOOKLOOP
	{
		if(g_HookList[i].entity != entity || g_HookList[i].type != SDKHook_Use)
			continue;

		callback = g_HookList[i].callback;
		callback->PushCell(entity);
		callback->PushCell(activator);
		callback->PushCell(caller);
		callback->PushCell(useType);
		callback->PushFloat(value);
		callback->Execute(&ret);
	}

	if (ret >= Pl_Handled)
		RETURN_META(MRES_SUPERCEDE);

	RETURN_META(MRES_IGNORED);
}

void SDKHooks::Hook_UsePost(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	int entity = gamehelpers->EntityToBCompatRef(META_IFACEPTR(CBaseEntity));
	int activator = gamehelpers->EntityToBCompatRef(pActivator);
	int caller = gamehelpers->EntityToBCompatRef(pCaller);

	IPluginFunction *callback = NULL;

	HOOKLOOP
	{
		if (g_HookList[i].entity != entity || g_HookList[i].type != SDKHook_UsePost)
			continue;

		callback = g_HookList[i].callback;
		callback->PushCell(entity);
		callback->PushCell(activator);
		callback->PushCell(caller);
		callback->PushCell(useType);
		callback->PushFloat(value);
		callback->Execute(NULL);
	}

	RETURN_META(MRES_IGNORED);
}

void SDKHooks::OnEntityDeleted(CBaseEntity *pEntity)
{
	int entity = gamehelpers->EntityToBCompatRef(pEntity);

	// Call OnEntityDestroyed forward
	g_pOnEntityDestroyed->PushCell(entity);
	g_pOnEntityDestroyed->Execute(NULL);
	
	SDKHooks::RemoveEntityHooks(pEntity);

	m_EntityExists.Set(gamehelpers->ReferenceToIndex(entity), false);
}

void SDKHooks::Hook_VPhysicsUpdate(IPhysicsObject *pPhysics)
{
	Call(META_IFACEPTR(CBaseEntity), SDKHook_VPhysicsUpdate);
}

void SDKHooks::Hook_VPhysicsUpdatePost(IPhysicsObject *pPhysics)
{
	Call(META_IFACEPTR(CBaseEntity), SDKHook_VPhysicsUpdatePost);
}

bool SDKHooks::Hook_WeaponCanSwitchTo(CBaseCombatWeapon *pWeapon)
{
	cell_t result = Call(META_IFACEPTR(CBaseEntity), SDKHook_WeaponCanSwitchTo, pWeapon);

	if(result >= Pl_Handled)
		RETURN_META_VALUE(MRES_SUPERCEDE, false);

	RETURN_META_VALUE(MRES_IGNORED, true);
}

bool SDKHooks::Hook_WeaponCanSwitchToPost(CBaseCombatWeapon *pWeapon)
{
	Call(META_IFACEPTR(CBaseEntity), SDKHook_WeaponCanSwitchToPost, pWeapon);
	RETURN_META_VALUE(MRES_IGNORED, true);
}

bool SDKHooks::Hook_WeaponCanUse(CBaseCombatWeapon *pWeapon)
{
	cell_t result = Call(META_IFACEPTR(CBaseEntity), SDKHook_WeaponCanUse, pWeapon);

	if(result >= Pl_Handled)
		RETURN_META_VALUE(MRES_SUPERCEDE, false);

	RETURN_META_VALUE(MRES_IGNORED, true);
}

bool SDKHooks::Hook_WeaponCanUsePost(CBaseCombatWeapon *pWeapon)
{
	Call(META_IFACEPTR(CBaseEntity), SDKHook_WeaponCanUsePost, pWeapon);
	RETURN_META_VALUE(MRES_IGNORED, true);
}

void SDKHooks::Hook_WeaponDrop(CBaseCombatWeapon *pWeapon, const Vector *pvecTarget, const Vector *pVelocity)
{
	cell_t result = Call(META_IFACEPTR(CBaseEntity), SDKHook_WeaponDrop, pWeapon);

	if(result >= Pl_Handled)
		RETURN_META(MRES_SUPERCEDE);

	RETURN_META(MRES_IGNORED);
}

void SDKHooks::Hook_WeaponDropPost(CBaseCombatWeapon *pWeapon, const Vector *pvecTarget, const Vector *pVelocity)
{
	Call(META_IFACEPTR(CBaseEntity), SDKHook_WeaponDropPost, pWeapon);
	RETURN_META(MRES_IGNORED);
}

void SDKHooks::Hook_WeaponEquip(CBaseCombatWeapon *pWeapon)
{
	cell_t result = Call(META_IFACEPTR(CBaseEntity), SDKHook_WeaponEquip, pWeapon);

	if(result >= Pl_Handled)
		RETURN_META(MRES_SUPERCEDE);

	RETURN_META(MRES_IGNORED);
}

void SDKHooks::Hook_WeaponEquipPost(CBaseCombatWeapon *pWeapon)
{
	Call(META_IFACEPTR(CBaseEntity), SDKHook_WeaponEquipPost, pWeapon);
	RETURN_META(MRES_IGNORED);
}

bool SDKHooks::Hook_WeaponSwitch(CBaseCombatWeapon *pWeapon, int viewmodelindex)
{
	cell_t result = Call(META_IFACEPTR(CBaseEntity), SDKHook_WeaponSwitch, pWeapon);

	if(result >= Pl_Handled)
		RETURN_META_VALUE(MRES_SUPERCEDE, false);

	RETURN_META_VALUE(MRES_IGNORED, true);
}

bool SDKHooks::Hook_WeaponSwitchPost(CBaseCombatWeapon *pWeapon, int viewmodelindex)
{
	cell_t result = Call(META_IFACEPTR(CBaseEntity), SDKHook_WeaponSwitchPost, pWeapon);
	RETURN_META_VALUE(MRES_IGNORED, true);
}


void SDKHooks::RemoveEntityHooks(CBaseEntity *pEnt)
{
	int entity = gamehelpers->EntityToBCompatRef(pEnt);
	
	// Remove hooks
	HOOKLOOP
	{
		if(g_HookList[i].entity == entity)
		{
#ifdef SDKHOOKSDEBUG
			META_CONPRINTF("DEBUG: Removing hook #%d on entity %d (UpdateOnRemove or clientdisconnect)\n", i, entity);
#endif
			Unhook(i);
		}
	}
}

CON_COMMAND(sdkhooks_listhooks, "Lists all current hooks")
{
	META_CONPRINTF("    %-24.23s %-18.17s %s\n", "Plugin", "Type", "Entity");
	
	IPlugin *pPlugin;
	const sm_plugininfo_t *info;
	for(int i = g_HookList.Count() - 1; i >= 0; i--)
	{
		g_HookList[i].callback->GetParentRuntime()->GetDefaultContext()->GetKey(2, (void **)&pPlugin);
		info = pPlugin->GetPublicInfo();
		META_CONPRINTF("%2d. %-24.23s %-18.17s %d\n", i + 1, info->name[0] ? info->name : pPlugin->GetFilename(), g_HookTypes[g_HookList[i].type].name, g_HookList[i].entity);
	}
}
