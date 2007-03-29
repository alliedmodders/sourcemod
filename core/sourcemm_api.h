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

#ifndef _INCLUDE_SOURCEMOD_MM_API_H_
#define _INCLUDE_SOURCEMOD_MM_API_H_

#include "convar_sm.h"
#include <ISmmPlugin.h>
#include <eiface.h>
#include <igameevents.h>
#include <iplayerinfo.h>
#include <random.h>
#include <filesystem.h>
#include <IEngineSound.h>

/**
 * @file Contains wrappers around required Metamod:Source API exports
 */

class SourceMod_Core : public ISmmPlugin
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
extern IUniformRandomStream *engrandom;
extern IPlayerInfoManager *playerinfo;
extern IBaseFileSystem *basefilesystem;
extern IEngineSound *enginesound;
extern IServerPluginHelpers *serverpluginhelpers;

#define ENGINE_CALL(func)		SH_CALL(enginePatch, &IVEngineServer::func)
#define SERVER_CALL(func)		SH_CALL(gamedllPatch, &IServerGameDLL::func)

PLUGIN_GLOBALVARS();

#endif //_INCLUDE_SOURCEMOD_MM_API_H_
