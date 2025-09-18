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
//   Hook name                DT required               Supported (always false til later)   Vtable Offset
	{"EndTouch",              "",                       false,                               0},
	{"FireBulletsPost",       "",                       false,                               0},
	{"OnTakeDamage",          "",                       false,                               0},
	{"OnTakeDamagePost",      "",                       false,                               0},
	{"PreThink",              "DT_BasePlayer",          false,                               0},
	{"PostThink",             "DT_BasePlayer",          false,                               0},
	{"SetTransmit",           "",                       false,                               0},
	{"Spawn",                 "",                       false,                               0},
	{"StartTouch",            "",                       false,                               0},
	{"Think",                 "",                       false,                               0},
	{"Touch",                 "",                       false,                               0},
	{"TraceAttack",           "",                       false,                               0},
	{"TraceAttackPost",       "",                       false,                               0},
	{"WeaponCanSwitchTo",     "DT_BaseCombatCharacter", false,                               0},
	{"WeaponCanUse",          "DT_BaseCombatCharacter", false,                               0},
	{"WeaponDrop",            "DT_BaseCombatCharacter", false,                               0},
	{"WeaponEquip",           "DT_BaseCombatCharacter", false,                               0},
	{"WeaponSwitch",          "DT_BaseCombatCharacter", false,                               0},
	{"ShouldCollide",         "",                       false,                               0},
	{"PreThinkPost",          "DT_BasePlayer",          false,                               0},
	{"PostThinkPost",         "DT_BasePlayer",          false,                               0},
	{"ThinkPost",             "",                       false,                               0},
	{"EndTouchPost",          "",                       false,                               0},
	{"GroundEntChangedPost",  "",                       false,                               0},
	{"SpawnPost",             "",                       false,                               0},
	{"StartTouchPost",        "",                       false,                               0},
	{"TouchPost",             "",                       false,                               0},
	{"VPhysicsUpdate",        "",                       false,                               0},
	{"VPhysicsUpdatePost",    "",                       false,                               0},
	{"WeaponCanSwitchToPost", "DT_BaseCombatCharacter", false,                               0},
	{"WeaponCanUsePost",      "DT_BaseCombatCharacter", false,                               0},
	{"WeaponDropPost",        "DT_BaseCombatCharacter", false,                               0},
	{"WeaponEquipPost",       "DT_BaseCombatCharacter", false,                               0},
	{"WeaponSwitchPost",      "DT_BaseCombatCharacter", false,                               0},
	{"Use",                   "",                       false,                               0},
	{"UsePost",               "",                       false,                               0},
	{"Reload",                "DT_BaseCombatWeapon",    false,                               0},
	{"ReloadPost",            "DT_BaseCombatWeapon",    false,                               0},
	{"GetMaxHealth",          "",                       false,                               0},
	{"Blocked",               "",                       false,                               0},
	{"BlockedPost",           "",                       false,                               0},
	{"OnTakeDamageAlive",     "DT_BaseCombatCharacter", false,                               0},
	{"OnTakeDamageAlivePost", "DT_BaseCombatCharacter", false,                               0},

	// There is no DT for CBaseMultiplayerPlayer. Going up a level
	{"CanBeAutobalanced",     "DT_BasePlayer",          false,                               0},
};

SDKHooks g_Interface;
SMEXT_LINK(&g_Interface);

unsigned int g_hookOffset[SDKHook_MAXHOOKS];

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
IForward *g_pOnGetGameNameDescription = NULL;
#endif

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

SDKHooks::SDKHooks() :
	m_HookLevelShutdown(&IServerGameDLL::LevelShutdown, this, &SDKHooks::LevelShutdown, nullptr),
#ifdef GAMEDESC_CAN_CHANGE
	m_HookGetGameDescription(&IServerGameDLL::GetGameDescription, this, &SDKHooks::Hook_GetGameDescription, nullptr),
#endif
	m_HookLevelInit(&IServerGameDLL::LevelInit, this, &SDKHooks::Hook_LevelInit, nullptr)
{}

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
#if SOURCE_ENGINE != SE_MOCK
		return false;
#endif
	}
	else
	{
		entListeners->AddToTail(this);
	}


	sharesys->AddDependency(myself, "bintools.ext", true, true);
	sharesys->AddNatives(myself, g_Natives);
	sharesys->RegisterLibrary(myself, "sdkhooks");
	sharesys->AddInterface(myself, &g_Interface);
	sharesys->AddCapabilityProvider(myself, this, "SDKHook_DmgCustomInOTD");
	sharesys->AddCapabilityProvider(myself, this, "SDKHook_LogicalEntSupport");

	m_HookLevelShutdown.Add(gamedll);

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

