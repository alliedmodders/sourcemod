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

#ifndef _INCLUDE_SOURCEMOD_ADMINISTRATION_SYSTEM_H_
#define _INCLUDE_SOURCEMOD_ADMINISTRATION_SYSTEM_H_

#include <IShareSys.h>

#define SMINTERFACE_ADMINSYS_NAME		"IAdminSys"
#define SMINTERFACE_ADMINSYS_VERSION	8

/**
 * @file IAdminSystem.h
 * @brief Defines the interface to manage the Admin Users/Groups and Override caches.
 *
 *   The administration system is more of a volatile cache than a system.  It is designed to be 
 * temporary rather than permanent, in order to compensate for more storage methods.  For example,
 * a flat file might be read into the cache all at once.  But a MySQL-based system might only cache
 * admin permissions when that specific admin connects.  
 *
 *   The override cache is the simplest to explain.  Any time an override is added, any existing 
 * and all future commands will gain a new access level set by the override.  If unset, the default
 * access level is restored.  This cache is dynamically changeable.
 *
 *   The group cache contains, for each group:
 *	1] A set of inherent flags - fully readable/writable.
 *  2] An immunity table - insertion and retrieval only.
 *  3] An override table - insertion and retrieval only.
 *   Individual groups can be invalidated entirely.  It should be considered an expensive
 *  operation, since each admin needs to be patched up to not reference the group.
 *
 *   For more information, see the SourceMod Development wiki.
 */

namespace SourceMod
{
	/**
	 * @brief Access levels (flags) for admins.
	 */
	enum AdminFlag
	{
		Admin_Reservation = 0,	/**< Reserved slot */
		Admin_Generic,			/**< Generic admin abilities */
		Admin_Kick,				/**< Kick another user */
		Admin_Ban,				/**< Ban another user */
		Admin_Unban,			/**< Unban another user */
		Admin_Slay,				/**< Slay/kill/damage another user */
		Admin_Changemap,		/**< Change the map */
		Admin_Convars,			/**< Change basic convars */
		Admin_Config,			/**< Change configuration */
		Admin_Chat,				/**< Special chat privileges */
		Admin_Vote,				/**< Special vote privileges */
		Admin_Password,			/**< Set a server password */
		Admin_RCON,				/**< Use RCON */
		Admin_Cheats,			/**< Change sv_cheats and use its commands */
		Admin_Root,				/**< All access by default */
		Admin_Custom1,			/**< First custom flag type */
		Admin_Custom2,			/**< Second custom flag type */
		Admin_Custom3,			/**< Third custom flag type */
		Admin_Custom4,			/**< Fourth custom flag type */
		Admin_Custom5,			/**< Fifth custom flag type */
		Admin_Custom6,			/**< Sixth custom flag type */
		/* --- */
		AdminFlags_TOTAL,
	};

	#define ADMFLAG_RESERVATION			(1<<0)		/**< Convenience macro for Admin_Reservation as a FlagBit */
	#define ADMFLAG_GENERIC				(1<<1)		/**< Convenience macro for Admin_Generic as a FlagBit */
	#define ADMFLAG_KICK				(1<<2)		/**< Convenience macro for Admin_Kick as a FlagBit */
	#define ADMFLAG_BAN					(1<<3)		/**< Convenience macro for Admin_Ban as a FlagBit */
	#define ADMFLAG_UNBAN				(1<<4)		/**< Convenience macro for Admin_Unban as a FlagBit */
	#define ADMFLAG_SLAY				(1<<5)		/**< Convenience macro for Admin_Slay as a FlagBit */
	#define ADMFLAG_CHANGEMAP			(1<<6)		/**< Convenience macro for Admin_Changemap as a FlagBit */
	#define ADMFLAG_CONVARS				(1<<7)		/**< Convenience macro for Admin_Convars as a FlagBit */
	#define ADMFLAG_CONFIG				(1<<8)		/**< Convenience macro for Admin_Config as a FlagBit */
	#define ADMFLAG_CHAT				(1<<9)		/**< Convenience macro for Admin_Chat as a FlagBit */
	#define ADMFLAG_VOTE				(1<<10)		/**< Convenience macro for Admin_Vote as a FlagBit */
	#define ADMFLAG_PASSWORD			(1<<11)		/**< Convenience macro for Admin_Password as a FlagBit */
	#define ADMFLAG_RCON				(1<<12)		/**< Convenience macro for Admin_RCON as a FlagBit */
	#define ADMFLAG_CHEATS				(1<<13)		/**< Convenience macro for Admin_Cheats as a FlagBit */
	#define ADMFLAG_ROOT				(1<<14)		/**< Convenience macro for Admin_Root as a FlagBit */
	#define ADMFLAG_CUSTOM1				(1<<15)		/**< Convenience macro for Admin_Custom1 as a FlagBit */
	#define ADMFLAG_CUSTOM2				(1<<16)		/**< Convenience macro for Admin_Custom2 as a FlagBit */
	#define ADMFLAG_CUSTOM3				(1<<17)		/**< Convenience macro for Admin_Custom3 as a FlagBit */
	#define ADMFLAG_CUSTOM4				(1<<18)		/**< Convenience macro for Admin_Custom4 as a FlagBit */
	#define ADMFLAG_CUSTOM5				(1<<19)		/**< Convenience macro for Admin_Custom5 as a FlagBit */
	#define ADMFLAG_CUSTOM6				(1<<20)		/**< Convenience macro for Admin_Custom6 as a FlagBit */

