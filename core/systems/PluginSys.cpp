#include <stdio.h>
#include "PluginSys.h"
#include "ShareSys.h"
#include "LibrarySys.h"
#include "HandleSys.h"
#include "sourcemm_api.h"
#include "sourcemod.h"
#include "CTextParsers.h"
#include "CLogger.h"

CPluginManager g_PluginSys;
HandleType_t g_PluginType = 0;
IdentityType_t g_PluginIdent = 0;

CPlugin::CPlugin(const char *file)
{
	static int MySerial = 0;

	m_type = PluginType_Private;
	m_status = Plugin_Uncompiled;
	m_serial = ++MySerial;
	m_plugin = NULL;
	m_funcsnum = 0;
	m_priv_funcs = NULL;
	m_pub_funcs = NULL;
	m_errormsg[256] = '\0';
	snprintf(m_filename, sizeof(m_filename), "%s", file);
	m_handle = 0;
	m_ident = NULL;
}

CPlugin::~CPlugin()
{
	if (m_ctx.base)
	{
		delete m_ctx.base;
		m_ctx.base = NULL;
	}
	if (m_ctx.ctx)
	{
		m_ctx.vm->FreeContext(m_ctx.ctx);
		m_ctx.ctx = NULL;
	}
	if (m_ctx.co)
	{
		m_ctx.vm->AbortCompilation(m_ctx.co);
		m_ctx.co = NULL;
	}

	if (m_pub_funcs)
	{
		for (uint32_t i=0; i<m_plugin->info.publics_num; i++)
		{
			g_PluginSys.ReleaseFunctionToPool(m_pub_funcs[i]);
		}
		delete [] m_pub_funcs;
		m_pub_funcs = NULL;
	}

	if (m_priv_funcs)
	{
		for (unsigned int i=0; i<m_funcsnum; i++)
		{
			g_PluginSys.ReleaseFunctionToPool(m_priv_funcs[i]);
		}
		delete [] m_priv_funcs;
		m_priv_funcs = NULL;
	}

	if (m_plugin)
	{
		g_pSourcePawn->FreeFromMemory(m_plugin);
		m_plugin = NULL;
	}

	if (m_handle)
	{
		HandleSecurity sec;
		sec.pOwner = g_PluginSys.GetIdentity();
		sec.pIdentity = sec.pOwner;

		g_HandleSys.FreeHandle(m_handle, &sec);
		g_ShareSys.DestroyIdentity(m_ident);
	}
}

void CPlugin::InitIdentity()
{
	if (!m_handle)
	{
		m_ident = g_ShareSys.CreateIdentity(g_PluginIdent);
		m_handle = g_HandleSys.CreateHandle(g_PluginType, this, g_PluginSys.GetIdentity(), g_PluginSys.GetIdentity(), NULL);
		m_ctx.base->SetIdentity(m_ident);
	}
}

CPlugin *CPlugin::CreatePlugin(const char *file, char *error, size_t maxlength)
{
	char fullpath[PLATFORM_MAX_PATH+1];
	g_LibSys.PathFormat(fullpath, sizeof(fullpath), "%s/plugins/%s", g_SourceMod.GetSMBaseDir(), file);
	FILE *fp = fopen(fullpath, "rb");

	if (!fp)
	{
		if (error)
		{
			snprintf(error, maxlength, "Unable to open file");
			return NULL;
		} else {
			CPlugin *pPlugin = new CPlugin(file);
			snprintf(pPlugin->m_errormsg, sizeof(pPlugin->m_errormsg), "Unable to open file");
			pPlugin->m_status = Plugin_BadLoad;
			return pPlugin;
		}
	}

	int err;
	sp_plugin_t *pl = g_pSourcePawn->LoadFromFilePointer(fp, &err);
	if (pl == NULL)
	{
		fclose(fp);
		if (error)
		{
			snprintf(error, maxlength, "Error %d while parsing plugin", err);
			return NULL;
		} else {
			CPlugin *pPlugin = new CPlugin(file);
			snprintf(pPlugin->m_errormsg, sizeof(pPlugin->m_errormsg), "Error %d while parsing plugin", err);
			pPlugin->m_status = Plugin_BadLoad;
			return pPlugin;
		}
	}

	fclose(fp);

	CPlugin *pPlugin = new CPlugin(file);
	pPlugin->m_plugin = pl;
	return pPlugin;
}