void SDKHooks::SDK_OnAllLoaded()
{
	SM_GET_LATE_IFACE(BINTOOLS, g_pBinTools);

	if (!g_pBinTools)
	{
		g_pSM->LogError(myself, "Could not find interface: " SMINTERFACE_BINTOOLS_NAME);
		return;
	}

	if (g_pOnLevelInit->GetFunctionCount() > 0)
		m_HookLevelInit.Add(gamedll);
#ifdef GAMEDESC_CAN_CHANGE
	if (g_pOnGetGameNameDescription->GetFunctionCount() > 0)
		m_HookGetGameDescription.Add(gamedll);
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

	m_HookLevelInit.Remove(gamedll);

#ifdef GAMEDESC_CAN_CHANGE
	m_HookGetGameDescription.Remove(gamedll);
#endif

	forwards->ReleaseForward(g_pOnEntityCreated);
	forwards->ReleaseForward(g_pOnEntityDestroyed);
#ifdef GAMEDESC_CAN_CHANGE
	forwards->ReleaseForward(g_pOnGetGameNameDescription);
#endif
	forwards->ReleaseForward(g_pOnLevelInit);

	plsys->RemovePluginsListener(&g_Interface);
	
	m_HookLevelShutdown.Remove(gamedll);
	
	playerhelpers->RemoveClientListener(&g_Interface);

	sharesys->DropCapabilityProvider(myself, this, "SDKHook_DmgCustomInOTD");
	sharesys->DropCapabilityProvider(myself, this, "SDKHook_LogicalEntSupport");

	CUtlVector<IEntityListener *> *entListeners = EntListeners();
	if (entListeners)
	{
		entListeners->FindAndRemove(this);
	}

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
	if (g_pOnLevelInit->GetFunctionCount() > 0)
		m_HookLevelInit.Add(gamedll);

#ifdef GAMEDESC_CAN_CHANGE
	if (g_pOnGetGameNameDescription->GetFunctionCount() > 0)
		m_HookGetGameDescription.Add(gamedll);
#endif
}

