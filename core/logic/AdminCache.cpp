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

#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <ITextParsers.h>
#include <ISourceMod.h>
#include <IGameHelpers.h>
#include <IPlayerHelpers.h>
#include "AdminCache.h"
#include "Translator.h"
#include "common_logic.h"
#include "stringutil.h"
#include <bridge/include/ILogger.h>
#include <bridge/include/CoreProvider.h>
#include <bridge/include/IVEngineServerBridge.h>

#define LEVEL_STATE_NONE		0
#define LEVEL_STATE_LEVELS		1
#define LEVEL_STATE_FLAGS		2

AdminCache g_Admins;
char g_ReverseFlags[AdminFlags_TOTAL];
AdminFlag g_FlagLetters[26];
bool g_FlagCharSet[26];

/* Default flags */
AdminFlag g_DefaultFlags[26] = 
{
	Admin_Reservation, Admin_Generic, Admin_Kick, Admin_Ban, Admin_Unban,
	Admin_Slay, Admin_Changemap, Admin_Convars, Admin_Config, Admin_Chat,
	Admin_Vote, Admin_Password, Admin_RCON, Admin_Cheats, Admin_Custom1, 
	Admin_Custom2, Admin_Custom3, Admin_Custom4, Admin_Custom5, Admin_Custom6,
	Admin_Generic, Admin_Generic, Admin_Generic, Admin_Generic, Admin_Generic,
	Admin_Root
};


class FlagReader : public ITextListener_SMC
{
public:
	void LoadLevels()
	{
		if (!Parse())
		{
			memcpy(g_FlagLetters, g_DefaultFlags, sizeof(AdminFlag) * 26);
			for (unsigned int i=0; i<20; i++)
			{
				g_FlagCharSet[i] = true;
			}
			g_FlagCharSet[25] = true;
		}
	}
private:
	bool Parse()
	{
		SMCStates states;
		SMCError error;

		m_bFileNameLogged = false;
		g_pSM->BuildPath(Path_SM, m_File, sizeof(m_File), "configs/admin_levels.cfg");

		if ((error = textparsers->ParseFile_SMC(m_File, this, &states))
			!= SMCError_Okay)
		{
			const char *err_string = textparsers->GetSMCErrorString(error);
			if (!err_string)
			{
				err_string = "Unknown error";
			}
			ParseError(NULL, "Error %d (%s)", error, err_string);
			return false;
		}

		return true;
	}
	void ReadSMC_ParseStart()
	{
		m_LevelState = LEVEL_STATE_NONE;
		m_IgnoreLevel = 0;
		memset(g_FlagCharSet, 0, sizeof(g_FlagCharSet));
	}
	SMCResult ReadSMC_NewSection(const SMCStates *states, const char *name)
	{
		if (m_IgnoreLevel)
		{
			m_IgnoreLevel++;
			return SMCResult_Continue;
		}

		if (m_LevelState == LEVEL_STATE_NONE)
		{
			if (strcmp(name, "Levels") == 0)
			{
				m_LevelState = LEVEL_STATE_LEVELS;
			} 
			else 
			{
				m_IgnoreLevel++;
			}
		} else if (m_LevelState == LEVEL_STATE_LEVELS) {
			if (strcmp(name, "Flags") == 0)
			{
				m_LevelState = LEVEL_STATE_FLAGS;
			} 
			else 
			{
				m_IgnoreLevel++;
			}
		} 
		else 
		{
			m_IgnoreLevel++;
		}

		return SMCResult_Continue;
	}
	SMCResult ReadSMC_KeyValue(const SMCStates *states, const char *key, const char *value)
	{
		if (m_LevelState != LEVEL_STATE_FLAGS || m_IgnoreLevel)
		{
			return SMCResult_Continue;
		}

		unsigned char c = (unsigned)value[0];

		if (c < (unsigned)'a' || c > (unsigned)'z')
		{
			ParseError(states, "Flag \"%c\" is not a lower-case ASCII letter", c);
			return SMCResult_Continue;
		}

		c -= (unsigned)'a';

		if (!g_Admins.FindFlag(key, &g_FlagLetters[c]))
		{
			ParseError(states, "Unrecognized admin level \"%s\"", key);
			return SMCResult_Continue;
		}

		g_FlagCharSet[c] = true;

		return SMCResult_Continue;
	}
	SMCResult ReadSMC_LeavingSection(const SMCStates *states)
	{
		if (m_IgnoreLevel)
		{
			m_IgnoreLevel--;
			return SMCResult_Continue;
		}

		if (m_LevelState == LEVEL_STATE_FLAGS)
		{
			m_LevelState = LEVEL_STATE_LEVELS;
			return SMCResult_Halt;
		} 
		else if (m_LevelState == LEVEL_STATE_LEVELS) 
		{
			m_LevelState = LEVEL_STATE_NONE;
		}

		return SMCResult_Continue;
	}
	void ParseError(const SMCStates *states, const char *message, ...)
	{
		va_list ap;
		char buffer[256];

		va_start(ap, message);
		g_pSM->FormatArgs(buffer, sizeof(buffer), message, ap);
		va_end(ap);

		if (!m_bFileNameLogged)
		{
			logger->LogError("[SM] Parse error(s) detected in file \"%s\":", m_File);
			m_bFileNameLogged = true;
		}

		logger->LogError("[SM] (Line %d): %s", states ? states->line : 0, buffer);
	}
private:
	bool m_bFileNameLogged;
	char m_File[PLATFORM_MAX_PATH];
	int m_LevelState;
	int m_IgnoreLevel;
} s_FlagReader;

