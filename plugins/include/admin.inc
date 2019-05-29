/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod (C)2004-2008 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This file is part of the SourceMod/SourcePawn SDK.
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

#if defined _admin_included
 #endinput
#endif
#define _admin_included

/**
 * Access levels (flags) for admins.
 */
enum AdminFlag
{
	Admin_Reservation = 0,  /**< Reserved slot */
	Admin_Generic,          /**< Generic admin abilities */
	Admin_Kick,             /**< Kick another user */
	Admin_Ban,              /**< Ban another user */
	Admin_Unban,            /**< Unban another user */
	Admin_Slay,             /**< Slay/kill/damage another user */
	Admin_Changemap,        /**< Change the map */
	Admin_Convars,          /**< Change basic convars */
	Admin_Config,           /**< Change configuration */
	Admin_Chat,             /**< Special chat privileges */
	Admin_Vote,             /**< Special vote privileges */
	Admin_Password,         /**< Set a server password */
	Admin_RCON,             /**< Use RCON */
	Admin_Cheats,           /**< Change sv_cheats and use its commands */
	Admin_Root,             /**< All access by default */
	Admin_Custom1,          /**< First custom flag type */
	Admin_Custom2,          /**< Second custom flag type */
	Admin_Custom3,          /**< Third custom flag type */
	Admin_Custom4,          /**< Fourth custom flag type */
	Admin_Custom5,          /**< Fifth custom flag type */
	Admin_Custom6           /**< Sixth custom flag type */
};

#define AdminFlags_TOTAL   21       /**< Total number of admin flags */

/**
 * @section Bitwise values definitions for admin flags.
 */
#define ADMFLAG_RESERVATION         (1<<0)      /**< Convenience macro for Admin_Reservation as a FlagBit */
#define ADMFLAG_GENERIC             (1<<1)      /**< Convenience macro for Admin_Generic as a FlagBit */
#define ADMFLAG_KICK                (1<<2)      /**< Convenience macro for Admin_Kick as a FlagBit */
#define ADMFLAG_BAN                 (1<<3)      /**< Convenience macro for Admin_Ban as a FlagBit */
#define ADMFLAG_UNBAN               (1<<4)      /**< Convenience macro for Admin_Unban as a FlagBit */
#define ADMFLAG_SLAY                (1<<5)      /**< Convenience macro for Admin_Slay as a FlagBit */
#define ADMFLAG_CHANGEMAP           (1<<6)      /**< Convenience macro for Admin_Changemap as a FlagBit */
#define ADMFLAG_CONVARS             (1<<7)      /**< Convenience macro for Admin_Convars as a FlagBit */
#define ADMFLAG_CONFIG              (1<<8)      /**< Convenience macro for Admin_Config as a FlagBit */
#define ADMFLAG_CHAT                (1<<9)      /**< Convenience macro for Admin_Chat as a FlagBit */
#define ADMFLAG_VOTE                (1<<10)     /**< Convenience macro for Admin_Vote as a FlagBit */
#define ADMFLAG_PASSWORD            (1<<11)     /**< Convenience macro for Admin_Password as a FlagBit */
#define ADMFLAG_RCON                (1<<12)     /**< Convenience macro for Admin_RCON as a FlagBit */
#define ADMFLAG_CHEATS              (1<<13)     /**< Convenience macro for Admin_Cheats as a FlagBit */
#define ADMFLAG_ROOT                (1<<14)     /**< Convenience macro for Admin_Root as a FlagBit */
#define ADMFLAG_CUSTOM1             (1<<15)     /**< Convenience macro for Admin_Custom1 as a FlagBit */
#define ADMFLAG_CUSTOM2             (1<<16)     /**< Convenience macro for Admin_Custom2 as a FlagBit */
#define ADMFLAG_CUSTOM3             (1<<17)     /**< Convenience macro for Admin_Custom3 as a FlagBit */
#define ADMFLAG_CUSTOM4             (1<<18)     /**< Convenience macro for Admin_Custom4 as a FlagBit */
#define ADMFLAG_CUSTOM5             (1<<19)     /**< Convenience macro for Admin_Custom5 as a FlagBit */
#define ADMFLAG_CUSTOM6             (1<<20)     /**< Convenience macro for Admin_Custom6 as a FlagBit */

