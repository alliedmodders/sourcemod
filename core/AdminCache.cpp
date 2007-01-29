/**
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

#include <string.h>
#include <assert.h>
#include "AdminCache.h"
#include "ShareSys.h"
#include "ForwardSys.h"

AdminCache g_Admins;

AdminCache::AdminCache()
{
	m_pCmdOverrides = NULL;
	m_pCmdGrpOverrides = NULL;
	m_pStrings = new BaseStringTable(1024);
	m_pMemory = m_pStrings->GetMemTable();
	m_FreeGroupList = m_FirstGroup = m_LastGroup = INVALID_GROUP_ID;
	m_FreeUserList = m_FirstUser = m_LastUser = INVALID_ADMIN_ID;
	m_pGroups = sm_trie_create();
	m_pCacheFwd = NULL;
	m_FirstGroup = -1;
}

AdminCache::~AdminCache()
{
	if (m_pCmdGrpOverrides)
	{
		sm_trie_destroy(m_pCmdGrpOverrides);
	}

	if (m_pCmdOverrides)
	{
		sm_trie_destroy(m_pCmdOverrides);
	}

	InvalidateGroupCache();

	if (m_pGroups)
	{
		sm_trie_destroy(m_pGroups);
	}

	delete m_pStrings;
}

void AdminCache::OnSourceModStartup(bool late)
{
	RegisterAuthIdentType("steam");
	RegisterAuthIdentType("name");
	RegisterAuthIdentType("ip");
}

void AdminCache::OnSourceModAllInitialized()
{
	m_pCacheFwd = g_Forwards.CreateForward("OnRebuildAdminCache", ET_Ignore, 1, NULL, Param_Cell);
	g_ShareSys.AddInterface(NULL, this);
}

void AdminCache::OnSourceModShutdown()
{
	g_Forwards.ReleaseForward(m_pCacheFwd);
	m_pCacheFwd = NULL;
}

void AdminCache::AddCommandOverride(const char *cmd, OverrideType type, AdminFlag flag)
{
	if (type == Override_Command)
	{
		_AddCommandOverride(cmd, flag);
	} else if (type == Override_CommandGroup) {
		_AddCommandGroupOverride(cmd, flag);
	}
}

bool AdminCache::GetCommandOverride(const char *cmd, OverrideType type, AdminFlag *pFlag)
{
	if (type == Override_Command)
	{
		return _GetCommandOverride(cmd, pFlag);
	} else if (type == Override_CommandGroup) {
		return _GetCommandGroupOverride(cmd, pFlag);
	}

	return false;
}

void AdminCache::UnsetCommandOverride(const char *cmd, OverrideType type)
{
	if (type == Override_Command)
	{
		return _UnsetCommandOverride(cmd);
	} else if (type == Override_CommandGroup) {
		return _UnsetCommandGroupOverride(cmd);
	}
}

void AdminCache::_AddCommandGroupOverride(const char *group, AdminFlag flag)
{
	if (!m_pCmdGrpOverrides)
	{
		m_pCmdGrpOverrides = sm_trie_create();
	}

	/* :TODO: Notify command system */

	sm_trie_insert(m_pCmdGrpOverrides, group, (void *)flag);
}

void AdminCache::_AddCommandOverride(const char *cmd, AdminFlag flag)
{
	if (!m_pCmdOverrides)
	{
		m_pCmdOverrides = sm_trie_create();
	}

	/* :TODO: Notify command system */

	sm_trie_insert(m_pCmdOverrides, cmd, (void *)flag);
}

bool AdminCache::_GetCommandGroupOverride(const char *cmd, AdminFlag *pFlag)
{
	if (!m_pCmdGrpOverrides)
	{
		return false;
	}

	if (!pFlag)
	{
		return sm_trie_retrieve(m_pCmdGrpOverrides, cmd, NULL);
	} else {
		void *object;
		bool ret;
		if (ret=sm_trie_retrieve(m_pCmdGrpOverrides, cmd, &object))
		{
			*pFlag = (AdminFlag)(int)object;
		}
		return ret;
	}
}

