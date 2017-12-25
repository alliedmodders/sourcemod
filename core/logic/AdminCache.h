/**
 * vim: set ts=4 sw=4 tw=99 noet :
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

#ifndef _INCLUDE_SOURCEMOD_ADMINCACHE_H_
#define _INCLUDE_SOURCEMOD_ADMINCACHE_H_

#include "common_logic.h"
#include <IAdminSystem.h>
#include "sm_memtable.h"
#include <sm_trie.h>
#include <sh_list.h>
#include <sh_string.h>
#include <IForwardSys.h>
#include <sm_stringhashmap.h>
#include <sm_namehashset.h>

using namespace SourceHook;

#define GRP_MAGIC_SET		0xDEADFADE
#define GRP_MAGIC_UNSET		0xFACEFACE
#define USR_MAGIC_SET		0xDEADFACE
#define USR_MAGIC_UNSET		0xFADEDEAD

typedef StringHashMap<OverrideRule> OverrideMap;

struct AdminGroup
{
	uint32_t magic;					/* Magic flag, for memory validation (ugh) */
	unsigned int immunity_level;	/* Immunity level */
	/* Immune from target table (-1 = nonexistent)
	 * [0] = number of entries
	 * [1...N] = immune targets
	 */
	int immune_table;			
	OverrideMap *pCmdTable;			/* Command override table (can be NULL) */
	OverrideMap *pCmdGrpTable;		/* Command group override table (can be NULL) */
	int next_grp;					/* Next group in the chain */
	int prev_grp;					/* Previous group in the chain */
	int nameidx;					/* Name */
	FlagBits addflags;				/* Additive flags */
};

struct AuthMethod
{
	String name;
	StringHashMap<AdminId> identities;

	AuthMethod(const char *name)
		: name(name)
	{
	}

	static inline bool matches(const char *name, const AuthMethod *method)
	{
		return strcmp(name, method->name.c_str()) == 0;
	}
	static inline uint32_t hash(const detail::CharsAndLength &key)
	{
		return key.hash();
	}
};

struct UserAuth
{
	unsigned int index;				/* Index into auth table */
	int identidx;					/* Index into the string table */
};

struct AdminUser
{
	uint32_t magic;					/* Magic flag, for memory validation */
	FlagBits flags;					/* Flags */
	FlagBits eflags;				/* Effective flags */
	int nameidx;					/* Name index */
	int password;					/* Password index */
	unsigned int grp_count;			/* Number of groups */
	unsigned int grp_size;			/* Size of groups table */
	int grp_table;					/* Group table itself */
	int next_user;					/* Next user in the list */
	int prev_user;					/* Previous user in the list */
	UserAuth auth;					/* Auth method for this user */
	unsigned int immunity_level;	/* Immunity level */
	unsigned int serialchange;		/* Serial # for changes */
};

