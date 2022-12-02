/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * =============================================================================
 * SourceMod
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

#ifndef _INCLUDE_SOURCEMOD_CONVARMANAGER_H_
#define _INCLUDE_SOURCEMOD_CONVARMANAGER_H_

#include "sm_globals.h"
#include "sourcemm_api.h"
#include <sh_list.h>
#include <IPluginSys.h>
#include <IForwardSys.h>
#include <IHandleSys.h>
#include <IRootConsoleMenu.h>
#include <IPlayerHelpers.h>
#include <compat_wrappers.h>
#include "concmd_cleaner.h"
#include "PlayerManager.h"
#include <sm_stringhashmap.h>

using namespace SourceHook;

class IConVarChangeListener
{
public:
	virtual void OnConVarChanged(ConVar *pConVar, const char *oldValue, float flOldValue) =0;
};

/**
 * Holds SourceMod-specific information about a convar
 */
struct ConVarInfo
{
	Handle_t handle;					/**< Handle to self */
	bool sourceMod;						/**< Determines whether or not convar was created by a SourceMod plugin */
	IChangeableForward *pChangeForward;	/**< Forward associated with convar */
	ConVar *pVar;						/**< The actual convar */
	IPlugin *pPlugin; 					/**< Originally owning plugin */
	List<IConVarChangeListener *> changeListeners;

	struct ConVarPolicy
	{
		static inline bool matches(const char *name, ConVarInfo *info)
		{
			const char *conVarChars = info->pVar->GetName();

			std::string convarName = ke::Lowercase(conVarChars);
			std::string input = ke::Lowercase(name);

			return convarName == input;
		}

		static inline uint32_t hash(const detail::CharsAndLength &key)
		{
			std::string lower = ke::Lowercase(key.c_str());
			return detail::CharsAndLength(lower.c_str()).hash();
		}
	};
};

/**
 * Holds information about a client convar query
 */
struct ConVarQuery
{
	QueryCvarCookie_t cookie;			/**< Cookie that identifies query */
	IPluginFunction *pCallback;			/**< Function that will be called when query is finished */
	cell_t value;						/**< Optional value passed to query function */
	cell_t client;						/**< Only used for cleaning up on client disconnection */
};

class ConVarManager :
	public SMGlobalClass,
	public IHandleTypeDispatch,
	public IPluginsListener,
	public IRootConsoleCommand,
	public IConCommandTracker,
	public IClientListener
{
public:
	ConVarManager();
	~ConVarManager();
public: // SMGlobalClass
	void OnSourceModStartup(bool late);
	void OnSourceModAllInitialized();
	void OnSourceModShutdown();
public: // IHandleTypeDispatch
	void OnHandleDestroy(HandleType_t type, void *object);
	bool GetHandleApproxSize(HandleType_t type, void *object, unsigned int *pSize);
public: // IPluginsListener
	void OnPluginUnloaded(IPlugin *plugin);
public: //IRootConsoleCommand
	void OnRootConsoleCommand(const char *cmdname, const ICommandArgs *command) override;
public: //IConCommandTracker
	void OnUnlinkConCommandBase(ConCommandBase *pBase, const char *name) override;
public: //IClientListener
	void OnClientDisconnected(int client);
public:
	/**
	 * Create a convar and return a handle to it.
	 */
	Handle_t CreateConVar(IPluginContext *pContext, const char *name, const char *defaultVal,
	                      const char *description, int flags, bool hasMin, float min, bool hasMax, float max);

	/**
	 * Searches for a convar and returns a handle to it
	 */
	Handle_t FindConVar(const char* name);

	/**
	 * Add a function to call when the specified convar changes.
	 */
	void HookConVarChange(ConVar *pConVar, IPluginFunction *pFunction);

	/**
	 * Remove a function from the forward that will be called when the specified convar changes.
	 */
	void UnhookConVarChange(ConVar *pConVar, IPluginFunction *pFunction);

	void AddConVarChangeListener(const char *name, IConVarChangeListener *pListener);
	void RemoveConVarChangeListener(const char *name, IConVarChangeListener *pListener);

	/**
	 * Starts a query to find the value of a client convar.
	 */
	QueryCvarCookie_t QueryClientConVar(edict_t *pPlayer, const char *name, IPluginFunction *pCallback,
	                                    Handle_t hndl);

	bool IsQueryingSupported();

	HandleError ReadConVarHandle(Handle_t hndl, ConVar **pVar, IPlugin **ppPlugin = nullptr);

	// Called via game hooks.
	void OnConVarChanged(ConVar *pConVar, const char *oldValue, float flOldValue);
#if SOURCE_ENGINE != SE_DARKMESSIAH
	void OnClientQueryFinished(
	  QueryCvarCookie_t cookie,
	  int client,
	  EQueryCvarValueStatus result,
	  const char *cvarName,
	  const char *cvarValue);
#endif

private:
	/**
	 * Adds a convar to a plugin's list.
	 */
	static void AddConVarToPluginList(IPlugin *plugin, const ConVar *pConVar);
private:
	HandleType_t m_ConVarType;
	List<ConVarInfo *> m_ConVars;
	List<ConVarQuery> m_ConVarQueries;
};

extern ConVarManager g_ConVarManager;

#endif // _INCLUDE_SOURCEMOD_CONVARMANAGER_H_

