/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2010 AlliedModders LLC.  All rights reserved.
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

#ifndef _INCLUDE_SOURCEMOD_PLUGINSYSTEM_H_
#define _INCLUDE_SOURCEMOD_PLUGINSYSTEM_H_

#include <time.h>
#include <errno.h>
#include <sys/stat.h>
#include <IPluginSys.h>
#include <IHandleSys.h>
#include <IForwardSys.h>
#include <sh_list.h>
#include <sh_stack.h>
#include <sh_vector.h>
#include <sh_string.h>
#include "common_logic.h"
#include <sm_trie_tpl.h>
#include <IRootConsoleMenu.h>
#include "ITranslator.h"
#include "IGameConfigs.h"
#include "NativeOwner.h"
#include "ShareSys.h"

class CPlayer;

using namespace SourceHook;

/**
 * NOTES:
 *
 * UPDATE 2008-03-11: These comments are horribly out of date.  They paint a good overall 
 * picture of how PluginSys works, but things like dependencies and fake natives have 
 * complicated things quite a bit.
 *
 *  Currently this system needs a lot of work but it's good skeletally.  Plugin creation 
 * is done without actually compiling anything.  This is done by Load functions in the 
 * manager.  This will need a rewrite when we add context switching.
 *
 *  The plugin object itself has a few things to note.  The most important is that it stores
 * a table of function objects.  The manager marshals allocation and freeing of these objects.
 * The plugin object can be in erroneous states, they are:
 *   Plugin_Error   --> Some error occurred any time during or after compilation.
 *						This error can be cleared since the plugin itself is valid.
 *						However, the state itself being set prevents any runtime action.
 *   Plugin_BadLoad	--> The plugin failed to load entirely and nothing can be done to save it.
 *
 *  If a plugin fails to load externally, it is never added to the internal tracker.  However, 
 * plugins that failed to load from the internal loading mechanism are always tracked.  This 
 * allows users to see which automatically loaded plugins failed, and makes the interface a bit
 * more flexible.
 *
 *  Once a plugin is compiled, it sets its own state to Plugin_Created.  This state is still invalid
 * for execution.  SourceMod is a two pass system, and even though the second pass is not implemented
 * yet, it is structured so Plugin_Created must be switched to Plugin_Running in the second pass.  When
 * implemented, a Created plugin will be switched to Error in the second pass if it not loadable.
 *
 *  The two pass loading mechanism is described below.  Modules/natives are not implemented yet.
 * PASS ONE: All loadable plugins are found and have the following steps performed:
 *			 1.  Loading and compilation is attempted.
 *			 2.  If successful, all natives from Core are added.
 *			 3.  OnPluginLoad() is called.
 *			 4.  If failed, any user natives are scrapped and the process halts here.
 *			 5.  If successful, the plugin is ready for Pass 2.
 * INTERMEDIATE:
 *			 1.  All forced modules are loaded.
 * PASS TWO: All loaded plugins are found and have these steps performed:
 *			 1. Any modules referenced in the plugin that are not already loaded, are loaded.
 *			 2. If any module fails to load and the plugin requires it, load fails and jump to step 6.
 *			 3. If any natives are unresolved, check if they are found in the user-natives pool.
 *			 4. If yes, load succeeds.  If not, natives are passed through a native acceptance filter.
 *			 5. If the filter fails, the plugin is marked as failed.
 *			 6. If the plugin has failed to load at this point, any dynamic natives it has added are scrapped.
 *			    Furthermore, any plugin that referenced these natives must now have pass 2 re-ran.
 * PASS THREE (not a real pass):
 *			 7. Once all plugins are deemed to be loaded, OnPluginStart() is called
 */

enum LoadRes
{
	LoadRes_Successful,
	LoadRes_AlreadyLoaded,
	LoadRes_Failure,
	LoadRes_SilentFailure,
	LoadRes_NeverLoad
};

