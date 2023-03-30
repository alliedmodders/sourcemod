/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2016 AlliedModders LLC.  All rights reserved.
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

#include "common_logic.h"
#include <IPluginSys.h>
#include <IPlayerHelpers.h>
#include <IGameHelpers.h>
#include <ISourceMod.h>
#include <ITranslator.h>
#include <sh_string.h>
#include <sh_list.h>
#include "GameConfigs.h"
#include "CellArray.h"
#include "AutoHandleRooter.h"
#include "stringutil.h"
#include <bridge/include/IPlayerInfoBridge.h>
#include <bridge/include/ILogger.h>
#include <bridge/include/CoreProvider.h>
#include <bridge/include/IVEngineServerBridge.h>

using namespace SourceHook;
using namespace SourceMod;

#ifndef PRIu64
#ifdef _WIN32
#define PRIu64 "I64u"
#else
#define PRIu64 "llu"
#endif
#endif

static const int kActivityNone = 0;
static const int kActivityNonAdmins = 1;		// Show admin activity to non-admins anonymously.
static const int kActivityNonAdminsNames = 2;	// If 1 is specified, admin names will be shown.
static const int kActivityAdmins = 4;			// Show admin activity to admins anonymously.
static const int kActivityAdminsNames = 8;		// If 4 is specified, admin names will be shown.
static const int kActivityRootNames = 16;		// Always show admin names to root users.

#define FEATURECAP_MULTITARGETFILTER_CLIENTPARAM    "SourceMod MultiTargetFilter ClientParam"

class PlayerLogicHelpers : 
	public SMGlobalClass,
	public IPluginsListener,
	public ICommandTargetProcessor,
	public IFeatureProvider
{
	struct SimpleMultiTargetFilter
	{
		IPlugin *plugin;
		SourceHook::String pattern;
		IPluginFunction *fun;
		SourceHook::String phrase;
		bool phraseIsML;

		SimpleMultiTargetFilter(IPlugin *plugin, const char *pattern, IPluginFunction *fun,
		                        const char *phrase, bool phraseIsML)
		  : plugin(plugin), pattern(pattern), fun(fun), phrase(phrase), phraseIsML(phraseIsML)
		{
		}
	};

	List<SimpleMultiTargetFilter *> simpleMultis;
	bool filterEnabled;

public:
	void AddMultiTargetFilter(IPlugin *plugin, const char *pattern, IPluginFunction *fun,
	                          const char *phrase, bool phraseIsML)
	{
		SimpleMultiTargetFilter *smtf = new SimpleMultiTargetFilter(plugin, pattern, fun, phrase,
		                                                            phraseIsML);

		simpleMultis.push_back(smtf);

		if (!filterEnabled) {
			playerhelpers->RegisterCommandTargetProcessor(this);
			filterEnabled = true;
		}
	}

	void RemoveMultiTargetFilter(const char *pattern, IPluginFunction *fun)
	{
		List<SimpleMultiTargetFilter *>::iterator iter = simpleMultis.begin();

		while (iter != simpleMultis.end()) {
			if ((*iter)->fun == fun && strcmp((*iter)->pattern.c_str(), pattern) == 0) {
				delete (*iter);
				iter = simpleMultis.erase(iter);
				break;
			}
			iter++;
		}
	}

	PlayerLogicHelpers()
	 : filterEnabled(false)
	{
	}

public: //ICommandTargetProcessor
	bool ProcessCommandTarget(cmd_target_info_t *info)
	{
		List<SimpleMultiTargetFilter *>::iterator iter;

		for (iter = simpleMultis.begin(); iter != simpleMultis.end(); iter++) {
			SimpleMultiTargetFilter *smtf = (*iter);
			if (strcmp(smtf->pattern.c_str(), info->pattern) == 0) {
				CellArray *array = new CellArray(1);
				HandleSecurity sec(g_pCoreIdent, g_pCoreIdent);
				Handle_t hndl = handlesys->CreateHandleEx(htCellArray, array, &sec, NULL, NULL);
				AutoHandleCloner ahc(hndl, sec);
				if (ahc.getClone() == BAD_HANDLE) {
					logger->LogError("[SM] Could not allocate a handle (%s, %d)", __FILE__, __LINE__);
					delete array;
					return false;
				}

				smtf->fun->PushString(info->pattern);
				smtf->fun->PushCell(ahc.getClone());
				smtf->fun->PushCell(info->admin);
				cell_t result = 0;
				if (smtf->fun->Execute(&result) != SP_ERROR_NONE || !result)
					return false;

				IGamePlayer *pAdmin = info->admin
				                      ? playerhelpers->GetGamePlayer(info->admin)
				                      : NULL;

				info->num_targets = 0;
				for (size_t i = 0; i < array->size(); i++) {
					cell_t client = *array->at(i);
					IGamePlayer *pClient = playerhelpers->GetGamePlayer(client);
					if (pClient == NULL || !pClient->IsConnected())
						continue;
					if (playerhelpers->FilterCommandTarget(pAdmin, pClient, info->flags) ==
					    COMMAND_TARGET_VALID)
					{
						info->targets[info->num_targets++] = client;
						if (info->num_targets >= unsigned(info->max_targets))
							break;
					}
				}

				info->reason = info->num_targets > 0
				               ? COMMAND_TARGET_VALID
				               : COMMAND_TARGET_EMPTY_FILTER;
				if (info->num_targets) {
					strncopy(info->target_name, smtf->phrase.c_str(), info->target_name_maxlength);
					info->target_name_style = smtf->phraseIsML
					                          ? COMMAND_TARGETNAME_ML
					                          : COMMAND_TARGETNAME_RAW;
				}

				return true;
			}
		}

		return false;
	}

public: //SMGlobalClass
	void OnSourceModAllInitialized()
	{
		pluginsys->AddPluginsListener(this);
		sharesys->AddCapabilityProvider(NULL, this, FEATURECAP_MULTITARGETFILTER_CLIENTPARAM);
	}

	void OnSourceModShutdown()
	{
		pluginsys->RemovePluginsListener(this);
		if (filterEnabled) {
			playerhelpers->UnregisterCommandTargetProcessor(this);
			filterEnabled = false;
		}
		sharesys->DropCapabilityProvider(NULL, this, FEATURECAP_MULTITARGETFILTER_CLIENTPARAM);
	}

public: //IPluginsListener

	void OnPluginDestroyed(IPlugin *plugin)
	{
		List<SimpleMultiTargetFilter *>::iterator iter = simpleMultis.begin();

		while (iter != simpleMultis.end()) {
			if ((*iter)->plugin != plugin) {
				iter++;
			} else {
				delete (*iter);
				iter = simpleMultis.erase(iter);
			}
		}
	}

public: //IFeatureProvider

	FeatureStatus GetFeatureStatus(FeatureType type, const char *name)
	{
		return FeatureStatus_Available;
	}
} s_PlayerLogicHelpers;