/**
 * @endsection
 */

/**
 * @section Hardcoded authentication methods
 */
#define AUTHMETHOD_STEAM            "steam"     /**< SteamID based authentication */
#define AUTHMETHOD_IP               "ip"        /**< IP based authentication */
#define AUTHMETHOD_NAME             "name"      /**< Name based authentication */

/**
 * @endsection
 */

/**
 * Access override types.
 */
enum OverrideType
{
	Override_Command = 1,   /**< Command */
	Override_CommandGroup   /**< Command group */
};

/**
 * Access override rules.
 */
enum OverrideRule
{
	Command_Deny = 0,
	Command_Allow = 1
};

/**
 * DEPRECATED, do not use.
 */
enum ImmunityType
{
	Immunity_Default = 1,   /**< Deprecated. */
	Immunity_Global         /**< Deprecated. */
};

/**
 * Identifies a unique entry in the group permissions cache.  These are not Handles.
 */
enum GroupId
{
	INVALID_GROUP_ID = -1   /**< An invalid/non-existent group */
};

/**
 * Identifies a unique entry in the admin permissions cache.  These are not Handles.
 */
enum AdminId
{
	INVALID_ADMIN_ID = -1   /**< An invalid/non-existent admin */
};

/**
 * Methods of computing access permissions.
 */
enum AdmAccessMode
{
	Access_Real,        /**< Access the user has inherently */
	Access_Effective    /**< Access the user has from their groups */
};

/**
 * Represents the various cache regions.
 */
enum AdminCachePart
{
	AdminCache_Overrides = 0,       /**< Global overrides */
	AdminCache_Groups = 1,          /**< All groups (automatically invalidates admins too) */
	AdminCache_Admins = 2           /**< All admins */
};

methodmap AdminId {
	// Retrieves an admin's user name as made with CreateAdmin().
	//
	// @note This function can return UTF-8 strings, and will safely chop UTF-8 strings.
	//
	// @param name          String buffer to store name.
	// @param maxlength     Maximum size of string buffer.
	// @return              Number of bytes written.
	public native void GetUsername(char[] name, int maxlength);

	// Binds an admin to an identity for fast lookup later on.  The bind must be unique.
	//
	// @param authMethod    Auth method to use, predefined or from RegisterAuthIdentType().
	// @param ident         String containing the arbitrary, unique identity.
	// @return              True on success, false if the auth method was not found,
	//                      ident was already taken, or ident invalid for auth method.
	public native bool BindIdentity(const char[] authMethod, const char[] ident);

	// Sets whether or not a flag is enabled on an admin.
	//
	// @param flag          Admin flag to use.
	// @param enabled       True to enable, false to disable.
	public native void SetFlag(AdminFlag flag, bool enabled);

	// Returns whether or not a flag is enabled on an admin.
	//
	// @param flag          Admin flag to use.
	// @param mode          Access mode to check.
	// @return              True if enabled, false otherwise.
	public native bool HasFlag(AdminFlag flag, AdmAccessMode mode=Access_Effective);

	// Returns the bitstring of access flags on an admin.
	//
	// @param mode          Access mode to use.
	// @return              A bitstring containing which flags are enabled.
	public native int GetFlags(AdmAccessMode mode);

	// Adds a group to an admin's inherited group list.  Any flags the group has
	// will be added to the admin's effective flags.
	//
	// @param gid           GroupId index of the group.
	// @return              True on success, false on invalid input or duplicate membership.
	public native bool InheritGroup(GroupId gid);

	// Returns group information from an admin.
	//
	// @param index         Group number to retrieve, from 0 to N-1, where N
	//                      is the value of the GroupCount property.
	// @param name          Buffer to store the group's name.
	//                      Note: This will safely chop UTF-8 strings.
	// @param maxlength     Maximum size of the output name buffer.
	// @return              A GroupId index and a name pointer, or
	//                      INVALID_GROUP_ID and NULL if an error occurred.
	public native GroupId GetGroup(int index, char[] name, int maxlength);

	// Sets a password on an admin.
	//
	// @param password      String containing the password.
	public native void SetPassword(const char[] password);

	// Gets an admin's password.
	//
	// @param buffer        Optional buffer to store the admin's password.
	// @param maxlength     Maximum size of the output name buffer.
	//                      Note: This will safely chop UTF-8 strings.
	// @return              True if there was a password set, false otherwise.
	public native bool GetPassword(char[] buffer="", int maxlength=0);

	// Tests whether one admin can target another.
	//
	// The heuristics for this check are as follows:
	// 0. If the targeting AdminId is INVALID_ADMIN_ID, targeting fails.
	// 1. If the targeted AdminId is INVALID_ADMIN_ID, targeting succeeds.
	// 2. If the targeted AdminId is the same as the targeting AdminId,
	//    (self) targeting succeeds.
	// 3. If the targeting admin is root, targeting succeeds.
	// 4. If the targeted admin has access higher (as interpreted by
	//    (sm_immunity_mode) than the targeting admin, then targeting fails.
	// 5. If the targeted admin has specific immunity from the
	//    targeting admin via group immunities, targeting fails.
	// 6. Targeting succeeds.
	//
	// @param target        Target admin (may be INVALID_ADMIN_ID).
	// @return              True if targetable, false if immune.
	public native bool CanTarget(AdminId other);

	// The number of groups of which this admin is a member.
	property int GroupCount {
		public native get();
	}

	// Immunity level used for targetting.
	property int ImmunityLevel {
		public native get();
		public native set(int level);
	}
}

