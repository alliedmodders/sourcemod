/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
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

#include <stdio.h>
#include <stdarg.h>
#include "PluginSys.h"
#include "ShareSys.h"
#include <ILibrarySys.h>
#include <ISourceMod.h>
#include <IHandleSys.h>
#include <IForwardSys.h>
#include <IPlayerHelpers.h>
#include "ExtensionSys.h"
#include "GameConfigs.h"
#include "common_logic.h"
#include "Translator.h"
#include "Logger.h"
#include "frame_tasks.h"
#include <am-string.h>
#include <bridge/include/IVEngineServerBridge.h>
#include <bridge/include/CoreProvider.h>

CPluginManager g_PluginSys;
HandleType_t g_PluginType = 0;
IdentityType_t g_PluginIdent = 0;

CPlugin::CPlugin(const char *file)
 : m_serial(0),
   m_status(Plugin_Uncompiled),
   m_WaitingToUnload(false),
   m_SilentFailure(false),
   m_FakeNativesMissing(false),
   m_LibraryMissing(false),
   m_pContext(nullptr),
   m_MaxClientsVar(nullptr),
   m_ident(nullptr),
   m_bGotAllLoaded(false),
   m_FileVersion(0),
   m_LastFileModTime(0),
   m_handle(BAD_HANDLE)
{
	static int MySerial = 0;

	m_serial = ++MySerial;
	m_errormsg[0] = '\0';
	ke::SafeSprintf(m_filename, sizeof(m_filename), "%s", file);

	memset(&m_info, 0, sizeof(m_info));

	m_pPhrases = g_Translator.CreatePhraseCollection();
}

CPlugin::~CPlugin()
{
	if (m_handle)
	{
		HandleSecurity sec;
		sec.pOwner = g_PluginSys.GetIdentity();
		sec.pIdentity = sec.pOwner;

		handlesys->FreeHandle(m_handle, &sec);
		g_ShareSys.DestroyIdentity(m_ident);
	}

	for (size_t i=0; i<m_configs.size(); i++)
		delete m_configs[i];
	m_configs.clear();
}

void CPlugin::InitIdentity()
{
	if (!m_handle)
	{
		m_ident = g_ShareSys.CreateIdentity(g_PluginIdent, this);
		m_handle = handlesys->CreateHandle(g_PluginType, this, g_PluginSys.GetIdentity(), g_PluginSys.GetIdentity(), NULL);
		m_pRuntime->GetDefaultContext()->SetKey(1, m_ident);
		m_pRuntime->GetDefaultContext()->SetKey(2, (IPlugin *)this);
	}
}

unsigned int CPlugin::CalcMemUsage()
{
	unsigned int base_size =
		sizeof(CPlugin)
		+ sizeof(IdentityToken_t)
		+ (m_configs.size() * (sizeof(AutoConfig *) + sizeof(AutoConfig)))
		+ m_Props.mem_usage();

	for (unsigned int i = 0; i < m_configs.size(); i++) {
		base_size += m_configs[i]->autocfg.size();
		base_size += m_configs[i]->folder.size();
	}

	for (auto i = m_Libraries.begin(); i != m_Libraries.end(); i++)
		base_size += (*i).size();

	for (auto i = m_RequiredLibs.begin(); i != m_RequiredLibs.end(); i++)
		base_size += (*i).size();

	return base_size;
}

Handle_t CPlugin::GetMyHandle()
{
	return m_handle;
}

CPlugin *CPlugin::CreatePlugin(const char *file, char *error, size_t maxlength)
{
	char fullpath[PLATFORM_MAX_PATH];
	g_pSM->BuildPath(Path_SM, fullpath, sizeof(fullpath), "plugins/%s", file);
	FILE *fp = fopen(fullpath, "rb");

	CPlugin *pPlugin = new CPlugin(file);

	if (!fp) {
		if (error)
			ke::SafeSprintf(error, maxlength, "Unable to open file");
		pPlugin->m_status = Plugin_BadLoad;
		return pPlugin;
	}

	fclose(fp);

	pPlugin->m_LastFileModTime = pPlugin->GetFileTimeStamp();
	return pPlugin;
}

bool CPlugin::GetProperty(const char *prop, void **ptr, bool remove/* =false */)
{
	StringHashMap<void *>::Result r = m_Props.find(prop);
	if (!r.found())
		return false;

	if (ptr)
		*ptr = r->value;

	if (remove)
		m_Props.remove(r);

	return true;
}

bool CPlugin::SetProperty(const char *prop, void *ptr)
{
	 return m_Props.insert(prop, ptr);
}

IPluginRuntime *CPlugin::GetRuntime()
{
	return m_pRuntime;
}

void CPlugin::SetErrorState(PluginStatus status, const char *error_fmt, ...)
{
	if (m_status == Plugin_Running)
	{
		/* Tell everyone we're now paused */
		SetPauseState(true);
		/* If we won't recover from this error, drop everything and pause dependent plugins too! */
		if (status == Plugin_Failed)
		{
			DropEverything();
		}
	}

	/* SetPauseState sets the status to Plugin_Paused, but we might want to see some other status set. */
	m_status = status;

	va_list ap;
	va_start(ap, error_fmt);
	ke::SafeVsprintf(m_errormsg, sizeof(m_errormsg), error_fmt, ap);
	va_end(ap);

	if (m_pRuntime != NULL)
	{
		m_pRuntime->SetPauseState(true);
	}
}

bool CPlugin::ReadInfo()
{
	/* Now grab the info */
	uint32_t idx;
	IPluginContext *base = GetBaseContext();
	int err = base->FindPubvarByName("myinfo", &idx);

	if (err == SP_ERROR_NONE) {
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

		auto update_field = [base](cell_t addr, ke::AString *dest) {
			const char* ptr;
			if (base->LocalToString(addr, (char **)&ptr) == SP_ERROR_NONE)
				*dest = ptr;
			else
				*dest = "";
		};

		base->GetPubvarAddrs(idx, &local_addr, (cell_t **)&cinfo);
		update_field(cinfo->name, &info_name_);
		update_field(cinfo->description, &info_description_);
		update_field(cinfo->author, &info_author_);
		update_field(cinfo->version, &info_version_);
		update_field(cinfo->url, &info_url_);
	}

	ke::SafeStrcpy(m_DateTime, sizeof(m_DateTime), "unknown");
	if ((err = base->FindPubvarByName("__version", &idx)) == SP_ERROR_NONE) {
		struct __version_info
		{
			cell_t version;
			cell_t filevers;
			cell_t date;
			cell_t time;
		};
		__version_info *info;
		cell_t local_addr;
		const char *pDate, *pTime, *pFileVers;

		pDate = "";
		pTime = "";

		base->GetPubvarAddrs(idx, &local_addr, (cell_t **)&info);
		m_FileVersion = info->version;
		if (m_FileVersion >= 4) {
			base->LocalToString(info->date, (char **)&pDate);
			base->LocalToString(info->time, (char **)&pTime);
			ke::SafeSprintf(m_DateTime, sizeof(m_DateTime), "%s %s", pDate, pTime);
		}
		if (m_FileVersion > 5) {
			base->LocalToString(info->filevers, (char **)&pFileVers);
			SetErrorState(Plugin_Failed, "Newer SourceMod required (%s or higher)", pFileVers);
			return false;
		}
	} else {
		m_FileVersion = 0;
	}

	if ((err = base->FindPubvarByName("MaxClients", &idx)) == SP_ERROR_NONE)
		base->GetPubvarByIndex(idx, &m_MaxClientsVar);
	else
		m_MaxClientsVar = nullptr;

	return true;
}

void CPlugin::SyncMaxClients(int max_clients)
{
	if (m_MaxClientsVar == NULL)
	{
		return;
	}

	*m_MaxClientsVar->offs = max_clients;
}

