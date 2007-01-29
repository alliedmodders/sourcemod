/**
 * vim: set ts=4 :
 * ===============================================================
 * SourceMod (C)2004-2007 AlliedModders LLC.  All rights reserved.
 * ===============================================================
 *
 * This file is not open source and may not be copied without explicit
 * written permission of AlliedModders LLC.  This file may not be redistributed 
 * in whole or significant part.
 * For information, see LICENSE.txt or http://www.sourcemod.net/license.php
 *
 * Version: $Id$
 */

#ifndef _INCLUDE_SOURCEMOD_ADMINCACHE_H_
#define _INCLUDE_SOURCEMOD_ADMINCACHE_H_

#include "sm_memtable.h"
#include <sm_trie.h>
#include <sh_list.h>
#include <sh_string.h>
#include <IAdminSystem.h>
#include "sm_globals.h"
#include <IForwardSys.h>

using namespace SourceHook;

#define GRP_MAGIC_SET		0xDEADFADE
#define GRP_MAGIC_UNSET		0xFACEFACE
#define USR_MAGIC_SET		0xDEADFACE
#define USR_MAGIC_UNSET		0xFADEDEAD

struct AdminGroup
{
	uint32_t magic;					/* Magic flag, for memory validation (ugh) */
	bool immune_global;				/* Global immunity? */
	bool immune_default;			/* Default immunity? */
	/* Immune from target table (-1 = nonexistant)
	 * [0] = number of entries
	 * [1...N] = immune targets
	 */
	int immune_table;			
	Trie *pCmdTable;				/* Command override table (can be NULL) */
	Trie *pCmdGrpTable;				/* Command group override table (can be NULL) */
	int next_grp;					/* Next group in the chain */
	int prev_grp;					/* Previous group in the chain */
	int nameidx;					/* Name */
	bool addflags[AdminFlags_TOTAL];	/* Additive flags */
};

struct AuthMethod
{
	String name;
	Trie *table;
};

struct UserAuth
{
	unsigned int index;				/* Index into auth table */
	int identidx;					/* Index into the string table */
};

struct AdminUser
{
	uint32_t magic;						/* Magic flag, for memory validation */
	bool flags[AdminFlags_TOTAL];	/* Base flags */
	bool eflags[AdminFlags_TOTAL];	/* Effective flags */
	int nameidx;					/* Name index */
	int password;					/* Password index */
	unsigned int grp_count;			/* Number of groups */
	unsigned int grp_size;			/* Size of groups table */
	int grp_table;					/* Group table itself */
	int next_user;					/* Next user in ze list */
	int prev_user;					/* Prev user in the list */
	UserAuth auth;					/* Auth method for this user */
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
	void OnSourceModShutdown();
public: //IAdminSystem
	/** Command cache stuff */
	void AddCommandOverride(const char *cmd, OverrideType type, AdminFlag flag);
	bool GetCommandOverride(const char *cmd, OverrideType type, AdminFlag *pFlag);
	void UnsetCommandOverride(const char *cmd, OverrideType type);
	/** Group cache stuff */
	GroupId AddGroup(const char *group_name);
	GroupId FindGroupByName(const char *group_name);
	void SetGroupAddFlag(GroupId id, AdminFlag flag, bool enabled);
	bool GetGroupAddFlag(GroupId id, AdminFlag flag);
	unsigned int GetGroupAddFlagBits(GroupId id, bool flags[], unsigned int total);
	void SetGroupGenericImmunity(GroupId id, ImmunityType type, bool enabled);
	bool GetGroupGenericImmunity(GroupId id, ImmunityType type);
	void InvalidateGroup(GroupId id);
	void AddGroupImmunity(GroupId id, GroupId other_id);
	unsigned int GetGroupImmunityCount(GroupId id);
	GroupId GetGroupImmunity(GroupId id, unsigned int number);
	void AddGroupCommandOverride(GroupId id, const char *name, OverrideType type, OverrideRule rule);
	bool GetGroupCommandOverride(GroupId id, const char *name, OverrideType type, OverrideRule *pRule);
	void DumpAdminCache(int cache_flags, bool rebuild);
	void AddAdminListener(IAdminListener *pListener);
	void RemoveAdminListener(IAdminListener *pListener);
	/** User stuff */
	void RegisterAuthIdentType(const char *name);
	AdminId CreateAdmin(const char *name);
	const char *GetAdminName(AdminId id);
	bool BindAdminIdentity(AdminId id, const char *auth, const char *ident);
	virtual void SetAdminFlag(AdminId id, AdminFlag flag, bool enabled);
	bool GetAdminFlag(AdminId id, AdminFlag flag, AccessMode mode);
	unsigned int GetAdminFlags(AdminId id, bool flags[], unsigned int total, AccessMode mode);
	bool AdminInheritGroup(AdminId id, GroupId gid);
	unsigned int GetAdminGroupCount(AdminId id);
	GroupId GetAdminGroup(AdminId id, unsigned int index, const char **name);
	void SetAdminPassword(AdminId id, const char *password);
	const char *GetAdminPassword(AdminId id);
	AdminId FindAdminByIdentity(const char *auth, const char *identity);
	bool InvalidateAdmin(AdminId id);
private:
	void _AddCommandOverride(const char *cmd, AdminFlag flag);
	void _AddCommandGroupOverride(const char *group, AdminFlag flag);
	bool _GetCommandOverride(const char *cmd, AdminFlag *pFlag);
	bool _GetCommandGroupOverride(const char *group, AdminFlag *pFlag);
	void _UnsetCommandOverride(const char *cmd);
	void _UnsetCommandGroupOverride(const char *group);
	void InvalidateGroupCache();
	void InvalidateAdminCache(bool unlink_admins);
	void DumpCommandOverrideCache(OverrideType type);
	Trie *GetMethodByIndex(unsigned int index);
	bool GetMethodIndex(const char *name, unsigned int *_index);
public:
	BaseStringTable *m_pStrings;
	BaseMemTable *m_pMemory;
	Trie *m_pCmdOverrides;
	Trie *m_pCmdGrpOverrides;
	int m_FirstGroup;
	int m_LastGroup;
	int m_FreeGroupList;
	Trie *m_pGroups;
	List<IAdminListener *> m_hooks;
	List<AuthMethod> m_AuthMethods;
	Trie *m_pAuthTables;
	IForward *m_pCacheFwd;
	int m_FirstUser;
	int m_LastUser;
	int m_FreeUserList;
};

extern AdminCache g_Admins;

#endif //_INCLUDE_SOURCEMOD_ADMINCACHE_H_
