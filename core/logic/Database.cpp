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

#include "Database.h"
#include "ISourceMod.h"
#include "HandleSys.h"
#include "ExtensionSys.h"
#include "PluginSys.h"
#include <chrono>
#include <amtl/am-thread.h>
#include <stdlib.h>
#include <IThreader.h>
#include <bridge/include/ILogger.h>
#include <bridge/include/CoreProvider.h>

using namespace std::chrono_literals;

#define DBPARSE_LEVEL_NONE		0
#define DBPARSE_LEVEL_MAIN		1
#define DBPARSE_LEVEL_DATABASE	2

DBManager g_DBMan;

DBManager::DBManager() 
	: m_Terminate(false),
	  m_pDefault(NULL)
{
}

static void FrameHook(bool simulating)
{
	g_DBMan.RunFrame();
}

void DBManager::OnSourceModAllInitialized()
{
	HandleAccess sec;

	g_HandleSys.InitAccessDefaults(NULL, &sec);
	sec.access[HandleAccess_Delete] |= HANDLE_RESTRICT_IDENTITY;
	sec.access[HandleAccess_Clone] |= HANDLE_RESTRICT_IDENTITY;

	m_DriverType = g_HandleSys.CreateType("IDriver", this, 0, NULL, &sec, g_pCoreIdent, NULL);
	m_DatabaseType = g_HandleSys.CreateType("IDatabase", this, 0, NULL, NULL, g_pCoreIdent, NULL);

	g_ShareSys.AddInterface(NULL, this);

	g_pSM->BuildPath(Path_SM, m_Filename, sizeof(m_Filename), "configs/databases.cfg");
	m_Builder.SetPath(m_Filename);
	
	g_PluginSys.AddPluginsListener(this);

	g_pSM->AddGameFrameHook(&FrameHook);

	auto sm_reload_databases = [this] (int client, const ICommandArgs *args) -> bool {
		m_Builder.StartParse();
		return true;
	};
	bridge->DefineCommand("sm_reload_databases", "Reparse database configurations file", sm_reload_databases);
}

void DBManager::OnSourceModLevelChange(const char *mapName)
{
	m_Builder.StartParse();
}

void DBManager::OnSourceModShutdown()
{
	g_pSM->RemoveGameFrameHook(&FrameHook);
	KillWorkerThread();
	g_PluginSys.RemovePluginsListener(this);
	g_HandleSys.RemoveType(m_DatabaseType, g_pCoreIdent);
	g_HandleSys.RemoveType(m_DriverType, g_pCoreIdent);
}

unsigned int DBManager::GetInterfaceVersion()
{
	return SMINTERFACE_DBI_VERSION;
}

const char *DBManager::GetInterfaceName()
{
	return SMINTERFACE_DBI_NAME;
}

void DBManager::OnHandleDestroy(HandleType_t type, void *object)
{
	if (type == m_DriverType)
	{
		/* Ignore */
		return;
	}

	if (g_HandleSys.TypeCheck(type, m_DatabaseType))
	{
		IDatabase *pdb = (IDatabase *)object;
		pdb->Close();
	}
}

bool DBManager::Connect(const char *name, IDBDriver **pdr, IDatabase **pdb, bool persistent, char *error, size_t maxlength)
{
	ConfDbInfoList &list = m_Builder.GetConfigList();
	ke::RefPtr<ConfDbInfo> pInfo = list.GetDatabaseConf(name);

	if (!pInfo)
	{
		if (pdr)
		{
			*pdr = NULL;
		}
		*pdb = NULL;
		g_pSM->Format(error, maxlength, "Configuration \"%s\" not found", name);
		return false;
	}

	const char *dname = pInfo->info.driver;
	if (!pInfo->realDriver)
	{
		/* Try to assign a real driver pointer */
		if (pInfo->info.driver[0] == '\0')
		{
			std::string defaultDriver = list.GetDefaultDriver();
			if (!m_pDefault && defaultDriver.length() > 0)
			{
				m_pDefault = FindOrLoadDriver(defaultDriver.c_str());
			}
			dname = defaultDriver.length() ? defaultDriver.c_str() : "default";
			pInfo->realDriver = m_pDefault;
		} else {
			pInfo->realDriver = FindOrLoadDriver(pInfo->info.driver);
		}
	}

	if (pInfo->realDriver)
	{
		if (pdr)
		{
			*pdr = pInfo->realDriver;
		}
		*pdb = pInfo->realDriver->Connect(&pInfo->info, persistent, error, maxlength);
		return (*pdb != NULL);
	}

	if (pdr)
	{
		*pdr = NULL;
	}
	*pdb = NULL;

	g_pSM->Format(error, maxlength, "Driver \"%s\" not found", dname);
	return false;
}

