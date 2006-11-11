#include <stdio.h>
#include "PluginSys.h"

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

	return pPlugin;
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
