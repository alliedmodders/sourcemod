/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2014 AlliedModders LLC.  All rights reserved.
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

#include <memory>

#include "common_logic.h"
#include "Database.h"
#include "ExtensionSys.h"
#include "sprintf.h"
#include "stringutil.h"
#include "ISourceMod.h"
#include "AutoHandleRooter.h"
#include "common_logic.h"
#include <amtl/am-string.h>
#include <amtl/am-vector.h>
#include <amtl/am-refcounting.h>
#include <bridge/include/IScriptManager.h>
#include <bridge/include/CoreProvider.h>

HandleType_t hStmtType;
HandleType_t hCombinedQueryType;
HandleType_t hTransactionType;

struct CombinedQuery
{
	IQuery *query;
	IDatabase *db;

	CombinedQuery(IQuery *query, IDatabase *db)
	: query(query), db(db)
	{
	}
};

struct Transaction
{
	struct Entry
	{
		std::string query;
		cell_t data;
	};

	std::vector<Entry> entries;
};

class DatabaseHelpers : 
	public SMGlobalClass,
	public IHandleTypeDispatch
{
public:
	virtual void OnSourceModAllInitialized()
	{
		/* Disable cloning */
		HandleAccess acc;
		handlesys->InitAccessDefaults(NULL, &acc);
		acc.access[HandleAccess_Clone] = HANDLE_RESTRICT_OWNER|HANDLE_RESTRICT_IDENTITY;

		TypeAccess tacc;
		handlesys->InitAccessDefaults(&tacc, NULL);
		tacc.ident = g_pCoreIdent;

		hCombinedQueryType = handlesys->CreateType("IQuery", this, 0, &tacc, &acc, g_pCoreIdent, NULL);
		hStmtType = handlesys->CreateType("IPreparedQuery", this, 0, &tacc, &acc, g_pCoreIdent, NULL);
		hTransactionType = handlesys->CreateType("Transaction", this, 0, &tacc, &acc, g_pCoreIdent, NULL);
	}

	virtual void OnSourceModShutdown()
	{
		handlesys->RemoveType(hTransactionType, g_pCoreIdent);
		handlesys->RemoveType(hStmtType, g_pCoreIdent);
		handlesys->RemoveType(hCombinedQueryType, g_pCoreIdent);
	}

	virtual void OnHandleDestroy(HandleType_t type, void *object)
	{
		if (type == hCombinedQueryType)
		{
			CombinedQuery *combined = (CombinedQuery *)object;
			combined->query->Destroy();
			delete combined;
		} else if (type == hStmtType) {
			IPreparedQuery *query = (IPreparedQuery *)object;
			query->Destroy();
		} else if (type == hTransactionType) {
			delete (Transaction *)object;
		}
	}
} s_DatabaseNativeHelpers;

// Create a handle that can only be closed locally. That's the intent, at
// least. Since its callers pass the plugin's identity, the plugin can just
// close it anyway.
static inline Handle_t CreateLocalHandle(HandleType_t type, void *object, const HandleSecurity *sec)
{
	HandleAccess access;
	handlesys->InitAccessDefaults(NULL, &access);

	access.access[HandleAccess_Delete] = HANDLE_RESTRICT_IDENTITY|HANDLE_RESTRICT_OWNER;

	return handlesys->CreateHandleEx(type, object, sec, &access, NULL);
}

//is this safe for stmt handles? i think since it's single inheritance, it always will be.
inline HandleError ReadQueryHndl(Handle_t hndl, IPluginContext *pContext, IQuery **query)
{
	HandleSecurity sec;
	CombinedQuery *c;
	sec.pOwner = pContext->GetIdentity();
	sec.pIdentity = g_pCoreIdent;

	HandleError ret;
	
	if ((ret = handlesys->ReadHandle(hndl, hStmtType, &sec, (void **)query)) != HandleError_None)
	{
		ret = handlesys->ReadHandle(hndl, hCombinedQueryType, &sec, (void **)&c);
		if (ret == HandleError_None)
		{
			*query = c->query;
		}
	}
	return ret;
}

inline HandleError ReadQueryAndDbHndl(Handle_t hndl, IPluginContext *pContext, IQuery **query, IDatabase **db)
{
	HandleSecurity sec;
	CombinedQuery *c;
	sec.pOwner = pContext->GetIdentity();
	sec.pIdentity = g_pCoreIdent;

	HandleError ret = handlesys->ReadHandle(hndl, hCombinedQueryType, &sec, (void **)&c);
	if (ret == HandleError_None)
	{
		*query = c->query;
		*db = c->db;
	}
	return ret;
}

inline HandleError ReadStmtHndl(Handle_t hndl, IPluginContext *pContext, IPreparedQuery **query)
{
	HandleSecurity sec;
	sec.pOwner = pContext->GetIdentity();
	sec.pIdentity = g_pCoreIdent;

	return handlesys->ReadHandle(hndl, hStmtType, &sec, (void **)query);
}

inline HandleError ReadDbOrStmtHndl(Handle_t hndl, IPluginContext *pContext, IDatabase **db, IPreparedQuery **query)
{
	HandleError err;
	if ((err = g_DBMan.ReadHandle(hndl, DBHandle_Database, (void **)db)) == HandleError_Type)
	{
		*db = NULL;
		return ReadStmtHndl(hndl, pContext, query);
	}
	return err;
}

class TQueryOp : public IDBThreadOperation
{
public:
	TQueryOp(IDatabase *db, IPluginFunction *pf, const char *query, cell_t data) : 
	  m_pDatabase(db), m_pFunction(pf), m_Query(query), m_Data(data),
	  me(scripts->FindPluginByContext(pf->GetParentContext()->GetContext())),
	  m_pQuery(NULL)
	{
		/* We always increase the reference count because this is potentially
		 * asynchronous.  Otherwise the original handle could be closed while 
		 * we're still latched onto it.
		 */
		m_pDatabase->IncReferenceCount();

		HandleSecurity sec(me->GetIdentity(), g_pCoreIdent);
		m_MyHandle = CreateLocalHandle(g_DBMan.GetDatabaseType(), m_pDatabase, &sec);
	}
	~TQueryOp()
	{
		if (m_pQuery)
		{
			m_pQuery->Destroy();
		}

		/* Close our Handle if it's valid. */
		if (m_MyHandle != BAD_HANDLE)
		{
			HandleSecurity sec(me->GetIdentity(), g_pCoreIdent);
			handlesys->FreeHandle(m_MyHandle, &sec);
		} else {
			/* Otherwise, there is an open ref to the db */
			m_pDatabase->Close();
		}
	}
	IdentityToken_t *GetOwner()
	{
		return me->GetIdentity();
	}
	IDBDriver *GetDriver()
	{
		return m_pDatabase->GetDriver();
	}
	void RunThreadPart()
	{
		m_pDatabase->LockForFullAtomicOperation();
		m_pQuery = m_pDatabase->DoQuery(m_Query.c_str());
		if (!m_pQuery)
		{
			g_pSM->Format(error, sizeof(error), "%s", m_pDatabase->GetError());
		}
		m_pDatabase->UnlockFromFullAtomicOperation();
	}
	void CancelThinkPart()
	{
		if (!m_pFunction->IsRunnable())
			return;

		m_pFunction->PushCell(BAD_HANDLE);
		m_pFunction->PushCell(BAD_HANDLE);
		m_pFunction->PushString("Driver is unloading");
		m_pFunction->PushCell(m_Data);
		m_pFunction->Execute(NULL);
	}
	void RunThinkPart()
	{
		/* Create a Handle for our query */
		HandleSecurity sec(me->GetIdentity(), g_pCoreIdent);

		Handle_t qh = BAD_HANDLE;
		
		if (m_pQuery)
		{
			CombinedQuery *c = new CombinedQuery(m_pQuery, m_pDatabase);
			
			qh = CreateLocalHandle(hCombinedQueryType, c, &sec);
			if (qh != BAD_HANDLE)
			{
				m_pQuery = NULL;
			} else {
				g_pSM->Format(error, sizeof(error), "Could not alloc handle");
				delete c;
			}
		}

		if (m_pFunction->IsRunnable())
		{
			m_pFunction->PushCell(m_MyHandle);
			m_pFunction->PushCell(qh);
			m_pFunction->PushString(qh == BAD_HANDLE ? error : "");
			m_pFunction->PushCell(m_Data);
			m_pFunction->Execute(NULL);
		}

		if (qh != BAD_HANDLE)
		{
			handlesys->FreeHandle(qh, &sec);
		}
	}
	void Destroy()
	{
		delete this;
	}
private:
	IDatabase *m_pDatabase;
	IPluginFunction *m_pFunction;
	String m_Query;
	cell_t m_Data;
	IPlugin *me;
	IQuery *m_pQuery;
	char error[255];
	Handle_t m_MyHandle;
};