	/**
	 * @brief Specifies which type of command to override (command or command group).
	 */
	enum OverrideType
	{
		Override_Command = 1,	/**< Command */
		Override_CommandGroup,	/**< Command group */
	};

	/**
	 * @brief Specifies how a command is overridden for a user group.
	 */
	enum OverrideRule
	{
		Command_Deny = 0,		/**< Deny access */
		Command_Allow = 1,		/**< Allow access */
	};

	/**
	 * @brief DEPRECATED.  Specifies a generic immunity type.
	 */
	enum ImmunityType
	{
		Immunity_Default = 1,	/**< Immunity value of 1 */
		Immunity_Global,		/**< Immunity value of 2 */
	};

	/**
	 * @brief Defines user access modes.
	 */
	enum AccessMode
	{
		Access_Real,		/**< Access the user has inherently */
		Access_Effective,	/**< Access the user has from their groups */
	};

	/**
	 * @brief Represents an index to one group.
	 */
	typedef int		GroupId;

	/**
	 * @brief Represents an index to one user entry.
	 */
	typedef int		AdminId;

	/**
	 * @brief Represents an invalid/nonexistent group or an erroneous operation.
	 */
	#define INVALID_GROUP_ID	-1

	/**
	 * @brief Represents an invalid/nonexistent user or an erroneous operation.
	 */
	#define INVALID_ADMIN_ID	-1

	/**
	 * @brief Represents the various cache regions.
	 */
	enum AdminCachePart
	{
		AdminCache_Overrides = 0,		/**< Global overrides */
		AdminCache_Groups = 1,			/**< All groups (automatically invalidates admins too) */
		AdminCache_Admins = 2,			/**< All admins */
	};

	/**
	 * @brief Provides callbacks for admin cache operations.
	 */
	class IAdminListener
	{
	public:
		virtual unsigned int GetInterfaceVersion()
		{
			return SMINTERFACE_ADMINSYS_VERSION;
		}
	public:
		/**
		 * @brief Called when the admin cache needs to be rebuilt.  
		 *
		 * @param auto_rebuild		True if this is being called because of a group rebuild.
		 */
		virtual void OnRebuildAdminCache(bool auto_rebuild) =0;

		/**
		 * @brief Called when the group cache needs to be rebuilt.
		 */
		virtual void OnRebuildGroupCache() =0;
		
		/**
		 * @brief Called when the global override cache needs to be rebuilt.
		 */
		virtual void OnRebuildOverrideCache() =0;
	};

	/**
	 * @brief Admin permission levels.
	 */
	typedef unsigned int FlagBits;

	/**
	 * @brief Provides functions for manipulating the admin options cache.
	 */
	class IAdminSystem : public SMInterface
	{
	public:
		const char *GetInterfaceName()
		{
			return SMINTERFACE_ADMINSYS_NAME;
		}
		unsigned int GetInterfaceVersion()
		{
			return SMINTERFACE_ADMINSYS_VERSION;
		}
	public:
		/**
		 * @brief Adds a global command flag override.  Any command registered with this name
		 * will assume the new flag.  This is applied retroactively as well.
		 *
		 * @param cmd			String containing command name (case sensitive).
		 * @param type			Override type (specific command or group).
		 * @param flags			New admin flag.
		 */
		virtual void AddCommandOverride(const char *cmd, OverrideType type, FlagBits flags) =0;

