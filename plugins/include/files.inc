/**
 * :TODO: license info
 */

#if defined _files_included
 #endinput
#endif
#define _files_included

/* @note All paths in SourceMod natives are relative to the mod folder unless otherwise noted. */
 
enum FileType
{
	FileType_Unknown = 0,	/* Unknown file type (device/socket) */
	FileType_Directory = 1,	/* File is a directory */
	FileType_File = 2,		/* File is a file */
};


/**
 * @brief Opens a directory/folder for contents enumeration.
 * @note Directories are closed with CloseHandle().
 * @note Directories Handles can be cloned.
 *
 * @param path			Path to open.
 * @return			A Handle to the directory, INVALID_HANDLE on open error.
 */
native Handle:OpenDirectory(const String:path[]);

/**
 * @brief Reads the current directory entry as a local filename, then moves to the next file.
 * @note Contents of buffers are undefined when returning false.
 * @note Both the '.' and '..' automatic directory entries will be retrieved for Windows and Linux.
 * 
 * @param dir			Handle to a directory.
 * @param buffer		String buffer to hold directory name.
 * @param maxlength		Maximum size of string buffer.
 * @param type			Optional variable to store the file type.
 * @return			True on success, false if there are no more files to read.
 * @error				Invalid or corrupt Handle. 
 */
native bool:ReadDirEntry(Handle:dir, String:buffer[], maxlength, &FileType:type=0);
