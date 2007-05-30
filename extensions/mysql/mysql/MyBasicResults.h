#ifndef _INCLUDE_SM_MYSQL_BASIC_RESULTS_H_
#define _INCLUDE_SM_MYSQL_BASIC_RESULTS_H_

#include "MyDatabase.h"

class MyQuery;

class MyBasicResults : 
	public IResultSet,
	public IResultRow
{
	friend class MyQuery;
public:
	MyBasicResults(MYSQL_RES *res);
	~MyBasicResults();
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
	DBResult GetString(unsigned int columnId, const char **pString, size_t *length);
	DBResult GetFloat(unsigned int columnId, float *pFloat);
	DBResult GetInt(unsigned int columnId, int *pInt);
	bool IsNull(unsigned int columnId);
	DBResult GetBlob(unsigned int columnId, const void **pData, size_t *length);
	DBResult CopyBlob(unsigned int columnId, void *buffer, size_t maxlength, size_t *written);
	DBResult CopyString(unsigned int columnId, 
						char *buffer, 
						size_t maxlength, 
						size_t *written);
	size_t GetDataSize(unsigned int columnId);
protected:
	void Update();
private:
	MYSQL_RES *m_pRes;
	unsigned int m_CurRow;
	MYSQL_ROW m_Row;
	unsigned long *m_Lengths;
	unsigned int m_ColCount;
	unsigned int m_RowCount;
};

class MyQuery : public IQuery
{
	friend class MyBasicResults;
public:
	MyQuery(MyDatabase *db, MYSQL_RES *res);
public:
	IResultSet *GetResultSet();
	bool FetchMoreResults();
	void Destroy();
private:
	MyDatabase *m_pParent;
	MyBasicResults m_rs;
};

#endif //_INCLUDE_SM_MYSQL_BASIC_RESULTS_H_
