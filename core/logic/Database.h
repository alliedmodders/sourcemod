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

#ifndef _INCLUDE_DATABASE_MANAGER_H_
#define _INCLUDE_DATABASE_MANAGER_H_

#include "common_logic.h"
#include <sh_vector.h>
#include <am-string.h>
#include <sh_list.h>
#include <IThreader.h>
#include <IPluginSys.h>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include "sm_simple_prioqueue.h"
#include <am-refcounting.h>
#include "DatabaseConfBuilder.h"

using namespace SourceHook;


class DBManager : 
	public IDBManager,
	public SMGlobalClass,
	public IHandleTypeDispatch,
	public IPluginsListener
{
public:
	DBManager();
public:
	const char *GetInterfaceName();
	unsigned int GetInterfaceVersion();
public: //SMGlobalClass
	void OnSourceModAllInitialized();
	void OnSourceModLevelChange(const char *mapName);
	void OnSourceModIdentityDropped(IdentityToken_t *pToken);
	void OnSourceModShutdown();
public: //IHandleTypeDispatch
	void OnHandleDestroy(HandleType_t type, void *object);
public: //IDBManager
	void AddDriver(IDBDriver *pDrivera);
	void RemoveDriver(IDBDriver *pDriver);
	const DatabaseInfo *FindDatabaseConf(const char *name);
	ke::RefPtr<ConfDbInfo> GetDatabaseConf(const char *name);
	bool Connect(const char *name, IDBDriver **pdr, IDatabase **pdb, bool persistent, char *error, size_t maxlength);
	unsigned int GetDriverCount();
	IDBDriver *GetDriver(unsigned int index);
	Handle_t CreateHandle(DBHandleType type, void *ptr, IdentityToken_t *pToken);
	HandleError ReadHandle(Handle_t hndl, DBHandleType type, void **ptr);
	HandleError ReleaseHandle(Handle_t hndl, DBHandleType type, IdentityToken_t *token);
	void AddDependency(IExtension *myself, IDBDriver *driver);
public: //ke::IRunnable
	void Run();
	void ThreadMain();
public: //IPluginsListener
	void OnPluginWillUnload(IPlugin *plugin);
public:
	IDBDriver *FindOrLoadDriver(const char *name);
	IDBDriver *GetDefaultDriver();
	std::string GetDefaultDriverName();
	bool AddToThreadQueue(IDBThreadOperation *op, PrioQueueLevel prio);
	void RunFrame();
	inline HandleType_t GetDatabaseType()
	{
		return m_DatabaseType;
	}
private:
	void ClearConfigs();
	void KillWorkerThread();
private:
	CVector<IDBDriver *> m_drivers;

	/* Threading stuff */
	PrioQueue<IDBThreadOperation *> m_OpQueue;
	Queue<IDBThreadOperation *> m_ThinkQueue;
	CVector<bool> m_drSafety;			/* which drivers are safe? */
	std::unique_ptr<std::thread> m_Worker;
	std::condition_variable m_QueueEvent;
	std::mutex m_ThinkLock;
	std::mutex m_Lock;
	bool m_Terminate;

	DatabaseConfBuilder m_Builder;
	HandleType_t m_DriverType;
	HandleType_t m_DatabaseType;
	char m_Filename[PLATFORM_MAX_PATH];
	IDBDriver *m_pDefault;
};

extern DBManager g_DBMan;

#endif //_INCLUDE_DATABASE_MANAGER_H_