ICompilation *CPlugin::StartMyCompile(IVirtualMachine *vm)
{
	if (!m_plugin)
	{
		return NULL;
	}

	/* :NOTICE: We will eventually need to change these natives
	 * for swapping in new contexts
	 */
	if (m_ctx.co || m_ctx.ctx)
	{
		return NULL;
	}

	m_status = Plugin_Uncompiled;

	m_ctx.vm = vm;
	m_ctx.co = vm->StartCompilation(m_plugin);

	return m_ctx.co;
}

void CPlugin::CancelMyCompile()
{
	if (!m_ctx.co)
	{
		return;
	}

	m_ctx.vm->AbortCompilation(m_ctx.co);
	m_ctx.co = NULL;
	m_ctx.vm = NULL;
}

bool CPlugin::FinishMyCompile(char *error, size_t maxlength)
{
	if (!m_ctx.co)
	{
		return false;
	}

	int err;
	m_ctx.ctx = m_ctx.vm->CompileToContext(m_ctx.co, &err);
	if (!m_ctx.ctx)
	{
		memset(&m_ctx, 0, sizeof(m_ctx));
		if (!error)
		{
			SetErrorState(Plugin_Failed, "Failed to compile (error %d)", err);
		} else {
			snprintf(error, maxlength, "Failed to compile (error %d)", err);
		}
		return false;
	}

	m_ctx.base = new BaseContext(m_ctx.ctx);
	m_ctx.ctx->user[SM_CONTEXTVAR_MYSELF] = (void *)this;

	m_funcsnum = m_ctx.vm->FunctionCount(m_ctx.ctx);

	/**
	 * Note: Since the m_plugin member will never change,
	 * it is safe to assume the function count will never change
	 */
	if (m_funcsnum && m_priv_funcs == NULL)
	{
		m_priv_funcs = new CFunction *[m_funcsnum];
		memset(m_priv_funcs, 0, sizeof(CFunction *) * m_funcsnum);
	} else {
		m_priv_funcs = NULL;
	}

	if (m_plugin->info.publics_num && m_pub_funcs == NULL)
	{
		m_pub_funcs = new CFunction *[m_plugin->info.publics_num];
		memset(m_pub_funcs, 0, sizeof(CFunction *) * m_plugin->info.publics_num);
	} else {
		m_pub_funcs = NULL;
	}

	m_status = Plugin_Created;
	m_ctx.co = NULL;

	UpdateInfo();

	return true;
}

void CPlugin::SetErrorState(PluginStatus status, const char *error_fmt, ...)
{
	m_status = status;

	va_list ap;
	va_start(ap, error_fmt);
	vsnprintf(m_errormsg, sizeof(m_errormsg), error_fmt, ap);
	va_end(ap);
}

IPluginFunction *CPlugin::GetFunctionById(funcid_t func_id)
{
	CFunction *pFunc = NULL;
	funcid_t save = func_id;

	if (func_id & 1)
	{
		func_id >>= 1;
		if (func_id >= m_plugin->info.publics_num)
		{
			return NULL;
		}
		pFunc = m_pub_funcs[func_id];
		if (!pFunc)
		{
			pFunc = g_PluginSys.GetFunctionFromPool(save, this);
			m_pub_funcs[func_id] = pFunc;
		}
	} else {
		func_id >>= 1;
		unsigned int index;
		if (!g_pVM->FunctionLookup(m_ctx.ctx, func_id, &index))
		{
			return NULL;
		}
		pFunc = m_priv_funcs[func_id];
		if (!pFunc)
		{
			pFunc = g_PluginSys.GetFunctionFromPool(save, this);
			m_priv_funcs[func_id] = pFunc;
		}
	}

	return pFunc;
}

IPluginFunction *CPlugin::GetFunctionByName(const char *public_name)
{
	uint32_t index;
	IPluginContext *base = m_ctx.base;

	if (base->FindPublicByName(public_name, &index) != SP_ERROR_NONE)
	{
		return NULL;
	}

	CFunction *pFunc = m_pub_funcs[index];
	if (!pFunc)
	{
		sp_public_t *pub = NULL;
		base->GetPublicByIndex(index, &pub);
		if (pub)
		{
			pFunc = g_PluginSys.GetFunctionFromPool(pub->funcid, this);
			m_pub_funcs[index] = pFunc;
		}
	}

	return pFunc;
}