bool AdminCache::_GetCommandOverride(const char *cmd, AdminFlag *pFlag)
{
	if (!m_pCmdOverrides)
	{
		return false;
	}

	if (!pFlag)
	{
		return sm_trie_retrieve(m_pCmdOverrides, cmd, NULL);
	} else {
		void *object;
		bool ret;
		if (ret=sm_trie_retrieve(m_pCmdOverrides, cmd, &object))
		{
			*pFlag = (AdminFlag)(int)object;
		}
		return ret;
	}
}

void AdminCache::_UnsetCommandGroupOverride(const char *group)
{
	if (!m_pCmdGrpOverrides)
	{
		return;
	}

	/* :TODO: Notify command system */

	sm_trie_delete(m_pCmdGrpOverrides, group);
}

void AdminCache::_UnsetCommandOverride(const char *cmd)
{
	if (!m_pCmdOverrides)
	{
		return;
	}

	/* :TODO: Notify command system */

	sm_trie_delete(m_pCmdOverrides, cmd);
}

void AdminCache::DumpCommandOverrideCache(OverrideType type)
{
	if (type == Override_Command && m_pCmdOverrides)
	{
		sm_trie_clear(m_pCmdOverrides);
	} else if (type == Override_CommandGroup && m_pCmdGrpOverrides) {
		sm_trie_clear(m_pCmdGrpOverrides);
	}
}

AdminId AdminCache::CreateAdmin(const char *name)
{
	AdminId id;
	AdminUser *pUser;

	if (m_FreeUserList != INVALID_ADMIN_ID)
	{
		pUser = (AdminUser *)m_pMemory->GetAddress(m_FreeUserList);
		assert(pUser->magic == USR_MAGIC_UNSET);
		id = m_FreeUserList;
		m_FreeUserList = pUser->next_user;
	} else {
		id = m_pMemory->CreateMem(sizeof(AdminUser), (void **)&pUser);
		pUser->grp_size = 0;
		pUser->grp_table = -1;
		pUser->auth_size = 0;
		pUser->auth_table = -1;
	}

	memset(pUser->flags, 0, sizeof(pUser->flags));
	memset(pUser->eflags, 0, sizeof(pUser->flags));
	pUser->grp_count = 0;
	pUser->auth_count = 0;
	pUser->password = -1;
	pUser->magic = USR_MAGIC_SET;

	if (m_FirstUser == INVALID_ADMIN_ID)
	{
		m_FirstUser = id;
		m_LastUser = id;
	} else {
		AdminUser *pPrev = (AdminUser *)m_pMemory->GetAddress(m_LastUser);
		pPrev->next_user = id;
		pUser->prev_user = m_LastUser;
		m_LastUser = id;
	}

	if (name && name[0] != '\0')
	{
		pUser->nameidx = m_pStrings->AddString(name);
	}

	return id;
}

GroupId AdminCache::AddGroup(const char *group_name)
{
	if (sm_trie_retrieve(m_pGroups, group_name, NULL))
	{
		return INVALID_GROUP_ID;
	}

	GroupId id;
	AdminGroup *pGroup;
	if (m_FreeGroupList != INVALID_GROUP_ID)
	{
		pGroup = (AdminGroup *)m_pMemory->GetAddress(m_FreeGroupList);
		assert(pGroup->magic == GRP_MAGIC_UNSET);
		id = m_FreeGroupList;
		m_FreeGroupList = pGroup->next_grp;
	} else {
		id = m_pMemory->CreateMem(sizeof(AdminGroup), (void **)&pGroup);
	}

	pGroup->immune_default = false;
	pGroup->immune_global = false;
	pGroup->immune_table = -1;
	pGroup->magic = GRP_MAGIC_SET;
	pGroup->next_grp = INVALID_GROUP_ID;
	pGroup->pCmdGrpTable = NULL;
	pGroup->pCmdTable = NULL;
	memset(pGroup->addflags, 0, sizeof(AdminFlag) * AdminFlags_TOTAL);

	if (m_FirstGroup == INVALID_GROUP_ID)
	{
		m_FirstGroup = id;
		m_LastGroup = id;
		pGroup->prev_grp = INVALID_GROUP_ID;
	} else {
		AdminGroup *pPrev = (AdminGroup *)m_pMemory->GetAddress(m_LastGroup);
		assert(pPrev->magic == GRP_MAGIC_SET);
		pPrev->next_grp = id;
		pGroup->prev_grp = m_LastGroup;
		m_LastGroup = id;
	}

	pGroup->nameidx = m_pStrings->AddString(group_name);

	sm_trie_insert(m_pGroups, group_name, (void *)id);

	return id;
}