enum AsyncCallbackMode {
	ACM_Old,
	ACM_New
};

class TConnectOp : public IDBThreadOperation
{
public:
	TConnectOp(IPluginFunction *func, IDBDriver *driver, const char *_dbname, AsyncCallbackMode acm, cell_t data)
	{
		m_pFunction = func;
		m_pDriver = driver;
		m_pDatabase = NULL;
		m_ACM = acm;
		m_Data = data;
		error[0] = '\0';
		strncopy(dbname, _dbname, sizeof(dbname));
		me = scripts->FindPluginByContext(m_pFunction->GetParentContext()->GetContext());
		
		m_pInfo = g_DBMan.GetDatabaseConf(dbname);
		if (!m_pInfo)
		{
			g_pSM->Format(error, sizeof(error), "Could not find database config \"%s\"", dbname);
		}
	}
	IdentityToken_t *GetOwner()
	{
		return me->GetIdentity();
	}
	IDBDriver *GetDriver()
	{
		return m_pDriver;
	}
	void RunThreadPart()
	{
		if (m_pInfo)
		{
			m_pDatabase = m_pDriver->Connect(&m_pInfo->info, false, error, sizeof(error));
		}
	}
	void CancelThinkPart()
	{
		if (m_pDatabase)
			m_pDatabase->Close();
		
		if (!m_pFunction->IsRunnable())
			return;

		if (m_ACM == ACM_Old)
			m_pFunction->PushCell(BAD_HANDLE);
		m_pFunction->PushCell(BAD_HANDLE);
		m_pFunction->PushString("Driver is unloading");
		m_pFunction->PushCell(m_Data);
		m_pFunction->Execute(NULL);
	}
	void RunThinkPart()
	{
		Handle_t hndl = BAD_HANDLE;
		
		if (!m_pFunction->IsRunnable())
		{
			if (m_pDatabase)
				m_pDatabase->Close();
			return;
		}

		if (m_pDatabase)
		{
			if ((hndl = g_DBMan.CreateHandle(DBHandle_Database, m_pDatabase, me->GetIdentity()))
				== BAD_HANDLE)
			{
				m_pDatabase->Close();
				g_pSM->Format(error, sizeof(error), "Unable to allocate Handle");
			}
		}

		if (m_ACM == ACM_Old)
			m_pFunction->PushCell(m_pDriver->GetHandle());
		m_pFunction->PushCell(hndl);
		m_pFunction->PushString(hndl == BAD_HANDLE ? error : "");
		m_pFunction->PushCell(m_Data);
		m_pFunction->Execute(NULL);
	}
	void Destroy()
	{
		delete this;
	}
private:
	ke::RefPtr<ConfDbInfo> m_pInfo;
	IPlugin *me;
	IPluginFunction *m_pFunction;
	IDBDriver *m_pDriver;
	IDatabase *m_pDatabase;
	AsyncCallbackMode m_ACM;
	char dbname[64];
	char error[255];
	cell_t m_Data;
};

static cell_t SQL_Connect(IPluginContext *pContext, const cell_t *params)
{
	char *conf, *err;

	size_t maxlength = (size_t)params[4];
	bool persistent = params[2] ? true : false;
	pContext->LocalToString(params[1], &conf);
	pContext->LocalToString(params[3], &err);
	
	IDBDriver *driver;
	IDatabase *db;
	if (!g_DBMan.Connect(conf, &driver, &db, persistent, err, maxlength))
	{
		return BAD_HANDLE;
	}

	Handle_t hndl = g_DBMan.CreateHandle(DBHandle_Database, db, pContext->GetIdentity());
	if (!hndl)
	{
		db->Close();
		return BAD_HANDLE;
	}

	/* HACK! Add us to the dependency list */
	IExtension *pExt = g_Extensions.GetExtensionFromIdent(driver->GetIdentity());
	if (pExt)
	{
		g_Extensions.BindChildPlugin(pExt, scripts->FindPluginByContext(pContext->GetContext()));
	}

	return hndl;
}

static cell_t ConnectToDbAsync(IPluginContext *pContext, const cell_t *params, AsyncCallbackMode acm)
{
	IPluginFunction *pf = pContext->GetFunctionById(params[1]);
	if (!pf)
	{
		return pContext->ThrowNativeError("Function id %x is invalid", params[1]);
	}

	char *conf;
	pContext->LocalToString(params[2], &conf);

	IDBDriver *driver = NULL;
	const DatabaseInfo *pInfo = g_DBMan.FindDatabaseConf(conf);
	char error[255];
	if (pInfo != NULL)
	{
		if (pInfo->driver[0] == '\0')
		{
			driver = g_DBMan.GetDefaultDriver();
		} else {
			driver = g_DBMan.FindOrLoadDriver(pInfo->driver);
		}
		if (!driver)
		{
			g_pSM->Format(error, 
				sizeof(error), 
				"Could not find driver \"%s\"", 
				pInfo->driver[0] == '\0' ? g_DBMan.GetDefaultDriverName().c_str() : pInfo->driver);
		} else if (!driver->IsThreadSafe()) {
			g_pSM->Format(error,
				sizeof(error),
				"Driver \"%s\" is not thread safe!",
				driver->GetIdentifier());
		}
	} else {
		g_pSM->Format(error, sizeof(error), "Could not find database conf \"%s\"", conf);
	}

	if (!pInfo || !driver)
	{
		pf->PushCell(BAD_HANDLE);
		if (acm == ACM_Old)
			pf->PushCell(BAD_HANDLE);
		pf->PushString(error);
		pf->PushCell(0);
		pf->Execute(NULL);
		return 0;
	}

	/* HACK! Add us to the dependency list */
	IExtension *pExt = g_Extensions.GetExtensionFromIdent(driver->GetIdentity());
	if (pExt)
	{
		g_Extensions.BindChildPlugin(pExt, scripts->FindPluginByContext(pContext->GetContext()));
	}

	/* Finally, add to the thread if we can */
	TConnectOp *op = new TConnectOp(pf, driver, conf, acm, params[3]);
	IPlugin *pPlugin = scripts->FindPluginByContext(pContext->GetContext());
	if (pPlugin->GetProperty("DisallowDBThreads", NULL)
		|| !g_DBMan.AddToThreadQueue(op, PrioQueue_High))
	{
		/* Do everything right now */
		op->RunThreadPart();
		op->RunThinkPart();
		op->Destroy();
	}

	return 1;
}

