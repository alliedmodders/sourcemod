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

ConVarManager::ConVarManager() : m_ConVarType(0)
{
	// Create a convar lookup trie
	m_ConVarCache = sm_trie_create();
}

ConVarManager::~ConVarManager()
{
	List<ConVarInfo *>::iterator iter;

	// Destroy our convar lookup trie
	sm_trie_destroy(m_ConVarCache);

	// Destroy all the ConVarInfo structures
	for (iter = m_ConVars.begin(); iter != m_ConVars.end(); iter++)
	{
		delete (*iter);
	}

	m_ConVars.clear();
}

void ConVarManager::OnSourceModAllInitialized()
{
	HandleAccess sec;

	// Set up access rights for the 'ConVar' handle type
	sec.access[HandleAccess_Read] = 0;
	sec.access[HandleAccess_Delete] = HANDLE_RESTRICT_IDENTITY | HANDLE_RESTRICT_OWNER;
	sec.access[HandleAccess_Clone] = HANDLE_RESTRICT_IDENTITY | HANDLE_RESTRICT_OWNER;

	// Create the 'ConVar' handle type
	m_ConVarType = g_HandleSys.CreateType("ConVar", this, 0, NULL, &sec, g_pCoreIdent, NULL);

	// Add the 'cvars' option to the 'sm' console command
	g_RootMenu.AddRootConsoleCommand("convars", "View convars created by a plugin", this);
}

void ConVarManager::OnSourceModShutdown()
{
	IChangeableForward *fwd;
	List<ConVarInfo *>::iterator i;

	// Iterate list of ConVarInfo structures
	for (i = m_ConVars.begin(); i != m_ConVars.end(); i++)
	{
		fwd = (*i)->changeForward;

		// Free any convar-change forwards that still exist
		if (fwd)
		{
			g_Forwards.ReleaseForward(fwd);
		}
	}

	// Remove the 'cvars' option from the 'sm' console command
	g_RootMenu.RemoveRootConsoleCommand("convars", this);

	// Remove the 'ConVar' handle type
	g_HandleSys.RemoveType(m_ConVarType, g_pCoreIdent);
}

void ConVarManager::OnPluginDestroyed(IPlugin *plugin)
{
	CVector<ConVar *> *cvarList;

	// If plugin has a convar list, free its memory
	if (plugin->GetProperty("ConVarList", reinterpret_cast<void **>(&cvarList), true))
	{
		delete cvarList;
	}
}

void ConVarManager::OnHandleDestroy(HandleType_t type, void *object)
{
	ConVarInfo *info;
	ConVar *pConVar = static_cast<ConVar *>(object);

	// Find convar in lookup trie
	sm_trie_retrieve(m_ConVarCache, pConVar->GetName(), reinterpret_cast<void **>(&info));

	// If convar was created by SourceMod plugin...
	if (info->sourceMod)
	{
		// Delete string allocations
		delete [] pConVar->GetName(); 
		delete [] pConVar->GetDefault();
		delete [] pConVar->GetHelpText();

		// Then unlink it from SourceMM
		g_SMAPI->UnregisterConCmdBase(g_PLAPI, pConVar);
	}
}

void ConVarManager::OnRootConsoleCommand(const char *command, unsigned int argcount)
{
	if (argcount >= 3)
	{
		int id = 1;

		// Get plugin index that was passed
		int num = atoi(g_RootMenu.GetArgument(2));
		
		// If invalid plugin index...
		if (num < 1 || num > (int)g_PluginSys.GetPluginCount())
		{
			g_RootMenu.ConsolePrint("[SM] Plugin index %d not found.", num);
			return;
		}

		// Get plugin object
		CPlugin *pl = g_PluginSys.GetPluginByOrder(num);

		CVector<ConVar *> *cvarList = NULL;

		// Get convar list from 'ConVar' property
		pl->GetProperty("ConVar", reinterpret_cast<void **>(&cvarList));

		// If no cvar list...
		if (cvarList == NULL)
		{
			g_RootMenu.ConsolePrint("[SM] No convars for \"%s\"", pl->GetPublicInfo()->name);
			return;
		}

		g_RootMenu.ConsolePrint("[SM] Displaying convars for \"%s\"", pl->GetPublicInfo()->name);

		// Iterate convar list and display each one
		for (size_t i = 0; i < cvarList->size(); i++, id++)
		{
			ConVar *pConVar = (*cvarList)[i];
			g_RootMenu.ConsolePrint("  %02d \"%s\" = \"%s\"", id, pConVar->GetName(), pConVar->GetString()); 
		}

		return;
	}

	// Display usage of subcommand
	g_RootMenu.ConsolePrint("[SM] Usage: sm convars <plugin #>");
}

