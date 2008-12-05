/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod (C)2004-2008 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This file is part of the SourceMod/SourcePawn SDK.
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

#if defined _files_included
 #endinput
#endif
#define _files_included

/**
 * @global All paths in SourceMod natives are relative to the mod folder 
 * unless otherwise noted.
 *
 * Most functions in SourceMod (at least, ones that deal with direct 
 * file manipulation) will support an alternate path specification.
 *
 * If the path starts with the string "file://" and the PathType is 
 * not relative, then the "file://" portion is stripped off, and the 
 * rest of the path is used without any modification (except for 
 * correcting slashes).  This can be used to override the path 
 * builder to supply alternate absolute paths.  Examples:
 *
 * file://C:/Temp/file.txt
 * file:///tmp/file.txt
 */

/**
 * File inode types.
 */
enum FileType
{
	FileType_Unknown = 0,	/* Unknown file type (device/socket) */
	FileType_Directory = 1,	/* File is a directory */
	FileType_File = 2,		/* File is a file */
};

/**
 * File time modes.
 */
enum FileTimeMode
{
	FileTime_LastAccess = 0,	/* Last access (does not work on FAT) */
	FileTime_Created = 1,		/* Creation (does not work on FAT) */
	FileTime_LastChange = 2,	/* Last modification */
};

#define PLATFORM_MAX_PATH		256		/**< Maximum path length. */

#define SEEK_SET 0						/**< Seek from start. */
#define SEEK_CUR 1						/**< Seek from current position. */
#define SEEK_END 2						/**< Seek from end position. */

/**
 * Path types.
 */
enum PathType
{
	Path_SM,		/**< SourceMod root folder */	
};

/**
 * Builds a path relative to the SourceMod folder.  This should be used instead of 
 * directly referencing addons/sourcemod, in case users change the name of their 
 * folder layout.
 *
 * @param type			Type of path to build as the base.
 * @param buffer		Buffer to store the path.
 * @param maxlength		Maximum length of buffer.
 * @param fmt			Format string.
 * @param ...			Format arguments.
 * @return				Number of bytes written to buffer (not including null terminator).
 */
native BuildPath(PathType:type, String:buffer[], maxlength, const String:fmt[], any:...);

/**
 * Opens a directory/folder for contents enumeration.
 *
 * @note Directories are closed with CloseHandle().
 * @note Directories Handles can be cloned.
 * @note OpenDirectory() supports the "file://" notation.
 *
 * @param path			Path to open.
 * @return				A Handle to the directory, INVALID_HANDLE on open error.
 */
native Handle:OpenDirectory(const String:path[]);

/**
 * Reads the current directory entry as a local filename, then moves to the next file.
 *
 * @note Contents of buffers are undefined when returning false.
 * @note Both the '.' and '..' automatic directory entries will be retrieved for Windows and Linux.
 * 
 * @param dir			Handle to a directory.
 * @param buffer		String buffer to hold directory name.
 * @param maxlength		Maximum size of string buffer.
 * @param type			Optional variable to store the file type.
 * @return				True on success, false if there are no more files to read.
 * @error				Invalid or corrupt Handle. 
 */
native bool:ReadDirEntry(Handle:dir, String:buffer[], maxlength, &FileType:type=FileType_Unknown);

/**
 * Opens a file.
 *
 * @note Files are closed with CloseHandle().
 * @note File Handles can be cloned.
 * @note OpenFile() supports the "file://" notation.
 *
 * @param file			File to open.
 * @param mode			Open mode.
 * @return				A Handle to the file, INVALID_HANDLE on open error.
 */
native Handle:OpenFile(const String:file[], const String:mode[]);

/**
 * Deletes a file.
 *
 * @param path			Path of the file to delete.
 * @return				True on success, false otherwise.
 */
native bool:DeleteFile(const String:path[]);

/**
 * Reads a line from a text file.
 *
 * @param hndl			Handle to the file.
 * @param buffer		String buffer to hold the line.
 * @param maxlength		Maximum size of string buffer.	
 * @return				True on success, false otherwise.
 */
