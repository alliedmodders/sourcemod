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

#ifndef _INCLUDE_SOURCEMOD_INTERFACE_IPLAYERHELPERS_H_
#define _INCLUDE_SOURCEMOD_INTERFACE_IPLAYERHELPERS_H_

#include <IShareSys.h>
#include <IAdminSystem.h>

#define SMINTERFACE_PLAYERMANAGER_NAME		"IPlayerManager"
#define SMINTERFACE_PLAYERMANAGER_VERSION	1

struct edict_t;

/**
 * @file IPlayerHelpers.h
 * @brief Defines basic helper functions for Half-Life 2 clients
 */
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
		 * @brief Called when a client has recieved authorization.
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
	};
}

#endif //_INCLUDE_SOURCEMOD_INTERFACE_IPLAYERHELPERS_H_
