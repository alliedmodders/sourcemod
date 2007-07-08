/**
 * vim: set ts=4 :
 * ===============================================================
 * SourceMod SDK Tools Extension
 * Copyright (C) 2004-2007 AlliedModders LLC. All rights reserved.
 * ===============================================================
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Version: $Id$
 */

#include "extension.h"
#include "vcallbuilder.h"
#include "vnatives.h"
#include "tempents.h"
#include "gamerules.h"

/**
 * @file extension.cpp
 * @brief Implements SDK Tools extension code.
 */

SDKTools g_SdkTools;		/**< Global singleton for extension's main interface */
IServerGameEnts *gameents = NULL;
IBinTools *g_pBinTools = NULL;
IGameConfig *g_pGameConf = NULL;
IGameHelpers *g_pGameHelpers = NULL;
IEngineSound *engsound = NULL;
HandleType_t g_CallHandle = 0;

SMEXT_LINK(&g_SdkTools);

extern sp_nativeinfo_t g_CallNatives[];
extern sp_nativeinfo_t g_TENatives[];

bool SDKTools::SDK_OnLoad(char *error, size_t maxlength, bool late)
{
	sharesys->AddDependency(myself, "bintools.ext", true, true);
	sharesys->AddNatives(myself, g_CallNatives);
	sharesys->AddNatives(myself, g_Natives);
	sharesys->AddNatives(myself, g_TENatives);
	sharesys->AddNatives(myself, g_SoundNatives);

	SM_GET_IFACE(GAMEHELPERS, g_pGameHelpers);

	if (!gameconfs->LoadGameConfigFile("sdktools.games", &g_pGameConf, error, maxlength))
	{
		return false;
	}

	g_CallHandle = handlesys->CreateType("ValveCall", this, 0, NULL, NULL, myself->GetIdentity(), NULL);

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

	g_TEManager.Shutdown();

	gameconfs->CloseGameConfigFile(g_pGameConf);
}

bool SDKTools::SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	GET_V_IFACE_ANY(serverFactory, gameents, IServerGameEnts, INTERFACEVERSION_SERVERGAMEENTS);
	GET_V_IFACE_ANY(engineFactory, engsound, IEngineSound, IENGINESOUND_SERVER_INTERFACE_VERSION);

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

	g_TEManager.Shutdown();
}

bool SDKTools::RegisterConCommandBase(ConCommandBase *pVar)
{
	return g_SMAPI->RegisterConCmdBase(g_PLAPI, pVar);
}
