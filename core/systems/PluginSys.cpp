/**
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

#include <stdio.h>
#include "PluginSys.h"
#include "ShareSys.h"
#include "LibrarySys.h"
#include "HandleSys.h"
#include "sourcemm_api.h"
#include "sourcemod.h"
#include "TextParsers.h"
#include "Logger.h"
#include "ExtensionSys.h"
#include "sm_srvcmds.h"
#include "sm_stringutil.h"

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
	m_errormsg[256] = '\0';
	snprintf(m_filename, sizeof(m_filename), "%s", file);
	m_handle = 0;
	m_ident = NULL;
	m_pProps = sm_trie_create();
}

CPlugin::~CPlugin()
{
	if (m_handle)
	{
		HandleSecurity sec;
		sec.pOwner = g_PluginSys.GetIdentity();
		sec.pIdentity = sec.pOwner;

		g_HandleSys.FreeHandle(m_handle, &sec);
		g_ShareSys.DestroyIdentity(m_ident);
	}

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

	if (m_plugin)
	{
		g_pSourcePawn->FreeFromMemory(m_plugin);
		m_plugin = NULL;
	}
	if (!m_pProps)
	{
		sm_trie_destroy(m_pProps);
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

Handle_t CPlugin::GetMyHandle()
{
	return m_handle;
}

CPlugin *CPlugin::CreatePlugin(const char *file, char *error, size_t maxlength)
{
	char fullpath[PLATFORM_MAX_PATH+1];
	g_SourceMod.BuildPath(Path_SM, fullpath, sizeof(fullpath), "plugins/%s", file);
	FILE *fp = fopen(fullpath, "rb");

	CPlugin *pPlugin = new CPlugin(file);

	if (!fp)
	{
		if (error)
		{
			snprintf(error, maxlength, "Unable to open file");
		}
		pPlugin->m_status = Plugin_BadLoad;
		return pPlugin;
	}

	int err;
	sp_plugin_t *pl = g_pSourcePawn->LoadFromFilePointer(fp, &err);
	if (pl == NULL)
	{
		fclose(fp);
		if (error)
		{
			snprintf(error, maxlength, "Error %d while parsing plugin", err);
		}
		pPlugin->m_status = Plugin_BadLoad;
		return pPlugin;
	}

	fclose(fp);

	pPlugin->m_plugin = pl;

	return pPlugin;
}

bool CPlugin::GetProperty(const char *prop, void **ptr, bool remove/* =false */)
{
	bool exists = sm_trie_retrieve(m_pProps, prop, ptr);

	if (exists && remove)
	{
		sm_trie_delete(m_pProps, prop);
	}

	return exists;
}

bool CPlugin::SetProperty(const char *prop, void *ptr)
{
	return sm_trie_insert(m_pProps, prop, ptr);
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
		if (error)
		{
			snprintf(error, maxlength, "Failed to compile (error %d)", err);
		}
		return false;
	}

	m_ctx.base = new BaseContext(m_ctx.ctx);
	m_ctx.ctx->user[SM_CONTEXTVAR_MYSELF] = (void *)this;

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

	if (m_ctx.ctx)
	{
		m_ctx.ctx->flags |= SPFLAG_PLUGIN_PAUSED;
	}
}

