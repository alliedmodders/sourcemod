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
#include <amtl/am-string.h>
#include <bridge/include/IVEngineServerBridge.h>
#include <bridge/include/CoreProvider.h>

CPluginManager g_PluginSys;
HandleType_t g_PluginType = 0;
IdentityType_t g_PluginIdent = 0;

CPlugin::CPlugin(const char *file)
 : m_serial(0),
   m_status(Plugin_Uncompiled),
   m_state(PluginState::Unregistered),
   m_AddedLibraries(false),
   m_EnteredSecondPass(false),
   m_SilentFailure(false),
   m_FakeNativesMissing(false),
   m_LibraryMissing(false),
   m_pContext(nullptr),
   m_MaxClientsVar(nullptr),
   m_bGotAllLoaded(false),
   m_FileVersion(0),
   m_ident(nullptr),
   m_LastFileModTime(0),
   m_handle(BAD_HANDLE)
{
	static int MySerial = 0;

	m_serial = ++MySerial;
	m_errormsg[0] = '\0';
	m_DateTime[0] = '\0';
	ke::SafeStrcpy(m_filename, sizeof(m_filename), file);

	memset(&m_info, 0, sizeof(m_info));

	m_pPhrases.reset(g_Translator.CreatePhraseCollection());
}

CPlugin::~CPlugin()
{
	DestroyIdentity();
	for (size_t i=0; i<m_configs.size(); i++)
		delete m_configs[i];
	m_configs.clear();
}

void CPlugin::InitIdentity()
{
	if (m_handle)
		return;

	m_ident = g_ShareSys.CreateIdentity(g_PluginIdent, this);
	m_handle = handlesys->CreateHandle(g_PluginType, this, g_PluginSys.GetIdentity(), g_PluginSys.GetIdentity(), NULL);
	m_pRuntime->GetDefaultContext()->SetKey(1, m_ident);
	m_pRuntime->GetDefaultContext()->SetKey(2, (IPlugin *)this);
}

void CPlugin::DestroyIdentity()
{
	if (m_handle) {
		HandleSecurity sec(g_PluginSys.GetIdentity(), g_PluginSys.GetIdentity());
		handlesys->FreeHandle(m_handle, &sec);
		m_handle = BAD_HANDLE;
	}
	if (m_ident) {
		g_ShareSys.DestroyIdentity(m_ident);
		m_ident = nullptr;
	}
}

bool CPlugin::IsEvictionCandidate() const
{
	switch (Status()) {
		case Plugin_Running:
		case Plugin_Loaded:
			// These states are valid, we should never evict.
			return false;
		case Plugin_BadLoad:
		case Plugin_Uncompiled:
			// These states imply that the plugin never loaded to begin with,
			// so we have nothing to evict.
			return false;
		case Plugin_Evicted:
			// We cannot be evicted twice.
			return false;
		default:
			return true;
	}
}