AdminCache::AdminCache()
{
	m_pStrings = new BaseStringTable(1024);
	m_pMemory = m_pStrings->GetMemTable();
	m_FreeGroupList = m_FirstGroup = m_LastGroup = INVALID_GROUP_ID;
	m_FreeUserList = m_FirstUser = m_LastUser = INVALID_ADMIN_ID;
	m_pCacheFwd = NULL;
	m_FirstGroup = -1;
	m_InvalidatingAdmins = false;
	m_destroying = false;
}

AdminCache::~AdminCache()
{
	m_destroying = true;
	DumpAdminCache(AdminCache_Overrides, false);
	DumpAdminCache(AdminCache_Groups, false);

	List<AuthMethod *>::iterator iter;
	for (iter=m_AuthMethods.begin();
		 iter!=m_AuthMethods.end();
		 iter++)
	{
		delete *iter;
	}

	delete m_pStrings;
}

void AdminCache::OnSourceModStartup(bool late)
{
	RegisterAuthIdentType("steam");
	RegisterAuthIdentType("name");
	RegisterAuthIdentType("ip");

	NameFlag("reservation", Admin_Reservation);
	NameFlag("kick", Admin_Kick);
	NameFlag("generic", Admin_Generic);
	NameFlag("ban", Admin_Ban);
	NameFlag("unban", Admin_Unban);
	NameFlag("slay", Admin_Slay);
	NameFlag("changemap", Admin_Changemap);
	NameFlag("cvars", Admin_Convars);
	NameFlag("config", Admin_Config);
	NameFlag("chat", Admin_Chat);
	NameFlag("vote", Admin_Vote);
	NameFlag("password", Admin_Password);
	NameFlag("rcon", Admin_RCON);
	NameFlag("cheats", Admin_Cheats);
	NameFlag("root", Admin_Root);
	NameFlag("custom1", Admin_Custom1);
	NameFlag("custom2", Admin_Custom2);
	NameFlag("custom3", Admin_Custom3);
	NameFlag("custom4", Admin_Custom4);
	NameFlag("custom5", Admin_Custom5);
	NameFlag("custom6", Admin_Custom6);

	auto sm_dump_admcache = [this] (int client, const ICommandArgs *args) -> bool {
		char buffer[PLATFORM_MAX_PATH];
		g_pSM->BuildPath(Path_SM, buffer, sizeof(buffer), "data/admin_cache_dump.txt");

		if (!DumpCache(buffer)) {
			bridge->ConsolePrint("Could not open file for writing: %s", buffer);
			return true;
		}

		bridge->ConsolePrint("Admin cache dumped to: %s", buffer);
		return true;
	};

	bridge->DefineCommand("sm_dump_admcache", "Dumps the admin cache for debugging", sm_dump_admcache);
}

void AdminCache::OnSourceModAllInitialized()
{
	m_pCacheFwd = forwardsys->CreateForward("OnRebuildAdminCache", ET_Ignore, 1, NULL, Param_Cell);
	sharesys->AddInterface(NULL, this);
}

void AdminCache::OnSourceModLevelChange(const char *mapName)
{
	int i;
	AdminFlag flag;

	/* For now, we only read these once per level. */
	s_FlagReader.LoadLevels();

	memset(g_ReverseFlags, '?', sizeof(g_ReverseFlags));

	for (i = 0; i < 26; i++)
	{
		if (FindFlag('a' + i, &flag))
		{
			g_ReverseFlags[flag] = 'a' + i;
		}
	}
}

void AdminCache::OnSourceModShutdown()
{
	forwardsys->ReleaseForward(m_pCacheFwd);
	m_pCacheFwd = NULL;
}

void AdminCache::OnSourceModPluginsLoaded()
{
	DumpAdminCache(AdminCache_Overrides, true);
	DumpAdminCache(AdminCache_Groups, true);
}

void AdminCache::NameFlag(const char *str, AdminFlag flag)
{
	m_LevelNames.insert(str, flag);
}

bool AdminCache::FindFlag(const char *str, AdminFlag *pFlag)
{
	return m_LevelNames.retrieve(str, pFlag);
}

