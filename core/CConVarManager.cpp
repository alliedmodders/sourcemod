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
#include "sm_stringutil.h"

CConVarManager g_ConVarManager;

CConVarManager::CConVarManager() : m_ConVarType(0)
{
	// Create a convar lookup trie
	m_ConVarCache = sm_trie_create();
}

CConVarManager::~CConVarManager()
{
	List<ConVarInfo *>::iterator i;

	// Destroy our convar lookup trie
	sm_trie_destroy(m_ConVarCache);

	// Destroy all the ConVarInfo structures
	for (i = m_ConVars.begin(); i != m_ConVars.end(); i++)
	{
		delete (*i);
	}

	m_ConVars.clear();
}

void CConVarManager::OnSourceModAllInitialized()
{
	HandleAccess sec;

	// Set up access rights for the 'ConVar' handle type
	sec.access[HandleAccess_Read] = 0;
	sec.access[HandleAccess_Delete] = HANDLE_RESTRICT_IDENTITY | HANDLE_RESTRICT_OWNER;
	sec.access[HandleAccess_Clone] = HANDLE_RESTRICT_IDENTITY | HANDLE_RESTRICT_OWNER;

	// Create the 'ConVar' handle type
	m_ConVarType = g_HandleSys.CreateType("ConVar", this, 0, NULL, &sec, g_pCoreIdent, NULL);

	// Add the 'cvars' option to the 'sm' console command
	g_RootMenu.AddRootConsoleCommand("cvars", "View convars created by a plugin", this);
}

void CConVarManager::OnSourceModShutdown()
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
	g_RootMenu.RemoveRootConsoleCommand("cvars", this);

	// Remove the 'ConVar' handle type
	g_HandleSys.RemoveType(m_ConVarType, g_pCoreIdent);
}

void CConVarManager::OnHandleDestroy(HandleType_t type, void *object)
{
	ConVarInfo *info;
	ConCommandBase *cvar = static_cast<ConCommandBase *>(object);

	// Find convar in lookup trie
	sm_trie_retrieve(m_ConVarCache, cvar->GetName(), reinterpret_cast<void **>(&info));

	// If convar was created by SourceMod plugin...
	if (info->sourceMod)
	{
		// Then unregister it
		g_SMAPI->UnregisterConCmdBase(g_PLAPI, cvar);
	}
}

void CConVarManager::OnRootConsoleCommand(const char *command, unsigned int argcount)
{
	if (argcount >= 3)
	{
		int id = 1;

		// Get plugin index that was passed
		int num = atoi(g_RootMenu.GetArgument(2));
		
		// If invalid plugin index...
		if (num < 1 || num > (int)g_PluginSys.GetPluginCount())
		{
			g_RootMenu.ConsolePrint("[SM] Plugin index not found.");
			return;
		}

		// Get plugin object
		CPlugin *pl = g_PluginSys.GetPluginByOrder(num);

		// Get number of convars created by plugin
		int convarnum = pl->GetConVarCount();

		if (convarnum == 0)
		{
			g_RootMenu.ConsolePrint("[SM] No convars for \"%s\"", pl->GetPublicInfo()->name);
			return;
		}

		g_RootMenu.ConsolePrint("[SM] Displaying convars for \"%s\":", pl->GetPublicInfo()->name);

		// Iterate convar list and display each one
		for (int i = 0; i < convarnum; i++, id++)
		{
			ConVar *cvar = pl->GetConVarByIndex(i);
			g_RootMenu.ConsolePrint("  %02d \"%s\" = \"%s\"", id, cvar->GetName(), cvar->GetString()); 
		}

		return;
	}

	// Display usage of subcommand
	g_RootMenu.ConsolePrint("[SM] Usage: sm cvars <#>");
}