static cell_t
AddMultiTargetFilter(IPluginContext *ctx, const cell_t *params)
{
	IPluginFunction *fun = ctx->GetFunctionById(funcid_t(params[2]));
	if (fun == NULL)
		return ctx->ThrowNativeError("Invalid function id (%X)", params[2]);

	char *pattern;
	char *phrase;

	ctx->LocalToString(params[1], &pattern);
	ctx->LocalToString(params[3], &phrase);

	bool phraseIsML = !!params[4];
	IPlugin *plugin = pluginsys->FindPluginByContext(ctx->GetContext());

	s_PlayerLogicHelpers.AddMultiTargetFilter(plugin, pattern, fun, phrase, phraseIsML);

	return 1;
}

static cell_t
RemoveMultiTargetFilter(IPluginContext *ctx, const cell_t *params)
{
	IPluginFunction *fun = ctx->GetFunctionById(funcid_t(params[2]));
	if (fun == NULL)
		return ctx->ThrowNativeError("Invalid function id (%X)", params[2]);

	char *pattern;

	ctx->LocalToString(params[1], &pattern);

	s_PlayerLogicHelpers.RemoveMultiTargetFilter(pattern, fun);

	return 1;
}

static cell_t sm_GetClientCount(IPluginContext *pCtx, const cell_t *params)
{
	if (params[1])
	{
		return playerhelpers->GetNumPlayers();
	}

	int maxplayers = playerhelpers->GetMaxClients();
	int count = 0;
	for (int i = 1; i <= maxplayers; ++i)
	{
		IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(i);
		if ((pPlayer->IsConnected()) && !(pPlayer->IsInGame()))
		{
			count++;
		}
	}

	return (playerhelpers->GetNumPlayers() + count);
}

static cell_t sm_GetMaxClients(IPluginContext *pCtx, const cell_t *params)
{
	return playerhelpers->GetMaxClients();
}

static cell_t sm_GetClientName(IPluginContext *pCtx, const cell_t *params)
{
	int index = params[1];

	if (index == 0)
	{
		static ConVar *hostname = NULL;
		if (!hostname)
		{
			hostname = bridge->FindConVar("hostname");
			if (!hostname)
			{
				return pCtx->ThrowNativeError("Could not find \"hostname\" cvar");
			}
		}
		pCtx->StringToLocalUTF8(params[2], static_cast<size_t>(params[3]), bridge->GetCvarString(hostname), NULL);
		return 1;
	}

	if ((index < 1) || (index > playerhelpers->GetMaxClients()))
	{
		return pCtx->ThrowNativeError("Client index %d is invalid", index);
	}

	IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(index);
	if (!pPlayer->IsConnected())
	{
		return pCtx->ThrowNativeError("Client %d is not connected", index);
	}

	pCtx->StringToLocalUTF8(params[2], static_cast<size_t>(params[3]), pPlayer->GetName(), NULL);
	return 1;
}

static cell_t sm_GetClientIP(IPluginContext *pCtx, const cell_t *params)
{
	int index = params[1];
	if ((index < 1) || (index > playerhelpers->GetMaxClients()))
	{
		return pCtx->ThrowNativeError("Client index %d is invalid", index);
	}

	IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(index);
	if (!pPlayer->IsConnected())
	{
		return pCtx->ThrowNativeError("Client %d is not connected", index);
	}

	char buf[64], *ptr;
	strcpy(buf, pPlayer->GetIPAddress());

	if (params[4] && (ptr = strchr(buf, ':')))
	{
		*ptr = '\0';
	}

	pCtx->StringToLocal(params[2], static_cast<size_t>(params[3]), buf);
	return 1;
}

