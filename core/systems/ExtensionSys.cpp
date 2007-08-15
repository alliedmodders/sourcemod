/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2007 AlliedModders LLC.  All rights reserved.
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

#include <stdlib.h>
#include "ExtensionSys.h"
#include "LibrarySys.h"
#include "ShareSys.h"
#include "Logger.h"
#include "sourcemm_api.h"
#include "sm_srvcmds.h"
#include "sm_stringutil.h"

CExtensionManager g_Extensions;
IdentityType_t g_ExtType;

CExtension::CExtension(const char *filename, char *error, size_t maxlength)
{
	m_File.assign(filename);
	m_pAPI = NULL;
	m_pIdentToken = NULL;
	m_PlId = 0;
	unload_code = 0;
	m_FullyLoaded = false;

	char path[PLATFORM_MAX_PATH];
	g_SourceMod.BuildPath(Path_SM, path, PLATFORM_MAX_PATH, "extensions/%s", filename);
	m_Path.assign(path);

	m_pLib = NULL;
}

bool CExtension::Load(char *error, size_t maxlength)
{
	m_pLib = g_LibSys.OpenLibrary(m_Path.c_str(), error, maxlength);

	if (m_pLib == NULL)
	{
		return false;
	}

	typedef IExtensionInterface *(*GETAPI)();
	GETAPI pfnGetAPI = NULL;

	if ((pfnGetAPI=(GETAPI)m_pLib->GetSymbolAddress("GetSMExtAPI")) == NULL)
	{
		m_pLib->CloseLibrary();
		m_pLib = NULL;
		snprintf(error, maxlength, "Unable to find extension entry point");
		return false;
	}

	m_pAPI = pfnGetAPI();
	if (!m_pAPI || m_pAPI->GetExtensionVersion() > SMINTERFACE_EXTENSIONAPI_VERSION)
	{
		m_pLib->CloseLibrary();
		m_pLib = NULL;
		snprintf(error, maxlength, "Extension version is too new to load (%d, max is %d)", m_pAPI->GetExtensionVersion(), SMINTERFACE_EXTENSIONAPI_VERSION);
		return false;
	}

	if (m_pAPI->IsMetamodExtension())
	{
		bool already;
		m_PlId = g_pMMPlugins->Load(m_Path.c_str(), g_PLID, already, error, maxlength);
	}

	m_pIdentToken = g_ShareSys.CreateIdentity(g_ExtType, this);

	if (!m_pAPI->OnExtensionLoad(this, &g_ShareSys, error, maxlength, !g_SourceMod.IsMapLoading()))
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
		return false;
	} else {
		/* Check if we're past load time */
		if (!g_SourceMod.IsMapLoading())
		{
			m_pAPI->OnExtensionsAllLoaded();
		}
	}

	return true;
}

