#ifndef _INCLUDE_FOLDER_LIST_H_
#define _INCLUDE_FOLDER_LIST_H_

#include "platform_headers.h"
#include <list>
#include <vector>

struct CFileListEntry
{
	TCHAR file[MAX_PATH];
	unsigned __int64 size;
};

class CFileList
{
public:
	CFileList(const TCHAR *name);
	~CFileList();
public:
	CFileList *PeekCurrentFolder();
	void PopCurrentFolder();
	const TCHAR *PeekCurrentFile();
	void PopCurrentFile();
	const TCHAR *GetFolderName();
public:
	void AddFolder(CFileList *pFileList);
	void AddFile(const TCHAR *name, unsigned __int64 size);
	unsigned __int64 GetRecursiveSize();
public:
	static CFileList *BuildFileList(const TCHAR *name, const TCHAR *root_folder);
private:
	std::list<CFileList *> m_folder_list;
	std::list<CFileListEntry> m_file_list;
	TCHAR m_FolderName[MAX_PATH];
	unsigned __int64 m_TotalSize;
	unsigned __int64 m_RecursiveSize;
	bool m_bGotRecursiveSize;
};

#endif //_INCLUDE_FOLDER_LIST_H_