Handle_t ConVarManager::CreateConVar(IPluginContext *pContext, const char *name, const char *defaultVal, const char *helpText, int flags, bool hasMin, float min, bool hasMax, float max)
{
	ConVar *pConVar = NULL;
	ConVarInfo *info = NULL;
	CVector<ConVar *> *cvarList = NULL;
	Handle_t hndl = 0;

	// Find out if the convar exists already
	pConVar = icvar->FindVar(name);

	// If the convar already exists...
	if (pConVar != NULL)
	{
		// First check if we already have a handle to it
		if (sm_trie_retrieve(m_ConVarCache, name, reinterpret_cast<void **>(&info)))
		{
			// If we do, then return that handle
			return info->handle;
		} else {
			// If we don't, then create a new handle from the convar
			hndl = g_HandleSys.CreateHandle(m_ConVarType, pConVar, NULL, g_pCoreIdent, NULL);

			info = new ConVarInfo;
			info->handle = hndl;
			info->sourceMod = false;
			info->changeForward = NULL;
			info->origCallback = pConVar->GetCallback();

			m_ConVars.push_back(info);

			return hndl;
		}
	}

	// To prevent creating a convar that has the same name as a console command... ugh
	ConCommandBase *pBase = icvar->GetCommands();

	while (pBase)
	{
		if (strcmp(pBase->GetName(), name) == 0)
		{
			return pContext->ThrowNativeError("Convar \"%s\" was not created. A console command with the same name already exists.", name);
		}

		pBase = const_cast<ConCommandBase *>(pBase->GetNext());
	}

	// Since we didn't find an existing convar (or concmd with the same name), now we can finally create it!
	pConVar = new ConVar(sm_strdup(name), sm_strdup(defaultVal), flags, sm_strdup(helpText), hasMin, min, hasMax, max);

	// Find plugin creating convar
	IPlugin *pl = g_PluginSys.FindPluginByContext(pContext->GetContext());

	// Get convar list from 'ConVar' property of plugin
	pl->GetProperty("ConVarList", reinterpret_cast<void **>(&cvarList));

	// If 'ConVar' property doesn't exist...
	if (cvarList == NULL)
	{
		// Then create it
		cvarList = new CVector<ConVar *>;
		pl->SetProperty("ConVarList", cvarList);
	}

	// Add new convar to plugin's list
	cvarList->push_back(pConVar);

	// Create a handle from the new convar
	hndl = g_HandleSys.CreateHandle(m_ConVarType, pConVar, NULL, g_pCoreIdent, NULL);

	info = new ConVarInfo;
	info->handle = hndl;
	info->sourceMod = true;
	info->changeForward = NULL;
	info->origCallback = NULL;

	m_ConVars.push_back(info);

	// Insert the handle into our lookup trie
	sm_trie_insert(m_ConVarCache, name, info);

	return hndl;
}

Handle_t ConVarManager::FindConVar(const char *name)
{
	ConVar *pConVar = NULL;
	ConVarInfo *info = NULL;
	Handle_t hndl = 0;

	// Search for convar
	pConVar = icvar->FindVar(name);

	// If it doesn't exist, then return an invalid handle
	if (pConVar == NULL)
	{
		return BAD_HANDLE;
	}

	// At this point, convar exists, so find out if we already have a handle for it
	if (sm_trie_retrieve(m_ConVarCache, name, reinterpret_cast<void **>(&info)))
	{
		// If we do, then return that handle
		return info->handle;
	}

	// If we don't, then create a new handle from the convar
	hndl = g_HandleSys.CreateHandle(m_ConVarType, pConVar, NULL, g_pCoreIdent, NULL);

	info = new ConVarInfo;
	info->handle = hndl;
	info->sourceMod = false;
	info->changeForward = NULL;
	info->origCallback = pConVar->GetCallback();

	m_ConVars.push_back(info);

	// Insert the handle into our cache
	sm_trie_insert(m_ConVarCache, name, info);

	return hndl;
}

