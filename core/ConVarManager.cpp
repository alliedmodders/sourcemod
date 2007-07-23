/**
 * vim: set ts=4 :
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
#include "HalfLife2.h"
#include "PluginSys.h"
#include "ForwardSys.h"
#include "HandleSys.h"
#include "sm_srvcmds.h"
#include "sm_stringutil.h"
#include <sh_vector.h>

ConVarManager g_ConVarManager;

SH_DECL_HOOK5_void(IServerGameDLL, OnQueryCvarValueFinished, SH_NOATTRIB, 0, QueryCvarCookie_t, edict_t *, EQueryCvarValueStatus, const char *, const char *);
SH_DECL_HOOK5_void(IServerPluginCallbacks, OnQueryCvarValueFinished, SH_NOATTRIB, 0, QueryCvarCookie_t, edict_t *, EQueryCvarValueStatus, const char *, const char *);

const ParamType CONVARCHANGE_PARAMS[] = {Param_Cell, Param_String, Param_String};
typedef List<const ConVar *> ConVarList;

ConVarManager::ConVarManager() : m_ConVarType(0), m_VSPIface(NULL), m_CanQueryConVars(false)
{
#if PLAPI_VERSION < 12
	m_IgnoreHandle = false;
#endif

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
	/* Only valid with ServerGameDLL006 or greater */
	if (g_SMAPI->GetGameDLLVersion() >= 6)
	{
		SH_ADD_HOOK_MEMFUNC(IServerGameDLL, OnQueryCvarValueFinished, gamedll, this, &ConVarManager::OnQueryCvarValueFinished, false);
		m_CanQueryConVars = true;
	}

	HandleAccess sec;

	/* Set up access rights for the 'ConVar' handle type */
	sec.access[HandleAccess_Read] = 0;
	sec.access[HandleAccess_Delete] = HANDLE_RESTRICT_IDENTITY | HANDLE_RESTRICT_OWNER;
	sec.access[HandleAccess_Clone] = HANDLE_RESTRICT_IDENTITY | HANDLE_RESTRICT_OWNER;

	/* Create the 'ConVar' handle type */
	m_ConVarType = g_HandleSys.CreateType("ConVar", this, 0, NULL, &sec, g_pCoreIdent, NULL);

	/* Add the 'convars' option to the 'sm' console command */
	g_RootMenu.AddRootConsoleCommand("cvars", "View convars created by a plugin", this);
}

void ConVarManager::OnSourceModShutdown()
{
	if (m_CanQueryConVars)
	{
		SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, OnQueryCvarValueFinished, gamedll, this, &ConVarManager::OnQueryCvarValueFinished, false);
		SH_REMOVE_HOOK_MEMFUNC(IServerPluginCallbacks, OnQueryCvarValueFinished, m_VSPIface, this, &ConVarManager::OnQueryCvarValueFinished, false);
	}

	IChangeableForward *fwd;
	List<ConVarInfo *>::iterator i;

	/* Iterate list of ConVarInfo structures */
	for (i = m_ConVars.begin(); i != m_ConVars.end(); i++)
	{
		fwd = (*i)->pChangeForward;

		/* Free any convar-change forwards that still exist */
		if (fwd)
		{
			g_Forwards.ReleaseForward(fwd);
		}
	}

	/* Remove the 'convars' option from the 'sm' console command */
	g_RootMenu.RemoveRootConsoleCommand("cvars", this);

	/* Remove the 'ConVar' handle type */
	g_HandleSys.RemoveType(m_ConVarType, g_pCoreIdent);
}

void ConVarManager::OnSourceModVSPReceived(IServerPluginCallbacks *iface)
{
	/* This will be called after OnSourceModAllInitialized(), so...
	 *
	 * If we haven't been able to hook the IServerGameDLL version at this point,
	 * then use hook IServerPluginCallbacks version from the engine.
	 */

	/* The Ship currently only supports ServerPluginCallbacks001, but we need 002 */
	if (g_IsOriginalEngine)
	{
		return;
	}

	if (!m_CanQueryConVars)
	{
		m_VSPIface = iface;

		SH_ADD_HOOK_MEMFUNC(IServerPluginCallbacks, OnQueryCvarValueFinished, iface, this, &ConVarManager::OnQueryCvarValueFinished, false);
		m_CanQueryConVars = true;
	}
}

#if PLAPI_VERSION >= 12
void ConVarManager::OnUnlinkConCommandBase(PluginId id, ConCommandBase *pCommand)
{
	/* Only check convars that have not been created by SourceMod's core */
	if (id != g_PLID && !pCommand->IsCommand())
	{
		ConVarInfo *pInfo;
		HandleSecurity sec(NULL, g_pCoreIdent);
		bool handleExists = sm_trie_retrieve(m_ConVarCache, pCommand->GetName(), reinterpret_cast<void **>(&pInfo));

		if (handleExists)
		{
			g_HandleSys.FreeHandle(pInfo->handle, &sec);
		}
	}
}

