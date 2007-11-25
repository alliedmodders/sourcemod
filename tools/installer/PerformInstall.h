#ifndef _INCLUDE_PERFORM_INSTALL_H_
#define _INCLUDE_PERFORM_INSTALL_H_

#include "InstallerMain.h"
#include "ICopyMethod.h"
#include "CFileList.h"

struct copy_thread_args_t
{
	ICopyMethod *pCopyMethod;
	CFileList *pFileList;
	HWND hWnd;
	bool m_bIsUpgrade;
	bool m_bWasCancelled;
	TCHAR basepath[MAX_PATH];
	TCHAR error[255];
};

void *DisplayPerformInstall(HWND hWnd);
void SetInstallMethod(ICopyMethod *pCopyMethod);

#endif //_INCLUDE_PERFORM_INSTALL_H_
