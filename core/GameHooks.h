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
#include <amtl/am-vector.h>

class ConVar;

namespace SourceMod {

enum class ClientCvarQueryMode {
	Unavailable,
	DLL,
	VSP
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

private:
	// Static callback that Valve's ConVar object executes when the convar's value changes.
#if SOURCE_ENGINE >= SE_ORANGEBOX
	static void OnConVarChanged(ConVar *pConVar, const char *oldValue, float flOldValue);
#else
	static void OnConVarChanged(ConVar *pConVar, const char *oldValue);
#endif

	// Callback for when StartQueryCvarValue() has finished.
#if SOURCE_ENGINE == SE_DOTA
	void OnQueryCvarValueFinished(QueryCvarCookie_t cookie, CEntityIndex player, EQueryCvarValueStatus result,
	                              const char *cvarName, const char *cvarValue);
#elif SOURCE_ENGINE != SE_DARKMESSIAH
	void OnQueryCvarValueFinished(QueryCvarCookie_t cookie, edict_t *pPlayer, EQueryCvarValueStatus result,
	                              const char *cvarName, const char *cvarValue);
#endif

private:
	class HookList : public ke::Vector<int>
	{
	public:
		HookList &operator += (int hook_id) {
			this->append(hook_id);
			return *this;
		}
	};
	HookList hooks_;

	ClientCvarQueryMode client_cvar_query_mode_;
};

} // namespace SourceMod

#endif // _INCLUDE_SOURCEMOD_PROVIDER_GAME_HOOKS_H_
