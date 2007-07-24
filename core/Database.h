/**
 * vim: set ts=4 :
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

#ifndef _INCLUDE_DATABASE_MANAGER_H_
#define _INCLUDE_DATABASE_MANAGER_H_

#include <IDBDriver.h>
#include "sm_globals.h"
#include <sh_vector.h>
#include <sh_string.h>
#include <sh_list.h>
#include <ITextParsers.h>
#include "sm_memtable.h"
#include <IThreader.h>
#include "sm_simple_prioqueue.h"
#include "PluginSys.h"

using namespace SourceHook;

struct ConfDbInfo
{
	ConfDbInfo() : name(-1), driver(-1), host(-1), user(-1), pass(-1), 
		database(-1), realDriver(NULL)
	{
	}
	int name;
	int driver;
	int host;
	int user;
	int pass;
	int database;
	IDBDriver *realDriver;
	DatabaseInfo info;
};

class DBManager : 
	public IDBManager,
	public SMGlobalClass,
	public IHandleTypeDispatch,
	public ITextListener_SMC,
	public IThread,
	public IThreadWorkerCallbacks,
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
	bool Connect(const char *name, IDBDriver **pdr, IDatabase **pdb, bool persistent, char *error, size_t maxlength);
	unsigned int GetDriverCount();
	IDBDriver *GetDriver(unsigned int index);
	Handle_t CreateHandle(DBHandleType type, void *ptr, IdentityToken_t *pToken);
	HandleError ReadHandle(Handle_t hndl, DBHandleType type, void **ptr);
	HandleError ReleaseHandle(Handle_t hndl, DBHandleType type, IdentityToken_t *token);
public: //ITextListener_SMC
	void ReadSMC_ParseStart();
	SMCParseResult ReadSMC_NewSection(const char *name, bool opt_quotes);
	SMCParseResult ReadSMC_KeyValue(const char *key, const char *value, bool key_quotes, bool value_quotes);
	SMCParseResult ReadSMC_LeavingSection();
	void ReadSMC_ParseEnd(bool halted, bool failed);
public: //IThread
	void RunThread(IThreadHandle *pThread);
	void OnTerminate(IThreadHandle *pThread, bool cancel);
public: //IThreadWorkerCallbacks
	void OnWorkerStart(IThreadWorker *pWorker);
	void OnWorkerStop(IThreadWorker *pWorker);
public: //IPluginsListener
	void OnPluginUnloaded(IPlugin *plugin);
public:
	ConfDbInfo *GetDatabaseConf(const char *name);
	IDBDriver *FindOrLoadDriver(const char *name);
	IDBDriver *GetDefaultDriver();
	const char *GetDefaultDriverName();
	bool AddToThreadQueue(IDBThreadOperation *op, PrioQueueLevel prio);
	void LockConfig();
	void UnlockConfig();
	void RunFrame();
	inline HandleType_t GetDatabaseType()
	{
		return m_DatabaseType;
	}
private:
	void KillWorkerThread();
private:
	CVector<IDBDriver *> m_drivers;

	/* Threading stuff */
	PrioQueue<IDBThreadOperation *> m_OpQueue;
	Queue<IDBThreadOperation *> m_ThinkQueue;
	CVector<bool> m_drSafety;			/* which drivers are safe? */
	IThreadWorker *m_pWorker;			/* Worker thread object */
	IMutex *m_pConfigLock;				/* Configuration lock */
	IMutex *m_pQueueLock;				/* Queue safety lock */
	IMutex *m_pThinkLock;				/* Think-queue lock */

	List<ConfDbInfo> m_confs;
	HandleType_t m_DriverType;
	HandleType_t m_DatabaseType;
	String m_DefDriver;
	BaseStringTable m_StrTab;
	char m_Filename[PLATFORM_MAX_PATH];
	unsigned int m_ParseLevel;
	unsigned int m_ParseState;
	IDBDriver *m_pDefault;
};

extern DBManager g_DBMan;

#endif //_INCLUDE_DATABASE_MANAGER_H_
