/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Dynamic Hooks Extension
 * Copyright (C) 2012-2021 AlliedModders LLC.  All rights reserved.
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

#include "extension.h"
#include "listeners.h"
#include "dynhooks_sourcepawn.h"
#include "signatures.h"

DHooks g_DHooksIface;		/**< Global singleton for extension's main interface */
SMEXT_LINK(&g_DHooksIface);

IBinTools *g_pBinTools;
ISDKHooks *g_pSDKHooks;
ISDKTools *g_pSDKTools;
DHooksEntityListener *g_pEntityListener = NULL;

HandleType_t g_HookSetupHandle = 0;
HandleType_t g_HookParamsHandle = 0;
HandleType_t g_HookReturnHandle = 0;

std::thread::id g_MainThreadId;

bool DHooks::SDK_OnLoad(char *error, size_t maxlength, bool late)
{
	HandleError err;
	g_HookSetupHandle = handlesys->CreateType("HookSetup", this, 0, NULL, NULL, myself->GetIdentity(), &err);
	if(g_HookSetupHandle == 0)
	{
		snprintf(error, maxlength, "Could not create hook setup handle type (err: %d)", err);	
		return false;
	}
	g_HookParamsHandle = handlesys->CreateType("HookParams", this, 0, NULL, NULL, myself->GetIdentity(), &err);
	if(g_HookParamsHandle == 0)
	{
		snprintf(error, maxlength, "Could not create hook params handle type (err: %d)", err);	
		return false;
	}
	g_HookReturnHandle = handlesys->CreateType("HookReturn", this, 0, NULL, NULL, myself->GetIdentity(), &err);
	if(g_HookReturnHandle == 0)
	{
		snprintf(error, maxlength, "Could not create hook return handle type (err: %d)", err);	
		return false;
	}

	if (!g_pPreDetours.init())
	{
		snprintf(error, maxlength, "Could not initialize pre hook detours hashmap.");
		return false;
	}

	if (!g_pPostDetours.init())
	{
		snprintf(error, maxlength, "Could not initialize post hook detours hashmap.");
		return false;
	}

	sharesys->AddDependency(myself, "bintools.ext", true, true);
	sharesys->AddDependency(myself, "sdktools.ext", true, true);
	sharesys->AddDependency(myself, "sdkhooks.ext", true, true);

	sharesys->RegisterLibrary(myself, "dhooks");
	plsys->AddPluginsListener(this);
	sharesys->AddNatives(myself, g_Natives);

	g_pEntityListener = new DHooksEntityListener();
	g_pSignatures = new SignatureGameConfig();
	g_MainThreadId = std::this_thread::get_id();

	return true;
}

void DHooks::OnHandleDestroy(HandleType_t type, void *object)
{
	if(type == g_HookSetupHandle)
	{
		delete (HookSetup *)object;
	}
	else if(type == g_HookParamsHandle)
	{
		delete (HookParamsStruct *)object;
	}
	else if(type == g_HookReturnHandle)
	{
		delete (HookReturnStruct *)object;
	}
}

void DHooks::SDK_OnAllLoaded()
{
	SM_GET_LATE_IFACE(SDKTOOLS, g_pSDKTools);
	SM_GET_LATE_IFACE(BINTOOLS, g_pBinTools);
	SM_GET_LATE_IFACE(SDKHOOKS, g_pSDKHooks);

	g_pSDKHooks->AddEntityListener(g_pEntityListener);
	gameconfs->AddUserConfigHook("Functions", g_pSignatures);
}

void DHooks::SDK_OnUnload()
{
	CleanupHooks();
	CleanupDetours();
	if(g_pEntityListener)
	{
		g_pEntityListener->CleanupListeners();
		g_pEntityListener->CleanupRemoveList();
		g_pSDKHooks->RemoveEntityListener(g_pEntityListener);
		delete g_pEntityListener;
	}
	plsys->RemovePluginsListener(this);

	handlesys->RemoveType(g_HookSetupHandle, myself->GetIdentity());
	handlesys->RemoveType(g_HookParamsHandle, myself->GetIdentity());
	handlesys->RemoveType(g_HookReturnHandle, myself->GetIdentity());

	gameconfs->RemoveUserConfigHook("Functions", g_pSignatures);
}

bool DHooks::SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlength, bool late)
{
	if(!SetupHookManager(ismm))
	{
		snprintf(error, maxlength, "Failed to get IHookManagerAutoGen iface");	
		return false;
	}
	return true;
}

void DHooks::OnPluginUnloaded(IPlugin *plugin)
{
	CleanupHooks(plugin->GetBaseContext());
	RemoveAllCallbacksForContext(plugin->GetBaseContext());
	if(g_pEntityListener)
	{
		g_pEntityListener->CleanupListeners(plugin->GetBaseContext());
	}
}
// The next 3 functions handle cleanup if our interfaces are going to be unloaded
bool DHooks::QueryRunning(char *error, size_t maxlength)
{
	SM_CHECK_IFACE(SDKTOOLS, g_pSDKTools);
	SM_CHECK_IFACE(BINTOOLS, g_pBinTools);
	SM_CHECK_IFACE(SDKHOOKS, g_pSDKHooks);
	return true;
}
void DHooks::NotifyInterfaceDrop(SMInterface *pInterface)
{
	if(strcmp(pInterface->GetInterfaceName(), SMINTERFACE_SDKHOOKS_NAME) == 0)
	{
		if(g_pEntityListener)
		{
			// If this fails, remove this line and just delete the ent listener instead
			g_pSDKHooks->RemoveEntityListener(g_pEntityListener);

			g_pEntityListener->CleanupListeners();
			delete g_pEntityListener;
			g_pEntityListener = NULL;
		}
		g_pSDKHooks = NULL;
	}
	else if(strcmp(pInterface->GetInterfaceName(), SMINTERFACE_BINTOOLS_NAME) == 0)
	{
		g_pBinTools = NULL;
	}
	else if(strcmp(pInterface->GetInterfaceName(), SMINTERFACE_SDKTOOLS_NAME) == 0)
	{
		g_pSDKTools = NULL;
	}
}
