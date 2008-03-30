#include "InstallerMain.h"
#include "InstallerUtil.h"
#include "PerformInstall.h"
#include "CCriticalSection.h"

#define WMU_INSTALLER_DONE		WM_USER+2
#define PBAR_RANGE_HIGH			100
#define PBAR_RANGE_LOW			0

ICopyMethod *g_pCopyMethod = NULL;
HANDLE g_hCopyThread = NULL;
copy_thread_args_t g_thread_args = {NULL, NULL, NULL, false, false};
CCriticalSection g_update_window;

class TrackProgress : public ICopyProgress
{
public:
	void Initialize(HWND hTextBar, HWND hCurBar, HWND hTotalBar, size_t total_size)
	{
		m_hTextBar = hTextBar;
		m_hCurBar = hCurBar;
		m_hTotalBar = hTotalBar;
		m_TotalSize = total_size;
		m_TotalDone = 0;
		RedrawProgressBars(0.0, 0.0);
	}
	void Finished()
	{
		RedrawProgressBars(100.0, 100.0);
	}
public:
	void StartingNewFile(const TCHAR *filename)
	{
		TCHAR buffer[255];

		if (g_update_window.TryEnter())
		{
			UTIL_Format(buffer, sizeof(buffer) / sizeof(TCHAR), _T("Copying: %s"), filename);
			SendMessage(m_hTextBar, WM_SETTEXT, 0, (LPARAM)buffer);
			UpdateWindow(m_hTextBar);
			g_update_window.Leave();
		}
	}
	void UpdateProgress(size_t bytes, size_t total_bytes)
	{
		float fCur = (float)bytes / (float)total_bytes;
		float fTotal = ((float)m_TotalDone + (float)bytes) / (float)m_TotalSize;
		RedrawProgressBars(fCur, fTotal);
	}
	void FileDone(size_t total_size)
	{
		m_TotalDone += total_size;
		RedrawProgressBars(0.0, (float)m_TotalDone / (float)m_TotalSize);
	}
private:
	void RedrawProgressBar(HWND hBar, float fPerc)
	{
		/* Get a percentage point in the range */
		float fPointInRange = (float)(PBAR_RANGE_HIGH - PBAR_RANGE_LOW) * fPerc;
		int iPointInRange = (int)fPointInRange;

		/* Scale it */
		iPointInRange += PBAR_RANGE_LOW;

		if (g_update_window.TryEnter())
		{
			SendMessage(hBar,
				PBM_SETPOS,
				iPointInRange,
				0);
			g_update_window.Leave();
		}
	}
	void RedrawProgressBars(float fCurrent, float fTotal)
	{
		RedrawProgressBar(m_hCurBar, fCurrent);
		RedrawProgressBar(m_hTotalBar, fTotal);
	}
private:
	size_t m_TotalSize;
	size_t m_TotalDone;
	HWND m_hTextBar;
	HWND m_hCurBar;
	HWND m_hTotalBar;
} s_ProgressTracker;

void CancelPerformInstall()
{
	delete g_thread_args.pFileList;
	g_thread_args.pFileList = NULL;
}

void SetInstallMethod(ICopyMethod *pCopyMethod)
{
	g_pCopyMethod = pCopyMethod;
}

