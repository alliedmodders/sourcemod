#ifndef _INCLUDE_SOURCEMOD_PLUGINSYSTEM_H_
#define _INCLUDE_SOURCEMOD_PLUGINSYSTEM_H_

#include <IPluginSys.h>
#include <sh_list.h>
#include <sh_stack.h>
#include "sm_globals.h"
#include "CFunction.h"
#include "PluginInfoDatabase.h"

using namespace SourceHook;

#define SM_CONTEXTVAR_MYSELF	0

struct ContextPair
{
	ContextPair() : base(NULL), ctx(NULL)
	{
	};
	IPluginContext *base;
	sp_context_t *ctx;
};

class CPlugin : public IPlugin
{
	friend class CPluginManager;
	friend class CFunction;
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
public:
	static CPlugin *CreatePlugin(const char *file, 
								bool debug_default, 
								PluginType life, 
								char *error, 
								size_t maxlen);
protected:
	void UpdateInfo();
private:
	ContextPair m_ctx_current;
	ContextPair m_ctx_backup;
	PluginType m_type;
	bool m_debugging;
	char m_filename[PLATFORM_MAX_PATH+1];
	PluginStatus m_status;
	unsigned int m_serial;
	sm_plugininfo_t m_info;
	sp_plugin_t *m_plugin;
	unsigned int m_funcsnum;
	CFunction **m_priv_funcs;
	CFunction **m_pub_funcs;
};

struct PluginDBInfo
{
	bool pause;
	PluginType lifetime;

};

class CPluginManager : public IPluginManager
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
		CPluginIterator(List<IPlugin *> *mylist);
		virtual ~CPluginIterator();
		virtual bool MorePlugins();
		virtual IPlugin *GetPlugin();
		virtual void NextPlugin();
		virtual void Release();
	public:
		void Reset();
	private:
		List<IPlugin *> *mylist;
		List<IPlugin *>::iterator current;
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
public:
	/**
	 * Refreshes and loads plugins, usually used on mapchange
	 */
	virtual void RefreshOrLoadPlugins(const char *config, const char *basedir);

	/**
	 * Tests a plugin file mask against a local folder.
	 * The alias is searched backwards from localdir - i.e., given this input:
	 *   csdm/ban        csdm/ban
	 *   ban             csdm/ban
	 *   csdm/ban        optional/csdm/ban
	 * All of these will return true for an alias match.  
	 * Wildcards are allowed in the filename.
	 */
	virtual bool TestAliasMatch(const char *alias, const char *localdir);
protected:
	void ReleaseIterator(CPluginIterator *iter);
	CFunction *GetFunctionFromPool(funcid_t f, CPlugin *plugin);
	void ReleaseFunctionToPool(CFunction *func);
private:
	List<IPluginsListener *> m_listeners;
	List<IPlugin *> m_plugins;
	CStack<CPluginManager::CPluginIterator *> m_iters;
	CStack<CFunction *> m_funcpool;
	CPluginInfoDatabase m_PluginInfo;
};

extern CPluginManager g_PluginMngr;

#endif //_INCLUDE_SOURCEMOD_PLUGINSYSTEM_H_
