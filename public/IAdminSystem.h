/**
 * vim: set ts=4 :
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

#ifndef _INCLUDE_SOURCEMOD_ADMINISTRATION_SYSTEM_H_
#define _INCLUDE_SOURCEMOD_ADMINISTRATION_SYSTEM_H_

#include <IShareSys.h>

#define SMINTERFACE_ADMINSYS_NAME		"IAdminSys"
#define SMINTERFACE_ADMINSYS_VERSION	1

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
 */

namespace SourceMod
{
	/**
	 * @brief Access levels (flags) for admins.
	 */
	enum AdminFlag
	{
		Admin_None = 0,
		Admin_Reservation,		/**< Reserved slot */
		Admin_Kick,				/**< Kick another user */
		Admin_Ban,				/**< Ban another user */
		Admin_Unban,			/**< Unban another user */
		Admin_Slay,				/**< Slay/kill/damage another user */
		Admin_Changemap,		/**< Change the map */
		Admin_Convars,			/**< Change basic convars */
		Admin_Configs,			/**< Change configs */
		Admin_Chat,				/**< Special chat privileges */
		Admin_Vote,				/**< Special vote privileges */
		Admin_Password,			/**< Set a server password */
		Admin_RCON,				/**< Use RCON */
		Admin_Cheats,			/**< Change sv_cheats and use its commands */
		Admin_Root,				/**< All access by default */
		/* --- */
		AdminFlags_TOTAL,
	};

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
	 * @brief Specifies a generic immunity type.
	 */
	enum ImmunityType
	{
		Immunity_Default = 1,	/**< Immune from everyone with no immunity */
		Immunity_Global,		/**< Immune from everyone (except root admins) */
	};

	/**
	 * @brief Represents an index to one group.
	 */
	typedef int		GroupId;

	#define ADMIN_CACHE_OVERRIDES	(1<<0)
	#define ADMIN_CACHE_ADMINS		(1<<1)
	#define ADMIN_CACHE_GROUPS		((1<<2)|ADMIN_CACHE_ADMINS)

	/**
	 * @brief Represents an invalid/nonexistant group or an erroneous operation.
	 */
	#define INVALID_GROUP_ID	-1

	/**
	 * @brief Provides callbacks for admin cache operations.
	 */
	class IAdminListener
	{
	public:
		/**
		 * Called when part of the admin cache needs to be rebuilt.  
		 * Groups should always be rebuilt before admins.
		 *
		 * @param cache_flags	Flags for which cache to dump.
		 */
		virtual void OnRebuildAdminCache(int cache_flags) =0;
	};

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
		 * @param flag			New admin flag.
		 */
		virtual void AddCommandOverride(const char *cmd, OverrideType type, AdminFlag flag) =0;

		/**
		 * @brief Returns a command override.
		 *
		 * @param cmd			String containing command name (case sensitive).
		 * @param type			Override type (specific command or group).
		 * @param pFlag			Optional pointer to the set flag.
		 * @return				True if there is an override, false otherwise.
		 */
		virtual bool GetCommandOverride(const char *cmd, OverrideType type, AdminFlag *pFlag) =0;

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
		 * @param flags			Array to store flags bits in.
		 * @param total			Total number of flags that can be stored in the array.
		 * @return				Number of flags that were written to the array.
		 */
		virtual unsigned int GetGroupAddFlagBits(GroupId id, bool flags[], unsigned int total) =0;

		/**
		 * @brief Toggles a generic immunity type.
		 *
		 * @param id			Group id.
		 * @param type			Generic immunity type.
		 * @param enabled		True to enable, false otherwise.
		 */
		virtual void SetGroupGenericImmunity(GroupId id, ImmunityType type, bool enabled) =0;

		/**
		 * @brief Returns whether or not a group has global immunity.
		 *
		 * @param id			Group id.
		 * @param type			Generic immunity type.
		 * @return				True if the group has this immunity, false otherwise.
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
		 * @brief Invalidates and removes a group from the group cache.
		 *
		 * @param id			Group id.
		 */
		virtual void InvalidateGroup(GroupId id) =0;

		/**
		 * @brief Tells the admin system to dump a portion of the cache.  
		 * This calls into plugin forwards to rebuild the cache.
		 *
		 * @param cache_flags	Flags for which cache to dump.  Specifying groups also dumps admins.
		 * @param rebuild		If true, the rebuild forwards/events will fire.
		 */
		virtual void DumpAdminCache(int cache_flags, bool rebuild) =0;

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
	};
};

#endif //_INCLUDE_SOURCEMOD_ADMINISTRATION_SYSTEM_H_
