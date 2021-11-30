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
#include <IBinTools.h>

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
	{"Blocked",               "",                       false},
	{"BlockedPost",           "",                       false},
	{"OnTakeDamageAlive",     "DT_BaseCombatCharacter", false},
	{"OnTakeDamageAlivePost", "DT_BaseCombatCharacter", false},

	// There is no DT for CBaseMultiplayerPlayer. Going up a level
	{"CanBeAutobalanced",     "DT_BasePlayer",          false},
};

SDKHooks g_Interface;
SMEXT_LINK(&g_Interface);

CGlobalVars *gpGlobals;
std::vector<CVTableList *> g_HookList[SDKHook_MAXHOOKS];

IBinTools *g_pBinTools = NULL;
ICvar *icvar = NULL;

#if SOURCE_ENGINE >= SE_ORANGEBOX
IServerTools *servertools = NULL;
#endif

// global hooks and forwards
IForward *g_pOnEntityCreated = NULL;
IForward *g_pOnEntityDestroyed = NULL;

#ifdef GAMEDESC_CAN_CHANGE
int g_hookOnGetGameDescription = 0;
IForward *g_pOnGetGameNameDescription = NULL;
#endif

int g_hookOnLevelInit = 0;
IForward *g_pOnLevelInit = NULL;

IGameConfig *g_pGameConf = NULL;

CUtlVector<IEntityListener *> *EntListeners()
{
	void *gEntList = gamehelpers->GetGlobalEntityList();
	if (gEntList)
	{
		int offset = -1; /* 65572 */
		if (g_pGameConf->GetOffset("EntityListeners", &offset))
		{
			return (CUtlVector<IEntityListener *> *)((intptr_t) gEntList + offset);
		}
	}
	else
	{
		void *entListeners;
		if (g_pGameConf->GetAddress("EntityListenersPtr", &entListeners))
		{
			return (CUtlVector<IEntityListener *> *)entListeners;
		}
	}

	return NULL;
}


/**
 * IServerGameDLL & IVEngineServer Hooks
 */
SH_DECL_HOOK6(IServerGameDLL, LevelInit, SH_NOATTRIB, 0, bool, const char *, const char *, const char *, const char *, bool, bool);
#ifdef GAMEDESC_CAN_CHANGE
SH_DECL_HOOK0(IServerGameDLL, GetGameDescription, SH_NOATTRIB, 0, const char *);
#endif

/**
 * CBaseEntity Hooks
 */
SH_DECL_MANUALHOOK1_void(EndTouch, 0, 0, 0, CBaseEntity *);
SH_DECL_MANUALHOOK1_void(FireBullets, 0, 0, 0, FireBulletsInfo_t const&);
#ifdef GETMAXHEALTH_IS_VIRTUAL
SH_DECL_MANUALHOOK0(GetMaxHealth, 0, 0, 0, int);
#endif
SH_DECL_MANUALHOOK1_void(GroundEntChanged, 0, 0, 0, void *);
SH_DECL_MANUALHOOK1(OnTakeDamage, 0, 0, 0, int, CTakeDamageInfoHack &);
SH_DECL_MANUALHOOK1(OnTakeDamage_Alive, 0, 0, 0, int, CTakeDamageInfoHack &);
SH_DECL_MANUALHOOK0_void(PreThink, 0, 0, 0);
SH_DECL_MANUALHOOK0_void(PostThink, 0, 0, 0);
SH_DECL_MANUALHOOK0(Reload, 0, 0, 0, bool);
SH_DECL_MANUALHOOK2_void(SetTransmit, 0, 0, 0, CCheckTransmitInfo *, bool);
SH_DECL_MANUALHOOK2(ShouldCollide, 0, 0, 0, bool, int, int);
SH_DECL_MANUALHOOK0_void(Spawn, 0, 0, 0);
SH_DECL_MANUALHOOK1_void(StartTouch, 0, 0, 0, CBaseEntity *);
SH_DECL_MANUALHOOK0_void(Think, 0, 0, 0);
SH_DECL_MANUALHOOK1_void(Touch, 0, 0, 0, CBaseEntity *);
#if SOURCE_ENGINE == SE_HL2DM || SOURCE_ENGINE == SE_DODS || SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_TF2 \
	|| SOURCE_ENGINE == SE_BMS || SOURCE_ENGINE == SE_SDK2013
SH_DECL_MANUALHOOK4_void(TraceAttack, 0, 0, 0, CTakeDamageInfoHack &, const Vector &, CGameTrace *, CDmgAccumulator *);
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
SH_DECL_MANUALHOOK1_void(Blocked, 0, 0, 0, CBaseEntity *);
SH_DECL_MANUALHOOK0(CanBeAutobalanced, 0, 0, 0, bool);


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

	buffer[0] = '\0';
	if (!gameconfs->LoadGameConfigFile("sdkhooks.games", &g_pGameConf, buffer, sizeof(buffer)))
	{
		if (buffer[0])
		{
			g_pSM->Format(error, maxlength, "Could not read sdkhooks.games gamedata: %s", buffer);
		}

		return false;
	}

	memset(m_EntityCache, INVALID_EHANDLE_INDEX, sizeof(m_EntityCache));

	CUtlVector<IEntityListener *> *entListeners = EntListeners();
	if (!entListeners)
	{
		g_pSM->Format(error, maxlength, "Failed to setup entity listeners");
		return false;
	}

	entListeners->AddToTail(this);

	sharesys->AddDependency(myself, "bintools.ext", true, true);
	sharesys->AddNatives(myself, g_Natives);
	sharesys->RegisterLibrary(myself, "sdkhooks");
	sharesys->AddInterface(myself, &g_Interface);
	sharesys->AddCapabilityProvider(myself, this, "SDKHook_DmgCustomInOTD");
	sharesys->AddCapabilityProvider(myself, this, "SDKHook_LogicalEntSupport");

	playerhelpers->AddClientListener(&g_Interface);
	
	plsys->AddPluginsListener(&g_Interface);

	g_pOnEntityCreated = forwards->CreateForward("OnEntityCreated", ET_Ignore, 2, NULL, Param_Cell, Param_String);
	g_pOnEntityDestroyed = forwards->CreateForward("OnEntityDestroyed", ET_Ignore, 1, NULL, Param_Cell);
