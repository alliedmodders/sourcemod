/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */

#include <sys/stat.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sm_platform.h>
#include "LibrarySys.h"
#include <amtl/am-string.h>
#include <amtl/os/am-path.h>
#include <amtl/os/am-fsutil.h>

LibrarySystem g_LibSys;

CLibrary::CLibrary(ke::RefPtr<ke::SharedLib> lib)
 : lib_(lib)
{
}

void CLibrary::CloseLibrary()
{
	delete this;
}

void *CLibrary::GetSymbolAddress(const char *symname)
{
	return lib_->lookup(symname);
}


/********************
 ** Directory Code **
 ********************/


CDirectory::CDirectory(const char *path)
{
#if defined PLATFORM_WINDOWS
	char newpath[PLATFORM_MAX_PATH];
	ke::SafeSprintf(newpath, sizeof(newpath), "%s\\*.*", path);
	m_dir = FindFirstFileA(newpath, &m_fd);
	if (!IsValid())
	{
		m_fd.cFileName[0] = '\0';
	}
#elif defined PLATFORM_POSIX
	m_dir = opendir(path);
	if (IsValid())
	{
		/* :TODO: we need to read past "." and ".."! */
		ep = readdir(m_dir);
		ke::SafeStrcpy(m_origpath, PLATFORM_MAX_PATH, path);
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
#elif defined PLATFORM_POSIX
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
#elif defined PLATFORM_POSIX
	if ((ep=readdir(m_dir)) == NULL)
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
#elif defined PLATFORM_POSIX
	char temppath[PLATFORM_MAX_PATH];
	int ret = ke::SafeSprintf(temppath, sizeof(temppath), "%s/%s", m_origpath, GetEntryName());
	if (static_cast<size_t>(ret) >= sizeof(temppath))
		return false;
	return ke::file::IsDirectory(temppath);
#endif
}

bool CDirectory::IsEntryFile()
{
#if defined PLATFORM_WINDOWS
	return !(m_fd.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_DEVICE));
#elif defined PLATFORM_POSIX
	char temppath[PLATFORM_MAX_PATH];
	int ret = ke::SafeSprintf(temppath, sizeof(temppath), "%s/%s", m_origpath, GetEntryName());
	if (static_cast<size_t>(ret) >= sizeof(temppath))
		return false;
	return ke::file::IsFile(temppath);
#endif
}

const char *CDirectory::GetEntryName()
{
#if defined PLATFORM_WINDOWS
	return m_fd.cFileName;
#elif defined PLATFORM_POSIX
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
#elif defined PLATFORM_POSIX
	return (m_dir != NULL);
#endif
}


/*************************
 ** Library System Code **
 *************************/


bool LibrarySystem::PathExists(const char *path)
{
	return ke::file::PathExists(path);
}

bool LibrarySystem::IsPathFile(const char *path)
{
	return ke::file::IsFile(path);
}

bool LibrarySystem::IsPathDirectory(const char *path)
{
	return ke::file::IsDirectory(path);
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

void LibrarySystem::GetPlatformError(char *error, size_t maxlength)
{
#if defined PLATFORM_WINDOWS
	return GetPlatformErrorEx(GetLastError(), error, maxlength);
#elif defined PLATFORM_POSIX
	return GetPlatformErrorEx(errno, error, maxlength);
#endif
}

void LibrarySystem::GetPlatformErrorEx(int code, char *error, size_t maxlength)
{
	if (error && maxlength)
	{
#if defined PLATFORM_WINDOWS
		if (FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
						   NULL,
						   (DWORD)code,
						   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
						   (LPSTR)error,
						   maxlength,
						   NULL) == 0)
		{
			ke::SafeSprintf(error, maxlength, "error code %08x", code);
		}
#elif defined PLATFORM_LINUX
		const char *ae = strerror_r(code, error, maxlength);
		if (ae != error)
		{
			ke::SafeStrcpy(error, maxlength, ae);
		}
#elif defined PLATFORM_POSIX
		strerror_r(code, error, maxlength);
#endif
	}
}

void LibrarySystem::CloseDirectory(IDirectory *dir)
{
	delete dir;
}

ILibrary *LibrarySystem::OpenLibrary(const char *path, char *error, size_t maxlength)
{
	ke::RefPtr<ke::SharedLib> lib = ke::SharedLib::Open(path, error, maxlength);
	if (!lib)
		return nullptr;
	return new CLibrary(lib);
}

size_t LibrarySystem::PathFormat(char *buffer, size_t len, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	size_t mylen = ke::path::FormatVa(buffer, len, fmt, ap);
	va_end(ap);
	return mylen;
}

const char *LibrarySystem::GetFileExtension(const char *filename)
{
	size_t len, end;

	len = strlen(filename);

	/* Minimum string length for filename with ext would be 3; example: a.a */
	if (len < 3)
	{
		return NULL;
	}

	end = len - 1;

	for (size_t i = end; i <= end; i--)
	{
		if (filename[i] == PLATFORM_SEP_CHAR || filename[i] == PLATFORM_SEP_ALTCHAR)
		{
			break;
		}

		if (filename[i] == '.' && i != end && i != 0)
		{
			return &filename[++i];
		}
	}

	return NULL;
}

bool LibrarySystem::CreateFolder(const char *path)
{
	return ke::file::CreateDirectory(path, 0775);
}

size_t LibrarySystem::GetFileFromPath(char *buffer, size_t maxlength, const char *path)
{
	size_t length = strlen(path);

	for (size_t i = length - 1;
		 i <= length - 1;
		 i--)
	{
		if (path[i] == '/' 
#if defined PLATFORM_WINDOWS
			|| path[i] == '\\'
#endif
			)
		{
			return ke::SafeStrcpy(buffer, maxlength, &path[i+1]);
		}
	}

	/* We scanned and found no path separator */
	return ke::SafeStrcpy(buffer, maxlength, path);
}

bool LibrarySystem::FileTime(const char *path, FileTimeType type, time_t *pTime)
{
#ifdef PLATFORM_WINDOWS
	struct _stat s;
	if (_stat(path, &s) != 0)
#elif defined PLATFORM_POSIX
	struct stat s;
	if (stat(path, &s) != 0)
#endif
	{
		return false;
	}

	if (type == FileTime_LastAccess)
	{
		*pTime = s.st_atime;
	}
	else if (type == FileTime_Created)
	{
		*pTime = s.st_ctime;
	}
	else if (type == FileTime_LastChange)
	{
		*pTime = s.st_mtime;
	}

	return true;
}