		/**
		 * @brief Returns a command override.
		 *
		 * @param cmd			String containing command name (case sensitive).
		 * @param type			Override type (specific command or group).
		 * @param pFlags		Optional pointer to the set flag.
		 * @return				True if there is an override, false otherwise.
		 */
		virtual bool GetCommandOverride(const char *cmd, OverrideType type, FlagBits *pFlags) =0;

		/**
		 * @brief Unsets a command override.
		 *
		 * @param cmd			String containing command name (case sensitive).
		 * @param type			Override type (specific command or group).
		 */
		virtual void UnsetCommandOverride(const char *cmd, OverrideType type) =0;

		/**
		 * @brief Adds a new group.  Name must be unique.
		 *
		 * @param group_name	String containing the group name.
		 * @return				A new group id, INVALID_GROUP_ID if it already exists.
		 */
		virtual GroupId AddGroup(const char *group_name) =0;

		/**
		 * @brief Finds a group by name.
		 *
		 * @param group_name	String containing the group name.
		 * @return				A group id, or INVALID_GROUP_ID if not found.
		 */
		virtual GroupId FindGroupByName(const char *group_name) =0;

		/**
		 * @brief Adds or removes a flag from a group's flag set.
		 * Note: These are called "add flags" because they add to a user's flags.
		 *
		 * @param id			Group id.
		 * @param flag			Admin flag to toggle.
		 * @param enabled		True to set the flag, false to unset/disable.
		 */
		virtual void SetGroupAddFlag(GroupId id, AdminFlag flag, bool enabled) =0;

		/**
		 * @brief Gets the set value of an add flag on a group's flag set.
		 *
		 * @param id			Group id.
		 * @param flag			Admin flag to retrieve.
		 * @return				True if enabled, false otherwise,
		 */
		virtual bool GetGroupAddFlag(GroupId id, AdminFlag flag) =0;

		/**
		 * @brief Returns an array of flag bits that are added to a user from their group.
		 * Note: These are called "add flags" because they add to a user's flags.
		 *
		 * @param id			GroupId of the group.
		 * @return				Bit string containing the bits of each flag.
		 */
		virtual FlagBits GetGroupAddFlags(GroupId id) =0;

		/**
		 * @brief DEPRECATED.  Sets a group's immunity level using backwards 
		 * compatible types.  
		 *
		 * If the new level being set is lower than the group's actual immunity 
		 * level, no operation takes place.
		 *
		 * @param id			Group id.
		 * @param type			Immunity type which will be converted to a 
		 * 						numerical level.
		 * @param enabled		True to set the level.  False sets the 
		 * 						group's immunity value to 0.
		 */
		virtual void SetGroupGenericImmunity(GroupId id, ImmunityType type, bool enabled) =0;

		/**
		 * @brief DEPRECATED.  Returns whether a group has an immunity level
		 * using backwards compatible types.
		 * 
		 * This simply checks whether the group's immunity value is greater 
		 * than or equal to the new-style value for the old type.
		 *
		 * @param id			Group id.
		 * @param type			Generic immunity type.
		 * @return				True if the group has this immunity, false 
		 * 						otherwise.
		 */
		virtual bool GetGroupGenericImmunity(GroupId id, ImmunityType type) =0;

		/**
		 * @brief Adds immunity to a specific group.
		 *
		 * @param id			Group id.
		 * @param other_id		Group id to receive immunity to.
		 */
		virtual void AddGroupImmunity(GroupId id, GroupId other_id) =0;

		/**
		 * @brief Returns the number of specific group immunities.
		 *
		 * @param id			Group id.
		 * @return				Number of group immunities.
		 */
		virtual unsigned int GetGroupImmunityCount(GroupId id) =0;

		/**
		 * @brief Returns a group that this group is immune to given an index.
		 *
		 * @param id			Group id.
		 * @param number		Index from 0 to N-1, from GetGroupImmunities().
		 * @return				GroupId that this group is immune to.
		 */
		virtual GroupId GetGroupImmunity(GroupId id, unsigned int number) =0;

		/**
		 * @brief Adds a group-specific override type.
		 *
		 * @param id			Group id.
		 * @param name			String containing command name (case sensitive).
		 * @param type			Override type (specific command or group).
		 * @param rule			Override allow/deny setting.
		 */
		virtual void AddGroupCommandOverride(GroupId id,
											 const char *name,
											 OverrideType type,
											 OverrideRule rule) =0;