static cell_t SQL_TConnect(IPluginContext *pContext, const cell_t *params)
{
	return ConnectToDbAsync(pContext, params, ACM_Old);
}

static cell_t Database_Connect(IPluginContext *pContext, const cell_t *params)
{
	return ConnectToDbAsync(pContext, params, ACM_New);
}

static cell_t SQL_ConnectEx(IPluginContext *pContext, const cell_t *params)
{
	IDBDriver *driver;
	if (params[1] == BAD_HANDLE)
	{
		if ((driver = g_DBMan.GetDefaultDriver()) == NULL)
		{
			return pContext->ThrowNativeError("Could not find any default driver");
		}
	} else {
		HandleError err;
		if ((err = g_DBMan.ReadHandle(params[1], DBHandle_Driver, (void **)&driver))
			!= HandleError_None)
		{
			return pContext->ThrowNativeError("Invalid driver Handle %x (error: %d)", params[1], err);
		}
	}
 
	char *host, *user, *pass, *database, *error;
	size_t maxlength = (size_t)params[7];
	bool persistent = params[8] ? true : false;
	unsigned int port = params[9];
	unsigned int maxTimeout = params[10];
	pContext->LocalToString(params[2], &host);
	pContext->LocalToString(params[3], &user);
	pContext->LocalToString(params[4], &pass);
	pContext->LocalToString(params[5], &database);
	pContext->LocalToString(params[6], &error);

	DatabaseInfo info;
	info.database = database;
	info.driver = driver->GetIdentifier();
	info.host = host;
	info.maxTimeout = maxTimeout;
	info.pass = pass;
	info.port = port;
	info.user = user;
	
	IDatabase *db = driver->Connect(&info, persistent, error, maxlength);

	if (db)
	{
		Handle_t hndl = g_DBMan.CreateHandle(DBHandle_Database, db, pContext->GetIdentity());
		if (!hndl)
		{
			db->Close();
			return BAD_HANDLE;
		}

		/* HACK! Add us to the dependency list */
		IExtension *pExt = g_Extensions.GetExtensionFromIdent(driver->GetIdentity());
		if (pExt)
		{
			g_Extensions.BindChildPlugin(pExt, scripts->FindPluginByContext(pContext->GetContext()));
		}

		return hndl;
	}
	
	return BAD_HANDLE;
}

static cell_t SQL_GetDriverIdent(IPluginContext *pContext, const cell_t *params)
{
	IDBDriver *driver;
	if (params[1] == BAD_HANDLE)
	{
		if ((driver = g_DBMan.GetDefaultDriver()) == NULL)
		{
			return pContext->ThrowNativeError("Could not find any default driver");
		}
	} else {
		HandleError err;
		if ((err = g_DBMan.ReadHandle(params[1], DBHandle_Driver, (void **)&driver))
			!= HandleError_None)
		{
			return pContext->ThrowNativeError("Invalid driver Handle %x (error: %d)", params[1], err);
		}
	}

	pContext->StringToLocalUTF8(params[2], params[3], driver->GetIdentifier(), NULL);

	return 1;
}

static cell_t SQL_GetDriver(IPluginContext *pContext, const cell_t *params)
{
	char *name;
	pContext->LocalToString(params[1], &name);

	IDBDriver *driver = NULL;
	if (name[0] == '\0')
	{
		driver = g_DBMan.GetDefaultDriver();
	} else {
		driver = g_DBMan.FindOrLoadDriver(name);
	}

	return (driver != NULL) ? driver->GetHandle() : BAD_HANDLE;
}

static cell_t SQL_GetDriverProduct(IPluginContext *pContext, const cell_t *params)
{
	IDBDriver *driver;
	if (params[1] == BAD_HANDLE)
	{
		if ((driver = g_DBMan.GetDefaultDriver()) == NULL)
		{
			return pContext->ThrowNativeError("Could not find any default driver");
		}
	} else {
		HandleError err;
		if ((err = g_DBMan.ReadHandle(params[1], DBHandle_Driver, (void **)&driver))
			!= HandleError_None)
		{
			return pContext->ThrowNativeError("Invalid driver Handle %x (error: %d)", params[1], err);
		}
	}

	pContext->StringToLocalUTF8(params[2], params[3], driver->GetProductName(), NULL);

	return 1;
}

static cell_t SQL_GetAffectedRows(IPluginContext *pContext, const cell_t *params)
{
	IDatabase *db = NULL;
	IPreparedQuery *stmt = NULL;
	IQuery *query = NULL;
	HandleError err;

	if (((err = ReadDbOrStmtHndl(params[1], pContext, &db, &stmt)) != HandleError_None)
		&& ((err = ReadQueryAndDbHndl(params[1], pContext, &query, &db)) != HandleError_None))
	{
		return pContext->ThrowNativeError("Invalid statement, db, or query Handle %x (error: %d)", params[1], err);
	}


	if (stmt)
	{
		return stmt->GetAffectedRows();
	}
	else if (query)
	{
		return db->GetAffectedRowsForQuery(query);
	}
	else if (db)
	{
		return db->GetAffectedRows();
	}

	return pContext->ThrowNativeError("Unknown error reading db/stmt/query handles");
}

static cell_t SQL_GetInsertId(IPluginContext *pContext, const cell_t *params)
{
	IDatabase *db = NULL;
	IQuery *query = NULL;
	IPreparedQuery *stmt = NULL;
	HandleError err;

	if (((err = ReadDbOrStmtHndl(params[1], pContext, &db, &stmt)) != HandleError_None)
		&& ((err = ReadQueryAndDbHndl(params[1], pContext, &query, &db)) != HandleError_None))
	{
		return pContext->ThrowNativeError("Invalid statement, db, or query Handle %x (error: %d)", params[1], err);
	}

	if (query)
	{
		return db->GetInsertIDForQuery(query);
	}
	else if (db)
	{
		return db->GetInsertID();
	}
	else if (stmt)
	{
		return stmt->GetInsertID();
	}

	return pContext->ThrowNativeError("Unknown error reading db/stmt/query handles");
}

