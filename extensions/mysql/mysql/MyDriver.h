#ifndef _INCLUDE_SM_MYSQL_DRIVER_H_
#define _INCLUDE_SM_MYSQL_DRIVER_H_

#include <IDBDriver.h>
#include <sm_platform.h>
#if defined PLATFORM_WINDOWS
#include <winsock.h>
#endif

#include <mysql.h>

#include <sh_string.h>
#include <sh_list.h>

using namespace SourceMod;
using namespace SourceHook;

#define M_CLIENT_MULTI_RESULTS    ((1) << 17)  /* Enable/disable multi-results */

class MyDatabase;

class MyDriver : public IDBDriver
{
public:
	MyDriver();
public: //IDBDriver
	IDatabase *Connect(const DatabaseInfo *info, bool persistent, char *error, size_t maxlength);
	const char *GetIdentifier();
	const char *GetProductName();
	Handle_t GetHandle();
	IdentityToken_t *GetIdentity();
	bool IsThreadSafe();
	bool InitializeThreadSafety();
	void ShutdownThreadSafety();
public:
	void Shutdown();
	void RemoveFromList(MyDatabase *pdb, bool persistent);
private:
	Handle_t m_MyHandle;
	List<MyDatabase *> m_TempDbs;
	List<MyDatabase *> m_PermDbs;
};

extern MyDriver g_MyDriver;

unsigned int strncopy(char *dest, const char *src, size_t count);

#endif //_INCLUDE_SM_MYSQL_DRIVER_H_