static cell_t SteamIdToLocal(IPluginContext *pCtx, int index, AuthIdType authType, cell_t local_addr, size_t bytes, bool validate)
{
	pCtx->StringToLocal(local_addr, bytes, "STEAM_ID_STOP_IGNORING_RETVALS");

	if ((index < 1) || (index > playerhelpers->GetMaxClients()))
	{
		return pCtx->ThrowNativeError("Client index %d is invalid", index);
	}

	IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(index);
	if (!pPlayer->IsConnected())
	{
		return pCtx->ThrowNativeError("Client %d is not connected", index);
	}
	
	const char *authstr;
	
	switch (authType)
	{
	case AuthIdType::Engine:
		authstr = pPlayer->GetAuthString(validate);
		if (!authstr || authstr[0] == '\0')
		{
			return 0;
		}

		pCtx->StringToLocal(local_addr, bytes, authstr);
		break;
	case AuthIdType::Steam2:
		authstr = pPlayer->GetSteam2Id(validate);
		if (!authstr || authstr[0] == '\0')
		{
			return 0;
		}
			
		pCtx->StringToLocal(local_addr, bytes, authstr);
		break;
	case AuthIdType::Steam3:
		authstr = pPlayer->GetSteam3Id(validate);
		if (!authstr || authstr[0] == '\0')
		{
			return 0;
		}
			
		pCtx->StringToLocal(local_addr, bytes, authstr);
		break;
	
	case AuthIdType::SteamId64:
		{
			if (pPlayer->IsFakeClient() || gamehelpers->IsLANServer())
			{
				return 0;
			}
			
			uint64_t steamId = pPlayer->GetSteamId64(validate);
			if (steamId == 0)
			{
				return 0;
			}
			
			char szAuth[64];
			ke::SafeSprintf(szAuth, sizeof(szAuth), "%" PRIu64, steamId);
			
			pCtx->StringToLocal(local_addr, bytes, szAuth);
		}
		break;
	}	

	return 1;
}

static cell_t sm_GetClientAuthStr(IPluginContext *pCtx, const cell_t *params)
{
	bool validate = true;
	if (params[0] >= 4)
	{
		validate = !!params[4];
	}
	
	return SteamIdToLocal(pCtx, params[1], AuthIdType::Steam2, params[2], (size_t)params[3], validate);
}

static cell_t sm_GetClientAuthId(IPluginContext *pCtx, const cell_t *params)
{
	return SteamIdToLocal(pCtx, params[1], (AuthIdType)params[2], params[3], (size_t)params[4], params[5] != 0);
}

static cell_t sm_GetSteamAccountID(IPluginContext *pCtx, const cell_t *params)
{
	int index = params[1];
	if ((index < 1) || (index > playerhelpers->GetMaxClients()))
	{
		return pCtx->ThrowNativeError("Client index %d is invalid", index);
	}

	IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(index);
	if (!pPlayer->IsConnected())
	{
		return pCtx->ThrowNativeError("Client %d is not connected", index);
	}

	return pPlayer->GetSteamAccountID(!!params[2]);
}

static cell_t sm_IsClientConnected(IPluginContext *pCtx, const cell_t *params)
{
	int index = params[1];
	if ((index < 1) || (index > playerhelpers->GetMaxClients()))
	{
		return pCtx->ThrowNativeError("Client index %d is invalid", index);
	}

	return (playerhelpers->GetGamePlayer(index)->IsConnected()) ? 1 : 0;
}

static cell_t sm_IsClientInGame(IPluginContext *pCtx, const cell_t *params)
{
	int index = params[1];
	if ((index < 1) || (index > playerhelpers->GetMaxClients()))
	{
		return pCtx->ThrowNativeError("Client index %d is invalid", index);
	}

	return (playerhelpers->GetGamePlayer(index)->IsInGame()) ? 1 : 0;
}

static cell_t sm_IsClientAuthorized(IPluginContext *pCtx, const cell_t *params)
{
	int index = params[1];
	if ((index < 1) || (index > playerhelpers->GetMaxClients()))
	{
		return pCtx->ThrowNativeError("Client index %d is invalid", index);
	}

	return (playerhelpers->GetGamePlayer(index)->IsAuthorized()) ? 1 : 0;
}

static cell_t sm_IsClientFakeClient(IPluginContext *pCtx, const cell_t *params)
{
	int index = params[1];
	if ((index < 1) || (index > playerhelpers->GetMaxClients()))
	{
		return pCtx->ThrowNativeError("Client index %d is invalid", index);
	}

	IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(index);
	if (!pPlayer->IsConnected())
	{
		return pCtx->ThrowNativeError("Client %d is not connected", index);
	}

	return (pPlayer->IsFakeClient()) ? 1 : 0;
}

static cell_t sm_IsClientSourceTV(IPluginContext *pCtx, const cell_t *params)
{
	int index = params[1];
	if ((index < 1) || (index > playerhelpers->GetMaxClients()))
	{
		return pCtx->ThrowNativeError("Client index %d is invalid", index);
	}

	IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(index);
	if (!pPlayer->IsConnected())
	{
		return pCtx->ThrowNativeError("Client %d is not connected", index);
	}

	return (pPlayer->IsSourceTV()) ? 1 : 0;
}

static cell_t sm_IsClientReplay(IPluginContext *pCtx, const cell_t *params)
{
	int index = params[1];
	if ((index < 1) || (index > playerhelpers->GetMaxClients()))
	{
		return pCtx->ThrowNativeError("Client index %d is invalid", index);
	}

	IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(index);
	if (!pPlayer->IsConnected())
	{
		return pCtx->ThrowNativeError("Client %d is not connected", index);
	}

	return (pPlayer->IsReplay()) ? 1 : 0;
}

static cell_t sm_GetClientInfo(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];
	IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	}
	if (!pPlayer->IsConnected())
	{
		return pContext->ThrowNativeError("Client %d is not connected", client);
	}

	char *key;
	pContext->LocalToString(params[2], &key);

	const char *val = engine->GetClientConVarValue(client, key);
	if (!val)
	{
		return false;
	}

	pContext->StringToLocalUTF8(params[3], params[4], val, NULL);
	return 1;
}

