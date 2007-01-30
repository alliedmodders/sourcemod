/**
 * ===============================================================
 * SourceMod (C)2004-2007 AlliedModders LLC.  All rights reserved.
 * ===============================================================
 *
 *  This file is part of the SourceMod/SourcePawn SDK.  This file may only be used 
 * or modified under the Terms and Conditions of its License Agreement, which is found 
 * in LICENSE.txt.  The Terms and Conditions for making SourceMod extensions/plugins 
 * may change at any time.  To view the latest information, see:
 *   http://www.sourcemod.net/license.php
 *
 * Version: $Id$
 */
 
#if defined _admin_included
 #endinput
#endif
#define _admin_included

enum AdminFlag
{
	Admin_None = 0,			/* Unused */
	Admin_Reservation,		/* Reserved slot */
	Admin_Kick,				/* Kick another user */
	Admin_Ban,				/* Ban another user */
	Admin_Unban,			/* Unban another user */
	Admin_Slay,				/* Slay/kill/damage another user */
	Admin_Changemap,		/* Change the map */
	Admin_Convars,			/* Change basic convars */
	Admin_Configs,			/* Change configs */
	Admin_Chat,				/* Special chat privileges */
	Admin_Vote,				/* Special vote privileges */
	Admin_Password,			/* Set a server password */
	Admin_RCON,				/* Use RCON */
	Admin_Cheats,			/* Change sv_cheats and use its commands */
	Admin_Root,				/* Root access */
	/* --- */
	AdminFlags_TOTAL,
};

enum OverrideType
{
	Override_Command = 1,	/* Command */
	Override_CommandGroup,	/* Command group */
};

enum OverrideRule
{
	Command_Deny = 0,
	Command_Allow = 1,
};

enum ImmunityType
{
	Immunity_Default = 1,	/* Immune from everyone with no immunity */
	Immunity_Global,		/* Immune from everyone (except root admins) */
};

/** Note: Groups are not Handles, nor are they indexes */
enum GroupId
{
	INVALID_GROUP_ID = -1,
};

/** Note: These are not Handles */
enum AdminId
{
	INVALID_ADMIN_ID = -1,
};

#define ADMIN_CACHE_OVERRIDES	(1<<0)
#define ADMIN_CACHE_ADMINS		(1<<1)
#define ADMIN_CACHE_GROUPS		((1<<2)|ADMIN_CACHE_ADMINS)

/**
 * Called when part of the admin cache needs to be rebuilt.
 * @note Groups should always be rebuilt before admins.
 *
 * @param cache_flags	Flags for which cache to dump.
 */
forward OnRebuildAdminCache(cache_flags);

/**
 * Tells the admin system to dump a portion of the cache.
 *
 * @param cache_flags	Flags for which cache to dump.  Specifying groups also dumps admins.
 * @param rebuild		If true, the rebuild forwards will fire.
 * @noreturn
 */
native DumpAdminCache(cache_flags, bool:rebuild);

/**
 * Adds a global command flag override.  Any command registered with this name
 * will assume the new flag.  This is applied retroactively as well.
 *
 * @param cmd			String containing command name (case sensitive).
 * @param type			Override type (specific command or group).
 * @param flag			New admin flag.
 * @noreturn
 */
native AddCommandOverride(const String:cmd[], OverrideType:type, AdminFlag:flag);

/**
 * Returns a command override.
 *
 * @param cmd			String containing command name (case sensitive).
 * @param type			Override type (specific command or group).
 * @param pFlag			By-reference cell to store the flag (undefined if not found).
 * @return				True if there is an override, false otherwise.
 */
native bool:GetCommandOverride(const String:cmd[], OverrideType:type, &AdminFlag:flag);

/**
 * Unsets a command override.
 *
 * @param cmd			String containing command name (case sensitive).
 * @param type			Override type (specific command or group).
 * @noreturn
 */
native UnsetCommandOverride(const String:cmd[], OverrideType:type);

/**
 * Adds a new group.  Name must be unique.
 *
 * @param group_name	String containing the group name.
 * @return				A new group id, INVALID_GROUP_ID if it already exists.
 */
native GroupId:CreateAdmGroup(const String:group_name[]);

/**
 * @brief Finds a group by name.
 *
 * @param group_name	String containing the group name.
 * @return				A group id, or INVALID_GROUP_ID if not found.
 */
native GroupId:FindAdmGroup(const String:group_name[]);

/**
 * Adds or removes a flag from a group's flag set.
 * @note These are called "add flags" because they add to a user's flags.
 *
 * @param id			Group id.
 * @param flag			Admin flag to toggle.
 * @param enabled		True to set the flag, false to unset/disable.
 * @noreturn
 */
native SetAdmGroupAddFlag(GroupId:id, AdminFlag:flag, bool:enabled);

/**
 * Gets the set value of an add flag on a group's flag set.
 * @note These are called "add flags" because they add to a user's flags.
 *
 * @param id			Group id.
 * @param flag			Admin flag to retrieve.
 * @return				True if enabled, false otherwise,
 */
native bool:GetAdmGroupAddFlag(GroupId:id, AdminFlag:flag);

/**
 * Returns the flag set that is added to a user from their group.
 * @note These are called "add flags" because they add to a user's flags.
 *
 * @param id			GroupId of the group.
 * @param flags			Array to store flags in.
 * @param total			Total number of flags that can be stored in the array (AdminFlags_TOTAL, usually).
 * @return				Number of flags that were written to the array.
 */
native GetAdmGroupAddFlagBits(GroupId:id, bool:flags[], total);

/**
 * Toggles a generic immunity type.
 *
 * @param id			Group id.
 * @param type			Generic immunity type.
 * @param enabled		True to enable, false otherwise.
 * @noreturn
 */
native SetAdmGroupImmunity(GroupId:id, ImmunityType:type, bool:enabled);

/**
 * Returns whether or not a group has global immunity.
 *
 * @param id			Group id.
 * @param type			Generic immunity type.
 * @return				True if the group has this immunity, false otherwise.
 */
native bool:GetAdmGroupImmunity(GroupId:id, ImmunityType:type);

/**
 * Adds immunity to a specific group.
 *
 * @param id			Group id.
 * @param other_id		Group id to receive immunity to.
 * @noreturn
 */
native SetAdmGroupImmuneFrom(GroupId:id, GroupId:other_id);

/**
 * Returns the number of specific group immunities.
 *
 * @param id			Group id.
 * @return				Number of group immunities.
 */
native GetAdmGroupImmuneCount(GroupId:id);

/**
 * Returns a group that this group is immune to given an index.
 *
 * @param id			Group id.
 * @param number		Index from 0 to N-1, from GetAdmGroupImmuneCount().
 * @return				GroupId that this group is immune to, or INVALID_GROUP_ID on failure.
 */
native GroupId:GetAdmGroupImmuneFrom(GroupId:id, number); 

/**
 * Adds a group-specific override type.
 *
 * @param id			Group id.
 * @param name			String containing command name (case sensitive).
 * @param type			Override type (specific command or group).
 * @param rule			Override allow/deny setting.
 * @noreturn
 */
native AddAdmGroupCmdOverride(GroupId:id, const String:name[], OverrideType:type, OverrideRule:rule);

/**
 * Retrieves a group-specific command override.
 *
 * @param id			Group id.
 * @param name			String containing command name (case sensitive).
 * @param type			Override type (specific command or group).
 * @param rule			Optional pointer to store allow/deny setting.
 * @return				True if an override exists, false otherwise.
 */
native bool:GetAdmGroupCmdOverride(GroupId:id, const String:name[], OverrideType:type, &OverrideRule:rule);