methodmap GroupId {
	// Gets whether or not a flag is enabled on a group's flag set.
	//
	// @param flag          Admin flag to retrieve.
	// @return              True if enabled, false otherwise,
	public native bool HasFlag(AdminFlag flag);

	// Adds or removes a flag from a group's flag set.
	//
	// @param flag          Admin flag to toggle.
	// @param enabled       True to set the flag, false to unset/disable.
	public native void SetFlag(AdminFlag flag, bool enabled);

	// Returns the flag set that is added to users from this group.
	//
	// @return              Bitstring containing the flags enabled.
	public native int GetFlags();

	// Returns a group that this group is immune to given an index.
	//
	// @param number        Index from 0 to N-1, from GroupImmunitiesCount.
	// @return              GroupId that this group is immune to, or INVALID_GROUP_ID on failure.
	public native GroupId GetGroupImmunity(int index);

	// Adds immunity to a specific group.
	//
	// @param other         Group id to receive immunity to.
	public native void AddGroupImmunity(GroupId other);

	// Retrieves a group-specific command override.
	//
	// @param name          String containing command name (case sensitive).
	// @param type          Override type (specific command or group).
	// @param rule          Optional pointer to store allow/deny setting.
	// @return              True if an override exists, false otherwise.
	public native bool GetCommandOverride(const char[] name, OverrideType type, OverrideRule &rule);

	// Adds a group-specific override type.
	//
	// @param name          String containing command name (case sensitive).
	// @param type          Override type (specific command or group).
	// @param rule          Override allow/deny setting.
	public native void AddCommandOverride(const char[] name, OverrideType type, OverrideRule rule);

	// Number of specific group immunities
	property int GroupImmunitiesCount {
		public native get();
	}

	// Immunity level used for targetting.
	property int ImmunityLevel {
		public native get();
		public native set(int level);
	}
}

/**
 * Called when part of the cache needs to be rebuilt.
 *
 * @param part          Part of the admin cache to rebuild.
 */
forward void OnRebuildAdminCache(AdminCachePart part);

/**
 * Tells the admin system to dump a portion of the cache.
 *
 * @param part          Part of the cache to dump.  Specifying groups also dumps admins.
 * @param rebuild       If true, the rebuild forwards will fire.
 */
native void DumpAdminCache(AdminCachePart part, bool rebuild);

/**
 * Adds a global command flag override.  Any command registered with this name
 * will assume the new flag.  This is applied retroactively as well.
 *
 * @param cmd           String containing command name (case sensitive).
 * @param type          Override type (specific command or group).
 * @param flags         New admin flag.
 */
native void AddCommandOverride(const char[] cmd, OverrideType type, int flags);

/**
 * Returns a command override.
 *
 * @param cmd           String containing command name (case sensitive).
 * @param type          Override type (specific command or group).
 * @param flags         By-reference cell to store the flag (undefined if not found).
 * @return              True if there is an override, false otherwise.
 */