GroupId AdminCache::FindGroupByName(const char *group_name)
{
	void *object;

	if (!sm_trie_retrieve(m_pGroups, group_name, &object))
	{
		return INVALID_GROUP_ID;
	}

	GroupId id = (GroupId)object;
	AdminGroup *pGroup = (AdminGroup *)m_pMemory->GetAddress(id);

	if (!pGroup || pGroup->magic != GRP_MAGIC_SET)
	{
		return INVALID_GROUP_ID;
	}

	return id;
}

void AdminCache::SetGroupAddFlag(GroupId id, AdminFlag flag, bool enabled)
{
	AdminGroup *pGroup = (AdminGroup *)m_pMemory->GetAddress(id);
	if (!pGroup || pGroup->magic != GRP_MAGIC_SET)
	{
		return;
	}

	if (flag < Admin_None || flag >= AdminFlags_TOTAL)
	{
		return;
	}

	pGroup->addflags[flag] = enabled;
}

bool AdminCache::GetGroupAddFlag(GroupId id, AdminFlag flag)
{
	AdminGroup *pGroup = (AdminGroup *)m_pMemory->GetAddress(id);
	if (!pGroup || pGroup->magic != GRP_MAGIC_SET)
	{
		return false;
	}

	if (flag < Admin_None || flag >= AdminFlags_TOTAL)
	{
		return false;
	}

	return pGroup->addflags[flag];
}

unsigned int AdminCache::GetGroupAddFlagBits(GroupId id, bool flags[], unsigned int total)
{
	AdminGroup *pGroup = (AdminGroup *)m_pMemory->GetAddress(id);
	if (!pGroup || pGroup->magic != GRP_MAGIC_SET)
	{
		return 0;
	}

	unsigned int i;

	for (i = Admin_None;
		 i < AdminFlags_TOTAL && i < total;
		 i++)
	{
		flags[i] = pGroup->addflags[i];
	}

	return i;
}

void AdminCache::SetGroupGenericImmunity(GroupId id, ImmunityType type, bool enabled)
{
	AdminGroup *pGroup = (AdminGroup *)m_pMemory->GetAddress(id);
	if (!pGroup || pGroup->magic != GRP_MAGIC_SET)
	{
		return;
	}

	if (type == Immunity_Default)
	{
		pGroup->immune_default = enabled;
	} else if (type == Immunity_Global) {
		pGroup->immune_global = enabled;
	} 
}

bool AdminCache::GetGroupGenericImmunity(GroupId id, ImmunityType type)
{
	AdminGroup *pGroup = (AdminGroup *)m_pMemory->GetAddress(id);
	if (!pGroup || pGroup->magic != GRP_MAGIC_SET)
	{
		return false;
	}

	if (type == Immunity_Default)
	{
		return pGroup->immune_default;
	} else if (type == Immunity_Global) {
		return pGroup->immune_global;
	}

	return false;
}

