/**
 * vim: set ts=4 :
 * ================================================================
 * SourceMod SDKTools Extension
 * Copyright (C) 2004-2007 AlliedModders LLC.  All rights reserved.
 * ================================================================
 *
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License, 
 * version 3.0, as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to 
 * link the code of this program (as well as its derivative works) to 
 * "Half-Life 2," the "Source Engine," the "SourcePawn JIT," and any 
 * Game MODs that run on software by the Valve Corporation.  You must 
 * obey the GNU General Public License in all respects for all other 
 * code used. Additionally, AlliedModders LLC grants this exception 
 * to all derivative works. AlliedModders LLC defines further 
 * exceptions, found in LICENSE.txt (as of this writing, version 
 * JULY-31-2007), or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */

#ifndef _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_

/**
 * @file extension.h
 * @brief SDK Tools extension code header.
 */

#include "smsdk_ext.h"
#include <IBinTools.h>
#include <IPlayerHelpers.h>
#include <IGameHelpers.h>
#include <IEngineTrace.h>
#include <IEngineSound.h>
#include <ivoiceserver.h>
#include <iplayerinfo.h>
#include <convar.h>

/**
 * @brief Implementation of the SDK Tools extension.
 * Note: Uncomment one of the pre-defined virtual functions in order to use it.
 */
class SDKTools : 
	public SDKExtension, 
	public IHandleTypeDispatch,
	public IConCommandBaseAccessor,
	public IClientListener
{
public: //public IHandleTypeDispatch
	void OnHandleDestroy(HandleType_t type, void *object);
public: //public SDKExtension
	virtual bool SDK_OnLoad(char *error, size_t maxlength, bool late);
	virtual void SDK_OnUnload();
	virtual void SDK_OnAllLoaded();
	//virtual void SDK_OnPauseChange(bool paused);
	virtual bool QueryRunning(char *error, size_t maxlength);
	virtual bool QueryInterfaceDrop(SMInterface *pInterface);
	virtual void NotifyInterfaceDrop(SMInterface *pInterface);
public:
#if defined SMEXT_CONF_METAMOD
	virtual bool SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlen, bool late);
	//virtual bool SDK_OnMetamodUnload(char *error, size_t maxlen);
	//virtual bool SDK_OnMetamodPauseChange(bool paused, char *error, size_t maxlen);
#endif
public: //IConCommandBaseAccessor
	bool RegisterConCommandBase(ConCommandBase *pVar);
public: //IClientListner
	void OnClientDisconnecting(int client);
public: // IVoiceServer
	bool OnSetClientListening(int iReceiver, int iSender, bool bListen);
};

extern SDKTools g_SdkTools;
/* Interfaces from engine or gamedll */
extern IServerGameEnts *gameents;
extern IEngineTrace *enginetrace;
extern IEngineSound *engsound;
extern INetworkStringTableContainer *netstringtables;
extern IServerPluginHelpers *pluginhelpers;
extern IServerGameClients *serverClients;
extern IVoiceServer *voiceserver;
extern IPlayerInfoManager *playerinfomngr;
/* Interfaces from SourceMod */
extern IBinTools *g_pBinTools;
extern IGameConfig *g_pGameConf;
extern IGameHelpers *g_pGameHelpers;
/* Handle types */
extern HandleType_t g_CallHandle;
extern HandleType_t g_TraceHandle;

#endif //_INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_
