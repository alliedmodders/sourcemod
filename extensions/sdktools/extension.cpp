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

#include "extension.h"
#include "vcallbuilder.h"
#include "vnatives.h"
#include "vhelpers.h"
#include "tempents.h"
#include "gamerules.h"

/**
 * @file extension.cpp
 * @brief Implements SDK Tools extension code.
 */

SDKTools g_SdkTools;		/**< Global singleton for extension's main interface */
IServerGameEnts *gameents = NULL;
IEngineTrace *enginetrace = NULL;
IEngineSound *engsound = NULL;
INetworkStringTableContainer *netstringtables = NULL;
IServerPluginHelpers *pluginhelpers = NULL;
IBinTools *g_pBinTools = NULL;
IGameConfig *g_pGameConf = NULL;
IGameHelpers *g_pGameHelpers = NULL;
IServerGameClients *serverClients = NULL;
IPlayerInfoManager *playerinfomngr = NULL;
HandleType_t g_CallHandle = 0;
HandleType_t g_TraceHandle = 0;
IVoiceServer *voiceserver = NULL;

SMEXT_LINK(&g_SdkTools);

extern sp_nativeinfo_t g_CallNatives[];
extern sp_nativeinfo_t g_TENatives[];
extern sp_nativeinfo_t g_TRNatives[];
extern sp_nativeinfo_t g_StringTableNatives[];
extern sp_nativeinfo_t g_VoiceNatives[];

bool SDKTools::SDK_OnLoad(char *error, size_t maxlength, bool late)
{
	sharesys->AddDependency(myself, "bintools.ext", true, true);
	sharesys->AddNatives(myself, g_CallNatives);
	sharesys->AddNatives(myself, g_Natives);
	sharesys->AddNatives(myself, g_TENatives);
	sharesys->AddNatives(myself, g_SoundNatives);
	sharesys->AddNatives(myself, g_TRNatives);
	sharesys->AddNatives(myself, g_StringTableNatives);
	sharesys->AddNatives(myself, g_VoiceNatives);

	SM_GET_IFACE(GAMEHELPERS, g_pGameHelpers);

	if (!gameconfs->LoadGameConfigFile("sdktools.games", &g_pGameConf, error, maxlength))
	{
		return false;
	}

	playerhelpers->AddClientListener(&g_SdkTools);
	g_CallHandle = handlesys->CreateType("ValveCall", this, 0, NULL, NULL, myself->GetIdentity(), NULL);
	g_TraceHandle = handlesys->CreateType("TraceRay", this, 0, NULL, NULL, myself->GetIdentity(), NULL);

	ConCommandBaseMgr::OneTimeInit(this);

	return true;
}

void SDKTools::OnHandleDestroy(HandleType_t type, void *object)
{
	if (type == g_CallHandle)
	{
		ValveCall *v = (ValveCall *)object;
		delete v;
	}
	if (type == g_TraceHandle)
	{
		trace_t *tr = (trace_t *)object;
		delete tr;
	}
}

void SDKTools::SDK_OnUnload()
{
	List<ValveCall *>::iterator iter;
	for (iter = g_RegCalls.begin();
		 iter != g_RegCalls.end();
		 iter++)
	{
		delete (*iter);
	}
	g_RegCalls.clear();
	ShutdownHelpers();

	g_TEManager.Shutdown();

	gameconfs->CloseGameConfigFile(g_pGameConf);
	playerhelpers->RemoveClientListener(&g_SdkTools);
}

bool SDKTools::SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	GET_V_IFACE_ANY(serverFactory, gameents, IServerGameEnts, INTERFACEVERSION_SERVERGAMEENTS);
	GET_V_IFACE_ANY(engineFactory, engsound, IEngineSound, IENGINESOUND_SERVER_INTERFACE_VERSION);
	GET_V_IFACE_ANY(engineFactory, enginetrace, IEngineTrace, INTERFACEVERSION_ENGINETRACE_SERVER);
	GET_V_IFACE_ANY(engineFactory, netstringtables, INetworkStringTableContainer, INTERFACENAME_NETWORKSTRINGTABLESERVER);
	GET_V_IFACE_ANY(engineFactory, pluginhelpers, IServerPluginHelpers, INTERFACEVERSION_ISERVERPLUGINHELPERS);
	GET_V_IFACE_ANY(serverFactory, serverClients, IServerGameClients, INTERFACEVERSION_SERVERGAMECLIENTS);
	GET_V_IFACE_ANY(engineFactory, voiceserver, IVoiceServer, INTERFACEVERSION_VOICESERVER);
	GET_V_IFACE_ANY(serverFactory, playerinfomngr, IPlayerInfoManager, INTERFACEVERSION_PLAYERINFOMANAGER);

	return true;
}

void SDKTools::SDK_OnAllLoaded()
{
	SM_GET_LATE_IFACE(BINTOOLS, g_pBinTools);

	g_TEManager.Initialize();
	InitializeGameRules();
}

bool SDKTools::QueryRunning(char *error, size_t maxlength)
{
	SM_CHECK_IFACE(BINTOOLS, g_pBinTools);

	return true;
}

bool SDKTools::QueryInterfaceDrop(SMInterface *pInterface)
{
	if (pInterface == g_pBinTools)
	{
		return false;
	}

	return IExtensionInterface::QueryInterfaceDrop(pInterface);
}

void SDKTools::NotifyInterfaceDrop(SMInterface *pInterface)
{
	List<ValveCall *>::iterator iter;
	for (iter = g_RegCalls.begin();
		iter != g_RegCalls.end();
		iter++)
	{
		delete (*iter);
	}
	g_RegCalls.clear();
	ShutdownHelpers();

	g_TEManager.Shutdown();
}

bool SDKTools::RegisterConCommandBase(ConCommandBase *pVar)
{
	return g_SMAPI->RegisterConCmdBase(g_PLAPI, pVar);
}
