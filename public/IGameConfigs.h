/**
* vim: set ts=4 :
* ===============================================================
* SourceMod, Copyright (C) 2004-2007 AlliedModders LLC. 
* All rights reserved.
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

#ifndef _INCLUDE_SOURCEMOD_GAMECONFIG_SYSTEM_H_
#define _INCLUDE_SOURCEMOD_GAMECONFIG_SYSTEM_H_

#include <IShareSys.h>

/**
 * @file IGameConfig.h
 * @brief Abstracts game private data configuration.
 */

#define SMINTERFACE_GAMECONFIG_NAME			"IGameConfigManager"
#define SMINTERFACE_GAMECONFIG_VERSION		1

class SendProp;

namespace SourceMod
{
	/**
	 * @brief Details the property types.
	 */
	enum PropType
	{
		PropType_Unknown = 0,		/**< Property type is not known */
		PropType_Send = 1,			/**< Property type is a networkable property */
		PropType_Data = 2,			/**< Property type is a data/save property */
	};

	enum PropError
	{
		PropError_Okay = 0,			/**< No error */
		PropError_NotSet,			/**< Property is not set in the config file */
		PropError_NotFound,			/**< Property was not found in the game */
	};

	#define INVALID_PROPERTY_VALUE		-1		/**< Property value is not valid */

	/**
	 * @brief Describes a game private data config file
	 */
	class IGameConfig
	{
	public:
		/**
		 * @brief Returns an offset value.
		 *
		 * @param key			Key to retrieve from the offset section.
		 * @param value			Pointer to store the offset value in.
		 * @return				True if found, false otherwise.
		 */
		virtual bool GetOffset(const char *key, int *value) =0;

		/**
		 * @brief Returns information about a dynamic offset.
		 *
		 * @param key			Key to retrieve from the property section.
		 * @param value			Pointer to store the offset value.  Will be -1 on failure.
		 * @return				A PropError error code.
		 */
		virtual SendProp *GetSendProp(const char *key) =0;

		/**
		 * @brief Returns the value of a key from the "Keys" section.
		 *
		 * @param key			Key to retrieve from the Keys section.
		 * @return				String containing the value, or NULL if not found.
		 */
		virtual const char *GetKeyValue(const char *key) =0;
	};

	/**
	 * @brief Manages game config files
	 */
	class IGameConfigManager : public SMInterface
	{
	public:
		const char *GetInterfaceName()
		{
			return SMINTERFACE_GAMECONFIG_NAME;
		}
		unsigned int GetInterfaceVersion()
		{
			return SMINTERFACE_GAMECONFIG_VERSION;
		}
	public:
		/**
		 * @brief Loads or finds an already loaded game config file.  
		 *
		 * @param file		File to load.  The path must be relative to the 'gamedata'
		 *					folder under the config folder, and the extension should be ommitted.
		 * @param pConfig	Pointer to store the game config pointer.  Pointer will be valid even on failure.
		 * @param error		Optional error message buffer.
		 * @param maxlength	Maximum length of the error buffer.
		 * @return			True on success, false if the file failed.
		 */
		virtual bool LoadGameConfigFile(const char *file, IGameConfig **pConfig, char *error, size_t maxlength) =0;

		/**
		 * @brief Closes an IGameConfig pointer.  Since a file can be loaded more than once,
		 * the file will not actually be removed from memory until it is closed once for each
		 * call to LoadGameConfigfile().
		 *
		 * @param cfg		Pointer to the IGameConfig to close.
		 */
		virtual void CloseGameConfigFile(IGameConfig *cfg) =0;
	};
}

#endif //_INCLUDE_SOURCEMOD_GAMECONFIG_SYSTEM_H_
