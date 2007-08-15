/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2007 AlliedModders LLC.  All rights reserved.
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

#include <oslink.h>
#include "sourcemm_api.h"
#include "sm_version.h"
#include "sourcemod.h"
#include "Logger.h"
#include "ConVarManager.h"

SourceMod_Core g_SourceMod_Core;
IVEngineServer *engine = NULL;
IServerGameDLL *gamedll = NULL;
IServerGameClients *serverClients = NULL;
ISmmPluginManager *g_pMMPlugins = NULL;
CGlobalVars *gpGlobals = NULL;
ICvar *icvar = NULL;
IGameEventManager2 *gameevents = NULL;
IUniformRandomStream *engrandom = NULL;
CallClass<IVEngineServer> *enginePatch = NULL;
CallClass<IServerGameDLL> *gamedllPatch = NULL;
IPlayerInfoManager *playerinfo = NULL;
IBaseFileSystem *basefilesystem = NULL;
IEngineSound *enginesound = NULL;
IServerPluginHelpers *serverpluginhelpers = NULL;

PLUGIN_EXPOSE(SourceMod, g_SourceMod_Core);

bool SourceMod_Core::Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	PLUGIN_SAVEVARS();

	GET_V_IFACE_ANY(serverFactory, gamedll, IServerGameDLL, INTERFACEVERSION_SERVERGAMEDLL);
	GET_V_IFACE_CURRENT(engineFactory, engine, IVEngineServer, INTERFACEVERSION_VENGINESERVER);
	GET_V_IFACE_CURRENT(serverFactory, serverClients, IServerGameClients, INTERFACEVERSION_SERVERGAMECLIENTS);
	GET_V_IFACE_CURRENT(engineFactory, icvar, ICvar, VENGINE_CVAR_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(engineFactory, gameevents, IGameEventManager2, INTERFACEVERSION_GAMEEVENTSMANAGER2);
	GET_V_IFACE_CURRENT(engineFactory, engrandom, IUniformRandomStream, VENGINE_SERVER_RANDOM_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(fileSystemFactory, basefilesystem, IBaseFileSystem, BASEFILESYSTEM_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(engineFactory, enginesound, IEngineSound, IENGINESOUND_SERVER_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(engineFactory, serverpluginhelpers, IServerPluginHelpers, INTERFACEVERSION_ISERVERPLUGINHELPERS);

	/* :TODO: Make this optional and... make it find earlier versions [?] */
	GET_V_IFACE_CURRENT(serverFactory, playerinfo, IPlayerInfoManager, INTERFACEVERSION_PLAYERINFOMANAGER);

	if ((g_pMMPlugins = (ISmmPluginManager *)g_SMAPI->MetaFactory(MMIFACE_PLMANAGER, NULL, NULL)) == NULL)
	{
		if (error)
		{
			snprintf(error, maxlen, "Unable to find interface %s", MMIFACE_PLMANAGER);
		}
		return false;
	}

	gpGlobals = ismm->pGlobals();
	
	ismm->AddListener(this, this);
	ismm->EnableVSPListener();

	return g_SourceMod.InitializeSourceMod(error, maxlen, late);
}

bool SourceMod_Core::Unload(char *error, size_t maxlen)
{
	g_SourceMod.CloseSourceMod();
	return true;
}

bool SourceMod_Core::Pause(char *error, size_t maxlen)
{
	return true;
}

bool SourceMod_Core::Unpause(char *error, size_t maxlen)
{
	return true;
}

void SourceMod_Core::AllPluginsLoaded()
{
	g_SourceMod.AllPluginsLoaded();
}

const char *SourceMod_Core::GetAuthor()
{
	return "AlliedModders LLC";
}

const char *SourceMod_Core::GetName()
{
	return "SourceMod";
}

const char *SourceMod_Core::GetDescription()
{
	return "Extensible administration and scripting system";
}

const char *SourceMod_Core::GetURL()
{
	return "http://www.sourcemod.net/";
}

const char *SourceMod_Core::GetLicense()
{
	return "See LICENSE.txt";
}

const char *SourceMod_Core::GetVersion()
{
	return SVN_FULL_VERSION;
}

const char *SourceMod_Core::GetDate()
{
	return __DATE__;
}

const char *SourceMod_Core::GetLogTag()
{
	return "SM";
}

void SourceMod_Core::OnVSPListening(IServerPluginCallbacks *iface)
{
	/* This shouldn't happen */
	if (!iface)
	{
		g_Logger.LogFatal("Metamod:Source version is out of date. SourceMod requires 1.4 or greater.");
		return;
	}

	/* Notify! */
	SMGlobalClass *pBase = SMGlobalClass::head;
	while (pBase)
	{
		pBase->OnSourceModVSPReceived(iface);
		pBase = pBase->m_pGlobalClassNext;
	}
}

#if PLAPI_VERSION >= 12
void SourceMod_Core::OnUnlinkConCommandBase(PluginId id, ConCommandBase *pCommand)
{
	g_ConVarManager.OnUnlinkConCommandBase(id, pCommand);
}
#else
void SourceMod_Core::OnPluginUnload(PluginId id)
{
	g_ConVarManager.OnMetamodPluginUnloaded(id);
}
#endif