bool CopyStructureRecursively(ICopyMethod *pCopyMethod,
							  CFileList *pFileList,
							  const TCHAR *basepath,
							  const TCHAR *local_path,
							  TCHAR *errbuf,
							  size_t maxchars)
{
	TCHAR file_path[MAX_PATH];
	const TCHAR *file;
	CFileList *pSubList;

	if (!pCopyMethod->SetCurrentFolder(local_path, errbuf, maxchars))
	{
		return false;
	}

	/* Copy files */
	while ((file = pFileList->PeekCurrentFile()) != NULL)
	{
		if (local_path == NULL)
		{
			UTIL_PathFormat(file_path,
				sizeof(file_path) / sizeof(TCHAR),
				_T("%s\\%s"),
				basepath,
				file);
		}
		else
		{
			UTIL_PathFormat(file_path,
				sizeof(file_path) / sizeof(TCHAR), 
				_T("%s\\%s\\%s"),
				basepath,
				local_path,
				file);
		}

		if (!pCopyMethod->SendFile(file_path, errbuf, maxchars))
		{
			return false;
		}

		pFileList->PopCurrentFile();
	}

	/* Now copy folders */
	while ((pSubList = pFileList->PeekCurrentFolder()) != NULL)
	{
		if (g_thread_args.m_bIsUpgrade)
		{
			/* :TODO: put this somewhere else because it technically 
			 * means the progress bars get calculated wrong 
			 */
			if (tstrcasecmp(pSubList->GetFolderName(), _T("cfg")) == 0
				|| tstrcasecmp(pSubList->GetFolderName(), _T("configs")) == 0)
			{
				pFileList->PopCurrentFolder();
				continue;
			}
		}

		/* Try creating the folder */
		if (!pCopyMethod->CreateFolder(pSubList->GetFolderName(), errbuf, maxchars))
		{
			return false;
		}

		TCHAR new_local_path[MAX_PATH];
		if (local_path == NULL)
		{
			UTIL_PathFormat(new_local_path,
				sizeof(new_local_path) / sizeof(TCHAR),
				_T("%s"),
				pSubList->GetFolderName());
		}
		else
		{
			UTIL_PathFormat(new_local_path,
				sizeof(new_local_path) / sizeof(TCHAR),
				_T("%s\\%s"),
				local_path,
				pSubList->GetFolderName());
		}

		if (!CopyStructureRecursively(pCopyMethod,
				pSubList,
				basepath, 
				new_local_path, 
				errbuf, 
				maxchars))
		{
			return false;
		}

		pFileList->PopCurrentFolder();

		/* Set the current folder again for the next operation */
		if (!pCopyMethod->SetCurrentFolder(local_path, errbuf, maxchars))
		{
			return false;
		}
	}

	return true;
}

DWORD WINAPI T_CopyFiles(LPVOID arg)
{
	bool result = 
		CopyStructureRecursively(g_thread_args.pCopyMethod,
		g_thread_args.pFileList,
		g_thread_args.basepath,
		NULL,
		g_thread_args.error,
		sizeof(g_thread_args.error) / sizeof(TCHAR));

	PostMessage(g_thread_args.hWnd, WMU_INSTALLER_DONE, result ? TRUE : FALSE, 0);

	return 0;
}

bool StartFileCopy(HWND hWnd)
{
	g_thread_args.m_bWasCancelled = false;
	g_thread_args.hWnd = hWnd;
	if ((g_hCopyThread = CreateThread(NULL, 
		0, 
		T_CopyFiles, 
		NULL, 
		0,
		NULL))
		== NULL)
	{
		MessageBox(
			hWnd,
			_T("Could not initialize copy thread."),
			_T("SourceMod Installer"),
			MB_OK|MB_ICONERROR);
		return false;
	}
	return true;
}

void StopFileCopy()
{
	g_thread_args.m_bWasCancelled = true;
	g_pCopyMethod->CancelCurrentCopy();

	if (g_hCopyThread != NULL)
	{
		g_update_window.Enter();
		WaitForSingleObject(g_hCopyThread, INFINITE);
		g_update_window.Leave();
		CloseHandle(g_hCopyThread);
		g_hCopyThread = NULL;
	}
}

bool RequestCancelInstall(HWND hWnd)
{
	StopFileCopy();

	int val = MessageBox(
		hWnd,
		_T("Are you sure you want to cancel the install?"),
		_T("SourceMod Installer"),
		MB_YESNO|MB_ICONQUESTION);

	if (val == IDYES)
	{
		return true;
	}
	
	if (g_thread_args.pFileList == NULL)
	{
		return false;
	}

	/* Start the thread, note our return value is opposite */
	return !StartFileCopy(hWnd);
}