void CPlugin::UpdateInfo()
{
	/* Now grab the info */
	uint32_t idx;
	IPluginContext *base = GetBaseContext();
	int err = base->FindPubvarByName("myinfo", &idx);

	memset(&m_info, 0, sizeof(m_info));

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

void CPlugin::Call_OnPluginStart()
{
	if (m_status != Plugin_Loaded)
	{
		return;
	}

	m_status = Plugin_Running;

	cell_t result;
	IPluginFunction *pFunction = m_ctx.base->GetFunctionByName("OnPluginStart");
	if (!pFunction)
	{
		return;
	}

	pFunction->Execute(&result);
}

void CPlugin::Call_OnPluginEnd()
{
	if (m_status < Plugin_Paused)
	{
		return;
	}

	cell_t result;
	IPluginFunction *pFunction = m_ctx.base->GetFunctionByName("OnPluginEnd");
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

	m_status = Plugin_Loaded;

	int err;
	cell_t result;
	IPluginFunction *pFunction = m_ctx.base->GetFunctionByName("AskPluginLoad");

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
		return false;
	}

	if (!result || m_status != Plugin_Loaded)
	{
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

	IPluginFunction *pFunction = m_ctx.base->GetFunctionByName("OnPluginPauseChange");
	if (pFunction)
	{
		cell_t result;
		pFunction->PushCell(paused ? 1 : 0);
		pFunction->Execute(&result);
	}

	if (paused)
	{
		m_status = Plugin_Paused;
		m_ctx.ctx->flags |= SPFLAG_PLUGIN_PAUSED;
	} else {
		m_status = Plugin_Running;
		m_ctx.ctx->flags &= ~SPFLAG_PLUGIN_PAUSED;
	}

	g_PluginSys._SetPauseState(this, paused);

	return true;
}

IdentityToken_t *CPlugin::GetIdentity() const
{
	return m_ident;
}

bool CPlugin::ToggleDebugMode(bool debug, char *error, size_t maxlength)
{
	int err;

	if (!IsRunnable())
	{
		if (error)
		{
			snprintf(error, maxlength, "Plugin is not runnable.");
		}
		return false;
	}

	if (debug && IsDebugging())
	{
		if (error)
		{
			snprintf(error, maxlength, "Plugin is already in debug mode.");
		}
		return false;
	} else if (!debug && !IsDebugging()) {
		if (error)
		{
			snprintf(error, maxlength, "Plugins is already in production mode.");
		}
		return false;
	}

	ICompilation *co = g_pVM->StartCompilation(m_ctx.ctx->plugin);
	if (!g_pVM->SetCompilationOption(co, "debug", (debug) ? "1" : "0"))
	{
		if (error)
		{
			snprintf(error, maxlength, "Failed to change plugin mode (JIT failure).");
		}
		return false;
	}

	sp_context_t *new_ctx = g_pVM->CompileToContext(co, &err);

	if (new_ctx)
	{
		memcpy(new_ctx->memory, m_ctx.ctx->memory, m_ctx.ctx->mem_size);
		new_ctx->hp = m_ctx.ctx->hp;
		new_ctx->sp = m_ctx.ctx->sp;
		new_ctx->frm = m_ctx.ctx->frm;
		new_ctx->dbreak = m_ctx.ctx->dbreak;
		new_ctx->context = m_ctx.ctx->context;
		memcpy(new_ctx->user, m_ctx.ctx->user, sizeof(m_ctx.ctx->user));

		g_pVM->FreeContext(m_ctx.ctx);
		m_ctx.ctx = new_ctx;
		m_ctx.base->SetContext(new_ctx);

		UpdateInfo();
	} else {
		if (error)
		{
			snprintf(error, maxlength, "Failed to recompile plugin (JIT error %d).", err);
		}
		return false;
	}

	return true;
}

bool CPlugin::IsRunnable() const
{
	return (m_status <= Plugin_Paused) ? true : false;
}

time_t CPlugin::GetFileTimeStamp()
{
	char path[PLATFORM_MAX_PATH+1];
	g_SourceMod.BuildPath(Path_SM, path, sizeof(path), "plugins/%s", m_filename);
#ifdef PLATFORM_WINDOWS
	struct _stat s;
	if (_stat(path, &s) != 0)
#elif defined PLATFORM_POSIX
	struct stat s;
	if (stat(path, &s) != 0)
#endif
	{
		return 0;
	} else {
		return s.st_mtime;
	}
}

time_t CPlugin::GetTimeStamp() const
{
	return m_LastAccess;
}

void CPlugin::SetTimeStamp(time_t t)
{
	m_LastAccess = t;
}

void CPlugin::AddLangFile(unsigned int index)
{
	m_PhraseFiles.push_back(index);
}

size_t CPlugin::GetLangFileCount() const
{
	return m_PhraseFiles.size();
}

unsigned int CPlugin::GetLangFileByIndex(unsigned int index) const
{
	return m_PhraseFiles.at(index);
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
	m_pNativeLookup = sm_trie_create();
}

CPluginManager::~CPluginManager()
{
	/* :NOTICE: 
	 * Ignore the fact that there might be plugins in the cache.
	 * This usually means that Core is not being unloaded properly, and everything
	 * will crash anyway.  YAY
	 */
	sm_trie_destroy(m_LoadLookup);
	sm_trie_destroy(m_pNativeLookup);
}

void CPluginManager::LoadAll_FirstPass(const char *config, const char *basedir)
{
	/* First read in the database of plugin settings */
	SMCParseError err;
	unsigned int line, col;
	m_AllPluginsLoaded = false;
	if ((err=g_TextParser.ParseFile_SMC(config, &m_PluginInfo, &line, &col)) != SMCParse_Okay)
	{
		g_Logger.LogError("[SM] Encountered fatal error parsing file \"%s\"", config);
		const char *err_msg = g_TextParser.GetSMCErrorString(err);
		if (err_msg)
		{
			g_Logger.LogError("[SM] Parse error encountered: \"%s\"", err_msg);
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
		g_Logger.LogError("[SM] Failure reading from plugins path: %s", localpath);
		g_Logger.LogError("[SM] Platform returned error: %s", error);
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
LoadRes CPluginManager::_LoadPlugin(CPlugin **_plugin, const char *path, bool debug, PluginType type, char error[], size_t err_max)
{
	/**
	 * Does this plugin already exist?
	 */
	CPlugin *pPlugin;
	if (sm_trie_retrieve(m_LoadLookup, path, (void **)&pPlugin))
	{
		/* Check to see if we should try reloading it */
		if (pPlugin->GetStatus() == Plugin_BadLoad
			|| pPlugin->GetStatus() == Plugin_Error
			|| pPlugin->GetStatus() == Plugin_Failed)
		{
			UnloadPlugin(pPlugin);
		} else {
			if (_plugin)
			{
				*_plugin = pPlugin;
			}
			return LoadRes_AlreadyLoaded;
		}
	}

	pPlugin = CPlugin::CreatePlugin(path, error, err_max);

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
		if ((pset=m_PluginInfo.GetSettingsIfMatch(i, path)) == NULL)
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
					if (error)
					{
						snprintf(error, err_max, "Unable to set JIT option (key \"%s\") (value \"%s\")", key, val);
					}
					pPlugin->CancelMyCompile();
					co = NULL;
					break;
				}
			}
		}
	}

	/* Do the actual compiling */
	if (co)
	{
		pPlugin->FinishMyCompile(NULL, 0);
		co = NULL;
	}

	/* Get the status */
	if (pPlugin->GetStatus() == Plugin_Created)
	{
		AddCoreNativesToPlugin(pPlugin);
		pPlugin->InitIdentity();
		if (pPlugin->Call_AskPluginLoad(error, err_max))
		{
			/* Autoload any modules */
			LoadOrRequireExtensions(pPlugin, 1, error, err_max);
		}
	}

	/* Save the time stamp */
	time_t t = pPlugin->GetFileTimeStamp();
	pPlugin->SetTimeStamp(t);

	if (_plugin)
	{
		*_plugin = pPlugin;
	}

	return (pPlugin->GetStatus() == Plugin_Loaded) ? LoadRes_Successful : LoadRes_Failure;
}

