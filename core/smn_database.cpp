/**
 * vim: set ts=4 :
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

#include "sm_globals.h"
#include "HandleSys.h"
#include "Database.h"
#include "ExtensionSys.h"
#include "PluginSys.h"
#include "sm_stringutil.h"

HandleType_t hQueryType;
HandleType_t hStmtType;

class DatabaseHelpers : 
	public SMGlobalClass,
	public IHandleTypeDispatch
{
public:
	virtual void OnSourceModAllInitialized()
	{
		HandleAccess acc;

		/* Disable cloning */
		g_HandleSys.InitAccessDefaults(NULL, &acc);
		acc.access[HandleAccess_Clone] = HANDLE_RESTRICT_OWNER|HANDLE_RESTRICT_IDENTITY;

		TypeAccess tacc;

		g_HandleSys.InitAccessDefaults(&tacc, NULL);
		tacc.ident = g_pCoreIdent;

		hQueryType = g_HandleSys.CreateType("IQuery", this, 0, &tacc, &acc, g_pCoreIdent, NULL);
		hStmtType = g_HandleSys.CreateType("IPreparedQuery", this, hQueryType, &tacc, &acc, g_pCoreIdent, NULL);
	}

	virtual void OnSourceModShutdown()
	{
		g_HandleSys.RemoveType(hStmtType, g_pCoreIdent);
		g_HandleSys.RemoveType(hQueryType, g_pCoreIdent);
	}

	virtual void OnHandleDestroy(HandleType_t type, void *object)
	{
		if (type == hQueryType)
		{
			IQuery *query = (IQuery *)object;
			query->Destroy();
		} else if (type == hStmtType) {
			IPreparedQuery *query = (IPreparedQuery *)object;
			query->Destroy();
		}
	}
} s_DatabaseNativeHelpers;

//is this safe for stmt handles? i think since it's single inheritance, it always will be.
inline HandleError ReadQueryHndl(Handle_t hndl, IPluginContext *pContext, IQuery **query)
{
	HandleSecurity sec;
	sec.pOwner = pContext->GetIdentity();
	sec.pIdentity = g_pCoreIdent;

	return g_HandleSys.ReadHandle(hndl, hQueryType, &sec, (void **)query);
}