void AdminCache::AddGroupImmunity(GroupId id, GroupId other_id)
{
	AdminGroup *pGroup = (AdminGroup *)m_pMemory->GetAddress(id);
	if (!pGroup || pGroup->magic != GRP_MAGIC_SET)
	{
		return;
	}

	AdminGroup *pOther = (AdminGroup *)m_pMemory->GetAddress(id);
	if (!pOther || pOther->magic != GRP_MAGIC_SET)
	{
		return;
	}

	/* We always need to resize the immunity table */
	int *table, tblidx;
	if (pOther->immune_table == -1)
	{
		tblidx = m_pMemory->CreateMem(sizeof(int) * 2, (void **)&table);
		table[0] = 0;
	} else {
		int *old_table = (int *)m_pMemory->GetAddress(pOther->immune_table);
		tblidx = m_pMemory->CreateMem(sizeof(int) * (old_table[0] + 2), (void **)&table);
		/* Get the old address again in caes of resize */
		old_table = (int *)m_pMemory->GetAddress(pOther->immune_table);
		table[0] = old_table[0];
		for (unsigned int i=1; i<=(unsigned int)old_table[0]; i++)
		{
			table[i] = old_table[i];
		}
	}

	/* Assign */
	pOther->immune_table = tblidx;

	/* Add to the array */
	table[0]++;
	table[table[0]] = other_id;
}

unsigned int AdminCache::GetGroupImmunityCount(GroupId id)
{
	AdminGroup *pGroup = (AdminGroup *)m_pMemory->GetAddress(id);
	if (!pGroup || pGroup->magic != GRP_MAGIC_SET)
	{
		return 0;
	}

	if (pGroup->immune_table == -1)
	{
		return 0;
	}

	int *table = (int *)m_pMemory->GetAddress(pGroup->immune_table);

	return table[0];
}

GroupId AdminCache::GetGroupImmunity(GroupId id, unsigned int number)
{
	AdminGroup *pGroup = (AdminGroup *)m_pMemory->GetAddress(id);
	if (!pGroup || pGroup->magic != GRP_MAGIC_SET)
	{
		return INVALID_GROUP_ID;
	}

	if (pGroup->immune_table == -1)
	{
		return INVALID_GROUP_ID;
	}

	int *table = (int *)m_pMemory->GetAddress(pGroup->immune_table);
	if (number >= (unsigned int)table[0])
	{
		return INVALID_GROUP_ID;
	}

	return table[1 + number];
}

void AdminCache::AddGroupCommandOverride(GroupId id, const char *name, OverrideType type, OverrideRule rule)
{
	AdminGroup *pGroup = (AdminGroup *)m_pMemory->GetAddress(id);
	if (!pGroup || pGroup->magic != GRP_MAGIC_SET)
	{
		return;
	}

	Trie *pTrie = NULL;
	if (type == Override_Command)
	{
		if (pGroup->pCmdTable == NULL)
		{
			pGroup->pCmdTable = sm_trie_create();
		}
		pTrie = pGroup->pCmdTable;
	} else if (type == Override_CommandGroup) {
		if (pGroup->pCmdGrpTable == NULL)
		{
			pGroup->pCmdGrpTable = sm_trie_create();
		}
		pTrie = pGroup->pCmdGrpTable;
	} else {
		return;
	}

	sm_trie_insert(pTrie, name, (void *)(int)rule);
}

bool AdminCache::GetGroupCommandOverride(GroupId id, const char *name, OverrideType type, OverrideRule *pRule)
{
	AdminGroup *pGroup = (AdminGroup *)m_pMemory->GetAddress(id);
	if (!pGroup || pGroup->magic != GRP_MAGIC_SET)
	{
		return false;
	}

	Trie *pTrie = NULL;
	if (type == Override_Command)
	{
		if (pGroup->pCmdTable == NULL)
		{
			return false;
		}
		pTrie = pGroup->pCmdTable;
	} else if (type == Override_CommandGroup) {
		if (pGroup->pCmdGrpTable == NULL)
		{
			return false;
		}
		pTrie = pGroup->pCmdGrpTable;
	} else {
		return false;
	}

	void *object;
	if (!sm_trie_retrieve(pTrie, name, &object))
	{
		return false;
	}

	if (pRule)
	{
		*pRule = (OverrideRule)(int)object;
	}
	
	return true;
}