native bool GetCommandOverride(const char[] cmd, OverrideType type, int &flags);

/**
 * Unsets a command override.
 *
 * @param cmd           String containing command name (case sensitive).
 * @param type          Override type (specific command or group).
 */
native void UnsetCommandOverride(const char[] cmd, OverrideType type);

/**
 * Adds a new group.  Name must be unique.
 *
 * @param group_name    String containing the group name.
 * @return              A new group id, INVALID_GROUP_ID if it already exists.
 */
native GroupId CreateAdmGroup(const char[] group_name);

/**
 * Finds a group by name.
 *
 * @param group_name    String containing the group name.
 * @return              A group id, or INVALID_GROUP_ID if not found.
 */
native GroupId FindAdmGroup(const char[] group_name);

/**
 * Adds or removes a flag from a group's flag set.
 * @note These are called "add flags" because they add to a user's flags.
 *
 * @param id            Group id.
 * @param flag          Admin flag to toggle.
 * @param enabled       True to set the flag, false to unset/disable.
 */
native void SetAdmGroupAddFlag(GroupId id, AdminFlag flag, bool enabled);

/**
 * Gets the set value of an add flag on a group's flag set.
 * @note These are called "add flags" because they add to a user's flags.
 *
 * @param id            Group id.
 * @param flag          Admin flag to retrieve.
 * @return              True if enabled, false otherwise,
 */
native bool GetAdmGroupAddFlag(GroupId id, AdminFlag flag);

/**
 * Returns the flag set that is added to a user from their group.
 * @note These are called "add flags" because they add to a user's flags.
 *
 * @param id            GroupId of the group.
 * @return              Bitstring containing the flags enabled.
 */
native int GetAdmGroupAddFlags(GroupId id);

/**
 * @deprecated          Functionality removed.
 */
#pragma deprecated Use SetAdmGroupImmunityLevel() instead.
native void SetAdmGroupImmunity(GroupId id, ImmunityType type, bool enabled);

/**
 * @deprecated          Functionality removed.
 */
#pragma deprecated Use GetAdmGroupImmunityLevel() instead.
native bool GetAdmGroupImmunity(GroupId id, ImmunityType type);

/**
 * Adds immunity to a specific group.
 *
 * @param id            Group id.
 * @param other_id      Group id to receive immunity to.
 */
native void SetAdmGroupImmuneFrom(GroupId id, GroupId other_id);

/**
 * Returns the number of specific group immunities.
 *
 * @param id            Group id.
 * @return              Number of group immunities.
 */
native int GetAdmGroupImmuneCount(GroupId id);

/**
 * Returns a group that this group is immune to given an index.
 *
 * @param id            Group id.
 * @param number        Index from 0 to N-1, from GetAdmGroupImmuneCount().
 * @return              GroupId that this group is immune to, or INVALID_GROUP_ID on failure.
 */
native GroupId GetAdmGroupImmuneFrom(GroupId id, int number);

/**
 * Adds a group-specific override type.
 *
 * @param id            Group id.
 * @param name          String containing command name (case sensitive).
 * @param type          Override type (specific command or group).
 * @param rule          Override allow/deny setting.
 */
native void AddAdmGroupCmdOverride(GroupId id, const char[] name, OverrideType type, OverrideRule rule);

/**
 * Retrieves a group-specific command override.
 *
 * @param id            Group id.
 * @param name          String containing command name (case sensitive).
 * @param type          Override type (specific command or group).
 * @param rule          Optional pointer to store allow/deny setting.
 * @return              True if an override exists, false otherwise.
 */
native bool GetAdmGroupCmdOverride(GroupId id, const char[] name, OverrideType type, OverrideRule &rule);

/**
 * Registers an authentication identity type.  You normally never need to call this except for
 * very specific systems.
 *
 * @param name          Codename to use for your authentication type.
 */
native void RegisterAuthIdentType(const char[] name);

/**
 * Creates a new admin entry in the permissions cache and returns the generated AdminId index.
 *
 * @param name          Name for this entry (does not have to be unique).
 *                      Specify an empty string for an anonymous admin.
 * @return              New AdminId index or INVALID_ADMIN_ID if name is empty
 */
native AdminId CreateAdmin(const char[] name="");