native bool:ReadFileLine(Handle:hndl, String:buffer[], maxlength);

/**
 * Reads binary data from a file.
 *
 * @param hndl			Handle to the file.
 * @param items			Array to store each item read.  
 * @param num_items		Number of items to read into the array.
 * @param size			Size of each element, in bytes, to be read.
 *						Valid sizes are 1, 2, or 4.
 * @return				Number of elements read, or -1 on error.
 */
native ReadFile(Handle:hndl, items[], num_items, size);

/**
 * Reads a UTF8 or ANSI string from a file.
 *
 * @param hndl			Handle to the file.
 * @param buffer		Buffer to store the string.
 * @param max_size		Maximum size of the string buffer.
 * @param read_count	If -1, reads until a null terminator is encountered in 
 *						the file.  Otherwise, read_count bytes are read 
 *						into the buffer provided.  In this case the buffer 
 *						is not explicitly null terminated, and the buffer 
 *						will contain any null terminators read from the file.
 * @return				Number of characters written to the buffer, or -1 
 *						if an error was encountered.
 * @error				Invalid Handle, or read_count > max_size.
 */
native ReadFileString(Handle:hndl, String:buffer[], max_size, read_count=-1);

/**
 * Writes binary data to a file.
 *
 * @param hndl			Handle to the file.
 * @param items			Array of items to write.  The data is read directly. 
 *						That is, in 1 or 2-byte mode, the lower byte(s) in 
 *						each cell are used directly, rather than performing 
 *						any casts from a 4-byte number to a smaller number.
 * @param num_items		Number of items in the array.
 * @param size			Size of each item in the array in bytes.  
 *						Valid sizes are 1, 2, or 4.
 * @return				True on success, false on error.
 * @error				Invalid Handle.
 */
native bool:WriteFile(Handle:hndl, const items[], num_items, size);

/**
 * Writes a binary string to a file.
 *
 * @param hndl			Handle to th efile.
 * @param buffer		String to write.
 * @param term			True to append NUL terminator, false otherwise.
 * @return				True on success, false on error.
 * @error				Invalid Handle.
 */
native bool:WriteFileString(Handle:hndl, const String:buffer[], bool:term);

/**
 * Writes a line of text to a text file.  A newline is automatically appended.
 *
 * @param hndl			Handle to the file.
 * @param format		Formatting rules.
 * @param ...			Variable number of format parameters.
 * @return				True on success, false otherwise.
 * @error				Invalid Handle.
 */
native bool:WriteFileLine(Handle:hndl, const String:format[], any:...);

/**
 * Reads a single binary cell from a file.
 *
 * @param hndl			Handle to the file.
 * @param data			Variable to store the data read.
 * @param size			Size of the data to read in bytes.  Valid 
 *						sizes are 1, 2, or 4 bytes.
 * @return				Number of elements read (max 1), or -1 on error.
 * @error				Invalid Handle.
 */
stock ReadFileCell(Handle:hndl, &data, size)
{
	new array[1], ret;
	
	if ((ret = ReadFile(hndl, array, 1, size)) == 1)
	{
		data = array[0];
	}
	
	return ret;
}

/**
 * Writes a single binary cell to a file.
 *
 * @param hndl			Handle to the file.
 * @param data			Cell to write to the file.
 * @param size			Size of the data to read in bytes.  Valid 
 *						sizes are 1, 2, or 4 bytes.  If the size 
 *						is less than 4 bytes, the data is truncated 
 *						rather than casted.  That is, only the lower 
 *						bits will be read.
 * @return				True on success, false on error.
 * @error				Invalid Handle.
 */
stock bool:WriteFileCell(Handle:hndl, data, size)
{
	new array[1];
	
	array[0] = data;
	
	return WriteFile(hndl, array, 1, size);
}
 
/**
 * Tests if the end of file has been reached.
 *
 * @param file			Handle to the file.
 * @return				True if end of file has been reached, false otherwise.
 * @error				Invalid Handle.
 */
