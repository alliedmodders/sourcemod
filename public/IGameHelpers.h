/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2016 AlliedModders LLC.  All rights reserved.
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

#ifndef _INCLUDE_SOURCEMOD_GAMEHELPERS_H_
#define _INCLUDE_SOURCEMOD_GAMEHELPERS_H_

#include <IShareSys.h>

/**
 * @file IGameHelpers.h
 * @brief Provides Source helper functions.
 */

#define SMINTERFACE_GAMEHELPERS_NAME		"IGameHelpers"
#define SMINTERFACE_GAMEHELPERS_VERSION		11

class CBaseEntity;
class CBaseHandle;
class SendProp;
class ServerClass;
class ICommandLine;
struct edict_t;
struct datamap_t;
struct typedescription_t;

#define TEXTMSG_DEST_NOTIFY  1
#define TEXTMSG_DEST_CONSOLE 2
#define TEXTMSG_DEST_CHAT    3
#define TEXTMSG_DEST_CENTER  4

namespace SourceMod
{
	/**
	 * @brief Maps the heirarchy of a SendProp.
	 */
	struct sm_sendprop_info_t
	{
		SendProp *prop;					/**< Property instance. */
		unsigned int actual_offset;		/**< Actual computed offset. */
	};
	
	struct sm_datatable_info_t
	{
		typedescription_t *prop;			/**< Property instance. */
		unsigned int actual_offset;		/**< Actual computed offset. */
	};

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
		 * @brief Deprecated; use FindSendPropInfo() instead.
		 *
		 * @param classname		Do not use.
		 * @param offset		Do not use.
		 * @return				Do not use.
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
		 * @param dest			Destination on the HUD (see TEXTMSG_DEST defines above).
		 * @param msg			Message to send.
		 * @return				True on success, false on failure.
		 */
		virtual bool TextMsg(int client, int dest, const char *msg) =0;
		
		/**
		 * @brief Returns whether the server ls a LAN server.
		 * 
		 * @return				True if LAN server, false otherwise.
		 */
		virtual bool IsLANServer() =0;

		/**
		 * @brief Finds a send property in a named ServerClass.
		 *
		 * This version, unlike FindInSendTable(), correctly deduces the 
		 * offsets of nested tables.
		 *
		 * @param classname		ServerClass name (such as CBasePlayer).
		 * @param offset		Offset name (such as m_iAmmo).
		 * @param info			Buffer to store sm_sendprop_info_t data.
		 * @return				True on success, false on failure.
		 */
		virtual bool FindSendPropInfo(const char *classname,
			const char *offset,
			sm_sendprop_info_t *info) =0;

		/**
		 * @brief Converts an entity index into an edict pointer.
		 *
		 * @param index			Entity Index.
		 * @return				Edict pointer or NULL on failure.
		 */
		virtual edict_t *EdictOfIndex(int index) =0;

		/**
		* @brief Converts an edict pointer into an entity index.
		*
		* @param index			Edict Pointer.
		* @return				Entity index or -1 on failure.
		*/
		virtual int IndexOfEdict(edict_t *pEnt) =0;

		/**
		 * @brief Retrieves the edict pointer from a CBaseHandle object.
		 *
		 * @param hndl			CBaseHandle object.
		 * @return				Edict pointer or NULL on failure.
		 */
		virtual edict_t *GetHandleEntity(CBaseHandle &hndl) =0;

		/**
		 * @brief Sets the edict pointer in a CBaseHandle object.
		 *
		 * @param hndl			CBaseHandle object.
		 * @param pEnt			Edict pointer.
		 */
		virtual void SetHandleEntity(CBaseHandle &hndl, edict_t *pEnt) =0;

		/**
		 * @brief Returns the current map name.
		 *
		 * @return				Current map name.
		 */
		virtual const char *GetCurrentMap() =0;

		/**
		 * @brief Wraps IVEngineServer::ServerCommand.
		 *
		 * @param buffer		Command buffer (does not auto \n terminate).
		 */
		virtual void ServerCommand(const char *buffer) =0;

		/**
		 * @brief Looks up a reference and returns the CBasseEntity* it points to.
		 *
		 * @note Supports 'old style' simple indexes and does a serial confirmation check on references.
		 *
		 * @param entRef		Entity reference.
		 * @return				Entity pointer.
		 */
		virtual CBaseEntity *ReferenceToEntity(cell_t entRef) =0;

