// vim: set ts=4 sw=4 tw=99 noet :
// =============================================================================
// SourceMod
// Copyright (C) 2004-2015 AlliedModders LLC.  All rights reserved.
// =============================================================================
//
// This program is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License, version 3.0, as published by the
// Free Software Foundation.
// 
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along with
// this program.  If not, see <http://www.gnu.org/licenses/>.
//
// As a special exception, AlliedModders LLC gives you permission to link the
// code of this program (as well as its derivative works) to "Half-Life 2," the
// "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
// by the Valve Corporation.  You must obey the GNU General Public License in
// all respects for all other code used.  Additionally, AlliedModders LLC grants
// this exception to all derivative works.  AlliedModders LLC defines further
// exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
// or <http://www.sourcemod.net/license.php>.
#include "GameHooks.h"
#include "sourcemod.h"
#include "ConVarManager.h"
#include "command_args.h"
#include "provider.h"

KHook::Virtual CallGlobalChangeCallback_Hook(&ICvar::CallGlobalChangeCallbacks, &GameHooks::OnConVarChanged, nullptr);

GameHooks::GameHooks()
	:
#if SOURCE_ENGINE != SE_DARKMESSIAH
	m_GameDLLOnQueryCvarValueFinishedHook(&IServerGameDLL::OnQueryCvarValueFinished, this, &GameHooks::GameDLLOnQueryCvarValueFinished, nullptr),
	m_VSPOnQueryCvarValueFinishedHook(&IServerPluginCallbacks::OnQueryCvarValueFinished, this, &GameHooks::VSPOnQueryCvarValueFinished, nullptr),
#endif
	m_SetCommandClient(&IServerGameClients::SetCommandClient, this, &GameHooks::SetCommandClient, nullptr)
	,client_cvar_query_mode_(ClientCvarQueryMode::Unavailable)
	,last_command_client_(-1)
{
}

void GameHooks::Start()
{
	// Hook ICvar::CallGlobalChangeCallbacks.
	CallGlobalChangeCallback_Hook.Add(icvar);

	// Episode 2 has this function by default, but the older versions do not.
#if SOURCE_ENGINE == SE_EPISODEONE
	if (g_SMAPI->GetGameDLLVersion() >= 6) {
		m_GameDLLOnQueryCvarValueFinishedHook.Add(gamedll);
		client_cvar_query_mode_ = ClientCvarQueryMode::DLL;
	}
#endif
	m_SetCommandClient.Add(serverClients);
}

void GameHooks::OnVSPReceived()
{
	if (client_cvar_query_mode_ != ClientCvarQueryMode::Unavailable)
		return;

	if (g_SMAPI->GetSourceEngineBuild() == SOURCE_ENGINE_ORIGINAL || vsp_version < 2)
		return;

#if SOURCE_ENGINE != SE_DARKMESSIAH
	m_VSPOnQueryCvarValueFinishedHook.Add(vsp_interface);
	client_cvar_query_mode_ = ClientCvarQueryMode::VSP;
#endif
}

void GameHooks::Shutdown()
{
	CallGlobalChangeCallback_Hook.Remove(icvar);
	m_GameDLLOnQueryCvarValueFinishedHook.Remove(gamedll);
	m_SetCommandClient.Remove(serverClients);
	m_VSPOnQueryCvarValueFinishedHook.Remove(vsp_interface);

	client_cvar_query_mode_ = ClientCvarQueryMode::Unavailable;
}

#if SOURCE_ENGINE >= SE_ORANGEBOX
KHook::Return<void> GameHooks::OnConVarChanged(ICvar*, ConVar *pConVar, const char *oldValue, float flOldValue)
#else
KHook::Return<void> GameHooks::OnConVarChanged(ICvar*, ConVar *pConVar, const char *oldValue)
#endif
{
#if SOURCE_ENGINE < SE_ORANGEBOX
  float flOldValue = atof(oldValue);
#endif
  g_ConVarManager.OnConVarChanged(pConVar, oldValue, flOldValue);

  return { KHook::Action::Ignore };
}

#if SOURCE_ENGINE != SE_DARKMESSIAH
void GameHooks::OnQueryCvarValueFinished(QueryCvarCookie_t cookie, edict_t *pPlayer, EQueryCvarValueStatus result,
                                         const char *cvarName, const char *cvarValue){
	int client = IndexOfEdict(pPlayer);

# if SOURCE_ENGINE == SE_CSGO || SOURCE_ENGINE == SE_BLADE || SOURCE_ENGINE == SE_MCV
	if (g_Players.HandleConVarQuery(cookie, client, result, cvarName, cvarValue))
		return;
# endif
	g_ConVarManager.OnClientQueryFinished(cookie, client, result, cvarName, cvarValue);
}
#endif

KHook::Return<void> GameHooks::GameDLLOnQueryCvarValueFinished(IServerGameDLL*, QueryCvarCookie_t cookie, edict_t *pPlayer, EQueryCvarValueStatus result,
	                              const char *cvarName, const char *cvarValue) {
	OnQueryCvarValueFinished(cookie, pPlayer, result, cvarName, cvarValue);
	return { KHook::Action::Ignore };
}
KHook::Return<void> GameHooks::VSPOnQueryCvarValueFinished(IServerPluginCallbacks*, QueryCvarCookie_t cookie, edict_t *pPlayer, EQueryCvarValueStatus result,
	                              const char *cvarName, const char *cvarValue) {
	OnQueryCvarValueFinished(cookie, pPlayer, result, cvarName, cvarValue);
	return { KHook::Action::Ignore };
}

ke::RefPtr<CommandHook>
GameHooks::AddCommandHook(ConCommand *cmd, const CommandHook::Callback &callback)
{
	return new CommandHook(cmd, callback, false);
}

ke::RefPtr<CommandHook>
GameHooks::AddPostCommandHook(ConCommand *cmd, const CommandHook::Callback &callback)
{
	return new CommandHook(cmd, callback, true);
}

KHook::Return<void> GameHooks::SetCommandClient(IServerGameClients*, int client)
{
	last_command_client_ = client + 1;

	return { KHook::Action::Ignore };
}

CommandHook::CommandHook(ConCommand *cmd, const Callback &callback, bool post)
 : callback_(callback),
   m_DispatchHook(&ConCommand::Dispatch, this, (post) ? nullptr : &CommandHook::Dispatch, (post) ? &CommandHook::Dispatch : nullptr)
{
	m_DispatchHook.Add(cmd);
}

CommandHook::~CommandHook()
{
}

#if SOURCE_ENGINE >= SE_ORANGEBOX
KHook::Return<void> CommandHook::Dispatch(ConCommand*, const CCommand& command)
#else
KHook::Return<void> CommandHook::Dispatch(ConCommand*)
#endif
{
	DISPATCH_PROLOGUE
	EngineArgs args(command);

	AddRef();
	bool rval = callback_(sCoreProviderImpl.CommandClient(), &args);
	Release();
	if (rval)
	{
		return { KHook::Action::Supersede };
	}
	return { KHook::Action::Ignore };
}