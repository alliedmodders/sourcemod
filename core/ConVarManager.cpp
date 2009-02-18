/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
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

#include "ConVarManager.h"
#include "HalfLife2.h"
#include "PluginSys.h"
#include "ForwardSys.h"
#include "HandleSys.h"
#include "sm_srvcmds.h"
#include "sm_stringutil.h"
#include <sh_vector.h>
#include <sm_trie_tpl.h>

ConVarManager g_ConVarManager;

#if SOURCE_ENGINE <= SE_DARKMESSIAH
#define CallGlobalChangeCallbacks	CallGlobalChangeCallback
#endif

#if SOURCE_ENGINE >= SE_ORANGEBOX
SH_DECL_HOOK3_void(ICvar, CallGlobalChangeCallbacks, SH_NOATTRIB, false, ConVar *, const char *, float);
#else
SH_DECL_HOOK2_void(ICvar, CallGlobalChangeCallbacks, SH_NOATTRIB, false, ConVar *, const char *);
#endif

#if SOURCE_ENGINE != SE_DARKMESSIAH
SH_DECL_HOOK5_void(IServerGameDLL, OnQueryCvarValueFinished, SH_NOATTRIB, 0, QueryCvarCookie_t, edict_t *, EQueryCvarValueStatus, const char *, const char *);
SH_DECL_HOOK5_void(IServerPluginCallbacks, OnQueryCvarValueFinished, SH_NOATTRIB, 0, QueryCvarCookie_t, edict_t *, EQueryCvarValueStatus, const char *, const char *);
#endif

const ParamType CONVARCHANGE_PARAMS[] = {Param_Cell, Param_String, Param_String};
typedef List<const ConVar *> ConVarList;
KTrie<ConVarInfo *> convar_cache;

ConVarManager::ConVarManager() : m_ConVarType(0), m_bIsDLLQueryHooked(false), m_bIsVSPQueryHooked(false)
{
}

ConVarManager::~ConVarManager()
{
}

void ConVarManager::OnSourceModStartup(bool late)
{
	HandleAccess sec;

	/* Set up access rights for the 'ConVar' handle type */
	sec.access[HandleAccess_Read] = 0;
	sec.access[HandleAccess_Delete] = HANDLE_RESTRICT_IDENTITY | HANDLE_RESTRICT_OWNER;
	sec.access[HandleAccess_Clone] = HANDLE_RESTRICT_IDENTITY | HANDLE_RESTRICT_OWNER;

	/* Create the 'ConVar' handle type */
	m_ConVarType = g_HandleSys.CreateType("ConVar", this, 0, NULL, &sec, g_pCoreIdent, NULL);
}

void ConVarManager::OnSourceModAllInitialized()
{
	/**
	 * Episode 2 has this function by default, but the older versions do not.
	 */
#if SOURCE_ENGINE == SE_EPISODEONE
	if (g_SMAPI->GetGameDLLVersion() >= 6)
	{
		SH_ADD_HOOK_MEMFUNC(IServerGameDLL, OnQueryCvarValueFinished, gamedll, this, &ConVarManager::OnQueryCvarValueFinished, false);
		m_bIsDLLQueryHooked = true;
	}
#endif

	SH_ADD_HOOK_STATICFUNC(ICvar, CallGlobalChangeCallbacks, icvar, OnConVarChanged, false);

	/* Add the 'convars' option to the 'sm' console command */
	g_RootMenu.AddRootConsoleCommand("cvars", "View convars created by a plugin", this);
}

void ConVarManager::OnSourceModShutdown()
{
	List<ConVarInfo *>::iterator iter = m_ConVars.begin();
	HandleSecurity sec(NULL, g_pCoreIdent);

	/* Iterate list of ConVarInfo structures, remove every one of them */
	while (iter != m_ConVars.end())
	{
		ConVarInfo *pInfo = (*iter);

		iter = m_ConVars.erase(iter);

		g_HandleSys.FreeHandle(pInfo->handle, &sec);
		if (pInfo->pChangeForward != NULL)
		{
			g_Forwards.ReleaseForward(pInfo->pChangeForward);
		}
		if (pInfo->sourceMod)
		{
			/* If we created it, we won't be tracking it, therefore it is 
			 * safe to remove everything in one go.
			 */
			META_UNREGCVAR(pInfo->pVar);
			delete [] pInfo->pVar->GetName();
			delete [] pInfo->pVar->GetHelpText();
			delete [] pInfo->pVar->GetDefault();
			delete pInfo->pVar;
		}
		else
		{
			/* If we didn't create it, we might be tracking it.  Also, 
			 * it could be unreadable.
			 */
			UntrackConCommandBase(pInfo->pVar, this);
		}

		/* It's not safe to read the name here, so we simply delete the 
		 * the info struct and clear the lookup cache at the end.
		 */
		delete pInfo;
	}
	convar_cache.clear();

#if SOURCE_ENGINE != SE_DARKMESSIAH
	/* Unhook things */
	if (m_bIsDLLQueryHooked)
	{
		SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, OnQueryCvarValueFinished, gamedll, this, &ConVarManager::OnQueryCvarValueFinished, false);
		m_bIsDLLQueryHooked = false;
	}
	else if (m_bIsVSPQueryHooked)
	{
		SH_REMOVE_HOOK_MEMFUNC(IServerPluginCallbacks, OnQueryCvarValueFinished, vsp_interface, this, &ConVarManager::OnQueryCvarValueFinished, false);
		m_bIsVSPQueryHooked = false;
	}
