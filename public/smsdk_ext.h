/**
 * vim: set ts=4 sw=4 tw=99 noet:
 * =============================================================================
 * SourceMod Base Extension Code
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

#ifndef _INCLUDE_SOURCEMOD_EXTENSION_BASESDK_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_BASESDK_H_

/**
 * @file smsdk_ext.h
 * @brief Contains wrappers for making Extensions easier to write.
 */

#include "smsdk_config.h"
#include <IExtensionSys.h>
#include <IHandleSys.h>
#include <sp_vm_api.h>
#include <sm_platform.h>
#include <ISourceMod.h>
#include "am-string.h"
#if defined SMEXT_ENABLE_FORWARDSYS
#include <IForwardSys.h>
#endif //SMEXT_ENABLE_FORWARDSYS
#if defined SMEXT_ENABLE_PLAYERHELPERS
#include <IPlayerHelpers.h>
#endif //SMEXT_ENABLE_PlAYERHELPERS
#if defined SMEXT_ENABLE_DBMANAGER
#include <IDBDriver.h>
#endif //SMEXT_ENABLE_DBMANAGER
#if defined SMEXT_ENABLE_GAMECONF
#include <IGameConfigs.h>
#endif
#if defined SMEXT_ENABLE_MEMUTILS
#include <IMemoryUtils.h>
#endif
#if defined SMEXT_ENABLE_GAMEHELPERS
#include <IGameHelpers.h>
#endif
#if defined SMEXT_ENABLE_TIMERSYS
#include <ITimerSystem.h>
#endif
#if defined SMEXT_ENABLE_ADTFACTORY
#include <IADTFactory.h>
#endif
#if defined SMEXT_ENABLE_THREADER
#include <IThreader.h>
#endif
#if defined SMEXT_ENABLE_LIBSYS
#include <ILibrarySys.h>
#endif
#if defined SMEXT_ENABLE_PLUGINSYS
#include <IPluginSys.h>
#endif
#if defined SMEXT_ENABLE_MENUS
#include <IMenuManager.h>
#endif
#if defined SMEXT_ENABLE_ADMINSYS
#include <IAdminSystem.h>
#endif
#if defined SMEXT_ENABLE_TEXTPARSERS
#include <ITextParsers.h>
#endif
#if defined SMEXT_ENABLE_USERMSGS
#include <IUserMessages.h>
#endif
#if defined SMEXT_ENABLE_TRANSLATOR
#include <ITranslator.h>
#endif
#if defined SMEXT_ENABLE_ROOTCONSOLEMENU
#include <IRootConsoleMenu.h>
#endif

#if defined SMEXT_CONF_METAMOD
#include <ISmmPlugin.h>
#ifndef META_NO_HL2SDK
#include <eiface.h>
#endif //META_NO_HL2SDK
#endif

#if !defined METAMOD_PLAPI_VERSION
#include <metamod_wrappers.h>
#endif

using namespace SourceMod;
using namespace SourcePawn;

class SDKExtension : 
#if defined SMEXT_CONF_METAMOD
	public ISmmPlugin,