Trie *AdminCache::GetMethodByIndex(unsigned int index)
{
	List<AuthMethod>::iterator iter;
	for (iter=m_AuthMethods.begin();
		 iter!=m_AuthMethods.end();
		 iter++)
	{
		if (index-- == 0)
		{
			return (*iter).table;
		}
	}

	return NULL;
}

bool AdminCache::InvalidateAdmin(AdminId id)
{
	AdminUser *pUser = (AdminUser *)m_pMemory->GetAddress(id);
	AdminUser *pOther;
	if (!pUser || pUser->magic != USR_MAGIC_SET)
	{
		return false;
	}

	/* Unlink from the dbl link list */
	if (id == m_FirstUser && id == m_LastUser)
	{
		m_FirstUser = INVALID_ADMIN_ID;
		m_LastUser = INVALID_ADMIN_ID;
	} else if (id == m_FirstUser) {
		m_FirstUser = pUser->next_user;
		pOther = (AdminUser *)m_pMemory->GetAddress(m_FirstUser);
		pOther->prev_user = INVALID_ADMIN_ID;
	} else if (id == m_LastUser) {
		m_LastUser = pUser->prev_user;
		pOther = (AdminUser *)m_pMemory->GetAddress(m_LastUser);
		pOther->next_user = INVALID_ADMIN_ID;
	} else {
		pOther = (AdminUser *)m_pMemory->GetAddress(pUser->prev_user);
		pOther->next_user = pUser->next_user;
		pOther = (AdminUser *)m_pMemory->GetAddress(pUser->next_user);
		pOther->prev_user = pUser->prev_user;
	}

	/* Unlink from auth tables */
	if (pUser->auth_count > 0)
	{
		UserAuth *pAuth = (UserAuth *)m_pMemory->GetAddress(pUser->auth_table);
		Trie *pTrie;
		for (unsigned int i=0; i<pUser->auth_count; i++)
		{
			pTrie = GetMethodByIndex(pAuth[i].index);
			if (!pTrie)
			{
				continue;
			}
			sm_trie_delete(pTrie, m_pStrings->GetString(pAuth->identidx));
		}
	}

	/* Clear table counts */
	pUser->grp_count = 0;
	pUser->auth_count = 0;

	/* Link into free list */
	pUser->magic = USR_MAGIC_UNSET;
	pUser->next_user = m_FreeUserList;
	m_FreeUserList = id;

	return true;
}