#else

/* I truly detest this code */
void ConVarManager::OnMetamodPluginUnloaded(PluginId id)
{
	ConVarInfo *pInfo;
	const char *cvarName;
	HandleSecurity sec(NULL, g_pCoreIdent);

	List<ConVarInfo *>::iterator i;
	for (i = m_ConVars.begin(); i != m_ConVars.end(); i++)
	{
		pInfo = (*i);
		cvarName = pInfo->name;

		if (!icvar->FindVar(cvarName))
		{
			m_IgnoreHandle = true;
			g_HandleSys.FreeHandle(pInfo->handle, &sec);

			sm_trie_delete(m_ConVarCache, cvarName);
			m_ConVars.erase(i);

			delete [] cvarName;
		}
	}
}
#endif

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
#if PLAPI_VERSION < 12
	/* Lovely workaround for our workaround! */
	if (m_IgnoreHandle)
	{
		m_IgnoreHandle = false;
		return;
	}
#endif

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
	Handle_t hndl = 0;

	/* Find out if the convar exists already */
	pConVar = icvar->FindVar(name);

	/* If the convar already exists... */
	if (pConVar)
	{
		/* Add convar to plugin's list */
		AddConVarToPluginList(pContext, pConVar);

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
			pInfo->pChangeForward = NULL;
			pInfo->origCallback = pConVar->GetCallback();
		#if PLAPI_VERSION < 12
			pInfo->name = sm_strdup(pConVar->GetName());
		#endif

			/* Insert struct into caches */
			m_ConVars.push_back(pInfo);
			sm_trie_insert(m_ConVarCache, name, pInfo);

			return hndl;
		}
	}

	/* To prevent creating a convar that has the same name as a console command... ugh */
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
	pInfo->pChangeForward = NULL;
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

	/* Create and initialize ConVarInfo structure */
	pInfo = new ConVarInfo();
	pInfo->handle = hndl;
	pInfo->sourceMod = false;
	pInfo->pChangeForward = NULL;
	pInfo->origCallback = pConVar->GetCallback();
#if PLAPI_VERSION < 12
	pInfo->name = sm_strdup(pConVar->GetName());
#endif

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
		pForward = pInfo->pChangeForward;

		/* If forward does not exist, create it */
		if (!pForward)
		{
			pForward = g_Forwards.CreateForwardEx(NULL, ET_Ignore, 3, CONVARCHANGE_PARAMS);
			pInfo->pChangeForward = pForward;

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

			/* Put back the original convar callback */
			pConVar->InstallChangeCallback(pInfo->origCallback);
		}
	}
}

QueryCvarCookie_t ConVarManager::QueryClientConVar(edict_t *pPlayer, const char *name, IPluginFunction *pCallback, Handle_t hndl)
{
	QueryCvarCookie_t cookie;

	/* Call StartQueryCvarValue() in either the IVEngineServer or IServerPluginHelpers depending on situation */
	if (!m_VSPIface)
	{
		cookie = engine->StartQueryCvarValue(pPlayer, name);	
	} else {
		cookie = serverpluginhelpers->StartQueryCvarValue(pPlayer, name);
	}

	ConVarQuery query = {cookie, pCallback, hndl};
	m_ConVarQueries.push_back(query);

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
	} else if (pConVarList->find(pConVar) != pConVarList->end()) {
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
	IChangeableForward *pForward = pInfo->pChangeForward;

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
		pCallback->PushCell(engine->IndexOfEdict(pPlayer));
		pCallback->PushCell(result);
		pCallback->PushString(cvarName);

		if (result == eQueryCvarValueStatus_ValueIntact)
		{
			pCallback->PushString(cvarValue);
		} else {
			pCallback->PushString("\0");
		}

		pCallback->PushCell(value);
		pCallback->Execute(&ret);

		m_ConVarQueries.erase(iter);
	}
}


static int s_YamagramState = 0;

