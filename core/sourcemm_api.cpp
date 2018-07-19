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

#include "sourcemod.h"
#include "sourcemm_api.h"
#include "Logger.h"
#include "concmd_cleaner.h"
#include "compat_wrappers.h"
#include "logic_bridge.h"
#include <sourcemod_version.h>
#include "provider.h"
#include <IExtensionSys.h>
#include <bridge/include/ILogger.h>

SourceMod_Core g_SourceMod_Core;
IVEngineServer *engine = NULL;
IServerGameDLL *gamedll = NULL;
IServerGameClients *serverClients = NULL;
ISmmPluginManager *g_pMMPlugins = NULL;
CGlobalVars *gpGlobals = NULL;
ICvar *icvar = NULL;
IGameEventManager2 *gameevents = NULL;
CallClass<IVEngineServer> *enginePatch = NULL;
CallClass<IServerGameDLL> *gamedllPatch = NULL;
IPlayerInfoManager *playerinfo = NULL;
IBaseFileSystem *basefilesystem = NULL;
IFileSystem *filesystem = NULL;
IEngineSound *enginesound = NULL;
#if SOURCE_ENGINE >= SE_ORANGEBOX
IServerTools *servertools = NULL;
#endif
IServerPluginHelpers *serverpluginhelpers = NULL;
IServerPluginCallbacks *vsp_interface = NULL;
int vsp_version = 0;

PLUGIN_EXPOSE(SourceMod, g_SourceMod_Core);

ConVar sourcemod_version("sourcemod_version", SOURCEMOD_VERSION, FCVAR_SPONLY|FCVAR_NOTIFY, "SourceMod Version");

bool SourceMod_Core::Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	PLUGIN_SAVEVARS();

	GET_V_IFACE_ANY(GetServerFactory, gamedll, IServerGameDLL, INTERFACEVERSION_SERVERGAMEDLL);
#if SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_DODS || SOURCE_ENGINE == SE_HL2DM || SOURCE_ENGINE == SE_SDK2013
	// Shim to avoid hooking shims
	engine = (IVEngineServer *)ismm->GetEngineFactory()("VEngineServer023", nullptr);
	if (!engine)
	{
		engine = (IVEngineServer *)ismm->GetEngineFactory()("VEngineServer022", nullptr);
		if (!engine)
		{
			engine = (IVEngineServer *)ismm->GetEngineFactory()("VEngineServer021", nullptr);
			if (!engine)
			{
				if (error && maxlen)
				{
					ismm->Format(error, maxlen, "Could not find interface: VEngineServer023 or VEngineServer022");
				}
				return false;
			}
		}
	}
#else
	GET_V_IFACE_CURRENT(GetEngineFactory, engine, IVEngineServer, INTERFACEVERSION_VENGINESERVER);
#endif
	GET_V_IFACE_CURRENT(GetServerFactory, serverClients, IServerGameClients, INTERFACEVERSION_SERVERGAMECLIENTS);
	GET_V_IFACE_CURRENT(GetEngineFactory, icvar, ICvar, CVAR_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, gameevents, IGameEventManager2, INTERFACEVERSION_GAMEEVENTSMANAGER2);
	GET_V_IFACE_CURRENT(GetFileSystemFactory, basefilesystem, IBaseFileSystem, BASEFILESYSTEM_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetFileSystemFactory, filesystem, IFileSystem, FILESYSTEM_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, enginesound, IEngineSound, IENGINESOUND_SERVER_INTERFACE_VERSION);
#if SOURCE_ENGINE >= SE_ORANGEBOX
	GET_V_IFACE_CURRENT(GetServerFactory, servertools, IServerTools, VSERVERTOOLS_INTERFACE_VERSION);
#endif
	GET_V_IFACE_CURRENT(GetEngineFactory, serverpluginhelpers, IServerPluginHelpers, INTERFACEVERSION_ISERVERPLUGINHELPERS);

	/* :TODO: Make this optional and... make it find earlier versions [?] */
	GET_V_IFACE_CURRENT(GetServerFactory, playerinfo, IPlayerInfoManager, INTERFACEVERSION_PLAYERINFOMANAGER);

	if ((g_pMMPlugins = (ISmmPluginManager *)g_SMAPI->MetaFactory(MMIFACE_PLMANAGER, NULL, NULL)) == NULL)
	{
		if (error)
		{
			ke::SafeSprintf(error, maxlen, "Unable to find interface %s", MMIFACE_PLMANAGER);
		}
		return false;
	}

	gpGlobals = ismm->GetCGlobals();
	
	ismm->AddListener(this, this);

	if ((vsp_interface = g_SMAPI->GetVSPInfo(&vsp_version)) == NULL)
	{
		g_SMAPI->EnableVSPListener();
	}

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
	return SOURCEMOD_VERSION;
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
		logger->LogFatal("Metamod:Source version is out of date. SourceMod requires 1.4.2 or greater.");
		return;
	}

	if (vsp_interface == NULL)
	{
		vsp_interface = iface;
	}

	if (!g_Loaded)
	{
		return;
	}

	if (vsp_version == 0)
	{
		g_SMAPI->GetVSPInfo(&vsp_version);
	}

	/* Notify! */
    sCoreProviderImpl.OnVSPReceived();
}

void SourceMod_Core::OnUnlinkConCommandBase(PluginId id, ConCommandBase *pCommand)
{
#if SOURCE_ENGINE < SE_ORANGEBOX
	Global_OnUnlinkConCommandBase(pCommand);
#endif
}

void *SourceMod_Core::OnMetamodQuery(const char *iface, int *ret)
{
	void *ptr = NULL;

	if (strcmp(iface, SOURCEMOD_INTERFACE_EXTENSIONS) == 0)
	{
		ptr = extsys;
	}

	if (ret != NULL)
	{
		*ret = (ptr == NULL) ? IFACE_FAILED : IFACE_OK;
	}

	return ptr;
}