void AdminCache::InvalidateGroup(GroupId id)
{
	AdminGroup *pGroup = (AdminGroup *)m_pMemory->GetAddress(id);
	AdminGroup *pOther;

	if (!pGroup || pGroup->magic != GRP_MAGIC_SET)
	{
		return;
	}

	const char *str = m_pStrings->GetString(pGroup->nameidx);
	sm_trie_delete(m_pGroups, str);

	/* Unlink from the live dbllink list */
	if (id == m_FirstGroup && id == m_LastGroup)
	{
		m_LastGroup = INVALID_GROUP_ID;
		m_FirstGroup = INVALID_GROUP_ID;
	} else if (id == m_FirstGroup) {
		m_FirstGroup = pGroup->next_grp;
		pOther = (AdminGroup *)m_pMemory->GetAddress(m_FirstGroup);
		pOther->prev_grp = INVALID_GROUP_ID;
	} else if (id == m_LastGroup) {
		m_LastGroup = pGroup->prev_grp;
		pOther = (AdminGroup *)m_pMemory->GetAddress(m_LastGroup);
		pOther->next_grp = INVALID_GROUP_ID;
	} else {
		pOther = (AdminGroup *)m_pMemory->GetAddress(pGroup->prev_grp);
		pOther->next_grp = pGroup->next_grp;
		pOther = (AdminGroup *)m_pMemory->GetAddress(pGroup->next_grp);
		pOther->prev_grp = pGroup->prev_grp;
	}

	/* Free any used memory to be safe */
	if (pGroup->pCmdGrpTable)
	{
		sm_trie_destroy(pGroup->pCmdGrpTable);
		pGroup->pCmdGrpTable = NULL;
	}
	if (pGroup->pCmdTable)
	{
		sm_trie_destroy(pGroup->pCmdTable);
		pGroup->pCmdTable = NULL;
	}

	/* Link into the free list */
	pGroup->magic = GRP_MAGIC_UNSET;
	pGroup->next_grp = m_FreeGroupList;
	m_FreeGroupList = id;

	int idx = m_FirstUser;
	AdminUser *pUser;
	int *table;
	while (idx != INVALID_ADMIN_ID)
	{
		pUser = (AdminUser *)m_pMemory->GetAddress(idx);
		if (pUser->grp_count > 0)
		{
			table = (int *)m_pMemory->GetAddress(pUser->grp_table);
			for (unsigned int i=0; i<pUser->grp_count; i++)
			{
				if (table[i] == id)
				{
					/* We have to remove this entry */
					for (unsigned int j=i+1; j<pUser->grp_count; j++)
					{
						/* Move everything down by one */
						table[j-1] = table[j];
					}
					/* Decrease count */
					pUser->grp_count--;
					/* Recalculate effective flags */
					memset(pUser->eflags, 0, sizeof(pUser->eflags));
					for (unsigned int j=0; j<pUser->grp_count; j++)
					{
						pOther = (AdminGroup *)m_pMemory->GetAddress(table[j]);
						for (unsigned int k=0; k<AdminFlags_TOTAL; k++)
						{
							pUser->eflags[k] = (pUser->flags[k] || pOther->addflags[k]);
						}
					}
					/* Break now, duplicates aren't allowed */
					break;
				}
			}
		}
		idx = pUser->next_user;
	}
}

void AdminCache::InvalidateGroupCache()
{
	/* Nuke the free list */
	m_FreeGroupList = -1;

	/* Nuke reverse lookups */
	sm_trie_clear(m_pGroups);

	/* Free memory on groups */
	GroupId cur = m_FirstGroup;
	AdminGroup *pGroup;
	while (cur != INVALID_GROUP_ID)
	{
		pGroup = (AdminGroup *)m_pMemory->GetAddress(cur);
		assert(pGroup->magic == GRP_MAGIC_SET);
		if (pGroup->pCmdGrpTable)
		{
			sm_trie_destroy(pGroup->pCmdGrpTable);
		}
		if (pGroup->pCmdTable)
		{
			sm_trie_destroy(pGroup->pCmdTable);
		}
		cur = pGroup->next_grp;
	}

	m_FirstGroup = INVALID_GROUP_ID;
	m_LastGroup = INVALID_GROUP_ID;

	InvalidateAdminCache(false);

	/* Reset the memory table */
	m_pMemory->Reset();
}

void AdminCache::AddAdminListener(IAdminListener *pListener)
{
	m_hooks.push_back(pListener);
}

void AdminCache::RemoveAdminListener(IAdminListener *pListener)
{
	m_hooks.remove(pListener);
}

void AdminCache::RegisterAuthIdentType(const char *name)
{
	if (sm_trie_retrieve(m_pAuthTables, name, NULL))
	{
		return;
	}

	Trie *pAuth = sm_trie_create();

	AuthMethod method;
	method.name.assign(name);
	method.table = pAuth;

	m_AuthMethods.push_back(method);

	sm_trie_insert(m_pAuthTables, name, pAuth);
}

void AdminCache::InvalidateAdminCache(bool unlink_admins)
{
	/* Wipe the identity cache first */
	List<AuthMethod>::iterator iter;
	for (iter=m_AuthMethods.begin();
		 iter!=m_AuthMethods.end();
		 iter++)
	{
		sm_trie_clear((*iter).table);
	}
	
	while (m_FirstUser != INVALID_ADMIN_ID)
	{
		InvalidateAdmin(m_FirstUser);
	}
}