void DBManager::AddDriver(IDBDriver *pDriver)
{
	 /* Let's kill the worker.  Join the thread and let the queries flush.
	  * This is kind of stupid but we just want to unload safely.
	  * Rather than recreate the worker, we'll wait until someone throws 
	  * another query through.
	  */
	KillWorkerThread();

	m_drivers.push_back(pDriver);
}

void DBManager::RemoveDriver(IDBDriver *pDriver)
{
	/* Again, we're forced to kill the worker.  How rude!
	 * Doing this flushes the queue, and thus we don't need to 
	 * clean anything else.  
	 */
	KillWorkerThread();

	for (size_t i=0; i<m_drivers.size(); i++)
	{
		if (m_drivers[i] == pDriver)
		{
			m_drivers.erase(m_drivers.iterAt(i));
			break;
		}
	}

	ConfDbInfoList &list = m_Builder.GetConfigList();
	for (auto conf : list) {
		if (conf->realDriver == pDriver)
		{
			conf->realDriver = NULL;
		}
	}

	/* Someone unloaded the default driver? Silly.. */
	if (pDriver == m_pDefault)
	{
		m_pDefault = NULL;
	}

	/* Now that the driver is gone, we have to test the think queue.
	 * Whatever happens therein is up to the db op!
	 */
	Queue<IDBThreadOperation *>::iterator qiter = m_ThinkQueue.begin();
	Queue<IDBThreadOperation *> templist;
	while (qiter != m_ThinkQueue.end())
	{
		IDBThreadOperation *op = (*qiter);
		if (op->GetDriver() == pDriver)
		{
			templist.push(op);
			qiter = m_ThinkQueue.erase(qiter);
		} else {
			qiter++;
		}
	}

	for (qiter = templist.begin();
		 qiter != templist.end();
		 qiter++)
	{
		IDBThreadOperation *op = (*qiter);
		op->CancelThinkPart();
		op->Destroy();
	}
}

IDBDriver *DBManager::GetDefaultDriver()
{
	ConfDbInfoList &list = m_Builder.GetConfigList();
	std::string defaultDriver = list.GetDefaultDriver();
	if (!m_pDefault && defaultDriver.length() > 0)
	{
		m_pDefault = FindOrLoadDriver(defaultDriver.c_str());
	}

	return m_pDefault;
}

Handle_t DBManager::CreateHandle(DBHandleType dtype, void *ptr, IdentityToken_t *pToken)
{
	HandleType_t type = 0;

	if (dtype == DBHandle_Driver)
	{
		type = m_DriverType;
	} else if (dtype == DBHandle_Database) {
		type = m_DatabaseType;
	} else {
		return BAD_HANDLE;
	}

	return g_HandleSys.CreateHandle(type, ptr, pToken, g_pCoreIdent, NULL);
}

HandleError DBManager::ReadHandle(Handle_t hndl, DBHandleType dtype, void **ptr)
{
	HandleType_t type = 0;

	if (dtype == DBHandle_Driver)
	{
		type = m_DriverType;
	} else if (dtype == DBHandle_Database) {
		type = m_DatabaseType;
	} else {
		return HandleError_Type;
	}

	HandleSecurity sec(NULL, g_pCoreIdent);

	return g_HandleSys.ReadHandle(hndl, type, &sec, ptr);
}