#ifdef GAMEDESC_CAN_CHANGE
	g_pOnGetGameNameDescription = forwards->CreateForward("OnGetGameDescription", ET_Hook, 2, NULL, Param_String);
#endif
	g_pOnLevelInit = forwards->CreateForward("OnLevelInit", ET_Ignore, 2, NULL, Param_String, Param_String);

	SetupHooks();

#if SOURCE_ENGINE >= SE_ORANGEBOX
	int index;
	CBaseHandle hndl;
	for (IHandleEntity *pEnt = (IHandleEntity *)servertools->FirstEntity(); pEnt; pEnt = (IHandleEntity *)servertools->NextEntity((CBaseEntity *)pEnt))
	{
		hndl = pEnt->GetRefEHandle();
		if (!hndl.IsValid())
			continue;
		
		index = hndl.GetEntryIndex();
		if (IsEntityIndexInRange(index))
		{
			m_EntityCache[index] = gamehelpers->IndexToReference(index);
		}
		else
		{
			g_pSM->LogError(myself, "SDKHooks::HandleEntityCreated - Got entity index out of range (%d)", index);
		}
	}
#else
	for (int i = 0; i < NUM_ENT_ENTRIES; i++)
	{
		if (gamehelpers->ReferenceToEntity(i) != NULL)
			m_EntityCache[i] = gamehelpers->IndexToReference(i);
	}
#endif

	return true;
}

inline void HookLevelInit()
{
	assert(g_hookOnLevelInit == 0);
	g_hookOnLevelInit = SH_ADD_HOOK(IServerGameDLL, LevelInit, gamedll, SH_MEMBER(&g_Interface, &SDKHooks::Hook_LevelInit), false);
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

bool SDKHooks::QueryRunning(char* error, size_t maxlength)
{
	SM_CHECK_IFACE(BINTOOLS, g_pBinTools);

	return true;
}

bool SDKHooks::QueryInterfaceDrop(SMInterface* pInterface)
{
	if (pInterface == g_pBinTools)
	{
		return false;
	}

	return IExtensionInterface::QueryInterfaceDrop(pInterface);
}

#define KILL_HOOK_IF_ACTIVE(hook) \
	if (hook != 0) \
	{ \
		SH_REMOVE_HOOK_ID(hook); \
		hook = 0; \
	}

void SDKHooks::SDK_OnUnload()
{
	// Remove left over hooks
	Unhook(reinterpret_cast<SourcePawn::IPluginContext *>(NULL));

	KILL_HOOK_IF_ACTIVE(g_hookOnLevelInit);

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

	CUtlVector<IEntityListener *> *entListeners = EntListeners();
	entListeners->FindAndRemove(this);

	gameconfs->CloseGameConfigFile(g_pGameConf);
}

bool SDKHooks::SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	GET_V_IFACE_CURRENT(GetEngineFactory, icvar, ICvar, CVAR_INTERFACE_VERSION);

#if SOURCE_ENGINE >= SE_ORANGEBOX
	GET_V_IFACE_ANY(GetServerFactory, servertools, IServerTools, VSERVERTOOLS_INTERFACE_VERSION);
	g_pCVar = icvar;
#endif
	CONVAR_REGISTER(this);
	
	gpGlobals = ismm->GetCGlobals();

	return true;
}

const char *SDKHooks::GetExtensionVerString()
{
	return SOURCEMOD_VERSION;
}

const char *SDKHooks::GetExtensionDateString()
{
	return SOURCEMOD_BUILD_TIME;
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
	Unhook(plugin->GetBaseContext());

	if (g_pOnLevelInit->GetFunctionCount() == 0)
	{
		KILL_HOOK_IF_ACTIVE(g_hookOnLevelInit);
	}

#ifdef GAMEDESC_CAN_CHANGE
	if (g_pOnGetGameNameDescription->GetFunctionCount() == 0)
		KILL_HOOK_IF_ACTIVE(g_hookOnGetGameDescription);
#endif
}

void SDKHooks::OnClientPutInServer(int client)
{
	CBaseEntity *pPlayer = gamehelpers->ReferenceToEntity(client);

	HandleEntityCreated(pPlayer, client, gamehelpers->EntityToReference(pPlayer));
}

void SDKHooks::OnClientDisconnecting(int client)
{
	CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(client);
	
	HandleEntityDeleted(pEntity);
}

void SDKHooks::AddEntityListener(ISMEntityListener *listener)
{
	m_EntListeners.push_back(listener);
}

void SDKHooks::RemoveEntityListener(ISMEntityListener *listener)
{
	m_EntListeners.remove(listener);
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

static void PopulateCallbackList(const std::vector<HookList> &source, std::vector<IPluginFunction *> &destination, int entity)
{
	destination.reserve(8);
	for (size_t iter = 0; iter < source.size(); ++iter)
	{
		if (source[iter].entity != entity)
		{
			continue;
		}

		destination.push_back(source[iter].callback);
	}
}

cell_t SDKHooks::Call(int entity, SDKHookType type, int other)
{
	return Call(gamehelpers->ReferenceToEntity(entity), type, gamehelpers->ReferenceToEntity(other));
}

cell_t SDKHooks::Call(CBaseEntity *pEnt, SDKHookType type, int other)
{
	return Call(pEnt, type, gamehelpers->ReferenceToEntity(other));
}

cell_t SDKHooks::Call(CBaseEntity *pEnt, SDKHookType type, CBaseEntity *pOther)
{
	cell_t ret = Pl_Continue;

	CVTableHook vhook(pEnt);
	std::vector<CVTableList *> &vtablehooklist = g_HookList[type];
	for (size_t entry = 0; entry < vtablehooklist.size(); ++entry)
	{
		if (vhook != vtablehooklist[entry]->vtablehook)
		{
			continue;
		}

		int entity = gamehelpers->EntityToBCompatRef(pEnt);
		int other = gamehelpers->EntityToBCompatRef(pOther);

		std::vector<IPluginFunction *> callbackList;
		PopulateCallbackList(vtablehooklist[entry]->hooks, callbackList, entity);
		for (entry = 0; entry < callbackList.size(); ++entry)
		{
			IPluginFunction *callback = callbackList[entry];
			callback->PushCell(entity);
			callback->PushCell(other);

			cell_t res;
			callback->Execute(&res);
			if(res > ret)
			{
				ret = res;
			}
		}

		break;
	}

	return ret;
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
	CHECKOFFSET(OnTakeDamage_Alive,true, true);
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
	CHECKOFFSET(Blocked,          true,  true);
	CHECKOFFSET(CanBeAutobalanced, true, false);

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

	CBaseEntity *pEnt = gamehelpers->ReferenceToEntity(entity);
	if(!pEnt)
		return HookRet_InvalidEntity;
	if(type < 0 || type >= SDKHook_MAXHOOKS)
		return HookRet_InvalidHookType;

	if (!!strcmp(g_HookTypes[type].dtReq, ""))
	{
		IServerUnknown *pUnk = (IServerUnknown *)pEnt;

		IServerNetworkable *pNet = pUnk->GetNetworkable();
		if (pNet && !UTIL_ContainsDataTable(pNet->GetServerClass()->m_pTable, g_HookTypes[type].dtReq))
			return HookRet_BadEntForHookType;
	}

	size_t entry;
	CVTableHook vhook(pEnt);
	std::vector<CVTableList *> &vtablehooklist = g_HookList[type];
	for (entry = 0; entry < vtablehooklist.size(); ++entry)
	{
		if (vhook == vtablehooklist[entry]->vtablehook)
		{
			break;
		}
	}

	if (entry == vtablehooklist.size())
	{
		int hookid = 0;
		switch(type)
		{
			case SDKHook_EndTouch:
				hookid = SH_ADD_MANUALVPHOOK(EndTouch, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_EndTouch), false);
				break;
			case SDKHook_EndTouchPost:
				hookid = SH_ADD_MANUALVPHOOK(EndTouch, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_EndTouchPost), true);
				break;
			case SDKHook_FireBulletsPost:
				hookid = SH_ADD_MANUALVPHOOK(FireBullets, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_FireBulletsPost), true);
				break;
#ifdef GETMAXHEALTH_IS_VIRTUAL
			case SDKHook_GetMaxHealth:
				hookid = SH_ADD_MANUALVPHOOK(GetMaxHealth, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_GetMaxHealth), false);
				break;
