/**
* vim: set ts=4 :
* =============================================================================
* SourceMod Counter-Strike:Source Extension
* Copyright (C) 2017 AlliedModders LLC.  All rights reserved.
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

#include "rulesfix.h"
#include "extension.h"
#include <sh_memory.h>

// Grab the convar ref
ConVar *host_rules_show = nullptr;
bool bPatched = false;

RulesFix rulesfix;

ISteamGameServer *(*SteamAPI_SteamGameServer)();
void (*SteamAPI_ISteamGameServer_SetKeyValue)(ISteamGameServer *self, const char *pKey, const char *pValue);

SH_DECL_HOOK1_void(IServerGameDLL, GameServerSteamAPIActivated, SH_NOATTRIB, 0, bool);

RulesFix::RulesFix() :
	m_OnSteamServersConnected(this, &RulesFix::OnSteamServersConnected)
{
}

bool SetMTUMax(int iValue)
{
	static int iOriginalValue = -1;
	static int *m_pMaxMTU = nullptr;

	//If we never changed skip resetting
	if (iOriginalValue == -1 && iValue == -1)
		return true;

	if (m_pMaxMTU == nullptr)
	{
		if (!g_pGameConf->GetAddress("MaxMTU", (void **)&m_pMaxMTU))
		{
			g_pSM->LogMessage(myself, "[CStrike] Failed to locate NET_SendPacket signature.");
			return false;
		}

		SourceHook::SetMemAccess(m_pMaxMTU, sizeof(int), SH_MEM_READ | SH_MEM_WRITE | SH_MEM_EXEC);

		iOriginalValue = *m_pMaxMTU;
	}

	if (iValue == -1)
		*m_pMaxMTU = iOriginalValue;
	else
		*m_pMaxMTU = iValue;
	return true;
}

void RulesFix::OnLoad()
{
	ILibrary *pLibrary = libsys->OpenLibrary(
#if defined ( PLATFORM_WINDOWS )
	"steam_api.dll"
#elif defined( PLATFORM_LINUX )
	"libsteam_api.so"
#elif defined( PLATFORM_APPLE )
	"libsteam_api.dylib"
#else
#error Unsupported platform
#endif
	, nullptr, 0);
	if (pLibrary != nullptr)
	{
		const char *pSteamGameServerFuncName = "SteamAPI_SteamGameServer_v013";
		const char *pSetKeyValueFuncName = "SteamAPI_ISteamGameServer_SetKeyValue";

		// When will hl2sdk-csgo be updated the SteamWorks SDK, change this to export.
		SteamAPI_SteamGameServer = reinterpret_cast<ISteamGameServer *(*)()>(pLibrary->GetSymbolAddress(pSteamGameServerFuncName));
		SteamAPI_ISteamGameServer_SetKeyValue = reinterpret_cast<void (*)(ISteamGameServer *self, const char *pKey, const char *pValue)>(pLibrary->GetSymbolAddress(pSetKeyValueFuncName));

		if(SteamAPI_SteamGameServer == nullptr)
		{
			g_pSM->LogError(myself, "[CStrike] Failed to get \"%s\" function", pSteamGameServerFuncName);
		}

		if(SteamAPI_ISteamGameServer_SetKeyValue == nullptr)
		{
			g_pSM->LogError(myself, "[CStrike] Failed to get \"%s\" function", pSetKeyValueFuncName);
		}

		pLibrary->CloseLibrary();
	}
	
	host_rules_show = g_pCVar->FindVar("host_rules_show");
	if (host_rules_show)
	{
		if (SetMTUMax(5000))
		{
			// Default to enabled. Explicit disable via cfg will still be obeyed.
			host_rules_show->SetValue(true);
			bPatched = true;
		}
	}

	SH_ADD_HOOK(IServerGameDLL, GameServerSteamAPIActivated, gamedll, SH_MEMBER(this, &RulesFix::Hook_GameServerSteamAPIActivated), true);
}

void RulesFix::OnUnload()
{
	SetMTUMax(-1);
	SH_REMOVE_HOOK(IServerGameDLL, GameServerSteamAPIActivated, gamedll, SH_MEMBER(this, &RulesFix::Hook_GameServerSteamAPIActivated), true);
}

void NotifyAllCVars()
{
	ICvar::Iterator iter(g_pCVar);
	for (iter.SetFirst(); iter.IsValid(); iter.Next())
	{
		ConCommandBase *cmd = iter.Get();
		if (!cmd->IsCommand() && cmd->IsFlagSet(FCVAR_NOTIFY))
		{
			rulesfix.OnNotifyConVarChanged((ConVar *)cmd);
		}
	}
}

void RulesFix::OnSteamServersConnected(SteamServersConnected_t *)
{
	// The engine clears all after the Steam interfaces become available after they've been gone.
	NotifyAllCVars();
}

static void OnConVarChanged(IConVar *var, const char *pOldValue, float flOldValue)
{
	if (host_rules_show && var == host_rules_show)
	{
		if (host_rules_show->GetBool())
		{
			if (!bPatched)
			{
				if (SetMTUMax(5000))
				{
					bPatched = true;
					NotifyAllCVars();
				}
			}
		}
		else
		{
			if (bPatched)
			{
				if (SetMTUMax(-1))
					bPatched = false;
			}
		}
	}

	if (var->IsFlagSet(FCVAR_NOTIFY))
	{
		rulesfix.OnNotifyConVarChanged((ConVar *)var);
	}
}

void RulesFix::OnNotifyConVarChanged(ConVar *pVar)
{
	if (!bPatched || !SteamAPI_SteamGameServer || !SteamAPI_ISteamGameServer_SetKeyValue)
		return;

	ISteamGameServer *pSteamClientGameServer = SteamAPI_SteamGameServer();

	if (pSteamClientGameServer)
	{
		if (pVar->IsFlagSet(FCVAR_PROTECTED))
		{
			SteamAPI_ISteamGameServer_SetKeyValue(pSteamClientGameServer, pVar->GetName(), !pVar->GetString()[0] ? "0" : "1");
		}
		else
		{
			SteamAPI_ISteamGameServer_SetKeyValue(pSteamClientGameServer, pVar->GetName(), pVar->GetString());
		}
	}
}

void RulesFix::Hook_GameServerSteamAPIActivated(bool bActivated)
{
	if (bActivated)
	{
		g_pCVar->InstallGlobalChangeCallback(OnConVarChanged);
		OnSteamServersConnected(nullptr);
	}
	else
	{
		g_pCVar->RemoveGlobalChangeCallback(OnConVarChanged);
	}
}
