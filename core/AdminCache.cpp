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

#include <string.h>
#include <assert.h>
#include "AdminCache.h"
#include "ShareSys.h"
#include "ForwardSys.h"
#include "PlayerManager.h"
#include "ConCmdManager.h"

AdminCache g_Admins;

AdminCache::AdminCache()
{
	m_pCmdOverrides = sm_trie_create();
	m_pCmdGrpOverrides = sm_trie_create();
	m_pStrings = new BaseStringTable(1024);
	m_pMemory = m_pStrings->GetMemTable();
	m_FreeGroupList = m_FirstGroup = m_LastGroup = INVALID_GROUP_ID;
	m_FreeUserList = m_FirstUser = m_LastUser = INVALID_ADMIN_ID;
	m_pGroups = sm_trie_create();
	m_pCacheFwd = NULL;
	m_FirstGroup = -1;
	m_pAuthTables = sm_trie_create();
	m_InvalidatingAdmins = false;
	m_destroying = false;
}

AdminCache::~AdminCache()
{
	m_destroying = true;
	DumpAdminCache(AdminCache_Overrides, false);
	DumpAdminCache(AdminCache_Groups, false);

	sm_trie_destroy(m_pCmdGrpOverrides);
	sm_trie_destroy(m_pCmdOverrides);

	if (m_pGroups)
	{
		sm_trie_destroy(m_pGroups);
	}

	List<AuthMethod>::iterator iter;
	for (iter=m_AuthMethods.begin();
		 iter!=m_AuthMethods.end();
		 iter++)
	{
		sm_trie_destroy((*iter).table);
	}

	sm_trie_destroy(m_pAuthTables);

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

void AdminCache::OnSourceModPluginsLoaded()
{
	DumpAdminCache(AdminCache_Overrides, true);
	DumpAdminCache(AdminCache_Groups, true);
}

void AdminCache::AddCommandOverride(const char *cmd, OverrideType type, FlagBits flags)
{
	Trie *pTrie = NULL;
	if (type == Override_Command)
	{
		pTrie = m_pCmdOverrides;
	} else if (type == Override_CommandGroup) {
		pTrie = m_pCmdGrpOverrides;
	} else {
		return;
	}

	sm_trie_insert(pTrie, cmd, (void *)(unsigned int)flags);

	g_ConCmds.UpdateAdminCmdFlags(cmd, type, flags);
}

bool AdminCache::GetCommandOverride(const char *cmd, OverrideType type, FlagBits *pFlags)
{
	Trie *pTrie = NULL;

	if (type == Override_Command)
	{
		pTrie = m_pCmdOverrides;
	} else if (type == Override_CommandGroup) {
		pTrie = m_pCmdGrpOverrides;
	} else {
		return false;
	}

	void *object;
	if (sm_trie_retrieve(pTrie, cmd, &object))
	{
		if (pFlags)
		{
			*pFlags = (FlagBits)object;
		}
		return true;
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

void AdminCache::_UnsetCommandGroupOverride(const char *group)
{
	if (!m_pCmdGrpOverrides)
	{
		return;
	}

	sm_trie_delete(m_pCmdGrpOverrides, group);

	g_ConCmds.UpdateAdminCmdFlags(group, Override_CommandGroup, 0);
}

void AdminCache::_UnsetCommandOverride(const char *cmd)
{
	if (!m_pCmdOverrides)
	{
		return;
	}

	sm_trie_delete(m_pCmdOverrides, cmd);

	g_ConCmds.UpdateAdminCmdFlags(cmd, Override_Command, 0);
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
	}

	pUser->flags = 0;
	pUser->eflags = 0;
	pUser->grp_count = 0;
	pUser->password = -1;
	pUser->magic = USR_MAGIC_SET;
	pUser->auth.identidx = -1;
	pUser->auth.index = 0;
	pUser->immune_default = false;
	pUser->immune_global = false;

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
	pGroup->addflags = 0;

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

	if (flag < Admin_Reservation || flag >= AdminFlags_TOTAL)
	{
		return;
	}

	FlagBits bits = (1<<(FlagBits)flag);

	if (enabled)
	{
		pGroup->addflags |= bits;
	} else {
		pGroup->addflags &= ~bits;
	}
}

bool AdminCache::GetGroupAddFlag(GroupId id, AdminFlag flag)
{
	AdminGroup *pGroup = (AdminGroup *)m_pMemory->GetAddress(id);
	if (!pGroup || pGroup->magic != GRP_MAGIC_SET)
	{
		return false;
	}

	if (flag < Admin_Reservation || flag >= AdminFlags_TOTAL)
	{
		return false;
	}

	FlagBits bit = 1<<(FlagBits)flag;
	return ((pGroup->addflags & bit) == bit);
}

FlagBits AdminCache::GetGroupAddFlags(GroupId id)
{
	AdminGroup *pGroup = (AdminGroup *)m_pMemory->GetAddress(id);
	if (!pGroup || pGroup->magic != GRP_MAGIC_SET)
	{
		return 0;
	}

	return pGroup->addflags;
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
		/* Break out if this group is already in the list */
		for (int i=0; i<old_table[0]; i++)
		{
			if (old_table[1+i] == other_id)
			{
				return;
			}
		}
		tblidx = m_pMemory->CreateMem(sizeof(int) * (old_table[0] + 2), (void **)&table);
		/* Get the old address again in case of resize */
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

	if (!m_InvalidatingAdmins && !m_destroying)
	{
		g_Players.ClearAdminId(id);
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
	if (pUser->auth.identidx != -1)
	{
		Trie *pTrie = GetMethodByIndex(pUser->auth.index);
		if (pTrie)
		{
			sm_trie_delete(pTrie, m_pStrings->GetString(pUser->auth.identidx));
		}
	}

	/* Clear table counts */
	pUser->grp_count = 0;
	
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
					pUser->eflags = pUser->flags;
					for (unsigned int j=0; j<pUser->grp_count; j++)
					{
						pOther = (AdminGroup *)m_pMemory->GetAddress(table[j]);
						pUser->eflags |= pOther->addflags;
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
	m_InvalidatingAdmins = true;
	if (!m_destroying)
	{
		g_Players.ClearAllAdmins();
	}
	/* Wipe the identity cache first */
	List<AuthMethod>::iterator iter;
	for (iter=m_AuthMethods.begin();
		 iter!=m_AuthMethods.end();
		 iter++)
	{
		sm_trie_clear((*iter).table);
	}
	
	if (unlink_admins)
	{
		while (m_FirstUser != INVALID_ADMIN_ID)
		{
			InvalidateAdmin(m_FirstUser);
		}
	} else {
		m_FirstUser = -1;
		m_LastUser = -1;
		m_FreeUserList = -1;
	}
	m_InvalidatingAdmins = false;
}

void AdminCache::DumpAdminCache(AdminCachePart part, bool rebuild)
{
	List<IAdminListener *>::iterator iter;
	IAdminListener *pListener;
	cell_t result;

	if (part == AdminCache_Overrides)
	{
		DumpCommandOverrideCache(Override_Command);
		DumpCommandOverrideCache(Override_CommandGroup);
		if (rebuild && !m_destroying)
		{
			for (iter=m_hooks.begin(); iter!=m_hooks.end(); iter++)
			{
				pListener = (*iter);
				pListener->OnRebuildOverrideCache();
			}
			m_pCacheFwd->PushCell(part);
			m_pCacheFwd->Execute(&result);
		}
	} else if (part == AdminCache_Groups || part == AdminCache_Admins) {
		if (part == AdminCache_Groups)
		{
			InvalidateGroupCache();
			if (rebuild && !m_destroying)
			{
				for (iter=m_hooks.begin(); iter!=m_hooks.end(); iter++)
				{
					pListener = (*iter);
					pListener->OnRebuildGroupCache();
				}
				m_pCacheFwd->PushCell(part);
				m_pCacheFwd->Execute(&result);
			}
		}
		InvalidateAdminCache(true);
		if (rebuild && !m_destroying)
		{
			for (iter=m_hooks.begin(); iter!=m_hooks.end(); iter++)
			{
				pListener = (*iter);
				pListener->OnRebuildAdminCache((part == AdminCache_Groups));
			}
			m_pCacheFwd->PushCell(AdminCache_Admins);
			m_pCacheFwd->Execute(&result);
		}
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

bool AdminCache::GetMethodIndex(const char *name, unsigned int *_index)
{
	List<AuthMethod>::iterator iter;
	unsigned int index = 0;
	for (iter=m_AuthMethods.begin();
		 iter!=m_AuthMethods.end();
		 iter++,index++)
	{
		if ((*iter).name.compare(name) == 0)
		{
			*_index = index;
			return true;
		}
	}

	return false;
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

	pUser->auth.identidx = m_pStrings->AddString(ident);
	GetMethodIndex(auth, &pUser->auth.index);

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

	if (flag < Admin_Reservation
		|| flag >= AdminFlags_TOTAL)
	{
		return;
	}
	
	FlagBits bits = (1<<(FlagBits)flag);

	if (enabled)
	{
		pUser->flags |= bits;
		pUser->eflags |= bits;
	} else {
		pUser->flags &= ~bits;
		pUser->eflags &= ~bits;
	}
}

bool AdminCache::GetAdminFlag(AdminId id, AdminFlag flag, AccessMode mode)
{
	AdminUser *pUser = (AdminUser *)m_pMemory->GetAddress(id);
	if (!pUser || pUser->magic != USR_MAGIC_SET)
	{
		return false;
	}

	if (flag < Admin_Reservation
		|| flag >= AdminFlags_TOTAL)
	{
		return false;
	}

	FlagBits bit = (1<<(FlagBits)flag);

	if (mode == Access_Real)
	{
		return ((pUser->flags & bit) == bit);
	} else if (mode == Access_Effective) {
		bool has_bit = ((pUser->eflags & bit) == bit);
		if (!has_bit && flag != Admin_Root && ((pUser->eflags & ADMFLAG_ROOT) == ADMFLAG_ROOT))
		{
			has_bit = true;
		}
		return has_bit;
	}

	return false;
}

FlagBits AdminCache::GetAdminFlags(AdminId id, AccessMode mode)
{
	AdminUser *pUser = (AdminUser *)m_pMemory->GetAddress(id);
	if (!pUser || pUser->magic != USR_MAGIC_SET)
	{
		return 0;
	}

	if (mode == Access_Real)
	{
		return pUser->flags;
	} else if (mode == Access_Effective) {
		return pUser->eflags;
	}

	return 0;
}

void AdminCache::SetAdminFlags(AdminId id, AccessMode mode, FlagBits bits)
{
	AdminUser *pUser = (AdminUser *)m_pMemory->GetAddress(id);
	if (!pUser || pUser->magic != USR_MAGIC_SET)
	{
		return;
	}

	if (mode == Access_Real)
	{
		pUser->flags = bits;
		pUser->eflags = bits;
	} else if (mode == Access_Effective) {
		pUser->eflags = bits;
	}
}

bool AdminCache::AdminInheritGroup(AdminId id, GroupId gid)
{
	AdminUser *pUser = (AdminUser *)m_pMemory->GetAddress(id);
	if (!pUser || pUser->magic != USR_MAGIC_SET)
	{
		return false;
	}

	AdminGroup *pGroup = (AdminGroup *)m_pMemory->GetAddress(gid);
	if (!pGroup || pGroup->magic != GRP_MAGIC_SET)
	{
		return false;
	}

	/* First check for duplicates */
	if (pUser->grp_count != 0)
	{
		int *temp_table = (int *)m_pMemory->GetAddress(pUser->grp_table);
		for (unsigned int i=0; i<pUser->grp_count; i++)
		{
			if (temp_table[i] == gid)
			{
				return false;
			}
		}
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

	table[pUser->grp_count] = gid;
	pUser->grp_count++;

	/* Compute new effective permissions */
	pUser->eflags |= pGroup->addflags;

	if (pGroup->immune_default)
	{
		pUser->immune_default = true;
	}
	if (pGroup->immune_global)
	{
		pUser->immune_global = true;
	}

	return true;
}

bool AdminCache::IsValidAdmin(AdminId id)
{
	AdminUser *pUser = (AdminUser *)m_pMemory->GetAddress(id);
	return (pUser != NULL && pUser->magic == USR_MAGIC_SET);
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

unsigned int AdminCache::FlagBitsToBitArray(FlagBits bits, bool array[], unsigned int maxSize)
{
	unsigned int i;
	for (i=0; i<maxSize && i<AdminFlags_TOTAL; i++)
	{
		array[i] = ((bits & (1<<i)) == (unsigned)(1<<i));
	}

	return i;
}

FlagBits AdminCache::FlagBitArrayToBits(const bool array[], unsigned int maxSize)
{
	FlagBits bits = 0;
	for (unsigned int i=0; i<maxSize && i<AdminFlags_TOTAL; i++)
	{
		if (array[i])
		{
			bits |= (1<<i);
		}
	}

	return bits;
}

FlagBits AdminCache::FlagArrayToBits(const AdminFlag array[], unsigned int numFlags)
{
	FlagBits bits = 0;
	for (unsigned int i=0; i<numFlags && i<AdminFlags_TOTAL; i++)
	{
		bits |= (1 << (FlagBits)array[i]);
	}

	return bits;
}

unsigned int AdminCache::FlagBitsToArray(FlagBits bits, AdminFlag array[], unsigned int maxSize)
{
	unsigned int i, num=0;
	for (i=0; i<maxSize && i<AdminFlags_TOTAL; i++)
	{
		if ((bits & (1<<i)) == (unsigned)(1<<i))
		{
			array[num++] = (AdminFlag)i;
		}
	}

	return num;
}

bool AdminCache::CheckAdminFlags(AdminId id, FlagBits bits)
{
	AdminUser *pUser = (AdminUser *)m_pMemory->GetAddress(id);
	if (!pUser || pUser->magic != USR_MAGIC_SET)
	{
		return false;
	}

	return ((pUser->eflags & bits) == bits);
}

bool AdminCache::CanAdminTarget(AdminId id, AdminId target)
{
	/** 
	 * Zeroth, if the targeting AdminId is INVALID_ADMIN_ID, targeting fails.
	 * First, if the targeted AdminId is INVALID_ADMIN_ID, targeting succeeds.
	 */

	if (id == INVALID_ADMIN_ID)
	{
		return false;
	}

	if (target == INVALID_ADMIN_ID)
	{
		return true;
	}

	AdminUser *pUser = (AdminUser *)m_pMemory->GetAddress(id);
	if (!pUser || pUser->magic != USR_MAGIC_SET)
	{
		return false;
	}

	AdminUser *pTarget = (AdminUser *)m_pMemory->GetAddress(target);
	if (!pTarget || pTarget->magic != USR_MAGIC_SET)
	{
		return false;
	}

	/** 
	 * Second, if the targeting admin is root, targeting succeeds.
	 */
	if (pUser->eflags & ADMFLAG_ROOT)
	{
		return true;
	}

	/** Fourth, if the targeted admin has global immunity, targeting fails. */
	if (pTarget->immune_global)
	{
		return false;
	}

	/** 
	 * Fifth, if the targeted admin has default immunity 
	 *  and the admin belongs to no groups, targeting fails.
	 */
	if (pTarget->immune_default && pUser->grp_count < 1)
	{
		return false;
	}

	/** 
	 * Sixth, if the targeted admin has specific immunity from the
	 *  targeting admin via group immunities, targeting fails.
	 */
	//:TODO: speed this up... maybe with trie hacks.
	//idea is to insert %d.%d in the trie after computing this and use it as a cache lookup.
	//problem is the trie cannot delete prefixes, so we'd have a problem with invalidations.
	if (pTarget->grp_count > 0 && pUser->grp_count > 0)
	{
		int *grp_table = (int *)m_pMemory->GetAddress(pTarget->grp_table);
		int *src_table = (int *)m_pMemory->GetAddress(pUser->grp_table);
		GroupId id, other;
		unsigned int num;
		for (unsigned int i=0; i<pTarget->grp_count; i++)
		{
			id = grp_table[i];
			num = GetGroupImmunityCount(id);
			for (unsigned int j=0; j<num; i++)
			{
				other = GetGroupImmunity(id, j);
				for (unsigned int k=0; k<pUser->grp_count; k++)
				{
					if (other == src_table[k])
					{
						return false;
					}
				}
			}
		}
	}
	
	return true;
}
