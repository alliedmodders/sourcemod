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

#include "Database.h"
#include "HandleSys.h"
#include "ShareSys.h"
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

	g_ShareSys.AddInterface(NULL, this);
}

void DBManager::OnSourceModShutdown()
{
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
	const DatabaseInfo *pInfo = FindDatabaseConf(name);

	for (size_t i=0; i<m_drivers.size(); i++)
	{
		if (strcasecmp(pInfo->driver, m_drivers[i]->GetIdentifier()) == 0)
		{
			*pdr = m_drivers[i];
			*pdb = m_drivers[i]->Connect(pInfo, persistent, error, maxlength);
			return (*pdb == NULL);
		}
	}

	*pdr = NULL;
	*pdb = NULL;

	return false;
}

void DBManager::AddDriver(IDBDriver *pDriver)
{
	 m_drivers.push_back(pDriver);
}

void DBManager::RemoveDriver(IDBDriver *pDriver)
{
	for (size_t i=0; i<m_drivers.size(); i++)
	{
		if (m_drivers[i] == pDriver)
		{
			m_drivers.erase(m_drivers.iterAt(i));
			return;
		}
	}
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

	return g_HandleSys.ReadHandle(hndl, dtype, &sec, ptr);
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
	/* :TODO: */
	return NULL;
}