void AdminCache::AddCommandOverride(const char *cmd, OverrideType type, FlagBits flags)
{
	FlagMap *map;
	if (type == Override_Command)
		map = &m_CmdOverrides;
	else if (type == Override_CommandGroup)
		map = &m_CmdGrpOverrides;
	else
		return;

	map->insert(cmd, flags);
	bridge->UpdateAdminCmdFlags(cmd, type, flags, false);
}

bool AdminCache::GetCommandOverride(const char *cmd, OverrideType type, FlagBits *pFlags)
{
	FlagMap *map;
	if (type == Override_Command)
		map = &m_CmdOverrides;
	else if (type == Override_CommandGroup)
		map = &m_CmdGrpOverrides;
	else
		return false;

	return map->retrieve(cmd, pFlags);
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
	m_CmdGrpOverrides.remove(group);
	bridge->UpdateAdminCmdFlags(group, Override_CommandGroup, 0, true);
}

void AdminCache::_UnsetCommandOverride(const char *cmd)
{
	m_CmdOverrides.remove(cmd);
	bridge->UpdateAdminCmdFlags(cmd, Override_Command, 0, true);
}

void AdminCache::DumpCommandOverrideCache(OverrideType type)
{
	if (type == Override_Command)
		m_CmdOverrides.clear();
	else if (type == Override_CommandGroup)
		m_CmdGrpOverrides.clear();
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
	}
	else
	{
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
	pUser->immunity_level = 0;
	pUser->serialchange = 1;

	if (m_FirstUser == INVALID_ADMIN_ID)
	{
		m_FirstUser = id;
		m_LastUser = id;
	}
	else
	{
		AdminUser *pPrev = (AdminUser *)m_pMemory->GetAddress(m_LastUser);
		pPrev->next_user = id;
		pUser->prev_user = m_LastUser;
		m_LastUser = id;
	}

	/* Since we always append to the tail, we should invalidate their next */
	pUser->next_user = -1;

	if (name && name[0] != '\0')
	{
		int nameidx = m_pStrings->AddString(name);
		pUser = (AdminUser *)m_pMemory->GetAddress(id);
		pUser->nameidx = nameidx;
	}
	else
	{
		pUser->nameidx = -1;
	}

	return id;
}

GroupId AdminCache::AddGroup(const char *group_name)
{
	if (m_Groups.contains(group_name))
		return INVALID_GROUP_ID;

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

	pGroup->immunity_level = 0;
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

	int nameidx = m_pStrings->AddString(group_name);
	pGroup = (AdminGroup *)m_pMemory->GetAddress(id);
	pGroup->nameidx = nameidx;

	m_Groups.insert(group_name, id);
	return id;
}

