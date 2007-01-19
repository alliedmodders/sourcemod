#include <stdio.h>
#include "LibrarySys.h"

LibrarySystem g_LibSys;

CLibrary::~CLibrary()
{
	if (m_lib)
	{
#if defined PLATFORM_WINDOWS
		FreeLibrary(m_lib);
#else if defined PLATFORM_POSIX
		dlclose(m_lib);
#endif
		m_lib = NULL;
	}
}

CLibrary::CLibrary(LibraryHandle me)
{
	m_lib = me;
}

void CLibrary::CloseLibrary()
{
	delete this;
}

void *CLibrary::GetSymbolAddress(const char *symname)
{
#if defined PLATFORM_WINDOWS
	return GetProcAddress(m_lib, symname);
#else if defined PLATFORM_POSIX
	return dlsym(m_lib, symname);
#endif
}


/********************
 ** Directory Code **
 ********************/


CDirectory::CDirectory(const char *path)
{
#if defined PLATFORM_WINDOWS
	char newpath[PLATFORM_MAX_PATH];
	snprintf(newpath, sizeof(newpath), "%s\\*.*", path);
	m_dir = FindFirstFileA(newpath, &m_fd);
	if (!IsValid())
	{
		m_fd.cFileName[0] = '\0';
	}
#else if defined PLATFORM_POSIX
	m_dir = opendir(path);
	if (IsValid())
	{
		/* :TODO: we need to read past "." and ".."! */
		ep = readdir(dp);
		snprintf(m_origpath, PLATFORM_MAX_PATH, "%s", path);
	} else {
		ep = NULL;
	}
#endif
}

CDirectory::~CDirectory()
{
	if (IsValid())
	{
#if defined PLATFORM_WINDOWS
		FindClose(m_dir);
#else if defined PLATFORM_POSIX
		closedir(m_dir);
#endif
	}
}

void CDirectory::NextEntry()
{
#if defined PLATFORM_WINDOWS
	if (FindNextFile(m_dir, &m_fd) == 0)
	{
		FindClose(m_dir);
		m_dir = INVALID_HANDLE_VALUE;
	}
#else if defined PLATFORM_POSIX
	if ((ep=readdir(m_dir) == NULL)
	{
		closedir(m_dir);
		m_dir = NULL;
	}
#endif
}

bool CDirectory::IsEntryValid()
{
	return IsValid();
}

bool CDirectory::IsEntryDirectory()
{
#if defined PLATFORM_WINDOWS
	return ((m_fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY);
#else if defined PLATFORM_LINUX
	char temppath[PLATFORM_MAX_PATH];
	snprintf(temppath, sizeof(temppath), "%s/%s", m_origpath, GetEntryName());
	return g_LibrarySys.IsPathDirectory(temppath);
#endif
}

bool CDirectory::IsEntryFile()
{
#if defined PLATFORM_WINDOWS
	return !(m_fd.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_DEVICE));
#else if defined PLATFORM_POSIX
	char temppath[PLATFORM_MAX_PATH];
	snprintf(temppath, sizeof(temppath), "%s/%s", m_origpath, GetEntryName());
	return g_LibrarySys.IsPathFile(temppath);
#endif
}

const char *CDirectory::GetEntryName()
{
#if defined PLATFORM_WINDOWS
	return m_fd.cFileName;
#else if defined PLATFORM_LINUX
	return ep ? ep->d_name : "";
#endif
}

bool CDirectory::MoreFiles()
{
	return IsValid();
}

bool CDirectory::IsValid()
{
#if defined PLATFORM_WINDOWS
	return (m_dir != INVALID_HANDLE_VALUE);
#else if defined PLATFORM_LINUX
	return (m_dir != NULL);
#endif
}


/*************************
 ** Library System Code **
 *************************/


bool LibrarySystem::PathExists(const char *path)
{
#if defined PLATFORM_WINDOWS
	DWORD attr = GetFileAttributesA(path);

	return (attr != INVALID_FILE_ATTRIBUTES);
#else if defined PLATFORM_POSIX
	struct stat s;

	return (stat(path, &s) == 0);
#endif
}

bool LibrarySystem::IsPathFile(const char *path)
{
#if defined PLATFORM_WINDOWS
	DWORD attr = GetFileAttributes(path);

	if (attr == INVALID_FILE_ATTRIBUTES)
	{
		return false;
	}

	if (attr & (FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_DEVICE))
	{
		return false;
	}

	return true;
#else if defined PLATFORM_LINUX
	struct stat s;

	if (stat(path, &s) != 0)
	{
		return false;
	}

	return S_ISREG(s.st_mode) ? true : false;
#endif
}

bool LibrarySystem::IsPathDirectory(const char *path)
{
#if defined PLATFORM_WINDOWS
	DWORD attr = GetFileAttributes(path);

	if (attr == INVALID_FILE_ATTRIBUTES)
	{
		return false;
	}

	if (attr & FILE_ATTRIBUTE_DIRECTORY)
	{
		return true;
	}

#else if defined PLATFORM_LINUX
	struct stat s;

	if (stat(path, &s) != 0)
	{
		return false;
	}

	if (S_ISDIR(s.st_mode))
	{
		return true;
	}
#endif

	return false;
}

IDirectory *LibrarySystem::OpenDirectory(const char *path)
{
	CDirectory *dir = new CDirectory(path);

	if (!dir->IsValid())
	{
		delete dir;
		return NULL;
	}

	return dir;
}

void LibrarySystem::GetPlatformError(char *error, size_t err_max)
{
	if (error && err_max)
	{
#if defined PLATFORM_WINDOWS
		DWORD dw = GetLastError();
		FormatMessageA(
			FORMAT_MESSAGE_FROM_SYSTEM,
			NULL,
			dw,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPSTR)error,
			err_max,
			NULL);
#else if defined PLATFORM_POSIX
		snprintf(error, err_max, "%s", strerror(errno));
#endif
	}
}

void LibrarySystem::CloseDirectory(IDirectory *dir)
{
	delete dir;
}

ILibrary *LibrarySystem::OpenLibrary(const char *path, char *error, size_t err_max)
{
	LibraryHandle lib;
#if defined PLATFORM_WINDOWS
	lib = LoadLibraryA(path);
	if (!lib)
	{
		GetPlatformError(error, err_max);
		return false;
	}
#else if defined PLATFORM_POSIX
	lib = dlopen(path, RTLD_NOW);
	if (!lib)
	{
		GetPlatformError(error, err_max);
		return false;
	}
#endif

	return new CLibrary(lib);
}

size_t LibrarySystem::PathFormat(char *buffer, size_t len, const char *fmt, ...)
{
	va_list ap;
	va_start(ap,fmt);
	size_t mylen = vsnprintf(buffer, len, fmt, ap);
	va_end(ap);

	mylen = (mylen >= len) ? (len - 1) : mylen;

	for (size_t i=0; i<mylen; i++)
	{
		if (buffer[i] == PLATFORM_SEP_ALTCHAR)
		{
			buffer[i] = PLATFORM_SEP_CHAR;
		}
	}

	return mylen;
}