void CPlugin::Call_OnPluginStart()
{
	if (m_status != Plugin_Loaded)
	{
		return;
	}

	m_status = Plugin_Running;

	SyncMaxClients(playerhelpers->GetMaxClients());

	cell_t result;
	IPluginFunction *pFunction = m_pRuntime->GetFunctionByName("OnPluginStart");
	if (!pFunction)
	{
		return;
	}

	int err;
	if ((err=pFunction->Execute(&result)) != SP_ERROR_NONE)
	{
		SetErrorState(Plugin_Error, "Error detected in plugin startup (see error logs)");
	}
}

void CPlugin::Call_OnPluginEnd()
{
	if (m_status > Plugin_Paused)
	{
		return;
	}

	cell_t result;
	IPluginFunction *pFunction = m_pRuntime->GetFunctionByName("OnPluginEnd");
	if (!pFunction)
	{
		return;
	}

	pFunction->Execute(&result);
}

void CPlugin::Call_OnAllPluginsLoaded()
{
	if (m_status > Plugin_Paused)
	{
		return;
	}

	if (m_bGotAllLoaded)
	{
		return;
	}

	m_bGotAllLoaded = true;

	cell_t result;
	IPluginFunction *pFunction = m_pRuntime->GetFunctionByName("OnAllPluginsLoaded");
	if (pFunction != NULL)
	{
		pFunction->Execute(&result);
	}

	if (bridge->IsMapRunning())
	{
		if ((pFunction = m_pRuntime->GetFunctionByName("OnMapStart")) != NULL)
		{
			pFunction->Execute(NULL);
		}
	}

	if (bridge->AreConfigsExecuted())
	{
		bridge->ExecuteConfigs(GetBaseContext());
	}
}

APLRes CPlugin::AskPluginLoad()
{
	assert(m_status == Plugin_Created);
	m_status = Plugin_Loaded;

	bool haveNewAPL = true;
	IPluginFunction *pFunction = m_pRuntime->GetFunctionByName("AskPluginLoad2");
	if (!pFunction) {
		pFunction = m_pRuntime->GetFunctionByName("AskPluginLoad");
		if (!pFunction)
			return APLRes_Success;
		haveNewAPL = false;
	}

	int err;
	cell_t result;
	pFunction->PushCell(m_handle);
	pFunction->PushCell(g_PluginSys.IsLateLoadTime() ? 1 : 0);
	pFunction->PushStringEx(m_errormsg, sizeof(m_errormsg), 0, SM_PARAM_COPYBACK);
	pFunction->PushCell(sizeof(m_errormsg));
	if ((err = pFunction->Execute(&result)) != SP_ERROR_NONE) {
		SetErrorState(Plugin_Failed, "unexpected error %d in AskPluginLoad callback", err);
		return APLRes_Failure;
	}

	APLRes res = haveNewAPL
		         ? (APLRes)result
				 : (result ? APLRes_Success : APLRes_Failure);
	if (res != APLRes_Success) {
		m_status = Plugin_Failed;
		if (res == APLRes_SilentFailure)
			m_SilentFailure = true;
	}
	return res;
}

void CPlugin::Call_OnLibraryAdded(const char *lib)
{
	if (m_status > Plugin_Paused)
	{
		return;
	}

	cell_t result;
	IPluginFunction *pFunction = m_pRuntime->GetFunctionByName("OnLibraryAdded");
	if (!pFunction)
	{
		return;
	}

	pFunction->PushString(lib);
	pFunction->Execute(&result);
}

void *CPlugin::GetPluginStructure()
{
	return NULL;
}

// Only called during plugin construction.
bool CPlugin::TryCompile(char *error, size_t maxlength)
{
	char fullpath[PLATFORM_MAX_PATH];
	g_pSM->BuildPath(Path_SM, fullpath, sizeof(fullpath), "plugins/%s", m_filename);

	char loadmsg[255];
	m_pRuntime = g_pSourcePawn2->LoadBinaryFromFile(fullpath, loadmsg, sizeof(loadmsg));
	if (!m_pRuntime) {
		ke::SafeSprintf(error, maxlength, "Unable to load plugin (%s)", loadmsg);
		m_status = Plugin_BadLoad;
		return false;
	}

	// ReadInfo() sets its own error state.
	if (!ReadInfo()) {
		ke::SafeSprintf(error, maxlength, "%s", m_errormsg);
		return false;
	}

	m_status = Plugin_Created;
	return true;
}

IPluginContext *CPlugin::GetBaseContext()
{
	if (m_pRuntime == NULL)
	{
		return NULL;
	}

	return m_pRuntime->GetDefaultContext();
}

sp_context_t *CPlugin::GetContext()
{
	return NULL;
}

const char *CPlugin::GetFilename()
{
	return m_filename;
}

PluginType CPlugin::GetType()
{
	return PluginType_MapUpdated;
}

const sm_plugininfo_t *CPlugin::GetPublicInfo()
{
	if (GetStatus() >= Plugin_Created)
		return nullptr;

	m_info.author = info_author_.chars();
	m_info.description = info_description_.chars();
	m_info.name = info_name_.chars();
	m_info.url = info_url_.chars();
	m_info.version = info_version_.chars();
	return &m_info;
}

unsigned int CPlugin::GetSerial()
{
	return m_serial;
}

PluginStatus CPlugin::GetStatus()
{
	return m_status;
}

bool CPlugin::IsSilentlyFailed()
{
	return m_SilentFailure;
}

bool CPlugin::IsDebugging()
{
	if (m_pRuntime == NULL)
	{
		return false;
	}

	return true;
}

void CPlugin::LibraryActions(LibraryAction action)
{
	List<String>::iterator iter;
	for (iter = m_Libraries.begin();
		iter != m_Libraries.end();
		iter++)
	{
		g_PluginSys.OnLibraryAction((*iter).c_str(), action);
	}
}

bool CPlugin::SetPauseState(bool paused)
{
	if (paused && GetStatus() != Plugin_Running)
	{
		return false;
	}
	else if (!paused && GetStatus() != Plugin_Paused && GetStatus() != Plugin_Error) {
		return false;
	}

	if (paused)
	{
		LibraryActions(LibraryAction_Removed);
	}
	else
	{
		// Set to running again BEFORE trying to call OnPluginPauseChange ;)
		m_status = Plugin_Running;
		m_pRuntime->SetPauseState(false);
	}

	IPluginFunction *pFunction = m_pRuntime->GetFunctionByName("OnPluginPauseChange");
	if (pFunction)
	{
		cell_t result;
		pFunction->PushCell(paused ? 1 : 0);
		pFunction->Execute(&result);
	}

	if (paused)
	{
		m_status = Plugin_Paused;
		m_pRuntime->SetPauseState(true);
	}

	g_PluginSys._SetPauseState(this, paused);

	if (!paused)
	{
		LibraryActions(LibraryAction_Added);
	}

	return true;
}

IdentityToken_t *CPlugin::GetIdentity()
{
	return m_ident;
}

bool CPlugin::IsRunnable()
{
	return (m_status <= Plugin_Paused) ? true : false;
}

time_t CPlugin::GetFileTimeStamp()
{
	char path[PLATFORM_MAX_PATH];
	g_pSM->BuildPath(Path_SM, path, sizeof(path), "plugins/%s", m_filename);
#ifdef PLATFORM_WINDOWS
	struct _stat s;
	if (_stat(path, &s) != 0)
#elif defined PLATFORM_POSIX
	struct stat s;
	if (stat(path, &s) != 0)
#endif
	{
		return 0;
	}
	else
	{
		return s.st_mtime;
	}
}

IPhraseCollection *CPlugin::GetPhrases()
{
	return m_pPhrases;
}

void CPlugin::DependencyDropped(CPlugin *pOwner)
{
	if (!m_pRuntime)
		return;

	for (auto lib_iter=pOwner->m_Libraries.begin(); lib_iter!=pOwner->m_Libraries.end(); lib_iter++) {
		if (m_RequiredLibs.find(*lib_iter) != m_RequiredLibs.end()) {
			m_LibraryMissing = true;
			break;
		}
	}

	unsigned int unbound = 0;
	for (size_t i = 0; i < pOwner->m_fakes.length(); i++)
	{
		ke::Ref<Native> entry(pOwner->m_fakes[i]);

		uint32_t idx;
		if (m_pRuntime->FindNativeByName(entry->name(), &idx) != SP_ERROR_NONE)
			continue;

		m_pRuntime->UpdateNativeBinding(idx, nullptr, 0, nullptr);
		unbound++;
	}

	if (unbound)
	{
		m_FakeNativesMissing = true;
	}

	/* :IDEA: in the future, add native trapping? */
	if (m_FakeNativesMissing || m_LibraryMissing)
	{
		SetErrorState(Plugin_Error, "Depends on plugin: %s", pOwner->GetFilename());
	}
}