static cell_t SetUserAdmin(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];
	IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	}
	if (!pPlayer->IsConnected())
	{
		return pContext->ThrowNativeError("Client %d is not connected", client);
	}
	if (!adminsys->IsValidAdmin(params[2]) && params[2] != INVALID_ADMIN_ID)
	{
		return pContext->ThrowNativeError("AdminId %x is invalid", params[2]);
	}

	pPlayer->SetAdminId(params[2], params[3] ? true : false);

	return 1;
}

static cell_t GetUserAdmin(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];
	IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	}
	if (!pPlayer->IsConnected())
	{
		return pContext->ThrowNativeError("Client %d is not connected", client);
	}

	return pPlayer->GetAdminId();
}

static cell_t AddUserFlags(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];
	IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	}
	if (!pPlayer->IsConnected())
	{
		return pContext->ThrowNativeError("Client %d is not connected", client);
	}

	AdminId id;
	if ((id = pPlayer->GetAdminId()) == INVALID_ADMIN_ID)
	{
		id = adminsys->CreateAdmin(NULL);
		pPlayer->SetAdminId(id, true);
	}

	cell_t *addr;
	for (int i = 2; i <= params[0]; i++)
	{
		pContext->LocalToPhysAddr(params[i], &addr);
		adminsys->SetAdminFlag(id, (AdminFlag) *addr, true);
	}

	return 1;
}

static cell_t RemoveUserFlags(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];
	IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	}
	if (!pPlayer->IsConnected())
	{
		return pContext->ThrowNativeError("Client %d is not connected", client);
	}

	AdminId id;
	if ((id = pPlayer->GetAdminId()) == INVALID_ADMIN_ID)
	{
		return 0;
	}

	cell_t *addr;
	for (int i = 2; i <= params[0]; i++)
	{
		pContext->LocalToPhysAddr(params[i], &addr);
		adminsys->SetAdminFlag(id, (AdminFlag) *addr, false);
	}

	return 1;
}

static cell_t SetUserFlagBits(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];
	IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	}
	if (!pPlayer->IsConnected())
	{
		return pContext->ThrowNativeError("Client %d is not connected", client);
	}

	AdminId id;
	if ((id = pPlayer->GetAdminId()) == INVALID_ADMIN_ID)
	{
		id = adminsys->CreateAdmin(NULL);
		pPlayer->SetAdminId(id, true);
	}

	adminsys->SetAdminFlags(id, Access_Effective, params[2]);

	return 1;
}

static cell_t GetUserFlagBits(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];
	IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	}
	if (!pPlayer->IsConnected())
	{
		return pContext->ThrowNativeError("Client %d is not connected", client);
	}

	AdminId id;
	if ((id = pPlayer->GetAdminId()) == INVALID_ADMIN_ID)
	{
		return 0;
	}

	return adminsys->GetAdminFlags(id, Access_Effective);
}

static cell_t GetClientUserId(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];
	IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	}
	if (!pPlayer->IsConnected())
	{
		return pContext->ThrowNativeError("Client %d is not connected", client);
	}

	return pPlayer->GetUserId();
}

static cell_t CanUserTarget(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];
	int target = params[2];

	if (client == 0)
	{
		return 1;
	}

	IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	}
	else if (!pPlayer->IsConnected()) {
		return pContext->ThrowNativeError("Client %d is not connected", client);
	}

	IGamePlayer *pTarget = playerhelpers->GetGamePlayer(target);
	if (!pTarget)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", target);
	}
	else if (!pTarget->IsConnected()) {
		return pContext->ThrowNativeError("Client %d is not connected", target);
	}

	return adminsys->CanAdminTarget(pPlayer->GetAdminId(), pTarget->GetAdminId()) ? 1 : 0;
}

static cell_t IsClientObserver(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];

	IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	}
	else if (!pPlayer->IsInGame()) {
		return pContext->ThrowNativeError("Client %d is not in game", client);
	}

	IPlayerInfo *pInfo = pPlayer->GetPlayerInfo();
	if (!pInfo)
	{
		return pContext->ThrowNativeError("IPlayerInfo not supported by game");
	}

	return bridge->playerInfo->IsObserver(pInfo) ? 1 : 0;
}

static cell_t GetClientTeam(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];

	IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	}
	else if (!pPlayer->IsInGame()) {
		return pContext->ThrowNativeError("Client %d is not in game", client);
	}

	IPlayerInfo *pInfo = pPlayer->GetPlayerInfo();
	if (!pInfo)
	{
		return pContext->ThrowNativeError("IPlayerInfo not supported by game");
	}

	return bridge->playerInfo->GetTeamIndex(pInfo);
}

static cell_t GetFragCount(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];

	IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	}
	else if (!pPlayer->IsInGame()) {
		return pContext->ThrowNativeError("Client %d is not in game", client);
	}

	IPlayerInfo *pInfo = pPlayer->GetPlayerInfo();
	if (!pInfo)
	{
		return pContext->ThrowNativeError("IPlayerInfo not supported by game");
	}

	return bridge->playerInfo->GetFragCount(pInfo);
}

static cell_t GetDeathCount(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];

	IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	}
	else if (!pPlayer->IsInGame()) {
		return pContext->ThrowNativeError("Client %d is not in game", client);
	}

	IPlayerInfo *pInfo = pPlayer->GetPlayerInfo();
	if (!pInfo)
	{
		return pContext->ThrowNativeError("IPlayerInfo not supported by game");
	}

	return bridge->playerInfo->GetDeathCount(pInfo);
}

