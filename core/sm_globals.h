/**
 * vim: set ts=4 sw=4 :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2016 AlliedModders LLC.  All rights reserved.
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

#ifndef _INCLUDE_SOURCEMOD_GLOBALS_H_
#define _INCLUDE_SOURCEMOD_GLOBALS_H_

/**
 * @file Contains global headers that most files in SourceMod will need.
 */

#include <sp_vm_types.h>
#include <sp_vm_api.h>
#include "sm_platform.h"
#include <IShareSys.h>

using namespace SourcePawn;
using namespace SourceMod;

class IServerPluginCallbacks;

/**
 * @brief Lists result codes possible from attempting to set a core configuration option.
 */
enum ConfigResult
{
	ConfigResult_Accept = 0,	/**< Config option was successfully set */
	ConfigResult_Reject = 1,	/**< Config option was given an invalid value and was rejected */
	ConfigResult_Ignore = 2		/**< Config option was invalid, but ignored */
};

/**
 * @brief Lists possible sources of a config option change
 */
enum ConfigSource
{
	ConfigSource_File = 0,		/**< Config option was set from config file (core.cfg) */
	ConfigSource_Console = 1,	/**< Config option was set from console command (sm config) */
};

// Must match clients.inc
enum class AuthIdType
{
	Engine = 0,
	Steam2,
	Steam3,
	SteamId64,
};

/** 
 * @brief Any class deriving from this will be automatically initiated/shutdown by SourceMod
 */
class SMGlobalClass
{
	friend class SourceMod_Core;
	friend class SourceModBase;
	friend class CoreConfig;
	friend class CExtensionManager;
	friend class PlayerManager;
public:
	SMGlobalClass()
	{
		m_pGlobalClassNext = SMGlobalClass::head;
		SMGlobalClass::head = this;
	}
public:
	/**
	 * @brief Called when SourceMod is initially loading
	 */
	virtual void OnSourceModStartup(bool late)
	{
	}

	/**
	 * @brief Called after all global classes have been started up
	 */
	virtual void OnSourceModAllInitialized()
	{
	}

	/**
	 * @brief Called after all global classes have initialized
	 */
	virtual void OnSourceModAllInitialized_Post()
	{
	}

	/**
	 * @brief Called when SourceMod is shutting down
	 */
	virtual void OnSourceModShutdown()
	{
	}

	/**
	 * @brief Called after SourceMod is completely shut down
	 */
	virtual void OnSourceModAllShutdown()
	{
	}

	/**
	 * @brief Called when a core config option is changed.
	 * @note This is called once BEFORE OnSourceModStartup() when SourceMod is loading
	 * @note It can then be called again when the 'sm config' command is used
	 */
	virtual ConfigResult OnSourceModConfigChanged(const char *key, 
												  const char *value, 
												  ConfigSource source,
												  char *error, 
												  size_t maxlength)
	{
		return ConfigResult_Ignore;
	}

	/**
	 * @brief Called when the level changes.
	 */
	virtual void OnSourceModLevelChange(const char *mapName)
	{
	}

	/**
	 * @brief Called when the level has activated.
	 */
	virtual void OnSourceModLevelActivated()
	{
	}

	/**
	 * @brief Called when the level ends.
	 */
	virtual void OnSourceModLevelEnd()
	{
	}

	/**
	 * @brief Called after plugins are loaded on mapchange.
	 */
	virtual void OnSourceModPluginsLoaded()
	{
	}

	/**
	 * @brief Called once all MM:S plugins are loaded.
	 */
	virtual void OnSourceModGameInitialized()
	{
	}

	/**
	 * @brief Called when an identity is dropped (right now, extensions only)
	 */
	virtual void OnSourceModIdentityDropped(IdentityToken_t *pToken)
	{
	}

	/**
	 * @brief Called when the server maxplayers changes
	 *
	 * @param newvalue		New maxplayers value.
	 */
	virtual void OnSourceModMaxPlayersChanged(int newvalue)
	{
	}
public:
	SMGlobalClass *m_pGlobalClassNext;
	static SMGlobalClass *head;
};

extern ISourcePawnEngine *g_pSourcePawn;
extern ISourcePawnEngine2 *g_pSourcePawn2;
extern IdentityToken_t *g_pCoreIdent;

namespace SourceMod
{
	class IThreader;
	class ITextParsers;
}

extern IThreader *g_pThreader;
extern ITextParsers *textparsers;

#include "sm_autonatives.h"

#endif //_INCLUDE_SOURCEMOD_GLOBALS_H_