void CPlugin::UpdateInfo()
{
	/* Now grab the info */
	uint32_t idx;
	IPluginContext *base = GetBaseContext();
	int err = base->FindPubvarByName("myinfo", &idx);

	if (err == SP_ERROR_NONE)
	{
		struct sm_plugininfo_c_t
		{
			cell_t name;
			cell_t description;
			cell_t author;
			cell_t version;
			cell_t url;
		};
		sm_plugininfo_c_t *cinfo;
		cell_t local_addr;

		base->GetPubvarAddrs(idx, &local_addr, (cell_t **)&cinfo);
		base->LocalToString(cinfo->name, (char **)&m_info.name);
		base->LocalToString(cinfo->description, (char **)&m_info.description);
		base->LocalToString(cinfo->author, (char **)&m_info.author);
		base->LocalToString(cinfo->url, (char **)&m_info.url);
		base->LocalToString(cinfo->version, (char **)&m_info.version);
	}

	m_info.author = m_info.author ? m_info.author : "";
	m_info.description = m_info.description ? m_info.description : "";
	m_info.name = m_info.name ? m_info.name : "";
	m_info.url = m_info.url ? m_info.url : "";
	m_info.version = m_info.version ? m_info.version : "";
}

void CPlugin::Call_OnPluginInit()
{
	if (m_status != Plugin_Loaded)
	{
		return;
	}

	m_status = Plugin_Running;

	cell_t result;
	IPluginFunction *pFunction = GetFunctionByName("OnPluginInit");
	if (!pFunction)
	{
		return;
	}

	pFunction->PushCell(m_handle);
	pFunction->Execute(&result);
}

void CPlugin::Call_OnPluginUnload()
{
	if (m_status < Plugin_Paused)
	{
		return;
	}

	cell_t result;
	IPluginFunction *pFunction = GetFunctionByName("OnPluginUnload");
	if (!pFunction)
	{
		return;
	}

	pFunction->Execute(&result);
}

bool CPlugin::Call_AskPluginLoad(char *error, size_t maxlength)
{
	if (m_status != Plugin_Created)
	{
		return false;
	}

	if (!error)
	{
		error = m_errormsg;
		maxlength = sizeof(m_errormsg);
	}

	m_status = Plugin_Loaded;

	int err;
	cell_t result;
	IPluginFunction *pFunction = GetFunctionByName("AskPluginLoad");

	if (!pFunction)
	{
		return true;
	}

	pFunction->PushCell(m_handle);
	pFunction->PushCell(g_PluginSys.IsLateLoadTime() ? 1 : 0);
	pFunction->PushStringEx(error, maxlength, 0, SM_PARAM_COPYBACK);
	pFunction->PushCell(maxlength);
	if ((err=pFunction->Execute(&result)) != SP_ERROR_NONE)
	{
		m_status = Plugin_Failed;
		return false;
	}

	if (!result || m_status != Plugin_Loaded)
	{
		m_status = Plugin_Failed;
		return false;
	}

	return true;
}

const sp_plugin_t *CPlugin::GetPluginStructure() const
{
	return m_plugin;
}

IPluginContext *CPlugin::GetBaseContext() const
{
	return m_ctx.base;
}

sp_context_t *CPlugin::GetContext() const
{
	return m_ctx.ctx;
}

const char *CPlugin::GetFilename() const
{
	return m_filename;
}

PluginType CPlugin::GetType() const
{
	return m_type;
}

const sm_plugininfo_t *CPlugin::GetPublicInfo() const
{
	return &m_info;
}

unsigned int CPlugin::GetSerial() const
{
	return m_serial;
}

PluginStatus CPlugin::GetStatus() const
{
	return m_status;
}

bool CPlugin::IsDebugging() const
{
	if (!m_ctx.ctx)
	{
		return false;
	}

	return ((m_ctx.ctx->flags & SP_FLAG_DEBUG) == SP_FLAG_DEBUG);
}