/**
 * Retrieves an admin's user name as made with CreateAdmin().
 *
 * @note This function can return UTF-8 strings, and will safely chop UTF-8 strings.
 *
 * @param id            AdminId of the admin.
 * @param name          String buffer to store name.
 * @param maxlength     Maximum size of string buffer.
 * @return              Number of bytes written.
 */
native int GetAdminUsername(AdminId id, char[] name, int maxlength);

/**
 * Binds an admin to an identity for fast lookup later on.  The bind must be unique.
 *
 * @param id            AdminId of the admin.
 * @param auth          Auth method to use, predefined or from RegisterAuthIdentType().
 * @param ident         String containing the arbitrary, unique identity.
 * @return              True on success, false if the auth method was not found,
 *                      ident was already taken, or ident invalid for auth method.
 */
native bool BindAdminIdentity(AdminId id, const char[] auth, const char[] ident);

/**
 * Sets whether or not a flag is enabled on an admin.
 *
 * @param id            AdminId index of the admin.
 * @param flag          Admin flag to use.
 * @param enabled       True to enable, false to disable.
 */
native void SetAdminFlag(AdminId id, AdminFlag flag, bool enabled);

/**
 * Returns whether or not a flag is enabled on an admin.
 *
 * @param id            AdminId index of the admin.
 * @param flag          Admin flag to use.
 * @param mode          Access mode to check.
 * @return              True if enabled, false otherwise.
 */
native bool GetAdminFlag(AdminId id, AdminFlag flag, AdmAccessMode mode=Access_Effective);

/**
 * Returns the bitstring of access flags on an admin.
 *
 * @param id            AdminId index of the admin.
 * @param mode          Access mode to use.
 * @return              A bitstring containing which flags are enabled.
 */
native int GetAdminFlags(AdminId id, AdmAccessMode mode);

/**
 * Adds a group to an admin's inherited group list.  Any flags the group has
 * will be added to the admin's effective flags.
 *
 * @param id            AdminId index of the admin.
 * @param gid           GroupId index of the group.
 * @return              True on success, false on invalid input or duplicate membership.
 */
native bool AdminInheritGroup(AdminId id, GroupId gid);

/**
 * Returns the number of groups this admin is a member of.
 *
 * @param id            AdminId index of the admin.
 * @return              Number of groups this admin is a member of.
 */
native int GetAdminGroupCount(AdminId id);

/**
 * Returns group information from an admin.
 *
 * @param id            AdminId index of the admin.
 * @param index         Group number to retrieve, from 0 to N-1, where N
 *                      is the value of GetAdminGroupCount(id).
 * @param name          Buffer to store the group's name.
 *                      Note: This will safely chop UTF-8 strings.
 * @param maxlength     Maximum size of the output name buffer.
 * @return              A GroupId index and a name pointer, or
 *                      INVALID_GROUP_ID and NULL if an error occurred.
 */
native GroupId GetAdminGroup(AdminId id, int index, char[] name, int maxlength);

/**
 * Sets a password on an admin.
 *
 * @param id            AdminId index of the admin.
 * @param password      String containing the password.
 */
native void SetAdminPassword(AdminId id, const char[] password);

/**
 * Gets an admin's password.
 *
 * @param id            AdminId index of the admin.
 * @param buffer        Optional buffer to store the admin's password.
 * @param maxlength     Maximum size of the output name buffer.
 *                      Note: This will safely chop UTF-8 strings.
 * @return              True if there was a password set, false otherwise.
 */
native bool GetAdminPassword(AdminId id, char[] buffer="", int maxlength=0);

/**
 * Attempts to find an admin by an auth method and an identity.
 *
 * @param auth          Auth method to try.
 * @param identity      Identity string to look up.
 * @return              An AdminId index if found, INVALID_ADMIN_ID otherwise.
 */
native AdminId FindAdminByIdentity(const char[] auth, const char[] identity);

/**
 * Removes an admin entry from the cache.
 *
 * @note This will remove any bindings to a specific user.
 *
 * @param id            AdminId index to remove/invalidate.
 * @return              True on success, false otherwise.
 */
native bool RemoveAdmin(AdminId id);

/**
 * Converts a flag bit string to a bit array.
 *
 * @param bits          Bit string containing the flags.
 * @param array         Array to write the flags to.  Enabled flags will be 'true'.
 * @param maxSize       Maximum number of flags the array can store.
 * @return              Number of flags written.
 */