#endif

	SH_REMOVE_HOOK_STATICFUNC(ICvar, CallGlobalChangeCallbacks, icvar, OnConVarChanged, false);

	/* Remove the 'convars' option from the 'sm' console command */
	g_RootMenu.RemoveRootConsoleCommand("cvars", this);

	/* Remove the 'ConVar' handle type */
	g_HandleSys.RemoveType(m_ConVarType, g_pCoreIdent);
}

/**
 * Orange Box will never use this.
 */
void ConVarManager::OnSourceModVSPReceived()
{
	/**
	 * Don't bother if the DLL is already hooked.
	 */
	if (m_bIsDLLQueryHooked)
	{
		return;
	}

	/* For later MM:S versions, use the updated API, since it's cleaner. */
#if defined METAMOD_PLAPI_VERSION
	int engine = g_SMAPI->GetSourceEngineBuild();
	if (engine == SOURCE_ENGINE_ORIGINAL || vsp_version < 2)
	{
		return;
	}
#else
	if (g_HL2.IsOriginalEngine() || vsp_version < 2)
	{
		return;
	}
#endif

#if SOURCE_ENGINE != SE_DARKMESSIAH
	SH_ADD_HOOK_MEMFUNC(IServerPluginCallbacks, OnQueryCvarValueFinished, vsp_interface, this, &ConVarManager::OnQueryCvarValueFinished, false);
	m_bIsVSPQueryHooked = true;
#endif
}

bool convar_cache_lookup(const char *name, ConVarInfo **pVar)
{
	ConVarInfo **pLookup = convar_cache.retrieve(name);
	if (pLookup != NULL)
	{
		*pVar = *pLookup;
		return true;
	}
	else
	{
		return false;
	}
}

void ConVarManager::OnUnlinkConCommandBase(ConCommandBase *pBase, const char *name, bool is_read_safe)
{
	/* Only check convars that have not been created by SourceMod's core */
	ConVarInfo *pInfo;
	if (!convar_cache_lookup(name, &pInfo))
	{
		return;
	}

	HandleSecurity sec(NULL, g_pCoreIdent);

	/* Remove it from our cache */
	m_ConVars.remove(pInfo);
	convar_cache.remove(name);

	/* Now make sure no plugins are referring to this pointer */
	IPluginIterator *pl_iter = g_PluginSys.GetPluginIterator();
	while (pl_iter->MorePlugins())
	{
		IPlugin *pl = pl_iter->GetPlugin();

		ConVarList *pConVarList;
		if (pl->GetProperty("ConVarList", (void **)&pConVarList, true) 
			&& pConVarList != NULL)
		{
			pConVarList->remove(pInfo->pVar);
		}

		pl_iter->NextPlugin();
	}

	/* Free resources */
	g_HandleSys.FreeHandle(pInfo->handle, &sec);
	delete pInfo;
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
}

bool ConVarManager::GetHandleApproxSize(HandleType_t type, void *object, unsigned int *pSize)
{
	*pSize = sizeof(ConVar) + sizeof(ConVarInfo);
	return true;
}