static cell_t GetArmorValue(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];

	IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	}
	else if (!pPlayer->IsInGame()) {
		return pContext->ThrowNativeError("Client %d is not in game", client);
	}

	IPlayerInfo *pInfo = pPlayer->GetPlayerInfo();
	if (!pInfo)
	{
		return pContext->ThrowNativeError("IPlayerInfo not supported by game");
	}

	return bridge->playerInfo->GetArmorValue(pInfo);
}

static cell_t GetAbsOrigin(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];

	IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	}
	else if (!pPlayer->IsInGame()) {
		return pContext->ThrowNativeError("Client %d is not in game", client);
	}

	IPlayerInfo *pInfo = pPlayer->GetPlayerInfo();
	if (!pInfo)
	{
		return pContext->ThrowNativeError("IPlayerInfo not supported by game");
	}

	cell_t *pVec;
	pContext->LocalToPhysAddr(params[2], &pVec);

	float x, y, z;
	bridge->playerInfo->GetAbsOrigin(pInfo, &x, &y, &z);
	pVec[0] = sp_ftoc(x);
	pVec[1] = sp_ftoc(y);
	pVec[2] = sp_ftoc(z);

	return 1;
}

static cell_t GetAbsAngles(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];

	IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	}
	else if (!pPlayer->IsInGame()) {
		return pContext->ThrowNativeError("Client %d is not in game", client);
	}

	IPlayerInfo *pInfo = pPlayer->GetPlayerInfo();
	if (!pInfo)
	{
		return pContext->ThrowNativeError("IPlayerInfo not supported by game");
	}

	cell_t *pAng;
	pContext->LocalToPhysAddr(params[2], &pAng);

	float x, y, z;
	bridge->playerInfo->GetAbsAngles(pInfo, &x, &y, &z);
	pAng[0] = sp_ftoc(x);
	pAng[1] = sp_ftoc(y);
	pAng[2] = sp_ftoc(z);

	return 1;
}

static cell_t GetPlayerMins(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];

	IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	}
	else if (!pPlayer->IsInGame()) {
		return pContext->ThrowNativeError("Client %d is not in game", client);
	}

	IPlayerInfo *pInfo = pPlayer->GetPlayerInfo();
	if (!pInfo)
	{
		return pContext->ThrowNativeError("IPlayerInfo not supported by game");
	}

	cell_t *pVec;
	pContext->LocalToPhysAddr(params[2], &pVec);

	float x, y, z;
	bridge->playerInfo->GetPlayerMins(pInfo, &x, &y, &z);
	pVec[0] = sp_ftoc(x);
	pVec[1] = sp_ftoc(y);
	pVec[2] = sp_ftoc(z);

	return 1;
}

static cell_t GetPlayerMaxs(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];

	IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	}
	else if (!pPlayer->IsInGame()) {
		return pContext->ThrowNativeError("Client %d is not in game", client);
	}

	IPlayerInfo *pInfo = pPlayer->GetPlayerInfo();
	if (!pInfo)
	{
		return pContext->ThrowNativeError("IPlayerInfo not supported by game");
	}

	cell_t *pVec;
	pContext->LocalToPhysAddr(params[2], &pVec);

	float x, y, z;
	bridge->playerInfo->GetPlayerMaxs(pInfo, &x, &y, &z);
	pVec[0] = sp_ftoc(x);
	pVec[1] = sp_ftoc(y);
	pVec[2] = sp_ftoc(z);

	return 1;
}

static cell_t GetWeaponName(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];

	IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	}
	else if (!pPlayer->IsInGame()) {
		return pContext->ThrowNativeError("Client %d is not in game", client);
	}

	IPlayerInfo *pInfo = pPlayer->GetPlayerInfo();
	if (!pInfo)
	{
		return pContext->ThrowNativeError("IPlayerInfo not supported by game");
	}

	const char *weapon = bridge->playerInfo->GetWeaponName(pInfo);
	pContext->StringToLocalUTF8(params[2], static_cast<size_t>(params[3]), weapon ? weapon : "", NULL);

	return 1;
}

static cell_t GetModelName(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];

	IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	}
	else if (!pPlayer->IsInGame()) {
		return pContext->ThrowNativeError("Client %d is not in game", client);
	}

	IPlayerInfo *pInfo = pPlayer->GetPlayerInfo();
	if (!pInfo)
	{
		return pContext->ThrowNativeError("IPlayerInfo not supported by game");
	}

	const char *model = bridge->playerInfo->GetModelName(pInfo);
	pContext->StringToLocalUTF8(params[2], static_cast<size_t>(params[3]), model ? model : "", NULL);

	return 1;
}

static cell_t GetHealth(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];

	IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	}
	else if (!pPlayer->IsInGame()) {
		return pContext->ThrowNativeError("Client %d is not in game", client);
	}

	IPlayerInfo *pInfo = pPlayer->GetPlayerInfo();
	if (!pInfo)
	{
		return pContext->ThrowNativeError("IPlayerInfo not supported by game");
	}

	return bridge->playerInfo->GetHealth(pInfo);
}

static cell_t GetClientOfUserId(IPluginContext *pContext, const cell_t *params)
{
	return playerhelpers->GetClientOfUserId(params[1]);
}