void ConVarManager::HookConVarChange(IPluginContext *pContext, ConVar *pConVar, funcid_t funcid)
{
	IPluginFunction *func = pContext->GetFunctionById(funcid);
	IChangeableForward *fwd = NULL;
	char fwdName[64];
	ConVarInfo *info = NULL;

	// This shouldn't happen...
	if (func == NULL)
	{
		pContext->ThrowNativeError("Invalid function specified");
		return;
	}

	// Create a forward name
	UTIL_Format(fwdName, sizeof(fwdName), "ConVar.%s", pConVar->GetName());

	// First find out if the forward already exists
	g_Forwards.FindForward(fwdName, &fwd);

	// If the forward doesn't exist...
	if (fwd == NULL)
	{
		// This is the forward's parameter type list
		ParamType p[] = {Param_Cell, Param_String, Param_String};

		// Create the forward
		fwd = g_Forwards.CreateForwardEx(fwdName, ET_Ignore, 3, p);

		// Find the convar in the lookup trie
		if (sm_trie_retrieve(m_ConVarCache, pConVar->GetName(), reinterpret_cast<void **>(&info)))
		{
			// Set the convar's forward to the newly created one
			info->changeForward = fwd;

			// Set the convar's callback to our static one
			pConVar->InstallChangeCallback(OnConVarChanged);
		}
	}

	// Add the function to the forward's list
	fwd->AddFunction(func);
}

void ConVarManager::UnhookConVarChange(IPluginContext *pContext, ConVar *pConVar, funcid_t funcid)
{
	IPluginFunction *func = pContext->GetFunctionById(funcid);
	IChangeableForward *fwd = NULL;
	ConVarInfo *info = NULL;

	// This shouldn't happen...
	if (func == NULL)
	{
		pContext->ThrowNativeError("Invalid function specified");
		return;
	}

	// Find the convar in the lookup trie
	if (sm_trie_retrieve(m_ConVarCache, pConVar->GetName(), reinterpret_cast<void **>(&info)))
	{
		// Get the forward
		fwd = info->changeForward;

		// If the forward doesn't exist, we can't unhook anything
		if (fwd == NULL)
		{
			pContext->ThrowNativeError("Convar \"%s\" has no active hook", pConVar->GetName());
			return;
		}

		// Remove the function from the forward's list
		if (!fwd->RemoveFunction(func))
		{
			pContext->ThrowNativeError("Invalid hook callback specified for convar \"%s\"", pConVar->GetName());
			return;
		}

		// If the forward now has 0 functions in it...
		if (fwd->GetFunctionCount() == 0)
		{
			// Free this forward
			g_Forwards.ReleaseForward(fwd);
			info->changeForward = NULL;

			// Put the back the original convar callback
			pConVar->InstallChangeCallback(info->origCallback);
		}
	}
}

void ConVarManager::OnConVarChanged(ConVar *pConVar, const char *oldValue)
{
	// If the values are the same...
	if (strcmp(pConVar->GetString(), oldValue) == 0)
	{
		// Exit early in order to not trigger callbacks
		return;
	}

	Trie *cache = g_ConVarManager.GetConVarCache();
	ConVarInfo *info;

	// Find the convar in the lookup trie
	sm_trie_retrieve(cache, pConVar->GetName(), reinterpret_cast<void **>(&info));

	FnChangeCallback origCallback = info->origCallback;
	IChangeableForward *fwd = info->changeForward;

	// If there was a change callback installed previously, call it
	if (origCallback)
	{
		origCallback(pConVar, oldValue);
	}

	// Now call forwards in plugins that have hooked this
	fwd->PushCell(info->handle);
	fwd->PushString(pConVar->GetString());
	fwd->PushString(oldValue);
	fwd->Execute(NULL);
}