HandleError DBManager::ReleaseHandle(Handle_t hndl, DBHandleType type, IdentityToken_t *token)
{
	HandleSecurity sec(token, g_pCoreIdent);
	return g_HandleSys.FreeHandle(hndl, &sec);
}

unsigned int DBManager::GetDriverCount()
{
	return (unsigned int)m_drivers.size();
}

IDBDriver *DBManager::GetDriver(unsigned int index)
{
	if (index >= m_drivers.size())
	{
		return NULL;
	}

	return m_drivers[index];
}

const DatabaseInfo *DBManager::FindDatabaseConf(const char *name)
{
	ConfDbInfoList &list = m_Builder.GetConfigList();
	ke::RefPtr<ConfDbInfo> info = list.GetDatabaseConf(name);
	if (!info)
	{
		// couldn't find requested conf, return default if exists
		info = list.GetDefaultConfiguration();
		if (!info)
		{
			return NULL;
		}
	}

	return &info->info;
}

ke::RefPtr<ConfDbInfo> DBManager::GetDatabaseConf(const char *name)
{
	ConfDbInfoList &list = m_Builder.GetConfigList();
	return list.GetDatabaseConf(name);
}

IDBDriver *DBManager::FindOrLoadDriver(const char *name)
{
	size_t last_size = m_drivers.size();
	for (size_t i=0; i<last_size; i++)
	{
		if (strcmp(m_drivers[i]->GetIdentifier(), name) == 0)
		{
			return m_drivers[i];
		}
	}

	char filename[PLATFORM_MAX_PATH];
	g_pSM->Format(filename, sizeof(filename), "dbi.%s.ext", name);

	IExtension *pExt = g_Extensions.LoadAutoExtension(filename);
	if (!pExt || !pExt->IsLoaded() || m_drivers.size() <= last_size)
	{
		return NULL;
	}

	/* last_size is now gauranteed to be a valid index.
	 * The identifier must match the name.
	 */
	if (strcmp(m_drivers[last_size]->GetIdentifier(), name) == 0)
	{
		return m_drivers[last_size];
	}

	return NULL;
}

void DBManager::KillWorkerThread()
{
	if (m_Worker)
	{
		{
			std::lock_guard<std::mutex> lock(m_Lock);
			m_Terminate = true;
			m_QueueEvent.notify_all();
		}
		m_Worker->join();
		m_Worker = nullptr;
		m_Terminate = false;
	}
}

static IdentityToken_t *s_pAddBlock = NULL;

bool DBManager::AddToThreadQueue(IDBThreadOperation *op, PrioQueueLevel prio)
{
	if (s_pAddBlock && op->GetOwner() == s_pAddBlock)
	{
		return false;
	}

	if (!m_Worker)
	{
		m_Worker = ke::NewThread("SM Database Worker", [this]() -> void {
			Run();
		});
	}

	/* Add to the queue */
	{
		std::lock_guard<std::mutex> lock(m_Lock);
		Queue<IDBThreadOperation *> &queue = m_OpQueue.GetQueue(prio);
		queue.push(op);
		m_QueueEvent.notify_one();
	}

	return true;
}

void DBManager::Run()
{
	// Initialize DB threadsafety.
	for (size_t i=0; i < m_drivers.size(); i++)
	{
		if (m_drivers[i]->IsThreadSafe())
			m_drSafety.push_back(m_drivers[i]->InitializeThreadSafety());
		else
			m_drSafety.push_back(false);
	}

	// Run actual worker thread logic.
	ThreadMain();

	// Shutdown DB threadsafety.
	for (size_t i=0; i<m_drivers.size(); i++)
	{
		if (m_drSafety[i])
			m_drivers[i]->ShutdownThreadSafety();
	}
	m_drSafety.clear();
}

