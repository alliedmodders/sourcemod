/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2010 AlliedModders LLC.  All rights reserved.
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

#ifndef _INCLUDE_SOURCEMOD_MM_API_H_
#define _INCLUDE_SOURCEMOD_MM_API_H_

#include "sm_convar.h"
#include <ISmmPlugin.h>
#include <eiface.h>
#include <igameevents.h>
#include <iplayerinfo.h>
#include <filesystem.h>
#include <IEngineSound.h>
#include <toolframework/itoolentity.h>

/**
 * @file Contains wrappers around required Metamod:Source API exports
 */

class SourceMod_Core : 
	public ISmmPlugin,
	public IMetamodListener
{
public:
	bool Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late);
	bool Unload(char *error, size_t maxlen);
	bool Pause(char *error, size_t maxlen);
	bool Unpause(char *error, size_t maxlen);
	void AllPluginsLoaded();
public:
	const char *GetAuthor();
	const char *GetName();
	const char *GetDescription();
	const char *GetURL();
	const char *GetLicense();
	const char *GetVersion();
	const char *GetDate();
	const char *GetLogTag();
public:
	void OnVSPListening(IServerPluginCallbacks *iface);
	void OnUnlinkConCommandBase(PluginId id, ConCommandBase *pCommand);
	void *OnMetamodQuery(const char *iface, int *ret);
};

extern SourceMod_Core g_SourceMod_Core;
extern IVEngineServer *engine;
extern IServerGameDLL *gamedll;
extern IServerGameClients *serverClients;
extern ICvar *icvar;
extern ISmmPluginManager *g_pMMPlugins;
extern CGlobalVars *gpGlobals;
extern IGameEventManager2 *gameevents;
extern SourceHook::CallClass<IVEngineServer> *enginePatch;
extern SourceHook::CallClass<IServerGameDLL> *gamedllPatch;
extern IPlayerInfoManager *playerinfo;
extern IBaseFileSystem *basefilesystem;
extern IFileSystem *filesystem;
extern IEngineSound *enginesound;
#if SOURCE_ENGINE >= SE_ORANGEBOX
extern IServerTools *servertools;
#endif
extern IServerPluginHelpers *serverpluginhelpers;
extern IServerPluginCallbacks *vsp_interface;
extern int vsp_version;

#define ENGINE_CALL(func)		SH_CALL(enginePatch, &IVEngineServer::func)
#define SERVER_CALL(func)		SH_CALL(gamedllPatch, &IServerGameDLL::func)

PLUGIN_GLOBALVARS();

#endif //_INCLUDE_SOURCEMOD_MM_API_H_