static cell_t SQL_GetError(IPluginContext *pContext, const cell_t *params)
{
	IDatabase *db = NULL;
	IPreparedQuery *stmt = NULL;
	HandleError err;

	if ((err = ReadDbOrStmtHndl(params[1], pContext, &db, &stmt)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid statement or db Handle %x (error: %d)", params[1], err);
	}

	const char *error = "";
	if (db)
	{
		error = db->GetError();
	} else if (stmt) {
		error = stmt->GetError();
	}

	if (error[0] == '\0')
	{
		return false;
	}

	pContext->StringToLocalUTF8(params[2], params[3], error, NULL);

	return 1;
}

static cell_t SQL_QuoteString(IPluginContext *pContext, const cell_t *params)
{
	IDatabase *db = NULL;
	HandleError err;

	if ((err = g_DBMan.ReadHandle(params[1], DBHandle_Database, (void **)&db))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid database Handle %x (error: %d)", params[1], err);
	}

	char *input, *output;
	size_t maxlength = (size_t)params[4];
	pContext->LocalToString(params[2], &input);
	pContext->LocalToString(params[3], &output);

	size_t written;
	bool s = db->QuoteString(input, output, maxlength, &written);

	cell_t *addr;
	pContext->LocalToPhysAddr(params[5], &addr);
	*addr = (cell_t)written;

	return s ? 1 : 0;
}

static cell_t SQL_FormatQuery(IPluginContext *pContext, const cell_t *params)
{
	IDatabase *db = NULL;
	HandleError err;

	if ((err = g_DBMan.ReadHandle(params[1], DBHandle_Database, (void **)&db))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid database Handle %x (error: %d)", params[1], err);
	}

	g_FormatEscapeDatabase = db;
	cell_t result = InternalFormat(pContext, params, 1);
	g_FormatEscapeDatabase = NULL;

	return result;
}

static cell_t SQL_FastQuery(IPluginContext *pContext, const cell_t *params)
{
	IDatabase *db = NULL;
	HandleError err;

	if ((err = g_DBMan.ReadHandle(params[1], DBHandle_Database, (void **)&db))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid database Handle %x (error: %d)", params[1], err);
	}

	char *query;
	pContext->LocalToString(params[2], &query);

	if (params[0] >= 3 && params[3] != -1)
	{
		return db->DoSimpleQueryEx(query, params[3]) ? 1 : 0;
	}

	return db->DoSimpleQuery(query) ? 1 : 0;
}

static cell_t SQL_Query(IPluginContext *pContext, const cell_t *params)
{
	IDatabase *db = NULL;
	HandleError err;

	if ((err = g_DBMan.ReadHandle(params[1], DBHandle_Database, (void **)&db))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid database Handle %x (error: %d)", params[1], err);
	}

	char *query;
	pContext->LocalToString(params[2], &query);

	IQuery *qr;
	
	if (params[0] >= 3 && params[3] != -1)
	{
		qr = db->DoQueryEx(query, params[3]);
	}
	else
	{
		qr = db->DoQuery(query);
	}

	if (!qr)
	{
		return BAD_HANDLE;
	}

	CombinedQuery *c = new CombinedQuery(qr, db);
	Handle_t hndl = handlesys->CreateHandle(hCombinedQueryType, c, pContext->GetIdentity(), g_pCoreIdent, NULL);
	if (hndl == BAD_HANDLE)
	{
		qr->Destroy();
		delete c;
		return BAD_HANDLE;
	}

	return hndl;
}

static cell_t SQL_TQuery(IPluginContext *pContext, const cell_t *params)
{
	IDatabase *db = NULL;
	HandleError err;

	if ((err = g_DBMan.ReadHandle(params[1], DBHandle_Database, (void **)&db))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid database Handle %x (error: %d)", params[1], err);
	}

	if (!db->GetDriver()->IsThreadSafe())
	{
		return pContext->ThrowNativeError("Driver \"%s\" is not thread safe!", db->GetDriver()->GetIdentifier());
	}

	IPluginFunction *pf = pContext->GetFunctionById(params[2]);
	if (!pf)
	{
		return pContext->ThrowNativeError("Function id %x is invalid", params[2]);
	}

	char *query;
	pContext->LocalToString(params[3], &query);

	cell_t data = params[4];
	PrioQueueLevel level = PrioQueue_Normal;
	if (params[5] == (cell_t)PrioQueue_High)
	{
		level = PrioQueue_High;
	} else if (params[5] == (cell_t)PrioQueue_Low) {
		level = PrioQueue_Low;
	}

	IPlugin *pPlugin = scripts->FindPluginByContext(pContext->GetContext());

	TQueryOp *op = new TQueryOp(db, pf, query, data);
	if (pPlugin->GetProperty("DisallowDBThreads", NULL)
		|| !g_DBMan.AddToThreadQueue(op, level))
	{
		/* Do everything right now */
		op->RunThreadPart();
		op->RunThinkPart();
		op->Destroy();
	}

	return 1;
}

static cell_t SQL_LockDatabase(IPluginContext *pContext, const cell_t *params)
{
	IDatabase *db = NULL;
	HandleError err;

	if ((err = g_DBMan.ReadHandle(params[1], DBHandle_Database, (void **)&db))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid database Handle %x (error: %d)", params[1], err);
	}

	db->LockForFullAtomicOperation();

	return 1;
}

static cell_t SQL_UnlockDatabase(IPluginContext *pContext, const cell_t *params)
{
	IDatabase *db = NULL;
	HandleError err;

	if ((err = g_DBMan.ReadHandle(params[1], DBHandle_Database, (void **)&db))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid database Handle %x (error: %d)", params[1], err);
	}

	db->UnlockFromFullAtomicOperation();

	return 1;
}

static cell_t SQL_PrepareQuery(IPluginContext *pContext, const cell_t *params)
{
	IDatabase *db = NULL;
	HandleError err;

	if ((err = g_DBMan.ReadHandle(params[1], DBHandle_Database, (void **)&db))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid database Handle %x (error: %d)", params[1], err);
	}

	char *query, *error;
	size_t maxlength = (size_t)params[4];
	pContext->LocalToString(params[2], &query);
	pContext->LocalToString(params[3], &error);

	IPreparedQuery *qr = db->PrepareQuery(query, error, maxlength);

	if (!qr)
	{
		return BAD_HANDLE;
	}

	Handle_t hndl = handlesys->CreateHandle(hStmtType, qr, pContext->GetIdentity(), g_pCoreIdent, NULL);
	if (hndl == BAD_HANDLE)
	{
		qr->Destroy();
		return BAD_HANDLE;
	}

	return hndl;
}

static cell_t SQL_FetchMoreResults(IPluginContext *pContext, const cell_t *params)
{
	IQuery *query;
	HandleError err;

	if ((err = ReadQueryHndl(params[1], pContext, &query)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid query Handle %x (error: %d)", params[1], err);
	}

	return query->FetchMoreResults() ? 1 : 0;
}

static cell_t SQL_HasResultSet(IPluginContext *pContext, const cell_t *params)
{
	IQuery *query;
	HandleError err;

	if ((err = ReadQueryHndl(params[1], pContext, &query)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid query Handle %x (error: %d)", params[1], err);
	}

	return query->GetResultSet() != NULL ? true : false;
}

static cell_t SQL_GetRowCount(IPluginContext *pContext, const cell_t *params)
{
	IQuery *query;
	HandleError err;

	if ((err = ReadQueryHndl(params[1], pContext, &query)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid query Handle %x (error: %d)", params[1], err);
	}

	IResultSet *rs = query->GetResultSet();
	if (!rs)
	{
		return 0;
	}

	return rs->GetRowCount();
}

static cell_t SQL_GetFieldCount(IPluginContext *pContext, const cell_t *params)
{
	IQuery *query;
	HandleError err;

	if ((err = ReadQueryHndl(params[1], pContext, &query)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid query Handle %x (error: %d)", params[1], err);
	}

	IResultSet *rs = query->GetResultSet();
	if (!rs)
	{
		return 0;
	}

	return rs->GetFieldCount();
}

static cell_t SQL_FieldNumToName(IPluginContext *pContext, const cell_t *params)
{
	IQuery *query;
	HandleError err;

	if ((err = ReadQueryHndl(params[1], pContext, &query)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid query Handle %x (error: %d)", params[1], err);
	}

	IResultSet *rs = query->GetResultSet();
	if (!rs)
	{
		return pContext->ThrowNativeError("No current result set");
	}

	unsigned int field = params[2];

	const char *fldname;
	if ((fldname = rs->FieldNumToName(field)) == NULL)
	{
		return pContext->ThrowNativeError("Invalid field index %d", field);
	}

	pContext->StringToLocalUTF8(params[3], params[4], fldname, NULL);

	return 1;
}

static cell_t SQL_FieldNameToNum(IPluginContext *pContext, const cell_t *params)
{
	IQuery *query;
	HandleError err;

	if ((err = ReadQueryHndl(params[1], pContext, &query)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid query Handle %x (error: %d)", params[1], err);
	}

	IResultSet *rs = query->GetResultSet();
	if (!rs)
	{
		return pContext->ThrowNativeError("No current result set");
	}

	char *field;
	pContext->LocalToString(params[2], &field);

	cell_t *num;
	pContext->LocalToPhysAddr(params[3], &num);

	return rs->FieldNameToNum(field, (unsigned int *)num) ? 1 : 0;
}

static cell_t SQL_FetchRow(IPluginContext *pContext, const cell_t *params)
{
	IQuery *query;
	HandleError err;

	if ((err = ReadQueryHndl(params[1], pContext, &query)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid query Handle %x (error: %d)", params[1], err);
	}

	IResultSet *rs = query->GetResultSet();
	if (!rs)
	{
		return pContext->ThrowNativeError("No current result set");
	}

	return (rs->FetchRow() != NULL) ? true : false;
}

static cell_t SQL_MoreRows(IPluginContext *pContext, const cell_t *params)
{
	IQuery *query;
	HandleError err;

	if ((err = ReadQueryHndl(params[1], pContext, &query)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid query Handle %x (error: %d)", params[1], err);
	}

	IResultSet *rs = query->GetResultSet();
	if (!rs)
	{
		return pContext->ThrowNativeError("No current result set");
	}

	return rs->MoreRows() ? 1 : 0;
}

static cell_t SQL_Rewind(IPluginContext *pContext, const cell_t *params)
{
	IQuery *query;
	HandleError err;

	if ((err = ReadQueryHndl(params[1], pContext, &query)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid query Handle %x (error: %d)", params[1], err);
	}

	IResultSet *rs = query->GetResultSet();
	if (!rs)
	{
		return pContext->ThrowNativeError("No current result set");
	}

	return rs->Rewind() ? 1 : 0;
}

static cell_t SQL_FetchString(IPluginContext *pContext, const cell_t *params)
{
	IQuery *query;
	HandleError err;

	if ((err = ReadQueryHndl(params[1], pContext, &query)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid query Handle %x (error: %d)", params[1], err);
	}

	IResultSet *rs = query->GetResultSet();
	if (!rs)
	{
		return pContext->ThrowNativeError("No current result set");
	}

	IResultRow *row = rs->CurrentRow();
	if (!row)
	{
		return pContext->ThrowNativeError("Current result set has no fetched rows");
	}

	const char *str;
	size_t length;
	DBResult res = row->GetString(params[2], &str, &length);

	if (res == DBVal_Error)
	{
		return pContext->ThrowNativeError("Error fetching data from field %d", params[2]);
	} else if (res == DBVal_TypeMismatch) {
		return pContext->ThrowNativeError("Could not fetch data in field %d as a string", params[2]);
	}

	pContext->StringToLocalUTF8(params[3], params[4], str, &length);

	cell_t *addr;
	pContext->LocalToPhysAddr(params[5], &addr);
	*addr = (cell_t)res;

	return (cell_t)length;
}

static cell_t SQL_FetchFloat(IPluginContext *pContext, const cell_t *params)
{
	IQuery *query;
	HandleError err;

	if ((err = ReadQueryHndl(params[1], pContext, &query)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid query Handle %x (error: %d)", params[1], err);
	}

	IResultSet *rs = query->GetResultSet();
	if (!rs)
	{
		return pContext->ThrowNativeError("No current result set");
	}

	IResultRow *row = rs->CurrentRow();
	if (!row)
	{
		return pContext->ThrowNativeError("Current result set has no fetched rows");
	}

	float f;
	DBResult res = row->GetFloat(params[2], &f);

	if (res == DBVal_Error)
	{
		return pContext->ThrowNativeError("Error fetching data from field %d", params[2]);
	} else if (res == DBVal_TypeMismatch) {
		return pContext->ThrowNativeError("Could not fetch data in field %d as a float", params[2]);
	}

	cell_t *addr;
	pContext->LocalToPhysAddr(params[3], &addr);
	*addr = (cell_t)res;

	return sp_ftoc(f);
}

static cell_t SQL_FetchInt(IPluginContext *pContext, const cell_t *params)
{
	IQuery *query;
	HandleError err;

	if ((err = ReadQueryHndl(params[1], pContext, &query)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid query Handle %x (error: %d)", params[1], err);
	}

	IResultSet *rs = query->GetResultSet();
	if (!rs)
	{
		return pContext->ThrowNativeError("No current result set");
	}

	IResultRow *row = rs->CurrentRow();
	if (!row)
	{
		return pContext->ThrowNativeError("Current result set has no fetched rows");
	}

	int iv;
	DBResult res = row->GetInt(params[2], &iv);

	if (res == DBVal_Error)
	{
		return pContext->ThrowNativeError("Error fetching data from field %d", params[2]);
	} else if (res == DBVal_TypeMismatch) {
		return pContext->ThrowNativeError("Could not fetch data in field %d as an integer", params[2]);
	}

	cell_t *addr;
	pContext->LocalToPhysAddr(params[3], &addr);
	*addr = (cell_t)res;

	return iv;
}

static cell_t SQL_IsFieldNull(IPluginContext *pContext, const cell_t *params)
{
	IQuery *query;
	HandleError err;

	if ((err = ReadQueryHndl(params[1], pContext, &query)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid query Handle %x (error: %d)", params[1], err);
	}

	IResultSet *rs = query->GetResultSet();
	if (!rs)
	{
		return pContext->ThrowNativeError("No current result set");
	}

	IResultRow *row = rs->CurrentRow();
	if (!row)
	{
		return pContext->ThrowNativeError("Current result set has no fetched rows");
	}

	if ((unsigned)params[2] >= rs->GetFieldCount())
	{
		return pContext->ThrowNativeError("Invalid field index %d", params[2]);
	}

	return row->IsNull(params[2]) ? 1 : 0;
}

static cell_t SQL_FetchSize(IPluginContext *pContext, const cell_t *params)
{
	IQuery *query;
	HandleError err;

	if ((err = ReadQueryHndl(params[1], pContext, &query)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid query Handle %x (error: %d)", params[1], err);
	}

	IResultSet *rs = query->GetResultSet();
	if (!rs)
	{
		return pContext->ThrowNativeError("No current result set");
	}

	IResultRow *row = rs->CurrentRow();
	if (!row)
	{
		return pContext->ThrowNativeError("Current result set has no fetched rows");
	}

	if ((unsigned)params[2] >= rs->GetFieldCount())
	{
		return pContext->ThrowNativeError("Invalid field index %d", params[2]);
	}

	return row->GetDataSize(params[2]);
}

static cell_t SQL_BindParamInt(IPluginContext *pContext, const cell_t *params)
{
	IPreparedQuery *stmt;
	HandleError err;

	if ((err = ReadStmtHndl(params[1], pContext, &stmt)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid statement Handle %x (error: %d)", params[1], err);
	}

	if (!stmt->BindParamInt(params[2], params[3], params[4] ? true : false))
	{
		return pContext->ThrowNativeError("Could not bind parameter %d as an integer", params[2]);
	}

	return 1;
}

static cell_t SQL_BindParamFloat(IPluginContext *pContext, const cell_t *params)
{
	IPreparedQuery *stmt;
	HandleError err;

	if ((err = ReadStmtHndl(params[1], pContext, &stmt)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid statement Handle %x (error: %d)", params[1], err);
	}

	if (!stmt->BindParamFloat(params[2], sp_ctof(params[3])))
	{
		return pContext->ThrowNativeError("Could not bind parameter %d as a float", params[2]);
	}

	return 1;
}

static cell_t SQL_BindParamString(IPluginContext *pContext, const cell_t *params)
{
	IPreparedQuery *stmt;
	HandleError err;

	if ((err = ReadStmtHndl(params[1], pContext, &stmt)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid statement Handle %x (error: %d)", params[1], err);
	}

	char *str;
	pContext->LocalToString(params[3], &str);

	if (!stmt->BindParamString(params[2], str, params[4] ? true : false))
	{
		return pContext->ThrowNativeError("Could not bind parameter %d as a string", params[2]);
	}

	return 1;
}

static cell_t SQL_Execute(IPluginContext *pContext, const cell_t *params)
{
	IPreparedQuery *stmt;
	HandleError err;

	if ((err = ReadStmtHndl(params[1], pContext, &stmt)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid statement Handle %x (error: %d)", params[1], err);
	}

	return stmt->Execute() ? 1 : 0;
}

static cell_t SQL_IsSameConnection(IPluginContext *pContext, const cell_t *params)
{
	IDatabase *db1=NULL, *db2=NULL;
	HandleError err;

	if ((err = g_DBMan.ReadHandle(params[1], DBHandle_Database, (void **)&db1))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid database Handle 1/%x (error: %d)", params[1], err);
	}

	if ((err = g_DBMan.ReadHandle(params[2], DBHandle_Database, (void **)&db2))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid database Handle 2/%x (error: %d)", params[2], err);
	}

	return (db1 == db2) ? true : false;
}

static cell_t SQL_ReadDriver(IPluginContext *pContext, const cell_t *params)
{
	IDatabase *db1=NULL;
	HandleError err;

	if ((err = g_DBMan.ReadHandle(params[1], DBHandle_Database, (void **)&db1))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid database Handle 1/%x (error: %d)", params[1], err);
	}

	IDBDriver *driver = db1->GetDriver();

	pContext->StringToLocalUTF8(params[2], params[3], driver->GetIdentifier(), NULL);

	return driver->GetHandle();
}

static cell_t Database_Driver_get(IPluginContext *pContext, const cell_t *params)
{
	IDatabase *db1=NULL;
	HandleError err;

	if ((err = g_DBMan.ReadHandle(params[1], DBHandle_Database, (void **)&db1))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid database Handle 1/%x (error: %d)", params[1], err);
	}

	return db1->GetDriver()->GetHandle();
}

static cell_t SQL_CheckConfig(IPluginContext *pContext, const cell_t *params)
{
	char *name;
	pContext->LocalToString(params[1], &name);

	return (g_DBMan.GetDatabaseConf(name) != nullptr) ? 1 : 0;
}

static cell_t SQL_ConnectCustom(IPluginContext *pContext, const cell_t *params)
{
	KeyValues *kv;
	HandleError err;

	kv = g_pSM->ReadKeyValuesHandle(params[1], &err, false);
	if (kv == NULL)
	{
		return pContext->ThrowNativeError("Invalid KeyValues handle %x (error: %d)",
										  params[1],
										  err);
	}

	DatabaseInfo info;
	bridge->GetDBInfoFromKeyValues(kv, &info);

	IDBDriver *driver;
	if (info.driver[0] == '\0' || strcmp(info.driver, "default") == 0)
	{
		driver = g_DBMan.GetDefaultDriver();
	}
	else
	{
		driver = g_DBMan.FindOrLoadDriver(info.driver);
	}

	if (driver == NULL)
	{
		char buffer[255];

		g_pSM->Format(buffer, sizeof(buffer), "Could not find driver \"%s\"", info.driver);
		pContext->StringToLocalUTF8(params[2], params[3], buffer, NULL);

		return BAD_HANDLE;
	}

	char *buffer;
	IDatabase *db;

	pContext->LocalToString(params[2], &buffer);
	
	db = driver->Connect(&info, params[4] ? true : false, buffer, params[3]);
	if (db == NULL)
	{
		return BAD_HANDLE;
	}

	Handle_t hndl = g_DBMan.CreateHandle(DBHandle_Database, db, pContext->GetIdentity());
	if (!hndl)
	{
		db->Close();
		return pContext->ThrowNativeError("Out of handles!");
	}

	/* HACK! Add us to the dependency list */
	IExtension *pExt = g_Extensions.GetExtensionFromIdent(driver->GetIdentity());
	if (pExt)
	{
		g_Extensions.BindChildPlugin(pExt, scripts->FindPluginByContext(pContext->GetContext()));
	}

	return hndl;
}

static cell_t SQL_SetCharset(IPluginContext *pContext, const cell_t *params)
{
	IDatabase *db = NULL;
	HandleError err;

	if ((err = g_DBMan.ReadHandle(params[1], DBHandle_Database, (void **)&db))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid database Handle %x (error: %d)", params[1], err);
	}

	char *characterset;
	pContext->LocalToString(params[2], &characterset);

	return db->SetCharacterSet(characterset);
}

static cell_t SQL_CreateTransaction(IPluginContext *pContext, const cell_t *params)
{
	Transaction *txn = new Transaction();
	Handle_t handle = handlesys->CreateHandle(hTransactionType, txn, pContext->GetIdentity(), g_pCoreIdent, NULL);
	if (!handle)
	{
		delete txn;
		return BAD_HANDLE;
	}

	return handle;
}

static cell_t SQL_AddQuery(IPluginContext *pContext, const cell_t *params)
{
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

	Transaction *txn;
	Handle_t handle = params[1];
	HandleError err = handlesys->ReadHandle(handle, hTransactionType, &sec, (void **)&txn);
	if (err != HandleError_None)
		return pContext->ThrowNativeError("Invalid handle %x (error %d)", handle, err);

	char *query;
	pContext->LocalToString(params[2], &query);

	Transaction::Entry entry;
	entry.query = query;
	entry.data = params[3];
	txn->entries.push_back(std::move(entry));

	return cell_t(txn->entries.size() - 1);
}

class TTransactOp : public IDBThreadOperation
{
public:
	TTransactOp(IDatabase *db, Transaction *txn, Handle_t txnHandle, IdentityToken_t *ident,
                IPluginFunction *onSuccess, IPluginFunction *onError, cell_t data)
	: 
		db_(db),
		txn_(txn),
		ident_(ident),
		success_(onSuccess),
		failure_(onError),
		data_(data),
		autoHandle_(txnHandle),
		failIndex_(-1)
	{
	}

	~TTransactOp()
	{
		for (size_t i = 0; i < results_.size(); i++)
			results_[i]->Destroy();
		results_.clear();
	}

	IdentityToken_t *GetOwner()
	{
		return ident_;
	}
	IDBDriver *GetDriver()
	{
		return db_->GetDriver();
	}
	void Destroy()
	{
		delete this;
	}

private:
	bool Succeeded() const
	{
		return error_.size() == 0;
	}

	void SetDbError()
	{
		const char *error = db_->GetError();
		if (!error || strlen(error) == 0)
			error_ = "unknown error";
		else
			error_ = error;
	}

	IQuery *Exec(const char *query)
	{
		IQuery *result = db_->DoQuery(query);
		if (!result)
		{
			SetDbError();
			db_->DoSimpleQuery("ROLLBACK");
			return NULL;
		}
		return result;
	}

	void ExecuteTransaction()
	{
		if (!db_->DoSimpleQuery("BEGIN"))
		{
			SetDbError();
			return;
		}

		for (size_t i = 0; i < txn_->entries.size(); i++)
		{
			Transaction::Entry &entry = txn_->entries[i];
			IQuery *result = Exec(entry.query.c_str());
			if (!result)
			{
				failIndex_ = (cell_t)i;
				return;
			}
			results_.push_back(result);
		}

		if (!db_->DoSimpleQuery("COMMIT"))
		{
			SetDbError();
			db_->DoSimpleQuery("ROLLBACK");
		}
	}

public:
	void RunThreadPart()
	{
		db_->LockForFullAtomicOperation();
		ExecuteTransaction();
		db_->UnlockFromFullAtomicOperation();
	}

	void CancelThinkPart()
	{
		if (Succeeded())
			error_ = "Driver is unloading";
		RunThinkPart();
	}

private:
	bool CallSuccess()
	{
		HandleSecurity sec(ident_, g_pCoreIdent);

		// Allocate all the handles for calling the success callback.
		Handle_t dbh = CreateLocalHandle(g_DBMan.GetDatabaseType(), db_, &sec);
		if (dbh == BAD_HANDLE)
		{
			error_ = "unable to allocate handle";
			return false;
		}

		// Add an extra refcount for the handle.
		db_->AddRef();

		assert(results_.size() == txn_->entries.size());

		std::unique_ptr<cell_t[]> data = std::make_unique<cell_t[]>(results_.size());
		std::unique_ptr<cell_t[]> handles = std::make_unique<cell_t[]>(results_.size());
		for (size_t i = 0; i < results_.size(); i++)
		{
			CombinedQuery *obj = new CombinedQuery(results_[i], db_);
			Handle_t rh = CreateLocalHandle(hCombinedQueryType, obj, &sec);
			if (rh == BAD_HANDLE)
			{
				// Messy - free handles up to what we've allocated, and then
				// manually destroy any remaining result sets.
				delete obj;
				for (size_t iter = 0; iter < i; iter++)
					handlesys->FreeHandle(handles[iter], &sec);
				for (size_t iter = i; iter < results_.size(); iter++)
					results_[iter]->Destroy();
				handlesys->FreeHandle(dbh, &sec);
				results_.clear();

				error_ = "unable to allocate handle";
				return false;
			}
			handles[i] = rh;
			data[i] = txn_->entries[i].data;
		}

		if (success_->IsRunnable())
		{
			success_->PushCell(dbh);
			success_->PushCell(data_);
			success_->PushCell(txn_->entries.size());
			success_->PushArray(handles.get(), results_.size());
			success_->PushArray(data.get(), results_.size());
			success_->Execute(NULL);
		}

		// Cleanup. Note we clear results_, since freeing their handles will
		// call Destroy(), and we don't want to double-free in ~TTransactOp.
		for (size_t i = 0; i < results_.size(); i++)
			handlesys->FreeHandle(handles[i], &sec);
		handlesys->FreeHandle(dbh, &sec);
		results_.clear();

		return true;
	}

public:
	void RunThinkPart()
	{
		if (!success_ && !failure_)
			return;

		if (Succeeded() && success_)
		{
			if (CallSuccess())
				return;
		}

		if (!Succeeded() && failure_)
		{
			HandleSecurity sec(ident_, g_pCoreIdent);

			std::unique_ptr<cell_t[]> data = std::make_unique<cell_t[]>(txn_->entries.size());
			for (size_t i = 0; i < txn_->entries.size(); i++)
				data[i] = txn_->entries[i].data;

			Handle_t dbh = CreateLocalHandle(g_DBMan.GetDatabaseType(), db_, &sec);
			if (dbh != BAD_HANDLE)
			{
				// Add an extra refcount for the handle.
				db_->AddRef();
			}

			if (failure_->IsRunnable())
			{
				failure_->PushCell(dbh);
				failure_->PushCell(data_);
				failure_->PushCell(txn_->entries.size());
				failure_->PushString(error_.c_str());
				failure_->PushCell(failIndex_);
				failure_->PushArray(data.get(), txn_->entries.size());
				failure_->Execute(NULL);
			}

			handlesys->FreeHandle(dbh, &sec);
		}
	}

private:
	ke::RefPtr<IDatabase> db_;
	Transaction *txn_;
	IdentityToken_t *ident_;
	IPluginFunction *success_;
	IPluginFunction *failure_;
	cell_t data_;
	AutoHandleRooter autoHandle_;
	std::string error_;
	std::vector<IQuery *> results_;
	cell_t failIndex_;
};

static cell_t SQL_ExecuteTransaction(IPluginContext *pContext, const cell_t *params)
{
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

	IDatabase *db = NULL;
	HandleError err = g_DBMan.ReadHandle(params[1], DBHandle_Database, (void **)&db);
	if (err != HandleError_None)
		return pContext->ThrowNativeError("Invalid database handle %x (error: %d)", params[1], err);

	Transaction *txn;
	if ((err = handlesys->ReadHandle(params[2], hTransactionType, &sec, (void **)&txn)) != HandleError_None)
		return pContext->ThrowNativeError("Invalid transaction handle %x (error %d)", params[2], err);

	if (!db->GetDriver()->IsThreadSafe())
		return pContext->ThrowNativeError("Driver \"%s\" is not thread safe!", db->GetDriver()->GetIdentifier());

	IPluginFunction *onSuccess = NULL;
	IPluginFunction *onError = NULL;
	if (params[3] != -1 && ((onSuccess = pContext->GetFunctionById(params[3])) == NULL))
		return pContext->ThrowNativeError("Function id %x is invalid", params[3]);
	if (params[4] != -1 && ((onError = pContext->GetFunctionById(params[4])) == NULL))
		return pContext->ThrowNativeError("Function id %x is invalid", params[4]);

	cell_t data = params[5];
	PrioQueueLevel priority = PrioQueue_Normal;
	if (params[6] == (cell_t)PrioQueue_High)
		priority = PrioQueue_High;
	else if (params[6] == (cell_t)PrioQueue_Low)
		priority = PrioQueue_Low;

	TTransactOp *op = new TTransactOp(db, txn, params[2], pContext->GetIdentity(), onSuccess, onError, data);

	// The handle owns the underlying Transaction object, but we want to close
	// the plugin's view both to ensure reliable access for us and to prevent
	// further tampering on the main thread. To do this, TTransactOp clones the
	// transaction handle and automatically closes it. Therefore, it's safe to
	// close the plugin's handle here.
	handlesys->FreeHandle(params[2], &sec);

	IPlugin *pPlugin = scripts->FindPluginByContext(pContext->GetContext());
	if (pPlugin->GetProperty("DisallowDBThreads", NULL) || !g_DBMan.AddToThreadQueue(op, priority))
	{
		// Do everything right now.
		op->RunThreadPart();
		op->RunThinkPart();
		op->Destroy();
	}

	return 0;
}

REGISTER_NATIVES(dbNatives)
{
	// Transitional syntax support.
	{"DBDriver.Find",			SQL_GetDriver},
	{"DBDriver.GetIdentifier",	SQL_GetDriverIdent},
	{"DBDriver.GetProduct",		SQL_GetDriverProduct},

	{"DBResultSet.FetchMoreResults",	SQL_FetchMoreResults},
	{"DBResultSet.HasResults.get",		SQL_HasResultSet},
	{"DBResultSet.RowCount.get",		SQL_GetRowCount},
	{"DBResultSet.FieldCount.get",		SQL_GetFieldCount},
	{"DBResultSet.AffectedRows.get",	SQL_GetAffectedRows},
	{"DBResultSet.InsertId.get",		SQL_GetInsertId},
	{"DBResultSet.FieldNameToNum",		SQL_FieldNameToNum},
	{"DBResultSet.FieldNumToName",		SQL_FieldNumToName},
	{"DBResultSet.FetchRow",            SQL_FetchRow},
	{"DBResultSet.MoreRows.get",		SQL_MoreRows},
	{"DBResultSet.Rewind",				SQL_Rewind},
	{"DBResultSet.FetchString",			SQL_FetchString},
	{"DBResultSet.FetchFloat",			SQL_FetchFloat},
	{"DBResultSet.FetchInt",			SQL_FetchInt},
	{"DBResultSet.IsFieldNull",			SQL_IsFieldNull},
	{"DBResultSet.FetchSize",			SQL_FetchSize},

	{"Transaction.Transaction",			SQL_CreateTransaction},
	{"Transaction.AddQuery",			SQL_AddQuery},

	{"DBStatement.BindInt",				SQL_BindParamInt},
	{"DBStatement.BindFloat",			SQL_BindParamFloat},
	{"DBStatement.BindString",			SQL_BindParamString},

	{"Database.Connect",				Database_Connect},
	{"Database.Driver.get",				Database_Driver_get},
	{"Database.SetCharset",				SQL_SetCharset},
	{"Database.Escape",					SQL_QuoteString},
	{"Database.Format",					SQL_FormatQuery},
	{"Database.IsSameConnection",		SQL_IsSameConnection},
	{"Database.Execute",				SQL_ExecuteTransaction},

	// Note: The callback is ABI compatible so we can re-use the native.
	{"Database.Query",					SQL_TQuery},

	{"SQL_BindParamInt",		SQL_BindParamInt},
	{"SQL_BindParamFloat",		SQL_BindParamFloat},
	{"SQL_BindParamString",		SQL_BindParamString},
	{"SQL_CheckConfig",			SQL_CheckConfig},
	{"SQL_Connect",				SQL_Connect},
	{"SQL_ConnectEx",			SQL_ConnectEx},
	{"SQL_EscapeString",		SQL_QuoteString},
	{"SQL_FormatQuery",			SQL_FormatQuery},
	{"SQL_Execute",				SQL_Execute},
	{"SQL_FastQuery",			SQL_FastQuery},
	{"SQL_FetchFloat",			SQL_FetchFloat},
	{"SQL_FetchInt",			SQL_FetchInt},
	{"SQL_FetchMoreResults",	SQL_FetchMoreResults},
	{"SQL_FetchRow",			SQL_FetchRow},
	{"SQL_FetchSize",			SQL_FetchSize},
	{"SQL_FetchString",			SQL_FetchString},
	{"SQL_FieldNameToNum",		SQL_FieldNameToNum},
	{"SQL_FieldNumToName",		SQL_FieldNumToName},
	{"SQL_GetAffectedRows",		SQL_GetAffectedRows},
	{"SQL_GetDriver",			SQL_GetDriver},
	{"SQL_GetDriverIdent",		SQL_GetDriverIdent},
	{"SQL_GetDriverProduct",	SQL_GetDriverProduct},
	{"SQL_GetError",			SQL_GetError},
	{"SQL_GetFieldCount",		SQL_GetFieldCount},
	{"SQL_GetInsertId",			SQL_GetInsertId},
	{"SQL_GetRowCount",			SQL_GetRowCount},
	{"SQL_HasResultSet",		SQL_HasResultSet},
	{"SQL_IsFieldNull",			SQL_IsFieldNull},
	{"SQL_IsSameConnection",	SQL_IsSameConnection},
	{"SQL_LockDatabase",		SQL_LockDatabase},
	{"SQL_MoreRows",			SQL_MoreRows},
	{"SQL_PrepareQuery",		SQL_PrepareQuery},
	{"SQL_Query",				SQL_Query},
	{"SQL_QuoteString",			SQL_QuoteString},
	{"SQL_ReadDriver",			SQL_ReadDriver},
	{"SQL_Rewind",				SQL_Rewind},
	{"SQL_TConnect",			SQL_TConnect},
	{"SQL_TQuery",				SQL_TQuery},
	{"SQL_UnlockDatabase",		SQL_UnlockDatabase},
	{"SQL_ConnectCustom",		SQL_ConnectCustom},
	{"SQL_SetCharset",			SQL_SetCharset},
	{"SQL_CreateTransaction",	SQL_CreateTransaction},
	{"SQL_AddQuery",			SQL_AddQuery},
	{"SQL_ExecuteTransaction",	SQL_ExecuteTransaction},
	{NULL,						NULL},
};

