/**
* ===============================================================
* SourceMod (C)2004-2007 AlliedModders LLC.  All rights reserved.
* ===============================================================
*
* This file is not open source and may not be copied without explicit
* written permission of AlliedModders LLC.  This file may not be redistributed 
* in whole or significant part.
* For information, see LICENSE.txt or http://www.sourcemod.net/license.php
*
* Version: $Id$
*/

#include "CConVarManager.h"
#include "CLogger.h"
#include "PluginSys.h"
#include "sm_srvcmds.h"

CConVarManager g_ConVarManager;

CConVarManager::CConVarManager()
{
	m_ConVarType = 0;
	m_CvarCache = sm_trie_create();
}

CConVarManager::~CConVarManager()
{
	sm_trie_destroy(m_CvarCache);
}

void CConVarManager::OnSourceModAllInitialized()
{
	HandleAccess sec;

	sec.access[HandleAccess_Read] = 0;
	sec.access[HandleAccess_Delete] = HANDLE_RESTRICT_IDENTITY|HANDLE_RESTRICT_OWNER;
	sec.access[HandleAccess_Clone] = HANDLE_RESTRICT_IDENTITY|HANDLE_RESTRICT_OWNER;

	m_ConVarType = g_HandleSys.CreateType("ConVar", this, 0, NULL, &sec, g_pCoreIdent, NULL);

	g_RootMenu.AddRootConsoleCommand("cvars", "View convars associated with a plugin", this);
}

void CConVarManager::OnSourceModShutdown()
{
	g_RootMenu.RemoveRootConsoleCommand("cvars", this);

	g_HandleSys.RemoveType(m_ConVarType, g_pCoreIdent);
}

void CConVarManager::OnHandleDestroy(HandleType_t type, void *object)
{
	g_SMAPI->UnregisterConCmdBase(g_PLAPI, static_cast<ConCommandBase *>(object));
}

void CConVarManager::OnRootConsoleCommand(const char *command, unsigned int argcount)
{
	if (argcount >= 3)
	{
		int id = 1;

		int num = atoi(g_RootMenu.GetArgument(2));
		
		if (num < 1 || num > (int)g_PluginSys.GetPluginCount())
		{
			g_RootMenu.ConsolePrint("[SM] Plugin index not found.");
			return;
		}

		CPlugin *pl = g_PluginSys.GetPluginByOrder(num);
		int convarnum = pl->GetConVarCount();

		if (convarnum == 0)
		{
			g_RootMenu.ConsolePrint("[SM] No convars for \"%s\"", pl->GetPublicInfo()->name);
			return;
		}

		g_RootMenu.ConsolePrint("[SM] Displaying convars for \"%s\":", pl->GetPublicInfo()->name);

		for (int i = 0; i < convarnum; i++, id++)
		{
			ConVar *cvar = pl->GetConVarByIndex(i);
			g_RootMenu.ConsolePrint("  %02d \"%s\" = \"%s\"", id, cvar->GetName(), cvar->GetString()); 
		}

		return;
	}

	g_RootMenu.ConsolePrint("[SM] Usage: sm cvars <#>");
}

Handle_t CConVarManager::CreateConVar(IPluginContext *pContext, const char *name, const char *defaultVal, const char *helpText, int flags, bool hasMin, float min, bool hasMax, float max)
{
	ConVar *cvar = NULL;
	void *temp = NULL;
	Handle_t hndl = 0;

	// Find out if the convar exists already
	cvar = icvar->FindVar(name);

	// If the convar already exists...
	if (cvar != NULL)
	{
		IPlugin *pl = g_PluginSys.FindPluginByContext(pContext->GetContext());

		// This isn't a fatal error because we can handle it, but user should be warned anyways
		g_Logger.LogError("[SM] Warning: Plugin \"%s\" has attempted to create already existing convar \"%s\"", pl->GetFilename(), name);

		// First check if we already have a handle to it
		if (sm_trie_retrieve(m_CvarCache, name, &temp))
		{
			// If we do, then return that handle
			return reinterpret_cast<Handle_t>(temp);
		} else {
			// If we don't, then create a new handle from the convar and return it
			return g_HandleSys.CreateHandle(m_ConVarType, cvar, NULL, g_pCoreIdent, NULL);
		}
	}

	// Since we didn't find an existing convar, now we can create it
	cvar = new ConVar(name, defaultVal, flags, helpText, hasMin, min, hasMax, max);

	// Add new convar to plugin's list
	g_PluginSys.GetPluginByCtx(pContext->GetContext())->AddConVar(cvar);

	// Create a new handle from the convar
	hndl = g_HandleSys.CreateHandle(m_ConVarType, cvar, NULL, g_pCoreIdent, NULL);

	// Insert the handle into our cache
	sm_trie_insert(m_CvarCache, name, reinterpret_cast<void *>(hndl));

	return hndl;
}

Handle_t CConVarManager::FindConVar(const char *name)
{
	ConVar *cvar = NULL;
	void *temp = NULL;

	// Search for convar
	cvar = icvar->FindVar(name);

	// If it doesn't exist, then return an invalid handle
	if (cvar == NULL)
	{
		return BAD_HANDLE;
	}

	// At this point, convar exists, so find out if we already have a handle for it
	if (sm_trie_retrieve(m_CvarCache, name, &temp))
	{
		// If we do, then return that handle
		return reinterpret_cast<Handle_t>(temp);
	}

	// If we don't, then create a new handle from the convar and return it
	return g_HandleSys.CreateHandle(m_ConVarType, cvar, NULL, g_pCoreIdent, NULL);
}