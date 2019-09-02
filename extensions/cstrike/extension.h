/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Counter-Strike:Source Extension
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

#ifndef _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_

/**
 * @file extension.h
 * @brief Sample extension code header.
 */

#include "smsdk_ext.h"
#include "CDetour/detours.h"
#include <IBinTools.h>
#include <ISDKTools.h>

int CallPriceForward(int client, const char *weapon_name, int price);

#if SOURCE_ENGINE == SE_CSGO
#define WEAPON_C4 49
#define WEAPON_KNIFE 42
#define WEAPON_KNIFE_GG 41
#define WEAPON_KEVLAR 50
#define WEAPON_ASSAULTSUIT 51
#define WEAPON_NIGHTVISION 52
#define WEAPON_DEFUSER 53
#define WEAPON_M4 16
#else
#define WEAPON_C4 6
#define WEAPON_KNIFE 28
#define WEAPON_SHIELD 30
#define WEAPON_KEVLAR 31
#define WEAPON_ASSAULTSUIT 32
#define WEAPON_NIGHTVISION 33
#endif

#if SOURCE_ENGINE == SE_CSGO
#define WEAPONDROP_GAMEDATA_NAME "CSWeaponDropBB"
#else
#define WEAPONDROP_GAMEDATA_NAME "CSWeaponDrop"
#endif
/**
 * @brief Sample implementation of the SDK Extension.
 * Note: Uncomment one of the pre-defined virtual functions in order to use it.
 */
class CStrike : 
	public SDKExtension,
	public ICommandTargetProcessor,
	public IPluginsListener
{
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
	 * @brief This is called right before the extension is unloaded.
	 */
	virtual void SDK_OnUnload();

	/**
	 * @brief This is called once all known extensions have been loaded.
	 * Note: It is is a good idea to add natives here, if any are provided.
	 */
	virtual void SDK_OnAllLoaded();

	/**
	 * @brief Called when the pause state is changed.
	 */
	//virtual void SDK_OnPauseChange(bool paused);

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
public:
	bool ProcessCommandTarget(cmd_target_info_t *info);
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

	/**
	 * @brief Called when Metamod is detaching, after the extension version is called.
	 * NOTE: By default this is blocked unless sent from SourceMod.
	 *
	 * @param error			Error buffer.
	 * @param maxlength		Maximum size of error buffer.
	 * @return				True to succeed, false to fail.
	 */
	//virtual bool SDK_OnMetamodUnload(char *error, size_t maxlength);

	/**
	 * @brief Called when Metamod's pause state is changing.
	 * NOTE: By default this is blocked unless sent from SourceMod.
	 *
	 * @param paused		Pause state being set.
	 * @param error			Error buffer.
	 * @param maxlength		Maximum size of error buffer.
	 * @return				True to succeed, false to fail.
	 */
	//virtual bool SDK_OnMetamodPauseChange(bool paused, char *error, size_t maxlength);
#endif
private:
	bool m_WeaponPriceDetourEnabled;
	bool m_TerminateRoundDetourEnabled;
	bool m_HandleBuyDetourEnabled;
	bool m_CSWeaponDetourEnabled;
};

/* Interfaces from SourceMod */
extern IBinTools *g_pBinTools;
extern IGameConfig *g_pGameConf;
extern ISDKTools *g_pSDKTools;
extern CGlobalVars *gpGlobals;
extern int g_msgHintText;
extern bool g_pIgnoreTerminateDetour;
extern bool g_pIgnoreCSWeaponDropDetour;
extern bool g_pTerminateRoundDetoured;
extern bool g_pCSWeaponDropDetoured;
extern int weaponNameOffset;

#endif // _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_