size_t CPlugin::GetConfigCount()
{
	return (unsigned int)m_configs.size();
}

AutoConfig *CPlugin::GetConfig(size_t i)
{
	if (i >= GetConfigCount())
	{
		return NULL;
	}

	return m_configs[i];
}

void CPlugin::AddConfig(bool autoCreate, const char *cfg, const char *folder)
{
	/* Do a check for duplicates to prevent double-execution */
	for (size_t i = 0; i < m_configs.size(); i++)
	{
		if (m_configs[i]->autocfg.compare(cfg) == 0
			&& m_configs[i]->folder.compare(folder) == 0
			&& m_configs[i]->create == autoCreate)
		{
			return;
		}
	}

	AutoConfig *c = new AutoConfig;

	c->autocfg = cfg;
	c->folder = folder;
	c->create = autoCreate;

	m_configs.push_back(c);
}

void CPlugin::DropEverything()
{
	CPlugin *pOther;
	List<WeakNative>::iterator wk_iter;

	/* Tell everyone that depends on us that we're about to drop */
	for (List<CPlugin *>::iterator iter = m_Dependents.begin();
		 iter != m_Dependents.end();
		 iter++)
	{
		pOther = static_cast<CPlugin *>(*iter);
		pOther->DependencyDropped(this);
	}

	/* Note: we don't care about things we depend on.
	 * The reason is that extensions have their own cleanup
	 * code for plugins.  Although the "right" design would be
	 * to centralize that here, i'm omitting it for now.  Thus,
	 * the code below to walk the plugins list will suffice.
	 */
	
	/* Other plugins could be holding weak references that were
	 * added by us.  We need to clean all of those up now.
	 */
	for (List<CPlugin *>::iterator iter = g_PluginSys.m_plugins.begin();
		 iter != g_PluginSys.m_plugins.end();
		 iter++)
	{
		(*iter)->ToNativeOwner()->DropRefsTo(this);
	}

	/* Proceed with the rest of the necessities. */
	CNativeOwner::DropEverything();
}

bool CPlugin::AddFakeNative(IPluginFunction *pFunc, const char *name, SPVM_FAKENATIVE_FUNC func)
{
	ke::Ref<Native> entry = g_ShareSys.AddFakeNative(pFunc, name, func);
	if (!entry)
		return false;

	m_fakes.append(entry);
	return true;
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
	m_AllPluginsLoaded = false;
	m_MyIdent = NULL;
	m_LoadingLocked = false;
	
	m_bBlockBadPlugins = true;
}

CPluginManager::~CPluginManager()
{
	CStack<CPluginManager::CPluginIterator *>::iterator iter;
	for (iter=m_iters.begin(); iter!=m_iters.end(); iter++)
	{
		delete (*iter);
	}
	m_iters.popall();
}

void CPluginManager::Shutdown()
{
	List<CPlugin *>::iterator iter;

	while ((iter = m_plugins.begin()) != m_plugins.end())
	{
		UnloadPlugin(*iter);
	}
}

void CPluginManager::LoadAll(const char *config_path, const char *plugins_path)
{
	LoadAll_FirstPass(config_path, plugins_path);
	g_Extensions.MarkAllLoaded();
	LoadAll_SecondPass();
	g_Extensions.MarkAllLoaded();
	AllPluginsLoaded();
}

void CPluginManager::LoadAll_FirstPass(const char *config, const char *basedir)
{
	/* First read in the database of plugin settings */
	m_AllPluginsLoaded = false;
	LoadPluginsFromDir(basedir, NULL);
}

void CPluginManager::LoadPluginsFromDir(const char *basedir, const char *localpath)
{
	char base_path[PLATFORM_MAX_PATH];

	/* Form the current path to start reading from */
	if (localpath == NULL)
	{
		libsys->PathFormat(base_path, sizeof(base_path), "%s", basedir);
	} else {
		libsys->PathFormat(base_path, sizeof(base_path), "%s/%s", basedir, localpath);
	}

	IDirectory *dir = libsys->OpenDirectory(base_path);

	if (!dir)
	{
		char error[256];
		libsys->GetPlatformError(error, sizeof(error));
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
			char new_local[PLATFORM_MAX_PATH];
			if (localpath == NULL)
			{
				/* If no path yet, don't add a former slash */
				ke::SafeSprintf(new_local, sizeof(new_local), "%s", dir->GetEntryName());
			} else {
				libsys->PathFormat(new_local, sizeof(new_local), "%s/%s", localpath, dir->GetEntryName());
			}
			LoadPluginsFromDir(basedir, new_local);
		} else if (dir->IsEntryFile()) {
			const char *name = dir->GetEntryName();
			size_t len = strlen(name);
			if (len >= 4
				&& strcmp(&name[len-4], ".smx") == 0)
			{
				/* If the filename matches, load the plugin */
				char plugin[PLATFORM_MAX_PATH];
				if (localpath == NULL)
				{
					ke::SafeSprintf(plugin, sizeof(plugin), "%s", name);
				} else {
					libsys->PathFormat(plugin, sizeof(plugin), "%s/%s", localpath, name);
				}
				LoadAutoPlugin(plugin);
			}
		}
		dir->NextEntry();
	}
	libsys->CloseDirectory(dir);
}

LoadRes CPluginManager::LoadPlugin(CPlugin **aResult, const char *path, bool debug, PluginType type, char error[], size_t maxlength)
{
	if (m_LoadingLocked)
		return LoadRes_NeverLoad;

	/**
	 * Does this plugin already exist?
	 */
	CPlugin *pPlugin;
	if (m_LoadLookup.retrieve(path, &pPlugin))
	{
		/* Check to see if we should try reloading it */
		if (pPlugin->GetStatus() == Plugin_BadLoad
			|| pPlugin->GetStatus() == Plugin_Error
			|| pPlugin->GetStatus() == Plugin_Failed)
		{
			UnloadPlugin(pPlugin);
		}
		else
		{
			if (aResult)
				*aResult = pPlugin;
			return LoadRes_AlreadyLoaded;
		}
	}

	CPlugin *plugin = CompileAndPrep(path, error, maxlength);

	// Assign our outparam so we can return early. It must be set.
	*aResult = plugin;

	if (plugin->GetStatus() != Plugin_Created)
		return LoadRes_Failure;

	if (plugin->AskPluginLoad() != APLRes_Success)
		return LoadRes_Failure;

	LoadExtensions(plugin);
	return LoadRes_Successful;
}

IPlugin *CPluginManager::LoadPlugin(const char *path, bool debug, PluginType type, char error[], size_t maxlength, bool *wasloaded)
{
	CPlugin *pl;
	LoadRes res;

	*wasloaded = false;
	if ((res=LoadPlugin(&pl, path, true, PluginType_MapUpdated, error, maxlength)) == LoadRes_Failure)
	{
		delete pl;
		return NULL;
	}

	if (res == LoadRes_AlreadyLoaded)
	{
		*wasloaded = true;
		return pl;
	}

	if (res == LoadRes_NeverLoad)
	{
		if (error)
		{
			if (m_LoadingLocked)
			{
				ke::SafeSprintf(error, maxlength, "There is a global plugin loading lock in effect");
			}
			else
			{
				ke::SafeSprintf(error, maxlength, "This plugin is blocked from loading (see plugin_settings.cfg)");
			}
		}
		return NULL;
	}

	AddPlugin(pl);

	/* Run second pass if we need to */
	if (IsLateLoadTime() && pl->GetStatus() == Plugin_Loaded)
	{
		if (!RunSecondPass(pl, error, maxlength))
		{
			UnloadPlugin(pl);
			return NULL;
		}
		pl->Call_OnAllPluginsLoaded();
	}

	return pl;
}

