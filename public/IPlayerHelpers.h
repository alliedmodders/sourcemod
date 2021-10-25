/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
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
#define SMINTERFACE_PLAYERMANAGER_VERSION	21

struct edict_t;
class IPlayerInfo;

#define SM_REPLY_CONSOLE	0			/**< Reply to console. */
#define SM_REPLY_CHAT		1			/**< Reply to chat. */

#define SM_MAXPLAYERS				65		/**< Maxplayer Count */

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
		 * @param		validated	Check backend validation status.
		 * @return		String containing the player's auth string.
		 *				May be NULL if unavailable.
		 */
		virtual const char *GetAuthString(bool validated = true) =0;

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

		/**
		 * @brief Runs through Core's admin authorization checks.  If the 
		 * client is already an admin, no checks are performed.  
		 *
		 * Note that this function operates solely against the in-memory admin 
		 * cache.  It will check steamids, IPs, names, and verify a password 
		 * if one exists.  To implement other authentication schemes, simply 
		 * don't call this function and use IGamePlayer::SetAdminId() instead.
		 *
		 * @return				True if access changed, false otherwise.
		 */
		virtual bool RunAdminCacheChecks() =0;

		/**
		 * @brief Notifies all listeners that the client has completed 
		 * all of your post-connection (in-game, auth, admin) checks.  
		 *
		 * If you returned "false" from OnClientPreAdminCheck(), you must 
		 * ALWAYS manually invoke this function, even if RunAdminCacheChecks() 
		 * failed or you did not assign an AdminId.  Failure to call this 
		 * function could result in plugins (such as reservedslots) not 
		 * working properly.
		 *
		 * If you are implementing asynchronous fetches, and the client 
		 * disconnects during your fetching process, you should make sure to 
		 * recognize that case and not call this function.  That is, do not 
		 * call this function on mismatched PreCheck calls, or on disconnected 
		 * clients.  A good way to check this is to pass userids around, which 
		 * are unique per client connection.
		 *
		 * Calling this has no effect if it has already been called on the 
		 * given client (thus it is safe for multiple asynchronous plugins to 
		 * call it at various times).
		 */
		virtual void NotifyPostAdminChecks() =0;

		/**
		 * @brief Returns the clients unique serial identifier.
		 *
		 * @return	Serial number.
		 */
		virtual unsigned int GetSerial() =0;

		/**
		 * @brief Return whether the client is authorized.
		 *
		 * @return		True if authorized, false otherwise.
		 */
		virtual bool IsAuthorized() =0;
		
		/**
		 * @brief Kicks the client with a message
		 *
		 * @param message   The message shown to the client when kicked
		 */
		virtual void Kick(const char *message) =0;
		
		/**
		 * @brief Returns whether the client is marked as being in the kick
		 * queue. The client doesn't necessarily have to be in the actual kick
		 * queue for this function to return true.
		 *
		 * @return      True if in the kick queue, false otherwise.
		 */
		virtual bool IsInKickQueue() =0;
		
		/**
		 * @brief Marks the client as being in the kick queue. They are not
		 * actually added to the kick queue. Use IGameHelpers::AddDelayedKick()
		 * to actually add them to the queue.
		 */
		virtual void MarkAsBeingKicked() =0;

		virtual void SetLanguageId(unsigned int id) =0;

		/**
		 * @brief Returns whether the player is the SourceTV bot.
		 *
		 * @return		True if the SourceTV bot, false otherwise.
		 */
		virtual bool IsSourceTV() const =0;
		
		/**
		 * @brief Returns whether the player is the Replay bot.
		 *
		 * @return		True if the Replay bot, false otherwise.
		 */
		virtual bool IsReplay() const =0;
		
		/**
		 * @brief Returns the client's Steam account ID.
		 *
		 * @param validated		Check backend validation status.
		 * 
		 * @return			Steam account ID or 0 if not available.
		 */
		virtual unsigned int GetSteamAccountID(bool validated = true) =0;

		/**
		 * @brief Returns the client's edict/entity index.
		 *
		 * @return			Client's index.
		 */
		virtual int GetIndex() const =0;
		
		/**
		 * @brief Prints a string to the client console.
		 *
		 * @param pMsg		String to print.
		 */
		virtual void PrintToConsole(const char *pMsg) =0;

		/**
		 * @brief Removes admin access from the client.
		 */
		virtual void ClearAdmin() =0;
		
		/**
		 * @brief Returns the client's Steam ID as a uint64.
		 *
		 * @param validated		Check backend validation status.
		 * 
		 * @return			Steam Id or 0 if not available.
		 */
		virtual uint64_t GetSteamId64(bool validated = true) =0;
		
		/**
		 * @brief Returns the client's Steam ID rendered in Steam2 format.
		 *
		 * @param validated		Check backend validation status.
		 * 
		 * @return			Steam2 Id on success or NULL if not available.
		 */
		virtual const char *GetSteam2Id(bool validated = true) =0;
		
		/**
		 * @brief Returns the client's Steam ID rendered in Steam3 format.
		 *
		 * @param validated		Check backend validation status.
		 * 
		 * @return			Steam3 Id on success or NULL if not available.
		 */
		virtual const char *GetSteam3Id(bool validated = true) =0;
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
		 * @param authstring	Client Steam2 id, if available, else engine auth id.
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

		/**
		 * @brief Called once a client is authorized and fully in-game, but 
		 * before admin checks are done.  This can be used to override the 
		 * default admin checks for a client.
		 *
		 * By default, this function allows the authentication process to 
		 * continue as normal.  If you need to delay the cache searching 
		 * process in order to get asynchronous data, then return false here. 
		 *
		 * If you return false, you must call IPlayerManager::NotifyPostAdminCheck 
		 * for the same client, or else the OnClientPostAdminCheck callback will 
		 * never be called.
		 *
		 * @param client		Client index.
		 * @return				True to continue normally, false to override 
		 *						the authentication process.
		 */
		virtual bool OnClientPreAdminCheck(int client)
		{
			return true;
		}

		/**
		 * @brief Called once a client is authorized and fully in-game, and 
		 * after all post-connection authorizations have been passed.  If the 
		 * client does not have an AdminId by this stage, it means that no 
		 * admin entry was in the cache that matched, and the user could not 
		 * be authenticated as an admin.
		 *
		 * @param client		Client index.
		 */
		virtual void OnClientPostAdminCheck(int client)
		{
		}

		/**
		* @brief Notifies the extension that the maxplayers value has changed
		*
		* @param newvalue			New maxplayers value.
		*/
		virtual void OnMaxPlayersChanged(int newvalue)
		{
		}

		/**
		* @brief Notifies the extension that a clients settings changed
		*
		* @param client			Client index.
		*/
		virtual void OnClientSettingsChanged(int client)
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

		/**
		 * @brief Returns the client index by its serial number.
		 *
		 * @return				Client index, or 0 for invalid serial.
		 */
		virtual int GetClientFromSerial(unsigned int serial) =0;

		/**
		 * @brief Processes the pattern inside a cmd_target_info_t structure
		 * and outputs the clients that match it.
		 *
		 * @param info			Pointer to the cmd_target_info_t structure to process.
		 */
		virtual void ProcessCommandTarget(cmd_target_info_t *info) =0;

		/**
		 * @brief Removes all access for the given admin id.
		 *
		 * @param id			Admin id.
		 */
		virtual void ClearAdminId(AdminId id) =0;

		/**
		 * @brief Reruns admin checks on all players.
		 */
		virtual void RecheckAnyAdmins() =0;
	};
}

#endif //_INCLUDE_SOURCEMOD_INTERFACE_IPLAYERHELPERS_H_
