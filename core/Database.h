#ifndef _INCLUDE_DATABASE_MANAGER_H_
#define _INCLUDE_DATABASE_MANAGER_H_

#include <IDBDriver.h>
#include <IHandleSys.h>
#include "sm_globals.h"
#include <sh_vector.h>

using namespace SourceHook;

struct db_driver
{
	IDBDriver *driver;
	IdentityToken_t *ident;
	HandleType_t htDatabase;
	HandleType_t htQuery;
	Handle_t hDriver;
};

class DBManager : 
	public IDBManager,
	public SMGlobalClass,
	public IHandleTypeDispatch
{
	const char *GetInterfaceName();
	unsigned int GetInterfaceVersion();
public: //SMGlobalClass
	void OnSourceModAllInitialized();
public: //IHandleTypeDispatch
	void OnHandleDestroy(HandleType_t type, void *object);
public: //IDBManager
	void AddDriver(IDBDriver *pDriver, IdentityToken_t *ident);
	void RemoveDriver(IDBDriver *pDriver);
	const DatabaseInfo *FindDatabaseConf(const char *name);
	bool Connect(const char *name, IDBDriver **pdr, IDatabase **pdb, bool persistent);
	unsigned int GetDriverCount();
	IDBDriver *GetDriver(unsigned int index, IdentityToken_t **pToken=NULL);
public:
	db_driver *GetDriverInfo(unsigned int index);
private:
	CVector<db_driver *> m_drivers;
	HandleType_t m_DriverType;
	HandleType_t m_DatabaseType;
	HandleType_t m_QueryType;
};

extern DBManager g_DBMan;

#endif //_INCLUDE_DATABASE_MANAGER_H_
