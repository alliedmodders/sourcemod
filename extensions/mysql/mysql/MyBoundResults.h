#ifndef _INCLUDE_SM_MYSQL_BOUND_RESULTS_H_
#define _INCLUDE_SM_MYSQL_BOUND_RESULTS_H_

#include "MyDatabase.h"

class MyStatement;

struct ResultBind
{
	my_bool my_null;
	unsigned long my_length;
	union
	{
		int ival;
		float fval;
	} data;
	unsigned char *blob;
	size_t length;
};

class MyBoundResults : 
	public IResultSet,
	public IResultRow
{
	friend class MyStatement;
public:
	MyBoundResults(MYSQL_STMT *stmt, MYSQL_RES *res);
	~MyBoundResults();
public: //IResultSet
	unsigned int GetRowCount();
	unsigned int GetFieldCount();
	const char *FieldNumToName(unsigned int columnId);
	bool FieldNameToNum(const char *name, unsigned int *columnId);
	bool MoreRows();
	IResultRow *FetchRow();
	bool Rewind();
	DBType GetFieldType(unsigned int field);
	DBType GetFieldDataType(unsigned int field);
	IResultRow *CurrentRow();
public: //IResultRow
	DBResult GetString(unsigned int id, const char **pString, size_t *length);
	DBResult CopyString(unsigned int id, 
		char *buffer, 
		size_t maxlength, 
		size_t *written);
	DBResult GetFloat(unsigned int id, float *pFloat);
	DBResult GetInt(unsigned int id, int *pInt);
	bool IsNull(unsigned int id);
	size_t GetDataSize(unsigned int id);
	DBResult GetBlob(unsigned int id, const void **pData, size_t *length);
	DBResult CopyBlob(unsigned int id, void *buffer, size_t maxlength, size_t *written);
public:
	bool Initialize();
	void Update();
private:
	MYSQL_STMT *m_stmt;
	MYSQL_RES *m_pRes;
	MYSQL_BIND *m_bind;
	ResultBind *m_pull;
	unsigned int m_ColCount;
	bool m_Initialized;
	unsigned int m_RowCount;
	unsigned int m_CurRow;
};

#endif //_INCLUDE_SM_MYSQL_BOUND_RESULTS_H_