		/**
		 * @brief Retrieves a group-specific command override.
		 *
		 * @param id			Group id.
		 * @param name			String containing command name (case sensitive).
		 * @param type			Override type (specific command or group).
		 * @param pRule			Optional pointer to store allow/deny setting.
		 * @return				True if an override exists, false otherwise.
		 */
		virtual bool GetGroupCommandOverride(GroupId id,
											 const char *name,
											 OverrideType type,
											 OverrideRule *pRule) =0;

		/**
		 * @brief Tells the admin system to dump a portion of the cache.  
		 * This calls into plugin forwards to rebuild the cache.
		 *
		 * @param part			Portion of the cache to dump.
		 * @param rebuild		If true, the rebuild forwards/events will fire.
		 */
		virtual void DumpAdminCache(AdminCachePart part, bool rebuild) =0;

		/**
		 * @brief Adds an admin interface listener.
		 *
		 * @param pListener		Pointer to an IAdminListener to add.
		 */
		virtual void AddAdminListener(IAdminListener *pListener) =0;

		/**
		 * @brief Removes an admin interface listener.
		 *
		 * @param pListener		Pointer to an IAdminListener to remove.
		 */
		virtual void RemoveAdminListener(IAdminListener *pListener) =0;

		/**
		 * @brief Registers an authentication identity type.
		 * Note: Default types are "steam," "name," and "ip."
		 *
		 * @param name			String containing the type name.
		 */
		virtual void RegisterAuthIdentType(const char *name) =0;

		/**
		 * @brief Creates a new user entry.
		 *
		 * @param name		Name for this entry (does not have to be unique).
		 *					Specify NULL for an anonymous admin.
		 * @return			A new AdminId index.
		 */
		virtual AdminId CreateAdmin(const char *name) =0;

		/**
		 * @brief Gets an admin's user name.
		 *
		 * @param id		AdminId index for this admin.
		 * @return			A string containing the admin's name, or NULL
		 *					if the admin was created anonymously.
		 */
		virtual const char *GetAdminName(AdminId id) =0;

		/**
		 * @brief Binds a user entry to a particular auth method.
		 * This bind must be unique.
		 *
		 * @param id		AdminId index of the admin.
		 * @param auth		Auth method to use.
		 * @param ident		Identity string to bind to.
		 * @return			True on success, false if auth method was not found,
		 *					id was invalid, or ident was already taken.
		 */
		virtual bool BindAdminIdentity(AdminId id, const char *auth, const char *ident) =0;

		/**
		 * @brief Sets whether or not a flag is enabled on an admin.
		 *
		 * @param id		AdminId index of the admin.
		 * @param flag		Admin flag to use.
		 * @param enabled	True to enable, false to disable.
		 */
		virtual void SetAdminFlag(AdminId id, AdminFlag flag, bool enabled) =0;

		/**
		 * @brief Returns whether or not a flag is enabled on an admin.
		 *
		 * @param id		AdminId index of the admin.
		 * @param flag		Admin flag to use.
		 * @param mode		Access mode to check.
		 * @return			True if enabled, false otherwise.
		 */
		virtual bool GetAdminFlag(AdminId id, AdminFlag flag, AccessMode mode) =0;

		/**
		 * @brief Returns the bitstring of access flags on an admin.
		 *
		 * @param id		AdminId index of the admin.
		 * @param mode		Access mode to use.
		 * @return			A bit string containing which flags are enabled.
		 */
		virtual FlagBits GetAdminFlags(AdminId id, AccessMode mode) =0;

		/**
		 * @brief Sets the bitstring of access flags on an admin.
		 *
		 * @param id		AdminId index of the admin.
		 * @param mode		Access mode to use (real affects both).
		 * @param bits		Bitstring to set.
		 */
		virtual void SetAdminFlags(AdminId id, AccessMode mode, FlagBits bits) =0;

		/**
		 * @brief Adds a group to an admin's inherited group list.  
		 * Any flags the group has will be added to the admin's effective flags.
		 *
		 * @param id		AdminId index of the admin.
		 * @param gid		GroupId index of the group.
		 * @return			True on success, false on invalid input or duplicate membership.
		 */
		virtual bool AdminInheritGroup(AdminId id, GroupId gid) =0;

		/**
		 * @brief Returns the number of groups this admin is a member of.
		 * 
		 * @param id		AdminId index of the admin.
		 * @return			Number of groups this admin is a member of.
		 */
		virtual unsigned int GetAdminGroupCount(AdminId id) =0;

