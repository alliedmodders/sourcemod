/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2009 AlliedModders LLC.  All rights reserved.
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
 */

#include "ConVarManager.h"
#include "HalfLife2.h"
#include "sm_stringutil.h"
#include <sh_vector.h>
#include <sm_namehashset.h>
#include "logic_bridge.h"
#include "sourcemod.h"
#include "provider.h"
#include <bridge/include/IScriptManager.h>

ConVarManager g_ConVarManager;

const ParamType CONVARCHANGE_PARAMS[] = {Param_Cell, Param_String, Param_String};
typedef List<const ConVar *> ConVarList;
NameHashSet<ConVarInfo *, ConVarInfo::ConVarPolicy> convar_cache;

enum {
	eQueryCvarValueStatus_Cancelled = -1,
};

class ConVarReentrancyGuard
{
	ConVar *cvar;
	ConVarReentrancyGuard *up;
public:
	static ConVarReentrancyGuard *chain;

	ConVarReentrancyGuard(ConVar *cvar)
		: cvar(cvar), up(chain)
	{
		chain = this;
	}

	~ConVarReentrancyGuard()
	{
		assert(chain == this);
		chain = up;
	}

	static bool IsCvarInChain(ConVar *cvar)
	{
		ConVarReentrancyGuard *guard = chain;
		while (guard != NULL)
		{
			if (guard->cvar == cvar)
				return true;
			guard = guard->up;
		}
		return false;
	}
};

ConVarReentrancyGuard *ConVarReentrancyGuard::chain = NULL;

ConVarManager::ConVarManager() : m_ConVarType(0)
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
	m_ConVarType = handlesys->CreateType("ConVar", this, 0, NULL, &sec, g_pCoreIdent, NULL);
}

void ConVarManager::OnSourceModAllInitialized()
{
	g_Players.AddClientListener(this);

	scripts->AddPluginsListener(this);

	/* Add the 'convars' option to the 'sm' console command */
	rootmenu->AddRootConsoleCommand3("cvars", "View convars created by a plugin", this);
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

		handlesys->FreeHandle(pInfo->handle, &sec);
		if (pInfo->pChangeForward != NULL)
		{
			forwardsys->ReleaseForward(pInfo->pChangeForward);
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

	g_Players.RemoveClientListener(this);

	/* Remove the 'convars' option from the 'sm' console command */
	rootmenu->RemoveRootConsoleCommand("cvars", this);

	scripts->RemovePluginsListener(this);

	/* Remove the 'ConVar' handle type */
	handlesys->RemoveType(m_ConVarType, g_pCoreIdent);
}

bool convar_cache_lookup(const char *name, ConVarInfo **pVar)
{
	return convar_cache.retrieve(name, pVar);
}

void ConVarManager::OnUnlinkConCommandBase(ConCommandBase *pBase, const char *name)
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
	IPluginIterator *pl_iter = scripts->GetPluginIterator();
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
	handlesys->FreeHandle(pInfo->handle, &sec);
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

	/* Clear any references to this plugin as the convar creator */
	for (List<ConVarInfo *>::iterator iter = m_ConVars.begin(); iter != m_ConVars.end(); ++iter)
	{
		ConVarInfo *pInfo = (*iter);

		if (pInfo->pPlugin == plugin)
		{
			pInfo->pPlugin = nullptr;
		}
	}

	const IPluginRuntime * pRuntime = plugin->GetRuntime();

	/* Remove convar queries for this plugin that haven't returned results yet */
	for (List<ConVarQuery>::iterator iter = m_ConVarQueries.begin(); iter != m_ConVarQueries.end();)
	{
		ConVarQuery &query = (*iter);
		if (query.pCallback->GetParentRuntime() == pRuntime)
		{
			iter = m_ConVarQueries.erase(iter);
			continue;
		}

		++iter;
	}
}

