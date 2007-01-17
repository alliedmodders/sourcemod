#include "ExtensionSys.h"
#include "LibrarySys.h"
#include "ShareSys.h"
#include "CLogger.h"
#include "sourcemm_api.h"
#include "PluginSys.h"

CExtensionManager g_Extensions;
IdentityType_t g_ExtType;

CExtension::CExtension(const char *filename, char *error, size_t err_max)
{
	m_File.assign(filename);
	m_pAPI = NULL;
	m_pIdentToken = NULL;
	m_PlId = 0;

	char path[PLATFORM_MAX_PATH+1];
	g_LibSys.PathFormat(path, PLATFORM_MAX_PATH, "%s/extensions/%s", g_SourceMod.GetSMBaseDir(), filename);

	m_pLib = g_LibSys.OpenLibrary(path, error, err_max);

	if (m_pLib == NULL)
	{
		return;
	}

	typedef IExtensionInterface *(*GETAPI)();
	GETAPI pfnGetAPI = NULL;

	if ((pfnGetAPI=(GETAPI)m_pLib->GetSymbolAddress("GetSMExtAPI")) == NULL)
	{
		m_pLib->CloseLibrary();
		m_pLib = NULL;
		snprintf(error, err_max, "Unable to find extension entry point");
		return;
	}

	m_pAPI = pfnGetAPI();
	if (!m_pAPI || m_pAPI->GetExtensionVersion() > SMINTERFACE_EXTENSIONAPI_VERSION)
	{
		m_pLib->CloseLibrary();
		m_pLib = NULL;
		snprintf(error, err_max, "Extension version is too new to load (%d, max is %d)", m_pAPI->GetExtensionVersion(), SMINTERFACE_EXTENSIONAPI_VERSION);
		return;
	}

	if (m_pAPI->IsMetamodExtension())
	{
		bool already;
		m_PlId = g_pMMPlugins->Load(path, g_PLID, already, error, err_max);
	}

	m_pIdentToken = g_ShareSys.CreateIdentity(g_ExtType);

	if (!m_pAPI->OnExtensionLoad(this, &g_ShareSys, error, err_max, g_SourceMod.IsLateLoadInMap()))
	{
		if (m_pAPI->IsMetamodExtension())
		{
			if (m_PlId)
			{
				char dummy[255];
				g_pMMPlugins->Unload(m_PlId, true, dummy, sizeof(dummy));
				m_PlId = 0;
			}
		}
		m_pAPI = NULL;
		m_pLib->CloseLibrary();
		m_pLib = NULL;
		g_ShareSys.DestroyIdentity(m_pIdentToken);
		m_pIdentToken = NULL;
		return;
	}
}

CExtension::~CExtension()
{
	if (m_pAPI)
	{
		m_pAPI->OnExtensionUnload();
		if (m_PlId)
		{
			g_pMMPlugins->Unload(m_PlId, true, NULL, 0);
		}
	}

	if (m_pIdentToken)
	{
		g_ShareSys.DestroyIdentity(m_pIdentToken);
	}

	if (m_pLib)
	{
		m_pLib->CloseLibrary();
	}
}

void CExtension::AddPlugin(IPlugin *pPlugin)
{
	m_Plugins.push_back(pPlugin);
}

void CExtension::RemovePlugin(IPlugin *pPlugin)
{
	m_Plugins.remove(pPlugin);
}

void CExtension::SetError(const char *error)
{
	m_Error.assign(error);
}

IExtensionInterface *CExtension::GetAPI()
{
	return m_pAPI;
}

const char *CExtension::GetFilename()
{
	return m_File.c_str();
}

IdentityToken_t *CExtension::GetIdentity()
{
	return m_pIdentToken;
}

bool CExtension::IsLoaded()
{
	return (m_pLib != NULL);
}

void CExtension::AddDependency(IfaceInfo *pInfo)
{
	m_Deps.push_back(*pInfo);
}

ITERATOR *CExtension::FindFirstDependency(IExtension **pOwner, SMInterface **pInterface)
{
	List<IfaceInfo>::iterator iter = m_Deps.begin();

	if (iter == m_Deps.end())
	{
		return NULL;
	}

	if (pOwner)
	{
		*pOwner = (*iter).owner;
	}
	if (pInterface)
	{
		*pInterface = (*iter).iface;
	}

	List<IfaceInfo>::iterator *pIter = new List<IfaceInfo>::iterator(iter);

	return (ITERATOR *)pIter;
}

bool CExtension::FindNextDependency(ITERATOR *iter, IExtension **pOwner, SMInterface **pInterface)
{
	List<IfaceInfo>::iterator *pIter = (List<IfaceInfo>::iterator *)iter;
	List<IfaceInfo>::iterator _iter;

	if (_iter == m_Deps.end())
	{
		return false;
	}

	_iter++;

	if (pOwner)
	{
		*pOwner = (*_iter).owner;
	}
	if (pInterface)
	{
		*pInterface = (*_iter).iface;
	}

	*pIter = _iter;

	if (_iter == m_Deps.end())
	{
		return false;
	}

	return true;
}

void CExtension::FreeDependencyIterator(ITERATOR *iter)
{
	List<IfaceInfo>::iterator *pIter = (List<IfaceInfo>::iterator *)iter;

	delete pIter;
}

void CExtension::AddInterface(SMInterface *pInterface)
{
	m_Interfaces.push_back(pInterface);
}

/*********************
 * EXTENSION MANAGER *
 *********************/

void CExtensionManager::OnSourceModAllInitialized()
{
	g_ExtType = g_ShareSys.CreateIdentType("EXTENSION");
	g_PluginSys.AddPluginsListener(this);
}

