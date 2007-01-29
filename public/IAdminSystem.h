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

	#define ADMIN_CACHE_OVERRIDES	(1<<0)
	#define ADMIN_CACHE_ADMINS		(1<<1)
	#define ADMIN_CACHE_GROUPS		((1<<2)|ADMIN_CACHE_ADMINS)

	/**
	 * @brief Represents an invalid/nonexistant group or an erroneous operation.
	 */
	#define INVALID_GROUP_ID	-1

	/**
	 * @brief Represents an invalid/nonexistant user or an erroneous operation.
	 */
	#define INVALID_ADMIN_ID	-1

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
		 * @brief Returns a bitarray of flags enabled on an admin.
		 *
		 * @param id		AdminId index of the admin.
		 * @param flag		Array to store flag bits in.
		 * @param total		Maximum size of the flag array.
		 * @param mode		Access mode to use.
		 * @return			Number of flags written to the array.
		 */
		virtual unsigned int GetAdminFlags(AdminId id, 
										   bool flags[], 
										   unsigned int total, 
										   AccessMode mode) =0;

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
		 * @param passwd	String containing the password.
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
	};
};

#endif //_INCLUDE_SOURCEMOD_ADMINISTRATION_SYSTEM_H_
