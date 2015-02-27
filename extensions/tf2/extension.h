/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Team Fortress 2 Extension
 * Copyright (C) 2004-2015 AlliedModders LLC.  All rights reserved.
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

#ifndef _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_

/**
 * @file extension.h
 * @brief Sample extension code header.
 */

#include "smsdk_ext.h"
#include <IBinTools.h>
#include <server_class.h>
#include <igameevents.h>

namespace SourceMod {
	class ISDKTools;
}

/**
 * @brief Sample implementation of the SDK Extension.
 * Note: Uncomment one of the pre-defined virtual functions in order to use it.
 */
class TF2Tools : 
	public SDKExtension,
	public ICommandTargetProcessor,
	public IConCommandBaseAccessor,
	public IGameEventListener2,
	public IPluginsListener
{
public: //SDKExtension
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
	 * @brief This is called right before the extension is unloaded.
	 */
	virtual void SDK_OnUnload();

	/**
	 * @brief This is called once all known extensions have been loaded.
	 * Note: It is is a good idea to add natives here, if any are provided.
	 */
	virtual void SDK_OnAllLoaded();

	/**
	 * @brief this is called when Core wants to know if your extension is working.
	 *
	 * @param error		Error message buffer.
	 * @param maxlength	Size of error message buffer.
	 * @return			True if working, false otherwise.
	 */
	virtual bool QueryRunning(char *error, size_t maxlength);

	void NotifyInterfaceDrop(SMInterface *pInterface);
	bool QueryInterfaceDrop(SMInterface *pInterface);

	const char *GetExtensionVerString();
	const char *GetExtensionDateString();
public: //ICommandTargetProcessor
	bool ProcessCommandTarget(cmd_target_info_t *info);
public: //IConCommandBaseAccessor
	bool RegisterConCommandBase(ConCommandBase *pVar);
public: //IGameEventManager
	IGameEventManager2 *m_GameEventManager;
	void FireGameEvent( IGameEvent *event );
public: //IPluginsListener
	void OnPluginLoaded(IPlugin *plugin);
	void OnPluginUnloaded(IPlugin *plugin);
public:
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
#endif
private:
	bool m_CritDetoursEnabled;
	bool m_CondChecksEnabled;
	bool m_RulesDetoursEnabled;
	bool m_TeleportDetourEnabled;
};

enum TFClassType
{
	TFClass_Unknown = 0,
	TFClass_Scout,
	TFClass_Sniper,
	TFClass_Soldier,
	TFClass_DemoMan,
	TFClass_Medic,
	TFClass_Heavy,
	TFClass_Pyro,
	TFClass_Spy,
	TFClass_Engineer
};

TFClassType ClassnameToType(const char *classname);

extern IBinTools *g_pBinTools;
extern ISDKTools *g_pSDKTools;
extern IGameConfig *g_pGameConf;
extern sm_sendprop_info_t *playerSharedOffset;

extern CGlobalVars *gpGlobals;

void OnServerActivate(edict_t *pEdictList, int edictCount, int clientMax);

int FindResourceEntity();
int FindEntityByNetClass(int start, const char *classname);

extern int g_resourceEntity;


#endif // _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_