class AdminCache : 
	public IAdminSystem,
	public SMGlobalClass
{
public:
	AdminCache();
	~AdminCache();
public: //SMGlobalClass
	void OnSourceModStartup(bool late);
	void OnSourceModAllInitialized();
	void OnSourceModLevelChange(const char *mapName);
	void OnSourceModShutdown();
	void OnSourceModPluginsLoaded();
public: //IAdminSystem
	/** Command cache stuff */
	void AddCommandOverride(const char *cmd, OverrideType type, FlagBits flags);
	bool GetCommandOverride(const char *cmd, OverrideType type, FlagBits *flags);
	void UnsetCommandOverride(const char *cmd, OverrideType type);
	/** Group cache stuff */
	GroupId AddGroup(const char *group_name);
	GroupId FindGroupByName(const char *group_name);
	void SetGroupAddFlag(GroupId id, AdminFlag flag, bool enabled);
	bool GetGroupAddFlag(GroupId id, AdminFlag flag);
	FlagBits GetGroupAddFlags(GroupId id);
	void SetGroupGenericImmunity(GroupId id, ImmunityType type, bool enabled);
	bool GetGroupGenericImmunity(GroupId id, ImmunityType type);
	void InvalidateGroup(GroupId id);
	void AddGroupImmunity(GroupId id, GroupId other_id);
	unsigned int GetGroupImmunityCount(GroupId id);
	GroupId GetGroupImmunity(GroupId id, unsigned int number);
	void AddGroupCommandOverride(GroupId id, const char *name, OverrideType type, OverrideRule rule);
	bool GetGroupCommandOverride(GroupId id, const char *name, OverrideType type, OverrideRule *pRule);
	void DumpAdminCache(AdminCachePart part, bool rebuild);
	void AddAdminListener(IAdminListener *pListener);
	void RemoveAdminListener(IAdminListener *pListener);
	/** User stuff */
	void RegisterAuthIdentType(const char *name);
	AdminId CreateAdmin(const char *name);
	const char *GetAdminName(AdminId id);
	bool BindAdminIdentity(AdminId id, const char *auth, const char *ident);
	virtual void SetAdminFlag(AdminId id, AdminFlag flag, bool enabled);
	bool GetAdminFlag(AdminId id, AdminFlag flag, AccessMode mode);
	FlagBits GetAdminFlags(AdminId id, AccessMode mode);
	bool AdminInheritGroup(AdminId id, GroupId gid);
	unsigned int GetAdminGroupCount(AdminId id);
	GroupId GetAdminGroup(AdminId id, unsigned int index, const char **name);
	void SetAdminPassword(AdminId id, const char *password);
	const char *GetAdminPassword(AdminId id);
	AdminId FindAdminByIdentity(const char *auth, const char *identity);
	bool InvalidateAdmin(AdminId id);
	unsigned int FlagBitsToBitArray(FlagBits bits, bool array[], unsigned int maxSize);
	FlagBits FlagBitArrayToBits(const bool array[], unsigned int maxSize);
	FlagBits FlagArrayToBits(const AdminFlag array[], unsigned int numFlags);
	unsigned int FlagBitsToArray(FlagBits bits, AdminFlag array[], unsigned int maxSize);
	bool CheckAdminFlags(AdminId id, FlagBits bits);
	bool CanAdminTarget(AdminId id, AdminId target);
	void SetAdminFlags(AdminId id, AccessMode mode, FlagBits bits);
	bool FindFlag(const char *str, AdminFlag *pFlag);
	bool FindFlag(char c, AdminFlag *pAdmFlag);
	FlagBits ReadFlagString(const char *flags, const char **end);
	size_t FillFlagString(FlagBits bits, char *buffer, size_t maxlen);
	unsigned int GetAdminSerialChange(AdminId id);
	bool CanAdminUseCommand(int client, const char *cmd);
	const char *GetGroupName(GroupId gid);
	unsigned int SetGroupImmunityLevel(GroupId gid, unsigned int level);
	unsigned int GetGroupImmunityLevel(GroupId gid);
	unsigned int SetAdminImmunityLevel(AdminId id, unsigned int level);
	unsigned int GetAdminImmunityLevel(AdminId id);
	bool CheckAccess(int client, 
		const char *cmd, 
		FlagBits flags, 
		bool override_only);
	bool FindFlagChar(AdminFlag flag, char *c);
	bool IsValidAdmin(AdminId id);
	bool CheckClientCommandAccess(int client, const char *cmd, FlagBits cmdflags);
public:
	bool DumpCache(const char *filename);
	AdminGroup *GetGroup(GroupId gid);
	AdminUser *GetUser(AdminId id);
	const char *GetString(int idx);
	bool CheckAdminCommandAccess(AdminId adm, const char *cmd, FlagBits flags);
private:
	void _UnsetCommandOverride(const char *cmd);
	void _UnsetCommandGroupOverride(const char *group);
	void InvalidateGroupCache();
	void InvalidateAdminCache(bool unlink_admins);
	void DumpCommandOverrideCache(OverrideType type);
	AuthMethod *GetMethodByIndex(unsigned int index);
	bool GetMethodIndex(const char *name, unsigned int *_index);
	const char *GetMethodName(unsigned int index);
	void NameFlag(const char *str, AdminFlag flag);
	bool GetUnifiedSteamIdentity(const char *ident, char *out, size_t maxlen);
public:
	typedef StringHashMap<FlagBits> FlagMap;

	BaseStringTable *m_pStrings;
	BaseMemTable *m_pMemory;
	FlagMap m_CmdOverrides;
	FlagMap m_CmdGrpOverrides;
	int m_FirstGroup;
	int m_LastGroup;
	int m_FreeGroupList;
	StringHashMap<GroupId> m_Groups;
	List<IAdminListener *> m_hooks;
	List<AuthMethod *> m_AuthMethods;
	NameHashSet<AuthMethod *> m_AuthTables;
	IForward *m_pCacheFwd;
	int m_FirstUser;
	int m_LastUser;
	int m_FreeUserList;
	bool m_InvalidatingAdmins;
	bool m_destroying;
	StringHashMap<AdminFlag> m_LevelNames;
};

extern AdminCache g_Admins;

#endif //_INCLUDE_SOURCEMOD_ADMINCACHE_H_
