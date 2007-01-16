#include "ExtensionSys.h"
#include "LibrarySys.h"
#include "ShareSys.h"
#include "CLogger.h"

CExtensionManager g_Extensions;
IdentityType_t g_ExtType;

CExtension::CExtension(const char *filename, char *error, size_t err_max)
{
	m_File.assign(filename);
	m_pAPI = NULL;
	m_pIdentToken = NULL;

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
		/* :TODO: STUFF */
	}

	m_pIdentToken = g_ShareSys.CreateIdentity(g_ExtType);

	if (!m_pAPI->OnExtensionLoad(this, &g_ShareSys, error, err_max, g_SourceMod.IsLateLoadInMap()))
	{
		if (m_pAPI->IsMetamodExtension())
		{
			/* :TODO: stuff */
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

void CExtensionManager::OnSourceModAllInitialized()
{
	g_ExtType = g_ShareSys.CreateIdentType("EXTENSION");
}

void CExtensionManager::OnSourceModShutdown()
{
	g_ShareSys.DestroyIdentType(g_ExtType);
}

IExtension *CExtensionManager::LoadAutoExtension(const char *path)
{
	char error[256];
	CExtension *p = new CExtension(path, error, sizeof(error));

	if (!p->IsLoaded())
	{
		g_Logger.LogError("[SOURCEMOD] Unable to load extension \"%s\": %s", path, error);
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

bool CExtensionManager::UnloadExtension(IExtension *pExt)
{
	/* :TODO: implement */
	return true;
}

unsigned int CExtensionManager::NumberOfPluginDependents(IExtension *pExt, unsigned int *optional)
{
	/* :TODO: implement */
	return 0;
}

bool CExtensionManager::IsExtensionUnloadable(IExtension *pExtension)
{
	/* :TODO: implement */
	return true;
}
