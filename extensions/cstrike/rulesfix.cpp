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
	if (!bPatched)
		return;

	if (m_Steam.SteamMasterServerUpdater())
	{
		if (pVar->IsFlagSet(FCVAR_PROTECTED))
		{
			if (!pVar->GetString()[0])
			{
				m_Steam.SteamMasterServerUpdater()->SetKeyValue(pVar->GetName(), "0");
			}
			else
			{
				m_Steam.SteamMasterServerUpdater()->SetKeyValue(pVar->GetName(), "1");
			}
		}
		else
		{
			m_Steam.SteamMasterServerUpdater()->SetKeyValue(pVar->GetName(), pVar->GetString());
		}
	}
}

void RulesFix::Hook_GameServerSteamAPIActivated(bool bActivated)
{
	if (bActivated)
	{
		FixSteam();
		m_Steam.Init();

		g_pCVar->InstallGlobalChangeCallback(OnConVarChanged);
		OnSteamServersConnected(nullptr);
	}
	else
	{
		g_pCVar->RemoveGlobalChangeCallback(OnConVarChanged);
		m_Steam.Clear();
	}
}

void RulesFix::FixSteam()
{
	if (!g_pSteamClientGameServer)
	{
		void *(*pGSInternalCreateAddress)(const char *) = nullptr;
		void *(*pInternalCreateAddress)(const char *) = nullptr;

		// CS:GO currently uses the old name, but will use the new name when they update to a 
		// newer Steamworks SDK. Stay compatible.
		const char *pGSInternalFuncName = "SteamGameServerInternal_CreateInterface";
		const char *pInternalFuncName = "SteamInternal_CreateInterface";

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
			if (pGSInternalCreateAddress == nullptr)
			{
				pGSInternalCreateAddress = reinterpret_cast<void *(*)(const char *)>(pLibrary->GetSymbolAddress(pGSInternalFuncName));
			}

			if (pInternalCreateAddress == nullptr)
			{
				pInternalCreateAddress = reinterpret_cast<void *(*)(const char *)>(pLibrary->GetSymbolAddress(pInternalFuncName));
			}

			pLibrary->CloseLibrary();
		}

		if (pGSInternalCreateAddress != nullptr)
			g_pSteamClientGameServer = reinterpret_cast<ISteamClient *>((*pGSInternalCreateAddress)(STEAMCLIENT_INTERFACE_VERSION));

		if (g_pSteamClientGameServer == nullptr && pInternalCreateAddress != nullptr)
			g_pSteamClientGameServer = reinterpret_cast<ISteamClient *>((*pInternalCreateAddress)(STEAMCLIENT_INTERFACE_VERSION));
	}
}
