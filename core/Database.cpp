#include "Database.h"
#include "HandleSys.h"
#include "sourcemod.h"

DBManager g_DBMan;

void DBManager::OnSourceModAllInitialized()
{
	HandleAccess sec;

	g_HandleSys.InitAccessDefaults(NULL, &sec);
	sec.access[HandleAccess_Delete] |= HANDLE_RESTRICT_IDENTITY;
	sec.access[HandleAccess_Clone] |= HANDLE_RESTRICT_IDENTITY;
	
	m_DriverType = g_HandleSys.CreateType("IDriver", this, 0, NULL, &sec, g_pCoreIdent, NULL);
	m_DatabaseType = g_HandleSys.CreateType("IDatabase", this, 0, NULL, NULL, g_pCoreIdent, NULL);
	m_QueryType = g_HandleSys.CreateType("IQuery", this, 0, NULL, NULL, g_pCoreIdent, NULL);
}

void DBManager::OnHandleDestroy(HandleType_t type, void *object)
{
	if (type == m_DriverType)
	{
		/* Ignore */
		return;
	}

	if (g_HandleSys.TypeCheck(type, m_QueryType))
	{
		IQuery *pQuery = (IQuery *)object;
		pQuery->Release();
	} else if (g_HandleSys.TypeCheck(type, m_DatabaseType)) {
		IDatabase *pdb = (IDatabase *)object;
		pdb->Close();
	}
}

void DBManager::AddDriver(IDBDriver *pDriver, IdentityToken_t *ident)
{
	if (!pDriver)
	{
		return;
	}

	if (m_drivers.size() >= HANDLESYS_MAX_SUBTYPES)
	{
		return;
	}

	db_driver *driver = new db_driver;

	driver->driver = pDriver;
	driver->ident = ident;

	HandleSecurity sec;

	sec.pIdentity = ident;
	sec.pOwner = g_pCoreIdent;
	driver->hDriver = g_HandleSys.CreateHandleEx(m_DriverType, driver, &sec, NULL, NULL);

	driver->htDatabase = g_HandleSys.CreateType(NULL, this, m_DatabaseType, NULL, NULL, g_pCoreIdent, NULL);
	driver->htQuery = g_HandleSys.CreateType(NULL, this, m_QueryType, NULL, NULL, g_pCoreIdent, NULL);
	
	m_drivers.push_back(driver);
}

void DBManager::RemoveDriver(IDBDriver *pDriver)
{
	CVector<db_driver *>::iterator iter;
	for (iter = m_drivers.begin();
		 iter != m_drivers.end();
		 iter++)
	{
		if ((*iter)->driver == pDriver)
		{
			db_driver *driver = (*iter);
			HandleSecurity sec;

			sec.pIdentity = driver->ident;
			sec.pOwner = g_pCoreIdent;
			g_HandleSys.RemoveType(driver->htQuery, driver->ident);
			g_HandleSys.RemoveType(driver->htDatabase, driver->ident);
			g_HandleSys.FreeHandle(driver->hDriver, &sec);

			delete driver;
			m_drivers.erase(iter);

			return;
		}
	}
}

const char *DBManager::GetInterfaceName()
{
	return SMINTERFACE_DBI_NAME;
}

unsigned int DBManager::GetInterfaceVersion()
{
	return SMINTERFACE_DBI_VERSION;
}

unsigned int DBManager::GetDriverCount()
{
	return (unsigned int)m_drivers.size();
}

IDBDriver *DBManager::GetDriver(unsigned int index, IdentityToken_t **pToken)
{
	if (index >= GetDriverCount())
	{
		return NULL;
	}

	if (pToken)
	{
		*pToken = m_drivers[index]->ident;
	}

	return m_drivers[index]->driver;
}

const DatabaseInfo *DBManager::FindDatabaseConf(const char *name)
{
	/* :TODO: implement */
	return NULL;
}

bool DBManager::Connect(const char *name, IDBDriver **pdr, IDatabase **pdb, bool persistent)
{
	const DatabaseInfo *pInfo = FindDatabaseConf(name);

	unsigned int count = GetDriverCount();
	for (unsigned int i=0; i<count; i++)
	{
		IDBDriver *driver = GetDriver(i);
		if (strcmp(driver->GetIdentifier(), pInfo->driver) == 0)
		{
			*pdr = driver;
			if (persistent)
			{
				*pdb = driver->PConnect(pInfo);
			} else {
				*pdb = driver->Connect(pInfo);
			}
		}
	}

	*pdr = NULL;
	*pdb = NULL;

	return false;
}
