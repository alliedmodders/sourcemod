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

#define PLATFORM_MAX_PATH		256

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

enum PathType
{
	Path_SM,		/* SourceMod root folder */	
};

/**
 * @brief Builds a path relative to the SourceMod folder.
 *
 * @param type			Type of path to build as the base.
 * @param buffer		Buffer to store the path.
 * @param maxlength		Maximum length of buffer.
 * @param fmt			Format string.
 * @param ...			Format arguments.
 * @return				Number of bytes written to buffer (not including null terminator).
 */
native BuildPath(PathType:type, String:buffer[], maxlength, const String:fmt[], ...);

/**
 * @brief Opens a directory/folder for contents enumeration.
 * @note Directories are closed with CloseHandle().
 * @note Directories Handles can be cloned.
 *
 * @param path			Path to open.
 * @return				A Handle to the directory, INVALID_HANDLE on open error.
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
 * @return				True on success, false if there are no more files to read.
 * @error				Invalid or corrupt Handle. 
 */
native bool:ReadDirEntry(Handle:dir, String:buffer[], maxlength, &FileType:type=FileType_Unknown);

/**
 * @brief Opens a file.
 * @note Files are closed with CloseHandle().
 * @note File Handles can be cloned.
 *
 * @param file		File to open.
 * @param mode		Open mode.
 * @return			A Handle to the file, INVALID_HANDLE on open error.
 */
native Handle:OpenFile(const String:file[], const String:mode[]);

/**
 * @brief Deletes a file.
 *
 * @param path		Path of the file to delete.
 * @return			True on success, false otherwise.
 */
native bool:DeleteFile(const String:path[]);

/**
 * @brief Reads a line from a text file.
 *
 * @param hndl			Handle to the file.
 * @param buffer		String buffer to hold the line.
 * @param maxlength		Maximum size of string buffer.	
 * @return				True on success, false otherwise.
 */
native bool:ReadFileLine(Handle:hndl, String:buffer[], maxlength);

/**
 * @brief Tests if the end of file has been reached.
 *
 * @param file		Handle to the file.
 * @return			True if end of file has been reached, false otherwise.
 */
native bool:IsEndOfFile(Handle:file);

/**
 * @brief Sets the file position indicator.
 *
 * @param file		Handle to the file.
 * @param position	Position relative to what is specified in whence.
 * @param whence	Look at the SEEK_* definitions.
 * @return			True on success, false otherwise.
 */
native bool:FileSeek(Handle:file, position, whence);

/**
 * @brief Get current position in the file.
 *
 * @param file		Handle to the file.
 * @return			Value for the file position indicator.
 */
native FilePosition(Handle:file);

/**
 * @brief Checks if a file exists.
 *
 * @param path		Path to the file.
 * @return			True if the file exists, false otherwise.
 */
native bool:FileExists(const String:path[]);

/**
 * @brief Renames a file.
 *
 * @param newpath		New path to the file.
 * @param oldpath		Path to the existing file.
 * @return				True on success, false otherwise.
 */
native bool:RenameFile(const String:newpath[], const String:oldpath[]);

/**
 * @brief Checks if a directory exists.
 *
 * @param path		Path to the directory.
 * @return			True if the directory exists, false otherwise.
 */
native bool:DirExists(const String:path[]);

/**
 * @brief Get the file size in bytes.
 *
 * @param path		Path to the file.
 * @return			File size in bytes, -1 if file not found.
 */
native FileSize(const String:path[]);

/**
 * @brief Removes a directory.
 * @note On most Operating Systems you cannot remove a directory which has files inside it.
 *
 * @param path		Path to the directory.
 * @return			True on success, false otherwise.
 */
native bool:RemoveDir(const String:path[]);

/**
 * @brief Writes a line of text in a file.
 * @note This native will append the newline character.
 *
 * @param hndl		Handle to the file.
 * @param format	Formatting rules.
 * @param ...		Variable number of format parameters.
 * @return			True on success, false otherwise.
 */
native bool:WriteFileLine(Handle:hndl, const String:format[], {Handle,Float,String,_}:...);