IPlugin *CPluginManager::LoadPlugin(const char *path, bool debug, PluginType type, char error[], size_t err_max, bool *wasloaded)
{
	CPlugin *pl;
	LoadRes res;

	*wasloaded = false;
	if ((res=_LoadPlugin(&pl, path, debug, type, error, err_max)) == LoadRes_Failure)
	{
		delete pl;
		return NULL;
	}

	if (res == LoadRes_AlreadyLoaded)
	{
		*wasloaded = true;
		return pl;
	}

	AddPlugin(pl);

	/* Run second pass if we need to */
	if (IsLateLoadTime() && pl->GetStatus() == Plugin_Loaded)
	{
		if (!RunSecondPass(pl, error, err_max))
		{
			UnloadPlugin(pl);
			return NULL;
		}
	}

	return pl;
}

void CPluginManager::LoadAutoPlugin(const char *plugin)
{
	CPlugin *pl;
	LoadRes res;
	char error[255] = "Unknown error";

	if ((res=_LoadPlugin(&pl, plugin, false, PluginType_MapUpdated, error, sizeof(error))) == LoadRes_Failure)
	{
		g_Logger.LogError("[SM] Failed to load plugin \"%s\": %s", plugin, error);
		pl->SetErrorState(Plugin_Failed, "%s", error);
	}

	if (res == LoadRes_Successful)
	{
		AddPlugin(pl);
	}
}