void CPlugin::FinishEviction()
{
	assert(IsEvictionCandidate());

	// Revoke our identity and handle. This could maybe be seen as bad faith,
	// since other plugins could be holding the handle and will now error. But
	// this was already an existing problem. We need a listener API to solve
	// it.
	DestroyIdentity();

	// Note that we do not set our status to Plugin_Evicted. This is a pseudo-status
	// so consumers won't attempt to read the context or runtime. The real state
	// is reflected here.
	m_state = PluginState::Evicted;
	m_pRuntime = nullptr;
	m_pPhrases = nullptr;
	m_pContext = nullptr;
	m_MaxClientsVar = nullptr;
	m_Props.clear();
	m_configs.clear();
	m_Libraries.clear();
	m_bGotAllLoaded = false;
	m_FileVersion = 0;
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

CPlugin *CPlugin::Create(const char *file)
{
	char fullpath[PLATFORM_MAX_PATH];
	g_pSM->BuildPath(Path_SM, fullpath, sizeof(fullpath), "plugins/%s", file);
	FILE *fp = fopen(fullpath, "rb");

	CPlugin *pPlugin = new CPlugin(file);

	if (!fp) {
		pPlugin->EvictWithError(Plugin_BadLoad, "Unable to open file");
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
	return m_pRuntime.get();
}

void CPlugin::EvictWithError(PluginStatus status, const char *error_fmt, ...)
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

	if (m_pRuntime)
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

		auto update_field = [base](cell_t addr, std::string *dest) {
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
			EvictWithError(Plugin_Failed, "Newer SourceMod required (%s or higher)", pFileVers);
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

bool CPlugin::OnPluginStart()
{
	m_EnteredSecondPass = true;

	if (m_status != Plugin_Loaded)
		return false;
	m_status = Plugin_Running;

	SyncMaxClients(playerhelpers->GetMaxClients());

	cell_t result;
	IPluginFunction *pFunction = m_pRuntime->GetFunctionByName("OnPluginStart");
	if (!pFunction)
		return true;

	int err;
	if ((err=pFunction->Execute(&result)) != SP_ERROR_NONE) {
		EvictWithError(Plugin_Error, "Error detected in plugin startup (see error logs)");
		return false;
	}
	return true;
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
		EvictWithError(Plugin_Failed, "unexpected error %d in AskPluginLoad callback", err);
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
bool CPlugin::TryCompile()
{
	char fullpath[PLATFORM_MAX_PATH];
	g_pSM->BuildPath(Path_SM, fullpath, sizeof(fullpath), "plugins/%s", m_filename);

	char loadmsg[255];
	m_pRuntime.reset(g_pSourcePawn2->LoadBinaryFromFile(fullpath, loadmsg, sizeof(loadmsg)));
	if (!m_pRuntime) {
		EvictWithError(Plugin_BadLoad, "Unable to load plugin (%s)", loadmsg);
		return false;
	}

	// ReadInfo() sets its own error state.
	if (!ReadInfo())
		return false;

	m_status = Plugin_Created;
	return true;
}

IPluginContext *CPlugin::GetBaseContext()
{
	if (!m_pRuntime)
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
	m_info.author = info_author_.c_str();
	m_info.description = info_description_.c_str();
	m_info.name = info_name_.c_str();
	m_info.url = info_url_.c_str();
	m_info.version = info_version_.c_str();
	return &m_info;
}

unsigned int CPlugin::GetSerial()
{
	return m_serial;
}

PluginStatus CPlugin::GetStatus()
{
	return Status();
}

PluginStatus CPlugin::Status() const
{
	// Even though we're evicted, we previously guaranteed a valid runtime
	// for error/fail states. A new failure case above BadLoad solves this.
	if (m_state == PluginState::Evicted)
		return Plugin_Evicted;
	return m_status;
}

bool CPlugin::IsSilentlyFailed()
{
	return m_SilentFailure;
}

bool CPlugin::IsDebugging()
{
	if (!m_pRuntime)
	{
		return false;
	}

	return true;
}

void CPlugin::LibraryActions(LibraryAction action)
{
	if (action == LibraryAction_Removed) {
		if (!m_AddedLibraries)
			return;
		m_AddedLibraries = false;
	}

	for (auto iter = m_Libraries.begin(); iter != m_Libraries.end(); iter++)
		g_PluginSys.OnLibraryAction((*iter).c_str(), action);

	if (action == LibraryAction_Added)
		m_AddedLibraries = true;
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
	return m_pPhrases.get();
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
	for (size_t i = 0; i < pOwner->m_fakes.size(); i++)
	{
		ke::RefPtr<Native> entry(pOwner->m_fakes[i]);

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
		EvictWithError(Plugin_Error, "Depends on plugin: %s", pOwner->GetFilename());
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
	g_PluginSys.ForEachPlugin([this] (CPlugin *other) -> void {
		other->ToNativeOwner()->DropRefsTo(this);
	});

	/* Proceed with the rest of the necessities. */
	CNativeOwner::DropEverything();
}

bool CPlugin::AddFakeNative(IPluginFunction *pFunc, const char *name, SPVM_FAKENATIVE_FUNC func)
{
	ke::RefPtr<Native> entry = g_ShareSys.AddFakeNative(pFunc, name, func);
	if (!entry)
		return false;

	m_fakes.push_back(entry);
	return true;
}

void CPlugin::BindFakeNativesTo(CPlugin *other)
{
	for (size_t i = 0; i < m_fakes.size(); i++)
		g_ShareSys.BindNativeToPlugin(other, m_fakes[i]);
}

/*******************
 * PLUGIN ITERATOR *
 *******************/

CPluginManager::CPluginIterator::CPluginIterator(ReentrantList<CPlugin *>& in)
{
	for (PluginIter iter(in); !iter.done(); iter.next())
		mylist.push_back(*iter);
	current = mylist.begin();
	g_PluginSys.AddPluginsListener(this);
}

IPlugin *CPluginManager::CPluginIterator::GetPlugin()
{
	return (*current);
}

bool CPluginManager::CPluginIterator::MorePlugins()
{
	return (current != mylist.end());
}

void CPluginManager::CPluginIterator::NextPlugin()
{
	current++;
}

void CPluginManager::CPluginIterator::Release()
{
	delete this;
}

CPluginManager::CPluginIterator::~CPluginIterator()
{
	g_PluginSys.RemovePluginsListener(this);
}

void CPluginManager::CPluginIterator::OnPluginDestroyed(IPlugin *plugin)
{
	if (*current == plugin)
		current = mylist.erase(current);
	else
		mylist.remove(static_cast<CPlugin *>(plugin));
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
}

void CPluginManager::Shutdown()
{
	UnloadAll();
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
				ke::SafeStrcpy(new_local, sizeof(new_local), dir->GetEntryName());
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
					ke::SafeStrcpy(plugin, sizeof(plugin), name);
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

LoadRes CPluginManager::LoadPlugin(CPlugin **aResult, const char *path, bool debug, PluginType type)
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

	CPlugin *plugin = CompileAndPrep(path);

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

	if (strstr(path, "..") != NULL)
	{
		ke::SafeStrcpy(error, maxlength, "Cannot load plugins outside the \"plugins\" folder");
		return NULL;
	}

	const char *ext = libsys->GetFileExtension(path);
	if (!ext || strcmp(ext, "smx") != 0)
	{
		ke::SafeStrcpy(error, maxlength, "Plugin files must have the \".smx\" file extension");
		return NULL;
	}

	if ((res=LoadPlugin(&pl, path, true, PluginType_MapUpdated)) == LoadRes_Failure)
	{
		ke::SafeStrcpy(error, maxlength, pl->GetErrorMsg());
		delete pl;
		return NULL;
	}

	if (res == LoadRes_AlreadyLoaded)
	{
		*wasloaded = true;
		return pl;
	}

	if (res == LoadRes_NeverLoad) {
		if (m_LoadingLocked)
			ke::SafeStrcpy(error, maxlength, "There is a global plugin loading lock in effect");
		else
			ke::SafeStrcpy(error, maxlength, "This plugin is blocked from loading (see plugin_settings.cfg)");
		return NULL;
	}

	AddPlugin(pl);

	/* Run second pass if we need to */
	if (IsLateLoadTime() && pl->GetStatus() == Plugin_Loaded)
	{
		if (!RunSecondPass(pl)) {
			ke::SafeStrcpy(error, maxlength, pl->GetErrorMsg());
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
	if ((res=LoadPlugin(&pl, plugin, false, PluginType_MapUpdated)) == LoadRes_Failure)
	{
		g_Logger.LogError("[SM] Failed to load plugin \"%s\": %s.", plugin, pl->GetErrorMsg());
	}

	if (res == LoadRes_Successful || res == LoadRes_Failure)
	{
		AddPlugin(pl);
	}
}

void CPluginManager::AddPlugin(CPlugin *pPlugin)
{
	m_plugins.push_back(pPlugin);
	m_LoadLookup.insert(pPlugin->GetFilename(), pPlugin);

	pPlugin->SetRegistered();

	for (ListenerIter iter(m_listeners); !iter.done(); iter.next())
		(*iter)->OnPluginCreated(pPlugin);

	if (pPlugin->IsEvictionCandidate()) {
		// If we get here, and the plugin isn't running, we evict it. This
		// should be safe since our call stack should be empty.
		Purge(pPlugin);
		pPlugin->FinishEviction();
	}
}

void CPluginManager::LoadAll_SecondPass()
{
	for (PluginIter iter(m_plugins); !iter.done(); iter.next()) {
		CPlugin *pPlugin = (*iter);
		if (pPlugin->GetStatus() == Plugin_Loaded) {
			char error[256] = {0};
			if (!RunSecondPass(pPlugin)) {
				g_Logger.LogError("[SM] Unable to load plugin \"%s\": %s", pPlugin->GetFilename(), pPlugin->GetErrorMsg());
				Purge(pPlugin);
				pPlugin->FinishEviction();
			}
		}
	}

	m_AllPluginsLoaded = true;
}

bool CPluginManager::FindOrRequirePluginDeps(CPlugin *pPlugin)
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
						pPlugin->EvictWithError(Plugin_Failed, "Fatal error during initializing plugin load");
						return false;
					}
				}
			} else {
				/* Check that we aren't registering the same library twice */
				pPlugin->AddRequiredLib(name);

				CPlugin *found = nullptr;
				for (PluginIter iter(m_plugins); !iter.done(); iter.next()) {
					CPlugin *pl = (*iter);
					if (pl->HasLibrary(name)) {
						found = pl;
						break;
					}
				}
				if (!found) {
					pPlugin->EvictWithError(Plugin_Failed, "Could not find required plugin \"%s\"", name);
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

void CPlugin::ForEachLibrary(ke::Function<void(const char *)> callback)
{
	for (auto iter = m_Libraries.begin(); iter != m_Libraries.end(); iter++)
		callback((*iter).c_str());
}

void CPlugin::AddRequiredLib(const char *name)
{
	if (m_RequiredLibs.find(name) == m_RequiredLibs.end())
		m_RequiredLibs.push_back(name);
}

bool CPlugin::ForEachRequiredLib(ke::Function<bool(const char *)> callback)
{
	for (auto iter = m_RequiredLibs.begin(); iter != m_RequiredLibs.end(); iter++) {
		if (!callback((*iter).c_str()))
			return false;
	}
	return true;
}

void CPlugin::SetRegistered()
{
	assert(m_state == PluginState::Unregistered);
	m_state = PluginState::Registered;
}

void CPlugin::SetWaitingToUnload(bool andReload)
{
	assert(m_state == PluginState::Registered ||
			(m_state == PluginState::WaitingToUnload && andReload));
	m_state = andReload ? PluginState::WaitingToUnloadAndReload : PluginState::WaitingToUnload;
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
	pPlugin->ForEachExtVar(std::move(callback));
}

bool CPluginManager::RequireExtensions(CPlugin *pPlugin)
{
	auto callback = [pPlugin]
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
				pPlugin->EvictWithError(Plugin_Failed, "Required extension \"%s\" file(\"%s\") not running", ext.name, ext.file);
				return false;
			}
			g_Extensions.BindChildPlugin(pExt, pPlugin);
		} else {
			char buffer[64];
			ke::SafeSprintf(buffer, sizeof(buffer), "__ext_%s_SetNTVOptional", &pubvar->name[6]);

			if (IPluginFunction *pFunc = pPlugin->GetBaseContext()->GetFunctionByName(buffer)) {
				cell_t res;
				if (pFunc->Execute(&res) != SP_ERROR_NONE) {
					pPlugin->EvictWithError(Plugin_Failed, "Fatal error during plugin initialization (ext req)");
					return false;
				}
			}
		}
		return true;
	};

	return pPlugin->ForEachExtVar(std::move(callback));
}

CPlugin *CPluginManager::CompileAndPrep(const char *path)
{
	CPlugin *plugin = CPlugin::Create(path);
	if (plugin->GetStatus() != Plugin_Uncompiled) {
		assert(plugin->GetStatus() == Plugin_BadLoad);
		return plugin;
	}

	if (!plugin->TryCompile())
		return plugin;
	assert(plugin->GetStatus() == Plugin_Created);

	if (!MalwareCheckPass(plugin))
		return plugin;
	assert(plugin->GetStatus() == Plugin_Created);

	g_ShareSys.BindNativesToPlugin(plugin, true);
	plugin->InitIdentity();
	return plugin;
}


bool CPluginManager::MalwareCheckPass(CPlugin *pPlugin)
{
	unsigned char *pCodeHash = pPlugin->GetRuntime()->GetCodeHash();

	char codeHashBuf[40];
	ke::SafeStrcpy(codeHashBuf, sizeof(codeHashBuf), "plugin_");
	for (int i = 0; i < 16; i++)
		ke::SafeSprintf(codeHashBuf + 7 + (i * 2), 3, "%02x", pCodeHash[i]);

	const char *bulletinUrl = g_pGameConf->GetKeyValue(codeHashBuf);
	if (!bulletinUrl)
		return true;

	if (m_bBlockBadPlugins) {
		if (bulletinUrl[0] != '\0') {
			pPlugin->EvictWithError(Plugin_BadLoad, "Known malware detected and blocked. See %s for more info", bulletinUrl);
		} else {
			pPlugin->EvictWithError(Plugin_BadLoad, "Possible malware or illegal plugin detected and blocked");
		}
		return false;
	}

	if (bulletinUrl[0] != '\0') {
		g_Logger.LogMessage("%s: Known malware detected. See %s for more info, blocking disabled in core.cfg", pPlugin->GetFilename(), bulletinUrl);
	} else {
		g_Logger.LogMessage("%s: Possible malware or illegal plugin detected, blocking disabled in core.cfg", pPlugin->GetFilename());
	}
	return true;
}

bool CPluginManager::RunSecondPass(CPlugin *pPlugin)
{
	// Make sure external dependencies are around.
	if (!RequireExtensions(pPlugin))
		return false;
	if (!FindOrRequirePluginDeps(pPlugin))
		return false;

	// Run another binding pass.
	g_ShareSys.BindNativesToPlugin(pPlugin, false);

	// Find any unbound natives. Right now, these are not allowed.
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
			pPlugin->EvictWithError(Plugin_Failed, "Native \"%s\" was not found", native->name);
			return false;
		}
	}

	// Finish by telling all listeners.
	for (ListenerIter iter(m_listeners); !iter.done(); iter.next())
		(*iter)->OnPluginLoaded(pPlugin);

	// Tell this plugin to finish initializing itself.
	if (!pPlugin->OnPluginStart())
		return false;

	// Now, if we have fake natives, go through all plugins that might need rebinding.
	if (pPlugin->HasFakeNatives()) {
		for (PluginIter iter(m_plugins); !iter.done(); iter.next()) {
			CPlugin *pOther = (*iter);
			if ((pOther->GetStatus() == Plugin_Error
				&& (pOther->HasMissingFakeNatives() || pOther->HasMissingLibrary()))
				|| pOther->HasMissingFakeNatives())
			{
				TryRefreshDependencies(pOther);
			}
			else if ((pOther->GetStatus() == Plugin_Running
					  || pOther->GetStatus() == Plugin_Paused)
					 && pOther != pPlugin)
			{
				g_ShareSys.BeginBindingFor(pPlugin);
				pPlugin->BindFakeNativesTo(pOther);
			}
		}
	}

	// Go through our libraries and tell other plugins they're added.
	pPlugin->LibraryActions(LibraryAction_Added);

	// Add the core phrase file.
	pPlugin->GetPhrases()->AddPhraseFile("core.phrases");

	// Go through all other already loaded plugins and tell this plugin, that their libraries are loaded.
	for (PluginIter iter(m_plugins); !iter.done(); iter.next()) {
		CPlugin *pl = (*iter);
		/* Don't call our own libraries again and only care for already loaded plugins */
		if (pl == pPlugin || pl->GetStatus() != Plugin_Running)
			continue;

		pl->ForEachLibrary([pPlugin] (const char *lib) -> void {
			pPlugin->Call_OnLibraryAdded(lib);
		});
	}

	return true;
}

void CPluginManager::TryRefreshDependencies(CPlugin *pPlugin)
{
	assert(pPlugin->GetBaseContext() != NULL);

	g_ShareSys.BindNativesToPlugin(pPlugin, false);

	bool all_found = pPlugin->ForEachRequiredLib([this, pPlugin] (const char *lib) -> bool {
		CPlugin *found = nullptr;
		for (PluginIter iter(m_plugins); !iter.done(); iter.next()) {
			CPlugin *search = (*iter);
			if (search->HasLibrary(lib)) {
				found = search;
				break;
			}
		}
		if (!found) {
			pPlugin->EvictWithError(Plugin_Error, "Library not found: %s", lib);
			return false;
		}
		found->AddDependent(pPlugin);
		return true;
	});
	if (!all_found)
		return;

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
			pPlugin->EvictWithError(Plugin_Error, "Native not found: %s", native->name);
			return;
		}
	}

	if (pPlugin->GetStatus() == Plugin_Error)
	{
		/* If we got here, all natives are okay again! */
		if (pPlugin->GetRuntime()->IsPaused())
		{
			pPlugin->SetPauseState(false);
		}
	}
}

