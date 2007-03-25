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

#include <oslink.h>
#include "sourcemm_api.h"
#include "sm_version.h"
#include "sourcemod.h"

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
}

const char *SourceMod_Core::GetAuthor()
{
	return "AlliedModders, LLC";
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
	return "SRCMOD";
}
