/**
 * vim: set ts=4 :
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

#ifndef _INCLUDE_SOURCEMOD_LIBRARY_INTERFACE_SYS_H_
#define _INCLUDE_SOURCEMOD_LIBRARY_INTERFACE_SYS_H_

/**
 * @file ILibrarySys.h
 * @brief Defines platform-dependent operations, such as opening libraries and files.
 */

#include <IShareSys.h>
#include <time.h>

namespace SourceMod
{
	#define SMINTERFACE_LIBRARYSYS_NAME		"ILibrarySys"
	#define SMINTERFACE_LIBRARYSYS_VERSION	5

	enum FileTimeType
	{
		FileTime_LastAccess = 0,	/* Last access (not available on FAT) */
		FileTime_Created = 1,		/* Creation (not available on FAT) */
		FileTime_LastChange = 2,	/* Last modification */
	};

	class ILibrary
	{
	public:
		/** Virtual destructor (calls CloseLibrary) */
		virtual ~ILibrary()
		{
		};
	public:
		/**
		 * @brief Closes dynamic library and invalidates pointer.
		 */
		virtual void CloseLibrary() =0;

		/**
		 * @brief Retrieves a symbol pointer from the dynamic library.
		 *
		 * @param symname	Symbol name.
		 * @return			Symbol pointer, NULL if not found.
		 */
		virtual void *GetSymbolAddress(const char *symname) =0;
	};

	/**
	 * @brief Directory browsing abstraction.
	 */
	class IDirectory
	{
	public:
		/** Virtual destructor */
		virtual ~IDirectory()
		{
		}
	public:
		/**
		 * @brief Returns true if there are more files to read, false otherwise.
		 */
		virtual bool MoreFiles() =0;

		/**
		 * @brief Advances to the next entry in the stream.
		 */
		virtual void NextEntry() =0;

		/**
		 * @brief Returns the name of the current entry.
		 */
		virtual const char *GetEntryName() =0;

		/**
		 * @brief Returns whether the current entry is a directory.
		 */
		virtual bool IsEntryDirectory() =0;

		/**
		 * @brief Returns whether the current entry is a file.
		 */
		virtual bool IsEntryFile() =0;

		/**
		 * @brief Returns true if the current entry is valid
		 *        (Used similarly to MoreFiles).
		 */
		virtual bool IsEntryValid() =0;
	};

	/**
	 * @brief Contains various operating system specific code.
	 */
	class ILibrarySys : public SMInterface
	{
	public:
		virtual const char *GetInterfaceName()
		{
			return SMINTERFACE_LIBRARYSYS_NAME;
		}
		virtual unsigned int GetInterfaceVersion()
		{
			return SMINTERFACE_LIBRARYSYS_VERSION;
		}
	public:
		/**
		 * @brief Opens a dynamic library file.
		 * 
		 * @param path		Path to library file (.dll/.so).
		 * @param error		Buffer for any error message (may be NULL).
		 * @param maxlength	Maximum length of error buffer.
		 * @return			Pointer to an ILibrary, NULL if failed.
		 */
		virtual ILibrary *OpenLibrary(const char *path, char *error, size_t maxlength) =0;

		/**
		 * @brief Opens a directory for reading.
		 *
		 * @param path		Path to directory.
		 * @return			Pointer to an IDirectory, NULL if failed.
		 */
		virtual IDirectory *OpenDirectory(const char *path) =0;

		/**
		 * @brief Closes a directory and frees its handle.
		 * 
		 * @param dir		Pointer to IDirectory.
		 */
		virtual void CloseDirectory(IDirectory *dir) =0;

		/**
		 * @brief Returns true if a path exists.
		 */
		virtual bool PathExists(const char *path) =0;

		/**
		 * @brief Returns true if the path is a normal file.
		 */
		virtual bool IsPathFile(const char *path) =0;

		/**
		 * @brief Returns true if the path is a normal directory.
		 */
		virtual bool IsPathDirectory(const char *path) =0;

		/**
		 * @brief Gets a platform-specific error message.
		 * This should only be called when an ILibrary function fails.
		 * Win32 equivalent: GetLastError() + FormatMessage()
		 * POSIX equivalent: errno + strerror()
		 *
		 * @param error		Error message buffer.
		 * @param maxlength	Maximum length of error buffer.
		 */
		virtual void GetPlatformError(char *error, size_t maxlength) =0;

		/**
		 * @brief Formats a string similar to snprintf(), except
		 * corrects all non-platform compatible path separators to be
		 * the correct platform character.
		 *
		 * @param buffer	Output buffer pointer.
		 * @param maxlength	Output buffer size.
		 * @param pathfmt	Format string of path.
		 * @param ...		Format string arguments.
		 */
		virtual size_t PathFormat(char *buffer, size_t maxlength, const char *pathfmt, ...) =0;

		/**
		 * @brief Returns a pointer to the extension in a filename.
		 *
		 * @param filename	Name of file from which the extension should be extracted.
		 * @return			Pointer to file extension.
		 */
		virtual const char *GetFileExtension(const char *filename) =0;

		/**
		 * @brief Creates a directory.
		 *
		 * @param path		Full, absolute path of the directory to create.
		 * @return			True on success, false otherwise.
		 */
		virtual bool CreateFolder(const char *path) =0;

		/**
		 * @brief Returns the requested timestamp of a file.
		 *
		 * NOTE: On FAT file systems, the access and creation times 
		 * may not be valid.
		 *
		 * @param path		Path to file.
		 * @param type		FileTimeType of time value to request.
		 * @param pTime		Pointer to store time.
		 * @return			True on success, false on failure.
		 */
		virtual bool FileTime(const char *path, FileTimeType type, time_t *pTime) =0;

		/**
		 * @brief Retrieve the file component of the given path.
		 *
		 * @param buffer	Output buffer.
		 * @param maxlength	Length of the output buffer.
		 * @param path		Path to search for a filename.
		 * @return			Number of bytes written to buffer, not including the null terminator.
		 */
		virtual size_t GetFileFromPath(char *buffer, size_t maxlength, const char *path) =0;
	};
}

#endif //_INCLUDE_SOURCEMOD_LIBRARY_INTERFACE_SYS_H_