void AdminCache::DumpAdminCache(int cache_flags, bool rebuild)
{
	if (cache_flags & ADMIN_CACHE_OVERRIDES)
	{
		DumpCommandOverrideCache(Override_Command);
		DumpCommandOverrideCache(Override_CommandGroup);
	}

	/* This will auto-invalidate the admin cache */
	if (cache_flags & ADMIN_CACHE_GROUPS)
	{
		InvalidateGroupCache();
	}

	/* If we only requested an admin rebuild, re-use the internal memory */
	if ((cache_flags & ADMIN_CACHE_ADMINS) &&
		!(cache_flags & ADMIN_CACHE_GROUPS))
	{
		InvalidateAdminCache(true);
	}

	if (rebuild)
	{
		List<IAdminListener *>::iterator iter;
		IAdminListener *pListener;
		cell_t result;
		for (iter=m_hooks.begin(); iter!=m_hooks.end(); iter++)
		{
			pListener = (*iter);
			pListener->OnRebuildAdminCache(cache_flags);
		}
		m_pCacheFwd->PushCell(cache_flags);
		m_pCacheFwd->Execute(&result);
	}
}

const char *AdminCache::GetAdminName(AdminId id)
{
	AdminUser *pUser = (AdminUser *)m_pMemory->GetAddress(id);
	if (!pUser || pUser->magic != USR_MAGIC_SET)
	{
		return NULL;
	}

	return m_pStrings->GetString(pUser->nameidx);
}

bool AdminCache::BindAdminIdentity(AdminId id, const char *auth, const char *ident)
{
	AdminUser *pUser = (AdminUser *)m_pMemory->GetAddress(id);
	if (!pUser || pUser->magic != USR_MAGIC_SET)
	{
		return false;
	}

	Trie *pTable;
	if (!sm_trie_retrieve(m_pAuthTables, auth, (void **)&pTable))
	{
		return false;
	}

	if (sm_trie_retrieve(pTable, ident, NULL))
	{
		return false;
	}

	return sm_trie_insert(pTable, ident, (void **)id);
}

AdminId AdminCache::FindAdminByIdentity(const char *auth, const char *identity)
{
	Trie *pTable;
	if (!sm_trie_retrieve(m_pAuthTables, auth, (void **)&pTable))
	{
		return INVALID_ADMIN_ID;
	}

	void *object;
	if (!sm_trie_retrieve(pTable, identity, &object))
	{
		return INVALID_ADMIN_ID;
	}

	return (AdminId)object;
}

void AdminCache::SetAdminFlag(AdminId id, AdminFlag flag, bool enabled)
{
	AdminUser *pUser = (AdminUser *)m_pMemory->GetAddress(id);
	if (!pUser || pUser->magic != USR_MAGIC_SET)
	{
		return;
	}

	if (flag < Admin_None
		|| flag >= AdminFlags_TOTAL)
	{
		return;
	}

	pUser->flags[enabled] = false;
}

bool AdminCache::GetAdminFlag(AdminId id, AdminFlag flag, AccessMode mode)
{
	AdminUser *pUser = (AdminUser *)m_pMemory->GetAddress(id);
	if (!pUser || pUser->magic != USR_MAGIC_SET)
	{
		return false;
	}

	if (flag < Admin_None
		|| flag >= AdminFlags_TOTAL)
	{
		return false;
	}

	if (mode == Access_Real)
	{
		return pUser->flags[flag];
	} else if (mode == Access_Effective) {
		return (pUser->flags[flag] || pUser->eflags[flag]);
	}

	return false;
}

