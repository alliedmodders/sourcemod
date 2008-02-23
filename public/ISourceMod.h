/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2007 AlliedModders LLC.  All rights reserved.
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

#ifndef _INCLUDE_SOURCEMOD_MAIN_HELPER_INTERFACE_H_
#define _INCLUDE_SOURCEMOD_MAIN_HELPER_INTERFACE_H_

/**
 * @file ISourceMod.h
 * @brief Defines miscellaneous helper functions useful to extensions.
 */

#include <IHandleSys.h>
#include <sp_vm_api.h>
#include <IDataPack.h>
#include <time.h>

#define SMINTERFACE_SOURCEMOD_NAME		"ISourceMod"
#define SMINTERFACE_SOURCEMOD_VERSION	5

/**
* @brief Forward declaration of the KeyValues class.
*/
class KeyValues;

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
		 * @brief Returns the full path to the game directory.
		 *
		 * @return			A string containing the full game path.
		 */
		virtual const char *GetGamePath() const =0;

		/**
		 * @brief Returns the full path to the SourceMod directory.
		 *
		 * @return			A string containing the full SourceMod path.
		 */
		virtual const char *GetSourceModPath() const =0;

		/**
		 * @brief Builds a platform path for a specific target base path.
		 *
		 * If the path starts with the string "file://" and the PathType is 
		 * not relative, then the "file://" portion is stripped off, and the 
		 * rest of the path is used without any modification (except for 
		 * correcting slashes).  This can be used to override the path 
		 * builder to supply alternate absolute paths.  Examples:
		 *
		 * file://C:/Temp/file.txt
		 * file:///tmp/file.txt
		 *
		 * @param type		Type of path to use as a base.
		 * @param buffer	Buffer to write to.
		 * @param maxlength	Size of buffer.
		 * @param format	Format string.
		 * @param ...		Format arguments.
		 * @return			Number of bytes written.
		 */
		virtual size_t BuildPath(PathType type, char *buffer, size_t maxlength, const char *format, ...) =0;

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
		 * @param maxlength		Maximum length of buffer (including null terminator).
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
		 * @brief Not implemented, do not use.
		 *
		 * @param readonly	Ignored
		 * @return			0
		 */
		virtual HandleType_t GetDataPackHandleType(bool readonly=false) =0;

		/**
		 * @brief Retrieves a KeyValues pointer from a handle.
		 *
		 * @param hndl		Handle_t from which to retrieve contents.
		 * @param err		Optional address to store a possible handle error.
		 * @param root		If true it will return the root KeyValues pointer for the whole structure.
		 *
		 * @return			The KeyValues pointer, or NULL for any error encountered.
		 */
		virtual KeyValues *ReadKeyValuesHandle(Handle_t hndl, HandleError *err=NULL, bool root=false) =0;

		/**
		 * @brief Returns the name of the game directory.
		 *
		 * @return			A string containing the name of the game directory.
		 */
		virtual const char *GetGameFolderName() const =0;

		/**
		 * @brief Returns the scripting engine interface.
		 *
		 * @return			A pointer to the scripting engine interface.
		 */
		virtual SourcePawn::ISourcePawnEngine *GetScriptingEngine() =0;

		/**
		 * @brief Returns the JIT interface.
		 *
		 * @return			A pointer to the JIT interface.
		 */
		virtual SourcePawn::IVirtualMachine *GetScriptingVM() =0;

		/**
		 * @brief Returns the adjusted server time.
		 *
		 * @return			Adjusted server time.
		 */
		virtual time_t GetAdjustedTime() =0;

		/**
		 * @brief Sets the global client SourceMod will use for assisted 
		 * translations (that is, %t).
		 *
		 * @param index		Client index.
		 * @return			Old global client value.
		 */
		virtual unsigned int SetGlobalTarget(unsigned int index) =0;

		/**
		 * @brief Returns the global client SourceMod is currently using 
		 * for assisted translations (that is, %t).
		 *
		 * @return			Global client value.
		 */
		virtual unsigned int GetGlobalTarget() const =0;
	};
}

#endif //_INCLUDE_SOURCEMOD_MAIN_HELPER_INTERFACE_H_
