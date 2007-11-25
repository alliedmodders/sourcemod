#include "InstallerUtil.h"
#include "LocalCopyMethod.h"

LocalCopyMethod g_LocalCopier;

DWORD CALLBACK CopyProgressRoutine(LARGE_INTEGER TotalFileSize,
								   LARGE_INTEGER TotalBytesTransferred,
								   LARGE_INTEGER StreamSize,
								   LARGE_INTEGER StreamBytesTransferred,
								   DWORD dwStreamNumber,
								   DWORD dwCallbackReason,
								   HANDLE hSourceFile,
								   HANDLE hDestinationFile,
								   LPVOID lpData)
{
	ICopyProgress *progress = (ICopyProgress *)lpData;

	progress->UpdateProgress((size_t)TotalBytesTransferred.QuadPart,
		(size_t)TotalFileSize.QuadPart);

	return PROGRESS_CONTINUE;
}

LocalCopyMethod::LocalCopyMethod()
{
	m_pProgress = NULL;
}

void LocalCopyMethod::SetOutputPath(const TCHAR *path)
{
	UTIL_PathFormat(m_OutputPath,
		sizeof(m_OutputPath) / sizeof(TCHAR),
		_T("%s"),
		path);

	UTIL_PathFormat(m_CurrentPath,
		sizeof(m_CurrentPath) / sizeof(TCHAR),
		_T("%s"),
		path);
}

void LocalCopyMethod::TrackProgress(ICopyProgress *pProgress)
{
	m_pProgress = pProgress;
}

bool LocalCopyMethod::CreateFolder(const TCHAR *name, TCHAR *buffer, size_t maxchars)
{
	TCHAR path[MAX_PATH];

	UTIL_PathFormat(path,
		sizeof(path) / sizeof(TCHAR),
		_T("%s\\%s"),
		m_CurrentPath,
		name);

	if (CreateDirectory(path, NULL))
	{
		return true;
	}

	DWORD error = GetLastError();
	if (error == ERROR_ALREADY_EXISTS)
	{
		return true;
	}

	GenerateErrorMessage(error, buffer, maxchars);

	return false;
}

bool LocalCopyMethod::SetCurrentFolder(const TCHAR *path, TCHAR *buffer, size_t maxchars)
{
	if (path == NULL)
	{
		UTIL_PathFormat(m_CurrentPath,
			sizeof(m_CurrentPath) / sizeof(TCHAR),
			_T("%s"),
			m_OutputPath);
	}
	else
	{
		UTIL_PathFormat(m_CurrentPath,
			sizeof(m_CurrentPath) / sizeof(TCHAR),
			_T("%s\\%s"),
			m_OutputPath,
			path);
	}

	return true;
}

bool LocalCopyMethod::SendFile(const TCHAR *path, TCHAR *buffer, size_t maxchars)
{
	const TCHAR *filename = GetFileFromPath(path);

	if (filename == NULL)
	{
		UTIL_Format(buffer, maxchars, _T("Invalid filename"));
		return false;
	}

	TCHAR new_path[MAX_PATH];
	UTIL_PathFormat(new_path,
		sizeof(new_path) / sizeof(TCHAR),
		_T("%s\\%s"),
		m_CurrentPath,
		filename);

	m_bCancelStatus = FALSE;

	if (m_pProgress != NULL)
	{
		m_pProgress->StartingNewFile(filename);
	}

	if (CopyFileEx(path,
		new_path,
		m_pProgress ? CopyProgressRoutine : NULL,
		m_pProgress,
		&m_bCancelStatus,
		0) == 0)
	{
		/* Delete the file in case it was a partial copy */
		DeleteFile(new_path);

		GenerateErrorMessage(GetLastError(), buffer, maxchars);

		return false;
	}

	if (m_pProgress != NULL)
	{
		m_pProgress->FileDone(UTIL_GetFileSize(path));
	}

	return true;
}

void LocalCopyMethod::CancelCurrentCopy()
{
	m_bCancelStatus = TRUE;
}

bool LocalCopyMethod::CheckForExistingInstall()
{
	TCHAR path[MAX_PATH];

	UTIL_PathFormat(path,
		sizeof(path) / sizeof(TCHAR),
		_T("%s\\addons\\sourcemod"),
		m_CurrentPath);
	if (GetFileAttributes(path) == INVALID_FILE_ATTRIBUTES)
	{
		UTIL_PathFormat(path,
			sizeof(path) / sizeof(TCHAR),
			_T("%s\\cfg\\sourcemod"),
			m_CurrentPath);
		if (GetFileAttributes(path) == INVALID_FILE_ATTRIBUTES)
		{
			return false;
		}
	}

	return true;
}
