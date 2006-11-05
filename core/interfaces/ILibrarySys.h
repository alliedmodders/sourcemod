#ifndef _INCLUDE_SOURCEMOD_LIBRARY_INTERFACE_SYS_H_
#define _INCLUDE_SOURCEMOD_LIBRARY_INTERFACE_SYS_H_

#include <IShareSys.h>

namespace SourceMod
{
	#define SMINTERFACE_LIBRARYSYS_NAME		"ILibrarySys"
	#define SMINTERFACE_LIBRARYSYS_VERSION	1

	class ILibrary
	{
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
		 * @param err_max	Maximum length of error buffer.
		 * @return			Pointer to an ILibrary, NULL if failed.
		 */
		virtual ILibrary *OpenLibrary(const char *path, char *error, size_t err_max) =0;

		/**
		 * @brief Opens a directory for reading.
		 *
		 * @param path		Path to directory.
		 * @param error		Buffer for any error message (may be NULL).
		 * @param err_max	Maximum length of error buffer.
		 * @return			Pointer to an IDirectory, NULL if failed.
		 */
		virtual IDirectory *OpenDirectory(const char *path, char *error, size_t err_max) =0;

		/**
		 * @brief Closes a directory and frees its handle.
		 * 
		 * @param dir		Pointer to IDirectory.
		 */
		virtual void CloseDirectory(IDirectory *dir) =0;

		/**
		 * @brief Returns true if the path is a normal file.
		 */
		virtual bool IsPathFile(const char *path) =0;

		/**
		 * @brief Returns true if the path is a normal directory.
		 */
		virtual bool IsPathDirectory(const char *path) =0;
	};
};

#endif //_INCLUDE_SOURCEMOD_LIBRARY_INTERFACE_SYS_H_