CExtension::~CExtension()
{
	if (m_pAPI)
	{
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

void CExtension::MarkAllLoaded()
{
	assert(m_FullyLoaded == false);
	if (!m_FullyLoaded)
	{
		m_FullyLoaded = true;
		m_pAPI->OnExtensionsAllLoaded();
	}
}

void CExtension::AddPlugin(IPlugin *pPlugin)
{
	/* Unfortunately we have to do this :( */
	if (m_Plugins.find(pPlugin) != m_Plugins.end())
	{
		m_Plugins.push_back(pPlugin);
	}
}

void CExtension::RemovePlugin(IPlugin *pPlugin)
{
	m_Plugins.remove(pPlugin);

	List<WeakNative>::iterator iter = m_WeakNatives.begin();
	while (iter != m_WeakNatives.end())
	{
		if ((*iter).pl == pPlugin)
		{
			iter = m_WeakNatives.erase(iter);
		} else {
			iter++;
		}
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

void CExtension::AddDependency(IfaceInfo *pInfo)
{
	m_Deps.push_back(*pInfo);
}

bool operator ==(const IfaceInfo &i1, const IfaceInfo &i2)
{
	return (i1.iface == i2.iface) && (i1.owner == i2.owner);
}

void CExtension::AddChildDependent(CExtension *pOther, SMInterface *iface)
{
	IfaceInfo info;
	info.iface = iface;
	info.owner = pOther;

	List<IfaceInfo>::iterator iter;
	for (iter = m_ChildDeps.begin();
		 iter != m_ChildDeps.end();
		 iter++)
	{
		IfaceInfo &other = (*iter);
		if (other == info)
		{
			return;
		}
	}

	m_ChildDeps.push_back(info);
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

bool CExtension::IsRunning(char *error, size_t maxlength)
{
	if (!IsLoaded())
	{
		if (error)
		{
			snprintf(error, maxlength, "%s", m_Error.c_str());
		}
		return false;
	}

	return m_pAPI->QueryRunning(error, maxlength);
}

/*********************
 * EXTENSION MANAGER *
 *********************/

void CExtensionManager::OnSourceModAllInitialized()
{
	g_ExtType = g_ShareSys.CreateIdentType("EXTENSION");
	g_PluginSys.AddPluginsListener(this);
	g_RootMenu.AddRootConsoleCommand("exts", "Manage extensions", this);
	g_ShareSys.AddInterface(NULL, this);
}

void CExtensionManager::OnSourceModShutdown()
{
	g_RootMenu.RemoveRootConsoleCommand("exts", this);
	g_ShareSys.DestroyIdentType(g_ExtType);
}

void CExtensionManager::Shutdown()
{
	List<CExtension *>::iterator iter;

	while ((iter = m_Libs.begin()) != m_Libs.end())
	{
		UnloadExtension((*iter));
	}
}

void CExtensionManager::TryAutoload()
{
	char path[PLATFORM_MAX_PATH];

	g_SourceMod.BuildPath(Path_SM, path, sizeof(path), "extensions");

	IDirectory *pDir = g_LibSys.OpenDirectory(path);
	if (!pDir)
	{
		return;
	}

	const char *lfile;
	size_t len;
	while (pDir->MoreFiles())
	{
		if (pDir->IsEntryDirectory())
		{
			pDir->NextEntry();
			continue;
		}

		lfile = pDir->GetEntryName();
		len = strlen(lfile);
		if (len <= 9) /* size of ".autoload" */
		{
			pDir->NextEntry();
			continue;
		}

		if (strcmp(&lfile[len - 9], ".autoload") != 0)
		{
			pDir->NextEntry();
			continue;
		}

		char file[PLATFORM_MAX_PATH];
		len = UTIL_Format(file, sizeof(file), "%s", lfile);
		strcpy(&file[len - 9], ".ext." PLATFORM_LIB_EXT);

		LoadAutoExtension(file);
		
		pDir->NextEntry();
	}
}

IExtension *CExtensionManager::LoadAutoExtension(const char *path)
{
	if (!strstr(path, "." PLATFORM_LIB_EXT))
	{
		char newpath[PLATFORM_MAX_PATH];
		snprintf(newpath, PLATFORM_MAX_PATH, "%s.%s", path, PLATFORM_LIB_EXT);
		return LoadAutoExtension(newpath);
	}

	IExtension *pAlready;
	if ((pAlready=FindExtensionByFile(path)) != NULL)
	{
		return pAlready;
	}

	char error[256];
	CExtension *p = new CExtension(path, error, sizeof(error));

	/* We put us in the list beforehand so extensions that check for each other
	 * won't recursively load each other.
	 */
	m_Libs.push_back(p);

	if (!p->Load(error, sizeof(error)) || !p->IsLoaded())
	{
		g_Logger.LogError("[SM] Unable to load extension \"%s\": %s", path, error);
		p->SetError(error);
	}

	return p;
}

IExtension *CExtensionManager::FindExtensionByFile(const char *file)
{
	List<CExtension *>::iterator iter;
	CExtension *pExt;

	if (!strstr(file, "." PLATFORM_LIB_EXT))
	{
		char path[PLATFORM_MAX_PATH];
		snprintf(path, PLATFORM_MAX_PATH, "%s.%s", file, PLATFORM_LIB_EXT);
		return FindExtensionByFile(path);
	}

	/* Make sure the file direction is right */
	char path[PLATFORM_MAX_PATH];
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

IExtension *CExtensionManager::LoadExtension(const char *file, ExtensionLifetime lifetime, char *error, size_t maxlength)
{
	IExtension *pAlready;
	if ((pAlready=FindExtensionByFile(file)) != NULL)
	{
		return pAlready;
	}

	CExtension *pExt = new CExtension(file, error, maxlength);

	/* :NOTE: lifetime is currently ignored */

	if (!pExt->Load(error, maxlength) || !pExt->IsLoaded())
	{
		delete pExt;
		return NULL;
	}

	/* :TODO: do QueryRunning check if the map is loaded */

	m_Libs.push_back(pExt);

	return pExt;
}

void CExtensionManager::BindDependency(IExtension *pRequester, IfaceInfo *pInfo)
{
	CExtension *pExt = (CExtension *)pRequester;
	CExtension *pOwner = (CExtension *)pInfo->owner;

	pExt->AddDependency(pInfo);

	IExtensionInterface *pAPI = pExt->GetAPI();
	if (pAPI && !pAPI->QueryInterfaceDrop(pInfo->iface))
	{
		pOwner->AddChildDependent(pExt, pInfo->iface);
	}
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

CExtension *CExtensionManager::FindByOrder(unsigned int num)
{
	if (num < 1 || num > m_Libs.size())
	{
		return NULL;
	}

	List<CExtension *>::iterator iter = m_Libs.begin();

	while (iter != m_Libs.end())
	{
		if (--num == 0)
		{
			return (*iter);
		}
		iter++;
	}

	return NULL;
}

void CExtensionManager::BindAllNativesToPlugin(IPlugin *pPlugin)
{
	List<CExtension *>::iterator iter;
	CExtension *pExt;
	IPluginContext *pContext = pPlugin->GetBaseContext();
	for (iter=m_Libs.begin(); iter!=m_Libs.end(); iter++)
	{
		pExt = (*iter);
		if (!pExt->IsLoaded())
		{
			continue;
		}
		if (!pExt->m_Natives.size())
		{
			continue;
		}
		bool set = false;
		List<const sp_nativeinfo_t *>::iterator n_iter;
		const sp_nativeinfo_t *natives;
		for (n_iter = pExt->m_Natives.begin();
			 n_iter != pExt->m_Natives.end();
			 n_iter++)
		{
			natives = (*n_iter);
			uint32_t idx;
			unsigned int i=0;
			while (natives[i].func != NULL && natives[i].name != NULL)
			{
				if (pContext->BindNative(&natives[i]) == SP_ERROR_NONE)
				{
					pContext->FindNativeByName(natives[i].name, &idx);
					if (!(pContext->GetContext()->natives[idx].flags & SP_NTVFLAG_OPTIONAL))
					{
						set = true;
					} else {
						WeakNative wkn = WeakNative((CPlugin *)pPlugin, idx);
						pExt->m_WeakNatives.push_back(wkn);
					}
				}
				i++;
			}
		}
		if (set && (pExt->m_Plugins.find(pPlugin) == pExt->m_Plugins.end()))
		{
			/* For now, mark this plugin as a requirement.  Later we'll introduce native filtering. */
			pExt->m_Plugins.push_back(pPlugin);
		}
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
			/* We have to manually unlink ourselves here, since we're no longer being managed */
			g_PluginSys.UnloadPlugin((*p_iter));
			p_iter = pExt->m_Plugins.erase(p_iter);
		}

		/* Unbound weak natives */
		List<WeakNative>::iterator wkn_iter;
		for (wkn_iter=pExt->m_WeakNatives.begin(); wkn_iter!=pExt->m_WeakNatives.end(); wkn_iter++)
		{
			WeakNative & wkn = (*wkn_iter);
			sp_context_t *ctx = wkn.pl->GetContext();
			ctx->natives[wkn.idx].status = SP_NATIVE_UNBOUND;
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
			if (pDep == pExt)
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
					if (!pAPI->QueryInterfaceDrop((*i_iter).iface))
					{
						if (!dropped)
						{
							dropped = true;
							UnloadQueue.push_back(pDep);
						}
					}
					pAPI->NotifyInterfaceDrop((*i_iter).iface);
					i_iter = pDep->m_Deps.erase(i_iter);
				} else {
					i_iter++;
				}
			}
			/* Flush out any back references to this plugin */
			i_iter = pDep->m_ChildDeps.begin();
			while (i_iter != pDep->m_ChildDeps.end())
			{
				if ((*i_iter).owner == pExt)
				{
					i_iter = pDep->m_ChildDeps.erase(i_iter);
				} else {
					i_iter++;
				}
			}
		}

		/* Tell it to unload */
		pAPI = pExt->GetAPI();
		pAPI->OnExtensionUnload();
	}

	IdentityToken_t *pIdentity;
	if ((pIdentity = pExt->GetIdentity()) != NULL)
	{
		SMGlobalClass *glob = SMGlobalClass::head;
		while (glob)
		{
			glob->OnSourceModIdentityDropped(pIdentity);
			glob = glob->m_pGlobalClassNext;
		}
	}

	delete pExt;

	List<CExtension *>::iterator iter;
	for (iter=UnloadQueue.begin(); iter!=UnloadQueue.end(); iter++)
	{
		/* NOTE: This is safe because the unload function backs out of anything not present */
		UnloadExtension((*iter));
	}

	return true;
}

void CExtensionManager::AddNatives(IExtension *pOwner, const sp_nativeinfo_t *natives)
{
	CExtension *pExt = (CExtension *)pOwner;

	pExt->m_Natives.push_back(natives);
}

void CExtensionManager::MarkAllLoaded()
{
	List<CExtension *>::iterator iter;
	CExtension *pExt;

	for (iter=m_Libs.begin(); iter!=m_Libs.end(); iter++)
	{
		pExt = (*iter);
		if (!pExt->IsLoaded())
		{
			continue;
		}
		if (pExt->m_FullyLoaded)
		{
			continue;
		}
		pExt->MarkAllLoaded();
	}
}

void CExtensionManager::AddDependency(IExtension *pSource, const char *file, bool required, bool autoload)
{
	/* This function doesn't really need to do anything now.  We make sure the 
	 * other extension is loaded, but handling of dependencies is really done 
	 * by the interface fetcher.
	 */
	if (required || autoload)
	{
		LoadAutoExtension(file);
	}
}

void CExtensionManager::OnRootConsoleCommand(const char *cmd, unsigned int argcount)
{
	if (argcount >= 3)
	{
		const char *cmd = g_RootMenu.GetArgument(2);
		if (strcmp(cmd, "list") == 0)
		{
			List<CExtension *>::iterator iter;
			CExtension *pExt;
			unsigned int num = 1;
			switch (m_Libs.size())
			{
			case 1:
				{
					g_RootMenu.ConsolePrint("[SM] Displaying 1 extension:");
					break;
				}
			case 0:
				{
					g_RootMenu.ConsolePrint("[SM] No extensions are loaded.");
					break;
				}
			default:
				{
					g_RootMenu.ConsolePrint("[SM] Displaying %d extensions:", m_Libs.size());
					break;
				}
			}
			for (iter=m_Libs.begin(); iter!=m_Libs.end(); iter++,num++)
			{
				pExt = (*iter);
				if (pExt->IsLoaded())
				{
					char error[255];
					if (!pExt->IsRunning(error, sizeof(error)))
					{
						g_RootMenu.ConsolePrint("[%02d] <FAILED> file \"%s\": %s", num, pExt->GetFilename(), error);
					} else {
						IExtensionInterface *pAPI = pExt->GetAPI();
						const char *name = pAPI->GetExtensionName();
						const char *version = pAPI->GetExtensionVerString();
						const char *descr = pAPI->GetExtensionDescription();
						g_RootMenu.ConsolePrint("[%02d] %s (%s): %s", num, name, version, descr);
					}
				} else {
					g_RootMenu.ConsolePrint("[%02d] <FAILED> file \"%s\": %s", num, pExt->GetFilename(), pExt->m_Error.c_str());
				}
			}
			return;
		} else if (strcmp(cmd, "info") == 0) {
			if (argcount < 4)
			{
				g_RootMenu.ConsolePrint("[SM] Usage: sm info <#>");
				return;
			}

			const char *sId = g_RootMenu.GetArgument(3);
			unsigned int id = atoi(sId);
			if (id <= 0)
			{
				g_RootMenu.ConsolePrint("[SM] Usage: sm info <#>");
				return;
			}

			if (m_Libs.size() == 0)
			{
				g_RootMenu.ConsolePrint("[SM] No extensions are loaded.");
				return;
			}

			if (id > m_Libs.size())
			{
				g_RootMenu.ConsolePrint("[SM] No extension was found with id %d.", id);
				return;
			}

			List<CExtension *>::iterator iter = m_Libs.begin();
			CExtension *pExt = NULL;
			while (iter != m_Libs.end())
			{
				if (--id == 0)
				{
					pExt = (*iter);
					break;
				}
				iter++;
			}
			/* This should never happen */
			if (!pExt)
			{
				g_RootMenu.ConsolePrint("[SM] No extension was found with id %d.", id);
				return;
			}

			if (!pExt->IsLoaded())
			{
				g_RootMenu.ConsolePrint(" File: %s", pExt->GetFilename());
				g_RootMenu.ConsolePrint(" Loaded: No (%s)", pExt->m_Error.c_str());
			} else {
				char error[255];
				if (!pExt->IsRunning(error, sizeof(error)))
				{
					g_RootMenu.ConsolePrint(" File: %s", pExt->GetFilename());
					g_RootMenu.ConsolePrint(" Loaded: Yes");
					g_RootMenu.ConsolePrint(" Running: No (%s)", error);
				} else {
					IExtensionInterface *pAPI = pExt->GetAPI();
					g_RootMenu.ConsolePrint(" File: %s", pExt->GetFilename());
					g_RootMenu.ConsolePrint(" Loaded: Yes (version %s)", pAPI->GetExtensionVerString());
					g_RootMenu.ConsolePrint(" Name: %s (%s)", pAPI->GetExtensionName(), pAPI->GetExtensionDescription());
					g_RootMenu.ConsolePrint(" Author: %s (%s)", pAPI->GetExtensionAuthor(), pAPI->GetExtensionURL());
					g_RootMenu.ConsolePrint(" Binary info: API version %d (compiled %s)", pAPI->GetExtensionVersion(), pAPI->GetExtensionDateString());
					g_RootMenu.ConsolePrint(" Metamod enabled: %s", pAPI->IsMetamodExtension() ? "yes" : "no");
				}
			}
			return;
		} else if (strcmp(cmd, "unload") == 0) {
			if (argcount < 4)
			{
				g_RootMenu.ConsolePrint("[SM] Usage: sm unload <#> [code]");
				return;
			}

			const char *arg = g_RootMenu.GetArgument(3);
			unsigned int num = atoi(arg);
			CExtension *pExt = FindByOrder(num);

			if (!pExt)
			{
				g_RootMenu.ConsolePrint("[SM] Extension number %d was not found.", num);
				return;
			}

			if (argcount > 4 && pExt->unload_code)
			{
				const char *unload = g_RootMenu.GetArgument(4);
				if (pExt->unload_code == (unsigned)atoi(unload))
				{
					char filename[PLATFORM_MAX_PATH];
					snprintf(filename, PLATFORM_MAX_PATH, "%s", pExt->GetFilename());
					UnloadExtension(pExt);
					g_RootMenu.ConsolePrint("[SM] Extension %s is now unloaded.", filename);
				} else {
					g_RootMenu.ConsolePrint("[SM] Please try again, the correct unload code is \"%d\"", pExt->unload_code);
				}
				return;
			}

			if (!pExt->IsLoaded() 
				|| (!pExt->m_ChildDeps.size() && !pExt->m_Plugins.size()))
			{
				char filename[PLATFORM_MAX_PATH];
				snprintf(filename, PLATFORM_MAX_PATH, "%s", pExt->GetFilename());
				UnloadExtension(pExt);
				g_RootMenu.ConsolePrint("[SM] Extension %s is now unloaded.", filename);
				return;
			} else {
				List<IPlugin *> plugins;
				if (pExt->m_ChildDeps.size())
				{
					g_RootMenu.ConsolePrint("[SM] Unloading %s will unload the following extensions: ", pExt->GetFilename());
					List<CExtension *>::iterator iter;
					CExtension *pOther;
					/* Get list of all extensions */
					for (iter=m_Libs.begin(); iter!=m_Libs.end(); iter++)
					{
						List<IfaceInfo>::iterator i_iter;
						pOther = (*iter);
						if (!pOther->IsLoaded() || pOther == pExt)
						{
							continue;
						}
						/* Get their dependencies */
						for (i_iter=pOther->m_Deps.begin();
							 i_iter!=pOther->m_Deps.end();
							 i_iter++)
						{
							/* Is this dependency to us? */
							if ((*i_iter).owner != pExt)
							{
								continue;
							}
							/* Will our dependent care? */
							if (!pExt->GetAPI()->QueryInterfaceDrop((*i_iter).iface))
							{
								g_RootMenu.ConsolePrint(" -> %s", pExt->GetFilename());
								/* Add to plugin unload list */
								List<IPlugin *>::iterator p_iter;
								for (p_iter=pOther->m_Plugins.begin();
									 p_iter!=pOther->m_Plugins.end();
									 p_iter++)
								{
									if (plugins.find((*p_iter)) == plugins.end())
									{
										plugins.push_back((*p_iter));
									}
								}
							}
						}
					}
				}
				if (pExt->m_Plugins.size())
				{
					g_RootMenu.ConsolePrint("[SM] Unloading %s will unload the following plugins: ", pExt->GetFilename());
					List<IPlugin *>::iterator iter;
					IPlugin *pPlugin;
					for (iter = pExt->m_Plugins.begin(); iter != pExt->m_Plugins.end(); iter++)
					{
						pPlugin = (*iter);
						if (plugins.find(pPlugin) == plugins.end())
						{
							plugins.push_back(pPlugin);
						}
					}
					for (iter = plugins.begin(); iter != plugins.end(); iter++)
					{
						pPlugin = (*iter);
						g_RootMenu.ConsolePrint(" -> %s", pPlugin->GetFilename());
					}
				}
				srand(static_cast<int>(time(NULL)));
				pExt->unload_code = (rand() % 877) + 123;	//123 to 999
				g_RootMenu.ConsolePrint("[SM] To verify unloading %s, please use the following: ", pExt->GetFilename());
				g_RootMenu.ConsolePrint("[SM] sm exts unload %d %d", num, pExt->unload_code);

				return;
			}
		}
	}

	g_RootMenu.ConsolePrint("SourceMod Extensions Menu:");
	g_RootMenu.DrawGenericOption("info", "Extra extension information");
	g_RootMenu.DrawGenericOption("list", "List extensions");
	g_RootMenu.DrawGenericOption("unload", "Unload an extension");
}

CExtension *CExtensionManager::GetExtensionFromIdent(IdentityToken_t *ptr)
{
	if (ptr->type == g_ExtType)
	{
		return (CExtension *)(ptr->ptr);
	}

	return NULL;
}