void CPluginManager::LoadAutoPlugin(const char *plugin)
{
	CPlugin *pl = NULL;
	LoadRes res;
	char error[255] = "Unknown error";

	if ((res=LoadPlugin(&pl, plugin, false, PluginType_MapUpdated, error, sizeof(error))) == LoadRes_Failure)
	{
		g_Logger.LogError("[SM] Failed to load plugin \"%s\": %s.", plugin, error);
		pl->SetErrorState(
			pl->GetStatus() <= Plugin_Created ? Plugin_BadLoad : pl->GetStatus(),
			"%s",
			error);
	}

	if (res == LoadRes_Successful || res == LoadRes_Failure)
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
	m_LoadLookup.insert(pPlugin->m_filename, pPlugin);
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
				pPlugin->SetErrorState(Plugin_BadLoad, "%s", error);
			}
		}
	}

	m_AllPluginsLoaded = true;
}

bool CPluginManager::FindOrRequirePluginDeps(CPlugin *pPlugin, char *error, size_t maxlength)
{
	struct _pl
	{
		cell_t name;
		cell_t file;
		cell_t required;
	} *pl;

	IPluginContext *pBase = pPlugin->GetBaseContext();
	uint32_t num = pBase->GetPubVarsNum();
	sp_pubvar_t *pubvar;
	char *name, *file;
	char pathfile[PLATFORM_MAX_PATH];

	for (uint32_t i=0; i<num; i++) {
		if (pBase->GetPubvarByIndex(i, &pubvar) != SP_ERROR_NONE)
			continue;
		if (strncmp(pubvar->name, "__pl_", 5) == 0) {
			pl = (_pl *)pubvar->offs;
			if (pBase->LocalToString(pl->file, &file) != SP_ERROR_NONE)
				continue;
			if (pBase->LocalToString(pl->name, &name) != SP_ERROR_NONE)
				continue;
			libsys->GetFileFromPath(pathfile, sizeof(pathfile), pPlugin->GetFilename());
			if (strcmp(pathfile, file) == 0)
				continue;
			if (pl->required == false) {
				IPluginFunction *pFunc;
				char buffer[64];
				ke::SafeSprintf(buffer, sizeof(buffer), "__pl_%s_SetNTVOptional", &pubvar->name[5]);
				if ((pFunc=pBase->GetFunctionByName(buffer))) {
					cell_t res;
					if (pFunc->Execute(&res) != SP_ERROR_NONE) {
						if (error)
							ke::SafeSprintf(error, maxlength, "Fatal error during initializing plugin load");
						return false;
					}
				}
			} else {
				/* Check that we aren't registering the same library twice */
				if (pPlugin->m_RequiredLibs.find(name) != pPlugin->m_RequiredLibs.end())
					continue;

				pPlugin->m_RequiredLibs.push_back(name);

				CPlugin *found;
				for (auto iter=m_plugins.begin(); iter!=m_plugins.end(); iter++) {
					CPlugin *pl = (*iter);
					if (pl->m_Libraries.find(name) != pl->m_Libraries.end()) {
						found = pl;
						break;
					}
				}
				if (!found) {
					if (error)
						ke::SafeSprintf(error, maxlength, "Could not find required plugin \"%s\"", name);
					return false;
				}

				found->AddDependent(pPlugin);
			}
		}
	}

	return true;
}

bool CPlugin::ForEachExtVar(const ExtVarCallback& callback)
{
	struct _ext
	{
		cell_t name;
		cell_t file;
		cell_t autoload;
		cell_t required;
	} *ext;

	IPluginContext *pBase = GetBaseContext();
	for (uint32_t i = 0; i < pBase->GetPubVarsNum(); i++)
	{
		sp_pubvar_t *pubvar;
		if (pBase->GetPubvarByIndex(i, &pubvar) != SP_ERROR_NONE)
			continue;

		if (strncmp(pubvar->name, "__ext_", 6) != 0)
			continue;

		ext = (_ext *)pubvar->offs;

		ExtVar var;
		if (pBase->LocalToString(ext->file, &var.file) != SP_ERROR_NONE)
			continue;
		if (pBase->LocalToString(ext->name, &var.name) != SP_ERROR_NONE)
			continue;
		var.autoload = !!ext->autoload;
		var.required = !!ext->required;

		if (!callback(pubvar, var))
			return false;
	}
	return true;
}

void CPluginManager::LoadExtensions(CPlugin *pPlugin)
{
	auto callback = [pPlugin] (const sp_pubvar_t *pubvar, const CPlugin::ExtVar& ext) -> bool
	{
		char path[PLATFORM_MAX_PATH];
		/* Attempt to auto-load if necessary */
		if (ext.autoload) {
			libsys->PathFormat(path, PLATFORM_MAX_PATH, "%s", ext.file);
			g_Extensions.LoadAutoExtension(path, ext.required);
		}
		return true;
	};
	pPlugin->ForEachExtVar(ke::Move(callback));
}

bool CPluginManager::RequireExtensions(CPlugin *pPlugin, char *error, size_t maxlength)
{
	auto callback = [pPlugin, error, maxlength]
                    (const sp_pubvar_t *pubvar, const CPlugin::ExtVar& ext) -> bool
	{
		/* Is this required? */
		if (ext.required) {
			char path[PLATFORM_MAX_PATH];
			libsys->PathFormat(path, PLATFORM_MAX_PATH, "%s", ext.file);
			IExtension *pExt = g_Extensions.FindExtensionByFile(path);
			if (!pExt)
				pExt = g_Extensions.FindExtensionByName(ext.name);

			if (!pExt || !pExt->IsRunning(nullptr, 0)) {
				ke::SafeSprintf(error, maxlength, "Required extension \"%s\" file(\"%s\") not running", ext.name, ext.file);
				return false;
			}
			g_Extensions.BindChildPlugin(pExt, pPlugin);
		} else {
			char buffer[64];
			ke::SafeSprintf(buffer, sizeof(buffer), "__ext_%s_SetNTVOptional", &pubvar->name[6]);

			if (IPluginFunction *pFunc = pPlugin->GetBaseContext()->GetFunctionByName(buffer)) {
				cell_t res;
				if (pFunc->Execute(&res) != SP_ERROR_NONE) {
					ke::SafeSprintf(error, maxlength, "Fatal error during plugin initialization (ext req)");
					return false;
				}
			}
		}
		return true;
	};

	return pPlugin->ForEachExtVar(ke::Move(callback));
}

CPlugin *CPluginManager::CompileAndPrep(const char *path, char *error, size_t maxlength)
{
	CPlugin *plugin = CPlugin::CreatePlugin(path, error, maxlength);
	if (plugin->GetStatus() != Plugin_Uncompiled) {
		assert(plugin->GetStatus() == Plugin_BadLoad);
		return plugin;
	}

	if (!plugin->TryCompile(error, maxlength))
		return plugin;
	assert(plugin->GetStatus() == Plugin_Created);

	if (!MalwareCheckPass(plugin, error, maxlength))
		return plugin;
	assert(plugin->GetStatus() == Plugin_Created);

	g_ShareSys.BindNativesToPlugin(plugin, true);
	plugin->InitIdentity();
	return plugin;
}


bool CPluginManager::MalwareCheckPass(CPlugin *pPlugin, char *error, size_t maxlength)
{
	unsigned char *pCodeHash = pPlugin->GetRuntime()->GetCodeHash();
	
	char codeHashBuf[40];
	ke::SafeSprintf(codeHashBuf, 40, "plugin_");
	for (int i = 0; i < 16; i++)
		ke::SafeSprintf(codeHashBuf + 7 + (i * 2), 3, "%02x", pCodeHash[i]);
	
	const char *bulletinUrl = g_pGameConf->GetKeyValue(codeHashBuf);
	if (!bulletinUrl)
		return true;

	if (m_bBlockBadPlugins) {
		if (bulletinUrl[0] != '\0') {
			ke::SafeSprintf(error, maxlength, "Known malware detected and blocked. See %s for more info", bulletinUrl);
		} else {
			ke::SafeSprintf(error, maxlength, "Possible malware or illegal plugin detected and blocked");
		}
		pPlugin->m_status = Plugin_BadLoad;
		return false;
	}

	if (bulletinUrl[0] != '\0') {
		g_Logger.LogMessage("%s: Known malware detected. See %s for more info, blocking disabled in core.cfg", pPlugin->GetFilename(), bulletinUrl);
	} else {
		g_Logger.LogMessage("%s: Possible malware or illegal plugin detected, blocking disabled in core.cfg", pPlugin->GetFilename());
	}
	return true;
}

