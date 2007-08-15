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

#ifndef _INCLUDE_SOURCEMOD_INTERFACE_IPLAYERHELPERS_H_
#define _INCLUDE_SOURCEMOD_INTERFACE_IPLAYERHELPERS_H_

/**
 * @file IPlayerHelpers.h
 * @brief Defines basic helper functions for Half-Life 2 clients
 */

#include <IShareSys.h>
#include <IAdminSystem.h>

#define SMINTERFACE_PLAYERMANAGER_NAME		"IPlayerManager"
#define SMINTERFACE_PLAYERMANAGER_VERSION	1

struct edict_t;

namespace SourceMod
{
	/**
	 * @brief Abstracts some Half-Life 2 and SourceMod properties about clients.
	 */
	class IGamePlayer
	{
	public:
		/**
		 * @brief Returns the player's name.
		 *
		 * @return		String containing the player's name,
		 *				or NULL if unavailable.
		 */
		virtual const char *GetName() =0;

		/**
		 * @brief Returns the player's IP address.
		 *
		 * @return		String containing the player's IP address,
		 *				or NULL if unavailable.
		 */
		virtual const char *GetIPAddress() =0;

		/**
		 * @brief Returns the player's authentication string.
		 *
		 * @return		String containing the player's auth string.
		 *				May be NULL if unavailable.
		 */
		virtual const char *GetAuthString() =0;

		/**
		 * @brief Returns the player's edict_t structure.
		 *
		 * @return		edict_t pointer, or NULL if unavailable.
		 */
		virtual edict_t *GetEdict() =0;

		/**
		 * @brief Returns whether the player is in game (putinserver).
		 *
		 * @return		True if in game, false otherwise.
		 */
		virtual bool IsInGame() =0;

		/**
		 * @brief Returns whether the player is connected.
		 *
		 * Note: If this returns true, all above functions except for
		 * GetAuthString() should return non-NULL results.
		 *
		 * @return		True if connected, false otherwise.
		 */
		virtual bool IsConnected() =0;

		/**
		 * @brief Returns whether the player is a fake client.
		 *
		 * @return		True if a fake client, false otherwise.
		 */
		virtual bool IsFakeClient() =0;

		/**
		 * @brief Returns the client's AdminId, if any.
		 *
		 * @return		AdminId, or INVALID_ADMIN_ID if none.
		 */
		virtual AdminId GetAdminId() =0;

		/**
		 * @brief Sets the client's AdminId.
		 *
		 * @param id	AdminId to set.
		 * @param temp	If true, the id will be invalidated on disconnect.
		 */
		virtual void SetAdminId(AdminId id, bool temp) =0;
	};

	/**
	 * @brief Provides callbacks for important client events.
	 */
	class IClientListener
	{
	public:
	   /**
		* @brief Returns the current client listener version.
		*
		* @return		Client listener version.
		*/
		virtual unsigned int GetClientListenerVersion()
		{
			return SMINTERFACE_PLAYERMANAGER_VERSION;
		}
	public:
		/**
		 * @brief Called when a client requests connection.
		 *
		 * @param client		Index of the client.
		 * @param error			Error buffer for a disconnect reason.
		 * @param maxlength		Maximum length of error buffer.
		 * @return				True to allow client, false to reject.
		 */
		virtual bool InterceptClientConnect(int client, char *error, size_t maxlength)
		{
			return true;
		}

		/**
		 * @brief Called when a client has connected.
		 *
		 * @param client		Index of the client.
		 */
		virtual void OnClientConnected(int client)
		{
		}

		/**
		 * @brief Called when a client is put in server.
		 *
		 * @param client		Index of the client.
		 */
		virtual void OnClientPutInServer(int client)
		{
		}

		/**
		 * @brief Called when a client is disconnecting (not fully disconnected yet).
		 *
		 * @param client		Index of the client.
		 */
		virtual void OnClientDisconnecting(int client)
		{
		}

		/**
		 * @brief Called when a client has fully disconnected.
		 *
		 * @param client		Index of the client.
		 */
		virtual void OnClientDisconnected(int client)
		{
		}

		/**
		 * @brief Called when a client has received authorization.
		 *
		 * @param client		Index of the client.
		 * @param authstring	Authorization string.
		 */
		virtual void OnClientAuthorized(int client, const char *authstring)
		{
		}
	};

	class IPlayerManager : public SMInterface
	{
	public:
		const char *GetInterfaceName()
		{
			return SMINTERFACE_PLAYERMANAGER_NAME;
		}
		unsigned int GetInterfaceVersion()
		{
			return SMINTERFACE_PLAYERMANAGER_VERSION;
		}
	public:
		/**
		 * @brief Adds a client listener.
		 *
		 * @param listener		Pointer to an IClientListener.
		 */
		virtual void AddClientListener(IClientListener *listener) =0;

		/**
		 * @brief Removes a client listener.
		 *
		 * @param listener		Pointer to an IClientListener.
		 */
		virtual void RemoveClientListener(IClientListener *listener) =0;

		/**
		 * @brief Retrieves an IGamePlayer object by its client index.
		 *
		 * Note: This will return a valid object for any player, connected or not.
		 * Note: Client indexes start at 1, not 0.
		 *
		 * @param client		Index of the client.
		 * @return				An IGamePlayer pointer, or NULL if out of range.
		 */
		virtual IGamePlayer *GetGamePlayer(int client) =0;

		/**
		 * @brief Retrieves an IGamePlayer object by its edict_t pointer.
		 *
		 * @param pEdict		Index of the client
		 * @return				An IGamePlayer pointer, or NULL if out of range.
		 */
		virtual IGamePlayer *GetGamePlayer(edict_t *pEdict) =0;

		/**
		 * @brief Returns the maximum number of clients.
		 *
		 * @return				Maximum number of clients.
		 */
		virtual int GetMaxClients() =0;

		/**
		 * @brief Returns the number of players currently connected.
		 *
		 * @return				Current number of connected clients.
		 */
		virtual int GetNumPlayers() =0;

		/**
		 * @brief Returns the client index by its userid.
		 *
		 * @param userid		Userid of the client.
		 * @return				Client index, or 0 if invalid userid passed.
		 */
		virtual int GetClientOfUserId(int userid) =0;
	};
}

#endif //_INCLUDE_SOURCEMOD_INTERFACE_IPLAYERHELPERS_H_
