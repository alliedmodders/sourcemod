#include <stdio.h>
#include "PluginSys.h"
#include "LibrarySys.h"
#include "sourcemm_api.h"
#include "CTextParsers.h"

CPluginManager g_PluginMngr;

CPluginManager::CPluginManager()
{
}

CPlugin *CPlugin::CreatePlugin(const char *file, 
								bool debug_default, 
								PluginType type, 
								char *error, 
								size_t maxlen)
{
	static unsigned int MySerial = 0;
	FILE *fp = fopen(file, "rb");

	if (!fp)
	{
		snprintf(error, maxlen, "Could not open file");
		return NULL;
	}

	int err;
	sp_plugin_t *pl = g_pSourcePawn->LoadFromFilePointer(fp, &err);
	if (pl == NULL)
	{
		snprintf(error, maxlen, "Could not load plugin, error %d", err);
		return NULL;
	}

	fclose(fp);

	ICompilation *co = g_pVM->StartCompilation(pl);
	
	if (debug_default)
	{
		if (!g_pVM->SetCompilationOption(co, "debug", "1"))
		{
			g_pVM->AbortCompilation(co);
			snprintf(error, maxlen, "Could not set plugin to debug mode");
			return NULL;
		}
	}

	sp_context_t *ctx = g_pVM->CompileToContext(co, &err);
	if (ctx == NULL)
	{
		snprintf(error, maxlen, "Plugin failed to load, JIT error: %d", err);
		return NULL;
	}

	IPluginContext *base = g_pSourcePawn->CreateBaseContext(ctx);
	CPlugin *pPlugin = new CPlugin;

	snprintf(pPlugin->m_filename, PLATFORM_MAX_PATH, "%s", file);
	pPlugin->m_debugging = debug_default;
	pPlugin->m_ctx_current.base = base;
	pPlugin->m_ctx_current.ctx = ctx;
	pPlugin->m_type = type;
	pPlugin->m_serial = ++MySerial;
	pPlugin->m_status = Plugin_Loaded;
	pPlugin->m_plugin = pl;

	pPlugin->UpdateInfo();

	ctx->user[SM_CONTEXTVAR_MYSELF] = (void *)(IPlugin *)pPlugin;

	/* Build function information loosely */
	pPlugin->m_funcsnum = g_pVM->FunctionCount(ctx);

	if (pPlugin->m_funcsnum)
	{
		pPlugin->m_priv_funcs = new CFunction *[pPlugin->m_funcsnum];
		memset(pPlugin->m_priv_funcs, 0, sizeof(CFunction *) * pPlugin->m_funcsnum);
	} else {
		pPlugin->m_priv_funcs = NULL;
	}

	if (pl->info.publics_num)
	{
		pPlugin->m_pub_funcs = new CFunction *[pl->info.publics_num];
		memset(pPlugin->m_pub_funcs, 0, sizeof(CFunction *) * pl->info.publics_num);
	} else {
		pPlugin->m_pub_funcs = NULL;
	}

	return pPlugin;
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
			pFunc = g_PluginMngr.GetFunctionFromPool(save, this);
			m_pub_funcs[func_id] = pFunc;
		}
	} else {
		func_id >>= 1;
		unsigned int index;
		if (!g_pVM->FunctionLookup(m_ctx_current.ctx, func_id, &index))
		{
			return NULL;
		}
		pFunc = m_priv_funcs[func_id];
		if (!pFunc)
		{
			pFunc = g_PluginMngr.GetFunctionFromPool(save, this);
			m_priv_funcs[func_id] = pFunc;
		}
	}

	return pFunc;
}