bool CPlugin::SetPauseState(bool paused)
{
	if (paused && GetStatus() != Plugin_Paused)
	{
		return false;
	} else if (!paused && GetStatus() != Plugin_Running) {
		return false;
	}

	m_status = (paused) ? Plugin_Paused : Plugin_Running;

	IPluginFunction *pFunction = GetFunctionByName("OnPluginPauseChange");
	if (pFunction)
	{
		cell_t result;
		pFunction->PushCell(paused ? 1 : 0);
		pFunction->Execute(&result);
	}

	return true;
}

IdentityToken_t *CPlugin::GetIdentity()
{
	return m_ident;
}

/*******************
 * PLUGIN ITERATOR *
 *******************/

CPluginManager::CPluginIterator::CPluginIterator(List<CPlugin *> *_mylist)
{
	mylist = _mylist;
	Reset();
}

IPlugin *CPluginManager::CPluginIterator::GetPlugin()
{
	return (*current);
}

bool CPluginManager::CPluginIterator::MorePlugins()
{
	return (current != mylist->end());
}

void CPluginManager::CPluginIterator::NextPlugin()
{
	current++;
}

void CPluginManager::CPluginIterator::Release()
{
	g_PluginSys.ReleaseIterator(this);
}

CPluginManager::CPluginIterator::~CPluginIterator()
{
}

void CPluginManager::CPluginIterator::Reset()
{
	current = mylist->begin();
}

/******************
 * PLUGIN MANAGER *
 ******************/

CPluginManager::CPluginManager()
{
	m_LoadLookup = sm_trie_create();
	m_AllPluginsLoaded = false;
	m_MyIdent = NULL;
}

CPluginManager::~CPluginManager()
{
	/* :NOTICE: 
	 * Ignore the fact that there might be plugins in the cache.
	 * This usually means that Core is not being unloaded properly, and everything
	 * will crash anyway.  YAY
	 */
	sm_trie_destroy(m_LoadLookup);
}


void CPluginManager::LoadAll_FirstPass(const char *config, const char *basedir)
{
	/* First read in the database of plugin settings */
	SMCParseError err;
	unsigned int line, col;
	m_AllPluginsLoaded = false;
	if ((err=g_TextParser.ParseFile_SMC(config, &m_PluginInfo, &line, &col)) != SMCParse_Okay)
	{
		g_Logger.LogError("[SOURCEMOD] Encountered fatal error parsing file \"%s\"", config);
		const char *err_msg = g_TextParser.GetSMCErrorString(err);
		if (err_msg)
		{
			g_Logger.LogError("[SOURCEMOD] Parse error encountered: \"%s\"", err_msg);
		}
	}

	LoadPluginsFromDir(basedir, NULL);
}

void CPluginManager::LoadPluginsFromDir(const char *basedir, const char *localpath)
{
	char base_path[PLATFORM_MAX_PATH+1];

	/* Form the current path to start reading from */
	if (localpath == NULL)
	{
		g_LibSys.PathFormat(base_path, sizeof(base_path), "%s", basedir);
	} else {
		g_LibSys.PathFormat(base_path, sizeof(base_path), "%s/%s", basedir, localpath);
	}

	IDirectory *dir = g_LibSys.OpenDirectory(base_path);

	if (!dir)
	{
		char error[256];
		g_LibSys.GetPlatformError(error, sizeof(error));
		g_Logger.LogError("[SOURCEMOD] Failure reading from plugins path: %s", localpath);
		g_Logger.LogError("[SOURCEMOD] Platform returned error: %s", error);
		return;
	}

	while (dir->MoreFiles())
	{
		if (dir->IsEntryDirectory() 
			&& (strcmp(dir->GetEntryName(), ".") != 0)
			&& (strcmp(dir->GetEntryName(), "..") != 0)
			&& (strcmp(dir->GetEntryName(), "disabled") != 0)
			&& (strcmp(dir->GetEntryName(), "optional") != 0))
		{
			char new_local[PLATFORM_MAX_PATH+1];
			if (localpath == NULL)
			{
				/* If no path yet, don't add a former slash */
				snprintf(new_local, sizeof(new_local), "%s", dir->GetEntryName());
			} else {
				g_LibSys.PathFormat(new_local, sizeof(new_local), "%s/%s", localpath, dir->GetEntryName());
			}
			LoadPluginsFromDir(basedir, new_local);
		} else if (dir->IsEntryFile()) {
			const char *name = dir->GetEntryName();
			size_t len = strlen(name);
			if (len >= 4
				&& strcmp(&name[len-4], ".smx") == 0)
			{
				/* If the filename matches, load the plugin */
				char plugin[PLATFORM_MAX_PATH+1];
				if (localpath == NULL)
				{
					snprintf(plugin, sizeof(plugin), "%s", name);
				} else {
					g_LibSys.PathFormat(plugin, sizeof(plugin), "%s/%s", localpath, name);
				}
				LoadAutoPlugin(plugin);
			}
		}
		dir->NextEntry();
	}
	g_LibSys.CloseDirectory(dir);
}