bool StartInstallProcess(HWND hWnd)
{
	if (g_pCopyMethod->CheckForExistingInstall())
	{
		int val = MessageBox(
			hWnd, 
			_T("It looks like a previous SourceMod installation exists.  Select \"Yes\" to skip copying configuration files.  Select \"No\" to perform a full re-install."),
			_T("SourceMod Installer"), 
			MB_YESNO|MB_ICONQUESTION);

		if (val == 0 || val == IDYES)
		{
			g_thread_args.m_bIsUpgrade = true;
		}
		else
		{
			g_thread_args.m_bIsUpgrade = false;
		}
	}

#if 0
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
#endif

#if 0
	UTIL_PathFormat(source_path,
		sizeof(source_path) / sizeof(TCHAR),
		_T("%s\\files"),
		cur_path);
#else
	UTIL_PathFormat(g_thread_args.basepath,
		sizeof(g_thread_args.basepath) / sizeof(TCHAR),
		_T("C:\\real\\done\\base"));
#endif

	if (GetFileAttributes(g_thread_args.basepath) == INVALID_FILE_ATTRIBUTES)
	{
		MessageBox(
			hWnd,
			_T("Could not locate the source installation files!"),
			_T("SourceMod Installer"),
			MB_OK|MB_ICONERROR);
		return false;
	}

	delete g_thread_args.pFileList;
	g_thread_args.pFileList = CFileList::BuildFileList(_T(""), g_thread_args.basepath);

	s_ProgressTracker.Initialize(
		GetDlgItem(hWnd, IDC_PROGRESS_CURCOPY),
		GetDlgItem(hWnd, IDC_PROGRESS_CURRENT),
		GetDlgItem(hWnd, IDC_PROGRESS_TOTAL),
		(size_t)g_thread_args.pFileList->GetRecursiveSize());
	g_pCopyMethod->TrackProgress(&s_ProgressTracker);
	g_thread_args.pCopyMethod = g_pCopyMethod;

	return StartFileCopy(hWnd);
}

INT_PTR CALLBACK PerformInstallHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		{
			SetToGlobalPosition(hDlg);
			return (INT_PTR)TRUE;
		}
	case WM_COMMAND:
		{
			if (LOWORD(wParam) == ID_INSTALL_CANCEL
				|| LOWORD(wParam) == ID_CLOSE)
			{
				if (RequestCancelInstall(hDlg))
				{
					CancelPerformInstall();
					UpdateGlobalPosition(hDlg);
					EndDialog(hDlg, NULL);
				}
				return (INT_PTR)TRUE;
			}
			else if (LOWORD(wParam) == ID_INSTALL_START)
			{
				HWND hButton = GetDlgItem(hDlg, ID_INSTALL_START);
				EnableWindow(hButton, FALSE);
				StartInstallProcess(hDlg);
			}
			break;
		}
	case WMU_INSTALLER_DONE:
		{
			if (wParam == TRUE)
			{
				s_ProgressTracker.Finished();
				MessageBox(hDlg,
					_T("SourceMod was successfully installed!  Please visit http://www.sourcemod.net/ for documentation."),
					_T("SourceMod Installer"),
					MB_OK);
				CancelPerformInstall();
				UpdateGlobalPosition(hDlg);
				EndDialog(hDlg, NULL);
				return (INT_PTR)TRUE;
			}
			else if (!g_thread_args.m_bWasCancelled)
			{
				TCHAR buffer[500];

				UTIL_Format(buffer, 
					sizeof(buffer) / sizeof(TCHAR), 
					_T("Encountered error: %s"),
					g_thread_args.error);
				int res = MessageBox(hDlg,
					buffer,
					_T("SourceMod Installer"),
					MB_ICONERROR|MB_RETRYCANCEL);

				if (res == IDRETRY)
				{
					StartFileCopy(hDlg);
				}
				else
				{
					CancelPerformInstall();
					UpdateGlobalPosition(hDlg);
					EndDialog(hDlg, NULL);
					return (INT_PTR)TRUE;
				}
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