void CExtensionManager::OnSourceModShutdown()
{
	g_PluginSys.RemovePluginsListener(this);
	g_ShareSys.DestroyIdentType(g_ExtType);
}

IExtension *CExtensionManager::LoadAutoExtension(const char *path)
{
	IExtension *pAlready;
	if ((pAlready=FindExtensionByFile(path)) != NULL)
	{
		return pAlready;
	}

	char error[256];
	CExtension *p = new CExtension(path, error, sizeof(error));

	if (!p->IsLoaded())
	{
		g_Logger.LogError("[SM] Unable to load extension \"%s\": %s", path, error);
		p->SetError(error);
	}

	m_Libs.push_back(p);

	return p;
}

IExtension *CExtensionManager::FindExtensionByFile(const char *file)
{
	List<CExtension *>::iterator iter;
	CExtension *pExt;

	/* Make sure the file direction is right */
	char path[PLATFORM_MAX_PATH+1];
	g_LibSys.PathFormat(path, PLATFORM_MAX_PATH, "%s", file);

	for (iter=m_Libs.begin(); iter!=m_Libs.end(); iter++)
	{
		pExt = (*iter);
		if (strcmp(pExt->GetFilename(), path) == 0)
		{
			return pExt;
		}
	}

	return NULL;
}

IExtension *CExtensionManager::FindExtensionByName(const char *ext)
{
	List<CExtension *>::iterator iter;
	CExtension *pExt;
	IExtensionInterface *pAPI;
	const char *name;

	for (iter=m_Libs.begin(); iter!=m_Libs.end(); iter++)
	{
		pExt = (*iter);
		if (!pExt->IsLoaded())
		{
			continue;
		}
		if ((pAPI = pExt->GetAPI()) == NULL)
		{
			continue;
		}
		name = pAPI->GetExtensionName();
		if (!name)
		{
			continue;
		}
		if (strcmp(name, ext) == 0)
		{
			return pExt;
		}
	}

	return NULL;
}

IExtension *CExtensionManager::LoadExtension(const char *file, ExtensionLifetime lifetime, char *error, size_t err_max)
{
	IExtension *pAlready;
	if ((pAlready=FindExtensionByFile(file)) != NULL)
	{
		return pAlready;
	}

	CExtension *pExt = new CExtension(file, error, err_max);

	/* :NOTE: lifetime is currently ignored */

	if (!pExt->IsLoaded())
	{
		delete pExt;
		return NULL;
	}

	m_Libs.push_back(pExt);

	return pExt;
}

void CExtensionManager::BindDependency(IExtension *pOwner, IfaceInfo *pInfo)
{
	CExtension *pExt = (CExtension *)pOwner;

	pExt->AddDependency(pInfo);
}

void CExtensionManager::AddInterface(IExtension *pOwner, SMInterface *pInterface)
{
	CExtension *pExt = (CExtension *)pOwner;

	pExt->AddInterface(pInterface);
}

void CExtensionManager::BindChildPlugin(IExtension *pParent, IPlugin *pPlugin)
{
	CExtension *pExt = (CExtension *)pParent;

	pExt->AddPlugin(pPlugin);
}

void CExtensionManager::OnPluginDestroyed(IPlugin *plugin)
{
	List<CExtension *>::iterator iter;

	for (iter=m_Libs.begin(); iter!=m_Libs.end(); iter++)
	{
		(*iter)->RemovePlugin(plugin);
	}
}

bool CExtensionManager::UnloadExtension(IExtension *_pExt)
{
	if (!_pExt)
	{
		return false;
	}

	CExtension *pExt = (CExtension *)_pExt;

	if (m_Libs.find(pExt) == m_Libs.end())
	{
		return false;
	}

	/* First remove us from internal lists */
	g_ShareSys.RemoveInterfaces(_pExt);
	m_Libs.remove(pExt);

	List<CExtension *> UnloadQueue;

	/* Handle dependencies */
	if (pExt->IsLoaded())
	{
		/* Unload any dependent plugins */
		List<IPlugin *>::iterator p_iter = pExt->m_Plugins.begin();
		while (p_iter != pExt->m_Plugins.end())
		{
			g_PluginSys.UnloadPlugin((*p_iter));
			/* It should already have been removed! */
			assert(pExt->m_Plugins.find((*p_iter)) != pExt->m_Plugins.end());
		}

		/* Notify and/or unload all dependencies */
		List<CExtension *>::iterator c_iter;
		CExtension *pDep;
		IExtensionInterface *pAPI;
		for (c_iter = m_Libs.begin(); c_iter != m_Libs.end(); c_iter++)
		{
			pDep = (*c_iter);
			if ((pAPI=pDep->GetAPI()) == NULL)
			{
				continue;
			}
			/* Now, get its dependency list */
			bool dropped = false;
			List<IfaceInfo>::iterator i_iter = pDep->m_Deps.begin();
			while (i_iter != pDep->m_Deps.end())
			{
				if ((*i_iter).owner == _pExt)
				{
					if (!dropped && !pAPI->QueryInterfaceDrop((*i_iter).iface))
					{
						dropped = true;
					}
					pAPI->NotifyInterfaceDrop((*i_iter).iface);
					i_iter = pDep->m_Deps.erase(i_iter);
				} else {
					i_iter++;
				}
			}
		}
	}

	List<CExtension *>::iterator iter;
	for (iter=UnloadQueue.begin(); iter!=UnloadQueue.end(); iter++)
	{
		/* NOTE: This is safe because the unload function backs out of anything not present */
		UnloadExtension((*iter));
	}

	return true;
}