//well i have discovered that gabe newell is very fat, so i wrote this comment now
//:TODO: remove this function, create a better wrapper for LoadPlugin()/LoadAutoPlugin()
void CPluginManager::LoadAutoPlugin(const char *file)
{
	/**
	 * Does this plugin already exist?
	 */
	CPlugin *pPlugin;
	if (sm_trie_retrieve(m_LoadLookup, file, (void **)&pPlugin))
	{
		/* First check the type */
		PluginType type = pPlugin->GetType();
		if (type == PluginType_Private
			|| type == PluginType_Global)
		{
			return;
		}
		/* Check to see if we should try reloading it */
		if (pPlugin->GetStatus() == Plugin_BadLoad
			|| pPlugin->GetStatus() == Plugin_Error
			|| pPlugin->GetStatus() == Plugin_Failed)
		{
			UnloadPlugin(pPlugin);
		}
	}

	pPlugin = CPlugin::CreatePlugin(file, NULL, 0);

	assert(pPlugin != NULL);

	pPlugin->m_type = PluginType_MapUpdated;

	ICompilation *co = NULL;

	if (pPlugin->m_status == Plugin_Uncompiled)
	{
		co = pPlugin->StartMyCompile(g_pVM);
	}

	PluginSettings *pset;
	unsigned int setcount = m_PluginInfo.GetSettingsNum();
	for (unsigned int i=0; i<setcount; i++)
	{
		pset = m_PluginInfo.GetSettingsIfMatch(i, file);
		if (!pset)
		{
			continue;
		}
		pPlugin->m_type = pset->type_val;
		if (co)
		{
			for (unsigned int j=0; j<pset->opts_num; j++)
			{
				const char *key, *val;
				m_PluginInfo.GetOptionsForPlugin(pset, j, &key, &val);
				if (!key || !val)
				{
					continue;
				}
				if (!g_pVM->SetCompilationOption(co, key, val))
				{
					pPlugin->SetErrorState(Plugin_Failed, "Unable to set option (key \"%s\") (value \"%s\")", key, val);
					pPlugin->CancelMyCompile();
					co = NULL;
					break;
				}
			}
		}
	}

	if (co)
	{
		pPlugin->FinishMyCompile(NULL, 0);
		co = NULL;
	}

	/* We don't care about the return value */
	if (pPlugin->GetStatus() == Plugin_Created)
	{
		AddCoreNativesToPlugin(pPlugin);
		pPlugin->InitIdentity();
		pPlugin->Call_AskPluginLoad(NULL, 0);
	}

	AddPlugin(pPlugin);
}