#endif
			case SDKHook_GroundEntChangedPost:
				hookid = SH_ADD_MANUALVPHOOK(GroundEntChanged, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_GroundEntChangedPost), true);
				break;
			case SDKHook_OnTakeDamage:
				hookid = SH_ADD_MANUALVPHOOK(OnTakeDamage, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_OnTakeDamage), false);
				break;
			case SDKHook_OnTakeDamagePost:
				hookid = SH_ADD_MANUALVPHOOK(OnTakeDamage, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_OnTakeDamagePost), true);
				break;
			case SDKHook_OnTakeDamage_Alive:
				hookid = SH_ADD_MANUALVPHOOK(OnTakeDamage_Alive, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_OnTakeDamage_Alive), false);
				break;
			case SDKHook_OnTakeDamage_AlivePost:
				hookid = SH_ADD_MANUALVPHOOK(OnTakeDamage_Alive, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_OnTakeDamage_AlivePost), true);
				break;
			case SDKHook_PreThink:
				hookid = SH_ADD_MANUALVPHOOK(PreThink, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_PreThink), false);
				break;
			case SDKHook_PreThinkPost:
				hookid = SH_ADD_MANUALVPHOOK(PreThink, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_PreThinkPost), true);
				break;
			case SDKHook_PostThink:
				hookid = SH_ADD_MANUALVPHOOK(PostThink, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_PostThink), false);
				break;
			case SDKHook_PostThinkPost:
				hookid = SH_ADD_MANUALVPHOOK(PostThink, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_PostThinkPost), true);
				break;
			case SDKHook_Reload:
				hookid = SH_ADD_MANUALVPHOOK(Reload, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_Reload), false);
				break;
			case SDKHook_ReloadPost:
				hookid = SH_ADD_MANUALVPHOOK(Reload, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_ReloadPost), true);
				break;
			case SDKHook_SetTransmit:
				hookid = SH_ADD_MANUALVPHOOK(SetTransmit, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_SetTransmit), false);
				break;
			case SDKHook_Spawn:
				hookid = SH_ADD_MANUALVPHOOK(Spawn, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_Spawn), false);
				break;
			case SDKHook_SpawnPost:
				hookid = SH_ADD_MANUALVPHOOK(Spawn, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_SpawnPost), true);
				break;
			case SDKHook_StartTouch:
				hookid = SH_ADD_MANUALVPHOOK(StartTouch, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_StartTouch), false);
				break;
			case SDKHook_StartTouchPost:
				hookid = SH_ADD_MANUALVPHOOK(StartTouch, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_StartTouchPost), true);
				break;
			case SDKHook_Think:
				hookid = SH_ADD_MANUALVPHOOK(Think, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_Think), false);
				break;
			case SDKHook_ThinkPost:
				hookid = SH_ADD_MANUALVPHOOK(Think, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_ThinkPost), true);
				break;
			case SDKHook_Touch:
				hookid = SH_ADD_MANUALVPHOOK(Touch, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_Touch), false);
				break;
			case SDKHook_TouchPost:
				hookid = SH_ADD_MANUALVPHOOK(Touch, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_TouchPost), true);
				break;
			case SDKHook_TraceAttack:
				hookid = SH_ADD_MANUALVPHOOK(TraceAttack, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_TraceAttack), false);
				break;
			case SDKHook_TraceAttackPost:
				hookid = SH_ADD_MANUALVPHOOK(TraceAttack, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_TraceAttackPost), true);
				break;
			case SDKHook_Use:
				hookid = SH_ADD_MANUALVPHOOK(Use, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_Use), false);
				break;
			case SDKHook_UsePost:
				hookid = SH_ADD_MANUALVPHOOK(Use, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_UsePost), true);
				break;
			case SDKHook_VPhysicsUpdate:
				hookid = SH_ADD_MANUALVPHOOK(VPhysicsUpdate, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_VPhysicsUpdate), false);
				break;
			case SDKHook_VPhysicsUpdatePost:
				hookid = SH_ADD_MANUALVPHOOK(VPhysicsUpdate, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_VPhysicsUpdatePost), true);
				break;
			case SDKHook_WeaponCanSwitchTo:
				hookid = SH_ADD_MANUALVPHOOK(Weapon_CanSwitchTo, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_WeaponCanSwitchTo), false);
				break;
			case SDKHook_WeaponCanSwitchToPost:
				hookid = SH_ADD_MANUALVPHOOK(Weapon_CanSwitchTo, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_WeaponCanSwitchToPost), true);
				break;
			case SDKHook_WeaponCanUse:
				hookid = SH_ADD_MANUALVPHOOK(Weapon_CanUse, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_WeaponCanUse), false);
				break;
			case SDKHook_WeaponCanUsePost:
				hookid = SH_ADD_MANUALVPHOOK(Weapon_CanUse, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_WeaponCanUsePost), true);
				break;
			case SDKHook_WeaponDrop:
				hookid = SH_ADD_MANUALVPHOOK(Weapon_Drop, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_WeaponDrop), false);
				break;
			case SDKHook_WeaponDropPost:
				hookid = SH_ADD_MANUALVPHOOK(Weapon_Drop, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_WeaponDropPost), true);
				break;
			case SDKHook_WeaponEquip:
				hookid = SH_ADD_MANUALVPHOOK(Weapon_Equip, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_WeaponEquip), false);
				break;
			case SDKHook_WeaponEquipPost:
				hookid = SH_ADD_MANUALVPHOOK(Weapon_Equip, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_WeaponEquipPost), true);
				break;
			case SDKHook_WeaponSwitch:
				hookid = SH_ADD_MANUALVPHOOK(Weapon_Switch, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_WeaponSwitch), false);
				break;
			case SDKHook_WeaponSwitchPost:
				hookid = SH_ADD_MANUALVPHOOK(Weapon_Switch, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_WeaponSwitchPost), true);
				break;
			case SDKHook_ShouldCollide:
				hookid = SH_ADD_MANUALVPHOOK(ShouldCollide, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_ShouldCollide), true);
				break;
			case SDKHook_Blocked:
				hookid = SH_ADD_MANUALVPHOOK(Blocked, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_Blocked), false);
				break;
			case SDKHook_BlockedPost:
				hookid = SH_ADD_MANUALVPHOOK(Blocked, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_BlockedPost), true);
				break;
			case SDKHook_CanBeAutobalanced:
				hookid = SH_ADD_MANUALVPHOOK(CanBeAutobalanced, pEnt, SH_MEMBER(&g_Interface, &SDKHooks::Hook_CanBeAutobalanced), false);
				break;
		}

		vhook.SetHookID(hookid);

		CVTableList *vtablelist = new CVTableList;
		vtablelist->vtablehook = new CVTableHook(vhook);
		vtablehooklist.push_back(vtablelist);
	}
	
	// Add hook to hook list
	HookList hook;
	hook.entity = gamehelpers->EntityToBCompatRef(pEnt);
	hook.callback = callback;
	vtablehooklist[entry]->hooks.push_back(hook);

	return HookRet_Successful;
}