bool CPluginManager::RunSecondPass(CPlugin *pPlugin, char *error, size_t maxlength)
{
	/* Second pass for extension requirements */
	if (!RequireExtensions(pPlugin, error, maxlength))
		return false;

	if (!FindOrRequirePluginDeps(pPlugin, error, maxlength))
		return false;

	/* Run another binding pass */
	g_ShareSys.BindNativesToPlugin(pPlugin, false);

	/* Find any unbound natives. Right now, these are not allowed. */
	IPluginContext *pContext = pPlugin->GetBaseContext();
	uint32_t num = pContext->GetNativesNum();
	for (unsigned int i=0; i<num; i++)
	{
		const sp_native_t *native = pContext->GetRuntime()->GetNative(i);
		if (!native)
			break;
		if (native->status == SP_NATIVE_UNBOUND &&
			native->name[0] != '@' &&
			!(native->flags & SP_NTVFLAG_OPTIONAL))
		{
			if (error)
			{
				ke::SafeSprintf(error, maxlength, "Native \"%s\" was not found", native->name);
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

	/* Now, if we have fake natives, go through all plugins that might need rebinding */
	if (pPlugin->GetStatus() <= Plugin_Paused && pPlugin->m_fakes.length())
	{
		List<CPlugin *>::iterator pl_iter;
		for (pl_iter = m_plugins.begin();
			 pl_iter != m_plugins.end();
			 pl_iter++)
		{
			CPlugin *pOther = (*pl_iter);
			if ((pOther->GetStatus() == Plugin_Error
				&& (pOther->m_FakeNativesMissing || pOther->m_LibraryMissing))
				|| pOther->m_FakeNativesMissing)
			{
				TryRefreshDependencies(pOther);
			}
			else if ((pOther->GetStatus() == Plugin_Running
					  || pOther->GetStatus() == Plugin_Paused)
					 && pOther != pPlugin)
			{
				g_ShareSys.BeginBindingFor(pPlugin);
				for (size_t i = 0; i < pPlugin->m_fakes.length(); i++)
					g_ShareSys.BindNativeToPlugin(pOther, pPlugin->m_fakes[i]);
			}
		}
	}

	/* Go through our libraries and tell other plugins they're added */
	List<String>::iterator s_iter;
	for (s_iter = pPlugin->m_Libraries.begin();
		s_iter != pPlugin->m_Libraries.end();
		s_iter++)
	{
		OnLibraryAction((*s_iter).c_str(), LibraryAction_Added);
	}

	/* :TODO: optimize? does this even matter? */
	pPlugin->GetPhrases()->AddPhraseFile("core.phrases");
	
	/* Go through all other already loaded plugins and tell this plugin, that their libraries are loaded */
	for (List<CPlugin *>::iterator pl_iter = m_plugins.begin(); pl_iter != m_plugins.end(); pl_iter++)
	{
		CPlugin *pl = (*pl_iter);
		/* Don't call our own libraries again and only care for already loaded plugins */
		if (pl == pPlugin || pl->GetStatus() != Plugin_Running)
			continue;

		for (s_iter=pl->m_Libraries.begin(); s_iter!=pl->m_Libraries.end(); s_iter++)
		{
			pPlugin->Call_OnLibraryAdded((*s_iter).c_str());
		}
	}

	return true;
}

void CPluginManager::TryRefreshDependencies(CPlugin *pPlugin)
{
	assert(pPlugin->GetBaseContext() != NULL);

	g_ShareSys.BindNativesToPlugin(pPlugin, false);

	for (auto req_iter=pPlugin->m_RequiredLibs.begin(); req_iter!=pPlugin->m_RequiredLibs.end(); req_iter++) {
		CPlugin *found = nullptr;
		for (auto pl_iter=m_plugins.begin(); pl_iter!=m_plugins.end(); pl_iter++) {
			CPlugin *search = (*pl_iter);
			if (search->m_Libraries.find(*req_iter) != search->m_Libraries.end()) {
				found = search;
				break;
			}
		}
		if (!found) {
			pPlugin->SetErrorState(Plugin_Error, "Library not found: %s", (*req_iter).c_str());
			return;
		}

		found->AddDependent(pPlugin);
	}

	/* Find any unbound natives
	 * Right now, these are not allowed
	 */
	IPluginContext *pContext = pPlugin->GetBaseContext();
	uint32_t num = pContext->GetNativesNum();
	for (unsigned int i=0; i<num; i++)
	{
		const sp_native_t *native = pContext->GetRuntime()->GetNative(i);
		if (!native)
			break;
		if (native->status == SP_NATIVE_UNBOUND &&
			native->name[0] != '@' &&
			!(native->flags & SP_NTVFLAG_OPTIONAL))
		{
			pPlugin->SetErrorState(Plugin_Error, "Native not found: %s", native->name);
			return;
		}
	}

	if (pPlugin->GetStatus() == Plugin_Error)
	{
		/* If we got here, all natives are okay again! */
		if (pPlugin->m_pRuntime->IsPaused())
		{
			pPlugin->SetPauseState(false);
		}
	}
}

bool CPluginManager::UnloadPlugin(IPlugin *plugin)
{
	CPlugin *pPlugin = (CPlugin *)plugin;

	// If we're already in the unload queue, just wait.
	if (pPlugin->WaitingToUnload())
		return false;

	return ScheduleUnload(pPlugin);
}

bool CPluginManager::ScheduleUnload(CPlugin *pPlugin)
{
	// Should not be recursively removing.
	assert(m_plugins.find(pPlugin) != m_plugins.end());

	IPluginContext *pContext = pPlugin->GetBaseContext();
	if (pContext && pContext->IsInExec()) {
		ke::Lambda<void()> callback = [this, pPlugin]() {
			this->ScheduleUnload(pPlugin);
		};
		pPlugin->SetWaitingToUnload();
		ScheduleTaskForNextFrame(ke::Move(callback));
		return false;
	}

	// No need to schedule an unload, we can unload immediately.
	UnloadPluginImpl(pPlugin);
	return true;
}

void CPluginManager::UnloadPluginImpl(CPlugin *pPlugin)
{
	/* Remove us from the lookup table and linked list */
	m_plugins.remove(pPlugin);
	m_LoadLookup.remove(pPlugin->m_filename);

	/* Go through our libraries and tell other plugins they're gone */
	List<String>::iterator s_iter;
	for (s_iter = pPlugin->m_Libraries.begin();
		 s_iter != pPlugin->m_Libraries.end();
		 s_iter++)
	{
		OnLibraryAction((*s_iter).c_str(), LibraryAction_Removed);
	}

	List<IPluginsListener *>::iterator iter;
	IPluginsListener *pListener;

	if (pPlugin->GetStatus() <= Plugin_Error || pPlugin->GetStatus() == Plugin_Failed)
	{
		/* Notify plugin */
		pPlugin->Call_OnPluginEnd();

		/* Notify listeners of unloading */
		for (iter=m_listeners.begin(); iter!=m_listeners.end(); iter++)
		{
			pListener = (*iter);
			pListener->OnPluginUnloaded(pPlugin);
		}
	}

	pPlugin->DropEverything();

	for (iter=m_listeners.begin(); iter!=m_listeners.end(); iter++)
	{
		/* Notify listeners of destruction */
		pListener = (*iter);
		pListener->OnPluginDestroyed(pPlugin);
	}
	
	/* Tell the plugin to delete itself */
	delete pPlugin;
}

IPlugin *CPluginManager::FindPluginByContext(const sp_context_t *ctx)
{
	IPlugin *pPlugin;
	IPluginContext *pContext;

	pContext = reinterpret_cast<IPluginContext *>(const_cast<sp_context_t *>(ctx));

	if (pContext->GetKey(2, (void **)&pPlugin))
	{
		return pPlugin;
	}

	return NULL;
}

CPlugin *CPluginManager::GetPluginByCtx(const sp_context_t *ctx)
{
	return (CPlugin *)FindPluginByContext(ctx);
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
	return (m_AllPluginsLoaded || !bridge->IsMapLoading());
}

void CPluginManager::OnSourceModAllInitialized()
{
	m_MyIdent = g_ShareSys.CreateCoreIdentity();

	HandleAccess sec;
	handlesys->InitAccessDefaults(NULL, &sec);

	sec.access[HandleAccess_Delete] = HANDLE_RESTRICT_IDENTITY;
	sec.access[HandleAccess_Clone] = HANDLE_RESTRICT_IDENTITY;

	g_PluginType = handlesys->CreateType("Plugin", this, 0, NULL, &sec, m_MyIdent, NULL);
	g_PluginIdent = g_ShareSys.CreateIdentType("PLUGIN");

	rootmenu->AddRootConsoleCommand3("plugins", "Manage Plugins", this);

	g_ShareSys.AddInterface(NULL, GetOldAPI());
	
	m_pOnLibraryAdded = forwardsys->CreateForward("OnLibraryAdded", ET_Ignore, 1, NULL, Param_String);
	m_pOnLibraryRemoved = forwardsys->CreateForward("OnLibraryRemoved", ET_Ignore, 1, NULL, Param_String);
}

void CPluginManager::OnSourceModShutdown()
{
	rootmenu->RemoveRootConsoleCommand("plugins", this);

	UnloadAll();

	handlesys->RemoveType(g_PluginType, m_MyIdent);
	g_ShareSys.DestroyIdentType(g_PluginIdent);
	g_ShareSys.DestroyIdentity(m_MyIdent);
	
	forwardsys->ReleaseForward(m_pOnLibraryAdded);
	forwardsys->ReleaseForward(m_pOnLibraryRemoved);
}

ConfigResult CPluginManager::OnSourceModConfigChanged(const char *key,
												 const char *value,
												 ConfigSource source,
												 char *error,
												 size_t maxlength)
{
	if (strcmp(key, "BlockBadPlugins") == 0) {
		if (strcasecmp(value, "yes") == 0)
		{
			m_bBlockBadPlugins = true;
		} else if (strcasecmp(value, "no") == 0) {
			m_bBlockBadPlugins = false;
		} else {
			ke::SafeSprintf(error, maxlength, "Invalid value: must be \"yes\" or \"no\"");
			return ConfigResult_Reject;
		}
		return ConfigResult_Accept;
	}
	return ConfigResult_Ignore;
}

void CPluginManager::OnHandleDestroy(HandleType_t type, void *object)
{
	/* We don't care about the internal object, actually */
}

bool CPluginManager::GetHandleApproxSize(HandleType_t type, void *object, unsigned int *pSize)
{
	*pSize = ((CPlugin *)object)->CalcMemUsage();
	return true;
}

IPlugin *CPluginManager::PluginFromHandle(Handle_t handle, HandleError *err)
{
	IPlugin *pPlugin;
	HandleError _err;

	HandleSecurity sec;

	sec.pOwner = NULL;
	sec.pIdentity = m_MyIdent;

	if ((_err=handlesys->ReadHandle(handle, g_PluginType, &sec, (void **)&pPlugin)) != HandleError_None)
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

	SourceHook::List<CPlugin *>::iterator iter;
	for (iter = m_plugins.begin(); iter != m_plugins.end() && id < num; iter++, id++) {}

	pl = *iter;

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

static inline bool IS_STR_FILLED(const char *text)
{
	return text[0] != '\0';
}

void CPluginManager::OnRootConsoleCommand(const char *cmdname, const ICommandArgs *command)
{
	int argcount = command->ArgC();
	if (argcount >= 3)
	{
		const char *cmd = command->Arg(2);
		if (strcmp(cmd, "list") == 0)
		{
			char buffer[256];
			unsigned int id = 1;
			int plnum = GetPluginCount();

			if (!plnum)
			{
				rootmenu->ConsolePrint("[SM] No plugins loaded");
				return;
			}
			else
			{
				rootmenu->ConsolePrint("[SM] Listing %d plugin%s:", GetPluginCount(), (plnum > 1) ? "s" : "");
			}

			CPlugin *pl;
			SourceHook::List<CPlugin *>::iterator iter;
			SourceHook::List<CPlugin *> m_FailList;

			for (iter = m_plugins.begin(); iter != m_plugins.end(); iter++, id++)
			{
				pl = (*iter);
				assert(pl->GetStatus() != Plugin_Created);
				int len = 0;
				const sm_plugininfo_t *info = pl->GetPublicInfo();
				if (pl->GetStatus() != Plugin_Running && !pl->IsSilentlyFailed())
				{
					len += ke::SafeSprintf(buffer, sizeof(buffer), "  %02d <%s>", id, GetStatusText(pl->GetStatus()));

					if (pl->GetStatus() <= Plugin_Error)
					{
						/* Plugin has failed to load. */
						m_FailList.push_back(pl);
					}
				}
				else
				{
					len += ke::SafeSprintf(buffer, sizeof(buffer), "  %02d", id);
				}
				if (pl->GetStatus() < Plugin_Created)
				{
					if (pl->IsSilentlyFailed())
						len += ke::SafeSprintf(&buffer[len], sizeof(buffer)-len, " Disabled:");
					len += ke::SafeSprintf(&buffer[len], sizeof(buffer)-len, " \"%s\"", (IS_STR_FILLED(info->name)) ? info->name : pl->GetFilename());
					if (IS_STR_FILLED(info->version))
					{
						len += ke::SafeSprintf(&buffer[len], sizeof(buffer)-len, " (%s)", info->version);
					}
					if (IS_STR_FILLED(info->author))
					{
						ke::SafeSprintf(&buffer[len], sizeof(buffer)-len, " by %s", info->author);
					}
				}
				else
				{
					ke::SafeSprintf(&buffer[len], sizeof(buffer)-len, " %s", pl->m_filename);
				}
				rootmenu->ConsolePrint("%s", buffer);
			}

			if (!m_FailList.empty())
			{
				rootmenu->ConsolePrint("Load Errors:");

				SourceHook::List<CPlugin *>::iterator _iter;

				CPlugin *pl;

				for (_iter=m_FailList.begin(); _iter!=m_FailList.end(); _iter++)
				{
					pl = (CPlugin *)*_iter;
					rootmenu->ConsolePrint("%s: %s",(IS_STR_FILLED(pl->GetPublicInfo()->name)) ? pl->GetPublicInfo()->name : pl->GetFilename(), pl->m_errormsg);
				}
			}

			return;
		}
		else if (strcmp(cmd, "load") == 0)
		{
			if (argcount < 4)
			{
				rootmenu->ConsolePrint("[SM] Usage: sm plugins load <file>");
				return;
			}

			char error[128];
			bool wasloaded;
			const char *filename = command->Arg(3);

			char pluginfile[256];
			const char *ext = libsys->GetFileExtension(filename) ? "" : ".smx";
			g_pSM->BuildPath(Path_None, pluginfile, sizeof(pluginfile), "%s%s", filename, ext);

			IPlugin *pl = LoadPlugin(pluginfile, false, PluginType_MapUpdated, error, sizeof(error), &wasloaded);

			if (wasloaded)
			{
				rootmenu->ConsolePrint("[SM] Plugin %s is already loaded.", pluginfile);
				return;
			}

			if (pl)
			{
				rootmenu->ConsolePrint("[SM] Loaded plugin %s successfully.", pluginfile);
			}
			else
			{
				rootmenu->ConsolePrint("[SM] Plugin %s failed to load: %s.", pluginfile, error);
			}

			return;
		}
		else if (strcmp(cmd, "unload") == 0)
		{
			if (argcount < 4)
			{
				rootmenu->ConsolePrint("[SM] Usage: sm plugins unload <#|file>");
				return;
			}

			CPlugin *pl;
			char *end;
			const char *arg = command->Arg(3);
			int id = strtol(arg, &end, 10);

			if (*end == '\0')
			{
				pl = GetPluginByOrder(id);
				if (!pl)
				{
					rootmenu->ConsolePrint("[SM] Plugin index %d not found.", id);
					return;
				}
			}
			else
			{
				char pluginfile[256];
				const char *ext = libsys->GetFileExtension(arg) ? "" : ".smx";
				g_pSM->BuildPath(Path_None, pluginfile, sizeof(pluginfile), "%s%s", arg, ext);

				if (!m_LoadLookup.retrieve(pluginfile, &pl))
				{
					rootmenu->ConsolePrint("[SM] Plugin %s is not loaded.", pluginfile);
					return;
				}
			}

			char name[PLATFORM_MAX_PATH];

			if (pl->GetStatus() < Plugin_Created)
			{
				const sm_plugininfo_t *info = pl->GetPublicInfo();
				ke::SafeSprintf(name, sizeof(name), (IS_STR_FILLED(info->name)) ? info->name : pl->GetFilename());
			}
			else
			{
				ke::SafeSprintf(name, sizeof(name), "%s", pl->GetFilename());
			}

			if (UnloadPlugin(pl))
			{
				rootmenu->ConsolePrint("[SM] Plugin %s unloaded successfully.", name);
			}
			else
			{
				rootmenu->ConsolePrint("[SM] Failed to unload plugin %s.", name);
			}

			return;
		}
		else if (strcmp(cmd, "unload_all") == 0)
		{
			g_PluginSys.UnloadAll();
			rootmenu->ConsolePrint("[SM] All plugins have been unloaded.");
			return;
		}
		else if (strcmp(cmd, "load_lock") == 0)
		{
			if (m_LoadingLocked)
			{
				rootmenu->ConsolePrint("[SM] There is already a loading lock in effect.");
			}
			else
			{
				m_LoadingLocked = true;
				rootmenu->ConsolePrint("[SM] Loading is now locked; no plugins will be loaded or re-loaded.");
			}
			return;
		}
		else if (strcmp(cmd, "load_unlock") == 0)
		{
			if (m_LoadingLocked)
			{
				m_LoadingLocked = false;
				rootmenu->ConsolePrint("[SM] The loading lock is no longer in effect.");
			}
			else
			{
				rootmenu->ConsolePrint("[SM] There was no loading lock in effect.");
			}
			return;
		}
		else if (strcmp(cmd, "info") == 0)
		{
			if (argcount < 4)
			{
				rootmenu->ConsolePrint("[SM] Usage: sm plugins info <#>");
				return;
			}

			CPlugin *pl;
			char *end;
			const char *arg = command->Arg(3);
			int id = strtol(arg, &end, 10);

			if (*end == '\0')
			{
				pl = GetPluginByOrder(id);
				if (!pl)
				{
					rootmenu->ConsolePrint("[SM] Plugin index %d not found.", id);
					return;
				}
			}
			else
			{
				char pluginfile[256];
				const char *ext = libsys->GetFileExtension(arg) ? "" : ".smx";
				g_pSM->BuildPath(Path_None, pluginfile, sizeof(pluginfile), "%s%s", arg, ext);

				if (!m_LoadLookup.retrieve(pluginfile, &pl))
				{
					rootmenu->ConsolePrint("[SM] Plugin %s is not loaded.", pluginfile);
					return;
				}
			}

			const sm_plugininfo_t *info = pl->GetPublicInfo();

			rootmenu->ConsolePrint("  Filename: %s", pl->GetFilename());
			if (pl->GetStatus() <= Plugin_Error || pl->GetStatus() == Plugin_Failed)
			{
				if (IS_STR_FILLED(info->name))
				{
					if (IS_STR_FILLED(info->description))
					{
						rootmenu->ConsolePrint("  Title: %s (%s)", info->name, info->description);
					} else {
						rootmenu->ConsolePrint("  Title: %s", info->name);
					}
				}
				if (IS_STR_FILLED(info->author))
				{
					rootmenu->ConsolePrint("  Author: %s", info->author);
				}
				if (IS_STR_FILLED(info->version))
				{
					rootmenu->ConsolePrint("  Version: %s", info->version);
				}
				if (IS_STR_FILLED(info->url))
				{
					rootmenu->ConsolePrint("  URL: %s", info->url);
				}
				if (pl->GetStatus() == Plugin_Error || pl->GetStatus() == Plugin_Failed)
				{
					rootmenu->ConsolePrint("  Error: %s", pl->m_errormsg);
				}
				else
				{
					if (pl->GetStatus() == Plugin_Running)
					{
						rootmenu->ConsolePrint("  Status: running");
					}
					else
					{
						rootmenu->ConsolePrint("  Status: not running");
					}
				}
				if (pl->m_FileVersion >= 3)
				{
					rootmenu->ConsolePrint("  Timestamp: %s", pl->m_DateTime);
				}
				
				unsigned char *pCodeHash = pl->m_pRuntime->GetCodeHash();
				unsigned char *pDataHash = pl->m_pRuntime->GetDataHash();
				
				char combinedHash[33];
				for (int i = 0; i < 16; i++)
					ke::SafeSprintf(combinedHash + (i * 2), 3, "%02x", pCodeHash[i] ^ pDataHash[i]);
				
				rootmenu->ConsolePrint("  Hash: %s", combinedHash);
			}
			else
			{
				rootmenu->ConsolePrint("  Load error: %s", pl->m_errormsg);
				if (pl->GetStatus() < Plugin_Created)
				{
					rootmenu->ConsolePrint("  File info: (title \"%s\") (version \"%s\")",
											info->name ? info->name : "<none>",
											info->version ? info->version : "<none>");
					if (IS_STR_FILLED(info->url))
					{
						rootmenu->ConsolePrint("  File URL: %s", info->url);
					}
				}
			}

			return;
		}
		else if (strcmp(cmd, "refresh") == 0)
		{
			RefreshAll();
			bridge->DoGlobalPluginLoads();
			rootmenu->ConsolePrint("[SM] The plugin list has been refreshed and reloaded.");
			return;
		}
		else if (strcmp(cmd, "reload") == 0)
		{
			if (argcount < 4)
			{
				rootmenu->ConsolePrint("[SM] Usage: sm plugins reload <#|file>");
				return;
			}

			CPlugin *pl;
			char *end;
			const char *arg = command->Arg(3);
			int id = strtol(arg, &end, 10);

			if (*end == '\0')
			{
				pl = GetPluginByOrder(id);
				if (!pl)
				{
					rootmenu->ConsolePrint("[SM] Plugin index %d not found.", id);
					return;
				}
			}
			else
			{
				char pluginfile[256];
				const char *ext = libsys->GetFileExtension(arg) ? "" : ".smx";
				g_pSM->BuildPath(Path_None, pluginfile, sizeof(pluginfile), "%s%s", arg, ext);

				if (!m_LoadLookup.retrieve(pluginfile, &pl))
				{
					rootmenu->ConsolePrint("[SM] Plugin %s is not loaded.", pluginfile);
					return;
				}
			}

			char name[PLATFORM_MAX_PATH];
			const sm_plugininfo_t *info = pl->GetPublicInfo();
			if (pl->GetStatus() <= Plugin_Paused)
				strcpy(name, (IS_STR_FILLED(info->name)) ? info->name : pl->GetFilename());
			else
				strcpy(name, pl->GetFilename());

			if (ReloadPlugin(pl))
			{
				rootmenu->ConsolePrint("[SM] Plugin %s reloaded successfully.", name);
			}
			else
			{
				rootmenu->ConsolePrint("[SM] Failed to reload plugin %s.", name);
			}

			return;
		}
	}

	/* Draw the main menu */
	rootmenu->ConsolePrint("SourceMod Plugins Menu:");
	rootmenu->DrawGenericOption("info", "Information about a plugin");
	rootmenu->DrawGenericOption("list", "Show loaded plugins");
	rootmenu->DrawGenericOption("load", "Load a plugin");
	rootmenu->DrawGenericOption("load_lock", "Prevents any more plugins from being loaded");
	rootmenu->DrawGenericOption("load_unlock", "Re-enables plugin loading");
	rootmenu->DrawGenericOption("refresh", "Reloads/refreshes all plugins in the plugins folder");
	rootmenu->DrawGenericOption("reload", "Reloads a plugin");
	rootmenu->DrawGenericOption("unload", "Unload a plugin");
	rootmenu->DrawGenericOption("unload_all", "Unloads all plugins");
}

bool CPluginManager::ReloadPlugin(CPlugin *pl)
{
	List<CPlugin *>::iterator iter;
	char filename[PLATFORM_MAX_PATH];
	bool wasloaded;
	PluginType ptype;
	IPlugin *newpl;
	int id = 1;

	strcpy(filename, pl->m_filename);
	ptype = pl->GetType();

	for (iter=m_plugins.begin(); iter!=m_plugins.end(); iter++, id++)
	{
		if ((*iter) == pl)
		{
			break;
		}
	}

	if (!UnloadPlugin(pl))
	{
		return false;
	}
	if (!(newpl=LoadPlugin(filename, true, ptype, NULL, 0, &wasloaded)))
	{
		return false;
	}

	for (iter=m_plugins.begin(); iter!=m_plugins.end(); iter++)
	{
		if ((*iter) == (CPlugin *)newpl)
		{
			m_plugins.erase(iter);
			break;
		}
	}

	int i;
	for (i=1, iter=m_plugins.begin(); iter!=m_plugins.end() && i<id; iter++, i++)
	{
	}
	m_plugins.insert(iter, (CPlugin *)newpl);

	return true;
}

void CPluginManager::RefreshAll()
{
	/* If we're in a load lock, just skip this whole bit. */
	if (m_LoadingLocked)
	{
		return;
	}

	List<CPlugin *> tmp_list = m_plugins;
	for (auto iter = tmp_list.begin(); iter != tmp_list.end(); iter++)
	{
		CPlugin *pl = (*iter);
		if (pl->HasUpdatedFile())
			UnloadPlugin(pl);
	}
}

bool CPlugin::HasUpdatedFile()
{
	time_t t = GetFileTimeStamp();
	if (!t || t > m_LastFileModTime) {
		m_LastFileModTime = t;
		return true;
	}
	return false;
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

void CPluginManager::AddFunctionsToForward(const char *name, IChangeableForward *pForward)
{
	List<CPlugin *>::iterator iter;
	CPlugin *pPlugin;
	IPluginFunction *pFunction;

	for (iter = m_plugins.begin(); iter != m_plugins.end(); iter++)
	{
		pPlugin = (*iter);

		if (pPlugin->GetStatus() <= Plugin_Paused)
		{
			pFunction = pPlugin->GetBaseContext()->GetFunctionByName(name);

			if (pFunction)
			{
				pForward->AddFunction(pFunction);
			}
		}
	}
}

CPlugin *CPluginManager::GetPluginFromIdentity(IdentityToken_t *pToken)
{
	if (pToken->type != g_PluginIdent)
	{
		return NULL;
	}

	return (CPlugin *)(pToken->ptr);
}

void CPluginManager::OnLibraryAction(const char *lib, LibraryAction action)
{
	switch (action)
	{
	case LibraryAction_Removed:
		m_pOnLibraryRemoved->PushString(lib);
		m_pOnLibraryRemoved->Execute(NULL);
		break;
	case LibraryAction_Added:
		m_pOnLibraryAdded->PushString(lib);
		m_pOnLibraryAdded->Execute(NULL);
		break;
	}
}

bool CPluginManager::LibraryExists(const char *lib)
{
	List<CPlugin *>::iterator iter;

	for (iter=m_plugins.begin();
		 iter!=m_plugins.end();
		 iter++)
	{
		CPlugin *pl = (*iter);
		if (pl->GetStatus() != Plugin_Running)
		{
			continue;
		}
		List<String>::iterator s_iter;
		for (s_iter = pl->m_Libraries.begin();
			 s_iter != pl->m_Libraries.end();
			 s_iter++)
		{
			if ((*s_iter).compare(lib) == 0)
			{
				return true;
			}
		}
	}

	return false;
}

void CPluginManager::AllPluginsLoaded()
{
	List<CPlugin *>::iterator iter;
	CPlugin *pl;

	for (iter=m_plugins.begin(); iter!=m_plugins.end(); iter++)
	{
		pl = (*iter);
		pl->Call_OnAllPluginsLoaded();
	}
}

void CPluginManager::UnloadAll()
{
	List<CPlugin *>::iterator iter;
	while ( (iter = m_plugins.begin()) != m_plugins.end() )
	{
		UnloadPlugin((*iter));
	}
}

int CPluginManager::GetOrderOfPlugin(IPlugin *pl)
{
	int id = 1;
	List<CPlugin *>::iterator iter;

	for (iter = m_plugins.begin(); iter != m_plugins.end(); iter++, id++)
	{
		if ((*iter) == pl)
		{
			return id;
		}
	}

	return -1;
}

SMPlugin *CPluginManager::FindPluginByConsoleArg(const char *arg)
{
	int id;
	char *end;
	CPlugin *pl;
	
	id = strtol(arg, &end, 10);

	if (*end == '\0')
	{
		pl = GetPluginByOrder(id);
		if (!pl)
		{
			return NULL;
		}
	}
	else
	{
		char pluginfile[256];
		const char *ext = libsys->GetFileExtension(arg) ? "" : ".smx";
		ke::SafeSprintf(pluginfile, sizeof(pluginfile), "%s%s", arg, ext);

		if (!m_LoadLookup.retrieve(pluginfile, &pl))
			return NULL;
	}

	return pl;
}

void CPluginManager::OnSourceModMaxPlayersChanged(int newvalue)
{
	SyncMaxClients(newvalue);
}

void CPluginManager::SyncMaxClients(int max_clients)
{
	List<CPlugin *>::iterator iter;

	for (iter = m_plugins.begin(); iter != m_plugins.end(); iter++)
	{
		(*iter)->SyncMaxClients(max_clients);
	}
}

const CVector<SMPlugin *> *CPluginManager::ListPlugins()
{
	CVector<SMPlugin *> *list = new CVector<SMPlugin *>();

	for (List<CPlugin *>::iterator iter = m_plugins.begin(); iter != m_plugins.end(); iter++)
		list->push_back((*iter));

	return list;
}

void CPluginManager::FreePluginList(const CVector<SMPlugin *> *list)
{
	delete const_cast<CVector<SMPlugin *> *>(list);
}

class OldPluginAPI : public IPluginManager
{
public:
	IPlugin *LoadPlugin(const char *path,
						bool debug,
						PluginType type,
						char error[],
						size_t maxlength,
						bool *wasloaded)
	{
		return g_PluginSys.LoadPlugin(path, debug, type, error, maxlength, wasloaded);
	}

	bool UnloadPlugin(IPlugin *plugin)
	{
		return g_PluginSys.UnloadPlugin(plugin);
	}

	IPlugin *FindPluginByContext(const sp_context_t *ctx)
	{
		return g_PluginSys.FindPluginByContext(ctx);
	}

	unsigned int GetPluginCount()
	{
		return g_PluginSys.GetPluginCount();
	}

	IPluginIterator *GetPluginIterator()
	{
		return g_PluginSys.GetPluginIterator();
	}

	void AddPluginsListener(IPluginsListener *listener)
	{
		g_PluginSys.AddPluginsListener(listener);
	}

	void RemovePluginsListener(IPluginsListener *listener)
	{
		g_PluginSys.RemovePluginsListener(listener);
	}

	IPlugin *PluginFromHandle(Handle_t handle, HandleError *err)
	{
		return g_PluginSys.PluginFromHandle(handle, err);
	}
};

static OldPluginAPI sOldPluginAPI;

IPluginManager *CPluginManager::GetOldAPI()
{
	return &sOldPluginAPI;
}