bool CPluginManager::UnloadPlugin(IPlugin *plugin)
{
	CPlugin *pPlugin = (CPlugin *)plugin;

	// Should not be recursively removing.
	assert(m_plugins.contains(pPlugin));

	// If we're already in the unload queue, just wait.
	if (pPlugin->State() == PluginState::WaitingToUnload ||
		pPlugin->State() == PluginState::WaitingToUnloadAndReload)
	{
		return false;
	}

	// It is not safe to unload any plugin while another is on the callstack.
	bool any_active = false;
	for (PluginIter iter(m_plugins); !iter.done(); iter.next()) {
		if (IPluginContext *context = (*iter)->GetBaseContext()) {
			if (context->IsInExec()) {
				any_active = true;
				break;
			}
		}
	}

	if (any_active) {
		pPlugin->SetWaitingToUnload();
		ScheduleTaskForNextFrame([this, pPlugin]() -> void {
			UnloadPluginImpl(pPlugin);
		});
		return false;
	}

	// No need to schedule an unload, we can unload immediately.
	UnloadPluginImpl(pPlugin);
	return true;
}

void CPluginManager::Purge(CPlugin *plugin)
{
	// Go through our libraries and tell other plugins they're gone.
	plugin->LibraryActions(LibraryAction_Removed);

	// Notify listeners of unloading.
	if (plugin->EnteredSecondPass()) {
		for (ListenerIter iter(m_listeners); !iter.done(); iter.next()) {
			if ((*iter)->GetApiVersion() >= kMinPluginSysApiWithWillUnloadCallback)
				(*iter)->OnPluginWillUnload(plugin);
		}
	}

	// We only pair OnPluginEnd with OnPluginStart if we would have
	// successfully called OnPluginStart, *and* SetFailState() wasn't called,
	// which guarantees no further code will execute.
	if (plugin->GetStatus() == Plugin_Running)
		plugin->Call_OnPluginEnd();

	m_pOnNotifyPluginUnloaded->PushCell(plugin->GetMyHandle());
	m_pOnNotifyPluginUnloaded->Execute(NULL);

	// Notify listeners of unloading.
	if (plugin->EnteredSecondPass()) {
		for (ListenerIter iter(m_listeners); !iter.done(); iter.next())
			(*iter)->OnPluginUnloaded(plugin);
	}

	plugin->DropEverything();

	for (ListenerIter iter(m_listeners); !iter.done(); iter.next())
		(*iter)->OnPluginDestroyed(plugin);
}