void SDKHooks::Unhook(CBaseEntity *pEntity)
{
	if (pEntity == NULL)
	{
		return;
	}

	int entity = gamehelpers->EntityToBCompatRef(pEntity);
	for (size_t type = 0; type < SDKHook_MAXHOOKS; ++type)
	{
		std::vector<CVTableList *> &vtablehooklist = g_HookList[type];
		for (size_t listentry = 0; listentry < vtablehooklist.size(); ++listentry)
		{
			std::vector<HookList> &pawnhooks = vtablehooklist[listentry]->hooks;
			for (size_t entry = 0; entry < pawnhooks.size(); ++entry)
			{
				if (entity != pawnhooks[entry].entity)
				{
					continue;
				}

				pawnhooks.erase(pawnhooks.begin() + entry);
				entry--;
			}

			if (pawnhooks.size() == 0)
			{
				delete vtablehooklist[listentry];
				vtablehooklist.erase(vtablehooklist.begin() + listentry);
				listentry--;
			}
		}
	}
}

void SDKHooks::Unhook(IPluginContext *pContext)
{
	for (size_t type = 0; type < SDKHook_MAXHOOKS; ++type)
	{
		std::vector<CVTableList *> &vtablehooklist = g_HookList[type];
		for (size_t listentry = 0; listentry < vtablehooklist.size(); ++listentry)
		{
			std::vector<HookList> &pawnhooks = vtablehooklist[listentry]->hooks;
			for (size_t entry = 0; entry < pawnhooks.size(); ++entry)
			{
				if (pContext != NULL && pContext != pawnhooks[entry].callback->GetParentRuntime()->GetDefaultContext())
				{
					continue;
				}

				pawnhooks.erase(pawnhooks.begin() + entry);
				entry--;
			}

			if (pawnhooks.size() == 0)
			{
				delete vtablehooklist[listentry];
				vtablehooklist.erase(vtablehooklist.begin() + listentry);
				listentry--;
			}
		}
	}
}

void SDKHooks::Unhook(int entity, SDKHookType type, IPluginFunction *pCallback)
{
	CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(entity);
	if (pEntity == NULL)
	{
		return;
	}

	CVTableHook vhook(pEntity);
	std::vector<CVTableList *> &vtablehooklist = g_HookList[type];
	for (size_t listentry = 0; listentry < vtablehooklist.size(); ++listentry)
	{
		if (vhook != vtablehooklist[listentry]->vtablehook)
		{
			continue;
		}

		entity = gamehelpers->EntityToBCompatRef(pEntity);

		std::vector<HookList> &pawnhooks = vtablehooklist[listentry]->hooks;
		for (size_t entry = 0; entry < pawnhooks.size(); ++entry)
		{
			HookList &hookentry = pawnhooks[entry];
			if (entity != hookentry.entity || pCallback != hookentry.callback)
			{
				continue;
			}

			pawnhooks.erase(pawnhooks.begin() + entry);
			entry--;
		}

		if (pawnhooks.size() == 0)
		{
			delete vtablehooklist[listentry];
			vtablehooklist.erase(vtablehooklist.begin() + listentry);
			listentry--;
		}

		break;
	}
}

/**
 * IEntityFactoryDictionary, IServerGameDLL & IVEngineServer Hook Handlers
 */
