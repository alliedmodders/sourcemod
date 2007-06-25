/**
 * vim: set ts=4 :
 * ===============================================================
 * SourceMod, Copyright (C) 2004-2007 AlliedModders LLC. 
 * All rights reserved.
 * ===============================================================
 *
 *  This file is part of the SourceMod/SourcePawn SDK.  This file may only be 
 * used or modified under the Terms and Conditions of its License Agreement, 
 * which is found in public/licenses/LICENSE.txt.  As of this notice, derivative 
 * works must be licensed under the GNU General Public License (version 2 or 
 * greater).  A copy of the GPL is included under public/licenses/GPL.txt.
 * 
 * To view the latest information, see: http://www.sourcemod.net/license.php
 *
 * Version: $Id$
 */

#include <stdio.h>
#include <malloc.h>
#include "smsdk_ext.h"

/**
 * @file smsdk_ext.cpp
 * @brief Contains wrappers for making Extensions easier to write.
 */

IExtension *myself = NULL;				/**< Ourself */
IShareSys *g_pShareSys = NULL;			/**< Share system */
IShareSys *sharesys = NULL;				/**< Share system */
ISourceMod *g_pSM = NULL;				/**< SourceMod helpers */
ISourceMod *smutils = NULL;				/**< SourceMod helpers */

#if defined SMEXT_ENABLE_FORWARDSYS
IForwardManager *g_pForwards = NULL;	/**< Forward system */
IForwardManager *forwards = NULL;		/**< Forward system */
#endif
#if defined SMEXT_ENABLE_HANDLESYS
IHandleSys *g_pHandleSys = NULL;		/**< Handle system */
IHandleSys *handlesys = NULL;			/**< Handle system */
#endif
#if defined SMEXT_ENABLE_PLAYERHELPERS
IPlayerManager *playerhelpers = NULL;	/**< Player helpers */
#endif //SMEXT_ENABLE_PLAYERHELPERS
#if defined SMEXT_ENABLE_DBMANAGER
IDBManager *dbi = NULL;					/**< DB Manager */
#endif //SMEXT_ENABLE_DBMANAGER
#if defined SMEXT_ENABLE_GAMECONF
IGameConfigManager *gameconfs = NULL;	/**< Game config manager */
#endif //SMEXT_ENABLE_DBMANAGER
#if defined SMEXT_ENABLE_MEMUTILS
IMemoryUtils *memutils = NULL;
#endif //SMEXT_ENABLE_DBMANAGER
#if defined SMEXT_ENABLE_GAMEHELPERS
IGameHelpers *gamehelpers = NULL;
#endif
#if defined SMEXT_ENABLE_TIMERSYS
ITimerSystem *timersys = NULL;
#endif

/** Exports the main interface */
PLATFORM_EXTERN_C IExtensionInterface *GetSMExtAPI()
{
	return g_pExtensionIface;
}

SDKExtension::SDKExtension()
{
#if defined SMEXT_CONF_METAMOD
	m_SourceMMLoaded = false;
	m_WeAreUnloaded = false;
	m_WeGotPauseChange = false;
#endif
}

bool SDKExtension::OnExtensionLoad(IExtension *me, IShareSys *sys, char *error, size_t maxlength, bool late)
{
	g_pShareSys = sharesys = sys;
	myself = me;

#if defined SMEXT_CONF_METAMOD
	m_WeAreUnloaded = true;

	if (!m_SourceMMLoaded)
	{
		if (error)
		{
			snprintf(error, maxlength, "Metamod attach failed");
		}
		return false;
	}
#endif
	SM_GET_IFACE(SOURCEMOD, g_pSM);
	smutils = g_pSM;
#if defined SMEXT_ENABLE_HANDLESYS
	SM_GET_IFACE(HANDLESYSTEM, g_pHandleSys);
	handlesys = g_pHandleSys;
#endif
#if defined SMEXT_ENABLE_FORWARDSYS
	SM_GET_IFACE(FORWARDMANAGER, g_pForwards);
	forwards = g_pForwards;
#endif
#if defined SMEXT_ENABLE_PLAYERHELPERS
	SM_GET_IFACE(PLAYERMANAGER, playerhelpers);
#endif
#if defined SMEXT_ENABLE_DBMANAGER
	SM_GET_IFACE(DBI, dbi);
#endif
#if defined SMEXT_ENABLE_GAMECONF
	SM_GET_IFACE(GAMECONFIG, gameconfs);
#endif
#if defined SMEXT_ENABLE_MEMUTILS
	SM_GET_IFACE(MEMORYUTILS, memutils);
#endif
#if defined SMEXT_ENABLE_GAMEHELPERS
	SM_GET_IFACE(GAMEHELPERS, gamehelpers);
#endif
#if defined SMEXT_ENABLE_TIMERSYS
	SM_GET_IFACE(TIMERSYS, timersys);
#endif

	if (SDK_OnLoad(error, maxlength, late))
	{
#if defined SMEXT_CONF_METAMOD
		m_WeAreUnloaded = true;
#endif
		return true;
	}

	return false;
}

bool SDKExtension::IsMetamodExtension()
{
#if defined SMEXT_CONF_METAMOD
	return true;
#else
	return false;
#endif
}

void SDKExtension::OnExtensionPauseChange(bool state)
{
#if defined SMEXT_CONF_METAMOD
	m_WeGotPauseChange = true;
#endif
	SDK_OnPauseChange(state);
}