void CPluginManager::UnloadPluginImpl(CPlugin *pPlugin)
{
	m_plugins.remove(pPlugin);
	m_LoadLookup.remove(pPlugin->GetFilename());

	// Evicted plugins were already purged from external systems.
	if (pPlugin->State() != PluginState::Evicted)
		Purge(pPlugin);

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
	return new CPluginIterator(m_plugins);
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
	m_pOnNotifyPluginUnloaded = forwardsys->CreateForward("OnNotifyPluginUnloaded", ET_Ignore, 1, NULL, Param_Cell);
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
	forwardsys->ReleaseForward(m_pOnNotifyPluginUnloaded);
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
			ke::SafeStrcpy(error, maxlength, "Invalid value: must be \"yes\" or \"no\"");
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

	PluginIter iter(m_plugins);
	for (int id = 1; !iter.done() && id < num; iter.next(), id++) {
		// Empty loop.
	}
	return *iter;
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
			char plstr[10];
			ke::SafeSprintf(plstr, sizeof(plstr), "%d", plnum);
			int plpadding = strlen(plstr);

			if (!plnum)
			{
				rootmenu->ConsolePrint("[SM] No plugins loaded");
				return;
			}
			else
			{
				rootmenu->ConsolePrint("[SM] Listing %d plugin%s:", plnum, (plnum > 1) ? "s" : "");
			}

			std::list<CPlugin *> fail_list;

			for (PluginIter iter(m_plugins); !iter.done(); iter.next(), id++) {
				CPlugin *pl = (*iter);
				assert(pl->GetStatus() != Plugin_Created);
				int len = 0;
				const sm_plugininfo_t *info = pl->GetPublicInfo();
				if (pl->GetStatus() != Plugin_Running && !pl->IsSilentlyFailed())
				{
					len += ke::SafeSprintf(buffer, sizeof(buffer), "  %0*d <%s>", plpadding, id, GetStatusText(pl->GetDisplayStatus()));

					/* Plugin has failed to load. */
					fail_list.push_back(pl);
				}
				else
				{
					len += ke::SafeSprintf(buffer, sizeof(buffer), "  %0*d", plpadding, id);
				}
				if (pl->GetStatus() < Plugin_Created || pl->GetStatus() == Plugin_Evicted)
				{
					if (pl->IsSilentlyFailed())
						len += ke::SafeStrcpy(&buffer[len], sizeof(buffer)-len, " Disabled:");
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
					ke::SafeSprintf(&buffer[len], sizeof(buffer)-len, " %s", pl->GetFilename());
				}
				rootmenu->ConsolePrint("%s", buffer);
			}

			if (!fail_list.empty()) {
				rootmenu->ConsolePrint("Errors:");

				for (auto iter = fail_list.begin(); iter != fail_list.end(); iter++) {
					CPlugin *pl = (*iter);
					const sm_plugininfo_t *info = pl->GetPublicInfo();
					if (IS_STR_FILLED(info->name)) {
						rootmenu->ConsolePrint("%s (%s): %s", pl->GetFilename(), info->name, pl->GetErrorMsg());
					} else {
						rootmenu->ConsolePrint("%s: %s", pl->GetFilename(), pl->GetErrorMsg());
					}
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
				ke::SafeStrcpy(name, sizeof(name), (IS_STR_FILLED(info->name)) ? info->name : pl->GetFilename());
			}
			else
			{
				ke::SafeStrcpy(name, sizeof(name), pl->GetFilename());
			}

			if (UnloadPlugin(pl))
			{
				rootmenu->ConsolePrint("[SM] Plugin %s unloaded successfully.", name);
			}
			else
			{
				rootmenu->ConsolePrint("[SM] Plugin %s will be unloaded on the next frame.", name);
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
			if (pl->GetStatus() != Plugin_BadLoad) {
				if (IS_STR_FILLED(info->name)) {
					if (IS_STR_FILLED(info->description))
						rootmenu->ConsolePrint("  Title: %s (%s)", info->name, info->description);
					else
						rootmenu->ConsolePrint("  Title: %s", info->name);
				}
				if (IS_STR_FILLED(info->author)) {
					rootmenu->ConsolePrint("  Author: %s", info->author);
				}
				if (IS_STR_FILLED(info->version)) {
					rootmenu->ConsolePrint("  Version: %s", info->version);
				}
				if (IS_STR_FILLED(info->url)) {
					rootmenu->ConsolePrint("  URL: %s", info->url);
				}
				if (pl->IsInErrorState()) {
					rootmenu->ConsolePrint("  Error: %s", pl->GetErrorMsg());
				} else {
					rootmenu->ConsolePrint("  Status: running");
				}
				if (pl->GetFileVersion() >= 3) {
					rootmenu->ConsolePrint("  Timestamp: %s", pl->GetDateTime());
				}

				if (IPluginRuntime *runtime = pl->GetRuntime()) {
				  unsigned char *pCodeHash = runtime->GetCodeHash();
				  unsigned char *pDataHash = runtime->GetDataHash();

				  char combinedHash[33];
				  for (int i = 0; i < 16; i++)
				  	ke::SafeSprintf(combinedHash + (i * 2), 3, "%02x", pCodeHash[i] ^ pDataHash[i]);

				  rootmenu->ConsolePrint("  Hash: %s", combinedHash);
				}
			} else {
				rootmenu->ConsolePrint("  Load error: %s", pl->GetErrorMsg());
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

			if (ReloadPlugin(pl, true))
			{
				rootmenu->ConsolePrint("[SM] Plugin %s reloaded successfully.", name);
			}
			else
			{
				switch (pl->State())
				{
					//the unload/reload attempt next frame will print a message
					case PluginState::WaitingToUnload:
					case PluginState::WaitingToUnloadAndReload:
						rootmenu->ConsolePrint("[SM] Plugin %s will be reloaded on the next frame.", name);
						break;

					default:
						rootmenu->ConsolePrint("[SM] Failed to reload plugin %s.", name);
				}
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

bool CPluginManager::ReloadPlugin(CPlugin *pl, bool print)
{
	PluginState state = pl->State();
	if (state == PluginState::WaitingToUnloadAndReload)
		return false;

	std::string filename(pl->GetFilename());
	PluginType ptype = pl->GetType();

	int id = 1;
	for (PluginIter iter(m_plugins); !iter.done(); iter.next(), id++) {
		if ((*iter) == pl)
			break;
	}

	if (!UnloadPlugin(pl))
	{
		if (pl->State() == PluginState::WaitingToUnload)
		{
			pl->SetWaitingToUnload(true);
			ScheduleTaskForNextFrame([this, id, filename, ptype, print]() -> void {
				ReloadPluginImpl(id, filename.c_str(), ptype, print);
			});
		}
		return false;
	}

	ReloadPluginImpl(id, filename.c_str(), ptype, false);
	return true;
}

void CPluginManager::ReloadPluginImpl(int id, const char filename[], PluginType ptype, bool print)
{
	char error[128];
	bool wasloaded;
	IPlugin *newpl = LoadPlugin(filename, true, ptype, error, sizeof(error), &wasloaded);
	if (!newpl)
	{
		rootmenu->ConsolePrint("[SM] Plugin %s failed to reload: %s.", filename, error);
		return;
	}

	if (print)
		rootmenu->ConsolePrint("[SM] Plugin %s reloaded successfully.", filename);

	m_plugins.remove(static_cast<CPlugin *>(newpl));

	PluginIter iter(m_plugins);
	for (int i = 1; !iter.done() && i < id; iter.next(), i++) {
		// Empty loop.
	}
	m_plugins.insertBefore(iter, static_cast<CPlugin *>(newpl));
}

void CPluginManager::RefreshAll()
{
	/* If we're in a load lock, just skip this whole bit. */
	if (m_LoadingLocked)
	{
		return;
	}

	for (PluginIter iter(m_plugins); !iter.done(); iter.next()) {
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
	for (ListenerIter iter(m_listeners); !iter.done(); iter.next())
		(*iter)->OnPluginPauseChange(pl, paused);
}

void CPluginManager::AddFunctionsToForward(const char *name, IChangeableForward *pForward)
{
	for (PluginIter iter(m_plugins); !iter.done(); iter.next()) {
		CPlugin *pPlugin = (*iter);

		if (pPlugin->GetStatus() <= Plugin_Paused) {
			if (IPluginFunction *pFunction = pPlugin->GetBaseContext()->GetFunctionByName(name))
				pForward->AddFunction(pFunction);
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
	for (PluginIter iter(m_plugins); !iter.done(); iter.next()) {
		CPlugin *pl = (*iter);
		if (pl->GetStatus() != Plugin_Running)
			continue;
		if (pl->HasLibrary(lib))
			return true;
	}

	return false;
}

void CPluginManager::AllPluginsLoaded()
{
	for (PluginIter iter(m_plugins); !iter.done(); iter.next())
		(*iter)->Call_OnAllPluginsLoaded();
}

void CPluginManager::UnloadAll()
{
	for (PluginIter iter(m_plugins); !iter.done(); iter.next()) {
		UnloadPlugin((*iter));
	}
}

int CPluginManager::GetOrderOfPlugin(IPlugin *pl)
{
	int id = 1;

	for (PluginIter iter(m_plugins); !iter.done(); iter.next()) {
		if ((*iter) == pl)
			return id;
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
	for (PluginIter iter(m_plugins); !iter.done(); iter.next())
		(*iter)->SyncMaxClients(max_clients);
}

const CVector<SMPlugin *> *CPluginManager::ListPlugins()
{
	CVector<SMPlugin *> *list = new CVector<SMPlugin *>();

	for (PluginIter iter(m_plugins); !iter.done(); iter.next())
		list->push_back((*iter));

	return list;
}

void CPluginManager::FreePluginList(const CVector<SMPlugin *> *list)
{
	delete const_cast<CVector<SMPlugin *> *>(list);
}

void CPluginManager::ForEachPlugin(ke::Function<void(CPlugin *)> callback)
{
	for (PluginIter iter(m_plugins); !iter.done(); iter.next())
		callback(*iter);
}

class PluginsListenerV1Wrapper final
	: public IPluginsListener,
	  public ke::Refcounted<PluginsListenerV1Wrapper>
{
public:
	PluginsListenerV1Wrapper(IPluginsListener_V1 *impl)
		: impl_(impl)
	{}

	// The v2 listener was added with API v7, so we pin these wrappers to v6.
	unsigned int GetApiVersion() const override {
		return 6;
	}

	void OnPluginLoaded(IPlugin *plugin) override {
		impl_->OnPluginLoaded(plugin);
	}
	void OnPluginPauseChange(IPlugin *plugin, bool paused) override {
		impl_->OnPluginPauseChange(plugin, paused);
	}
	void OnPluginUnloaded(IPlugin *plugin) override {
		impl_->OnPluginUnloaded(plugin);
	}
	void OnPluginDestroyed(IPlugin *plugin) override {
		impl_->OnPluginDestroyed(plugin);
	}

	bool matches(IPluginsListener_V1 *impl) const {
		return impl_ == impl;
	}

private:
	IPluginsListener_V1 *impl_;
};

class OldPluginAPI final : public IPluginManager
{
public:
	IPlugin *LoadPlugin(const char *path,
						bool debug,
						PluginType type,
						char error[],
						size_t maxlength,
						bool *wasloaded) override
	{
		return g_PluginSys.LoadPlugin(path, debug, type, error, maxlength, wasloaded);
	}

	bool UnloadPlugin(IPlugin *plugin) override
	{
		return g_PluginSys.UnloadPlugin(plugin);
	}

	IPlugin *FindPluginByContext(const sp_context_t *ctx) override
	{
		return g_PluginSys.FindPluginByContext(ctx);
	}

	unsigned int GetPluginCount() override
	{
		return g_PluginSys.GetPluginCount();
	}

	IPluginIterator *GetPluginIterator() override
	{
		return g_PluginSys.GetPluginIterator();
	}

	void AddPluginsListener_V1(IPluginsListener_V1 *listener) override
	{
		ke::RefPtr<PluginsListenerV1Wrapper> wrapper = new PluginsListenerV1Wrapper(listener);

		v1_wrappers_.push_back(wrapper);
		g_PluginSys.AddPluginsListener(wrapper);
	}

	void RemovePluginsListener_V1(IPluginsListener_V1 *listener) override
	{
		ke::RefPtr<PluginsListenerV1Wrapper> wrapper;

		// Find which wrapper has this listener.
		for (decltype(v1_wrappers_)::iterator iter(v1_wrappers_); !iter.done(); iter.next()) {
			if ((*iter)->matches(listener)) {
				wrapper = *iter;
				iter.remove();
				break;
			}
		}
		g_PluginSys.RemovePluginsListener(wrapper);
	}

	IPlugin *PluginFromHandle(Handle_t handle, HandleError *err) override
	{
		return g_PluginSys.PluginFromHandle(handle, err);
	}

	void AddPluginsListener(IPluginsListener *listener) override
	{
		g_PluginSys.AddPluginsListener(listener);
	}

	void RemovePluginsListener(IPluginsListener *listener) override
	{
		g_PluginSys.RemovePluginsListener(listener);
	}

private:
	ReentrantList<ke::RefPtr<PluginsListenerV1Wrapper>> v1_wrappers_;
};

static OldPluginAPI sOldPluginAPI;

IPluginManager *CPluginManager::GetOldAPI()
{
	return &sOldPluginAPI;
}