unsigned int AdminCache::GetAdminFlags(AdminId id, bool flags[], unsigned int total, AccessMode mode)
{
	AdminUser *pUser = (AdminUser *)m_pMemory->GetAddress(id);
	if (!pUser || pUser->magic != USR_MAGIC_SET)
	{
		return 0;
	}

	unsigned int i = 0;
	if (mode == Access_Real)
	{
		for (i=0; i<AdminFlags_TOTAL && i<total; i++)
		{
			flags[i] = pUser->flags[i];
		}
	} else if (mode == Access_Effective) {
		for (i=0; i<AdminFlags_TOTAL && i<total; i++)
		{
			flags[i] = (pUser->flags[i] || pUser->eflags[i]);
		}
	}

	return i;
}

bool AdminCache::AdminInheritGroup(AdminId id, GroupId gid)
{
	AdminUser *pUser = (AdminUser *)m_pMemory->GetAddress(id);
	if (!pUser || pUser->magic != USR_MAGIC_SET)
	{
		return false;
	}

	AdminGroup *pGroup = (AdminGroup *)m_pMemory->GetAddress(gid);
	if (!pGroup || pGroup->magic != USR_MAGIC_SET)
	{
		return false;
	}

	int *table;
	if (pUser->grp_count + 1 > pUser->grp_size)
	{
		int new_size = 0;
		int tblidx;
		
		if (pUser->grp_size == 0)
		{
			new_size = 2;
		} else {
			new_size = pUser->grp_size * 2;
		}
		
		/* Create and refresh pointers */
		tblidx = m_pMemory->CreateMem(new_size * sizeof(int), (void **)&table);
		pUser = (AdminUser *)m_pMemory->GetAddress(id);
		pGroup = (AdminGroup *)m_pMemory->GetAddress(gid);

		/* Copy old data if necessary */
		if (pUser->grp_table != -1)
		{
			int *old_table = (int *)m_pMemory->GetAddress(pUser->grp_table);
			memcpy(table, old_table, sizeof(int) * pUser->grp_count);
		}
		pUser->grp_table = tblidx;
		pUser->grp_size = new_size;
	} else {
		table = (int *)m_pMemory->GetAddress(pUser->grp_table);
	}

	for (unsigned int i=0; i<pUser->grp_count; i++)
	{
		if (table[i] == gid)
		{
			return false;
		}
	}

	table[pUser->grp_count] = gid;
	pUser->grp_count++;

	/* Compute new effective flags */
	for (unsigned int i=Admin_None;
		 i<AdminFlags_TOTAL;
		 i++)
	{
		/* Only inherit flags additive flags we don't have */
		if (!pUser->eflags[i])
		{
			pUser->eflags[i] = pGroup->addflags[i];
		}
	}

	return true;
}

unsigned int AdminCache::GetAdminGroupCount(AdminId id)
{
	AdminUser *pUser = (AdminUser *)m_pMemory->GetAddress(id);
	if (!pUser || pUser->magic != USR_MAGIC_SET)
	{
		return 0;
	}

	return pUser->grp_count;
}

GroupId AdminCache::GetAdminGroup(AdminId id, unsigned int index, const char **name)
{
	AdminUser *pUser = (AdminUser *)m_pMemory->GetAddress(id);
	if (!pUser || pUser->magic != USR_MAGIC_SET || index >= pUser->grp_count)
	{
		return INVALID_GROUP_ID;
	}

	int *table = (int *)m_pMemory->GetAddress(pUser->grp_table);

	return table[index];
}

const char *AdminCache::GetAdminPassword(AdminId id)
{
	AdminUser *pUser = (AdminUser *)m_pMemory->GetAddress(id);
	if (!pUser || pUser->magic != USR_MAGIC_SET)
	{
		return NULL;
	}

	return m_pStrings->GetString(pUser->password);
}

void AdminCache::SetAdminPassword(AdminId id, const char *password)
{	
	AdminUser *pUser = (AdminUser *)m_pMemory->GetAddress(id);
	if (!pUser || pUser->magic != USR_MAGIC_SET)
	{
		return;
	}

	pUser->password = m_pStrings->AddString(password);
}