GroupId AdminCache::FindGroupByName(const char *group_name)
{
	GroupId id;
	if (!m_Groups.retrieve(group_name, &id))
		return INVALID_GROUP_ID;

	AdminGroup *pGroup = (AdminGroup *)m_pMemory->GetAddress(id);
	if (!pGroup || pGroup->magic != GRP_MAGIC_SET)
		return INVALID_GROUP_ID;

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

const char *AdminCache::GetGroupName(GroupId gid)
{
	AdminGroup *pGroup = (AdminGroup *)m_pMemory->GetAddress(gid);
	if (!pGroup || pGroup->magic != GRP_MAGIC_SET)
	{
		return NULL;
	}

	return m_pStrings->GetString(pGroup->nameidx);
}

void AdminCache::SetGroupGenericImmunity(GroupId id, ImmunityType type, bool enabled)
{
	AdminGroup *pGroup = (AdminGroup *)m_pMemory->GetAddress(id);
	if (!pGroup || pGroup->magic != GRP_MAGIC_SET)
	{
		return;
	}

	unsigned int level = 0;

	if (enabled)
	{
		if (type == Immunity_Default)
		{
			level = 1;
		} else if (type == Immunity_Global) {
			level = 2;
		}
		if (level > pGroup->immunity_level)
		{
			pGroup->immunity_level = level;
		}
	} else {
		pGroup->immunity_level = 0;
	}
}

bool AdminCache::GetGroupGenericImmunity(GroupId id, ImmunityType type)
{
	AdminGroup *pGroup = (AdminGroup *)m_pMemory->GetAddress(id);
	if (!pGroup || pGroup->magic != GRP_MAGIC_SET)
	{
		return false;
	}

	if (type == Immunity_Default && pGroup->immunity_level >= 1)
	{
		return true;
	} else if (type == Immunity_Global && pGroup->immunity_level >= 2) {
		return true;
	}

	return false;
}

void AdminCache::AddGroupImmunity(GroupId id, GroupId other_id)
{
	AdminGroup *pGroup = (AdminGroup *)m_pMemory->GetAddress(other_id);
	if (!pGroup || pGroup->magic != GRP_MAGIC_SET)
	{
		return;
	}

	pGroup = (AdminGroup *)m_pMemory->GetAddress(id);
	if (!pGroup || pGroup->magic != GRP_MAGIC_SET)
	{
		return;
	}

	/* We always need to resize the immunity table */
	int *table, tblidx;
	if (pGroup->immune_table == -1)
	{
		tblidx = m_pMemory->CreateMem(sizeof(int) * 2, (void **)&table);
		pGroup = (AdminGroup *)m_pMemory->GetAddress(id);
		table[0] = 0;
	} else {
		int *old_table = (int *)m_pMemory->GetAddress(pGroup->immune_table);
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
		pGroup = (AdminGroup *)m_pMemory->GetAddress(id);
		old_table = (int *)m_pMemory->GetAddress(pGroup->immune_table);
		table[0] = old_table[0];
		for (unsigned int i=1; i<=(unsigned int)old_table[0]; i++)
		{
			table[i] = old_table[i];
		}
	}

	/* Assign */
	pGroup->immune_table = tblidx;

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

	OverrideMap *map;
	if (type == Override_Command)
	{
		if (pGroup->pCmdTable == NULL)
			pGroup->pCmdTable = new OverrideMap();
		map = pGroup->pCmdTable;
	} else if (type == Override_CommandGroup) {
		if (pGroup->pCmdGrpTable == NULL)
			pGroup->pCmdGrpTable = new OverrideMap();
		map = pGroup->pCmdGrpTable;
	} else {
		return;
	}

	map->insert(name, rule);
}

bool AdminCache::GetGroupCommandOverride(GroupId id, const char *name, OverrideType type, OverrideRule *pRule)
{
	AdminGroup *pGroup = (AdminGroup *)m_pMemory->GetAddress(id);
	if (!pGroup || pGroup->magic != GRP_MAGIC_SET)
	{
		return false;
	}

	OverrideMap *map;
	if (type == Override_Command)
	{
		if (pGroup->pCmdTable == NULL)
			return false;
		map = pGroup->pCmdTable;
	} else if (type == Override_CommandGroup) {
		if (pGroup->pCmdGrpTable == NULL)
			return false;
		map = pGroup->pCmdGrpTable;
	} else {
		return false;
	}

	return map->retrieve(name, pRule);
}

AuthMethod *AdminCache::GetMethodByIndex(unsigned int index)
{
	List<AuthMethod *>::iterator iter;
	for (iter=m_AuthMethods.begin();
		 iter!=m_AuthMethods.end();
		 iter++)
	{
		if (index-- == 0)
		{
			return *iter;
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
		playerhelpers->ClearAdminId(id);
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
		AuthMethod *method = GetMethodByIndex(pUser->auth.index);
		if (method)
			method->identities.remove(m_pStrings->GetString(pUser->auth.identidx));
	}

	/* Clear table counts */
	pUser->grp_count = 0;
	
	/* Link into free list */
	pUser->magic = USR_MAGIC_UNSET;
	pUser->next_user = m_FreeUserList;
	m_FreeUserList = id;

	/* Unset serial change */
	pUser->serialchange = 0;

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
	m_Groups.remove(str);

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

	/* Free any used memory */
	delete pGroup->pCmdGrpTable;
	pGroup->pCmdGrpTable = NULL;
	delete pGroup->pCmdTable;
	pGroup->pCmdTable = NULL;

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
					/* Mark as changed */
					pUser->serialchange++;
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
	m_Groups.clear();

	/* Free memory on groups */
	GroupId cur = m_FirstGroup;
	AdminGroup *pGroup;
	while (cur != INVALID_GROUP_ID)
	{
		pGroup = (AdminGroup *)m_pMemory->GetAddress(cur);
		assert(pGroup->magic == GRP_MAGIC_SET);
		delete pGroup->pCmdGrpTable;
		delete pGroup->pCmdTable;
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
	if (m_AuthTables.contains(name))
		return;

	AuthMethod *method = new AuthMethod(name);
	m_AuthMethods.push_back(method);
	m_AuthTables.insert(name, method);
}

void AdminCache::InvalidateAdminCache(bool unlink_admins)
{
	m_InvalidatingAdmins = true;
	if (!m_destroying)
	{
		int maxClients = playerhelpers->GetMaxClients();
		for (int i = 1; i <= maxClients; ++i)
		{
			playerhelpers->GetGamePlayer(i)->ClearAdmin();
		}
	}
	/* Wipe the identity cache first */
	List<AuthMethod *>::iterator iter;
	for (iter=m_AuthMethods.begin();
		 iter!=m_AuthMethods.end();
		 iter++)
	{
		(*iter)->identities.clear();
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
			m_pCacheFwd->Execute();
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
				m_pCacheFwd->Execute();
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
			m_pCacheFwd->Execute();
			playerhelpers->RecheckAnyAdmins();
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
	List<AuthMethod *>::iterator iter;
	unsigned int index = 0;
	for (iter=m_AuthMethods.begin();
		 iter!=m_AuthMethods.end();
		 iter++,index++)
	{
		if ((*iter)->name.compare(name) == 0)
		{
			*_index = index;
			return true;
		}
	}

	return false;
}

/*
 * Converts Steam2 id, Steam3 id, or SteamId64 to unified, legacy
 * admin identity format. (account id part of Steam2 format)
 */
bool AdminCache::GetUnifiedSteamIdentity(const char *ident, char *out, size_t maxlen)
{
	int len = strlen(ident);
	if (!strcmp(ident, "BOT"))
	{
		// Bots
		strncopy(out, ident, maxlen);
		return true;
	}
	else if (len >= 11 && !strncmp(ident, "STEAM_", 6) && ident[8] != '_')
	{
		// non-bot/lan Steam2 Id, strip off the STEAM_* part
		ke::SafeStrcpy(out, maxlen, &ident[8]);
		return true;
	}
	else if (len >= 7 && !strncmp(ident, "[U:", 3) && ident[len-1] == ']')
	{
		// Steam3 Id, replicate the Steam2 Post-"STEAM_" part
		uint32_t accountId = strtoul(&ident[5], nullptr, 10);
		ke::SafeSprintf(out, maxlen, "%u:%u", accountId & 1, accountId >> 1);
		return true;
	}
	else
	{
		// 64-bit CSteamID, replicate the Steam2 Post-"STEAM_" part

		// some constants from steamclientpublic.h
		static const uint32_t k_EAccountTypeIndividual = 1;
		static const int k_EUniverseInvalid = 0;
		static const int k_EUniverseMax = 5;
		static const unsigned int k_unSteamUserWebInstance	= 4;
		
		uint64_t steamId = strtoull(ident, nullptr, 10);
		if (steamId > 0)
		{
			// Make some attempt at being sure it's a valid id rather than other number,
			// even though we're only going to use the lower 32 bits.
			uint32_t accountId = steamId & 0xFFFFFFFF;
			uint32_t accountType = (steamId >> 52) & 0xF;
			int universe = steamId >> 56;
			uint32_t accountInstance = (steamId >> 32) & 0xFFFFF;
			if (accountId > 0
				&& universe > k_EUniverseInvalid && universe < k_EUniverseMax
				&& accountType == k_EAccountTypeIndividual && accountInstance <= k_unSteamUserWebInstance
				)
			{
				ke::SafeSprintf(out, maxlen, "%u:%u", accountId & 1, accountId >> 1);
				return true;
			}
		}
	}
	
	return false;
}

bool AdminCache::BindAdminIdentity(AdminId id, const char *auth, const char *ident)
{
	if (ident[0] == '\0')
	{
		return false;
	}

	AdminUser *pUser = (AdminUser *)m_pMemory->GetAddress(id);
	if (!pUser || pUser->magic != USR_MAGIC_SET)
	{
		return false;
	}

	AuthMethod *method;
	if (!m_AuthTables.retrieve(auth, &method))
		return false;

	/* If the auth type is steam, the id could be in a number of formats. Unify it. */
	char steamIdent[16];
	if (strcmp(auth, "steam") == 0)
	{
		if (!GetUnifiedSteamIdentity(ident, steamIdent, sizeof(steamIdent)))
			return false;
		
		ident = steamIdent;
	}

	if (method->identities.contains(ident))
		return false;

	int i_ident = m_pStrings->AddString(ident);

	pUser = (AdminUser *)m_pMemory->GetAddress(id);
	pUser->auth.identidx = i_ident;
	GetMethodIndex(auth, &pUser->auth.index);

	return method->identities.insert(ident, id);
}

AdminId AdminCache::FindAdminByIdentity(const char *auth, const char *identity)
{
	AuthMethod *method;
	if (!m_AuthTables.retrieve(auth, &method))
		return INVALID_ADMIN_ID;

	/* If the auth type is steam, the id could be in a number of formats. Unify it. */
	char steamIdent[16];
	if (strcmp(auth, "steam") == 0)
	{
		if (!GetUnifiedSteamIdentity(identity, steamIdent, sizeof(steamIdent)))
			return INVALID_ADMIN_ID;
		
		identity = steamIdent;
	}

	AdminId id;
	if (!method->identities.retrieve(identity, &id))
		return INVALID_ADMIN_ID;
	return id;
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

	pUser->serialchange++;
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

	pUser->serialchange++;
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

	if (pGroup->immunity_level > pUser->immunity_level)
	{
		pUser->immunity_level = pGroup->immunity_level;
	}

	pUser->serialchange++;

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

	GroupId gid = table[index];

	if (name)
	{
		*name = GetGroupName(gid);
	}

	return gid;
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

	if (password[0] == '\0')
	{
		pUser->password = -1;
		return;
	}

	int i_password = m_pStrings->AddString(password);
	pUser = (AdminUser *)m_pMemory->GetAddress(id);
	pUser->password = i_password;
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
	for (i=0; num<maxSize && i<AdminFlags_TOTAL; i++)
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

	if (id == target)
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

	/** Fourth, if the targeted admin is immune from targeting admin. */
	int mode = bridge->GetImmunityMode();
	switch (mode)
	{
		case 1:
		{
			if (pTarget->immunity_level > pUser->immunity_level)
			{
				return false;
			}
			break;
		}
		case 3:
		{
			/* If neither has any immunity, let this pass. */
			if (!pUser->immunity_level && !pTarget->immunity_level)
			{
				return true;
			}
			/* Don't break, go to the next case. */
		}
		case 2:
		{
			if (pTarget->immunity_level >= pUser->immunity_level)
			{
				return false;
			}
			break;
		}
	}

	/** 
	 * Fifth, if the targeted admin has specific immunity from the
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
			for (unsigned int j=0; j<num; j++)
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

bool AdminCache::FindFlag(char c, AdminFlag *pAdmFlag)
{
	if (c < 'a' 
		|| c > 'z'
		|| !g_FlagCharSet[(unsigned)c - (unsigned)'a'])
	{
		return false;
	}

	if (pAdmFlag)
	{
		*pAdmFlag = g_FlagLetters[(unsigned)c - (unsigned)'a'];
	}

	return true;
}

bool AdminCache::FindFlagChar(AdminFlag flag, char *c)
{
	char flagchar = g_ReverseFlags[flag];
	if (c)
	{
		*c = flagchar;
	}

	return flagchar != '?';
}

FlagBits AdminCache::ReadFlagString(const char *flags, const char **end)
{
	FlagBits bits = 0;

	while (flags && (*flags != '\0'))
	{
		AdminFlag flag;
		if (!FindFlag(*flags, &flag))
		{
			break;
		}
		bits |= FlagArrayToBits(&flag, 1);
		flags++;
	}

	if (end)
	{
		*end = flags;
	}

	return bits;
}

unsigned int AdminCache::GetAdminSerialChange(AdminId id)
{
	AdminUser *pUser = (AdminUser *)m_pMemory->GetAddress(id);
	if (!pUser || pUser->magic != USR_MAGIC_SET)
	{
		return 0;
	}

	return pUser->serialchange;
}

bool AdminCache::CanAdminUseCommand(int client, const char *cmd)
{
	FlagBits bits;
	OverrideType otype = Override_Command;

	if (cmd[0] == '@')
	{
		otype = Override_CommandGroup;
		cmd++;
	}

	if (!bridge->LookForCommandAdminFlags(cmd, &bits))
	{
		if (!GetCommandOverride(cmd, otype, &bits))
		{
			bits = 0;
		}
	}

	return CheckClientCommandAccess(client, cmd, bits);
}

unsigned int AdminCache::SetGroupImmunityLevel(GroupId gid, unsigned int level)
{
	AdminGroup *pGroup = (AdminGroup *)m_pMemory->GetAddress(gid);
	if (!pGroup || pGroup->magic != GRP_MAGIC_SET)
	{
		return 0;
	}

	unsigned int old_level = pGroup->immunity_level;

	pGroup->immunity_level = level;

	return old_level;
}

unsigned int AdminCache::GetGroupImmunityLevel(GroupId gid)
{
	AdminGroup *pGroup = (AdminGroup *)m_pMemory->GetAddress(gid);
	if (!pGroup || pGroup->magic != GRP_MAGIC_SET)
	{
		return 0;
	}

	return pGroup->immunity_level;
}

unsigned int AdminCache::SetAdminImmunityLevel(AdminId id, unsigned int level)
{
	AdminUser *pUser = (AdminUser *)m_pMemory->GetAddress(id);
	if (!pUser || pUser->magic != USR_MAGIC_SET)
	{
		return 0;
	}

	unsigned int old_level = pUser->immunity_level;

	pUser->immunity_level = level;

	return old_level;
}

unsigned int AdminCache::GetAdminImmunityLevel(AdminId id)
{
	AdminUser *pUser = (AdminUser *)m_pMemory->GetAddress(id);
	if (!pUser || pUser->magic != USR_MAGIC_SET)
	{
		return 0;
	}

	return pUser->immunity_level;
}

bool AdminCache::CheckAccess(int client, const char *cmd, FlagBits flags, bool override_only)
{
	if (client == 0)
	{
		return true;
	}

	/* Auto-detect a command if we can */
	FlagBits bits = flags;
	bool found_command = false;
	if (!override_only)
	{
		found_command = bridge->LookForCommandAdminFlags(cmd, &bits);
	}

	if (!found_command)
	{
		GetCommandOverride(cmd, Override_Command, &bits);
	}

	return CheckClientCommandAccess(client, cmd, bits) ? 1 : 0;
}

void iterator_glob_basic_override(FILE *fp, const char *key, FlagBits flags)
{
	char flagstr[64];
	g_Admins.FillFlagString(flags, flagstr, sizeof(flagstr));
	fprintf(fp, "\t\"%s\"\t\t\"%s\"\n", key, flagstr);
}

void iterator_glob_grp_override(FILE *fp, const char *key, FlagBits flags)
{
	char flagstr[64];
	g_Admins.FillFlagString(flags, flagstr, sizeof(flagstr));
	fprintf(fp, "\t\"@%s\"\t\t\"%s\"\n", key, flagstr);
}

void iterator_group_basic_override(FILE *fp, const char *key, OverrideRule rule)
{
	const char *str = (rule == Command_Allow) ? "allow" : "deny";
	fprintf(fp, "\t\t\t\"%s\"\t\t\"%s\"\n", key, str);
}

void iterator_group_grp_override(FILE *fp, const char *key, OverrideRule rule)
{
	const char *str = (rule == Command_Allow) ? "allow" : "deny";
	fprintf(fp, "\t\t\t\"@%s\"\t\t\"%s\"\n", key, str);
}

bool AdminCache::DumpCache(const char *filename)
{
	int *itable;
	AdminId aid;
	GroupId gid;
	char flagstr[64];
	unsigned int num;
	AdminUser *pAdmin;
	AdminGroup *pGroup;
	
	FILE *fp;
	if ((fp = fopen(filename, "wt")) == NULL)
	{
		return false;
	}

	fprintf(fp, "\"Groups\"\n{\n");
	
	num = 0;
	gid = m_FirstGroup;
	while (gid != INVALID_GROUP_ID
			&& (pGroup = GetGroup(gid)) != NULL)
	{
		num++;
		FillFlagString(pGroup->addflags, flagstr, sizeof(flagstr));

		fprintf(fp, "\t/* num = %d, gid = 0x%X */\n", num, gid);
		fprintf(fp, "\t\"%s\"\n\t{\n", GetString(pGroup->nameidx));
		fprintf(fp, "\t\t\"flags\"\t\t\t\"%s\"\n", flagstr);
		fprintf(fp, "\t\t\"immunity\"\t\t\"%d\"\n", pGroup->immunity_level);
		
		if (pGroup->immune_table != -1
			&& (itable = (int *)m_pMemory->GetAddress(pGroup->immune_table)) != NULL)
		{
			AdminGroup *pAltGroup;
			const char *gname, *mod;

			for (int i = 1; i <= itable[0]; i++)
			{
				if ((pAltGroup = GetGroup(itable[i])) == NULL)
				{
					/* Assume the rest of the table is corrupt */
					break;
				}
				gname = GetString(pAltGroup->nameidx);
				if (atoi(gname) != 0)
				{
					mod = "@";
				}
				else
				{
					mod = "";
				}
				fprintf(fp, "\t\t\"immunity\"\t\t\"%s%s\"\n", mod, gname);
			}
		}

		fprintf(fp, "\n\t\t\"Overrides\"\n\t\t{\n");
		if (pGroup->pCmdGrpTable != NULL)
		{
			for (OverrideMap::iterator iter = pGroup->pCmdTable->iter(); !iter.empty(); iter.next())
				iterator_group_grp_override(fp, iter->key.c_str(), iter->value);
		}
		if (pGroup->pCmdTable != NULL)
		{
			for (OverrideMap::iterator iter = pGroup->pCmdTable->iter(); !iter.empty(); iter.next())
				iterator_group_basic_override(fp, iter->key.c_str(), iter->value);
		}
		fprintf(fp, "\t\t}\n");

		fprintf(fp, "\t}\n");

		if ((gid = pGroup->next_grp) != INVALID_GROUP_ID)
		{
			fprintf(fp, "\n");
		}
	}

	fprintf(fp, "}\n\n");
	fprintf(fp, "\"Admins\"\n{\n");

	num = 0;
	aid = m_FirstUser;
	while (aid != INVALID_ADMIN_ID
			&& (pAdmin = GetUser(aid)) != NULL)
	{
		num++;
		FillFlagString(pAdmin->flags, flagstr, sizeof(flagstr));

		fprintf(fp, "\t/* num = %d, aid = 0x%X, serialno = 0x%X*/\n", num, aid, pAdmin->serialchange);

		if (pAdmin->nameidx != -1)
		{
			fprintf(fp, "\t\"%s\"\n\t{\n", GetString(pAdmin->nameidx));
		}
		else
		{
			fprintf(fp, "\t\"\"\n\t{\n");
		}

		if (pAdmin->auth.identidx != -1)
		{
			fprintf(fp, "\t\t\"auth\"\t\t\t\"%s\"\n", GetMethodName(pAdmin->auth.index));
			fprintf(fp, "\t\t\"identity\"\t\t\"%s\"\n", GetString(pAdmin->auth.identidx));
		}
		if (pAdmin->password != -1)
		{
			fprintf(fp, "\t\t\"password\"\t\t\"%s\"\n", GetString(pAdmin->password));
		}
		fprintf(fp, "\t\t\"flags\"\t\t\t\"%s\"\n", flagstr);
		fprintf(fp, "\t\t\"immunity\"\t\t\"%d\"\n", pAdmin->immunity_level);

		if (pAdmin->grp_count != 0 
			&& pAdmin->grp_table != -1 
			&& (itable = (int *)m_pMemory->GetAddress(pAdmin->grp_table)) != NULL)
		{
			unsigned int i;

			for (i = 0; i < pAdmin->grp_count; i++)
			{
				if ((pGroup = GetGroup(itable[i])) == NULL)
				{
					/* Assume the rest of the table is corrupt */
					break;
				}
				fprintf(fp, "\t\t\"group\"\t\t\t\"%s\"\n", GetString(pGroup->nameidx));
			}
		}

		fprintf(fp, "\t}\n");

		if ((aid = pAdmin->next_user) != INVALID_ADMIN_ID)
		{
			fprintf(fp, "\n");
		}
	}

	fprintf(fp, "}\n\n");

	fprintf(fp, "\"Overrides\"\n{\n");
	for (FlagMap::iterator iter = m_CmdGrpOverrides.iter(); !iter.empty(); iter.next())
		iterator_glob_grp_override(fp, iter->key.c_str(), iter->value);
	for (FlagMap::iterator iter = m_CmdOverrides.iter(); !iter.empty(); iter.next())
		iterator_glob_basic_override(fp, iter->key.c_str(), iter->value);
	fprintf(fp, "}\n");
	
	fclose(fp);
	
	return true;
}

AdminGroup *AdminCache::GetGroup(GroupId gid)
{
	AdminGroup *pGroup;
	
	pGroup = (AdminGroup *)m_pMemory->GetAddress(gid);
	if (!pGroup || pGroup->magic != GRP_MAGIC_SET)
	{
		return NULL;
	}

	return pGroup;
}

AdminUser *AdminCache::GetUser(AdminId aid)
{
	AdminUser *pAdmin;

	pAdmin = (AdminUser *)m_pMemory->GetAddress(aid);
	if (!pAdmin || pAdmin->magic != USR_MAGIC_SET)
	{
		return NULL;
	}

	return pAdmin;
}

const char *AdminCache::GetMethodName(unsigned int index)
{
	List<AuthMethod *>::iterator iter;
	for (iter=m_AuthMethods.begin();
		iter!=m_AuthMethods.end();
		iter++)
	{
		if (index-- == 0)
		{
			return (*iter)->name.c_str();
		}
	}

	return NULL;
}

const char *AdminCache::GetString(int idx)
{
	return m_pStrings->GetString(idx);
}

size_t AdminCache::FillFlagString(FlagBits bits, char *buffer, size_t maxlen)
{
	size_t pos;
	unsigned int i, num_flags;
	AdminFlag flags[AdminFlags_TOTAL];

	num_flags = FlagBitsToArray(bits, flags, AdminFlags_TOTAL);

	pos = 0;
	for (i = 0; pos < maxlen && i < num_flags; i++)
	{
		if (FindFlagChar(flags[i], &buffer[pos]))
		{
			pos++;
		}
	}
	buffer[pos] = '\0';

	return pos;
}

bool AdminCache::CheckClientCommandAccess(int client, const char *cmd, FlagBits cmdflags)
{
	if (cmdflags == 0 || client == 0)
	{
		return true;
	}

	/* If running listen server, then client 1 is the server host and should have 'root' access */
	if (client == 1 && !engine->IsDedicatedServer())
	{
		return true;
	}

	IGamePlayer *player = playerhelpers->GetGamePlayer(client);
	if (!player
		|| player->GetEdict() == NULL
		|| player->IsFakeClient())
	{
		return false;
	}

	return CheckAdminCommandAccess(player->GetAdminId(), cmd, cmdflags);
}

bool AdminCache::CheckAdminCommandAccess(AdminId adm, const char *cmd, FlagBits cmdflags)
{
	if (adm != INVALID_ADMIN_ID)
	{
		FlagBits bits = GetAdminFlags(adm, Access_Effective);

		/* root knows all, WHOA */
		if ((bits & ADMFLAG_ROOT) == ADMFLAG_ROOT)
		{
			return true;
		}

		/* Check for overrides
		* :TODO: is it worth optimizing this?
		*/
		unsigned int groups = GetAdminGroupCount(adm);
		GroupId gid;
		OverrideRule rule;
		bool override = false;
		for (unsigned int i = 0; i<groups; i++)
		{
			gid = GetAdminGroup(adm, i, NULL);
			/* First get group-level override */
			override = GetGroupCommandOverride(gid, cmd, Override_CommandGroup, &rule);
			/* Now get the specific command override */
			if (GetGroupCommandOverride(gid, cmd, Override_Command, &rule))
			{
				override = true;
			}
			if (override)
			{
				if (rule == Command_Allow)
				{
					return true;
				}
				else if (rule == Command_Deny)
				{
					return false;
				}
			}
		}

		/* See if our other flags match */
		if ((bits & cmdflags) == cmdflags)
		{
			return true;
		}
	}

	return false;
}