void SDKExtension::OnExtensionsAllLoaded()
{
	SDK_OnAllLoaded();
}

void SDKExtension::OnExtensionUnload()
{
#if defined SMEXT_CONF_METAMOD
	m_WeAreUnloaded = true;
#endif
	SDK_OnUnload();
}

const char *SDKExtension::GetExtensionAuthor()
{
	return SMEXT_CONF_AUTHOR;
}

const char *SDKExtension::GetExtensionDateString()
{
	return SMEXT_CONF_DATESTRING;
}

const char *SDKExtension::GetExtensionDescription()
{
	return SMEXT_CONF_DESCRIPTION;
}

const char *SDKExtension::GetExtensionVerString()
{
	return SMEXT_CONF_VERSION;
}

const char *SDKExtension::GetExtensionName()
{
	return SMEXT_CONF_NAME;
}

const char *SDKExtension::GetExtensionTag()
{
	return SMEXT_CONF_LOGTAG;
}

const char *SDKExtension::GetExtensionURL()
{
	return SMEXT_CONF_URL;
}

bool SDKExtension::SDK_OnLoad(char *error, size_t maxlength, bool late)
{
	return true;
}

void SDKExtension::SDK_OnUnload()
{
}

void SDKExtension::SDK_OnPauseChange(bool paused)
{
}

void SDKExtension::SDK_OnAllLoaded()
{
}

#if defined SMEXT_CONF_METAMOD

PluginId g_PLID = 0;						/**< Metamod plugin ID */
ISmmPlugin *g_PLAPI = NULL;					/**< Metamod plugin API */
SourceHook::ISourceHook *g_SHPtr = NULL;	/**< SourceHook pointer */
ISmmAPI *g_SMAPI = NULL;					/**< SourceMM API pointer */

IVEngineServer *engine = NULL;				/**< IVEngineServer pointer */
IServerGameDLL *gamedll = NULL;				/**< IServerGameDLL pointer */

/** Exposes the extension to Metamod */
SMM_API void *PL_EXPOSURE(const char *name, int *code)
{
	if (name && !strcmp(name, PLAPI_NAME))
	{
		if (code)
		{
			*code = IFACE_OK;
		}
		return static_cast<void *>(g_pExtensionIface);
	}

	if (code)
	{
		*code = IFACE_FAILED;
	}

	return NULL;
}

bool SDKExtension::Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	PLUGIN_SAVEVARS();

	GET_V_IFACE_ANY(serverFactory, gamedll, IServerGameDLL, INTERFACEVERSION_SERVERGAMEDLL);
	GET_V_IFACE_CURRENT(engineFactory, engine, IVEngineServer, INTERFACEVERSION_VENGINESERVER);

	m_SourceMMLoaded = true;

	return SDK_OnMetamodLoad(ismm, error, maxlen, late);
}

bool SDKExtension::Unload(char *error, size_t maxlen)
{
	if (!m_WeAreUnloaded)
	{
		if (error)
		{
			snprintf(error, maxlen, "This extension must be unloaded by SourceMod.");
		}
		return false;
	}

	return SDK_OnMetamodUnload(error, maxlen);
}

bool SDKExtension::Pause(char *error, size_t maxlen)
{
	if (!m_WeGotPauseChange)
	{
		if (error)
		{
			snprintf(error, maxlen, "This extension must be paused by SourceMod.");
		}
		return false;
	}

	m_WeGotPauseChange = false;

	return SDK_OnMetamodPauseChange(true, error, maxlen);
}

bool SDKExtension::Unpause(char *error, size_t maxlen)
{
	if (!m_WeGotPauseChange)
	{
		if (error)
		{
			snprintf(error, maxlen, "This extension must be unpaused by SourceMod.");
		}
		return false;
	}

	m_WeGotPauseChange = false;

	return SDK_OnMetamodPauseChange(false, error, maxlen);
}

const char *SDKExtension::GetAuthor()
{
	return GetExtensionAuthor();
}

const char *SDKExtension::GetDate()
{
	return GetExtensionDateString();
}

const char *SDKExtension::GetDescription()
{
	return GetExtensionDescription();
}

const char *SDKExtension::GetLicense()
{
	return SMEXT_CONF_LICENSE;
}

const char *SDKExtension::GetLogTag()
{
	return GetExtensionTag();
}

const char *SDKExtension::GetName()
{
	return GetExtensionName();
}

const char *SDKExtension::GetURL()
{
	return GetExtensionURL();
}

const char *SDKExtension::GetVersion()
{
	return GetExtensionVerString();
}

bool SDKExtension::SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlength, bool late)
{
	return true;
}

bool SDKExtension::SDK_OnMetamodUnload(char *error, size_t maxlength)
{
	return true;
}

bool SDKExtension::SDK_OnMetamodPauseChange(bool paused, char *error, size_t maxlength)
{
	return true;
}

#endif

/* Overload a few things to prevent libstdc++ linking */
#if defined __linux__
extern "C" void __cxa_pure_virtual(void)
{
}

void *operator new(size_t size)
{
	return malloc(size);
}

void *operator new[](size_t size) 
{
	return malloc(size);
}

void operator delete(void *ptr) 
{
	free(ptr);
}

void operator delete[](void * ptr)
{
	free(ptr);
}
#endif
