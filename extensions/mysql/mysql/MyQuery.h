#ifndef _INCLUDE_SM_MYSQL_QUERY_H_
#define _INCLUDE_SM_MYSQL_QUERY_H_

#include "MyDriver.h"
#include "MyDatabase.h"

class MyResultSet :
	public IResultSet,
	public IResultRow
{
public:

};

class MyQuery : public IQuery
{
public:
	MyQuery(MyDatabase *db, MYSQL_RES *res);
public:
	IResultSet *GetResults();
	void Destroy();
private:
	MyDatabase *m_pParent;
};

#endif //_INCLUDE_SM_MYSQL_QUERY_H_