void SDKHooks::OnEntityCreated(CBaseEntity *pEntity)
{
	// Call OnEntityCreated forward
	int ref = gamehelpers->EntityToReference(pEntity);
	int index = gamehelpers->ReferenceToIndex(ref);

	// This can be -1 for player ents before any players have connected
	if ((unsigned)index == INVALID_EHANDLE_INDEX || (index > 0 && index <= playerhelpers->GetMaxClients()))
	{
		return;
	}

	if (!IsEntityIndexInRange(index))
	{
		g_pSM->LogError(myself, "SDKHooks::OnEntityCreated - Got entity index out of range (%d)", index);
		return;
	}

	// The entity could already exist. The creation notifier fires twice for some paths
	if (m_EntityCache[index] != ref)
	{
		HandleEntityCreated(pEntity, index, ref);
	}
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

bool SDKHooks::Hook_LevelInit(char const *pMapName, char const *pMapEntities, char const *pOldLevel, char const *pLandmarkName, bool loadGame, bool background)
{
	// Call OnLevelInit forward
	g_pOnLevelInit->PushString(pMapName);
	g_pOnLevelInit->PushString("");
	g_pOnLevelInit->Execute();

	RETURN_META_VALUE(MRES_IGNORED, true);
}


/**
 * CBaseEntity Hook Handlers
 */
bool SDKHooks::Hook_CanBeAutobalanced()
{
	CBaseEntity *pPlayer = META_IFACEPTR(CBaseEntity);

	CVTableHook vhook(pPlayer);
	std::vector<CVTableList *> &vtablehooklist = g_HookList[SDKHook_CanBeAutobalanced];
	for (size_t entry = 0; entry < vtablehooklist.size(); ++entry)
	{
		if (vhook != vtablehooklist[entry]->vtablehook)
		{
			continue;
		}

		int entity = gamehelpers->EntityToBCompatRef(pPlayer);

		bool origRet = SH_MCALL(pPlayer, CanBeAutobalanced)();
		bool newRet = origRet;

		std::vector<IPluginFunction *> callbackList;
		PopulateCallbackList(vtablehooklist[entry]->hooks, callbackList, entity);
		for (entry = 0; entry < callbackList.size(); ++entry)
		{
			cell_t res = origRet;
			IPluginFunction *callback = callbackList[entry];
			callback->PushCell(entity);
			callback->PushCell(origRet);
			callback->Execute(&res);

			// Only update our new ret if different from original
			// (so if multiple plugins returning different answers,
			//  the one(s) that changed it win)
			if ((bool)res != origRet)
				newRet = !origRet;
		}

		if (newRet != origRet)
			RETURN_META_VALUE(MRES_SUPERCEDE, newRet);

		break;
	}

	RETURN_META_VALUE(MRES_IGNORED, false);
}

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
	CBaseEntity *pEntity = META_IFACEPTR(CBaseEntity);
	int entity = gamehelpers->EntityToBCompatRef(pEntity);

	IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(entity);
	if(!pPlayer)
		RETURN_META(MRES_IGNORED);

	IPlayerInfo *pInfo = pPlayer->GetPlayerInfo();
	if(!pInfo)
		RETURN_META(MRES_IGNORED);

	CVTableHook vhook(pEntity);
	std::vector<CVTableList *> &vtablehooklist = g_HookList[SDKHook_FireBulletsPost];
	for (size_t entry = 0; entry < vtablehooklist.size(); ++entry)
	{
		if (vhook != vtablehooklist[entry]->vtablehook)
		{
			continue;
		}

		const char *weapon = pInfo->GetWeaponName();

		std::vector<IPluginFunction *> callbackList;
		PopulateCallbackList(vtablehooklist[entry]->hooks, callbackList, entity);
		for (entry = 0; entry < callbackList.size(); ++entry)
		{
			IPluginFunction *callback = callbackList[entry];
			callback->PushCell(entity);
			callback->PushCell(info.m_iShots);
			callback->PushString(weapon?weapon:"");
			callback->Execute(NULL);
		}

		break;
	}

	RETURN_META(MRES_IGNORED);
}

#ifdef GETMAXHEALTH_IS_VIRTUAL
int SDKHooks::Hook_GetMaxHealth()
{
	CBaseEntity *pEntity = META_IFACEPTR(CBaseEntity);
	int original_max = SH_MCALL(pEntity, GetMaxHealth)();

	CVTableHook vhook(pEntity);
	std::vector<CVTableList *> &vtablehooklist = g_HookList[SDKHook_GetMaxHealth];
	for (size_t entry = 0; entry < vtablehooklist.size(); ++entry)
	{
		if (vhook != vtablehooklist[entry]->vtablehook)
		{
			continue;
		}

		int entity = gamehelpers->EntityToBCompatRef(pEntity);

		int new_max = original_max;

		cell_t res = Pl_Continue;

		std::vector<IPluginFunction *> callbackList;
		PopulateCallbackList(vtablehooklist[entry]->hooks, callbackList, entity);
		for (entry = 0; entry < callbackList.size(); ++entry)
		{
			IPluginFunction *callback = callbackList[entry];
			callback->PushCell(entity);
			callback->PushCellByRef(&new_max);
			callback->Execute(&res);
		}

		if (res >= Pl_Changed)
			RETURN_META_VALUE(MRES_SUPERCEDE, new_max);

		break;
	}

	RETURN_META_VALUE(MRES_IGNORED, original_max);
}
#endif

void SDKHooks::Hook_GroundEntChangedPost(void *pVar)
{
	Call(META_IFACEPTR(CBaseEntity), SDKHook_GroundEntChangedPost);
}