		/**
		 * @brief Returns group information from an admin.
		 *
		 * @param id		AdminId index of the admin.
		 * @param index		Group number to retrieve, from 0 to N-1, where N
		 *					is the value of GetAdminGroupCount(id).
		 * @param name		Optional pointer to store the group's name.
		 * @return			A GroupId index and a name pointer, or
		 *					INVALID_GROUP_ID and NULL if an error occurred.
		 */
		virtual GroupId GetAdminGroup(AdminId id, unsigned int index, const char **name) =0;
		
		/**
		 * @brief Sets a password on an admin.
		 *
		 * @param id		AdminId index of the admin.
		 * @param password	String containing the password.
		 */
		virtual void SetAdminPassword(AdminId id, const char *password) =0;

		/**
		 * @brief Gets an admin's password.
		 *
		 * @param id		AdminId index of the admin.
		 * @return			Password of the admin, or NULL if none.
		 */
		virtual const char *GetAdminPassword(AdminId id) =0;
		
		/**
		 * @brief Attempts to find an admin by an auth method and an identity.
		 *
		 * @param auth		Auth method to try.
		 * @param identity	Identity string to look up.
		 * @return			An AdminId index if found, INVALID_ADMIN_ID otherwise.
		 */
		virtual AdminId FindAdminByIdentity(const char *auth, const char *identity) =0;

		/**
		 * @brief Invalidates an admin from the cache so its resources can be re-used.
		 *
		 * @param id		AdminId index to invalidate.
		 * @return			True on success, false otherwise.
		 */
		virtual bool InvalidateAdmin(AdminId id) =0;

		/**
		 * @brief Converts a flag bit string to a bit array.
		 *
		 * @param bits		Bit string containing the flags.
		 * @param array		Array to write the flags to.  Enabled flags will be 'true'.
		 * @param maxSize	Maximum number of flags the array can store.
		 * @return			Number of flags written.
		 */
		virtual unsigned int FlagBitsToBitArray(FlagBits bits, bool array[], unsigned int maxSize) =0;

		/**
		 * @brief Converts a flag array to a bit string.
		 *
		 * @param array		Array containing true or false for each AdminFlag.
		 * @param maxSize	Maximum size of the flag array.
		 * @return			A bit string composed of the array bits.
		 */
		virtual FlagBits FlagBitArrayToBits(const bool array[], unsigned int maxSize) =0;

		/**
		 * @brief Converts an array of flags to bits.
		 *
		 * @param array		Array containing flags that are enabled.
		 * @param numFlags	Number of flags in the array.
		 * @return			A bit string composed of the array flags.
		 */
		virtual FlagBits FlagArrayToBits(const AdminFlag array[], unsigned int numFlags) =0;

		/**
		 * @brief Converts a bit string to an array of flags.
		 *
		 * @param bits		Bit string containing the flags.
		 * @param array		Output array to write flags.
		 * @param maxSize	Maximum size of the flag array.
		 * @return			Number of flags written.
		 */
		virtual unsigned int FlagBitsToArray(FlagBits bits, AdminFlag array[], unsigned int maxSize) =0;

		/**
		 * @brief Checks whether a user has access to a given set of flag bits.
		 * Note: This is a wrapper around GetAdminFlags().
		 *
		 * @param id		AdminId index of admin.
		 * @param bits		Bitstring containing the permissions to check.
		 * @return			True if user has permission, false otherwise.
		 */
		virtual bool CheckAdminFlags(AdminId id, FlagBits bits) =0;

		/**
		 * @brief Checks whether an AdminId can target another AdminId.
		 * 
		 * The hueristics for this check are as follows:
		 * 0. If the targeting AdminId is INVALID_ADMIN_ID, targeting fails.
		 * 1. If the targeted AdminId is INVALID_ADMIN_ID, targeting succeeds.
		 * 2. If the targeted AdminId is the same as the targeting AdminId,
		 *    (self) targeting succeeds.
		 * 3. If the targeting admin is root, targeting succeeds.
		 * 4. If the targeted admin has access higher (as interpreted by 
		 *    (sm_immunity_mode) than the targeting admin, then targeting fails.
		 * 5. If the targeted admin has specific immunity from the
		 *    targeting admin via group immunities, targeting fails.
		 * 6. Targeting succeeds.
		 *
		 * @param id		AdminId index of admin doing the targeting.  Can be INVALID_ADMIN_ID.
		 * @param target	AdminId index of the target admin.  Can be INVALID_ADMIN_ID.
		 * @return			True if this admin has permission to target the other admin.
		 */
		virtual bool CanAdminTarget(AdminId id, AdminId target) =0;

