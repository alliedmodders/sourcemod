#ifndef _INCLUDE_SOURCEMOD_PLUGINSYSTEM_H_
#define _INCLUDE_SOURCEMOD_PLUGINSYSTEM_H_

#include <time.h>
#include <IPluginSys.h>
#include <IHandleSys.h>
#include <sh_list.h>
#include <sh_stack.h>
#include "sm_globals.h"
#include "CFunction.h"
#include "PluginInfoDatabase.h"
#include "sm_trie.h"
#include "sourcemod.h"

using namespace SourceHook;

/**
 * NOTES:
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
 *			 7. Once all plugins are deemed to be loaded, OnPluginInit() is called
 */

#define SM_CONTEXTVAR_MYSELF	0

struct ContextPair
{
	ContextPair() : base(NULL), ctx(NULL), co(NULL)
	{
	};
	IPluginContext *base;
	sp_context_t *ctx;
	ICompilation *co;
	IVirtualMachine *vm;
};

class CPlugin : public IPlugin
{
	friend class CPluginManager;
	friend class CFunction;
public:
	CPlugin(const char *file);
	~CPlugin();
public:
	virtual PluginType GetType() const;
	virtual SourcePawn::IPluginContext *GetBaseContext() const;
	virtual sp_context_t *GetContext() const;
	virtual const sm_plugininfo_t *GetPublicInfo() const;
	virtual const char *GetFilename() const;
	virtual bool IsDebugging() const;
	virtual PluginStatus GetStatus() const;
	virtual bool SetPauseState(bool paused);
	virtual unsigned int GetSerial() const;
	virtual const sp_plugin_t *GetPluginStructure() const;
	virtual IPluginFunction *GetFunctionByName(const char *public_name);
	virtual IPluginFunction *GetFunctionById(funcid_t func_id);
	virtual IdentityToken_t *GetIdentity();
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
	 * Starts the initial compilation of a plugin.
	 * Returns false if another compilation exists or there is a current context set.
	 */
	ICompilation *StartMyCompile(IVirtualMachine *vm);
	/** 
	 * Finalizes a compilation.  If error buffer is NULL, the error is saved locally.
	 */
	bool FinishMyCompile(char *error, size_t maxlength);
	void CancelMyCompile();

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
	 * If validated, plugin state is changed to Plugin_Loaded
	 *
	 * If the error buffer is NULL, the error message is cached locally.
	 */
	bool Call_AskPluginLoad(char *error, size_t maxlength);

	/**
	 * Calls the OnPluginInit function.
	 * NOTE: Valid pre-states are: Plugin_Created
	 * NOTE: Pre-state will be changed to Plugin_Running
	 */
	void Call_OnPluginInit();
public:
	time_t HasUpdatedFile();
protected:
	void UpdateInfo();
private:
	ContextPair m_ctx;
	PluginType m_type;
	char m_filename[PLATFORM_MAX_PATH+1];
	PluginStatus m_status;
	unsigned int m_serial;
	sm_plugininfo_t m_info;
	sp_plugin_t *m_plugin;
	unsigned int m_funcsnum;
	CFunction **m_priv_funcs;
	CFunction **m_pub_funcs;
	char m_errormsg[256];
	time_t m_LastAccess;
	IdentityToken_t *m_ident;
	Handle_t m_handle;
};

class CPluginManager : 
	public IPluginManager,
	public SMGlobalClass,
	public IHandleTypeDispatch
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
		virtual void Release();
	public:
		void Reset();
	private:
		List<CPlugin *> *mylist;
		List<CPlugin *>::iterator current;
	};
	friend class CPluginManager::CPluginIterator;
public: //IPluginManager
	virtual IPlugin *LoadPlugin(const char *path, 
								bool debug,
								PluginType type,
								char error[],
								size_t err_max);
	virtual bool UnloadPlugin(IPlugin *plugin);
	virtual IPlugin *FindPluginByContext(const sp_context_t *ctx);
	virtual unsigned int GetPluginCount();
	virtual IPluginIterator *GetPluginIterator();
	virtual void AddPluginsListener(IPluginsListener *listener);
	virtual void RemovePluginsListener(IPluginsListener *listener);
public: //SMGlobalClass
	virtual void OnSourceModAllInitialized();
	virtual void OnSourceModShutdown();
public: //IHandleTypeDispatch
	virtual void OnHandleDestroy(HandleType_t type, void *object);
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
	bool IsLateLoadTime();

	/**
	 * Adds natives from core into the native pool.
	 */
	void RegisterNativesFromCore(sp_nativeinfo_t *natives);
private:
	/**
	 * Recursively loads all plugins in the given directory.
	 */
	void LoadPluginsFromDir(const char *basedir, const char *localdir);

	/**
	 * Loads a plugin using automatic information.
	 * The file must be relative to the plugins folder.
	 */
	void LoadAutoPlugin(const char *file);

	/**
	 * Adds a plugin object.  This is wrapped by LoadPlugin functions.
	 */
	void AddPlugin(CPlugin *pPlugin);

	/**
	 * Runs the second loading pass on a plugin.
	 */
	void RunSecondPass(CPlugin *pPlugin);

	/**
	 * Adds any globally registered natives to a plugin
	 */
	void AddCoreNativesToPlugin(CPlugin *pPlugin);
protected:
	/**
	 * Caching internal objects
	 */
	void ReleaseIterator(CPluginIterator *iter);
	CFunction *GetFunctionFromPool(funcid_t f, CPlugin *plugin);
	void ReleaseFunctionToPool(CFunction *func);
	inline IdentityToken_t *GetIdentity()
	{
		return m_MyIdent;
	}
private:
	List<IPluginsListener *> m_listeners;
	List<CPlugin *> m_plugins;
	List<sp_nativeinfo_t *> m_natives;
	CStack<CPluginManager::CPluginIterator *> m_iters;
	CStack<CFunction *> m_funcpool;
	CPluginInfoDatabase m_PluginInfo;
	Trie *m_LoadLookup;
	bool m_AllPluginsLoaded;
	IdentityToken_t *m_MyIdent;
};

extern CPluginManager g_PluginSys;

#endif //_INCLUDE_SOURCEMOD_PLUGINSYSTEM_H_