Handle_t CConVarManager::CreateConVar(IPluginContext *pContext, const char *name, const char *defaultVal, const char *helpText, int flags, bool hasMin, float min, bool hasMax, float max)
{
	ConVar *cvar = NULL;
	ConVarInfo *info = NULL;
	Handle_t hndl = 0;

	// Find out if the convar exists already
	cvar = icvar->FindVar(name);

	// If the convar already exists...
	if (cvar != NULL)
	{
		// First check if we already have a handle to it
		if (sm_trie_retrieve(m_ConVarCache, name, reinterpret_cast<void **>(&info)))
		{
			// If we do, then return that handle
			return info->handle;
		} else {
			// If we don't, then create a new handle from the convar
			hndl = g_HandleSys.CreateHandle(m_ConVarType, cvar, NULL, g_pCoreIdent, NULL);

			info = new ConVarInfo;
			info->handle = hndl;
			info->sourceMod = false;
			info->changeForward = NULL;
			info->origCallback = cvar->GetCallback();

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
	cvar = new ConVar(name, defaultVal, flags, helpText, hasMin, min, hasMax, max);

	// Add new convar to plugin's list
	g_PluginSys.GetPluginByCtx(pContext->GetContext())->AddConVar(cvar);

	// Create a handle from the new convar
	hndl = g_HandleSys.CreateHandle(m_ConVarType, cvar, NULL, g_pCoreIdent, NULL);

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

Handle_t CConVarManager::FindConVar(const char *name)
{
	ConVar *cvar = NULL;
	ConVarInfo *info = NULL;
	Handle_t hndl = 0;

	// Search for convar
	cvar = icvar->FindVar(name);

	// If it doesn't exist, then return an invalid handle
	if (cvar == NULL)
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
	hndl = g_HandleSys.CreateHandle(m_ConVarType, cvar, NULL, g_pCoreIdent, NULL);

	info = new ConVarInfo;
	info->handle = hndl;
	info->sourceMod = false;
	info->changeForward = NULL;
	info->origCallback = cvar->GetCallback();

	m_ConVars.push_back(info);

	// Insert the handle into our cache
	sm_trie_insert(m_ConVarCache, name, info);

	return hndl;
}

void CConVarManager::HookConVarChange(IPluginContext *pContext, ConVar *cvar, funcid_t funcid)
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
	UTIL_Format(fwdName, sizeof(fwdName), "ConVar.%s", cvar->GetName());

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
		if (sm_trie_retrieve(m_ConVarCache, cvar->GetName(), reinterpret_cast<void **>(&info)))
		{
			// Set the convar's forward to the newly created one
			info->changeForward = fwd;

			// Set the convar's callback to our static one
			cvar->InstallChangeCallback(OnConVarChanged);
		}
	}

	// Add the function to the forward's list
	fwd->AddFunction(func);
}

void CConVarManager::UnhookConVarChange(IPluginContext *pContext, ConVar *cvar, funcid_t funcid)
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
	if (sm_trie_retrieve(m_ConVarCache, cvar->GetName(), reinterpret_cast<void **>(&info)))
	{
		// Get the forward
		fwd = info->changeForward;

		// If the forward doesn't exist, we can't unhook anything
		if (fwd == NULL)
		{
			pContext->ThrowNativeError("Convar \"%s\" has no active hook", cvar->GetName());
			return;
		}

		// Remove the function from the forward's list
		if (!fwd->RemoveFunction(func))
		{
			pContext->ThrowNativeError("Invalid hook callback specified for convar \"%s\"", cvar->GetName());
			return;
		}

		// If the forward now has 0 functions in it...
		if (fwd->GetFunctionCount() == 0)
		{
			// Free this forward
			g_Forwards.ReleaseForward(fwd);
			info->changeForward = NULL;

			// Put the back the original convar callback
			cvar->InstallChangeCallback(info->origCallback);
		}
	}
}

void CConVarManager::OnConVarChanged(ConVar *cvar, const char *oldValue)
{
	Trie *cache = g_ConVarManager.GetConVarCache();
	ConVarInfo *info;

	// Find the convar in the lookup trie
	sm_trie_retrieve(cache, cvar->GetName(), reinterpret_cast<void **>(&info));

	FnChangeCallback origCallback = info->origCallback;
	IChangeableForward *fwd = info->changeForward;

	// If there was a change callback installed previously, call it
	if (origCallback)
	{
		origCallback(cvar, oldValue);
	}

	// Now call forwards in plugins that have hooked this
	fwd->PushCell(info->handle);
	fwd->PushString(cvar->GetString());
	fwd->PushString(oldValue);
	fwd->Execute(NULL);
}