inline HandleError ReadStmtHndl(Handle_t hndl, IPluginContext *pContext, IPreparedQuery **query)
{
	HandleSecurity sec;
	sec.pOwner = pContext->GetIdentity();
	sec.pIdentity = g_pCoreIdent;

	return g_HandleSys.ReadHandle(hndl, hStmtType, &sec, (void **)query);
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
	  me(g_PluginSys.GetPluginByCtx(pf->GetParentContext()->GetContext())),
	  m_pQuery(NULL)
	{
		/* We always increase the reference count because this is potentially
		 * asynchronous.  Otherwise the original handle could be closed while 
		 * we're still latched onto it.
		 */
		m_pDatabase->IncReferenceCount();

		/* Now create our own Handle such that it can only be closed by us.
		 * We allow cloning just in case someone wants to hold onto it.
		 */
		HandleSecurity sec(me->GetIdentity(), g_pCoreIdent);
		HandleAccess access;
		g_HandleSys.InitAccessDefaults(NULL, &access);
		access.access[HandleAccess_Delete] = HANDLE_RESTRICT_IDENTITY|HANDLE_RESTRICT_OWNER;
		m_MyHandle = g_HandleSys.CreateHandleEx(g_DBMan.GetDatabaseType(),
			db,
			&sec,
			&access,
			NULL);
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
			g_HandleSys.FreeHandle(m_MyHandle, &sec);
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
			UTIL_Format(error, sizeof(error), "%s", m_pDatabase->GetError());
		}
		m_pDatabase->UnlockFromFullAtomicOperation();
	}
	void CancelThinkPart()
	{
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
		HandleAccess access;
		g_HandleSys.InitAccessDefaults(NULL, &access);
		access.access[HandleAccess_Delete] = HANDLE_RESTRICT_IDENTITY|HANDLE_RESTRICT_OWNER;

		Handle_t qh = BAD_HANDLE;
		
		if (m_pQuery)
		{
			qh = g_HandleSys.CreateHandle(hQueryType, m_pQuery, me->GetIdentity(), g_pCoreIdent, NULL);
			if (qh != BAD_HANDLE)
			{
				m_pQuery = NULL;
			} else {
				UTIL_Format(error, sizeof(error), "Could not alloc handle");
			}
		}

		m_pFunction->PushCell(m_MyHandle);
		m_pFunction->PushCell(qh);
		m_pFunction->PushString(qh == BAD_HANDLE ? error : "");
		m_pFunction->PushCell(m_Data);
		m_pFunction->Execute(NULL);

		if (qh != BAD_HANDLE)
		{
			g_HandleSys.FreeHandle(qh, &sec);
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
	CPlugin *me;
	IQuery *m_pQuery;
	char error[255];
	Handle_t m_MyHandle;
};

class TConnectOp : public IDBThreadOperation
{
public:
	TConnectOp(IPluginFunction *func, IDBDriver *driver, const char *_dbname, cell_t data)
	{
		m_pFunction = func;
		m_pDriver = driver;
		m_pDatabase = NULL;
		m_Data = data;
		error[0] = '\0';
		strncopy(dbname, _dbname, sizeof(dbname));
		me = g_PluginSys.GetPluginByCtx(m_pFunction->GetParentContext()->GetContext());
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
		g_DBMan.LockConfig();
		const DatabaseInfo *pInfo = g_DBMan.FindDatabaseConf(dbname);
		if (!pInfo)
		{
			UTIL_Format(error, sizeof(error), "Could not find database config \"%s\"", dbname);
		} else {
			m_pDatabase = m_pDriver->Connect(pInfo, false, error, sizeof(error));
		}
		g_DBMan.UnlockConfig();
	}
	void CancelThinkPart()
	{
		if (m_pDatabase)
		{
			m_pDatabase->Close();
		}
		m_pFunction->PushCell(BAD_HANDLE);
		m_pFunction->PushCell(BAD_HANDLE);
		m_pFunction->PushString("Driver is unloading");
		m_pFunction->PushCell(m_Data);
		m_pFunction->Execute(NULL);
	}
	void RunThinkPart()
	{
		Handle_t hndl = BAD_HANDLE;
		
		if (m_pDatabase)
		{
			if ((hndl = g_DBMan.CreateHandle(DBHandle_Database, m_pDatabase, me->GetIdentity()))
				== BAD_HANDLE)
			{
				m_pDatabase->Close();
				UTIL_Format(error, sizeof(error), "Unable to allocate Handle");
			}
		}

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
	CPlugin *me;
	IPluginFunction *m_pFunction;
	IDBDriver *m_pDriver;
	IDatabase *m_pDatabase;
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
	CExtension *pExt = g_Extensions.GetExtensionFromIdent(driver->GetIdentity());
	if (pExt)
	{
		g_Extensions.BindChildPlugin(pExt, g_PluginSys.GetPluginByCtx(pContext->GetContext()));
	}

	return hndl;
}

static cell_t SQL_TConnect(IPluginContext *pContext, const cell_t *params)
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
			UTIL_Format(error, 
				sizeof(error), 
				"Could not find driver \"%s\"", 
				pInfo->driver[0] == '\0' ? g_DBMan.GetDefaultDriverName() : pInfo->driver);
		} else if (!driver->IsThreadSafe()) {
			UTIL_Format(error,
				sizeof(error),
				"Driver \"%s\" is not thread safe!",
				driver->GetIdentifier());
		}
	} else {
		UTIL_Format(error, sizeof(error), "Could not find database conf \"%s\"", conf);
	}

	if (!pInfo || !driver)
	{
		pf->PushCell(BAD_HANDLE);
		pf->PushCell(BAD_HANDLE);
		pf->PushString(error);
		pf->PushCell(0);
		pf->Execute(NULL);
		return 0;
	}

	/* HACK! Add us to the dependency list */
	CExtension *pExt = g_Extensions.GetExtensionFromIdent(driver->GetIdentity());
	if (pExt)
	{
		g_Extensions.BindChildPlugin(pExt, g_PluginSys.GetPluginByCtx(pContext->GetContext()));
	}

	/* Finally, add to the thread if we can */
	TConnectOp *op = new TConnectOp(pf, driver, conf, params[3]);
	CPlugin *pPlugin = g_PluginSys.GetPluginByCtx(pContext->GetContext());
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
		CExtension *pExt = g_Extensions.GetExtensionFromIdent(driver->GetIdentity());
		if (pExt)
		{
			g_Extensions.BindChildPlugin(pExt, g_PluginSys.GetPluginByCtx(pContext->GetContext()));
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
	HandleError err;

	if ((err = ReadDbOrStmtHndl(params[1], pContext, &db, &stmt)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid statement or db Handle %x (error: %d)", params[1], err);
	}

	if (db)
	{
		return db->GetAffectedRows();
	} else if (stmt) {
		return stmt->GetAffectedRows();
	}

	return pContext->ThrowNativeError("Unknown error reading db/stmt handles");
}

static cell_t SQL_GetInsertId(IPluginContext *pContext, const cell_t *params)
{
	IDatabase *db = NULL;
	IPreparedQuery *stmt = NULL;
	HandleError err;

	if ((err = ReadDbOrStmtHndl(params[1], pContext, &db, &stmt)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid statement or db Handle %x (error: %d)", params[1], err);
	}

	if (db)
	{
		return db->GetInsertID();
	} else if (stmt) {
		return stmt->GetInsertID();
	}

	return pContext->ThrowNativeError("Unknown error reading db/stmt handles");
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

	Handle_t hndl = g_HandleSys.CreateHandle(hQueryType, qr, pContext->GetIdentity(), g_pCoreIdent, NULL);
	if (hndl == BAD_HANDLE)
	{
		qr->Destroy();
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

	CPlugin *pPlugin = g_PluginSys.GetPluginByCtx(pContext->GetContext());

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

	Handle_t hndl = g_HandleSys.CreateHandle(hStmtType, qr, pContext->GetIdentity(), g_pCoreIdent, NULL);
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

static cell_t SQL_CheckConfig(IPluginContext *pContext, const cell_t *params)
{
	char *name;
	pContext->LocalToString(params[1], &name);

	return (g_DBMan.FindDatabaseConf(name) != NULL) ? 1 : 0;
}

REGISTER_NATIVES(dbNatives)
{
	{"SQL_BindParamInt",		SQL_BindParamInt},
	{"SQL_BindParamFloat",		SQL_BindParamFloat},
	{"SQL_BindParamString",		SQL_BindParamString},\
	{"SQL_CheckConfig",			SQL_CheckConfig},
	{"SQL_Connect",				SQL_Connect},
	{"SQL_ConnectEx",			SQL_ConnectEx},
	{"SQL_EscapeString",		SQL_QuoteString},
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
	{NULL,						NULL},
};
