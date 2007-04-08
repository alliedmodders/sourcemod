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

/** 
 * @brief Any class deriving from this will be automatically initiated/shutdown by SourceMod
 */
class SMGlobalClass
{
	friend class SourceMod_Core;
	friend class SourceModBase;
	friend class CoreConfig;
public:
	SMGlobalClass();
public:
	/**
	 * @brief Called when SourceMod is initially loading
	 */
	virtual void OnSourceModStartup(bool late)
	{
	}

	/**
	 * @brief Called after all global classes have initialized
	 */
	virtual void OnSourceModAllInitialized()
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
	 * @brief Called after plugins are loaded on mapchange.
	 */
	virtual void OnSourceModPluginsLoaded()
	{
	}

	/**
	 * @brief Called when SourceMod receives a pointer to IServerPluginCallbacks from SourceMM
	 */
	virtual void OnSourceModVSPReceived(IServerPluginCallbacks *iface)
	{
	}
private:
	SMGlobalClass *m_pGlobalClassNext;
	static SMGlobalClass *head;
};

extern ISourcePawnEngine *g_pSourcePawn;
extern IVirtualMachine *g_pVM;
extern IdentityToken_t *g_pCoreIdent;

#include "sm_autonatives.h"

#endif //_INCLUDE_SOURCEMOD_GLOBALS_H_
