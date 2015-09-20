/**
 * vim: set ts=4 sw=4 tw=99 noet :
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
#include <IRootConsoleMenu.h>
#include <sm_stringhashmap.h>
#include <sm_namehashset.h>
#include "ITranslator.h"
#include "IGameConfigs.h"
#include "NativeOwner.h"
#include "ShareSys.h"
#include "PhraseCollection.h"
#include <am-string.h>
#include <bridge/include/IScriptManager.h>
#include <am-function.h>

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

	struct ExtVar {
		char *name;
		char *file;
		bool autoload;
		bool required;
	};

	typedef ke::Lambda<bool(const sp_pubvar_t *, const ExtVar& ext)> ExtVarCallback;
	bool ForEachExtVar(const ExtVarCallback& callback);

	void ForEachLibrary(ke::Lambda<void(const char *)> callback);
public:
	/**
	 * Creates a plugin object with default values.
	 *   If an error buffer is specified, and an error occurs, the error will be copied to the buffer
	 * and NULL will be returned.
	 *   If an error buffer is not specified, the error will be copied to an internal buffer and 
	 * a valid (but error-stated) CPlugin will be returned.
	 */
	static CPlugin *Create(const char *file);

	static inline bool matches(const char *file, const CPlugin *plugin)
	{
		return strcmp(plugin->m_filename, file) == 0;
	}

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
	 * After invoking AskPluginLoad, its state is either Running or Failed.
	 */
	APLRes AskPluginLoad();

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
	 * Calls the OnLibraryAdded function.
	 */
	void Call_OnLibraryAdded(const char *lib);

	/**
	 * Returns true if a plugin is usable.
	 */
	bool IsRunnable();

	/**
	 * Get languages info.
	 */
	IPhraseCollection *GetPhrases();

public:
	// Returns true if the plugin was running, but is now invalid.
	bool WasRunning();

	bool WaitingToUnload() const {
		return m_WaitingToUnload;
	}
	void SetWaitingToUnload() {
		m_WaitingToUnload = true;
	}

	Handle_t GetMyHandle();

	bool AddFakeNative(IPluginFunction *pFunc, const char *name, SPVM_FAKENATIVE_FUNC func);
	void AddConfig(bool autoCreate, const char *cfg, const char *folder);
	size_t GetConfigCount();
	AutoConfig *GetConfig(size_t i);
	inline void AddLibrary(const char *name) {
		m_Libraries.push_back(name);
	}
	inline bool HasLibrary(const char *name) {
		return m_Libraries.find(name) != m_Libraries.end();
	}
	void LibraryActions(LibraryAction action);
	void SyncMaxClients(int max_clients);

	// Returns true if the plugin's underlying file structure has a newer
	// modification time than either when it was first instantiated or
	// since the last call to HasUpdatedFile().
	bool HasUpdatedFile();

	const char *GetDateTime() const {
		return m_DateTime;
	}
	int GetFileVersion() const {
		return m_FileVersion;
	}

protected:
	bool ReadInfo();
	void DependencyDropped(CPlugin *pOwner);
	bool TryCompile();

private:
	time_t GetFileTimeStamp();

private:
	// This information is static for the lifetime of the plugin.
	char m_filename[PLATFORM_MAX_PATH];
	unsigned int m_serial;

	PluginStatus m_status;
	bool m_WaitingToUnload;

	// Statuses that are set during failure.
	bool m_SilentFailure;
	bool m_FakeNativesMissing;
	bool m_LibraryMissing;
	char m_errormsg[256];

	// Internal properties that must by reset if the runtime is evicted.
	ke::AutoPtr<IPluginRuntime> m_pRuntime;
	ke::AutoPtr<CPhraseCollection> m_pPhrases;
	IPluginContext *m_pContext;
	sp_pubvar_t *m_MaxClientsVar;
	StringHashMap<void *> m_Props;
	CVector<AutoConfig *> m_configs;
	IdentityToken_t *m_ident;
	bool m_bGotAllLoaded;
	int m_FileVersion;

	// Information that survives past eviction.
	List<String> m_RequiredLibs;
	List<String> m_Libraries;
	time_t m_LastFileModTime;
	Handle_t m_handle;
	char m_DateTime[256];

	// Cached.
	sm_plugininfo_t m_info;
	ke::AString info_name_;
	ke::AString info_author_;
	ke::AString info_description_;
	ke::AString info_version_;
	ke::AString info_url_;
};

class CPluginManager : 
	public IScriptManager,
	public SMGlobalClass,
	public IHandleTypeDispatch,
	public IRootConsoleCommand
{
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
	const CVector<SMPlugin *> *ListPlugins();
	void FreePluginList(const CVector<SMPlugin *> *plugins);
public: //SMGlobalClass
	void OnSourceModAllInitialized();
	void OnSourceModShutdown();
	ConfigResult OnSourceModConfigChanged(const char *key, const char *value, ConfigSource source, char *error, size_t maxlength);
	void OnSourceModMaxPlayersChanged(int newvalue);
public: //IHandleTypeDispatch
	void OnHandleDestroy(HandleType_t type, void *object);
	bool GetHandleApproxSize(HandleType_t type, void *object, unsigned int *pSize);
public: //IRootConsoleCommand
	void OnRootConsoleCommand(const char *cmdname, const ICommandArgs *command) override;
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

	void _SetPauseState(CPlugin *pPlugin, bool pause);

	void ForEachPlugin(ke::Lambda<void(CPlugin *)> callback);
private:
	LoadRes LoadPlugin(CPlugin **pPlugin, const char *path, bool debug, PluginType type);

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
	 * First pass for loading a plugin, and its helpers.
	 */
	CPlugin *CompileAndPrep(const char *path);
	bool MalwareCheckPass(CPlugin *pPlugin);

	/**
	 * Runs the second loading pass on a plugin.
	 */
	bool RunSecondPass(CPlugin *pPlugin, char *error, size_t maxlength);

	/**
	 * Runs an extension pass on a plugin.
	 */
	void LoadExtensions(CPlugin *pPlugin);
	bool RequireExtensions(CPlugin *pPlugin, char *error, size_t maxlength);

	/**
	* Manages required natives.
	*/
	bool FindOrRequirePluginDeps(CPlugin *pPlugin, char *error, size_t maxlength);

	bool ScheduleUnload(CPlugin *plugin);
	void UnloadPluginImpl(CPlugin *plugin);
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
	NameHashSet<CPlugin *> m_LoadLookup;
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

