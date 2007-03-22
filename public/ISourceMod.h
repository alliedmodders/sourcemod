/**
 * vim: set ts=4 :
 * ===============================================================
 * SourceMod (C)2004-2007 AlliedModders LLC.  All rights reserved.
 * ===============================================================
 *
 *  This file is part of the SourceMod/SourcePawn SDK.  This file may only be 
 * used or modified under the Terms and Conditions of its License Agreement, 
 * which is found in public/licenses/LICENSE.txt.  As of this notice, derivative 
 * works must be licensed under the GNU General Public License (version 2 or 
 * greater).  A copy of the GPL is included under public/licenses/GPL.txt.
 * 
 * To view the latest information, see: http://www.sourcemod.net/license.php
 *
 * Version: $Id$
 */

#ifndef _INCLUDE_SOURCEMOD_MAIN_HELPER_INTERFACE_H_
#define _INCLUDE_SOURCEMOD_MAIN_HELPER_INTERFACE_H_

/**
 * @file ISourceMod.h
 * @brief Defines miscellanious helper functions useful to extensions.
 */

#include <IShareSys.h>
#include <sp_vm_api.h>
#include <IDataPack.h>

#define SMINTERFACE_SOURCEMOD_NAME		"ISourceMod"
#define SMINTERFACE_SOURCEMOD_VERSION	1

namespace SourceMod
{
	/**
	 * @brief Describes various ways of formatting a base path.
	 */
	enum PathType
	{
		Path_None = 0,			/**< No base path */
		Path_Game,				/**< Base path is absolute mod folder */
		Path_SM,				/**< Base path is absolute to SourceMod */
		Path_SM_Rel,			/**< Base path is relative to SourceMod */
	};

	/**
	 * @brief Contains miscellaneous helper functions.
	 */
	class ISourceMod : public SMInterface
	{
	public:
		virtual const char *GetInterfaceName()
		{
			return SMINTERFACE_SOURCEMOD_NAME;
		}
		virtual unsigned int GetInterfaceVersion()
		{
			return SMINTERFACE_SOURCEMOD_VERSION;
		}
	public:
		/**
		 * @brief Returns the full path to the mod directory.
		 *
		 * @return			A string containing the full mod path.
		 */
		virtual const char *GetModPath() const =0;

		/**
		 * @brief Returns the full path to the SourceMod directory.
		 *
		 * @return			A string containing the full SourceMod path.
		 */
		virtual const char *GetSourceModPath() const =0;

		/**
		 * @brief Builds a platform path for a specific target base path.
		 *
		 * @param type		Type of path to use as a base.
		 * @param buffer	Buffer to write to.
		 * @param maxlength	Size of buffer.
		 * @param format	Format string.
		 * @param ...		Format arguments.
		 * @return			Number of bytes written.
		 */
		virtual size_t BuildPath(PathType type, char *buffer, size_t maxlength, char *format, ...) =0;

		/**
		 * @brief Logs a message to the SourceMod logs.
		 *
		 * @param pExt		Extension calling this function.
		 * @param format	Message format.
		 * @param ...		Message format parameters.
		 */
		virtual void LogMessage(IExtension *pExt, const char *format, ...) =0;

		/**
		 * @brief Logs a message to the SourceMod error logs.
		 *
		 * @param pExt		Extension calling this function.
		 * @param format	Message format.
		 * @param ...		Message format parameters.
		 */
		virtual void LogError(IExtension *pExt, const char *format, ...) =0;

		/**
		 * @brief Formats a string from a native.
		 *
		 * @param buffer		Buffer to store message.
		 * @param maxlength		Maximum length of buffer (inculding null terminator).
		 * @param pContext		Pointer to the plugin's context.
		 * @param params		Parameter array that was passed to the native.
		 * @param param			Parameter index where format string and variable arguments begin.
		 *						Note: parameter indexes start at 1.
		 * @return				Number of bytes written, not including the null terminator.
		 */
		virtual size_t FormatString(char *buffer, 
									size_t maxlength, 
									SourcePawn::IPluginContext *pContext,
									const cell_t *params,
									unsigned int param) =0;

		/**
		 * @brief Creates a data pack object.
		 *
		 * @return			A new IDataPack object.
		 */
		virtual IDataPack *CreateDataPack() =0;

		/**
		 * @brief Releases a data pack's resources so it can be re-used.
		 *
		 * @param pack		An IDataPack object to release.
		 */
		virtual void FreeDataPack(IDataPack *pack) =0;

		/**
		 * @brief Returns the automated data pack handle type.
		 *
		 * The readonly data type is the parent of the writable type.  
		 * Note that calling CloseHandle() on either type will release the data pack.
		 * The readonly type is inheritable, but due to limitations of the Handle System,
		 * the writable type is not.
		 *
		 * @param readonly	If true, the readonly type will be returned.
		 * @return			The Handle type for storing generic data packs.
		 */
		 virtual HandleType_t GetDataPackHandleType(bool readonly=false) =0;
	};
}

#endif //_INCLUDE_SOURCEMOD_MAIN_HELPER_INTERFACE_H_