		/**
		 * @brief Returns a flag from a named string.
		 *
		 * @param flagname	Case sensitive flag name string (like "kick").
		 * @param pAdmFlag	Pointer to store the found admin flag in.
		 * @return			True on success, false on failure.
		 */
		virtual bool FindFlag(const char *flagname, AdminFlag *pAdmFlag) =0;

		/**
		 * @brief Reads a single character as a flag.
		 *
		 * @param c			Flag character.
		 * @param pAdmFlag	Pointer to store the admin flag.
		 * @return			True on success, false if invalid.
		 */
		virtual bool FindFlag(char c, AdminFlag *pAdmFlag) =0;

		/**
		 * @brief Reads a string of flag letters and returns its access value.
		 *
		 * @param flags		Flag string.
		 * @param end		Pointer to store the last value read.  On success, 
		 *					this will store a pointer to the null terminator.
		 * @return			FlagBits value of the flags.
		 */
		virtual FlagBits ReadFlagString(const char *flags, const char **end) =0;

		/**
		 * @brief Returns a "serial number" for an AdminId.  If the serial 
		 * number has changed for a given AdminId, it means the permissions 
		 * have changed.
		 *
		 * @param id		AdminId value.
		 * @return			Serial number, or 0 on failure.
		 */
		virtual unsigned int GetAdminSerialChange(AdminId id) =0;

		/**
		 * @brief Checks whether an admin can use the given command name.
		 *
		 * If the command does not exist, this will return true.
		 *
		 * @param client	Client index.
		 * @param cmd		Command name.
		 * @return			True on success, false on failure.
		 */
		virtual bool CanAdminUseCommand(int client, const char *cmd) =0;

		/**
		 * @brief Returns the name of a group.
		 *
		 * @param gid		Group Id.
		 * @return			Group name, or NULL on failure.
		 */
		virtual const char *GetGroupName(GroupId gid) =0;

		/**
		 * @brief Sets the immunity level of a group.
		 * 
		 * @param gid		Group Id.
		 * @param level		Immunity level value.
		 * @return			Old immunity level.
		 */
		virtual unsigned int SetGroupImmunityLevel(GroupId gid, unsigned int level) =0;

		/**
		 * @brief Retrieves the immunity level of a group.
		 *
		 * @param gid		Group Id.
		 * @return			Immunity level value.
		 */
		virtual unsigned int GetGroupImmunityLevel(GroupId gid) =0;

		/**
		 * @brief Sets the immunity level of an admin.
		 *
		 * @param id		Admin Id.
		 * @param level		Immunity level value.
		 * @return			Old immunity level.
		 */
		virtual unsigned int SetAdminImmunityLevel(AdminId id, unsigned int level) =0;

		/**
		 * @brief Retrieves the immunity level of an admin.
		 *
		 * @param id		Admin Id.
		 * @return			Immunity level value.
		 */
		virtual unsigned int GetAdminImmunityLevel(AdminId id) =0;

		/**
		 * @brief Computers access to an override.
		 *
		 * @param client		Client index.
		 * @param cmd			Override name.
		 * @param flags			Default flags.
		 * @param override_only	If false, if a command matches the override, 
		 *						then its flags will override the default.
		 * @return				True if the client has access, false otherwise.
		 */
		virtual bool CheckAccess(int client, 
			const char *cmd, 
			FlagBits flags, 
			bool override_only) =0;

		/**
		 * @brief Reads a flag as the corresponding character.
		 *
		 * @param flag		Flag to look up.
		 * @param c			Variable to store flag char.
		 * @return			True on success, false if not found.
		 */
		virtual bool FindFlagChar(AdminFlag flag, char *c) =0;

		/**
		 * @brief Returns whether or not an admin id is valid.
		 *
		 * @param id		Admin id to check.
		 * @return			True if valid, otherwise false.
		 */
		virtual bool IsValidAdmin(AdminId id) =0;

		/**
		 * @brief Returns whether or not a client has access to a given command.
		 *
		 * @param client	Client index.
		 * @param cmd		Command name.
		 * @param flags		Command admin flags.
		 * @return			True if allowed access, otherwise false;
		 */
		virtual bool CheckClientCommandAccess(int client, const char *cmd, FlagBits cmdflags) =0;
	};
}

#endif //_INCLUDE_SOURCEMOD_ADMINISTRATION_SYSTEM_H_