		/**
		 * @brief Returns the entity reference for an entity.
		 *
		 * @param pEntity		Entity pointer.
		 * @return				Entity reference.
		 */
		virtual cell_t EntityToReference(CBaseEntity *pEntity) =0;

		/**
		 * @brief Returns the entity reference for logical entities, or the index for networked entities.
		 *
		 * @param pEntity		Entity pointer.
		 * @return				Entity reference/index.
		 */
		virtual cell_t EntityToBCompatRef(CBaseEntity *pEntity) =0;

		/**
		 * @brief Converts an entity index into an entity reference.
		 *
		 * @param entIndex		Entity index.
		 * @return				Entity reference.
		 */
		virtual cell_t IndexToReference(int entIndex) =0;

		/**
		 * @brief Retrieves the entity index from a reference.
		 *
		 * @param entRef		Entity reference.
		 * @return				Entity index.
		 */
		virtual int ReferenceToIndex(cell_t entRef) =0;

		/**
		 * @brief Converts a reference into a bcompat version (index for networked entities).
		 *
		 * @param entRef		Entity reference.
		 * @return				Entity reference/index.
		 */
		virtual cell_t ReferenceToBCompatRef(cell_t entRef) =0;
		
		/**
		 * @brief Returns the g_EntList pointer.
		 *
		 * @return				g_EntList pointer.
		 */
		virtual void *GetGlobalEntityList() =0;
		
		/**
		 * @brief Adds a client to the kick queue, where they will be kicked
		 * next game frame.
		 *
		 * The user ID is used to ensure the correct player is kicked.
		 *
		 * @param client		The index of the client to kick.
		 * @param userid		The user ID of the client to kick.
		 * @param msg			The kick message to show to the player.
		 */
		virtual void AddDelayedKick(int client, int userid, const char *msg) =0;

		/**
		 * @brief Returns the uncomputed offset of a SendProp.
		 *
		 * @param prop          SendProp pointer.
		 * @return              Uncomputed sendprop offset.
		 */
		virtual int GetSendPropOffset(SendProp *prop) =0;
		
		/**
		 * @brief Sends a hint message to a client.
		 *
		 * @param client		Client index.
		 * @param msg			Message to send.
		 * @return				True on success, false on failure.
		 */
		virtual bool HintTextMsg(int client, const char *msg) =0;

		/**
		 * @brief Retrieves the Valve command line pointer.
		 *
		 * @return				ICommandLine ptr or NULL if not found.
		 */
		virtual ICommandLine *GetValveCommandLine() =0;

		/**
		 * @brief Gets a Classname from an edict_t pointer.
		 *
		 * @param pEdict		edict_t pointer.
		 * @return				Pointer to the string, or NULL if bad pointer.
		 */
		virtual const char *GetEntityClassname(edict_t *pEdict) =0;

		/**
		 * @brief Gets a Classname from an CBaseEntity pointer.
		 *
		 * @param pEntity		CBaseEntity pointer.
		 * @return				Pointer to the string, or NULL if bad pointer.
		 */
		virtual const char *GetEntityClassname(CBaseEntity *pEntity) =0;

		/**
		 * @brief Returns whether or not a map name is valid to use with the
		 * engine's Changelevel functionality. It need not be an exact filename on
		 * some engines. For a check on the exact name, use IVEngineServer::IsMapValid.
		 *
		 * @param map			Map name.
		 * @return				True if valid, otherwise false.
		 */
		virtual bool IsMapValid(const char *map) =0;
		
		/**
		 * @brief Finds a datamap_t definition.
		 *
		 * @param pMap			datamap_t pointer.
		 * @param offset			Property name.
		 * @param pDataTable	Buffer to store sm_datatable_info_t data.
		 * @return				typedescription_t pointer on success, NULL 
		 *						on failure.
		 */
		virtual bool FindDataMapInfo(datamap_t *pMap, const char *offset, sm_datatable_info_t *pDataTable) =0;

		/**
		 * @brief Retrieves the server's rendered Steam3 id.
		 *
		 * @param pszOut		Buffer to which id will be copied.
		 * @param len			Max size of buffer in bytes.
		 * @return				True on id successfully retrieved and buffer populated
		 *						(even if id invalid/unset) false on failure (unknown id type).
		 */
		virtual bool GetServerSteam3Id(char *pszOut, size_t len) const =0;

		/**
		 * @brief Retrieves the server's SteamID64.
		 *
		 * @return				64-bit server Steam id.
		 */
		virtual uint64_t GetServerSteamId64() const =0;
	};
}

#endif //_INCLUDE_SOURCEMOD_GAMEHELPERS_H_