enum APLRes
{
	APLRes_Success,
	APLRes_Failure,
	APLRes_SilentFailure
};

class CPlugin : 
	public SMPlugin,
	public CNativeOwner
{
	friend class CPluginManager;
	friend class CFunction;
public:
	CPlugin(const char *file);
	~CPlugin();
public:
	PluginType GetType();
	SourcePawn::IPluginContext *GetBaseContext();
	sp_context_t *GetContext();
	void *GetPluginStructure();
	const char *GetFilename();
	bool IsDebugging();
	PluginStatus GetStatus();
	bool IsSilentlyFailed();
	void SetSilentlyFailed(bool sf);
	const sm_plugininfo_t *GetPublicInfo();
	bool SetPauseState(bool paused);
	unsigned int GetSerial();
	IdentityToken_t *GetIdentity();
	unsigned int CalcMemUsage();
	bool SetProperty(const char *prop, void *ptr);
	bool GetProperty(const char *prop, void **ptr, bool remove=false);
	void DropEverything();
	SourcePawn::IPluginRuntime *GetRuntime();
	CNativeOwner *ToNativeOwner() {
		return this;
	}
public:
	/**
	 * Creates a plugin object with default values.
	 *   If an error buffer is specified, and an error occurs, the error will be copied to the buffer
	 * and NULL will be returned.
	 *   If an error buffer is not specified, the error will be copied to an internal buffer and 
	 * a valid (but error-stated) CPlugin will be returned.
	 */
	static CPlugin *CreatePlugin(const char *file, char *error, size_t maxlength);
public:

	/**
	 * Sets an error state on the plugin
	 */
	void SetErrorState(PluginStatus status, const char *error_fmt, ...);

	/**
	 * Initializes the plugin's identity information
	 */
	void InitIdentity();

	/**
	 * Calls the OnPluginLoad function, and sets any failed states if necessary.
	 * NOTE: Valid pre-states are: Plugin_Created
	 * NOTE: If validated, plugin state is changed to Plugin_Loaded
	 *
	 * If the error buffer is NULL, the error message is cached locally.
	 */
	APLRes Call_AskPluginLoad(char *error, size_t maxlength);

	/**
	 * Calls the OnPluginStart function.
	 * NOTE: Valid pre-states are: Plugin_Created
	 * NOTE: Post-state will be Plugin_Running
	 */
	void Call_OnPluginStart();

	/**
	 * Calls the OnPluginEnd function.
	 */
	void Call_OnPluginEnd();

	/**
	 * Calls the OnAllPluginsLoaded function.
	 */
	void Call_OnAllPluginsLoaded();

	/**
	 * Returns true if a plugin is usable.
	 */
	bool IsRunnable();

	/**
	 * Get languages info.
	 */
	IPhraseCollection *GetPhrases();
public:
	/**
	 * Returns the modification time during last plugin load.
	 */
	time_t GetTimeStamp();

	/**
	 * Returns the current modification time of the plugin file.
	 */
	time_t GetFileTimeStamp();

	/** 
	 * Returns true if the plugin was running, but is now invalid.
	 */
	bool WasRunning();

	Handle_t GetMyHandle();

	bool AddFakeNative(IPluginFunction *pFunc, const char *name, SPVM_FAKENATIVE_FUNC func);
	void AddConfig(bool autoCreate, const char *cfg, const char *folder);
	unsigned int GetConfigCount();
	AutoConfig *GetConfig(unsigned int i);
	inline void AddLibrary(const char *name)
	{
		m_Libraries.push_back(name);
	}
	void LibraryActions(LibraryAction action);
	void SyncMaxClients(int max_clients);
protected:
	bool UpdateInfo();
	void SetTimeStamp(time_t t);
	void DependencyDropped(CPlugin *pOwner);
private:
	PluginType m_type;
	char m_filename[PLATFORM_MAX_PATH];
	PluginStatus m_status;
	bool m_bSilentlyFailed;
	unsigned int m_serial;
	sm_plugininfo_t m_info;
	char m_errormsg[256];
	time_t m_LastAccess;
	IdentityToken_t *m_ident;
	Handle_t m_handle;
	bool m_WasRunning;
	IPhraseCollection *m_pPhrases;
	List<String> m_RequiredLibs;
	List<String> m_Libraries;
	KTrie<void *> m_pProps;
	bool m_FakeNativesMissing;
	bool m_LibraryMissing;
	CVector<AutoConfig *> m_configs;
	bool m_bGotAllLoaded;
	int m_FileVersion;
	char m_DateTime[256];
	IPluginRuntime *m_pRuntime;
	IPluginContext *m_pContext;
	sp_pubvar_t *m_MaxClientsVar;
};