int SDKHooks::HandleOnTakeDamageHook(CTakeDamageInfoHack &info, SDKHookType hookType)
{
	CBaseEntity *pEntity = META_IFACEPTR(CBaseEntity);

	CVTableHook vhook(pEntity);
	std::vector<CVTableList *> &vtablehooklist = g_HookList[hookType];
	for (size_t entry = 0; entry < vtablehooklist.size(); ++entry)
	{
		if (vhook != vtablehooklist[entry]->vtablehook)
		{
			continue;
		}

		int entity = gamehelpers->EntityToBCompatRef(pEntity);
		int attacker = info.GetAttacker();
		int inflictor = info.GetInflictor();
		float damage = info.GetDamage();
		int damagetype = info.GetDamageType();
		int weapon = info.GetWeapon();

		Vector force = info.GetDamageForce();
		cell_t damageForce[3] = { sp_ftoc(force.x), sp_ftoc(force.y), sp_ftoc(force.z) };

		Vector pos = info.GetDamagePosition();
		cell_t damagePosition[3] = { sp_ftoc(pos.x), sp_ftoc(pos.y), sp_ftoc(pos.z) };

		cell_t res, ret = Pl_Continue;

		std::vector<IPluginFunction *> callbackList;
		PopulateCallbackList(vtablehooklist[entry]->hooks, callbackList, entity);
		for (entry = 0; entry < callbackList.size(); ++entry)
		{
			IPluginFunction *callback = callbackList[entry];
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

			if (res >= ret)
			{
				ret = res;
				if (ret == Pl_Changed)
				{
					CBaseEntity *pEntAttacker = gamehelpers->ReferenceToEntity(attacker);
					if (!pEntAttacker && attacker != -1)
					{
						callback->GetParentContext()->BlamePluginError(callback, "Callback-provided entity %d for attacker is invalid", attacker);
						RETURN_META_VALUE(MRES_IGNORED, 0);
					}
					CBaseEntity *pEntInflictor = gamehelpers->ReferenceToEntity(inflictor);
					if (!pEntInflictor && inflictor != -1)
					{
						callback->GetParentContext()->BlamePluginError(callback, "Callback-provided entity %d for inflictor is invalid", inflictor);
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
				}
			}
		}

		if (ret >= Pl_Handled)
			RETURN_META_VALUE(MRES_SUPERCEDE, 1);

		if (ret == Pl_Changed)
			RETURN_META_VALUE(MRES_HANDLED, 1);

		break;
	}

	RETURN_META_VALUE(MRES_IGNORED, 0);
}

int SDKHooks::HandleOnTakeDamageHookPost(CTakeDamageInfoHack &info, SDKHookType hookType)
{
	CBaseEntity *pEntity = META_IFACEPTR(CBaseEntity);

	CVTableHook vhook(pEntity);
	std::vector<CVTableList *> &vtablehooklist = g_HookList[hookType];
	for (size_t entry = 0; entry < vtablehooklist.size(); ++entry)
	{
		if (vhook != vtablehooklist[entry]->vtablehook)
		{
			continue;
		}

		int entity = gamehelpers->EntityToBCompatRef(pEntity);

		std::vector<IPluginFunction *> callbackList;
		PopulateCallbackList(vtablehooklist[entry]->hooks, callbackList, entity);
		for (entry = 0; entry < callbackList.size(); ++entry)
		{
			IPluginFunction *callback = callbackList[entry];
			callback->PushCell(entity);
			callback->PushCell(info.GetAttacker());
			callback->PushCell(info.GetInflictor());
			callback->PushFloat(info.GetDamage());
			callback->PushCell(info.GetDamageType());
			callback->PushCell(info.GetWeapon());

			Vector force = info.GetDamageForce();
			cell_t damageForce[3] = { sp_ftoc(force.x), sp_ftoc(force.y), sp_ftoc(force.z) };
			callback->PushArray(damageForce, 3);

			Vector pos = info.GetDamagePosition();
			cell_t damagePosition[3] = { sp_ftoc(pos.x), sp_ftoc(pos.y), sp_ftoc(pos.z) };
			callback->PushArray(damagePosition, 3);

			callback->PushCell(info.GetDamageCustom());

			callback->Execute(NULL);
		}

		break;
	}

	RETURN_META_VALUE(MRES_IGNORED, 0);
}

int SDKHooks::Hook_OnTakeDamage(CTakeDamageInfoHack &info)
{
	return HandleOnTakeDamageHook(info, SDKHook_OnTakeDamage);
}

int SDKHooks::Hook_OnTakeDamagePost(CTakeDamageInfoHack &info)
{
	return HandleOnTakeDamageHookPost(info, SDKHook_OnTakeDamagePost);
}

int SDKHooks::Hook_OnTakeDamage_Alive(CTakeDamageInfoHack &info)
{
	return HandleOnTakeDamageHook(info, SDKHook_OnTakeDamage_Alive);
}

int SDKHooks::Hook_OnTakeDamage_AlivePost(CTakeDamageInfoHack &info)
{
	return HandleOnTakeDamageHookPost(info, SDKHook_OnTakeDamage_AlivePost);
}

void SDKHooks::Hook_PreThink()
{
	Call(META_IFACEPTR(CBaseEntity), SDKHook_PreThink);
	RETURN_META(MRES_IGNORED);
}

void SDKHooks::Hook_PreThinkPost()
{
	Call(META_IFACEPTR(CBaseEntity), SDKHook_PreThinkPost);
}

void SDKHooks::Hook_PostThink()
{
	Call(META_IFACEPTR(CBaseEntity), SDKHook_PostThink);
	RETURN_META(MRES_IGNORED);
}

void SDKHooks::Hook_PostThinkPost()
{
	Call(META_IFACEPTR(CBaseEntity), SDKHook_PostThinkPost);
}

bool SDKHooks::Hook_Reload()
{
	CBaseEntity *pEntity = META_IFACEPTR(CBaseEntity);

	CVTableHook vhook(pEntity);
	std::vector<CVTableList *> &vtablehooklist = g_HookList[SDKHook_Reload];
	for (size_t entry = 0; entry < vtablehooklist.size(); ++entry)
	{
		if (vhook != vtablehooklist[entry]->vtablehook)
		{
			continue;
		}

		int entity = gamehelpers->EntityToBCompatRef(pEntity);
		cell_t res = Pl_Continue;

		std::vector<IPluginFunction *> callbackList;
		PopulateCallbackList(vtablehooklist[entry]->hooks, callbackList, entity);
		for (entry = 0; entry < callbackList.size(); ++entry)
		{
			IPluginFunction *callback = callbackList[entry];
			callback->PushCell(entity);
			callback->Execute(&res);
		}

		if (res >= Pl_Handled)
			RETURN_META_VALUE(MRES_SUPERCEDE, false);

		break;
	}

	RETURN_META_VALUE(MRES_IGNORED, true);
}

bool SDKHooks::Hook_ReloadPost()
{
	CBaseEntity *pEntity = META_IFACEPTR(CBaseEntity);

	CVTableHook vhook(pEntity);
	std::vector<CVTableList *> &vtablehooklist = g_HookList[SDKHook_ReloadPost];
	for (size_t entry = 0; entry < vtablehooklist.size(); ++entry)
	{
		if (vhook != vtablehooklist[entry]->vtablehook)
		{
			continue;
		}

		int entity = gamehelpers->EntityToBCompatRef(pEntity);
		cell_t origreturn = META_RESULT_ORIG_RET(bool) ? 1 : 0;

		std::vector<IPluginFunction *> callbackList;
		PopulateCallbackList(vtablehooklist[entry]->hooks, callbackList, entity);
		for (entry = 0; entry < callbackList.size(); ++entry)
		{
			IPluginFunction *callback = callbackList[entry];
			callback->PushCell(entity);
			callback->PushCell(origreturn);
			callback->Execute(NULL);
		}

		break;
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
	CBaseEntity *pEntity = META_IFACEPTR(CBaseEntity);

	CVTableHook vhook(pEntity);
	std::vector<CVTableList *> &vtablehooklist = g_HookList[SDKHook_ShouldCollide];
	for (size_t entry = 0; entry < vtablehooklist.size(); ++entry)
	{
		if (vhook != vtablehooklist[entry]->vtablehook)
		{
			continue;
		}

		int entity = gamehelpers->EntityToBCompatRef(pEntity);
		cell_t origRet = ((META_RESULT_STATUS >= MRES_OVERRIDE)?(META_RESULT_OVERRIDE_RET(bool)):(META_RESULT_ORIG_RET(bool))) ? 1 : 0;
		cell_t res = 0;

		std::vector<IPluginFunction *> callbackList;
		PopulateCallbackList(vtablehooklist[entry]->hooks, callbackList, entity);
		for (entry = 0; entry < callbackList.size(); ++entry)
		{
			IPluginFunction *callback = callbackList[entry];
			callback->PushCell(entity);
			callback->PushCell(collisionGroup);
			callback->PushCell(contentsMask);
			callback->PushCell(origRet);
			callback->Execute(&res);
		}

		bool ret = false;
		if (res != 0)
		{
			ret = true;
		}

		RETURN_META_VALUE(MRES_SUPERCEDE, ret);
	}

	RETURN_META_VALUE(MRES_IGNORED, true);
}

void SDKHooks::Hook_Spawn()
{
	CBaseEntity *pEntity = META_IFACEPTR(CBaseEntity);

	CVTableHook vhook(pEntity);
	std::vector<CVTableList *> &vtablehooklist = g_HookList[SDKHook_Spawn];
	for (size_t entry = 0; entry < vtablehooklist.size(); ++entry)
	{
		if (vhook != vtablehooklist[entry]->vtablehook)
		{
			continue;
		}

		int entity = gamehelpers->EntityToBCompatRef(pEntity);
		cell_t res = Pl_Continue;

		std::vector<IPluginFunction *> callbackList;
		PopulateCallbackList(vtablehooklist[entry]->hooks, callbackList, entity);
		for (entry = 0; entry < callbackList.size(); ++entry)
		{
			IPluginFunction *callback = callbackList[entry];
			callback->PushCell(entity);
			callback->Execute(&res);
		}

		if (res >= Pl_Handled)
			RETURN_META(MRES_SUPERCEDE);

		break;
	}

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
	cell_t result = Call(META_IFACEPTR(CBaseEntity), SDKHook_Think);

	if(result >= Pl_Handled)
		RETURN_META(MRES_SUPERCEDE);

	RETURN_META(MRES_IGNORED);
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

#if SOURCE_ENGINE == SE_HL2DM || SOURCE_ENGINE == SE_DODS || SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_TF2 \
	|| SOURCE_ENGINE == SE_BMS || SOURCE_ENGINE == SE_SDK2013
void SDKHooks::Hook_TraceAttack(CTakeDamageInfoHack &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator)
#else
void SDKHooks::Hook_TraceAttack(CTakeDamageInfoHack &info, const Vector &vecDir, trace_t *ptr)
#endif
{
	CBaseEntity *pEntity = META_IFACEPTR(CBaseEntity);

	CVTableHook vhook(pEntity);
	std::vector<CVTableList *> &vtablehooklist = g_HookList[SDKHook_TraceAttack];
	for (size_t entry = 0; entry < vtablehooklist.size(); ++entry)
	{
		if (vhook != vtablehooklist[entry]->vtablehook)
		{
			continue;
		}

		int entity = gamehelpers->EntityToBCompatRef(pEntity);
		int attacker = info.GetAttacker();
		int inflictor = info.GetInflictor();
		float damage = info.GetDamage();
		int damagetype = info.GetDamageType();
		int ammotype = info.GetAmmoType();
		cell_t res, ret = Pl_Continue;

		std::vector<IPluginFunction *> callbackList;
		PopulateCallbackList(vtablehooklist[entry]->hooks, callbackList, entity);
		for (entry = 0; entry < callbackList.size(); ++entry)
		{
			IPluginFunction *callback = callbackList[entry];
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
			{
				ret = res;

				if(ret == Pl_Changed)
				{
					CBaseEntity *pEntAttacker = gamehelpers->ReferenceToEntity(attacker);
					if(!pEntAttacker)
					{
						callback->GetParentContext()->BlamePluginError(callback, "Callback-provided entity %d for attacker is invalid", attacker);
						RETURN_META(MRES_IGNORED);
					}
					CBaseEntity *pEntInflictor = gamehelpers->ReferenceToEntity(inflictor);
					if(!pEntInflictor)
					{
						callback->GetParentContext()->BlamePluginError(callback, "Callback-provided entity %d for inflictor is invalid", inflictor);
						RETURN_META(MRES_IGNORED);
					}
					
					info.SetAttacker(pEntAttacker);
					info.SetInflictor(pEntInflictor);
					info.SetDamage(damage);
					info.SetDamageType(damagetype);
					info.SetAmmoType(ammotype);
				}
			}
		}

		if(ret >= Pl_Handled)
			RETURN_META(MRES_SUPERCEDE);

		if(ret == Pl_Changed)
			RETURN_META(MRES_HANDLED);

		break;
	}

	RETURN_META(MRES_IGNORED);
}

#if SOURCE_ENGINE == SE_HL2DM || SOURCE_ENGINE == SE_DODS || SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_TF2 \
	|| SOURCE_ENGINE == SE_BMS || SOURCE_ENGINE == SE_SDK2013
void SDKHooks::Hook_TraceAttackPost(CTakeDamageInfoHack &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator)
#else
void SDKHooks::Hook_TraceAttackPost(CTakeDamageInfoHack &info, const Vector &vecDir, trace_t *ptr)
#endif
{
	CBaseEntity *pEntity = META_IFACEPTR(CBaseEntity);

	CVTableHook vhook(pEntity);
	std::vector<CVTableList *> &vtablehooklist = g_HookList[SDKHook_TraceAttackPost];
	for (size_t entry = 0; entry < vtablehooklist.size(); ++entry)
	{
		if (vhook != vtablehooklist[entry]->vtablehook)
		{
			continue;
		}

		int entity = gamehelpers->EntityToBCompatRef(pEntity);

		std::vector<IPluginFunction *> callbackList;
		PopulateCallbackList(vtablehooklist[entry]->hooks, callbackList, entity);
		for (entry = 0; entry < callbackList.size(); ++entry)
		{
			IPluginFunction *callback = callbackList[entry];
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

		break;
	}

	RETURN_META(MRES_IGNORED);
}

void SDKHooks::Hook_Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	CBaseEntity *pEntity = META_IFACEPTR(CBaseEntity);

	CVTableHook vhook(pEntity);
	std::vector<CVTableList *> &vtablehooklist = g_HookList[SDKHook_Use];
	for (size_t entry = 0; entry < vtablehooklist.size(); ++entry)
	{
		if (vhook != vtablehooklist[entry]->vtablehook)
		{
			continue;
		}

		int entity = gamehelpers->EntityToBCompatRef(pEntity);
		int activator = gamehelpers->EntityToBCompatRef(pActivator);
		int caller = gamehelpers->EntityToBCompatRef(pCaller);
		cell_t ret = Pl_Continue;

		std::vector<IPluginFunction *> callbackList;
		PopulateCallbackList(vtablehooklist[entry]->hooks, callbackList, entity);
		for (entry = 0; entry < callbackList.size(); ++entry)
		{
			IPluginFunction *callback = callbackList[entry];
			callback->PushCell(entity);
			callback->PushCell(activator);
			callback->PushCell(caller);
			callback->PushCell(useType);
			callback->PushFloat(value);
			callback->Execute(&ret);
		}

		if (ret >= Pl_Handled)
			RETURN_META(MRES_SUPERCEDE);

		break;
	}

	RETURN_META(MRES_IGNORED);
}

void SDKHooks::Hook_UsePost(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	CBaseEntity *pEntity = META_IFACEPTR(CBaseEntity);

	CVTableHook vhook(pEntity);
	std::vector<CVTableList *> &vtablehooklist = g_HookList[SDKHook_UsePost];
	for (size_t entry = 0; entry < vtablehooklist.size(); ++entry)
	{
		if (vhook != vtablehooklist[entry]->vtablehook)
		{
			continue;
		}

		int entity = gamehelpers->EntityToBCompatRef(pEntity);
		int activator = gamehelpers->EntityToBCompatRef(pActivator);
		int caller = gamehelpers->EntityToBCompatRef(pCaller);

		std::vector<IPluginFunction *> callbackList;
		PopulateCallbackList(vtablehooklist[entry]->hooks, callbackList, entity);
		for (entry = 0; entry < callbackList.size(); ++entry)
		{
			IPluginFunction *callback = callbackList[entry];
			callback->PushCell(entity);
			callback->PushCell(activator);
			callback->PushCell(caller);
			callback->PushCell(useType);
			callback->PushFloat(value);
			callback->Execute(NULL);
		}

		break;
	}

	RETURN_META(MRES_IGNORED);
}

void SDKHooks::OnEntityDeleted(CBaseEntity *pEntity)
{
	int index = gamehelpers->ReferenceToIndex(gamehelpers->EntityToReference(pEntity));

	// This can be -1 for player ents before any players have connected
	if ((unsigned)index == INVALID_EHANDLE_INDEX || (index > 0 && index <= playerhelpers->GetMaxClients()))
	{
		return;
	}

	HandleEntityDeleted(pEntity);
}

void SDKHooks::Hook_VPhysicsUpdate(IPhysicsObject *pPhysics)
{
	Call(META_IFACEPTR(CBaseEntity), SDKHook_VPhysicsUpdate);
	RETURN_META(MRES_IGNORED);
}

void SDKHooks::Hook_VPhysicsUpdatePost(IPhysicsObject *pPhysics)
{
	Call(META_IFACEPTR(CBaseEntity), SDKHook_VPhysicsUpdatePost);
}

void SDKHooks::Hook_Blocked(CBaseEntity *pOther)
{
	cell_t result = Call(META_IFACEPTR(CBaseEntity), SDKHook_Blocked, pOther);

	if(result >= Pl_Handled)
		RETURN_META(MRES_SUPERCEDE);

	RETURN_META(MRES_IGNORED);
}

void SDKHooks::Hook_BlockedPost(CBaseEntity *pOther)
{
	Call(META_IFACEPTR(CBaseEntity), SDKHook_BlockedPost, pOther);
	RETURN_META(MRES_IGNORED);
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

void SDKHooks::HandleEntityCreated(CBaseEntity *pEntity, int index, cell_t ref)
{
	const char *pName = gamehelpers->GetEntityClassname(pEntity);
	cell_t bcompatRef = gamehelpers->EntityToBCompatRef(pEntity);

	// Send OnEntityCreated to SM listeners
	SourceHook::List<ISMEntityListener *>::iterator iter;
	ISMEntityListener *pListener = NULL;
	for (iter = m_EntListeners.begin(); iter != m_EntListeners.end(); iter++)
	{
		pListener = (*iter);
		pListener->OnEntityCreated(pEntity, pName ? pName : "");
	}

	// Call OnEntityCreated forward
	g_pOnEntityCreated->PushCell(bcompatRef);
	g_pOnEntityCreated->PushString(pName ? pName : "");
	g_pOnEntityCreated->Execute(NULL);

	m_EntityCache[index] = ref;
}

void SDKHooks::HandleEntityDeleted(CBaseEntity *pEntity)
{
	cell_t bcompatRef = gamehelpers->EntityToBCompatRef(pEntity);

	// Send OnEntityDestroyed to SM listeners
	SourceHook::List<ISMEntityListener *>::iterator iter;
	ISMEntityListener *pListener = NULL;
	for (iter = m_EntListeners.begin(); iter != m_EntListeners.end(); iter++)
	{
		pListener = (*iter);
		pListener->OnEntityDestroyed(pEntity);
	}

	// Call OnEntityDestroyed forward
	g_pOnEntityDestroyed->PushCell(bcompatRef);
	g_pOnEntityDestroyed->Execute(NULL);

	Unhook(pEntity);
}