IPlugin *CPluginManager::LoadPlugin(const char *path, bool debug, PluginType type, char error[], size_t err_max)
{
	/* See if this plugin is already loaded... reformat to get sep chars right */
	char checkpath[PLATFORM_MAX_PATH+1];
	g_LibSys.PathFormat(checkpath, sizeof(checkpath), "%s", path);

	/**
	 * In manually loading a plugin, any sort of load error causes a deletion.
	 * This is because it's assumed manually loaded plugins will not be managed.
	 * For managed plugins, we need the UI to report them properly.
	 */

	CPlugin *pPlugin;
	if (sm_trie_retrieve(m_LoadLookup, checkpath, (void **)&pPlugin))
	{
		snprintf(error, err_max, "Plugin file is already loaded");
		return NULL;
	}

	pPlugin = CPlugin::CreatePlugin(path, error, err_max);

	if (!pPlugin)
	{
		return NULL;
	}

	ICompilation *co = pPlugin->StartMyCompile(g_pVM);
	if (!co || (debug && !g_pVM->SetCompilationOption(co, "debug", "1")))
	{
		snprintf(error, err_max, "Unable to start%s compilation", debug ? " debug" : "");
		pPlugin->CancelMyCompile();
		delete pPlugin;
		return NULL;
	}

	if (!pPlugin->FinishMyCompile(error, err_max))
	{
		delete pPlugin;
		return NULL;
	}

	pPlugin->m_type = type;

	AddCoreNativesToPlugin(pPlugin);

	pPlugin->InitIdentity();

	/* Finally, ask the plugin if it wants to be loaded */
	if (!pPlugin->Call_AskPluginLoad(error, err_max))
	{
		delete pPlugin;
		return NULL;
	}

	AddPlugin(pPlugin);

	return pPlugin;
}

void CPluginManager::AddPlugin(CPlugin *pPlugin)
{
	m_plugins.push_back(pPlugin);
	sm_trie_insert(m_LoadLookup, pPlugin->m_filename, pPlugin);

	List<IPluginsListener *>::iterator iter;
	IPluginsListener *pListener;
	for (iter=m_listeners.begin(); iter!=m_listeners.end(); iter++)
	{
		pListener = (*iter);
		pListener->OnPluginCreated(pPlugin);
	}

	/* If the second pass was already completed, we have to run the pass on this plugin */
	if (m_AllPluginsLoaded && pPlugin->GetStatus() == Plugin_Loaded)
	{
		RunSecondPass(pPlugin);
	}
}

void CPluginManager::LoadAll_SecondPass()
{
	List<CPlugin *>::iterator iter;
	CPlugin *pPlugin;

	for (iter=m_plugins.begin(); iter!=m_plugins.end(); iter++)
	{
		pPlugin = (*iter);
		if (pPlugin->GetStatus() == Plugin_Loaded)
		{
			RunSecondPass(pPlugin);
		}
	}
	m_AllPluginsLoaded = true;
}

void CPluginManager::RunSecondPass(CPlugin *pPlugin)
{
	/* Tell this plugin to finish initializing itself */
	pPlugin->Call_OnPluginInit();

	/* Finish by telling all listeners */
	List<IPluginsListener *>::iterator iter;
	IPluginsListener *pListener;
	for (iter=m_listeners.begin(); iter!=m_listeners.end(); iter++)
	{
		pListener = (*iter);
		pListener->OnPluginLoaded(pPlugin);
	}
}

void CPluginManager::AddCoreNativesToPlugin(CPlugin *pPlugin)
{
	List<sp_nativeinfo_t *>::iterator iter;

	for (iter=m_natives.begin(); iter!=m_natives.end(); iter++)
	{
		sp_nativeinfo_t *natives = (*iter);
		IPluginContext *ctx = pPlugin->GetBaseContext();
		unsigned int i=0;
		/* Attempt to bind every native! */
		while (natives[i].func != NULL)
		{
			ctx->BindNative(&natives[i++]);
		}
	}
}

bool CPluginManager::UnloadPlugin(IPlugin *plugin)
{
	CPlugin *pPlugin = (CPlugin *)plugin;
	List<IPluginsListener *>::iterator iter;
	IPluginsListener *pListener;

	if (pPlugin->GetStatus() >= Plugin_Error)
	{
		/* Notify listeners of unloading */
		for (iter=m_listeners.begin(); iter!=m_listeners.end(); iter++)
		{
			pListener = (*iter);
			pListener->OnPluginUnloaded(pPlugin);
		}
		/* Notify plugin */
		pPlugin->Call_OnPluginUnload();
	}

	for (iter=m_listeners.begin(); iter!=m_listeners.end(); iter++)
	{
		/* Notify listeners of destruction */
		pListener = (*iter);
		pListener->OnPluginDestroyed(pPlugin);
	}

	/* Remove us from the lookup table and linked list */
	m_plugins.remove(pPlugin);
	sm_trie_delete(m_LoadLookup, pPlugin->m_filename);
	
	/* Tell the plugin to delete itself */
	delete pPlugin;

	return true;
}

