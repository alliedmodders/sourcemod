#ifndef _INCLUDE_SM_MYSQL_DATABASE_H_
#define _INCLUDE_SM_MYSQL_DATABASE_H_

#include "MyDriver.h"
#include <IThreader.h>

class MyQuery;
class MyStatement;

class MyDatabase : public IDatabase
{
	friend class MyQuery;
	friend class MyStatement;
public:
	MyDatabase(MYSQL *mysql, const DatabaseInfo *info, bool persistent);
	~MyDatabase();
public: //IDatabase
	bool Close();
	const char *GetError(int *errorCode=NULL);
	bool DoSimpleQuery(const char *query);
	IQuery *DoQuery(const char *query);
	IPreparedQuery *PrepareQuery(const char *query, char *error, size_t maxlength, int *errCode=NULL);
	bool QuoteString(const char *str, char buffer[], size_t maxlen, size_t *newSize);
	unsigned int GetAffectedRows();
	unsigned int GetInsertID();
	bool LockForFullAtomicOperation();
	void UnlockFromFullAtomicOperation();
	void IncReferenceCount();
	IDBDriver *GetDriver();
public:
	const DatabaseInfo &GetInfo();
private:
	MYSQL *m_mysql;
	unsigned int m_refcount;
	IMutex *m_pFullLock;

	/* ---------- */
	DatabaseInfo m_Info;
	String m_Host;
	String m_Database;
	String m_User;
	String m_Pass;
	bool m_bPersistent;
};

DBType GetOurType(enum_field_types type);

#endif //_INCLUDE_SM_MYSQL_DATABASE_H_