static cell_t _ShowActivity(IPluginContext *pContext,
	const cell_t *params,
	const char *tag,
	cell_t fmt_param)
{
	char message[255];
	char buffer[255];
	int value = bridge->GetActivityFlags();
	unsigned int replyto = playerhelpers->GetReplyTo();
	int client = params[1];

	const char *name = "Console";
	const char *sign = "ADMIN";
	bool display_in_chat = false;
	if (client != 0)
	{
		IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(client);
		if (!pPlayer || !pPlayer->IsConnected())
		{
			return pContext->ThrowNativeError("Client index %d is invalid", client);
		}
		name = pPlayer->GetName();
		AdminId id = pPlayer->GetAdminId();
		if (id == INVALID_ADMIN_ID
			|| !adminsys->GetAdminFlag(id, Admin_Generic, Access_Effective))
		{
			sign = "PLAYER";
		}

		/* Display the message to the client? */
		if (replyto == SM_REPLY_CONSOLE)
		{
			g_pSM->SetGlobalTarget(client);

			{
				DetectExceptions eh(pContext);
				g_pSM->FormatString(buffer, sizeof(buffer), pContext, params, fmt_param);
				if (eh.HasException())
					return 0;
			}

			g_pSM->Format(message, sizeof(message), "%s%s\n", tag, buffer);
			pPlayer->PrintToConsole(message);
			display_in_chat = true;
		}
	}
	else
	{
		g_pSM->SetGlobalTarget(SOURCEMOD_SERVER_LANGUAGE);

		{
			DetectExceptions eh(pContext);
			g_pSM->FormatString(buffer, sizeof(buffer), pContext, params, fmt_param);
			if (eh.HasException())
				return 0;
		}

		g_pSM->Format(message, sizeof(message), "%s%s\n", tag, buffer);
		bridge->ConPrint(message);
	}

	if (value == kActivityNone)
	{
		return 1;
	}

	int maxClients = playerhelpers->GetMaxClients();
	for (int i = 1; i <= maxClients; i++)
	{
		IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(i);
		if (!pPlayer->IsInGame()
			|| (display_in_chat && i == client))
		{
			continue;
		}
		AdminId id = pPlayer->GetAdminId();
		g_pSM->SetGlobalTarget(i);
		if (id == INVALID_ADMIN_ID
			|| !adminsys->GetAdminFlag(id, Admin_Generic, Access_Effective))
		{
			/* Treat this as a normal user */
			if ((value & kActivityNonAdmins) || (value & kActivityNonAdminsNames))
			{
				const char *newsign = sign;
				if ((value & kActivityNonAdminsNames) || (i == client))
				{
					newsign = name;
				}

				{
					DetectExceptions eh(pContext);
					g_pSM->FormatString(buffer, sizeof(buffer), pContext, params, fmt_param);
					if (eh.HasException())
						return 0;
				}

				g_pSM->Format(message, sizeof(message), "%s%s: %s", tag, newsign, buffer);
				gamehelpers->TextMsg(i, TEXTMSG_DEST_CHAT, message);
			}
		}
		else
		{
			/* Treat this as an admin user */
			bool is_root = adminsys->GetAdminFlag(id, Admin_Root, Access_Effective);
			if ((value & kActivityAdmins)
				|| (value & kActivityAdminsNames)
				|| ((value & kActivityRootNames) && is_root))
			{
				const char *newsign = sign;
				if ((value & kActivityAdminsNames) || ((value & kActivityRootNames) && is_root) || (i == client))
				{
					newsign = name;
				}

				{
					DetectExceptions eh(pContext);
					g_pSM->FormatString(buffer, sizeof(buffer), pContext, params, fmt_param);
					if (eh.HasException())
						return 0;
				}

				g_pSM->Format(message, sizeof(message), "%s%s: %s", tag, newsign, buffer);
				gamehelpers->TextMsg(i, TEXTMSG_DEST_CHAT, message);
			}
		}
	}

	return 1;
}

static cell_t _ShowActivity2(IPluginContext *pContext,
	const cell_t *params,
	const char *tag,
	cell_t fmt_param)
{
	char message[255];
	char buffer[255];
	int value = bridge->GetActivityFlags();
	unsigned int replyto = playerhelpers->GetReplyTo();
	int client = params[1];

	const char *name = "Console";
	const char *sign = "ADMIN";
	if (client != 0)
	{
		IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(client);
		if (!pPlayer || !pPlayer->IsConnected())
		{
			return pContext->ThrowNativeError("Client index %d is invalid", client);
		}
		name = pPlayer->GetName();
		AdminId id = pPlayer->GetAdminId();
		if (id == INVALID_ADMIN_ID
			|| !adminsys->GetAdminFlag(id, Admin_Generic, Access_Effective))
		{
			sign = "PLAYER";
		}

		g_pSM->SetGlobalTarget(client);
		{
			DetectExceptions eh(pContext);
			g_pSM->FormatString(buffer, sizeof(buffer), pContext, params, fmt_param);
			if (eh.HasException())
				return 0;
		}

		/* We don't display directly to the console because the chat text
		* simply gets added to the console, so we don't want it to print
		* twice.
		*/
		g_pSM->Format(message, sizeof(message), "%s%s", tag, buffer);
		gamehelpers->TextMsg(client, TEXTMSG_DEST_CHAT, message);
	}
	else
	{
		g_pSM->SetGlobalTarget(SOURCEMOD_SERVER_LANGUAGE);
		{
			DetectExceptions eh(pContext);
			g_pSM->FormatString(buffer, sizeof(buffer), pContext, params, fmt_param);
			if (eh.HasException())
				return 0;
		}

		g_pSM->Format(message, sizeof(message), "%s%s\n", tag, buffer);
		bridge->ConPrint(message);
	}

	if (value == kActivityNone)
	{
		return 1;
	}

	int maxClients = playerhelpers->GetMaxClients();
	for (int i = 1; i <= maxClients; i++)
	{
		IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(i);
		if (!pPlayer->IsInGame()
			|| i == client)
		{
			continue;
		}
		AdminId id = pPlayer->GetAdminId();
		g_pSM->SetGlobalTarget(i);
		if (id == INVALID_ADMIN_ID
			|| !adminsys->GetAdminFlag(id, Admin_Generic, Access_Effective))
		{
			/* Treat this as a normal user */
			if ((value & kActivityNonAdmins) || (value & kActivityNonAdminsNames))
			{
				const char *newsign = sign;
				if ((value & kActivityNonAdminsNames))
				{
					newsign = name;
				}

				{
					DetectExceptions eh(pContext);
					g_pSM->FormatString(buffer, sizeof(buffer), pContext, params, fmt_param);
					if (eh.HasException())
						return 0;
				}

				g_pSM->Format(message, sizeof(message), "%s%s: %s", tag, newsign, buffer);
				gamehelpers->TextMsg(i, TEXTMSG_DEST_CHAT, message);
			}
		}
		else
		{
			/* Treat this as an admin user */
			bool is_root = adminsys->GetAdminFlag(id, Admin_Root, Access_Effective);
			if ((value & kActivityAdmins)
				|| (value & kActivityAdminsNames)
				|| ((value & kActivityRootNames) && is_root))
			{
				const char *newsign = sign;
				if ((value & kActivityAdminsNames) || ((value & kActivityRootNames) && is_root))
				{
					newsign = name;
				}

				{
					DetectExceptions eh(pContext);
					g_pSM->FormatString(buffer, sizeof(buffer), pContext, params, fmt_param);
					if (eh.HasException())
						return 0;
				}

				g_pSM->Format(message, sizeof(message), "%s%s: %s", tag, newsign, buffer);
				gamehelpers->TextMsg(i, TEXTMSG_DEST_CHAT, message);
			}
		}
	}

	return 1;
}