void CPluginManager::AddPlugin(CPlugin *pPlugin)
{
	List<IPluginsListener *>::iterator iter;
	IPluginsListener *pListener;

	for (iter=m_listeners.begin(); iter!=m_listeners.end(); iter++)
	{
		pListener = (*iter);
		pListener->OnPluginCreated(pPlugin);
	}

	m_plugins.push_back(pPlugin);
	sm_trie_insert(m_LoadLookup, pPlugin->m_filename, pPlugin);
}

void CPluginManager::LoadAll_SecondPass()
{
	List<CPlugin *>::iterator iter;
	CPlugin *pPlugin;

	char error[256];
	for (iter=m_plugins.begin(); iter!=m_plugins.end(); iter++)
	{
		pPlugin = (*iter);
		if (pPlugin->GetStatus() == Plugin_Loaded)
		{
			error[0] = '\0';
			if (!RunSecondPass(pPlugin, error, sizeof(error)))
			{
				g_Logger.LogError("[SM] Unable to load plugin \"%s\": %s", pPlugin->GetFilename(), error);
				pPlugin->SetErrorState(Plugin_Failed, "%s", error);
			}
		}
	}

	m_AllPluginsLoaded = true;
}

bool CPluginManager::LoadOrRequireExtensions(CPlugin *pPlugin, unsigned int pass, char *error, size_t maxlength)
{
	/* Find any extensions this plugin needs */
	struct _ext
	{
		cell_t name;
		cell_t file;
		cell_t autoload;
		cell_t required;
	} *ext;

	IPluginContext *pBase = pPlugin->GetBaseContext();
	uint32_t num = pBase->GetPubVarsNum();
	sp_pubvar_t *pubvar;
	IExtension *pExt;
	char path[PLATFORM_MAX_PATH+1];
	char *file, *name;
	for (uint32_t i=0; i<num; i++)
	{
		if (pBase->GetPubvarByIndex(i, &pubvar) != SP_ERROR_NONE)
		{
			continue;
		}
		if (strncmp(pubvar->name, "__ext_", 6) == 0)
		{
			ext = (_ext *)pubvar->offs;
			if (!ext->required && !ext->autoload)
			{
				continue;
			}
			if (pBase->LocalToString(ext->file, &file) != SP_ERROR_NONE)
			{
				continue;
			}
			if (pBase->LocalToString(ext->name, &name) != SP_ERROR_NONE)
			{
				continue;
			}
			if (pass == 1)
			{
				/* Attempt to auto-load if necessary */
				if (ext->autoload)
				{
					g_LibSys.PathFormat(path, PLATFORM_MAX_PATH, "%s", file);
					g_Extensions.LoadAutoExtension(path);
				}
			} else if (pass == 2) {
				/* Is this required? */
				if (ext->required)
				{
					g_LibSys.PathFormat(path, PLATFORM_MAX_PATH, "%s", file);
					if ((pExt = g_Extensions.FindExtensionByFile(path)) == NULL)
					{
						pExt = g_Extensions.FindExtensionByName(name);
					}
					/* :TODO: should we bind to unloaded extensions?
					 * Currently the extension manager will ignore this.
					 */
					if (!pExt || !pExt->IsRunning(NULL, 0))
					{
						if (error)
						{
							snprintf(error, maxlength, "Required extension \"%s\" file(\"%s\") not running", name, file);
						}
						return false;
					} else {
						g_Extensions.BindChildPlugin(pExt, pPlugin);
					}
				}
			}
		}
	}

	return true;
}