IPluginFunction *CPlugin::GetFunctionByName(const char *public_name)
{
	uint32_t index;
	IPluginContext *base = m_ctx_current.base;

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
			pFunc = g_PluginMngr.GetFunctionFromPool(pub->funcid, this);
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

const sp_plugin_t *CPlugin::GetPluginStructure() const
{
	return m_plugin;
}

IPluginContext *CPlugin::GetBaseContext() const
{
	return m_ctx_current.base;
}

sp_context_t *CPlugin::GetContext() const
{
	return m_ctx_current.ctx;
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
	return m_debugging;
}

bool CPlugin::SetPauseState(bool paused)
{
	if (paused && GetStatus() != Plugin_Paused)
	{
		return false;
	} else if (!paused && GetStatus() != Plugin_Running) {
		return false;
	}
	
	/* :TODO: execute some forwards or some crap */

	return true;
}

/*******************
 * PLUGIN ITERATOR *
 *******************/

CPluginManager::CPluginIterator::CPluginIterator(List<IPlugin *> *_mylist)
{
	mylist = _mylist;
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
	g_PluginMngr.ReleaseIterator(this);
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

void CPluginManager::RefreshOrLoadPlugins(const char *config, const char *basedir)
{
	/* First read in the database of plugin settings */
	SMCParseError err;
	unsigned int line, col;
	if ((err=g_TextParse.ParseFile_SMC(config, &m_PluginInfo, &line, &col)) != SMCParse_Okay)
	{
		/* :TODO: log the error, don't bail out though */
	}


	//:TODO: move this to a separate recursive function and do stuff
	/*IDirectory *dir = g_LibSys.OpenDirectory(basedir);
	while (dir->MoreFiles())
	{
		if (dir->IsEntryDirectory() && (strcmp(dir->GetEntryName(), "disabled") != 0))
		{
			char path[PLATFORM_MAX_PATH+1];
			g_SMAPI->PathFormat(path, sizeof(path)-1, "%s/%s", basedir, dir->GetEntryName());
			RefreshOrLoadPlugins(basedir);
		}
	}
	g_LibSys.CloseDirectory(dir);*/
}

IPlugin *CPluginManager::LoadPlugin(const char *path, bool debug, PluginType type, char error[], size_t err_max)
{
	CPlugin *pPlugin = CPlugin::CreatePlugin(path, debug, type, error, err_max);

	if (!pPlugin)
	{
		return NULL;
	}

	m_plugins.push_back(pPlugin);

	List<IPluginsListener *>::iterator iter;
	IPluginsListener *pListener;
	for (iter=m_listeners.begin(); iter!=m_listeners.end(); iter++)
	{
		pListener = (*iter);
		pListener->OnPluginCreated(pPlugin);
	}

	/* :TODO: a lot more... */

	return pPlugin;
}

bool CPluginManager::UnloadPlugin(IPlugin *plugin)
{
	CPlugin *pPlugin = (CPlugin *)plugin;

	/* :TODO: More */

	List<IPluginsListener *>::iterator iter;
	IPluginsListener *pListener;
	for (iter=m_listeners.begin(); iter!=m_listeners.end(); iter++)
	{
		pListener = (*iter);
		pListener->OnPluginDestroyed(pPlugin);
	}
	
	if (pPlugin->m_pub_funcs)
	{
		for (uint32_t i=0; i<pPlugin->m_plugin->info.publics_num; i++)
		{
			g_PluginMngr.ReleaseFunctionToPool(pPlugin->m_pub_funcs[i]);
		}
		delete [] pPlugin->m_pub_funcs;
		pPlugin->m_pub_funcs = NULL;
	}

	if (pPlugin->m_priv_funcs)
	{
		for (unsigned int i=0; i<pPlugin->m_funcsnum; i++)
		{
			g_PluginMngr.ReleaseFunctionToPool(pPlugin->m_priv_funcs[i]);
		}
		delete [] pPlugin->m_priv_funcs;
		pPlugin->m_priv_funcs = NULL;
	}

	if (pPlugin->m_ctx_current.base)
	{
		g_pSourcePawn->FreeBaseContext(pPlugin->m_ctx_current.base);
	}
	if (pPlugin->m_ctx_backup.base)
	{
		g_pSourcePawn->FreeBaseContext(pPlugin->m_ctx_backup.base);
	}
	if (pPlugin->m_ctx_current.ctx)
	{
		pPlugin->m_ctx_current.ctx->vmbase->FreeContext(pPlugin->m_ctx_current.ctx);
	}
	if (pPlugin->m_ctx_backup.ctx)
	{
		pPlugin->m_ctx_backup.ctx->vmbase->FreeContext(pPlugin->m_ctx_backup.ctx);
	}

	g_pSourcePawn->FreeFromMemory(pPlugin->m_plugin);

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

	if (alias_path_end == alias_len - 1)
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

CPluginManager::~CPluginManager()
{
	//:TODO: we need a good way to free what we're holding
}