void _YamagramPrinterTwoPointOhOh(int yamagram)
{
	switch (yamagram)
	{
	case 0:
		g_RootMenu.ConsolePrint("Answer the following questions correctly and Gaben may not eat you after all.");
		g_RootMenu.ConsolePrint("You will be given one hint in the form of my patented yamagrams.");
		g_RootMenu.ConsolePrint("Type sm_nana to see the last question.");
		g_RootMenu.ConsolePrint("Type sm_nana <answer> to attempt an answer of the question.");
		g_RootMenu.ConsolePrint("-------------------------------");
		_YamagramPrinterTwoPointOhOh(1);
		return;
	case 1:
		g_RootMenu.ConsolePrint("Question Ichi (1)");
		g_RootMenu.ConsolePrint("One can turn into a cow by doing what action?");
		g_RootMenu.ConsolePrint("Hint: AGE SANS GRIT");
		break;
	case 2:
		g_RootMenu.ConsolePrint("Question Ni (2)");
		g_RootMenu.ConsolePrint("What kind of hat should you wear when using the Internet?");
		g_RootMenu.ConsolePrint("Hint: BRR MOOSE");
		break;
	case 3:
		g_RootMenu.ConsolePrint("Question San (3)");
		g_RootMenu.ConsolePrint("Who is the lead developer of SourceMod?");
		g_RootMenu.ConsolePrint("Hint: VEAL BANDANA DID RIP SOON");
		break;
	case 4:
		g_RootMenu.ConsolePrint("Question Yon (4)");
		g_RootMenu.ConsolePrint("A terrible translation of 'SVN Revision' to Japanese romaji might be ...");
		g_RootMenu.ConsolePrint("Hint: I TAKE IN AN AIR OK");
		break;
	case 5:
		g_RootMenu.ConsolePrint("Question Go (5)");
		g_RootMenu.ConsolePrint("What is a fundamental concept in the game of Go?");
		g_RootMenu.ConsolePrint("Hint: AD LADEN THIEF");
		break;
	case 6:
		g_RootMenu.ConsolePrint("Question Roku (6)");
		g_RootMenu.ConsolePrint("Why am I asking all these strange questions?");
		g_RootMenu.ConsolePrint("Hint: CHUBBY TITAN EATS EWE WAGE DATA");
		break;
	case 7:
		g_RootMenu.ConsolePrint("Question Nana (7)");
		g_RootMenu.ConsolePrint("What is my name?");
		g_RootMenu.ConsolePrint("Hint: AD MODE LAG US");
		break;
	default:
		break;
	}

	s_YamagramState = yamagram;
}

void _IntExt_CallYamagrams()
{
	bool correct = false;
	const char *arg = engine->Cmd_Args();

	if (!arg || arg[0] == '\0')
	{
		_YamagramPrinterTwoPointOhOh(s_YamagramState);
		return;
	}

	switch (s_YamagramState)
	{
	case 1:
		correct = !strcasecmp(arg, "eating grass");
		break;
	case 2:
		correct = !strcasecmp(arg, "sombrero");
		break;
	case 3:
		correct = !strcasecmp(arg, "david bailopan anderson");
		break;
	case 4:
		correct = !strcasecmp(arg, "kaitei no kairan");
		break;
	case 5:
		correct = !strcasecmp(arg, "life and death");
		break;
	case 6:
		correct = !strcasecmp(arg, "because gabe wanted it that way");
		if (correct)
		{
			g_RootMenu.ConsolePrint("Congratulations, you have answered 6 of my questions.");
			g_RootMenu.ConsolePrint("However, I have one final question for you. It wouldn't be nana without it.");
			g_RootMenu.ConsolePrint("-------------------------------");
			_YamagramPrinterTwoPointOhOh(7);
			return;
		}
		break;
	case 7:
		correct = !strcasecmp(arg, "damaged soul");
		if (correct)
		{
			g_RootMenu.ConsolePrint("You don't know how lucky you are to still be alive!");
			g_RootMenu.ConsolePrint("Congratulations. You have answered all 7 questions correctly.");
			g_RootMenu.ConsolePrint("The SourceMod Dev Team will be at your door with anti-Gaben grenades");
			g_RootMenu.ConsolePrint("within seconds. You will also be provided with a rocket launcher,");
			g_RootMenu.ConsolePrint("just in case Alfred decides to strike you with a blitzkrieg in retaliation.");

			s_YamagramState = 0;
			return;
		}
		break;
	default:
		break;
	}

	if (s_YamagramState > 0)
	{
		if (correct)
		{
			g_RootMenu.ConsolePrint("Correct! You are one step closer to avoiding the deadly jaws of Gaben.");
			g_RootMenu.ConsolePrint("-------------------------------");
			s_YamagramState++;
		} else {
			g_RootMenu.ConsolePrint("Wrong! You better be more careful. Gaben may be at your door at any minute.");
			return;
		}
	}

	_YamagramPrinterTwoPointOhOh(s_YamagramState);
}

void _IntExt_EnableYamagrams()
{
	static ConCommand *pCmd = NULL;
	if (!pCmd)
	{
		pCmd = new ConCommand("sm_nana", _IntExt_CallYamagrams, "Try these yamagrams!", FCVAR_GAMEDLL);
		g_RootMenu.ConsolePrint("[SM] Warning: Gaben has been alerted of your actions. You may be eaten.");
	} else {
		g_RootMenu.ConsolePrint("[SM] Gaben has already been alerted of your actions...");
	}
}

void _IntExt_OnHostnameChanged(ConVar *pConVar, char const *oldValue)
{
	if (strcmp(oldValue, "Good morning, DS-san.") == 0
		&& strcmp(pConVar->GetString(), "Good night, talking desk lamp.") == 0)
	{
		_IntExt_EnableYamagrams();
	}
}
