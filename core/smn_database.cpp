#include "sm_globals.h"
#include "HandleSys.h"
#include "Database.h"
#include "ExtensionSys.h"
#include "PluginSys.h"

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
		hStmtType = g_HandleSys.CreateType("IPreparedQuery", this, hQueryType, &tacc, &acc, NULL, NULL);
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

	/* HACK! Add us to the dependency list */
	CExtension *pExt = g_Extensions.GetExtensionFromIdent(driver->GetIdentity());
	if (pExt)
	{
		g_Extensions.BindChildPlugin(pExt, g_PluginSys.FindPluginByContext(pContext->GetContext()));
	}

	return db->GetHandle();
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
		/* HACK! Add us to the dependency list */
		CExtension *pExt = g_Extensions.GetExtensionFromIdent(driver->GetIdentity());
		if (pExt)
		{
			g_Extensions.BindChildPlugin(pExt, g_PluginSys.FindPluginByContext(pContext->GetContext()));
		}

		return db->GetHandle();
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

	IQuery *qr = db->DoQuery(query);

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
	pContext->LocalToPhysAddr(params[5], &addr);
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
	pContext->LocalToPhysAddr(params[5], &addr);
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

REGISTER_NATIVES(dbNatives)
{
	{"SQL_BindParamInt",		SQL_BindParamInt},
	{"SQL_BindParamFloat",		SQL_BindParamFloat},
	{"SQL_BindParamString",		SQL_BindParamString},
	{"SQL_Connect",				SQL_Connect},
	{"SQL_ConnectEx",			SQL_ConnectEx},
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
	{"SQL_MoreRows",			SQL_MoreRows},
	{"SQL_Query",				SQL_Query},
	{"SQL_QuoteString",			SQL_QuoteString},
	{"SQL_PrepareQuery",		SQL_PrepareQuery},
	{"SQL_Rewind",				SQL_Rewind},
	{NULL,						NULL},
};