static cell_t ShowActivity(IPluginContext *pContext, const cell_t *params)
{
	return _ShowActivity(pContext, params, "[SM] ", 2);
}

static cell_t ShowActivityEx(IPluginContext *pContext, const cell_t *params)
{
	char *str;
	pContext->LocalToString(params[2], &str);

	return _ShowActivity(pContext, params, str, 3);
}

static cell_t ShowActivity2(IPluginContext *pContext, const cell_t *params)
{
	char *str;
	pContext->LocalToString(params[2], &str);

	return _ShowActivity2(pContext, params, str, 3);
}

static cell_t KickClient(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];

	IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	}
	else if (!pPlayer->IsConnected())
	{
		return pContext->ThrowNativeError("Client %d is not connected", client);
	}

	/* Ignore duplicate kicks */
	if (pPlayer->IsInKickQueue())
	{
		return 1;
	}

	g_pSM->SetGlobalTarget(client);

	char buffer[256];
	{
		DetectExceptions eh(pContext);
		g_pSM->FormatString(buffer, sizeof(buffer), pContext, params, 2);
		if (eh.HasException())
			return 0;
	}

	gamehelpers->AddDelayedKick(client, pPlayer->GetUserId(), buffer);

	return 1;
}

static cell_t KickClientEx(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];

	IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	}
	else if (!pPlayer->IsConnected())
	{
		return pContext->ThrowNativeError("Client %d is not connected", client);
	}

	g_pSM->SetGlobalTarget(client);

	char buffer[256];
	{
		DetectExceptions eh(pContext);
		g_pSM->FormatString(buffer, sizeof(buffer), pContext, params, 2);
		if (eh.HasException())
			return 0;
	}

	pPlayer->Kick(buffer);

	return 1;
}

static cell_t ChangeClientTeam(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];

	IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	}
	else if (!pPlayer->IsInGame())
	{
		return pContext->ThrowNativeError("Client %d is not in game", client);
	}

	IPlayerInfo *pInfo = pPlayer->GetPlayerInfo();
	if (!pInfo)
	{
		return pContext->ThrowNativeError("IPlayerInfo not supported by game");
	}

	bridge->playerInfo->ChangeTeam(pInfo, params[2]);

	return 1;
}

static cell_t NotifyPostAdminCheck(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];

	IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	}
	else if (!pPlayer->IsInGame()) {
		return pContext->ThrowNativeError("Client %d is not in game", client);
	}
	else if (!pPlayer->IsAuthorized())
	{
		return pContext->ThrowNativeError("Client %d is not authorized", client);
	}

	pPlayer->NotifyPostAdminChecks();

	return 1;
}

static cell_t IsClientInKickQueue(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];

	IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(client);

	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	}
	else if (!pPlayer->IsConnected())
	{
		return pContext->ThrowNativeError("Client %d is not in game", client);
	}

	return pPlayer->IsInKickQueue() ? 1 : 0;
}

static cell_t ProcessTargetString(IPluginContext *pContext, const cell_t *params)
{
	cmd_target_info_t info;

	pContext->LocalToString(params[1], (char **) &info.pattern);
	info.admin = params[2];
	pContext->LocalToPhysAddr(params[3], &info.targets);
	info.max_targets = params[4];
	info.flags = params[5];
	pContext->LocalToString(params[6], &info.target_name);
	info.target_name_maxlength = params[7];

	cell_t *tn_is_ml;
	pContext->LocalToPhysAddr(params[8], &tn_is_ml);

	playerhelpers->ProcessCommandTarget(&info);

	if (info.target_name_style == COMMAND_TARGETNAME_ML)
	{
		*tn_is_ml = 1;
	}
	else
	{
		*tn_is_ml = 0;
	}

	if (info.num_targets == 0)
	{
		return info.reason;
	}
	else
	{
		return info.num_targets;
	}
}

