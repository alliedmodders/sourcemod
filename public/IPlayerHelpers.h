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
#define SMINTERFACE_PLAYERMANAGER_VERSION	6

struct edict_t;
class IPlayerInfo;

#define SM_REPLY_CONSOLE	0			/**< Reply to console. */
#define SM_REPLY_CHAT		1			/**< Reply to chat. */

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

		/**
		 * @brief Returns the client's userid.
		 *
		 * @return		Userid.
		 */
		virtual int GetUserId() =0;

		/**
		 * @brief Returns the client's language id.
		 *
		 * @return		Language id.
		 */
		virtual unsigned int GetLanguageId() =0;

		/**
		 * @brief Returns a player's IPlayerInfo object, if any.
		 *
		 * @return		IPlayerInfo pointer, or NULL if none.
		 */
		virtual IPlayerInfo *GetPlayerInfo() =0;
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

		/**
		 * @brief Called when the server is activated.
		 */
		virtual void OnServerActivated(int max_clients)
		{
		}
	};

	#define COMMAND_FILTER_ALIVE		(1<<0)		/**< Only allow alive players */
	#define COMMAND_FILTER_DEAD			(1<<1)		/**< Only filter dead players */
	#define COMMAND_FILTER_CONNECTED	(1<<2)		/**< Allow players not fully in-game */
	#define COMMAND_FILTER_NO_IMMUNITY	(1<<3)		/**< Ignore immunity rules */
	#define COMMAND_FILTER_NO_MULTI		(1<<4)		/**< Do not allow multiple target patterns */
	#define COMMAND_FILTER_NO_BOTS		(1<<5)		/**< Do not allow bots to be targetted */

	#define COMMAND_TARGET_VALID		1			/**< Client passed the filter */
	#define COMMAND_TARGET_NONE			0			/**< No target was found */
	#define COMMAND_TARGET_NOT_ALIVE	-1			/**< Single client is not alive */
	#define COMMAND_TARGET_NOT_DEAD		-2			/**< Single client is not dead */
	#define COMMAND_TARGET_NOT_IN_GAME	-3			/**< Single client is not in game */
	#define COMMAND_TARGET_IMMUNE		-4			/**< Single client is immune */
	#define COMMAND_TARGET_EMPTY_FILTER	-5			/**< A multi-filter (such as @all) had no targets */
	#define COMMAND_TARGET_NOT_HUMAN	-6			/**< Target was not human */
	#define COMMAND_TARGET_AMBIGUOUS	-7			/**< Partial name had too many targets */

	#define COMMAND_TARGETNAME_RAW		0			/**< Target name is a raw string */
	#define COMMAND_TARGETNAME_ML		1			/**< Target name is a multi-lingual phrase */

	/**
	 * @brief Holds the many command target info parameters.
	 */
	struct cmd_target_info_t
	{
		const char *pattern;			/**< IN: Target pattern string. */
		int admin;						/**< IN: Client admin index, or 0 if server .*/
		cell_t *targets;				/**< IN: Array to store targets. */
		cell_t max_targets;				/**< IN: Max targets (always >= 1) */
		int flags;						/**< IN: COMMAND_FILTER flags. */
		char *target_name;				/**< OUT: Buffer to store target name. */
		size_t target_name_maxlength;	/**< IN: Maximum length of the target name buffer. */
		int target_name_style;			/**< OUT: Target name style (COMMAND_TARGETNAME) */
		int reason;						/**< OUT: COMMAND_TARGET reason. */
		unsigned int num_targets;		/**< OUT: Number of targets. */
	};

	/**
	 * @brief Intercepts a command target operation.
	 */
	class ICommandTargetProcessor
	{
	public:
		/**
		 * @brief Must process the command target and return a COMMAND_TARGET value.
		 *
		 * @param info			Struct containing command target information.
		 *						Any members labelled OUT must be filled if processing 
		 *						is to be completed (i.e. true returned).
		 * @return				True to end processing, false to let Core continue.
		 */
		virtual bool ProcessCommandTarget(cmd_target_info_t *info) =0;
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
		 * Note: this will not work until the server is activated.
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

		/**
		 * @brief Returns whether or not the server is activated.
		 *
		 * @return				True if ServerActivate() has been called
		 *						at least once, false otherwise.
		 */
		virtual bool IsServerActivated() =0;

		/**
		 * @brief Gets SourceMod's reply source.
		 *
		 * @return				ReplyTo source.
		 */
		virtual unsigned int GetReplyTo() =0;

		/**
		 * @brief Sets SourceMod's reply source.
		 *
		 * @param reply			Reply source.
		 * @return				Old reply source.
		 */
		virtual unsigned int SetReplyTo(unsigned int reply) =0;

		/**
		 * @brief Tests if a player meets command filtering rules.
		 *
		 * @param pAdmin		IGamePlayer of the admin, or NULL if the server. 
		 * @param pTarget		IGamePlayer of the player being targeted.
		 * @param flags			COMMAND_FILTER flags.
		 * @return				COMMAND_TARGET value.
		 */
		virtual int FilterCommandTarget(IGamePlayer *pAdmin, IGamePlayer *pTarget, int flags) =0;

		/**
		 * @brief Registers a command target processor.
		 *
		 * @param pHandler		Pointer to an ICommandTargetProcessor instance.
		 */
		virtual void RegisterCommandTargetProcessor(ICommandTargetProcessor *pHandler) =0;

		/**
		 * @brief Removes a command target processor.
		 *
		 * @param pHandler		Pointer to an ICommandTargetProcessor instance.
		 */
		virtual void UnregisterCommandTargetProcessor(ICommandTargetProcessor *pHandler) =0;
	};
}

#endif //_INCLUDE_SOURCEMOD_INTERFACE_IPLAYERHELPERS_H_
