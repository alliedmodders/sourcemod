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

#ifndef _INCLUDE_SOURCEMOD_GAMEHELPERS_H_
#define _INCLUDE_SOURCEMOD_GAMEHELPERS_H_

#include <IShareSys.h>
#include <dt_send.h>
#include <server_class.h>
#include <datamap.h>
#include <edict.h>

#define SMINTERFACE_GAMEHELPERS_NAME		"IGameHelpers"
#define SMINTERFACE_GAMEHELPERS_VERSION		1

/**
 * @file IGameHelpers.h
 * @brief Provides Source helper functions.
 */

namespace SourceMod
{
	class IGameHelpers : public SMInterface
	{
	public:
		virtual const char *GetInterfaceName()
		{
			return SMINTERFACE_GAMEHELPERS_NAME;
		}
		virtual unsigned int GetInterfaceVersion()
		{
			return SMINTERFACE_GAMEHELPERS_VERSION;
		}
	public:
		/**
		 * @brief Finds a send property in a named send table.
		 *
		 * @param classname		Top-level sendtable name.
		 * @param offset		Property name.
		 * @return				SendProp pointer on success, NULL on failure.
		 */
		virtual SendProp *FindInSendTable(const char *classname, const char *offset) =0;

		/**
		 * @brief Finds a named server class.
		 *
		 * @return				ServerClass pointer on success, NULL on failure.
		 */
		virtual ServerClass *FindServerClass(const char *classname) =0;

		/**
		 * @brief Finds a datamap_t definition.
		 *
		 * @param pMap			datamap_t pointer.
		 * @param offset		Property name.
		 * @return				typedescription_t pointer on success, NULL 
		 *						on failure.
		 */
		virtual typedescription_t *FindInDataMap(datamap_t *pMap, const char *offset) =0;

		/**
		 * @brief Retrieves an entity's datamap_t pointer.
		 *
		 * @param pEntity		CBaseEntity entity.
		 * @return				datamap_t pointer, or NULL on failure.
		 */
		virtual datamap_t *GetDataMap(CBaseEntity *pEntity) =0;

		/**
		 * @brief Marks an edict as state changed for an offset.
		 *
		 * @param pEdict		Edict pointer.
		 * @param offset		Offset index.
		 */
		virtual void SetEdictStateChanged(edict_t *pEdict, unsigned short offset) =0;

		/**
		 * @brief Sends a text message to a client.
		 *
		 * @param client		Client index.
		 * @param dest			Destination on the HUD.
		 * @param msg			Message to send.
		 * @return				True on success, false on failure.
		 */
		virtual bool TextMsg(int client, int dest, const char *msg) =0;
	};
}

#endif //_INCLUDE_SOURCEMOD_GAMEHELPERS_H_
