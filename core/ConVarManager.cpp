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

#include "ConVarManager.h"
#include "PluginSys.h"
#include "ForwardSys.h"
#include "HandleSys.h"
#include "sm_srvcmds.h"
#include "sm_stringutil.h"
#include <sh_vector.h>

ConVarManager g_ConVarManager;

const ParamType CONVARCHANGE_PARAMS[] = {Param_Cell, Param_String, Param_String};
typedef List<const ConVar *> ConVarList;

ConVarManager::ConVarManager() : m_ConVarType(0)
{
	/* Create a convar lookup trie */
	m_ConVarCache = sm_trie_create();
}

ConVarManager::~ConVarManager()
{
	List<ConVarInfo *>::iterator iter;

	/* Destroy our convar lookup trie */
	sm_trie_destroy(m_ConVarCache);

	/* Destroy all the ConVarInfo structures */
	for (iter = m_ConVars.begin(); iter != m_ConVars.end(); iter++)
	{
		delete (*iter);
	}

	m_ConVars.clear();
}

void ConVarManager::OnSourceModAllInitialized()
{
	HandleAccess sec;

	/* Set up access rights for the 'ConVar' handle type */
	sec.access[HandleAccess_Read] = 0;
	sec.access[HandleAccess_Delete] = HANDLE_RESTRICT_IDENTITY | HANDLE_RESTRICT_OWNER;
	sec.access[HandleAccess_Clone] = HANDLE_RESTRICT_IDENTITY | HANDLE_RESTRICT_OWNER;

	/* Create the 'ConVar' handle type */
	m_ConVarType = g_HandleSys.CreateType("ConVar", this, 0, NULL, &sec, g_pCoreIdent, NULL);

	/* Add the 'convars' option to the 'sm' console command */
	g_RootMenu.AddRootConsoleCommand("convars", "View convars created by a plugin", this);
}

void ConVarManager::OnSourceModShutdown()
{
	IChangeableForward *fwd;
	List<ConVarInfo *>::iterator i;

	/* Iterate list of ConVarInfo structures */
	for (i = m_ConVars.begin(); i != m_ConVars.end(); i++)
	{
		fwd = (*i)->changeForward;

		/* Free any convar-change forwards that still exist */
		if (fwd)
		{
			g_Forwards.ReleaseForward(fwd);
		}
	}

	/* Remove the 'convars' option from the 'sm' console command */
	g_RootMenu.RemoveRootConsoleCommand("convars", this);

	/* Remove the 'ConVar' handle type */
	g_HandleSys.RemoveType(m_ConVarType, g_pCoreIdent);
}

void ConVarManager::OnPluginUnloaded(IPlugin *plugin)
{
	ConVarList *pConVarList;

	/* If plugin has a convar list, free its memory */
	if (plugin->GetProperty("ConVarList", (void **)&pConVarList, true))
	{
		delete pConVarList;
	}
}

void ConVarManager::OnHandleDestroy(HandleType_t type, void *object)
{
	ConVarInfo *info;
	ConVar *pConVar = static_cast<ConVar *>(object);

	/* Find convar in lookup trie */
	sm_trie_retrieve(m_ConVarCache, pConVar->GetName(), reinterpret_cast<void **>(&info));

	/* If convar was created by SourceMod plugin... */
	if (info->sourceMod)
	{
		/* Delete string allocations */
		delete [] pConVar->GetName(); 
		delete [] pConVar->GetDefault();
		delete [] pConVar->GetHelpText();

		/* Then unlink it from SourceMM */
		g_SMAPI->UnregisterConCmdBase(g_PLAPI, pConVar);
	}
}