void ConVarManager::OnRootConsoleCommand(const char *cmdname, const CCommand &command)
{
	int argcount = command.ArgC();
	if (argcount >= 3)
	{
		/* Get plugin index that was passed */
		const char *arg = command.Arg(2);
		
		/* Get plugin object */
		CPlugin *plugin = g_PluginSys.FindPluginByConsoleArg(arg);

		if (!plugin)
		{
			g_RootMenu.ConsolePrint("[SM] Plugin \"%s\" was not found.", arg);
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
	Handle_t hndl = 0;

	/* Find out if the convar exists already */
	pConVar = icvar->FindVar(name);

	/* If the convar already exists... */
	if (pConVar)
	{
		/* Add convar to plugin's list */
		AddConVarToPluginList(pContext, pConVar);

		/* First find out if we already have a handle to it */
		if (convar_cache_lookup(name, &pInfo))
		{
			return pInfo->handle;
		}
		else
		{
			/* Create and initialize ConVarInfo structure */
			pInfo = new ConVarInfo();
			pInfo->sourceMod = false;
			pInfo->pChangeForward = NULL;
			pInfo->pVar = pConVar;

			/* If we don't, then create a new handle from the convar */
			hndl = g_HandleSys.CreateHandle(m_ConVarType, pInfo, NULL, g_pCoreIdent, NULL);
			if (hndl == BAD_HANDLE)
			{
				delete pInfo;
				return BAD_HANDLE;
			}

			pInfo->handle = hndl;

			/* Insert struct into caches */
			m_ConVars.push_back(pInfo);
			convar_cache.insert(name, pInfo);
			TrackConCommandBase(pConVar, this);

			return hndl;
		}
	}

	/* Prevent creating a convar that has the same name as a console command */
	if (FindCommand(name))
	{
		return BAD_HANDLE;
	}

	/* Create and initialize ConVarInfo structure */
	pInfo = new ConVarInfo();
	pInfo->handle = hndl;
	pInfo->sourceMod = true;
	pInfo->pChangeForward = NULL;

	/* Create a handle from the new convar */
	hndl = g_HandleSys.CreateHandle(m_ConVarType, pInfo, NULL, g_pCoreIdent, NULL);
	if (hndl == BAD_HANDLE)
	{
		delete pInfo;
		return BAD_HANDLE;
	}

	pInfo->handle = hndl;

	/* Since an existing convar (or concmd with the same name) was not found , now we can finally create it */
	pConVar = new ConVar(sm_strdup(name), sm_strdup(defaultVal), flags, sm_strdup(description), hasMin, min, hasMax, max);
	pInfo->pVar = pConVar;

	/* Add convar to plugin's list */
	AddConVarToPluginList(pContext, pConVar);

	/* Insert struct into caches */
	m_ConVars.push_back(pInfo);
	convar_cache.insert(name, pInfo);

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
	if (convar_cache_lookup(name, &pInfo))
	{
		return pInfo->handle;
	}

	/* Create and initialize ConVarInfo structure */
	pInfo = new ConVarInfo();
	pInfo->sourceMod = false;
	pInfo->pChangeForward = NULL;
	pInfo->pVar = pConVar;

	/* If we don't have a handle, then create a new one */
	hndl = g_HandleSys.CreateHandle(m_ConVarType, pInfo, NULL, g_pCoreIdent, NULL);
	if (hndl == BAD_HANDLE)
	{
		delete pInfo;
		return BAD_HANDLE;
	}

	pInfo->handle = hndl;

	/* Insert struct into our caches */
	m_ConVars.push_back(pInfo);
	convar_cache.insert(name, pInfo);
	TrackConCommandBase(pConVar, this);

	return hndl;
}

void ConVarManager::AddConVarChangeListener(const char *name, IConVarChangeListener *pListener)
{
	ConVarInfo *pInfo;

	if (FindConVar(name) == BAD_HANDLE)
	{
		return;
	}

	/* Find the convar in the lookup trie */
	if (convar_cache_lookup(name, &pInfo))
	{
		pInfo->changeListeners.push_back(pListener);
	}
}

void ConVarManager::RemoveConVarChangeListener(const char *name, IConVarChangeListener *pListener)
{
	ConVarInfo *pInfo;

	/* Find the convar in the lookup trie */
	if (convar_cache_lookup(name, &pInfo))
	{
		pInfo->changeListeners.remove(pListener);
	}
}

void ConVarManager::HookConVarChange(ConVar *pConVar, IPluginFunction *pFunction)
{
	ConVarInfo *pInfo;
	IChangeableForward *pForward;

	/* Find the convar in the lookup trie */
	if (convar_cache_lookup(pConVar->GetName(), &pInfo))
	{
		/* Get the forward */
		pForward = pInfo->pChangeForward;

		/* If forward does not exist, create it */
		if (!pForward)
		{
			pForward = g_Forwards.CreateForwardEx(NULL, ET_Ignore, 3, CONVARCHANGE_PARAMS);
			pInfo->pChangeForward = pForward;
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
	if (convar_cache_lookup(pConVar->GetName(), &pInfo))
	{
		/* Get the forward */
		pForward = pInfo->pChangeForward;

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
			pInfo->pChangeForward = NULL;
		}
	}
}

QueryCvarCookie_t ConVarManager::QueryClientConVar(edict_t *pPlayer, const char *name, IPluginFunction *pCallback, Handle_t hndl)
{
	QueryCvarCookie_t cookie = 0;

#if SOURCE_ENGINE != SE_DARKMESSIAH
	/* Call StartQueryCvarValue() in either the IVEngineServer or IServerPluginHelpers depending on situation */
	if (m_bIsDLLQueryHooked)
	{
		cookie = engine->StartQueryCvarValue(pPlayer, name);	
	}
	else if (m_bIsVSPQueryHooked)
	{
		cookie = serverpluginhelpers->StartQueryCvarValue(pPlayer, name);
	}
	else
	{
		return InvalidQueryCvarCookie;
	}

	ConVarQuery query = {cookie, pCallback, hndl};
	m_ConVarQueries.push_back(query);
#endif

	return cookie;
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
	else if (pConVarList->find(pConVar) != pConVarList->end())
	{
		/* If convar is already in list, then don't add it */
		return;
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

#if SOURCE_ENGINE >= SE_ORANGEBOX
void ConVarManager::OnConVarChanged(ConVar *pConVar, const char *oldValue, float flOldValue)
#else
void ConVarManager::OnConVarChanged(ConVar *pConVar, const char *oldValue)
#endif
{
	/* If the values are the same, exit early in order to not trigger callbacks */
	if (strcmp(pConVar->GetString(), oldValue) == 0)
	{
		return;
	}

	ConVarInfo *pInfo;

	/* Find the convar in the lookup trie */
	if (!convar_cache_lookup(pConVar->GetName(), &pInfo))
	{
		return;
	}

	IChangeableForward *pForward = pInfo->pChangeForward;

	if (pInfo->changeListeners.size() != 0)
	{
		for (List<IConVarChangeListener *>::iterator i = pInfo->changeListeners.begin();
			 i != pInfo->changeListeners.end();
			 i++)
		{
#if SOURCE_ENGINE >= SE_ORANGEBOX
			(*i)->OnConVarChanged(pConVar, oldValue, flOldValue);
#else
			(*i)->OnConVarChanged(pConVar, oldValue, atof(oldValue));
#endif
		}
	}

	if (pForward != NULL)
	{
		/* Now call forwards in plugins that have hooked this */
		pForward->PushCell(pInfo->handle);
		pForward->PushString(oldValue);
		pForward->PushString(pConVar->GetString());
		pForward->Execute(NULL);
	}
}

bool ConVarManager::IsQueryingSupported()
{
	return (m_bIsDLLQueryHooked || m_bIsVSPQueryHooked);
}

#if SOURCE_ENGINE != SE_DARKMESSIAH
void ConVarManager::OnQueryCvarValueFinished(QueryCvarCookie_t cookie, edict_t *pPlayer, EQueryCvarValueStatus result, const char *cvarName, const char *cvarValue)
{
	IPluginFunction *pCallback = NULL;
	cell_t value = 0;
	List<ConVarQuery>::iterator iter;

	for (iter = m_ConVarQueries.begin(); iter != m_ConVarQueries.end(); iter++)
	{
		ConVarQuery &query = (*iter);
		if (query.cookie == cookie)
		{
			pCallback = query.pCallback;
			value = query.value;
			break;
		}
	}

	if (pCallback)
	{
		cell_t ret;

		pCallback->PushCell(cookie);
		pCallback->PushCell(IndexOfEdict(pPlayer));
		pCallback->PushCell(result);
		pCallback->PushString(cvarName);

		if (result == eQueryCvarValueStatus_ValueIntact)
		{
			pCallback->PushString(cvarValue);
		}
		else
		{
			pCallback->PushString("\0");
		}

		pCallback->PushCell(value);
		pCallback->Execute(&ret);

		m_ConVarQueries.erase(iter);
	}
}
#endif

HandleError ConVarManager::ReadConVarHandle(Handle_t hndl, ConVar **pVar)
{
	ConVarInfo *pInfo;
	HandleError error;
	
	if ((error = g_HandleSys.ReadHandle(hndl, m_ConVarType, NULL, (void **)&pInfo)) != HandleError_None)
	{
		return error;
	}

	if (pVar)
	{
		*pVar = pInfo->pVar;
	}

	return error;
}