#endif
	public IExtensionInterface
{
public:
	/** Constructor */
	SDKExtension();
public:
	/**
	 * @brief This is called after the initial loading sequence has been processed.
	 *
	 * @param error		Error message buffer.
	 * @param maxlength	Size of error message buffer.
	 * @param late		Whether or not the module was loaded after map load.
	 * @return			True to succeed loading, false to fail.
	 */
	virtual bool SDK_OnLoad(char *error, size_t maxlength, bool late);
	
	/**
	 * @brief This is called once the extension unloading process begins.
	 */
	virtual void SDK_OnUnload();

	/**
	 * @brief This is called once all known extensions have been loaded.
	 */
	virtual void SDK_OnAllLoaded();

	/**
	 * @brief Called when the pause state is changed.
	 */
	virtual void SDK_OnPauseChange(bool paused);

	/**
	 * @brief Called after SDK_OnUnload, once all dependencies have been
	 * removed, and the extension is about to be removed from memory.
	 */
	virtual void SDK_OnDependenciesDropped();

#if defined SMEXT_CONF_METAMOD
	/**
	 * @brief Called when Metamod is attached, before the extension version is called.
	 *
	 * @param error			Error buffer.
	 * @param maxlength		Maximum size of error buffer.
	 * @param late			Whether or not Metamod considers this a late load.
	 * @return				True to succeed, false to fail.
	 */
	virtual bool SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlength, bool late);

	/**
	 * @brief Called when Metamod is detaching, after the extension version is called.
	 * NOTE: By default this is blocked unless sent from SourceMod.
	 *
	 * @param error			Error buffer.
	 * @param maxlength		Maximum size of error buffer.
	 * @return				True to succeed, false to fail.
	 */
	virtual bool SDK_OnMetamodUnload(char *error, size_t maxlength);

	/**
	 * @brief Called when Metamod's pause state is changing.
	 * NOTE: By default this is blocked unless sent from SourceMod.
	 *
	 * @param paused		Pause state being set.
	 * @param error			Error buffer.
	 * @param maxlength		Maximum size of error buffer.
	 * @return				True to succeed, false to fail.
	 */
	virtual bool SDK_OnMetamodPauseChange(bool paused, char *error, size_t maxlength);
#endif

public: //IExtensionInterface
	virtual bool OnExtensionLoad(IExtension *me, IShareSys *sys, char *error, size_t maxlength, bool late);
	virtual void OnExtensionUnload();
	virtual void OnExtensionsAllLoaded();

	/** Returns whether or not this is a Metamod-based extension */
	virtual bool IsMetamodExtension();

	/**
	 * @brief Called when the pause state changes.
	 * 
	 * @param state			True if being paused, false if being unpaused.
	 */
	virtual void OnExtensionPauseChange(bool state);

	/** Returns name */
	virtual const char *GetExtensionName();
	/** Returns URL */
	virtual const char *GetExtensionURL();
	/** Returns log tag */
	virtual const char *GetExtensionTag();
	/** Returns author */
	virtual const char *GetExtensionAuthor();
	/** Returns version string */
	virtual const char *GetExtensionVerString();
	/** Returns description string */
	virtual const char *GetExtensionDescription();
	/** Returns date string */
	virtual const char *GetExtensionDateString();

	/** Called after OnExtensionUnload, once dependencies have been dropped. */
	virtual void OnDependenciesDropped();
#if defined SMEXT_CONF_METAMOD
public: //ISmmPlugin
	/** Called when the extension is attached to Metamod. */
	virtual bool Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlength, bool late);
	/** Returns the author to MM */
	virtual const char *GetAuthor();
	/** Returns the name to MM */
	virtual const char *GetName();
	/** Returns the description to MM */
	virtual const char *GetDescription();
	/** Returns the URL to MM */
	virtual const char *GetURL();
	/** Returns the license to MM */
	virtual const char *GetLicense();
	/** Returns the version string to MM */
	virtual const char *GetVersion();
	/** Returns the date string to MM */
	virtual const char *GetDate();
	/** Returns the logtag to MM */
	virtual const char *GetLogTag();
	/** Called on unload */
	virtual bool Unload(char *error, size_t maxlength);
	/** Called on pause */
	virtual bool Pause(char *error, size_t maxlength);
	/** Called on unpause */
	virtual bool Unpause(char *error, size_t maxlength);
private:
	bool m_SourceMMLoaded;
	bool m_WeAreUnloaded;
	bool m_WeGotPauseChange;
#endif
};

extern SDKExtension *g_pExtensionIface;
extern IExtension *myself;

extern IShareSys *g_pShareSys;
extern IShareSys *sharesys;				/* Note: Newer name */
extern ISourceMod *g_pSM;
extern ISourceMod *smutils;				/* Note: Newer name */