void SDKHooks::OnPluginUnloaded(IPlugin *plugin)
{
	Unhook(plugin->GetBaseContext());

	if (g_pOnLevelInit->GetFunctionCount() == 0)
	{
		m_HookLevelInit.Remove(gamedll);
	}

#ifdef GAMEDESC_CAN_CHANGE
	if (g_pOnGetGameNameDescription->GetFunctionCount() == 0)
		m_HookGetGameDescription.Remove(gamedll);
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

KHook::Return<void> SDKHooks::LevelShutdown(IServerGameDLL*)
{
#if defined PLATFORM_LINUX
	for (size_t type = 0; type < SDKHook_MAXHOOKS; ++type)
	{
		std::vector<CVTableList *> &vtablehooklist = g_HookList[type];
		for (size_t listentry = 0; listentry < vtablehooklist.size(); ++listentry)
		{
			std::vector<HookList> &pawnhooks = vtablehooklist[listentry]->hooks;
			pawnhooks.clear();
			
			delete vtablehooklist[listentry];
		}
		vtablehooklist.clear();
	}
#endif
	return { KHook::Action::Ignore };
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

	void** vtable = *(void***)pEnt;
	std::vector<CVTableList *> &vtablehooklist = g_HookList[type];
	for (size_t entry = 0; entry < vtablehooklist.size(); ++entry)
	{
		if (vtable != vtablehooklist[entry]->vtablehook->GetVTablePtr())
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
		ServerClass *pServerClass = gamehelpers->FindEntityServerClass(pEnt);
		if (pServerClass == nullptr)
		{
			return HookRet_BadEntForHookType;
		}

		if (!UTIL_ContainsDataTable(pServerClass->m_pTable, g_HookTypes[type].dtReq))
		{
			return HookRet_BadEntForHookType;
		}
	}

	size_t entry;
	void** vtable = *(void***)pEnt;
	std::vector<CVTableList *> &vtablehooklist = g_HookList[type];
	for (entry = 0; entry < vtablehooklist.size(); ++entry)
	{
		if (vtable == vtablehooklist[entry]->vtablehook->GetVTablePtr())
		{
			break;
		}
	}

	if (entry == vtablehooklist.size())
	{
		struct {
			void* callback_address;
			unsigned int offset;
			bool post;
		} hook_details;
		KHook::__Hook* khook = nullptr;
		int hookid = 0;
		switch(type)
		{
			case SDKHook_EndTouch:
				khook = new KHook::Member(vtable[g_HookTypes[type].offset], this, &SDKHooks::Hook_EndTouch, nullptr);
				break;
			case SDKHook_EndTouchPost:
				khook = new KHook::Member(vtable[g_HookTypes[type].offset], this, nullptr, &SDKHooks::Hook_EndTouchPost);
				break;
			case SDKHook_FireBulletsPost:
				khook = new KHook::Member(vtable[g_HookTypes[type].offset], this, nullptr, &SDKHooks::Hook_FireBulletsPost);
				break;
#ifdef GETMAXHEALTH_IS_VIRTUAL
			case SDKHook_GetMaxHealth:
				khook = new KHook::Member(vtable[g_HookTypes[type].offset], this, &SDKHooks::Hook_GetMaxHealth, nullptr);
				break;
#endif
			case SDKHook_GroundEntChangedPost:
				khook = new KHook::Member(vtable[g_HookTypes[type].offset], this, nullptr, &SDKHooks::Hook_GroundEntChangedPost);
				break;
			case SDKHook_OnTakeDamage:
				khook = new KHook::Member(vtable[g_HookTypes[type].offset], this, &SDKHooks::Hook_OnTakeDamage, nullptr);
				break;
			case SDKHook_OnTakeDamagePost:
				khook = new KHook::Member(vtable[g_HookTypes[type].offset], this, nullptr, &SDKHooks::Hook_OnTakeDamagePost);
				break;
			case SDKHook_OnTakeDamage_Alive:
				khook = new KHook::Member(vtable[g_HookTypes[type].offset], this, &SDKHooks::Hook_OnTakeDamage_Alive, nullptr);
				break;
			case SDKHook_OnTakeDamage_AlivePost:
				khook = new KHook::Member(vtable[g_HookTypes[type].offset], this, nullptr, &SDKHooks::Hook_OnTakeDamage_AlivePost);
				break;
			case SDKHook_PreThink:
				khook = new KHook::Member(vtable[g_HookTypes[type].offset], this, &SDKHooks::Hook_PreThink, nullptr);
				break;
			case SDKHook_PreThinkPost:
				khook = new KHook::Member(vtable[g_HookTypes[type].offset], this, nullptr, &SDKHooks::Hook_PreThinkPost);
				break;
			case SDKHook_PostThink:
				khook = new KHook::Member(vtable[g_HookTypes[type].offset], this, &SDKHooks::Hook_PostThink, nullptr);
				break;
			case SDKHook_PostThinkPost:
				khook = new KHook::Member(vtable[g_HookTypes[type].offset], this, nullptr, &SDKHooks::Hook_PostThinkPost);
				break;
			case SDKHook_Reload:
				khook = new KHook::Member(vtable[g_HookTypes[type].offset], this, &SDKHooks::Hook_Reload, nullptr);
				break;
			case SDKHook_ReloadPost:
				khook = new KHook::Member(vtable[g_HookTypes[type].offset], this, nullptr, &SDKHooks::Hook_ReloadPost);
				break;
			case SDKHook_SetTransmit:
				khook = new KHook::Member(vtable[g_HookTypes[type].offset], this, &SDKHooks::Hook_SetTransmit, nullptr);
				break;
			case SDKHook_Spawn:
				khook = new KHook::Member(vtable[g_HookTypes[type].offset], this, &SDKHooks::Hook_Spawn, nullptr);
				break;
			case SDKHook_SpawnPost:
				khook = new KHook::Member(vtable[g_HookTypes[type].offset], this, nullptr, &SDKHooks::Hook_SpawnPost);
				break;
			case SDKHook_StartTouch:
				khook = new KHook::Member(vtable[g_HookTypes[type].offset], this, &SDKHooks::Hook_StartTouch, nullptr);
				break;
			case SDKHook_StartTouchPost:
				khook = new KHook::Member(vtable[g_HookTypes[type].offset], this, nullptr, &SDKHooks::Hook_StartTouchPost);
				break;
			case SDKHook_Think:
				khook = new KHook::Member(vtable[g_HookTypes[type].offset], this, &SDKHooks::Hook_Think, nullptr);
				break;
			case SDKHook_ThinkPost:
				khook = new KHook::Member(vtable[g_HookTypes[type].offset], this, nullptr, &SDKHooks::Hook_ThinkPost);
				break;
			case SDKHook_Touch:
				khook = new KHook::Member(vtable[g_HookTypes[type].offset], this, &SDKHooks::Hook_Touch, nullptr);
				break;
			case SDKHook_TouchPost:
				khook = new KHook::Member(vtable[g_HookTypes[type].offset], this, nullptr, &SDKHooks::Hook_TouchPost);
				break;
			case SDKHook_TraceAttack:
				khook = new KHook::Member(vtable[g_HookTypes[type].offset], this, &SDKHooks::Hook_TraceAttack, nullptr);
				break;
			case SDKHook_TraceAttackPost:
				khook = new KHook::Member(vtable[g_HookTypes[type].offset], this, nullptr, &SDKHooks::Hook_TraceAttackPost);
				break;
			case SDKHook_Use:
				khook = new KHook::Member(vtable[g_HookTypes[type].offset], this, &SDKHooks::Hook_Use, nullptr);
				break;
			case SDKHook_UsePost:
				khook = new KHook::Member(vtable[g_HookTypes[type].offset], this, nullptr, &SDKHooks::Hook_UsePost);
				break;
			case SDKHook_VPhysicsUpdate:
				khook = new KHook::Member(vtable[g_HookTypes[type].offset], this, &SDKHooks::Hook_VPhysicsUpdate, nullptr);
				break;
			case SDKHook_VPhysicsUpdatePost:
				khook = new KHook::Member(vtable[g_HookTypes[type].offset], this, nullptr, &SDKHooks::Hook_VPhysicsUpdatePost);
				break;
			case SDKHook_WeaponCanSwitchTo:
				khook = new KHook::Member(vtable[g_HookTypes[type].offset], this, &SDKHooks::Hook_WeaponCanSwitchTo, nullptr);
				break;
			case SDKHook_WeaponCanSwitchToPost:
				khook = new KHook::Member(vtable[g_HookTypes[type].offset], this, nullptr, &SDKHooks::Hook_WeaponCanSwitchToPost);
				break;
			case SDKHook_WeaponCanUse:
				khook = new KHook::Member(vtable[g_HookTypes[type].offset], this, &SDKHooks::Hook_WeaponCanUse, nullptr);
				break;
			case SDKHook_WeaponCanUsePost:
				khook = new KHook::Member(vtable[g_HookTypes[type].offset], this, nullptr, &SDKHooks::Hook_WeaponCanUsePost);
				break;
			case SDKHook_WeaponDrop:
				khook = new KHook::Member(vtable[g_HookTypes[type].offset], this, &SDKHooks::Hook_WeaponDrop, nullptr);
				break;
			case SDKHook_WeaponDropPost:
				khook = new KHook::Member(vtable[g_HookTypes[type].offset], this, nullptr, &SDKHooks::Hook_WeaponDropPost);
				break;
			case SDKHook_WeaponEquip:
				khook = new KHook::Member(vtable[g_HookTypes[type].offset], this, &SDKHooks::Hook_WeaponEquip, nullptr);
				break;
			case SDKHook_WeaponEquipPost:
				khook = new KHook::Member(vtable[g_HookTypes[type].offset], this, nullptr, &SDKHooks::Hook_WeaponEquipPost);
				break;
			case SDKHook_WeaponSwitch:
				khook = new KHook::Member(vtable[g_HookTypes[type].offset], this, &SDKHooks::Hook_WeaponSwitch, nullptr);
				break;
			case SDKHook_WeaponSwitchPost:
				khook = new KHook::Member(vtable[g_HookTypes[type].offset], this, nullptr, &SDKHooks::Hook_WeaponSwitchPost);
				break;
			case SDKHook_ShouldCollide:
				khook = new KHook::Member(vtable[g_HookTypes[type].offset], this, nullptr, &SDKHooks::Hook_ShouldCollide);
				break;
			case SDKHook_Blocked:
				khook = new KHook::Member(vtable[g_HookTypes[type].offset], this, &SDKHooks::Hook_Blocked, nullptr);
				break;
			case SDKHook_BlockedPost:
				khook = new KHook::Member(vtable[g_HookTypes[type].offset], this, nullptr, &SDKHooks::Hook_BlockedPost);
				break;
			case SDKHook_CanBeAutobalanced:
				khook = new KHook::Member(vtable[g_HookTypes[type].offset], this, &SDKHooks::Hook_CanBeAutobalanced, nullptr);
				break;
		}

		CVTableList *vtablelist = new CVTableList;
		vtablelist->vtablehook = new CVTableHook(vtable, khook);
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

#if !defined PLATFORM_LINUX
			if (pawnhooks.size() == 0)
			{
				delete vtablehooklist[listentry];
				vtablehooklist.erase(vtablehooklist.begin() + listentry);
				listentry--;
			}
#endif
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

#if !defined PLATFORM_LINUX
			if (pawnhooks.size() == 0)
			{
				delete vtablehooklist[listentry];
				vtablehooklist.erase(vtablehooklist.begin() + listentry);
				listentry--;
			}
#endif
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

	void** vtable = *(void***)pEntity;
	std::vector<CVTableList *> &vtablehooklist = g_HookList[type];
	for (size_t listentry = 0; listentry < vtablehooklist.size(); ++listentry)
	{
		if (vtable != vtablehooklist[listentry]->vtablehook->GetVTablePtr())
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

#if !defined PLATFORM_LINUX
		if (pawnhooks.size() == 0)
		{
			delete vtablehooklist[listentry];
			vtablehooklist.erase(vtablehooklist.begin() + listentry);
			listentry--;
		}
#endif

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
KHook::Return<const char*> SDKHooks::Hook_GetGameDescription(IServerGameDLL*)
{
	static char szGameDesc[64];
	cell_t result = Pl_Continue;

	g_pSM->Format(szGameDesc, sizeof(szGameDesc), "%s",
		KHook::Recall(&IServerGameDLL::GetGameDescription, KHook::Action<const char*>{ KHook::Action::Ignore, nullptr }, gamedll));

	// Call OnGetGameDescription forward
	g_pOnGetGameNameDescription->PushStringEx(szGameDesc, sizeof(szGameDesc), SM_PARAM_STRING_COPY, SM_PARAM_COPYBACK);
	g_pOnGetGameNameDescription->Execute(&result);

	if(result == Pl_Changed)
		return { KHook::Action::Supersede, szGameDesc };

	return { KHook::Action::Ignore };
}
#endif

KHook::Return<bool> SDKHooks::Hook_LevelInit(IServerGameDLL*, char const *pMapName, char const *pMapEntities, char const *pOldLevel, char const *pLandmarkName, bool loadGame, bool background)
{
	// Call OnLevelInit forward
	g_pOnLevelInit->PushString(pMapName);
	g_pOnLevelInit->PushString("");
	g_pOnLevelInit->Execute();

	return { KHook::Action::Ignore, true };
}


/**
 * CBaseEntity Hook Handlers
 */
KHook::Return<bool> SDKHooks::Hook_CanBeAutobalanced(CBaseEntity* this_ptr)
{
	CBaseEntity *pPlayer = this_ptr;

	void** vtable = *(void***)pPlayer;
	std::vector<CVTableList *> &vtablehooklist = g_HookList[SDKHook_CanBeAutobalanced];
	for (size_t entry = 0; entry < vtablehooklist.size(); ++entry)
	{
		if (vtable != vtablehooklist[entry]->vtablehook->GetVTablePtr())
		{
			continue;
		}

		int entity = gamehelpers->EntityToBCompatRef(pPlayer);

		auto mfp = KHook::BuildMFP<CBaseEntity, bool>(KHook::GetOriginal((*(void***)this_ptr)[g_HookTypes[SDKHook_CanBeAutobalanced].offset]));
		bool origRet = (this_ptr->*mfp)();
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
			return { KHook::Action::Supersede, newRet };

		break;
	}

	return { KHook::Action::Ignore, false };
}

KHook::Return<void> SDKHooks::Hook_EndTouch(CBaseEntity* this_ptr, CBaseEntity *pOther)
{
	cell_t result = Call(this_ptr, SDKHook_EndTouch, pOther);
	if(result >= Pl_Handled)
		return { KHook::Action::Supersede };

	return { KHook::Action::Supersede };
}

KHook::Return<void> SDKHooks::Hook_EndTouchPost(CBaseEntity* this_ptr, CBaseEntity *pOther)
{
	Call(this_ptr, SDKHook_EndTouchPost, pOther);
	return { KHook::Action::Ignore };
}

KHook::Return<void> SDKHooks::Hook_FireBulletsPost(CBaseEntity* this_ptr, const FireBulletsInfo_t &info)
{
	CBaseEntity *pEntity = this_ptr;
	int entity = gamehelpers->EntityToBCompatRef(pEntity);

	IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(entity);
	if(!pPlayer)
		return { KHook::Action::Ignore };

	IPlayerInfo *pInfo = pPlayer->GetPlayerInfo();
	if(!pInfo)
		return { KHook::Action::Ignore };

	void** vtable = *(void***)pEntity;
	std::vector<CVTableList *> &vtablehooklist = g_HookList[SDKHook_FireBulletsPost];
	for (size_t entry = 0; entry < vtablehooklist.size(); ++entry)
	{
		if (vtable != vtablehooklist[entry]->vtablehook->GetVTablePtr())
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

	return { KHook::Action::Ignore };
}

#ifdef GETMAXHEALTH_IS_VIRTUAL
KHook::Return<int> SDKHooks::Hook_GetMaxHealth(CBaseEntity* this_ptr)
{
	CBaseEntity *pEntity = this_ptr;
	auto mfp = KHook::BuildMFP<CBaseEntity, int>(KHook::GetOriginal((*(void***)pEntity)[g_HookTypes[SDKHook_GetMaxHealth].offset]));
	int original_max = (pEntity->*mfp)();

	void** vtable = *(void***)this_ptr;
	std::vector<CVTableList *> &vtablehooklist = g_HookList[SDKHook_GetMaxHealth];
	for (size_t entry = 0; entry < vtablehooklist.size(); ++entry)
	{
		if (vtable != vtablehooklist[entry]->vtablehook->GetVTablePtr())
		{
			continue;
		}

		int entity = gamehelpers->EntityToBCompatRef(pEntity);

		int new_max = original_max;

		cell_t ret = Pl_Continue;

		std::vector<IPluginFunction *> callbackList;
		PopulateCallbackList(vtablehooklist[entry]->hooks, callbackList, entity);
		for (entry = 0; entry < callbackList.size(); ++entry)
		{
			IPluginFunction *callback = callbackList[entry];
			callback->PushCell(entity);
			callback->PushCellByRef(&new_max);

			cell_t res;
			callback->Execute(&res);

			if (res > ret)
			{
				ret = res;
			}
		}

		if (ret >= Pl_Handled)
			return { KHook::Action::Supersede, original_max };

		if (ret >= Pl_Changed)
			return { KHook::Action::Supersede, new_max };

		break;
	}

	return { KHook::Action::Ignore, original_max };
}
#endif

KHook::Return<void> SDKHooks::Hook_GroundEntChangedPost(CBaseEntity* this_ptr, void *pVar)
{
	Call(this_ptr, SDKHook_GroundEntChangedPost);
	return { KHook::Action::Ignore };
}

KHook::Return<int> SDKHooks::HandleOnTakeDamageHook(CBaseEntity* this_ptr, CTakeDamageInfoHack &info, SDKHookType hookType)
{
	CBaseEntity *pEntity = this_ptr;

	void** vtable = *(void***)pEntity;
	std::vector<CVTableList *> &vtablehooklist = g_HookList[hookType];
	for (size_t entry = 0; entry < vtablehooklist.size(); ++entry)
	{
		if (vtable != vtablehooklist[entry]->vtablehook->GetVTablePtr())
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
						return { KHook::Action::Ignore, 0 };
					}
					CBaseEntity *pEntInflictor = gamehelpers->ReferenceToEntity(inflictor);
					if (!pEntInflictor && inflictor != -1)
					{
						callback->GetParentContext()->BlamePluginError(callback, "Callback-provided entity %d for inflictor is invalid", inflictor);
						return { KHook::Action::Ignore, 0 };
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
			return { KHook::Action::Supersede, 1 };

		if (ret == Pl_Changed)
			return { KHook::Action::Override, 1 };

		break;
	}

	return { KHook::Action::Ignore, 0 };
}

KHook::Return<int> SDKHooks::HandleOnTakeDamageHookPost(CBaseEntity* this_ptr, CTakeDamageInfoHack &info, SDKHookType hookType)
{
	CBaseEntity *pEntity = this_ptr;

	void** vtable = *(void***)pEntity;
	std::vector<CVTableList *> &vtablehooklist = g_HookList[hookType];
	for (size_t entry = 0; entry < vtablehooklist.size(); ++entry)
	{
		if (vtable != vtablehooklist[entry]->vtablehook->GetVTablePtr())
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

	return { KHook::Action::Ignore, 0 };
}

KHook::Return<int> SDKHooks::Hook_OnTakeDamage(CBaseEntity* this_ptr, CTakeDamageInfoHack &info)
{
	return HandleOnTakeDamageHook(this_ptr, info, SDKHook_OnTakeDamage);
}

KHook::Return<int> SDKHooks::Hook_OnTakeDamagePost(CBaseEntity* this_ptr, CTakeDamageInfoHack &info)
{
	return HandleOnTakeDamageHookPost(this_ptr, info, SDKHook_OnTakeDamagePost);
}

KHook::Return<int> SDKHooks::Hook_OnTakeDamage_Alive(CBaseEntity* this_ptr, CTakeDamageInfoHack &info)
{
	return HandleOnTakeDamageHook(this_ptr, info, SDKHook_OnTakeDamage_Alive);
}

KHook::Return<int> SDKHooks::Hook_OnTakeDamage_AlivePost(CBaseEntity* this_ptr, CTakeDamageInfoHack &info)
{
	return HandleOnTakeDamageHookPost(this_ptr, info, SDKHook_OnTakeDamage_AlivePost);
}

KHook::Return<void> SDKHooks::Hook_PreThink(CBaseEntity* this_ptr)
{
	Call(this_ptr, SDKHook_PreThink);
	return { KHook::Action::Ignore };
}

KHook::Return<void> SDKHooks::Hook_PreThinkPost(CBaseEntity* this_ptr)
{
	Call(this_ptr, SDKHook_PreThinkPost);
	return { KHook::Action::Ignore };
}

KHook::Return<void> SDKHooks::Hook_PostThink(CBaseEntity* this_ptr)
{
	Call(this_ptr, SDKHook_PostThink);
	return { KHook::Action::Ignore };
}

KHook::Return<void> SDKHooks::Hook_PostThinkPost(CBaseEntity* this_ptr)
{
	Call(this_ptr, SDKHook_PostThinkPost);
	return { KHook::Action::Ignore };
}

KHook::Return<bool> SDKHooks::Hook_Reload(CBaseEntity* this_ptr)
{
	CBaseEntity *pEntity = this_ptr;

	void** vtable = *(void***)pEntity;
	std::vector<CVTableList *> &vtablehooklist = g_HookList[SDKHook_Reload];
	for (size_t entry = 0; entry < vtablehooklist.size(); ++entry)
	{
		if (vtable != vtablehooklist[entry]->vtablehook->GetVTablePtr())
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
			return { KHook::Action::Supersede, false };

		break;
	}

	return { KHook::Action::Ignore, true };
}

KHook::Return<bool> SDKHooks::Hook_ReloadPost(CBaseEntity* this_ptr)
{
	CBaseEntity *pEntity = this_ptr;

	void** vtable = *(void***)pEntity;
	std::vector<CVTableList *> &vtablehooklist = g_HookList[SDKHook_ReloadPost];
	for (size_t entry = 0; entry < vtablehooklist.size(); ++entry)
	{
		if (vtable != vtablehooklist[entry]->vtablehook->GetVTablePtr())
		{
			continue;
		}

		int entity = gamehelpers->EntityToBCompatRef(pEntity);
		cell_t origreturn = (*(bool*)::KHook::GetOriginalValuePtr()) ? 1 : 0;

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

	return { KHook::Action::Ignore, true };
}

KHook::Return<void> SDKHooks::Hook_SetTransmit(CBaseEntity* this_ptr, CCheckTransmitInfo *pInfo, bool bAlways)
{
	cell_t result = Call(this_ptr, SDKHook_SetTransmit, gamehelpers->IndexOfEdict(pInfo->m_pClientEnt));

	if(result >= Pl_Handled)
		return { KHook::Action::Supersede };

	return { KHook::Action::Ignore };
}

KHook::Return<bool> SDKHooks::Hook_ShouldCollide(CBaseEntity* this_ptr, int collisionGroup, int contentsMask)
{
	CBaseEntity *pEntity = this_ptr;

	void** vtable = *(void***)pEntity;
	std::vector<CVTableList *> &vtablehooklist = g_HookList[SDKHook_ShouldCollide];
	for (size_t entry = 0; entry < vtablehooklist.size(); ++entry)
	{
		if (vtable != vtablehooklist[entry]->vtablehook->GetVTablePtr())
		{
			continue;
		}

		int entity = gamehelpers->EntityToBCompatRef(pEntity);
		cell_t origRet = (*(bool*)KHook::GetCurrentValuePtr()) ? 1 : 0;
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

		return { KHook::Action::Supersede, ret };
	}

	return { KHook::Action::Ignore, true };
}

KHook::Return<void> SDKHooks::Hook_Spawn(CBaseEntity* this_ptr)
{
	CBaseEntity *pEntity = this_ptr;

	void** vtable = *(void***)pEntity;
	std::vector<CVTableList *> &vtablehooklist = g_HookList[SDKHook_Spawn];
	for (size_t entry = 0; entry < vtablehooklist.size(); ++entry)
	{
		if (vtable != vtablehooklist[entry]->vtablehook->GetVTablePtr())
		{
			continue;
		}

		int entity = gamehelpers->EntityToBCompatRef(pEntity);
		cell_t ret = Pl_Continue;

		std::vector<IPluginFunction *> callbackList;
		PopulateCallbackList(vtablehooklist[entry]->hooks, callbackList, entity);
		for (entry = 0; entry < callbackList.size(); ++entry)
		{
			IPluginFunction *callback = callbackList[entry];
			callback->PushCell(entity);

			cell_t res;
			callback->Execute(&res);

			if (res > ret)
			{
				ret = res;
			}
		}

		if (ret >= Pl_Handled)
			return { KHook::Action::Supersede };

		break;
	}

	return { KHook::Action::Ignore };
}

KHook::Return<void> SDKHooks::Hook_SpawnPost(CBaseEntity* this_ptr)
{
	Call(this_ptr, SDKHook_SpawnPost);
	return { KHook::Action::Ignore };
}

KHook::Return<void> SDKHooks::Hook_StartTouch(CBaseEntity* this_ptr, CBaseEntity *pOther)
{
	cell_t result = Call(this_ptr, SDKHook_StartTouch, pOther);
	if(result >= Pl_Handled)
		return { KHook::Action::Supersede };

	return { KHook::Action::Ignore };
}

KHook::Return<void> SDKHooks::Hook_StartTouchPost(CBaseEntity* this_ptr, CBaseEntity *pOther)
{
	Call(this_ptr, SDKHook_StartTouchPost, pOther);
	return { KHook::Action::Ignore };
}

KHook::Return<void> SDKHooks::Hook_Think(CBaseEntity* this_ptr)
{
	cell_t result = Call(this_ptr, SDKHook_Think);

	if(result >= Pl_Handled)
		return { KHook::Action::Supersede };

	return { KHook::Action::Ignore };
}

KHook::Return<void> SDKHooks::Hook_ThinkPost(CBaseEntity* this_ptr)
{
	Call(this_ptr, SDKHook_ThinkPost);
	return { KHook::Action::Ignore };
}

KHook::Return<void> SDKHooks::Hook_Touch(CBaseEntity* this_ptr, CBaseEntity *pOther)
{
	cell_t result = Call(this_ptr, SDKHook_Touch, pOther);

	if(result >= Pl_Handled)
		return { KHook::Action::Supersede };

	return { KHook::Action::Ignore };
}

KHook::Return<void> SDKHooks::Hook_TouchPost(CBaseEntity* this_ptr, CBaseEntity *pOther)
{
	Call(this_ptr, SDKHook_TouchPost, pOther);
	return { KHook::Action::Ignore };
}

#if SOURCE_ENGINE == SE_HL2DM || SOURCE_ENGINE == SE_DODS || SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_TF2 \
	|| SOURCE_ENGINE == SE_BMS || SOURCE_ENGINE == SE_SDK2013 || SOURCE_ENGINE == SE_PVKII
KHook::Return<void> SDKHooks::Hook_TraceAttack(CBaseEntity* this_ptr, CTakeDamageInfoHack &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator)
#else
KHook::Return<void> SDKHooks::Hook_TraceAttack(CBaseEntity* this_ptr, CTakeDamageInfoHack &info, const Vector &vecDir, trace_t *ptr)
#endif
{
	CBaseEntity *pEntity = this_ptr;

	void** vtable = *(void***)pEntity;
	std::vector<CVTableList *> &vtablehooklist = g_HookList[SDKHook_TraceAttack];
	for (size_t entry = 0; entry < vtablehooklist.size(); ++entry)
	{
		if (vtable != vtablehooklist[entry]->vtablehook->GetVTablePtr())
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
						return { KHook::Action::Ignore };
					}
					CBaseEntity *pEntInflictor = gamehelpers->ReferenceToEntity(inflictor);
					if(!pEntInflictor)
					{
						callback->GetParentContext()->BlamePluginError(callback, "Callback-provided entity %d for inflictor is invalid", inflictor);
						return { KHook::Action::Ignore };
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
			return { KHook::Action::Supersede };

		if(ret == Pl_Changed)
			return { KHook::Action::Ignore };

		break;
	}

	return { KHook::Action::Ignore };
}

#if SOURCE_ENGINE == SE_HL2DM || SOURCE_ENGINE == SE_DODS || SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_TF2 \
	|| SOURCE_ENGINE == SE_BMS || SOURCE_ENGINE == SE_SDK2013 || SOURCE_ENGINE == SE_PVKII
KHook::Return<void> SDKHooks::Hook_TraceAttackPost(CBaseEntity* this_ptr, CTakeDamageInfoHack &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator)
#else
KHook::Return<void> SDKHooks::Hook_TraceAttackPost(CBaseEntity* this_ptr, CTakeDamageInfoHack &info, const Vector &vecDir, trace_t *ptr)
#endif
{
	CBaseEntity *pEntity = this_ptr;

	void** vtable = *(void***)pEntity;
	std::vector<CVTableList *> &vtablehooklist = g_HookList[SDKHook_TraceAttackPost];
	for (size_t entry = 0; entry < vtablehooklist.size(); ++entry)
	{
		if (vtable != vtablehooklist[entry]->vtablehook->GetVTablePtr())
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

	return { KHook::Action::Ignore };
}

KHook::Return<void> SDKHooks::Hook_Use(CBaseEntity* this_ptr, CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	CBaseEntity *pEntity = this_ptr;

	void** vtable = *(void***)pEntity;
	std::vector<CVTableList *> &vtablehooklist = g_HookList[SDKHook_Use];
	for (size_t entry = 0; entry < vtablehooklist.size(); ++entry)
	{
		if (vtable != vtablehooklist[entry]->vtablehook->GetVTablePtr())
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

			cell_t res;
			callback->Execute(&res);

			if (res > ret)
			{
				ret = res;
			}
		}

		if (ret >= Pl_Handled)
			return { KHook::Action::Supersede };

		break;
	}

	return { KHook::Action::Ignore };
}

KHook::Return<void> SDKHooks::Hook_UsePost(CBaseEntity* this_ptr, CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	CBaseEntity *pEntity = this_ptr;

	void** vtable = *(void***)pEntity;
	std::vector<CVTableList *> &vtablehooklist = g_HookList[SDKHook_UsePost];
	for (size_t entry = 0; entry < vtablehooklist.size(); ++entry)
	{
		if (vtable != vtablehooklist[entry]->vtablehook->GetVTablePtr())
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

	return { KHook::Action::Ignore };
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

KHook::Return<void> SDKHooks::Hook_VPhysicsUpdate(CBaseEntity* this_ptr, IPhysicsObject *pPhysics)
{
	Call(this_ptr, SDKHook_VPhysicsUpdate);
	return { KHook::Action::Ignore };
}

KHook::Return<void> SDKHooks::Hook_VPhysicsUpdatePost(CBaseEntity* this_ptr, IPhysicsObject *pPhysics)
{
	Call(this_ptr, SDKHook_VPhysicsUpdatePost);
	return { KHook::Action::Ignore };
}

KHook::Return<void> SDKHooks::Hook_Blocked(CBaseEntity* this_ptr, CBaseEntity *pOther)
{
	cell_t result = Call(this_ptr, SDKHook_Blocked, pOther);

	if(result >= Pl_Handled)
		return { KHook::Action::Supersede };

	return { KHook::Action::Ignore };
}

KHook::Return<void> SDKHooks::Hook_BlockedPost(CBaseEntity* this_ptr, CBaseEntity *pOther)
{
	Call(this_ptr, SDKHook_BlockedPost, pOther);
	return { KHook::Action::Ignore };
}

KHook::Return<bool> SDKHooks::Hook_WeaponCanSwitchTo(CBaseEntity* this_ptr, CBaseCombatWeapon *pWeapon)
{
	cell_t result = Call(this_ptr, SDKHook_WeaponCanSwitchTo, pWeapon);

	if(result >= Pl_Handled)
		return { KHook::Action::Supersede, false };

	return { KHook::Action::Ignore, true };
}

KHook::Return<bool> SDKHooks::Hook_WeaponCanSwitchToPost(CBaseEntity* this_ptr, CBaseCombatWeapon *pWeapon)
{
	Call(this_ptr, SDKHook_WeaponCanSwitchToPost, pWeapon);
	return { KHook::Action::Ignore, true };
}

KHook::Return<bool> SDKHooks::Hook_WeaponCanUse(CBaseEntity* this_ptr, CBaseCombatWeapon *pWeapon)
{
	cell_t result = Call(this_ptr, SDKHook_WeaponCanUse, pWeapon);

	if(result >= Pl_Handled)
		return { KHook::Action::Supersede, false };

	return { KHook::Action::Ignore, true };
}

KHook::Return<bool> SDKHooks::Hook_WeaponCanUsePost(CBaseEntity* this_ptr, CBaseCombatWeapon *pWeapon)
{
	Call(this_ptr, SDKHook_WeaponCanUsePost, pWeapon);
	return { KHook::Action::Ignore, true };
}

KHook::Return<void> SDKHooks::Hook_WeaponDrop(CBaseEntity* this_ptr, CBaseCombatWeapon *pWeapon, const Vector *pvecTarget, const Vector *pVelocity)
{
	cell_t result = Call(this_ptr, SDKHook_WeaponDrop, pWeapon);

	if(result >= Pl_Handled)
		return { KHook::Action::Supersede };

	return { KHook::Action::Ignore };
}

KHook::Return<void> SDKHooks::Hook_WeaponDropPost(CBaseEntity* this_ptr, CBaseCombatWeapon *pWeapon, const Vector *pvecTarget, const Vector *pVelocity)
{
	Call(this_ptr, SDKHook_WeaponDropPost, pWeapon);
	return { KHook::Action::Ignore };
}

KHook::Return<void> SDKHooks::Hook_WeaponEquip(CBaseEntity* this_ptr, CBaseCombatWeapon *pWeapon)
{
	cell_t result = Call(this_ptr, SDKHook_WeaponEquip, pWeapon);

	if(result >= Pl_Handled)
		return { KHook::Action::Supersede };

	return { KHook::Action::Ignore };
}

KHook::Return<void> SDKHooks::Hook_WeaponEquipPost(CBaseEntity* this_ptr, CBaseCombatWeapon *pWeapon)
{
	Call(this_ptr, SDKHook_WeaponEquipPost, pWeapon);
	return { KHook::Action::Ignore };
}

KHook::Return<bool> SDKHooks::Hook_WeaponSwitch(CBaseEntity* this_ptr, CBaseCombatWeapon *pWeapon, int viewmodelindex)
{
	cell_t result = Call(this_ptr, SDKHook_WeaponSwitch, pWeapon);

	if(result >= Pl_Handled)
		return { KHook::Action::Supersede, false };

	return { KHook::Action::Ignore, true };
}

KHook::Return<bool> SDKHooks::Hook_WeaponSwitchPost(CBaseEntity* this_ptr, CBaseCombatWeapon *pWeapon, int viewmodelindex)
{
	cell_t result = Call(this_ptr, SDKHook_WeaponSwitchPost, pWeapon);
	return { KHook::Action::Ignore, true };
}

void SDKHooks::HandleEntityCreated(CBaseEntity *pEntity, int index, cell_t ref)
{
	const char *pName = gamehelpers->GetEntityClassname(pEntity);
	cell_t bcompatRef = gamehelpers->EntityToBCompatRef(pEntity);

	// Send OnEntityCreated to SM listeners
	ISMEntityListener *pListener = NULL;
	for (auto iter = m_EntListeners.begin(); iter != m_EntListeners.end(); iter++)
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
	ISMEntityListener *pListener = NULL;
	for (auto iter = m_EntListeners.begin(); iter != m_EntListeners.end(); iter++)
	{
		pListener = (*iter);
		pListener->OnEntityDestroyed(pEntity);
	}

	// Call OnEntityDestroyed forward
	g_pOnEntityDestroyed->PushCell(bcompatRef);
	g_pOnEntityDestroyed->Execute(NULL);

	Unhook(pEntity);
}
