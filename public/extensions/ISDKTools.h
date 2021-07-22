/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod SDKTools Extension
 * Copyright (C) 2004-2009 AlliedModders LLC.  All rights reserved.
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

#ifndef _INCLUDE_SMEXT_I_SDKTOOLS_H_
#define _INCLUDE_SMEXT_I_SDKTOOLS_H_

#include <IShareSys.h>

#define SMINTERFACE_SDKTOOLS_NAME		"ISDKTools"
#define SMINTERFACE_SDKTOOLS_VERSION	3

class IServer;

/**
 * @brief SDKTools shared API
 * @file ISDKTools.h
 */

namespace SourceMod
{
	/**
	 * @brief Manager for temporary entities.
	 */
	class ISDKTempEntityManager
	{
	public:
		/**
		 * @brief Wraps information about a temp entity.
		 */
		class ISDKTempEntityInfo
		{
		public:
			/**
			 * @brief Gets the type of the described TE.
			 *
			 * @return			Name of the TE's type.
			 */
			virtual const char* GetName() = 0;

			/**
			 * @brief Sets an integer value in the given temp entity.
			 *
			 * @param name		Name of the property to set.
			 * @param value		Property value.
			 * @return			True, if the property exists.
			 */
			virtual bool TE_SetEntData(const char* name, const int value) = 0;

			/**
			 * @brief Sets a float value in the given temp entity.
			 *
			 * @param name		Name of the property to set.
			 * @param value		Property value.
			 * @return			True, if the property exists.
			 */
			virtual bool TE_SetEntDataFloat(const char* name, const float value) = 0;

			/**
			 * @brief Sets a vector value in the given temp entity.
			 *
			 * @param name		Name of the property to set.
			 * @param value		Property value.
			 * @return			True, if the property exists.
			 */
			virtual bool TE_SetEntDataVector(const char* name, const float vector[3]) = 0;

			/**
			 * @brief Sets a float array value in the given temp entity.
			 *
			 * @param name		Name of the property to set.
			 * @param array		Property value array.
			 * @param size		Number of elements in array.
			 * @return			True, if the property exists.
			 */
			virtual bool TE_SetEntDataFloatArray(const char* name, const float* array, const int size) = 0;

			/**
			 * @brief Send the temp entity.
			 *
			 * @param filter	Filter object for clients.
			 * @param delay		Delay in seconds to send the TE.
			 */
			virtual void TE_Send(IRecipientFilter& filter, const float delay) = 0;
		};

		/**
		 * @brief Checks if the temp entity system is available.
		 *
		 * @return			True if the temp entity system is available.
		 */
		virtual bool IsAvailable() = 0;

		/**
		 * @brief Creates a new ISDKTempEntityInfo pointer based on the temp entities name.
		 *
		 * @param name		Name of the TE type.
		 * @return			ISDKTempEntityInfo pointer.
		 */
		virtual ISDKTempEntityInfo* GetTempEntityInfo(const char* name) = 0;
	};
	
	/**
	 * @brief SDKTools API.
	 */
	class ISDKTools : public SMInterface
	{
	public:
		virtual const char *GetInterfaceName() = 0;
		virtual unsigned int GetInterfaceVersion() = 0;
	public:
		/**
		 * @brief Returns a pointer to IServer if one was found.
		 *
		 * @return			IServer pointer, or NULL if SDKTools was unable to find one.
		 */
		virtual IServer* GetIServer() = 0;
		
		/**
		 * @brief Returns a pointer to game's CGameRules class.
		 *
		 * @return			CGameRules pointer or NULL if not found.
		 */
		virtual void* GetGameRules() = 0;

		/**
		 * @brief Returns a pointer to SDKTools' ISDKTempEntityManager.
		 *
		 * @return			ISDKTempEntityManager pointer.
		 */
		virtual ISDKTempEntityManager* GetTempEntityManager() = 0;
	};
}

#endif /* _INCLUDE_SMEXT_I_SDKTOOLS_H_ */