bool CPluginManager::RunSecondPass(CPlugin *pPlugin, char *error, size_t maxlength)
{
	/* Second pass for extension requirements */
	if (!LoadOrRequireExtensions(pPlugin, 2, error, maxlength))
	{
		return false;
	}

	/* Bind all extra natives */
	g_Extensions.BindAllNativesToPlugin(pPlugin);
	AddFakeNativesToPlugin(pPlugin);

	/* Find any unbound natives
	 * Right now, these are not allowed
	 */
	IPluginContext *pContext = pPlugin->GetBaseContext();
	uint32_t num = pContext->GetNativesNum();
	sp_native_t *native;
	for (unsigned int i=0; i<num; i++)
	{
		if (pContext->GetNativeByIndex(i, &native) != SP_ERROR_NONE)
		{
			break;
		}
		if (native->status == SP_NATIVE_UNBOUND)
		{
			if (error)
			{
				snprintf(error, maxlength, "Native \"%s\" was not found.", native->name);
			}
			return false;
		}
	}

	/* Finish by telling all listeners */
	List<IPluginsListener *>::iterator iter;
	IPluginsListener *pListener;
	for (iter=m_listeners.begin(); iter!=m_listeners.end(); iter++)
	{
		pListener = (*iter);
		pListener->OnPluginLoaded(pPlugin);
	}
	
	/* Tell this plugin to finish initializing itself */
	pPlugin->Call_OnPluginStart();

	return true;
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

void CPluginManager::AddFakeNativesToPlugin(CPlugin *pPlugin)
{
	IPluginContext *pContext = pPlugin->GetBaseContext();
	sp_nativeinfo_t native;

	List<FakeNative *>::iterator iter;
	FakeNative *pNative;
	sp_context_t *ctx;
	for (iter = m_Natives.begin(); iter != m_Natives.end(); iter++)
	{
		pNative = (*iter);
		ctx = pNative->ctx->GetContext();
		if ((ctx->flags & SPFLAG_PLUGIN_PAUSED) == SPFLAG_PLUGIN_PAUSED)
		{
			/* Ignore natives in paused plugins */
			continue;
		}
		native.name = pNative->name.c_str();
		native.func = pNative->func;
		if (pContext->BindNative(&native) == SP_ERROR_NONE)
		{
			/* :TODO: add to dependency list */
		}
	}
}

bool CPluginManager::UnloadPlugin(IPlugin *plugin)
{
	CPlugin *pPlugin = (CPlugin *)plugin;

	/* This prevents removal during insertion or anything else weird */
	if (m_plugins.find(pPlugin) == m_plugins.end())
	{
		return false;
	}

	List<IPluginsListener *>::iterator iter;
	IPluginsListener *pListener;

	if (pPlugin->GetStatus() <= Plugin_Error)
	{
		/* Notify listeners of unloading */
		for (iter=m_listeners.begin(); iter!=m_listeners.end(); iter++)
		{
			pListener = (*iter);
			pListener->OnPluginUnloaded(pPlugin);
		}
		/* Notify plugin */
		pPlugin->Call_OnPluginEnd();
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
			if ((unsigned)(aliasptr - alias) == alias_len - 1)
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

bool CPluginManager::IsLateLoadTime() const
{
	return (m_AllPluginsLoaded || !g_SourceMod.IsMapLoading());
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

	g_RootMenu.AddRootConsoleCommand("plugins", "Manage Plugins", this);

	g_ShareSys.AddInterface(NULL, this);
}

void CPluginManager::OnSourceModShutdown()
{
	g_RootMenu.RemoveRootConsoleCommand("plugins", this);

	List<CPlugin *>::iterator iter;
	while ( (iter = m_plugins.begin()) != m_plugins.end() )
	{
		UnloadPlugin((*iter));
	}

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

CPlugin *CPluginManager::GetPluginByOrder(int num)
{
	if (num < 1 || num > (int)GetPluginCount())
	{
		return NULL;
	}

	CPlugin *pl;
	int id = 1;

	IPluginIterator *iter = GetPluginIterator();
	for (; iter->MorePlugins() && id<num; iter->NextPlugin(), id++) {}

	pl = (CPlugin *)(iter->GetPlugin());
	iter->Release();

	return pl;
}

const char *CPluginManager::GetStatusText(PluginStatus st)
{
	switch (st)
	{
	case Plugin_Running:
		return "Running";
	case Plugin_Paused:
		return "Paused";
	case Plugin_Error:
		return "Error";
	case Plugin_Uncompiled:
		return "Uncompiled";
	case Plugin_BadLoad:
		return "Bad Load";
	case Plugin_Failed:
		return "Failed";
	default:
		assert(false);
		return "-";
	}
}

void CPluginManager::OnRootConsoleCommand(const char *command, unsigned int argcount)
{
	if (argcount >= 3)
	{
		const char *cmd = g_RootMenu.GetArgument(2);
		if (strcmp(cmd, "list") == 0)
		{
			char buffer[256];
			unsigned int id = 1;
			int plnum = GetPluginCount();

			if (!plnum)
			{
				g_RootMenu.ConsolePrint("[SM] No plugins loaded");
				return;
			} else {
				g_RootMenu.ConsolePrint("[SM] Listing %d plugin%s:", GetPluginCount(), (plnum > 1) ? "s" : "");
			}

			IPluginIterator *iter = GetPluginIterator();
			for (; iter->MorePlugins(); iter->NextPlugin(), id++)
			{
				IPlugin *pl = iter->GetPlugin();
				assert(pl->GetStatus() != Plugin_Created);
				int len = 0;
				const sm_plugininfo_t *info = pl->GetPublicInfo();
				if (pl->GetStatus() != Plugin_Running)
				{
					len += UTIL_Format(buffer, sizeof(buffer), "  %02d <%s>", id, GetStatusText(pl->GetStatus()));
				} else {
					len += UTIL_Format(buffer, sizeof(buffer), "  %02d", id);
				}
				len += UTIL_Format(&buffer[len], sizeof(buffer)-len, " \"%s\"", (IS_STR_FILLED(info->name)) ? info->name : pl->GetFilename());
				if (IS_STR_FILLED(info->version))
				{
					len += UTIL_Format(&buffer[len], sizeof(buffer)-len, " (%s)", info->version);
				}
				if (IS_STR_FILLED(info->author))
				{
					UTIL_Format(&buffer[len], sizeof(buffer)-len, " by %s", info->author);
				}
				g_RootMenu.ConsolePrint("%s", buffer);
			}

			iter->Release();
			return;
		} else if (strcmp(cmd, "load") == 0) {
			if (argcount < 4)
			{
				g_RootMenu.ConsolePrint("[SM] Usage: sm plugins load <file>");
				return;
			}

			char error[128];
			bool wasloaded;
			const char *filename = g_RootMenu.GetArgument(3);
			IPlugin *pl = LoadPlugin(filename, false, PluginType_MapUpdated, error, sizeof(error), &wasloaded);

			if (wasloaded)
			{
				g_RootMenu.ConsolePrint("[SM] Plugin %s is already loaded.", filename);
				return;
			}

			if (pl)
			{
				g_RootMenu.ConsolePrint("[SM] Loaded plugin %s successfully.", filename);
			} else {
				g_RootMenu.ConsolePrint("[SM] Plugin %s failed to load: %s.", filename, error);
			}

			return;
		} else if (strcmp(cmd, "unload") == 0) {
			if (argcount < 4)
			{
				g_RootMenu.ConsolePrint("[SM] Usage: sm plugins unload <#>");
				return;
			}

			int id = 1;
			int num = atoi(g_RootMenu.GetArgument(3));
			if (num < 1 || num > (int)GetPluginCount())
			{
				g_RootMenu.ConsolePrint("[SM] Plugin index %d not found.", num);
				return;
			}

			IPluginIterator *iter = GetPluginIterator();
			for (; iter->MorePlugins() && id<num; iter->NextPlugin(), id++)
				;
			IPlugin *pl = iter->GetPlugin();

			char name[PLATFORM_MAX_PATH+1];
			const sm_plugininfo_t *info = pl->GetPublicInfo();
			strcpy(name, (IS_STR_FILLED(info->name)) ? info->name : pl->GetFilename());

			if (UnloadPlugin(pl))
			{
				g_RootMenu.ConsolePrint("[SM] Plugin %s unloaded successfully.", name);
			} else {
				g_RootMenu.ConsolePrint("[SM] Failed to unload plugin %s.", name);
			}

			iter->Release();
			return;
		} else if (strcmp(cmd, "info") == 0) {
			if (argcount < 4)
			{
				g_RootMenu.ConsolePrint("[SM] Usage: sm plugins info <#>");
				return;
			}

			int num = atoi(g_RootMenu.GetArgument(3));
			if (num < 1 || num > (int)GetPluginCount())
			{
				g_RootMenu.ConsolePrint("[SM] Plugin index not found.");
				return;
			}

			CPlugin *pl = GetPluginByOrder(num);
			const sm_plugininfo_t *info = pl->GetPublicInfo();

			g_RootMenu.ConsolePrint("  Filename: %s", pl->GetFilename());
			if (pl->GetStatus() <= Plugin_Error)
			{
				if (IS_STR_FILLED(info->name))
				{
					g_RootMenu.ConsolePrint("  Title: %s", info->name);
				}
				if (IS_STR_FILLED(info->version))
				{
					g_RootMenu.ConsolePrint("  Version: %s", info->version);
				}
				if (IS_STR_FILLED(info->url))
				{
					g_RootMenu.ConsolePrint("  URL: %s", info->url);
				}
				if (IS_STR_FILLED(info->author))
				{
					g_RootMenu.ConsolePrint("  Author: %s", info->author);
				}
				if (IS_STR_FILLED(info->version))
				{
					g_RootMenu.ConsolePrint("  Version: %s", info->version);
				}
				if (IS_STR_FILLED(info->description))
				{
					g_RootMenu.ConsolePrint("  Description: %s", info->description);
				}	
				g_RootMenu.ConsolePrint("  Debugging: %s", pl->IsDebugging() ? "yes" : "no");
				g_RootMenu.ConsolePrint("  Paused: %s", pl->GetStatus() == Plugin_Running ? "no" : "yes");
			} else {
				g_RootMenu.ConsolePrint("  Load error: %s", pl->m_errormsg);
				g_RootMenu.ConsolePrint("  File info: (title \"%s\") (version \"%s\")", 
										info->name ? info->name : "<none>",
										info->version ? info->version : "<none>");
				if (IS_STR_FILLED(info->url))
				{
					g_RootMenu.ConsolePrint("  File URL: %s", info->url);
				}
			}

			return;
		} else if (strcmp(cmd, "debug") == 0) {
			if (argcount < 5)
			{
				g_RootMenu.ConsolePrint("[SM] Usage: sm plugins debug <#> [on|off]");
				return;
			}

			int num = atoi(g_RootMenu.GetArgument(3));
			if (num < 1 || num > (int)GetPluginCount())
			{
				g_RootMenu.ConsolePrint("[SM] Plugin index not found.");
				return;
			}

			int res;
			const char *mode = g_RootMenu.GetArgument(4);
			if ((res=strcmp("on", mode)) && strcmp("off", mode))
			{
				g_RootMenu.ConsolePrint("[SM] The only possible options are \"on\" and \"off.\"");
				return;
			}

			bool debug;
			if (!res)
			{
				debug = true;
			} else {
				debug = false;
			}

			CPlugin *pl = GetPluginByOrder(num);
			if (debug && pl->IsDebugging())
			{
				g_RootMenu.ConsolePrint("[SM] This plugin is already in debug mode.");
				return;
			} else if (!debug && !pl->IsDebugging()) {
				g_RootMenu.ConsolePrint("[SM] Debug mode is already disabled in this plugin.");
				return;
			}

			char error[256];
			if (pl->ToggleDebugMode(debug, error, sizeof(error)))
			{
				g_RootMenu.ConsolePrint("[SM] Successfully toggled debug mode on plugin %s.", pl->GetFilename());
				return;
			} else {
				g_RootMenu.ConsolePrint("[SM] Could not toggle debug mode on plugin %s.", pl->GetFilename());
				g_RootMenu.ConsolePrint("[SM] Plugin returned error: %s", error);
				return;
			}
		}
	}

	/* Draw the main menu */
	g_RootMenu.ConsolePrint("SourceMod Plugins Menu:");
	g_RootMenu.DrawGenericOption("list", "Show loaded plugins");
	g_RootMenu.DrawGenericOption("load", "Load a plugin");
	g_RootMenu.DrawGenericOption("unload", "Unload a plugin");
	g_RootMenu.DrawGenericOption("info", "Information about a plugin");
	g_RootMenu.DrawGenericOption("debug", "Toggle debug mode on a plugin");
}

void CPluginManager::ReloadOrUnloadPlugins()
{
	List<CPlugin *>::iterator iter;
	List<CPlugin *> tmp_list = m_plugins;
	CPlugin *pl;
	time_t t;

	for (iter=tmp_list.begin(); iter!=tmp_list.end(); iter++)
	{
		pl = (*iter);
		if (pl->GetType() ==  PluginType_MapOnly)
		{
			UnloadPlugin((IPlugin *)pl);
		} else if (pl->GetType() == PluginType_MapUpdated) {
			t = pl->GetFileTimeStamp();
			if (!t || t > pl->GetTimeStamp())
			{
				pl->SetTimeStamp(t);
				UnloadPlugin((IPlugin *)pl);
			}
		}
	}
}

void CPluginManager::_SetPauseState(CPlugin *pl, bool paused)
{
	List<IPluginsListener *>::iterator iter;
	IPluginsListener *pListener;
	for (iter=m_listeners.begin(); iter!=m_listeners.end(); iter++)
	{
		pListener = (*iter);
		pListener->OnPluginPauseChange(pl, paused);
	}	
}

bool CPluginManager::AddFakeNative(IPluginFunction *pFunction, const char *name, SPVM_FAKENATIVE_FUNC func)
{
	if (sm_trie_retrieve(m_pNativeLookup, name, NULL))
	{
		return false;
	}

	FakeNative *pNative = new FakeNative;

	pNative->func = g_pVM->CreateFakeNative(func, pNative);
	if (!pNative->func)
	{
		delete pNative;
		return false;
	}

	pNative->call = pFunction;
	pNative->name.assign(name);
	pNative->ctx = pFunction->GetParentContext();

	m_Natives.push_back(pNative);
	sm_trie_insert(m_pNativeLookup, name,pNative);

	return true;
}
