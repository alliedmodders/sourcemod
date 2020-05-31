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
#ifndef _INCLUDE_SOURCEMOD_PROVIDER_GAME_HOOKS_H_
#define _INCLUDE_SOURCEMOD_PROVIDER_GAME_HOOKS_H_

// Needed for CEntityIndex, edict_t, etc.
#include <stdint.h>
#include <stddef.h>
#include <eiface.h>
#include <iserverplugin.h>
#include <amtl/am-refcounting.h>
#include <amtl/am-vector.h>
#include <amtl/am-function.h>

class ConVar;
class CCommand;
struct CCommandContext;

#if SOURCE_ENGINE >= SE_ORANGEBOX
# define DISPATCH_ARGS      const CCommand &command
# define DISPATCH_PROLOGUE
#else
# define DISPATCH_ARGS
# define DISPATCH_PROLOGUE  CCommand command
#endif

namespace SourceMod {

// Describes the mechanism in which client cvar queries are implemented.
enum class ClientCvarQueryMode {
	Unavailable,
	DLL,
	VSP
};

class ICommandArgs;

class CommandHook : public ke::Refcounted<CommandHook>
{
public:
	// return false to RETURN_META(MRES_IGNORED), or true to SUPERCEDE.
	typedef ke::Function<bool(int, const ICommandArgs *)> Callback;

public:
	CommandHook(ConCommand *cmd, const Callback &callback, bool post);
	~CommandHook();
	void Dispatch(DISPATCH_ARGS);
	void Zap();

private:
	int hook_id_;
	Callback callback_;
};

class GameHooks
{
public:
	GameHooks();

	void Start();
	void Shutdown();
	void OnVSPReceived();

	ClientCvarQueryMode GetClientCvarQueryMode() const {
		return client_cvar_query_mode_;
	}

public:
	ke::RefPtr<CommandHook> AddCommandHook(ConCommand *cmd, const CommandHook::Callback &callback);
	ke::RefPtr<CommandHook> AddPostCommandHook(ConCommand *cmd, const CommandHook::Callback &callback);

	int CommandClient() const {
		return last_command_client_;
	}

private:
	// Static callback that Valve's ConVar object executes when the convar's value changes.
#if SOURCE_ENGINE >= SE_ORANGEBOX
	static void OnConVarChanged(ConVar *pConVar, const char *oldValue, float flOldValue);
#else
	static void OnConVarChanged(ConVar *pConVar, const char *oldValue);
#endif

	// Callback for when StartQueryCvarValue() has finished.
#if SOURCE_ENGINE != SE_DARKMESSIAH
	void OnQueryCvarValueFinished(QueryCvarCookie_t cookie, edict_t *pPlayer, EQueryCvarValueStatus result,
	                              const char *cvarName, const char *cvarValue);
#endif

	void SetCommandClient(int client);

private:
	class HookList : public std::vector<int>
	{
	public:
		HookList &operator += (int hook_id) {
			this->push_back(hook_id);
			return *this;
		}
	};
	HookList hooks_;

	ClientCvarQueryMode client_cvar_query_mode_;
	int last_command_client_;
};

} // namespace SourceMod

#endif // _INCLUDE_SOURCEMOD_PROVIDER_GAME_HOOKS_H_
