#ifndef _INCLUDE_SM_MYSQL_STATEMENT_H_
#define _INCLUDE_SM_MYSQL_STATEMENT_H_

#include "MyDatabase.h"
#include "MyBoundResults.h"

struct ParamBind
{
	union
	{
		float fval;
		int ival;
	} data;
	void *blob;
	size_t length;
};

class MyStatement : public IPreparedQuery
{
public:
	MyStatement(MyDatabase *db, MYSQL_STMT *stmt);
	~MyStatement();
public: //IQuery
	IResultSet *GetResultSet();
	bool FetchMoreResults();
	void Destroy();
public: //IPreparedQuery
	bool BindParamInt(unsigned int param, int num, bool signd=true);
	bool BindParamFloat(unsigned int param, float f);
	bool BindParamNull(unsigned int param);
	bool BindParamString(unsigned int param, const char *text, bool copy);
	bool BindParamBlob(unsigned int param, const void *data, size_t length, bool copy);
	bool Execute();
	const char *GetError(int *errCode=NULL);
	unsigned int GetAffectedRows();
	unsigned int GetInsertID();
private:
	void *CopyBlob(unsigned int param, const void *blobptr, size_t length);
private:
	MYSQL *m_mysql;
	MYSQL_STMT *m_stmt;
	MYSQL_BIND *m_bind;
	MYSQL_RES *m_pRes;
	MyDatabase *m_pParent;
	ParamBind *m_pushinfo;
	unsigned int m_Params;
	MyBoundResults *m_rs;
	bool m_Results;
};

#endif //_INCLUDE_SM_MYSQL_STATEMENT_H_