void ConVarManager::OnRootConsoleCommand(const char *command, unsigned int argcount)
{
	if (argcount >= 3)
	{
		/* Get plugin index that was passed */
		int id = atoi(g_RootMenu.GetArgument(2));
		
		/* Get plugin object */
		CPlugin *plugin = g_PluginSys.GetPluginByOrder(id);

		if (!plugin)
		{
			g_RootMenu.ConsolePrint("[SM] Plugin index %d not found.", id);
			return;
		}

		/* Get plugin name */
		const sm_plugininfo_t *plinfo = plugin->GetPublicInfo();
		const char *plname = IS_STR_FILLED(plinfo->name) ? plinfo->name : plugin->GetFilename();

		ConVarList *pConVarList;
		ConVarList::iterator iter;

		/* If no convar list... */
		if (!plugin->GetProperty("ConVarList", (void **)&pConVarList))
		{
			g_RootMenu.ConsolePrint("[SM] No convars found for: %s", plname);
			return;
		}

		g_RootMenu.ConsolePrint("[SM] Listing %d convars for: %s", pConVarList->size(), plname);
		g_RootMenu.ConsolePrint("  %-32.31s %s", "[Name]", "[Value]");

		/* Iterate convar list and display each one */
		for (iter = pConVarList->begin(); iter != pConVarList->end(); iter++)
		{
			const ConVar *pConVar = (*iter);
			g_RootMenu.ConsolePrint("  %-32.31s %s", pConVar->GetName(), pConVar->GetString()); 
		}

		return;
	}

	/* Display usage of subcommand */
	g_RootMenu.ConsolePrint("[SM] Usage: sm convars <plugin #>");
}

Handle_t ConVarManager::CreateConVar(IPluginContext *pContext, const char *name, const char *defaultVal, const char *description, int flags, bool hasMin, float min, bool hasMax, float max)
{
	ConVar *pConVar = NULL;
	ConVarInfo *pInfo = NULL;
	ConVarList *pConVarList = NULL;
	Handle_t hndl = 0;

	/* Find out if the convar exists already */
	pConVar = icvar->FindVar(name);

	/* If the convar already exists... */
	if (pConVar)
	{
		/* First find out if we already have a handle to it */
		if (sm_trie_retrieve(m_ConVarCache, name, (void **)&pInfo))
		{
			return pInfo->handle;
		} else {
			/* If we don't, then create a new handle from the convar */
			hndl = g_HandleSys.CreateHandle(m_ConVarType, pConVar, NULL, g_pCoreIdent, NULL);

			/* Create and initialize ConVarInfo structure */
			pInfo = new ConVarInfo();
			pInfo->handle = hndl;
			pInfo->sourceMod = false;
			pInfo->changeForward = NULL;
			pInfo->origCallback = pConVar->GetCallback();

			/* Insert struct into caches */
			m_ConVars.push_back(pInfo);
			sm_trie_insert(m_ConVarCache, name, pInfo);

			return hndl;
		}
	}

	// To prevent creating a convar that has the same name as a console command... ugh
	ConCommandBase *pBase = icvar->GetCommands();

	while (pBase)
	{
		if (pBase->IsCommand() && strcmp(pBase->GetName(), name) == 0)
		{
			return BAD_HANDLE;
		}

		pBase = const_cast<ConCommandBase *>(pBase->GetNext());
	}

	/* Since an existing convar (or concmd with the same name) was not found , now we can finally create it */
	pConVar = new ConVar(sm_strdup(name), sm_strdup(defaultVal), flags, sm_strdup(description), hasMin, min, hasMax, max);

	/* Add convar to plugin's list */
	AddConVarToPluginList(pContext, pConVar);

	/* Create a handle from the new convar */
	hndl = g_HandleSys.CreateHandle(m_ConVarType, pConVar, NULL, g_pCoreIdent, NULL);

	/* Create and initialize ConVarInfo structure */
	pInfo = new ConVarInfo();
	pInfo->handle = hndl;
	pInfo->sourceMod = true;
	pInfo->changeForward = NULL;
	pInfo->origCallback = NULL;

	/* Insert struct into caches */
	m_ConVars.push_back(pInfo);
	sm_trie_insert(m_ConVarCache, name, pInfo);

	return hndl;
}

Handle_t ConVarManager::FindConVar(const char *name)
{
	ConVar *pConVar = NULL;
	ConVarInfo *pInfo;
	Handle_t hndl;

	/* Search for convar */
	pConVar = icvar->FindVar(name);

	/* If it doesn't exist, then return an invalid handle */
	if (!pConVar)
	{
		return BAD_HANDLE;
	}

	/* At this point, the convar exists. So, find out if we already have a handle */
	if (sm_trie_retrieve(m_ConVarCache, name, (void **)&pInfo))
	{
		return pInfo->handle;
	}

	/* If we don't have a handle, then create a new one */
	hndl = g_HandleSys.CreateHandle(m_ConVarType, pConVar, NULL, g_pCoreIdent, NULL);

	/* Create and initilize ConVarInfo structure */
	pInfo = new ConVarInfo();
	pInfo->handle = hndl;
	pInfo->sourceMod = false;
	pInfo->changeForward = NULL;
	pInfo->origCallback = pConVar->GetCallback();

	/* Insert struct into our caches */
	m_ConVars.push_back(pInfo);
	sm_trie_insert(m_ConVarCache, name, pInfo);

	return hndl;
}