/* Optional interfaces are below */
#if defined SMEXT_ENABLE_FORWARDSYS
extern IForwardManager *g_pForwards;
extern IForwardManager *forwards;		/* Note: Newer name */
#endif //SMEXT_ENABLE_FORWARDSYS
#if defined SMEXT_ENABLE_HANDLESYS
extern IHandleSys *g_pHandleSys;
extern IHandleSys *handlesys;			/* Note: Newer name */
#endif //SMEXT_ENABLE_HANDLESYS
#if defined SMEXT_ENABLE_PLAYERHELPERS
extern IPlayerManager *playerhelpers;
#endif //SMEXT_ENABLE_PLAYERHELPERS
#if defined SMEXT_ENABLE_DBMANAGER
extern IDBManager *dbi;
#endif //SMEXT_ENABLE_DBMANAGER
#if defined SMEXT_ENABLE_GAMECONF
extern IGameConfigManager *gameconfs;
#endif //SMEXT_ENABLE_DBMANAGER
#if defined SMEXT_ENABLE_MEMUTILS
extern IMemoryUtils *memutils;
#endif
#if defined SMEXT_ENABLE_GAMEHELPERS
extern IGameHelpers *gamehelpers;
#endif
#if defined SMEXT_ENABLE_TIMERSYS
extern ITimerSystem *timersys;
#endif
#if defined SMEXT_ENABLE_ADTFACTORY
extern IADTFactory *adtfactory;
#endif
#if defined SMEXT_ENABLE_THREADER
extern IThreader *threader;
#endif
#if defined SMEXT_ENABLE_LIBSYS
extern ILibrarySys *libsys;
#endif
#if defined SMEXT_ENABLE_PLUGINSYS
extern SourceMod::IPluginManager *plsys;
#endif
#if defined SMEXT_ENABLE_MENUS
extern IMenuManager *menus;
#endif
#if defined SMEXT_ENABLE_ADMINSYS
extern IAdminSystem *adminsys;
#endif
#if defined SMEXT_ENABLE_USERMSGS
extern IUserMessages *usermsgs;
#endif
#if defined SMEXT_ENABLE_TRANSLATOR
extern ITranslator *translator;
#endif
#if defined SMEXT_ENABLE_ROOTCONSOLEMENU
extern IRootConsole *rootconsole;
#endif

#if defined SMEXT_CONF_METAMOD
PLUGIN_GLOBALVARS();
#ifndef META_NO_HL2SDK
extern IVEngineServer *engine;
extern IServerGameDLL *gamedll;
#endif //META_NO_HL2SDK
#endif

/** Creates a SourceMod interface macro pair */
#define SM_MKIFACE(name) SMINTERFACE_##name##_NAME, SMINTERFACE_##name##_VERSION
/** Automates retrieving SourceMod interfaces */
#define SM_GET_IFACE(prefix, addr) \
	if (!g_pShareSys->RequestInterface(SM_MKIFACE(prefix), myself, (SMInterface **)&addr)) \
	{ \
		if (error != NULL && maxlength) \
		{ \
			size_t len = ke::SafeSprintf(error, maxlength, "Could not find interface: %s", SMINTERFACE_##prefix##_NAME); \
			if (len >= maxlength) \
			{ \
				error[maxlength - 1] = '\0'; \
			} \
		} \
		return false; \
	}
/** Automates retrieving SourceMod interfaces when needed outside of SDK_OnLoad() */
#define SM_GET_LATE_IFACE(prefix, addr) \
	g_pShareSys->RequestInterface(SM_MKIFACE(prefix), myself, (SMInterface **)&addr)
/** Validates a SourceMod interface pointer */
#define SM_CHECK_IFACE(prefix, addr) \
	if (!addr) \
	{ \
		if (error != NULL && maxlength) \
		{ \
			size_t len = ke::SafeSprintf(error, maxlength, "Could not find interface: %s", SMINTERFACE_##prefix##_NAME); \
			if (len >= maxlength) \
			{ \
				error[maxlength - 1] = '\0'; \
			} \
		} \
		return false; \
	}

#endif // _INCLUDE_SOURCEMOD_EXTENSION_BASESDK_H_