IPlugin *CPluginManager::FindPluginByContext(const sp_context_t *ctx)
{
	IPlugin *pl = (IPlugin *)ctx->user[SM_CONTEXTVAR_MYSELF];
	return pl;
}

unsigned int CPluginManager::GetPluginCount()
{
	return m_plugins.size();
}

void CPluginManager::AddPluginsListener(IPluginsListener *listener)
{
	m_listeners.push_back(listener);
}

void CPluginManager::RemovePluginsListener(IPluginsListener *listener)
{
	m_listeners.remove(listener);
}

IPluginIterator *CPluginManager::GetPluginIterator()
{
	if (m_iters.empty())
	{
		return new CPluginIterator(&m_plugins);
	} else {
		CPluginIterator *iter = m_iters.front();
		m_iters.pop();
		iter->Reset();
		return iter;
	}
}

void CPluginManager::ReleaseIterator(CPluginIterator *iter)
{
	m_iters.push(iter);
}

void CPluginManager::ReleaseFunctionToPool(CFunction *func)
{
	if (!func)
	{
		return;
	}
	func->Cancel();
	m_funcpool.push(func);
}

CFunction *CPluginManager::GetFunctionFromPool(funcid_t f, CPlugin *plugin)
{
	if (m_funcpool.empty())
	{
		return new CFunction(f, plugin);
	} else {
		CFunction *func = m_funcpool.front();
		m_funcpool.pop();
		func->Set(f, plugin);
		return func;
	}
}

bool CPluginManager::TestAliasMatch(const char *alias, const char *localpath)
{
	/* As an optimization, we do not call strlen, but compute the length in the first pass */
	size_t alias_len = 0;
	size_t local_len = 0;

	const char *ptr = alias;
	unsigned int alias_explicit_paths = 0;
	unsigned int alias_path_end = 0;
	while (*ptr != '\0')
	{
		if (*ptr == '\\' || *ptr == '/')
		{
			alias_explicit_paths++;
			alias_path_end = alias_len;
		}
		alias_len++;
		ptr++;
	}

	if (alias_explicit_paths && alias_path_end == alias_len - 1)
	{
		/* Trailing slash is totally invalid here */
		return false;
	}

	ptr = localpath;
	unsigned int local_explicit_paths = 0;
	unsigned int local_path_end = 0;
	while (*ptr != '\0')
	{
		if (*ptr == '\\' || *ptr == '/')
		{
			local_explicit_paths++;
			local_path_end = local_len;
		}
		local_len++;
		ptr++;
	}

	/* If the alias has more explicit paths than the real path,
	 * no match will be possible.
	 */
	if (alias_explicit_paths > local_explicit_paths)
	{
		return false;
	}

	if (alias_explicit_paths)
	{
		/* We need to find if the paths match now.  For example, these should all match:
		 * csdm     csdm
		 * csdm     optional/csdm
		 * csdm/ban optional/crab/csdm/ban
		*/
		const char *aliasptr = alias;
		const char *localptr = localpath;
		bool match = true;
		do
		{
			if (*aliasptr != *localptr)
			{
				/* We have to knock one path off */
				local_explicit_paths--;
				if (alias_explicit_paths > local_explicit_paths)
				{
					/* Skip out if we're gonna have an impossible match */
					return false;
				}
				/* Eat up localptr tokens until we get a result */
				while (((localptr - localpath) < (int)local_path_end)
					&& *localptr != '/'
					&& *localptr != '\\')
				{
					localptr++;
				}
				/* Check if we hit the end of our searchable area.
				 * This probably isn't possible because of the path 
				 * count check, but it's a good idea anyway.
				 */
				if ((localptr - localpath) >= (int)local_path_end)
				{
					return false;
				} else {
					/* Consume the slash token */
					localptr++;
				}
				/* Reset the alias pointer so we can continue consuming */
				aliasptr = alias;
				match = false;
				continue;
			}
			/* Note:
			 * This is safe because if localptr terminates early, aliasptr will too
			 */
			do
			{
				/* We should never reach the end of the string because of this check. */
				bool aliasend = (aliasptr - alias) > (int)alias_path_end;
				bool localend = (localptr - localpath) > (int)local_path_end;
				if (aliasend || localend)
				{
					if (aliasend && localend)
					{
						/* we matched, and we can break out now */
						match = true;
						break;
					}
					/* Otherwise, we've hit the end somehow and rest won't match up.  Break out. */
					match = false;
					break;
				}

				/* If we got here, it's safe to compare the next two tokens */
				if (*localptr != *aliasptr)
				{
					match = false;
					break;
				}
				localptr++;
				aliasptr++;
			} while (true);
		} while (!match);
	}

	/* If we got here, it's time to compare filenames */
	const char *aliasptr = alias;
	const char *localptr = localpath;

	if (alias_explicit_paths)
	{
		aliasptr = &alias[alias_path_end + 1];
	}

	if (local_explicit_paths)
	{
		localptr = &localpath[local_path_end + 1];
	}

	while (true)
	{
		if (*aliasptr == '*')
		{
			/* First, see if this is the last character */
			if (aliasptr - alias == alias_len - 1)
			{
				/* If so, there's no need to match anything else */
				return true;
			}
			/* Otherwise, we need to search for an appropriate matching sequence in local.
			 * Note that we only need to search up to the next asterisk.
			 */
			aliasptr++;
			bool match = true;
			const char *local_orig = localptr;
			do
			{
				match = true;
				while (*aliasptr != '\0' && *aliasptr != '*')
				{
					/* Since aliasptr is never '\0', localptr hitting the end will fail */
					if (*aliasptr != *localptr)
					{
						match = false;
						break;
					}
					aliasptr++;
					localptr++;
				}
				if (!match)
				{
					/* If we didn't get a match, we need to advance the search stream.
					 * This will let us skip tokens while still searching for another match.
					 */
					localptr = ++local_orig;
					/* Make sure we don't go out of bounds */
					if (*localptr == '\0')
					{
						break;
					}
				}
			} while (!match);

			if (!match)
			{
				return false;
			} else {
				/* If we got a match, move on to the next token */
				continue;
			}
		} else if (*aliasptr == '\0') {
			if (*localptr == '\0'
				||
				strcmp(localptr, ".smx") == 0)
			{
				return true;
			} else {
				return false;
			}
		} else if (*aliasptr != *localptr) {
			return false;
		}
		aliasptr++;
		localptr++;
	}

	return true;
}

