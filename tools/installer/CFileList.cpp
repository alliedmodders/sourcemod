#include "CFileList.h"
#include "InstallerUtil.h"

using namespace std;

CFileList::CFileList(const TCHAR *name) : m_TotalSize(0), m_RecursiveSize(0), 
	m_bGotRecursiveSize(false)
{
	UTIL_Format(m_FolderName, sizeof(m_FolderName) / sizeof(TCHAR), _T("%s"), name);
}

CFileList::~CFileList()
{
	list<CFileList *>::iterator iter;

	for (iter = m_folder_list.begin();
		 iter != m_folder_list.end();
		 iter++)
	{
		delete (*iter);
	}
}

const TCHAR *CFileList::GetFolderName()
{
	return m_FolderName;
}

void CFileList::AddFolder(CFileList *pFileList)
{
	m_folder_list.push_back(pFileList);
}

void CFileList::AddFile(const TCHAR *name, unsigned __int64 size)
{
	CFileListEntry entry;
	
	UTIL_Format(entry.file, sizeof(entry.file) / sizeof(TCHAR), _T("%s"), name);
	entry.size = size;

	m_file_list.push_back(entry);
	m_TotalSize += size;
}

unsigned __int64 CFileList::GetRecursiveSize()
{
	if (m_bGotRecursiveSize)
	{
		return m_RecursiveSize;
	}

	m_RecursiveSize = m_TotalSize;

	list<CFileList *>::iterator iter;
	for (iter = m_folder_list.begin(); iter != m_folder_list.end(); iter++)
	{
		m_RecursiveSize += (*iter)->GetRecursiveSize();
	}

	m_bGotRecursiveSize = true;

	return m_RecursiveSize;
}

const TCHAR *CFileList::PeekCurrentFile()
{
	if (m_file_list.empty())
	{
		return NULL;
	}

	return m_file_list.begin()->file;
}

void CFileList::PopCurrentFile()
{
	m_file_list.erase(m_file_list.begin());
}

CFileList *CFileList::PeekCurrentFolder()
{
	if (m_folder_list.empty())
	{
		return NULL;
	}

	return *(m_folder_list.begin());
}

void CFileList::PopCurrentFolder()
{
	m_folder_list.erase(m_folder_list.begin());
}

void RecursiveBuildFileList(CFileList *file_list, const TCHAR *current_folder)
{
	HANDLE hFind;
	WIN32_FIND_DATA fd;
	TCHAR path[MAX_PATH];

	UTIL_PathFormat(path, sizeof(path) / sizeof(TCHAR), _T("%s\\*.*"), current_folder);

	if ((hFind = FindFirstFile(path, &fd)) == INVALID_HANDLE_VALUE)
	{
		return;
	}

	do
	{
		if (tstrcasecmp(fd.cFileName, _T(".")) == 0
			|| tstrcasecmp(fd.cFileName, _T("..")) == 0)
		{
			continue;
		}

		if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
		{
			CFileList *pSubList = new CFileList(fd.cFileName);

			UTIL_PathFormat(path,
				sizeof(path) / sizeof(TCHAR),
				_T("%s\\%s"), 
				current_folder,
				fd.cFileName);			

			RecursiveBuildFileList(pSubList, path);

			file_list->AddFolder(pSubList);
		}
		else
		{
			LARGE_INTEGER li;

			li.LowPart = fd.nFileSizeLow;
			li.HighPart = fd.nFileSizeHigh;

			file_list->AddFile(fd.cFileName, li.QuadPart);
		}
	} while (FindNextFile(hFind, &fd));

	FindClose(hFind);
}

CFileList *CFileList::BuildFileList(const TCHAR *name, const TCHAR *root_folder)
{
	CFileList *pFileList = new CFileList(name);

	RecursiveBuildFileList(pFileList, root_folder);

	return pFileList;
}