native bool:IsEndOfFile(Handle:file);

/**
 * Sets the file position indicator.
 *
 * @param file			Handle to the file.
 * @param position		Position relative to what is specified in whence.
 * @param where			SEEK_ constant value of where to see from.
 * @return				True on success, false otherwise.
 * @error				Invalid Handle.
 */
native bool:FileSeek(Handle:file, position, where);

/**
 * Get current position in the file.
 *
 * @param file			Handle to the file.
 * @return				Value for the file position indicator.
 * @error				Invalid Handle.
 */
native FilePosition(Handle:file);

/**
 * Checks if a file exists.
 *
 * @param path			Path to the file.
 * @param use_valve_fs	If true, the Valve file system will be used instead.
 *						This can be used to check for the existance of files 
 *						inside GCFs or the game cache, rather than solely files 
 *						that are on disk.
 * @return				True if the file exists, false otherwise.
 */
native bool:FileExists(const String:path[], bool:use_valve_fs=false);

/**
 * Renames a file.
 *
 * @param newpath		New path to the file.
 * @param oldpath		Path to the existing file.
 * @return				True on success, false otherwise.
 */
native bool:RenameFile(const String:newpath[], const String:oldpath[]);

/**
 * Checks if a directory exists.
 *
 * @param path			Path to the directory.
 * @return				True if the directory exists, false otherwise.
 */
native bool:DirExists(const String:path[]);

/**
 * Get the file size in bytes.
 *
 * @param path			Path to the file.
 * @return				File size in bytes, -1 if file not found.
 */
native FileSize(const String:path[]);

/**
 * Flushes a file's buffered output; any buffered output
 * is immediately written to the file.
 *
 * @param file			Handle to the file.
 * @return				True on success, false on failure.
 */
native FlushFile(Handle:file);

/**
 * Removes a directory.
 * @note On most Operating Systems you cannot remove a directory which has files inside it.
 *
 * @param path			Path to the directory.
 * @return				True on success, false otherwise.
 */
native bool:RemoveDir(const String:path[]);

#define FPERM_U_READ		0x0100	/* User can read. */
#define FPERM_U_WRITE		0x0080	/* User can write. */
#define FPERM_U_EXEC		0x0040	/* User can exec. */
#define FPERM_G_READ		0x0020	/* Group can read. */
#define FPERM_G_WRITE		0x0010	/* Group can write. */
#define FPERM_G_EXEC		0x0008	/* Group can exec. */
#define FPERM_O_READ		0x0004	/* Anyone can read. */
#define FPERM_O_WRITE		0x0002	/* Anyone can write. */
#define FPERM_O_EXEC		0x0001	/* Anyone can exec. */

/**
 * Creates a directory.
 *
 * @param path			Path to create.
 * @param mode			Permissions (default is o=rx,g=rx,u=rwx).  Note that folders must have 
 *						the execute bit set on Linux.  On Windows, the mode is ignored.
 */
native bool:CreateDirectory(const String:path[], mode);

/**
 * Returns a file timestamp as a unix timestamp.
 *
 * @param file			File name.
 * @param tmode			Time mode.
 * @return				Time value, or -1 on failure.
 */
native GetFileTime(const String:file[], FileTimeMode:tmode);

/**
 * Same as LogToFile(), except uses an open file Handle.  The file must 
 * be opened in text appending mode.
 *
 * @param hndl			Handle to the file.
 * @param message		Message format.
 * @param ...			Message format parameters.
 * @noreturn
 * @error				Invalid Handle.
 */
native LogToOpenFile(Handle:hndl, const String:message[], any:...);

/**
 * Same as LogToFileEx(), except uses an open file Handle.  The file must 
 * be opened in text appending mode.
 *
 * @param hndl			Handle to the file.
 * @param message		Message format.
 * @param ...			Message format parameters.
 * @noreturn
 * @error				Invalid Handle.
 */
native LogToOpenFileEx(Handle:hndl, const String:message[], any:...);

