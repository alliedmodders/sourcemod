/**
 * vim: set ts=4 :
 * =============================================================================
 * Source SDK Hooks Extension
 * Copyright (C) 2010-2012 Nicholas Hastings
 * Copyright (C) 2009-2010 Erik Minekus
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

#ifndef _INCLUDE_SMEXT_I_SDKHOOKS_H_
#define _INCLUDE_SMEXT_I_SDKHOOKS_H_

#include <IShareSys.h>

#define SMINTERFACE_SDKHOOKS_NAME		"ISDKHooks"
#define SMINTERFACE_SDKHOOKS_VERSION	1

class CBaseEntity;

/**
 * @brief SDKHooks shared API
 * @file ISDKHooks.h
 */

namespace SourceMod
{
	/**
	 * @brief Provides callbacks for entity events.
	 */
	class ISMEntityListener
	{
	public:
		/**
		 * @brief	When an entity is created
		 *
		 * @param	pEntity		CBaseEntity entity.
		 * @param	classname	Entity classname.
		 */
		virtual void OnEntityCreated(CBaseEntity *pEntity, const char *classname)
		{
		}
		
		/**
		 * @brief	When an entity is destroyed
		 *
		 * @param	pEntity		CBaseEntity entity.
		 */
		virtual void OnEntityDestroyed(CBaseEntity *pEntity)
		{
		}
	};

	/**
	 * @brief SDKHooks API.
	 */
	class ISDKHooks : public SMInterface
	{
	public:
		virtual const char *GetInterfaceName()
		{
			return SMINTERFACE_SDKHOOKS_NAME;
		}
		virtual unsigned int GetInterfaceVersion()
		{
			return SMINTERFACE_SDKHOOKS_VERSION;
		}
	public:
		/**
		 * @brief	Adds an entity listener.
		 *
		 * @param	listener		Pointer to an ISMEntityListener.
		 */
		virtual void AddEntityListener(ISMEntityListener *listener) =0;

		/**
		 * @brief	Removes an entity listener.
		 *
		 * @param	listener		Pointer to an ISMEntityListener.
		 */
		virtual void RemoveEntityListener(ISMEntityListener *listener) =0;
	};
}

#endif /* _INCLUDE_SMEXT_I_SDKHOOKS_H_ */