void ConVarManager::HookConVarChange(ConVar *pConVar, IPluginFunction *pFunction)
{
	ConVarInfo *pInfo;
	IChangeableForward *pForward;

	/* Find the convar in the lookup trie */
	if (sm_trie_retrieve(m_ConVarCache, pConVar->GetName(), (void **)&pInfo))
	{
		/* Get the forward */
		pForward = pInfo->changeForward;

		/* If forward does not exist, create it */
		if (!pForward)
		{
			pForward = g_Forwards.CreateForwardEx(NULL, ET_Ignore, 3, CONVARCHANGE_PARAMS);
			pInfo->changeForward = pForward;

			/* Install our own callback */
			pConVar->InstallChangeCallback(OnConVarChanged);
		}

		/* Add function to forward's list */
		pForward->AddFunction(pFunction);
	}
}

void ConVarManager::UnhookConVarChange(ConVar *pConVar, IPluginFunction *pFunction)
{
	ConVarInfo *pInfo;
	IChangeableForward *pForward;
	IPluginContext *pContext = pFunction->GetParentContext();

	/* Find the convar in the lookup trie */
	if (sm_trie_retrieve(m_ConVarCache, pConVar->GetName(), (void **)&pInfo))
	{
		/* Get the forward */
		pForward = pInfo->changeForward;

		/* If the forward doesn't exist, we can't unhook anything */
		if (!pForward)
		{
			pContext->ThrowNativeError("Convar \"%s\" has no active hook", pConVar->GetName());
			return;
		}

		/* Remove the function from the forward's list */
		if (!pForward->RemoveFunction(pFunction))
		{
			pContext->ThrowNativeError("Invalid hook callback specified for convar \"%s\"", pConVar->GetName());
			return;
		}

		/* If the forward now has 0 functions in it... */
		if (pForward->GetFunctionCount() == 0)
		{
			/* Free this forward */
			g_Forwards.ReleaseForward(pForward);
			pInfo->changeForward = NULL;

			/* Put back the original convar callback */
			pConVar->InstallChangeCallback(pInfo->origCallback);
		}
	}
}

void ConVarManager::AddConVarToPluginList(IPluginContext *pContext, const ConVar *pConVar)
{
	ConVarList *pConVarList;
	ConVarList::iterator iter;
	bool inserted = false;
	const char *orig = pConVar->GetName();

	IPlugin *plugin = g_PluginSys.FindPluginByContext(pContext->GetContext());

	/* Check plugin for an existing convar list */
	if (!plugin->GetProperty("ConVarList", (void **)&pConVarList))
	{
		pConVarList = new ConVarList();
		plugin->SetProperty("ConVarList", pConVarList);
	}

	/* Insert convar into list which is sorted alphabetically */
	for (iter = pConVarList->begin(); iter != pConVarList->end(); iter++)
	{
		if (strcmp(orig, (*iter)->GetName()) < 0)
		{
			pConVarList->insert(iter, pConVar);
			inserted = true;
			break;
		}
	}

	if (!inserted)
	{
		pConVarList->push_back(pConVar);
	}
}

void ConVarManager::OnConVarChanged(ConVar *pConVar, const char *oldValue)
{
	/* If the values are the same, exit early in order to not trigger callbacks */
	if (strcmp(pConVar->GetString(), oldValue) == 0)
	{
		return;
	}

	Trie *pCache = g_ConVarManager.GetConVarCache();
	ConVarInfo *pInfo;

	/* Find the convar in the lookup trie */
	sm_trie_retrieve(pCache, pConVar->GetName(), (void **)&pInfo);

	FnChangeCallback origCallback = pInfo->origCallback;
	IChangeableForward *pForward = pInfo->changeForward;

	/* If there was a change callback installed previously, call it */
	if (origCallback)
	{
		origCallback(pConVar, oldValue);
	}

	/* Now call forwards in plugins that have hooked this */
	pForward->PushCell(pInfo->handle);
	pForward->PushString(oldValue);
	pForward->PushString(pConVar->GetString());
	pForward->Execute(NULL);
}