native int FlagBitsToBitArray(int bits, bool[] array, int maxSize);

/**
 * Converts a flag array to a bit string.
 *
 * @param array         Array containing true or false for each AdminFlag.
 * @param maxSize       Maximum size of the flag array.
 * @return              A bit string composed of the array bits.
 */
native int FlagBitArrayToBits(const bool[] array, int maxSize);

/**
 * Converts an array of flags to bits.
 *
 * @param array         Array containing flags that are enabled.
 * @param numFlags      Number of flags in the array.
 * @return              A bit string composed of the array flags.
 */
native int FlagArrayToBits(const AdminFlag[] array, int numFlags);

/**
 * Converts a bit string to an array of flags.
 *
 * @param bits          Bit string containing the flags.
 * @param array         Output array to write flags.
 * @param maxSize       Maximum size of the flag array.
 * @return              Number of flags written.
 */
native int FlagBitsToArray(int bits, AdminFlag[] array, int maxSize);

/**
 * Finds a flag by its string name.
 *
 * @param name          Flag name (like "kick"), case sensitive.
 * @param flag          Variable to store flag in.
 * @return              True on success, false if not found.
 */
native bool FindFlagByName(const char[] name, AdminFlag &flag);

/**
 * Finds a flag by a given character.
 *
 * @param c             Flag ASCII character/token.
 * @param flag          Variable to store flag in.
 * @return              True on success, false if not found.
 */
native bool FindFlagByChar(int c, AdminFlag &flag);

/**
 * Finds the flag char for a given admin flag.
 *
 * @param flag          Flag to look up.
 * @param c             Variable to store flag char.
 * @return              True on success, false if not found.
 */
native bool FindFlagChar(AdminFlag flag, int &c);

/**
 * Converts a string of flag characters to a bit string.
 *
 * @param flags         Flag ASCII string.
 * @param numchars      Optional variable to store the number of bytes read.
 * @return              Bit string of ADMFLAG values.
 */
native int ReadFlagString(const char[] flags, int &numchars=0);

/**
 * Tests whether one admin can target another.
 *
 * The heuristics for this check are as follows:
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
 * @param admin         Admin doing the targetting (may be INVALID_ADMIN_ID).
 * @param target        Target admin (may be INVALID_ADMIN_ID).
 * @return              True if targetable, false if immune.
 */
native bool CanAdminTarget(AdminId admin, AdminId target);

/**
 * Creates an admin auth method.  This does not need to be called more than once
 * per method, ever.
 *
 * @param method        Name of the authentication method.
 * @return              True on success, false on failure.
 */
native bool CreateAuthMethod(const char[] method);

/**
 * Sets a group's immunity level.
 *
 * @param gid           Group Id.
 * @param level         Immunity level value.
 * @return              Old immunity level value.
 */
native int SetAdmGroupImmunityLevel(GroupId gid, int level);

/**
 * Gets a group's immunity level (defaults to 0).
 *
 * @param gid           Group Id.
 * @return              Immunity level value.
 */
native int GetAdmGroupImmunityLevel(GroupId gid);

/**
 * Sets an admin's immunity level.
 *
 * @param id            Admin Id.
 * @param level         Immunity level value.
 * @return              Old immunity level value.
 */
native int SetAdminImmunityLevel(AdminId id, int level);

/**
 * Gets an admin's immunity level.
 *
 * @param id            Admin Id.
 * @return              Immunity level value.
 */
native int GetAdminImmunityLevel(AdminId id);

/**
 * Converts a flag to its single bit.
 *
 * @param flag          Flag to convert.
 * @return              Bit representation of the flag.
 */
stock int FlagToBit(AdminFlag flag)
{
	return (1 << view_as<int>(flag));
}

/**
 * Converts a bit to an AdminFlag.
 *
 * @param bit           Bit to convert.
 * @param flag          Stores the converted flag by reference.
 * @return              True on success, false otherwise.
 */
stock bool BitToFlag(int bit, AdminFlag &flag)
{
	AdminFlag array[1];

	if (FlagBitsToArray(bit, array, 1))
	{
		flag = array[0];
		return true;
	}

	return false;
}