class CPluginManager : 
	public IScriptManager,
	public SMGlobalClass,
	public IHandleTypeDispatch,
	public IRootConsoleCommand
{
	friend class CPlugin;
public:
	CPluginManager();
	~CPluginManager();
public:
	/* Implements iterator class */
	class CPluginIterator : public IPluginIterator
	{
	public:
		CPluginIterator(List<CPlugin *> *mylist);
		virtual ~CPluginIterator();
		virtual bool MorePlugins();
		virtual IPlugin *GetPlugin();
		virtual void NextPlugin();
		void Release();
	public:
		void Reset();
	private:
		List<CPlugin *> *mylist;
		List<CPlugin *>::iterator current;
	};
	friend class CPluginManager::CPluginIterator;
public: //IScriptManager
	IPlugin *LoadPlugin(const char *path, 
								bool debug,
								PluginType type,
								char error[],
								size_t maxlength,
								bool *wasloaded);
	bool UnloadPlugin(IPlugin *plugin);
	IPlugin *FindPluginByContext(const sp_context_t *ctx);
	unsigned int GetPluginCount();
	IPluginIterator *GetPluginIterator();
	void AddPluginsListener(IPluginsListener *listener);
	void RemovePluginsListener(IPluginsListener *listener);
	void LoadAll(const char *config_path, const char *plugins_path);
	void RefreshAll();
	SMPlugin *FindPluginByOrder(unsigned num) {
		return GetPluginByOrder(num);
	}
	SMPlugin *FindPluginByIdentity(IdentityToken_t *ident) {
		return GetPluginFromIdentity(ident);
	}
	SMPlugin *FindPluginByContext(IPluginContext *ctx) {
		return GetPluginByCtx(ctx->GetContext());
	}
	SMPlugin *FindPluginByContext(sp_context_t *ctx) {
		return GetPluginByCtx(ctx);
	}
	SMPlugin *FindPluginByConsoleArg(const char *text);
	SMPlugin *FindPluginByHandle(Handle_t hndl, HandleError *errp) {
		return static_cast<SMPlugin *>(PluginFromHandle(hndl, errp));
	}
	void ListPlugins(CVector<SMPlugin *> *plugins);
public: //SMGlobalClass
	void OnSourceModAllInitialized();
	void OnSourceModShutdown();
	ConfigResult OnSourceModConfigChanged(const char *key, const char *value, ConfigSource source, char *error, size_t maxlength);
	void OnSourceModMaxPlayersChanged(int newvalue);
public: //IHandleTypeDispatch
	void OnHandleDestroy(HandleType_t type, void *object);
	bool GetHandleApproxSize(HandleType_t type, void *object, unsigned int *pSize);
public: //IRootConsoleCommand
	void OnRootConsoleCommand(const char *cmdname, const CCommand &command);
public:
	/**
	 * Loads all plugins not yet loaded
	 */
	void LoadAll_FirstPass(const char *config, const char *basedir);

	/**
	 * Runs the second loading pass for all plugins
	 */
	void LoadAll_SecondPass();

	/**
	 * Tests a plugin file mask against a local folder.
	 * The alias is searched backwards from localdir - i.e., given this input:
	 *   csdm/ban        csdm/ban
	 *   ban             csdm/ban
	 *   csdm/ban        optional/csdm/ban
	 * All of these will return true for an alias match.  
	 * Wildcards are allowed in the filename.
	 */
	bool TestAliasMatch(const char *alias, const char *localdir);

	/** 
	 * Returns whether anything loaded will be a late load.
	 */
	bool IsLateLoadTime() const;

	/**
	 * Converts a Handle to an IPlugin if possible.
	 */
	IPlugin *PluginFromHandle(Handle_t handle, HandleError *err);

	/**
	 * Finds a plugin based on its index. (starts on index 1)
	 */
	CPlugin *GetPluginByOrder(int num);

	int GetOrderOfPlugin(IPlugin *pl);

	/** 
	 * Internal version of FindPluginByContext()
	 */
	CPlugin *GetPluginByCtx(const sp_context_t *ctx);

	/**
	 * Gets status text for a status code 
	 */
	const char *GetStatusText(PluginStatus status);

	/**
	 * Add public functions from all running or paused
	 * plugins to the specified forward if the names match.
	 */
	void AddFunctionsToForward(const char *name, IChangeableForward *pForward);

	/**
	 * Iterates through plugins to call OnAllPluginsLoaded.
	 */
	void AllPluginsLoaded();

	CPlugin *GetPluginFromIdentity(IdentityToken_t *pToken);

	void Shutdown();

	void OnLibraryAction(const char *lib, LibraryAction action);

	bool LibraryExists(const char *lib);

	bool ReloadPlugin(CPlugin *pl);

	void UnloadAll();

	void SyncMaxClients(int max_clients);

	void ListPluginsToClient(CPlayer *player, const CCommand &args);
private:
	LoadRes _LoadPlugin(CPlugin **pPlugin, const char *path, bool debug, PluginType type, char error[], size_t maxlength);

	void LoadAutoPlugin(const char *plugin);

	/**
	 * Recursively loads all plugins in the given directory.
	 */
	void LoadPluginsFromDir(const char *basedir, const char *localdir);

	/**
	 * Adds a plugin object.  This is wrapped by LoadPlugin functions.
	 */
	void AddPlugin(CPlugin *pPlugin);

	/**
	 * Runs the second loading pass on a plugin.
	 */
	bool RunSecondPass(CPlugin *pPlugin, char *error, size_t maxlength);

	/**
	 * Runs an extension pass on a plugin.
	 */
	bool LoadOrRequireExtensions(CPlugin *pPlugin, unsigned int pass, char *error, size_t maxlength);

	/**
	* Manages required natives.
	*/
	bool FindOrRequirePluginDeps(CPlugin *pPlugin, char *error, size_t maxlength);

	void _SetPauseState(CPlugin *pPlugin, bool pause);
protected:
	/**
	 * Caching internal objects
	 */
	void ReleaseIterator(CPluginIterator *iter);
public:
	inline IdentityToken_t *GetIdentity()
	{
		return m_MyIdent;
	}
	IPluginManager *GetOldAPI();
private:
	void TryRefreshDependencies(CPlugin *pOther);
private:
	List<IPluginsListener *> m_listeners;
	List<CPlugin *> m_plugins;
	CStack<CPluginManager::CPluginIterator *> m_iters;
	KTrie<CPlugin *> m_LoadLookup;
	bool m_AllPluginsLoaded;
	IdentityToken_t *m_MyIdent;

	/* Dynamic native stuff */
	List<FakeNative *> m_Natives;

	bool m_LoadingLocked;
	
	// Config
	bool m_bBlockBadPlugins;
	
	// Forwards
	IForward *m_pOnLibraryAdded;
	IForward *m_pOnLibraryRemoved;
};

extern CPluginManager g_PluginSys;

#endif //_INCLUDE_SOURCEMOD_PLUGINSYSTEM_H_

