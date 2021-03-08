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

#if SOURCE_ENGINE >= SE_ORANGEBOX
SH_DECL_HOOK3_void(ICvar, CallGlobalChangeCallbacks, SH_NOATTRIB, false, ConVar *, const char *, float);
#else
SH_DECL_HOOK2_void(ICvar, CallGlobalChangeCallback, SH_NOATTRIB, false, ConVar *, const char *);
#endif

#if SOURCE_ENGINE != SE_DARKMESSIAH
SH_DECL_HOOK5_void(IServerGameDLL, OnQueryCvarValueFinished, SH_NOATTRIB, 0, QueryCvarCookie_t, edict_t *, EQueryCvarValueStatus, const char *, const char *);
SH_DECL_HOOK5_void(IServerPluginCallbacks, OnQueryCvarValueFinished, SH_NOATTRIB, 0, QueryCvarCookie_t, edict_t *, EQueryCvarValueStatus, const char *, const char *);
#endif

#if SOURCE_ENGINE >= SE_ORANGEBOX
SH_DECL_HOOK1_void(ConCommand, Dispatch, SH_NOATTRIB, false, const CCommand &);
#else
SH_DECL_HOOK0_void(ConCommand, Dispatch, SH_NOATTRIB, false);
#endif

SH_DECL_HOOK1_void(IServerGameClients, SetCommandClient, SH_NOATTRIB, false, int);

GameHooks::GameHooks()
	: client_cvar_query_mode_(ClientCvarQueryMode::Unavailable),
	  last_command_client_(-1)
{
}

void GameHooks::Start()
{
	// Hook ICvar::CallGlobalChangeCallbacks.
#if SOURCE_ENGINE >= SE_ORANGEBOX
	hooks_ += SH_ADD_HOOK(ICvar, CallGlobalChangeCallbacks, icvar, SH_STATIC(OnConVarChanged), false);
#else
	hooks_ += SH_ADD_HOOK(ICvar, CallGlobalChangeCallback, icvar, SH_STATIC(OnConVarChanged), false);
#endif

	// Episode 2 has this function by default, but the older versions do not.
#if SOURCE_ENGINE == SE_EPISODEONE
	if (g_SMAPI->GetGameDLLVersion() >= 6) {
		hooks_ += SH_ADD_HOOK(IServerGameDLL, OnQueryCvarValueFinished, gamedll, SH_MEMBER(this, &GameHooks::OnQueryCvarValueFinished), false);
		client_cvar_query_mode_ = ClientCvarQueryMode::DLL;
	}
#endif

	hooks_ += SH_ADD_HOOK(IServerGameClients, SetCommandClient, serverClients, SH_MEMBER(this, &GameHooks::SetCommandClient), false);
}

void GameHooks::OnVSPReceived()
{
	if (client_cvar_query_mode_ != ClientCvarQueryMode::Unavailable)
		return;

	if (g_SMAPI->GetSourceEngineBuild() == SOURCE_ENGINE_ORIGINAL || vsp_version < 2)
		return;

#if SOURCE_ENGINE != SE_DARKMESSIAH
	hooks_ += SH_ADD_HOOK(IServerPluginCallbacks, OnQueryCvarValueFinished, vsp_interface, SH_MEMBER(this, &GameHooks::OnQueryCvarValueFinished), false);
	client_cvar_query_mode_ = ClientCvarQueryMode::VSP;
#endif
}

void GameHooks::Shutdown()
{
	for (size_t i = 0; i < hooks_.size(); i++)
		SH_REMOVE_HOOK_ID(hooks_[i]);
	hooks_.clear();

	client_cvar_query_mode_ = ClientCvarQueryMode::Unavailable;
}

#if SOURCE_ENGINE >= SE_ORANGEBOX
void GameHooks::OnConVarChanged(ConVar *pConVar, const char *oldValue, float flOldValue)
#else
void GameHooks::OnConVarChanged(ConVar *pConVar, const char *oldValue)
#endif
{
#if SOURCE_ENGINE < SE_ORANGEBOX
  float flOldValue = atof(oldValue);
#endif
  g_ConVarManager.OnConVarChanged(pConVar, oldValue, flOldValue);
}

#if SOURCE_ENGINE != SE_DARKMESSIAH
void GameHooks::OnQueryCvarValueFinished(QueryCvarCookie_t cookie, edict_t *pPlayer, EQueryCvarValueStatus result,
                                         const char *cvarName, const char *cvarValue){
	int client = IndexOfEdict(pPlayer);

# if SOURCE_ENGINE == SE_CSGO
	if (g_Players.HandleConVarQuery(cookie, client, result, cvarName, cvarValue))
		return;
# endif
	g_ConVarManager.OnClientQueryFinished(cookie, client, result, cvarName, cvarValue);
}
#endif

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

void GameHooks::SetCommandClient(int client)
{
	last_command_client_ = client + 1;
}

CommandHook::CommandHook(ConCommand *cmd, const Callback &callback, bool post)
 : hook_id_(0),
   callback_(callback)
{
	hook_id_ = SH_ADD_HOOK(ConCommand, Dispatch, cmd, SH_MEMBER(this, &CommandHook::Dispatch), post);
}

CommandHook::~CommandHook()
{
	if (hook_id_)
	  SH_REMOVE_HOOK_ID(hook_id_);
}

void CommandHook::Dispatch(DISPATCH_ARGS)
{
	DISPATCH_PROLOGUE;
	EngineArgs args(command);

	AddRef();
	bool rval = callback_(sCoreProviderImpl.CommandClient(), &args);
	Release();
	if (rval)
		RETURN_META(MRES_SUPERCEDE);
}

void CommandHook::Zap()
{
	hook_id_ = 0;
}