void DBManager::ThreadMain()
{
	std::unique_lock<std::mutex> lock(m_Lock);

	while (true) {
		// The lock has been acquired. Grab everything we can out of the
		// queue. Since we want to flush the queue even if we're terminated,
		// we process all operations we can before checking to terminate.
		// There's no risk of starvation since the main thread blocks on us
		// terminating.
		auto queue = &m_OpQueue.GetLikelyQueue();
		if (queue->empty()) {
			// If the queue is empty and we've been asked to stop, leave now.
			if (m_Terminate)
				return;

			// Otherwise, wait for something to happen.
			m_QueueEvent.wait(lock);
			continue;
		}

		IDBThreadOperation *op = queue->first();
		queue->pop();

		// Unlock the queue when we run the query, so the main thread can
		// keep pumping events. We re-acquire the lock to check for more
		// items. It's okay if we terminate while unlocked; the main
		// thread would be blocked and we'd need to flush the queue
		// anyway, so after we've depleted the queue here, we'll just
		// reach the terminate at the top of the loop.
		lock.unlock();
		op->RunThreadPart();

		// Re-acquire the lock and give the data back to the main thread
		// immediately. We use a separate lock to minimize game thread
		// contention.
		{
			std::lock_guard<std::mutex> think_lock(m_ThinkLock);
			m_ThinkQueue.push(op);
		}

		// Note that we add a 20ms delay after processing a query. This is
		// questionable but the intent is to avoid starving the game thread.
		if (!m_Terminate)
			std::this_thread::sleep_for(20ms);

		lock.lock();
	}
}

void DBManager::RunFrame()
{
	/* Don't bother if we're empty */
	if (!m_ThinkQueue.size())
	{
		return;
	}

	/* Dump one thing per-frame so the server stays sane. */
	IDBThreadOperation *op;
	{
		std::lock_guard<std::mutex> lock(m_ThinkLock);
		op = m_ThinkQueue.first();
		m_ThinkQueue.pop();
	}
	op->RunThinkPart();
	op->Destroy();
}

void DBManager::OnSourceModIdentityDropped(IdentityToken_t *pToken)
{
	s_pAddBlock = pToken;

	/* Kill the thread so we can flush everything into the think queue... */
	KillWorkerThread();

	/* Run all of the think operations.
 	 * Unlike the driver unloading example, we'll let these calls go through, 
	 * since a plugin unloading is far more normal.
	 */
	Queue<IDBThreadOperation *>::iterator iter = m_ThinkQueue.begin();
	Queue<IDBThreadOperation *> templist;
	while (iter != m_ThinkQueue.end())
	{
		IDBThreadOperation *op = (*iter);
		if (op->GetOwner() == pToken)
		{
			templist.push(op);
			iter = m_ThinkQueue.erase(iter);
		} else {
			iter++;
		}
	}

	for (iter = templist.begin();
		iter != templist.end();
		iter++)
	{
		IDBThreadOperation *op = (*iter);
		op->RunThinkPart();
		op->Destroy();
	}

	s_pAddBlock = NULL;
}

void DBManager::OnPluginWillUnload(IPlugin *plugin)
{
	/* Kill the thread so we can flush everything into the think queue... */
	KillWorkerThread();

	/* Mark the plugin as being unloaded so future database calls will ignore threading... */
	plugin->SetProperty("DisallowDBThreads", NULL);

	/* Run all of the think operations.
	 * Unlike the driver unloading example, we'll let these calls go through, 
	 * since a plugin unloading is far more normal.
	 */
	Queue<IDBThreadOperation *>::iterator iter = m_ThinkQueue.begin();
	Queue<IDBThreadOperation *> templist;
	while (iter != m_ThinkQueue.end())
	{
		IDBThreadOperation *op = (*iter);
		if (op->GetOwner() == plugin->GetIdentity())
		{
			templist.push(op);
			iter = m_ThinkQueue.erase(iter);
		} else {
			iter++;
		}
	}

	for (iter = templist.begin(); iter != templist.end(); iter++)
	{
		IDBThreadOperation *op = (*iter);
		op->RunThinkPart();
		op->Destroy();
	}
}

std::string DBManager::GetDefaultDriverName()
{
	ConfDbInfoList &list = m_Builder.GetConfigList();
	return list.GetDefaultDriver();
}

void DBManager::AddDependency(IExtension *myself, IDBDriver *driver)
{
	g_Extensions.AddRawDependency(myself, driver->GetIdentity(), driver);
}