static cell_t FormatActivitySource(IPluginContext *pContext, const cell_t *params)
{
	int value;
	int client;
	int target;
	IGamePlayer *pTarget;
	AdminId aidTarget;
	const char *identity[2] = { "Console", "ADMIN" };

	client = params[1];
	target = params[2];

	if ((pTarget = playerhelpers->GetGamePlayer(target)) == NULL)
	{
		return pContext->ThrowNativeError("Invalid client index %d", target);
	}
	if (!pTarget->IsConnected())
	{
		return pContext->ThrowNativeError("Client %d not connected", target);
	}

	value = bridge->GetActivityFlags();

	if (client != 0)
	{
		IGamePlayer *pPlayer;

		if ((pPlayer = playerhelpers->GetGamePlayer(client)) == NULL)
		{
			return pContext->ThrowNativeError("Invalid client index %d", client);
		}
		if (!pPlayer->IsConnected())
		{
			return pContext->ThrowNativeError("Client %d not connected", client);
		}

		identity[0] = pPlayer->GetName();

		AdminId id = pPlayer->GetAdminId();
		if (id == INVALID_ADMIN_ID
			|| !adminsys->GetAdminFlag(id, Admin_Generic, Access_Effective))
		{
			identity[1] = "PLAYER";
		}
	}

	int mode = 1;
	bool bShowActivity = false;

	if ((aidTarget = pTarget->GetAdminId()) == INVALID_ADMIN_ID
		|| !adminsys->GetAdminFlag(aidTarget, Admin_Generic, Access_Effective))
	{
		/* Treat this as a normal user */
		if ((value & kActivityNonAdmins) || (value & kActivityNonAdminsNames))
		{
			if ((value & 2) || (target == client))
			{
				mode = 0;
			}
			bShowActivity = true;
		}
	}
	else
	{
		/* Treat this as an admin user */
		bool is_root = adminsys->GetAdminFlag(aidTarget, Admin_Root, Access_Effective);
		if ((value & kActivityAdmins)
			|| (value & kActivityAdminsNames)
			|| ((value & kActivityRootNames) && is_root))
		{
			if ((value & kActivityAdminsNames) || ((value & kActivityRootNames) && is_root) || (target == client))
			{
				mode = 0;
			}
			bShowActivity = true;
		}
	}

	/* Otherwise, send it back to the script. */
	pContext->StringToLocalUTF8(params[3], params[4], identity[mode], NULL);

	return bShowActivity ? 1 : 0;
}

static cell_t sm_GetClientSerial(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];

	IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(client);
	if (!pPlayer)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", client);
	}

	return pPlayer->GetSerial();
}

static cell_t sm_GetClientFromSerial(IPluginContext *pContext, const cell_t *params)
{
	return playerhelpers->GetClientFromSerial(params[1]);
}


REGISTER_NATIVES(playernatives)
{
	{"AddMultiTargetFilter",	AddMultiTargetFilter},
	{"RemoveMultiTargetFilter",	RemoveMultiTargetFilter},
	{ "AddUserFlags", AddUserFlags },
	{ "CanUserTarget", CanUserTarget },
	{ "ChangeClientTeam", ChangeClientTeam },
	{ "GetClientAuthString", sm_GetClientAuthStr },
	{ "GetClientAuthId", sm_GetClientAuthId },
	{ "GetSteamAccountID", sm_GetSteamAccountID },
	{ "GetClientCount", sm_GetClientCount },
	{ "GetClientInfo", sm_GetClientInfo },
	{ "GetClientIP", sm_GetClientIP },
	{ "GetClientName", sm_GetClientName },
	{ "GetClientTeam", GetClientTeam },
	{ "GetClientUserId", GetClientUserId },
	{ "GetMaxClients", sm_GetMaxClients },
	{ "GetUserAdmin", GetUserAdmin },
	{ "GetUserFlagBits", GetUserFlagBits },
	{ "IsClientAuthorized", sm_IsClientAuthorized },
	{ "IsClientConnected", sm_IsClientConnected },
	{ "IsFakeClient", sm_IsClientFakeClient },
	{ "IsClientSourceTV", sm_IsClientSourceTV },
	{ "IsClientReplay", sm_IsClientReplay },
	{ "IsClientInGame", sm_IsClientInGame },
	{ "IsClientObserver", IsClientObserver },
	{ "RemoveUserFlags", RemoveUserFlags },
	{ "SetUserAdmin", SetUserAdmin },
	{ "SetUserFlagBits", SetUserFlagBits },
	{ "GetClientDeaths", GetDeathCount },
	{ "GetClientFrags", GetFragCount },
	{ "GetClientArmor", GetArmorValue },
	{ "GetClientAbsOrigin", GetAbsOrigin },
	{ "GetClientAbsAngles", GetAbsAngles },
	{ "GetClientMins", GetPlayerMins },
	{ "GetClientMaxs", GetPlayerMaxs },
	{ "GetClientWeapon", GetWeaponName },
	{ "GetClientModel", GetModelName },
	{ "GetClientHealth", GetHealth },
	{ "GetClientOfUserId", GetClientOfUserId },
	{ "ShowActivity", ShowActivity },
	{ "ShowActivityEx", ShowActivityEx },
	{ "ShowActivity2", ShowActivity2 },
	{ "KickClient", KickClient },
	{ "KickClientEx", KickClientEx },
	{ "NotifyPostAdminCheck", NotifyPostAdminCheck },
	{ "IsClientInKickQueue", IsClientInKickQueue },
	{ "ProcessTargetString", ProcessTargetString },
	{ "FormatActivitySource", FormatActivitySource },
	{ "GetClientSerial", sm_GetClientSerial },
	{ "GetClientFromSerial", sm_GetClientFromSerial },
	{NULL,						NULL}
};