bool CPluginManager::IsLateLoadTime()
{
	return (m_AllPluginsLoaded || g_SourceMod.IsLateLoadInMap());
}

void CPluginManager::OnSourceModAllInitialized()
{
	m_MyIdent = g_ShareSys.CreateCoreIdentity();

	HandleAccess sec;
	g_HandleSys.InitAccessDefaults(NULL, &sec);

	sec.access[HandleAccess_Delete] = HANDLE_RESTRICT_IDENTITY;
	sec.access[HandleAccess_Clone] = HANDLE_RESTRICT_IDENTITY;

	g_PluginType = g_HandleSys.CreateType("Plugin", this, 0, NULL, &sec, m_MyIdent, NULL);
	g_PluginIdent = g_ShareSys.CreateIdentType("PLUGIN");
}

void CPluginManager::OnSourceModShutdown()
{
	g_HandleSys.RemoveType(g_PluginType, m_MyIdent);
	g_ShareSys.DestroyIdentType(g_PluginIdent);
	g_ShareSys.DestroyIdentity(m_MyIdent);
}

void CPluginManager::OnHandleDestroy(HandleType_t type, void *object)
{
	/* We don't care about the internal object, actually */
}

void CPluginManager::RegisterNativesFromCore(sp_nativeinfo_t *natives)
{
	m_natives.push_back(natives);
}

IPlugin *CPluginManager::PluginFromHandle(Handle_t handle, HandleError *err)
{
	IPlugin *pPlugin;
	HandleError _err;

	HandleSecurity sec;

	sec.pOwner = NULL;
	sec.pIdentity = m_MyIdent;

	if ((_err=g_HandleSys.ReadHandle(handle, g_PluginType, &sec, (void **)&pPlugin)) != HandleError_None)
	{
		pPlugin = NULL;
	}

	if (err)
	{
		*err = _err;
	}

	return pPlugin;
}
