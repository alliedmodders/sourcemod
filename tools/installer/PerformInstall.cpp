#include "InstallerMain.h"
#include "InstallerUtil.h"
#include "PerformInstall.h"

struct folder_t
{
	TCHAR path[MAX_PATH];
};

ICopyMethod *g_pCopyMethod = NULL;
bool do_not_copy_binaries = false;
TCHAR source_path[MAX_PATH];

void SetInstallMethod(ICopyMethod *pCopyMethod)
{
	g_pCopyMethod = pCopyMethod;
}

bool CopyStructureRecursively(const TCHAR *basepath,
							  const TCHAR *local_path,
							  TCHAR *errbuf,
							  size_t maxchars)
{
	HANDLE hFind;
	WIN32_FIND_DATA fd;
	TCHAR search_path[MAX_PATH];
	folder_t *folder_list = NULL;
	unsigned int folder_count = 0;

	if (local_path == NULL)
	{
		UTIL_PathFormat(search_path,
			sizeof(search_path) / sizeof(TCHAR),
			_T("%s\\*.*"),
			basepath);
	}
	else
	{
		UTIL_PathFormat(search_path,
			sizeof(search_path) / sizeof(TCHAR),
			_T("%s\\%s\\*.*"),
			basepath,
			local_path);
	}

	if (!g_pCopyMethod->SetCurrentFolder(local_path, errbuf, maxchars))
	{
		/* :TODO: set fail state */
		return false;
	}

	if ((hFind = FindFirstFile(search_path, &fd)) == INVALID_HANDLE_VALUE)
	{
		/* :TODO: set a fail state */
		return false;
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
			/* We cache the folder list so we don't have to keep changing folders back and forth, 
			 * which could be annoying on a slow copy connection.
			 */
			if (folder_list == NULL)
			{
				folder_list = (folder_t *)malloc(sizeof(folder_t) * (folder_count + 1));
			}
			else
			{
				folder_list = (folder_t *)realloc(folder_list, sizeof(folder_t) * (folder_count + 1));
			}

			UTIL_Format(folder_list[folder_count].path, MAX_PATH, _T("%s"), fd.cFileName);
			folder_count++;
		}
		else
		{
			TCHAR file_path[MAX_PATH];

			if (local_path == NULL)
			{
				UTIL_PathFormat(file_path,
					sizeof(file_path),
					_T("%s\\%s"),
					basepath,
					fd.cFileName);
			}
			else
			{
				UTIL_PathFormat(file_path,
					sizeof(file_path),
					_T("%s\\%s\\%s"),
					basepath,
					local_path,
					fd.cFileName);
			}

			if (!g_pCopyMethod->SendFile(file_path, errbuf, maxchars))
			{
				FindClose(hFind);
				free(folder_list);
				return false;
			}
		}
	} while (FindNextFile(hFind, &fd));

	FindClose(hFind);

	/* Now copy folders */
	for (unsigned int i = 0; i < folder_count; i++)
	{
		/* Try creating the folder */
		if (!g_pCopyMethod->CreateFolder(folder_list[i].path, errbuf, maxchars))
		{
			free(folder_list);
			return false;
		}

		TCHAR new_local_path[MAX_PATH];
		if (local_path == NULL)
		{
			UTIL_PathFormat(new_local_path,
				sizeof(new_local_path) / sizeof(TCHAR),
				_T("%s"),
				folder_list[i].path);
		}
		else
		{
			UTIL_PathFormat(new_local_path,
				sizeof(new_local_path) / sizeof(TCHAR),
				_T("%s\\%s"),
				local_path,
				folder_list[i].path);
		}

		if (!CopyStructureRecursively(basepath, new_local_path, errbuf, maxchars))
		{
			free(folder_list);
			return false;
		}
	}

	free(folder_list);

	return true;
}

bool StartInstallProcess(HWND hWnd)
{
	if (g_pCopyMethod->CheckForExistingInstall())
	{
		int val = MessageBox(
			hWnd, 
			_T("It looks like a previous SourceMod installation exists.  Do you want to upgrade?  Select \"Yes\" to upgrade and keep configuration files.  Select \"No\" to perform a full re-install."),
			_T("SourceMod Installer"), 
			MB_YESNO|MB_ICONQUESTION);

		if (val == 0 || val == IDYES)
		{
			do_not_copy_binaries = true;
		}
		else
		{
			do_not_copy_binaries = false;
		}
	}

	TCHAR cur_path[MAX_PATH];
	if (_tgetcwd(cur_path, sizeof(cur_path)) == NULL)
	{
		MessageBox(
			hWnd,
			_T("Could not locate current directory!"),
			_T("SourceMod Installer"),
			MB_OK|MB_ICONERROR);
		return false;
	}

	UTIL_PathFormat(source_path,
		sizeof(source_path) / sizeof(TCHAR),
		_T("%s\\files"),
		cur_path);

	if (GetFileAttributes(source_path) == INVALID_FILE_ATTRIBUTES)
	{
		MessageBox(
			hWnd,
			_T("Could not locate the source installation files!"),
			_T("SourceMod Installer"),
			MB_OK|MB_ICONERROR);
		return false;
	}



	return true;
}

INT_PTR CALLBACK PerformInstallHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		{
			return (INT_PTR)TRUE;
		}
	case WM_COMMAND:
		{
			if (LOWORD(wParam) == ID_INSTALL_CANCEL
				|| LOWORD(wParam) == ID_CLOSE)
			{
				/* :TODO: enhance */
				EndDialog(hDlg, NULL);
				return (INT_PTR)TRUE;
			}
			break;
		}
	}

	return (INT_PTR)FALSE;
}

void *DisplayPerformInstall(HWND hWnd)
{
	INT_PTR val;

	if ((val = DialogBox(
		g_hInstance, 
		MAKEINTRESOURCE(IDD_PERFORM_INSTALL),
		hWnd,
		PerformInstallHandler)) == -1)
	{
		return NULL;
	}

	return (void *)val;
}
