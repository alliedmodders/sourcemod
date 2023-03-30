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

#include <errno.h>
#include <sys/stat.h>
#include <time.h>

#include <memory>

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
#include <ReentrantList.h>

class CPlayer;

using namespace SourceHook;

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

// Plugin membership state.
enum class PluginState
{
	// The plugin has not yet been added to the global plugin list.
	Unregistered,

	// The plugin is a member of the global plugin list.
	Registered,

	// The plugin has been evicted.
	Evicted,

	// The plugin is waiting to be unloaded.
	WaitingToUnload,
	WaitingToUnloadAndReload,
};

class CPlugin : 
	public SMPlugin,
	public CNativeOwner
{
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
	PluginStatus Status() const;
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

	typedef ke::Function<bool(const sp_pubvar_t *, const ExtVar& ext)> ExtVarCallback;
	bool ForEachExtVar(const ExtVarCallback& callback);

	void ForEachLibrary(ke::Function<void(const char *)> callback);
public:
	/**
	 * Creates a plugin object with default values.
	 *   If an error buffer is specified, and an error occurs, the error will be copied to the buffer
	 * and NULL will be returned.
	 *   If an error buffer is not specified, the error will be copied to an internal buffer and 
	 * a valid (but error-stated) CPlugin will be returned.
	 */
	static CPlugin *Create(const char *file);

public:
	// Evicts the plugin from memory and sets an error state.
	void EvictWithError(PluginStatus status, const char *error_fmt, ...);
	void FinishEviction();

	// Initializes the plugin's identity information
	void InitIdentity();

	// Calls the OnPluginLoad function, and sets any failed states if necessary.
	// After invoking AskPluginLoad, its state is either Running or Failed.
	APLRes AskPluginLoad();

	// Transition to the fully running state, if possible.
	bool OnPluginStart();

	void Call_OnPluginEnd();
	void Call_OnAllPluginsLoaded();
	void Call_OnLibraryAdded(const char *lib);

	// Returns true if a plugin is usable.
	bool IsRunnable();

	// Get languages info.
	IPhraseCollection *GetPhrases();

	PluginState State() const {
		return m_state;
	}
	void SetRegistered();
	void SetWaitingToUnload(bool andReload=false);

	PluginStatus GetDisplayStatus() const {
		return m_status;
	}
	bool IsEvictionCandidate() const;

public:
	// Returns true if the plugin was running, but is now invalid.
	bool WasRunning();

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
	const char *GetErrorMsg() const {
		assert(m_status != Plugin_Running && m_status != Plugin_Loaded);
		return m_errormsg;
	}

	void AddRequiredLib(const char *name);
	bool ForEachRequiredLib(ke::Function<bool(const char *)> callback);

	bool HasMissingFakeNatives() const {
		return m_FakeNativesMissing;
	}
	bool HasMissingLibrary() const {
		return m_LibraryMissing;
	}
	bool HasFakeNatives() const {
		return m_fakes.size() > 0;
	}

	// True if we got far enough into the second pass to call OnPluginLoaded
	// in the listener list.
	bool EnteredSecondPass() const {
		return m_EnteredSecondPass;
	}

	bool IsInErrorState() const {
		if (m_status == Plugin_Running || m_status == Plugin_Loaded)
			return false;
		return true;
	}

	bool TryCompile();
	void BindFakeNativesTo(CPlugin *other);

protected:
	void DependencyDropped(CPlugin *pOwner);

private:
	time_t GetFileTimeStamp();
	bool ReadInfo();
	void DestroyIdentity();

private:
	// This information is static for the lifetime of the plugin.
	char m_filename[PLATFORM_MAX_PATH];
	unsigned int m_serial;

	PluginStatus m_status;
	PluginState m_state;
	bool m_AddedLibraries;
	bool m_EnteredSecondPass;

	// Statuses that are set during failure.
	bool m_SilentFailure;
	bool m_FakeNativesMissing;
	bool m_LibraryMissing;
	char m_errormsg[256];

	// Internal properties that must by reset if the runtime is evicted.
	std::unique_ptr<IPluginRuntime> m_pRuntime;
	std::unique_ptr<CPhraseCollection> m_pPhrases;
	IPluginContext *m_pContext;
	sp_pubvar_t *m_MaxClientsVar;
	StringHashMap<void *> m_Props;
	CVector<AutoConfig *> m_configs;
	List<String> m_Libraries;
	bool m_bGotAllLoaded;
	int m_FileVersion;

	// Information that survives past eviction.
	List<String> m_RequiredLibs;
	IdentityToken_t *m_ident;
	time_t m_LastFileModTime;
	Handle_t m_handle;
	char m_DateTime[256];

	// Cached.
	sm_plugininfo_t m_info;
	std::string info_name_;
	std::string info_author_;
	std::string info_description_;
	std::string info_version_;
	std::string info_url_;
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
	class CPluginIterator
		: public IPluginIterator,
		  public IPluginsListener
	{
	public:
		CPluginIterator(ReentrantList<CPlugin *>& in);
		virtual ~CPluginIterator();
		virtual bool MorePlugins();
		virtual IPlugin *GetPlugin();
		virtual void NextPlugin();
		void Release();
		void OnPluginDestroyed(IPlugin *plugin) override;
	private:
		std::list<CPlugin *> mylist;
		std::list<CPlugin *>::iterator current;
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

	bool ReloadPlugin(CPlugin *pl, bool print=false);

	void UnloadAll();

	void SyncMaxClients(int max_clients);

	void ListPluginsToClient(CPlayer *player, const CCommand &args);

	void _SetPauseState(CPlugin *pPlugin, bool pause);

	void ForEachPlugin(ke::Function<void(CPlugin *)> callback);
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

	// First pass for loading a plugin, and its helpers.
	CPlugin *CompileAndPrep(const char *path);
	bool MalwareCheckPass(CPlugin *pPlugin);

	// Runs the second loading pass on a plugin.
	bool RunSecondPass(CPlugin *pPlugin);
	void LoadExtensions(CPlugin *pPlugin);
	bool RequireExtensions(CPlugin *pPlugin);
	bool FindOrRequirePluginDeps(CPlugin *pPlugin);

	void UnloadPluginImpl(CPlugin *plugin);
	void ReloadPluginImpl(int id, const char filename[], PluginType ptype, bool print);

	void Purge(CPlugin *plugin);
public:
	inline IdentityToken_t *GetIdentity()
	{
		return m_MyIdent;
	}
	IPluginManager *GetOldAPI();
private:
	void TryRefreshDependencies(CPlugin *pOther);
private:
	ReentrantList<IPluginsListener *> m_listeners;
	ReentrantList<CPlugin *> m_plugins;
	std::list<CPluginIterator *> m_iterators;

	typedef decltype(m_listeners)::iterator ListenerIter;
	typedef decltype(m_plugins)::iterator PluginIter;

	struct CPluginPolicy
	{
		static inline uint32_t hash(const detail::CharsAndLength &key)
		{
/* For windows & mac, we convert the path to lower-case in order to avoid duplicate plugin loading */
#if defined PLATFORM_WINDOWS || defined PLATFORM_APPLE
			std::string lower = ke::Lowercase(key.c_str());
			return detail::CharsAndLength(lower.c_str()).hash();
#else
			return key.hash();
#endif
		}
		
		static inline bool matches(const char *file, const CPlugin *plugin)
		{
			const char *pluginFileChars = const_cast<CPlugin*>(plugin)->GetFilename();
#if defined PLATFORM_WINDOWS || defined PLATFORM_APPLE
			std::string pluginFile = ke::Lowercase(pluginFileChars);
			std::string input = ke::Lowercase(file);
			
			return pluginFile == input;
#else
			return strcmp(pluginFileChars, file) == 0;
#endif
		}
	};
	NameHashSet<CPlugin *, CPluginPolicy> m_LoadLookup;

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
	IForward *m_pOnNotifyPluginUnloaded;
};

extern CPluginManager g_PluginSys;

#endif //_INCLUDE_SOURCEMOD_PLUGINSYSTEM_H_