void ConVarManager::OnClientDisconnected(int client)
{
	/* Remove convar queries for this client that haven't returned results yet */
	for (List<ConVarQuery>::iterator iter = m_ConVarQueries.begin(); iter != m_ConVarQueries.end();)
	{
		ConVarQuery &query = (*iter);
		if (query.client == client)
		{
			IPluginFunction *pCallback = query.pCallback;
			if (pCallback)
			{
				cell_t ret;

				pCallback->PushCell(query.cookie);
				pCallback->PushCell(client);
				pCallback->PushCell(eQueryCvarValueStatus_Cancelled);
				pCallback->PushString("");
				pCallback->PushString("");
				pCallback->PushCell(query.value);
				pCallback->Execute(&ret);
			}
			iter = m_ConVarQueries.erase(iter);
			continue;
		}

		++iter;
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

void ConVarManager::OnRootConsoleCommand(const char *cmdname, const ICommandArgs *command)
{
	int argcount = command->ArgC();
	if (argcount >= 3)
	{
		bool wantReset = false;
		
		/* Get plugin index that was passed */
		const char *arg = command->Arg(2);
		if (argcount >= 4 && strcmp(arg, "reset") == 0)
		{
			wantReset = true;
			arg = command->Arg(3);
		}
		
		/* Get plugin object */
		IPlugin *plugin = scripts->FindPluginByConsoleArg(arg);

		if (!plugin)
		{
			UTIL_ConsolePrint("[SM] Plugin \"%s\" was not found.", arg);
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
			UTIL_ConsolePrint("[SM] No convars found for: %s", plname);
			return;
		}

		if (!wantReset)
		{
			UTIL_ConsolePrint("[SM] Listing %d convars for: %s", pConVarList->size(), plname);
			UTIL_ConsolePrint("  %-32.31s %s", "[Name]", "[Value]");
		}
		
		/* Iterate convar list and display/reset each one */
		for (iter = pConVarList->begin(); iter != pConVarList->end(); iter++)
		{
			/*const */ConVar *pConVar = const_cast<ConVar *>(*iter);
			if (!wantReset)
			{
				UTIL_ConsolePrint("  %-32.31s %s", pConVar->GetName(), pConVar->GetString()); 
			} else {
				pConVar->Revert();
			}
		}
		
		if (wantReset)
		{
			UTIL_ConsolePrint("[SM] Reset %d convars for: %s", pConVarList->size(), plname);
		}

		return;
	}

	/* Display usage of subcommand */
	UTIL_ConsolePrint("[SM] Usage: sm cvars [reset] <plugin #>");
}

Handle_t ConVarManager::CreateConVar(IPluginContext *pContext, const char *name, const char *defaultVal, const char *description, int flags, bool hasMin, float min, bool hasMax, float max)
{
	ConVar *pConVar = NULL;
	ConVarInfo *pInfo = NULL;
	Handle_t hndl = 0;

	IPlugin *plugin = scripts->FindPluginByContext(pContext->GetContext());

	/* Find out if the convar exists already */
	pConVar = icvar->FindVar(name);

	/* If the convar already exists... */
	if (pConVar)
	{
		/* Add convar to plugin's list */
		AddConVarToPluginList(plugin, pConVar);

		/* First find out if we already have a handle to it */
		if (convar_cache_lookup(name, &pInfo))
		{
			/* If the convar doesn't have an owning plugin, but SM created it, adopt it */
			if (pInfo->sourceMod && pInfo->pPlugin == nullptr) {
				pInfo->pPlugin = plugin;
			}

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
			hndl = handlesys->CreateHandle(m_ConVarType, pInfo, NULL, g_pCoreIdent, NULL);
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
	pInfo->pPlugin = plugin;

	/* Create a handle from the new convar */
	hndl = handlesys->CreateHandle(m_ConVarType, pInfo, NULL, g_pCoreIdent, NULL);
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
	AddConVarToPluginList(plugin, pConVar);

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

	/* Check convar cache to find out if we already have a handle */
	if (convar_cache_lookup(name, &pInfo))
	{
		return pInfo->handle;
	}

	/* Couldn't find it in cache, so search for it */
	pConVar = icvar->FindVar(name);

	/* If it doesn't exist, then return an invalid handle */
	if (!pConVar)
	{
		return BAD_HANDLE;
	}

	/* Create and initialize ConVarInfo structure */
	pInfo = new ConVarInfo();
	pInfo->sourceMod = false;
	pInfo->pChangeForward = NULL;
	pInfo->pVar = pConVar;

	/* If we don't have a handle, then create a new one */
	hndl = handlesys->CreateHandle(m_ConVarType, pInfo, NULL, g_pCoreIdent, NULL);
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
			pForward = forwardsys->CreateForwardEx(NULL, ET_Ignore, 3, CONVARCHANGE_PARAMS);
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
		if (pForward->GetFunctionCount() == 0 &&
			!ConVarReentrancyGuard::IsCvarInChain(pConVar))
		{
			/* Free this forward */
			forwardsys->ReleaseForward(pForward);
			pInfo->pChangeForward = NULL;
		}
	}
}

QueryCvarCookie_t ConVarManager::QueryClientConVar(edict_t *pPlayer, const char *name, IPluginFunction *pCallback, Handle_t hndl)
{
	QueryCvarCookie_t cookie = sCoreProviderImpl.QueryClientConVar(IndexOfEdict(pPlayer), name);

	if (pCallback != NULL)
	{
		ConVarQuery query = { cookie, pCallback, (cell_t) hndl, IndexOfEdict(pPlayer) };
		m_ConVarQueries.push_back(query);
	}

	return cookie;
}

void ConVarManager::AddConVarToPluginList(IPlugin *plugin, const ConVar *pConVar)
{
	ConVarList *pConVarList;
	ConVarList::iterator iter;
	bool inserted = false;
	const char *orig = pConVar->GetName();

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

void ConVarManager::OnConVarChanged(ConVar *pConVar, const char *oldValue, float flOldValue)
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
		for (auto i = pInfo->changeListeners.begin(); i != pInfo->changeListeners.end(); i++)
			(*i)->OnConVarChanged(pConVar, oldValue, flOldValue);
	}

	if (pForward != NULL)
	{
		ConVarReentrancyGuard guard(pConVar);

		/* Now call forwards in plugins that have hooked this */
		pForward->PushCell(pInfo->handle);
		pForward->PushString(oldValue);
		pForward->PushString(pConVar->GetString());
		pForward->Execute(NULL);
	}
}

bool ConVarManager::IsQueryingSupported()
{
	return sCoreProviderImpl.IsClientConVarQueryingSupported();
}

#if SOURCE_ENGINE != SE_DARKMESSIAH
void ConVarManager::OnClientQueryFinished(QueryCvarCookie_t cookie,
                                          int client,
                                          EQueryCvarValueStatus result,
										  const char *cvarName,
										  const char *cvarValue)
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
		pCallback->PushCell(client);
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

HandleError ConVarManager::ReadConVarHandle(Handle_t hndl, ConVar **pVar, IPlugin **ppPlugin)
{
	ConVarInfo *pInfo;
	HandleError error;
	
	if ((error = handlesys->ReadHandle(hndl, m_ConVarType, NULL, (void **)&pInfo)) != HandleError_None)
	{
		return error;
	}

	if (pVar)
	{
		*pVar = pInfo->pVar;
	}

	if (ppPlugin)
	{
		*ppPlugin = pInfo->pPlugin;
	}

	return error;
}
