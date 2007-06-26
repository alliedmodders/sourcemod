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

/**
 * @file extension.cpp
 * @brief Implements SDK Tools extension code.
 */

SDKTools g_SdkTools;		/**< Global singleton for extension's main interface */
IServerGameEnts *gameents = NULL;
IBinTools *g_pBinTools = NULL;
IGameConfig *g_pGameConf = NULL;
HandleType_t g_CallHandle = 0;

SMEXT_LINK(&g_SdkTools);

extern sp_nativeinfo_t g_CallNatives[];

bool SDKTools::SDK_OnLoad(char *error, size_t maxlength, bool late)
{
	sharesys->AddDependency(myself, "bintools.ext", true, true);
	sharesys->AddNatives(myself, g_CallNatives);
	sharesys->AddNatives(myself, g_Natives);

	if (!gameconfs->LoadGameConfigFile("sdktools.games", &g_pGameConf, error, maxlength))
	{
		return false;
	}

	g_CallHandle = handlesys->CreateType("ValveCall", this, 0, NULL, NULL, myself->GetIdentity(), NULL);

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

	gameconfs->CloseGameConfigFile(g_pGameConf);
}

bool SDKTools::SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	GET_V_IFACE_CURRENT(serverFactory, gameents, IServerGameEnts, INTERFACEVERSION_SERVERGAMEENTS);

	return true;
}

void SDKTools::SDK_OnAllLoaded()
{
	SM_GET_LATE_IFACE(BINTOOLS, g_pBinTools);
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

}
