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

#include "common_logic.h"
#include <IAdminSystem.h>
#include <IForwardSys.h>

using namespace SourceMod;

static cell_t DumpAdminCache(IPluginContext *pContext, const cell_t *params)
{
	adminsys->DumpAdminCache((AdminCachePart)params[1], (params[2] == 1) ? true : false);
	return 1;
}

static cell_t AddCommandOverride(IPluginContext *pContext, const cell_t *params)
{
	char *str;

	pContext->LocalToString(params[1], &str);
	adminsys->AddCommandOverride(str, (OverrideType)params[2], (AdminFlag)params[3]);

	return 1;
}

static cell_t GetCommandOverride(IPluginContext *pContext, const cell_t *params)
{
	char *cmd;
	FlagBits flags;

	pContext->LocalToString(params[1], &cmd);

	if (!adminsys->GetCommandOverride(cmd, (OverrideType)params[2], &flags))
	{
		return 0;
	}

	cell_t *addr;
	pContext->LocalToPhysAddr(params[3], &addr);

	*addr = (cell_t)flags;

	return 1;
}

static cell_t UnsetCommandOverride(IPluginContext *pContext, const cell_t *params)
{
	char *cmd;

	pContext->LocalToString(params[1], &cmd);

	adminsys->UnsetCommandOverride(cmd, (OverrideType)params[2]);

	return 1;
}

static cell_t CreateAdmGroup(IPluginContext *pContext, const cell_t *params)
{
	char *str;

	pContext->LocalToString(params[1], &str);

	return adminsys->AddGroup(str);
}

static cell_t FindAdmGroup(IPluginContext *pContext, const cell_t *params)
{
	char *str;

	pContext->LocalToString(params[1], &str);

	return adminsys->FindGroupByName(str);
}

static cell_t SetAdmGroupAddFlag(IPluginContext *pContext, const cell_t *params)
{
	GroupId id = (GroupId)params[1];
	AdminFlag flag = (AdminFlag)params[2];
	bool enabled = params[3] ? 1 : 0;

	adminsys->SetGroupAddFlag(id, flag, enabled);

	return 1;
}

static cell_t GetAdmGroupAddFlag(IPluginContext *pContext, const cell_t *params)
{
	GroupId id = (GroupId)params[1];
	AdminFlag flag = (AdminFlag)params[2];
	
	return adminsys->GetGroupAddFlag(id, flag) ? 1 : 0;
}

static cell_t SetAdmGroupImmunity(IPluginContext *pContext, const cell_t *params)
{
	GroupId id = (GroupId)params[1];
	ImmunityType type = (ImmunityType)params[2];
	bool enabled = params[3] ? 1 : 0;

	adminsys->SetGroupGenericImmunity(id, type, enabled);

	return 1;
}

static cell_t GetAdmGroupImmunity(IPluginContext *pContext, const cell_t *params)
{
	GroupId id = (GroupId)params[1];
	ImmunityType type = (ImmunityType)params[2];

	return adminsys->GetGroupGenericImmunity(id, type) ? 1 : 0;
}

static cell_t SetAdmGroupImmuneFrom(IPluginContext *pContext, const cell_t *params)
{
	GroupId id = (GroupId)params[1];
	GroupId other_id = (GroupId)params[2];

	adminsys->AddGroupImmunity(id, other_id);

	return 1;
}

static cell_t GetAdmGroupImmuneCount(IPluginContext *pContext, const cell_t *params)
{
	GroupId id = (GroupId)params[1];

	return adminsys->GetGroupImmunityCount(id);
}

static cell_t GetAdmGroupImmuneFrom(IPluginContext *pContext, const cell_t *params)
{
	GroupId id = (GroupId)params[1];

	return adminsys->GetGroupImmunity(id, params[2]);
}

static cell_t AddAdmGroupCmdOverride(IPluginContext *pContext, const cell_t *params)
{
	GroupId id = (GroupId)params[1];
	OverrideType type = (OverrideType)params[3];
	OverrideRule rule = (OverrideRule)params[4];
	char *cmd;

	pContext->LocalToString(params[2], &cmd);

	adminsys->AddGroupCommandOverride(id, cmd, type, rule);

	return 1;
}

static cell_t GetAdmGroupCmdOverride(IPluginContext *pContext, const cell_t *params)
{
	GroupId id = (GroupId)params[1];
	OverrideType type = (OverrideType)params[3];
	OverrideRule rule;
	char *cmd;

	pContext->LocalToString(params[2], &cmd);

	if (!adminsys->GetGroupCommandOverride(id, cmd, type, &rule))
	{
		return 0;
	}

	cell_t *addr;
	pContext->LocalToPhysAddr(params[4], &addr);
	*addr = (cell_t)rule;

	return 1;
}

static cell_t GetAdmGroupAddFlags(IPluginContext *pContext, const cell_t *params)
{
	GroupId id = (GroupId)params[1];
	return adminsys->GetGroupAddFlags(id);
}

static cell_t RegisterAuthIdentType(IPluginContext *pContext, const cell_t *params)
{
	char *type;
	pContext->LocalToString(params[1], &type);

	adminsys->RegisterAuthIdentType(type);

	return 1;
}

static cell_t CreateAdmin(IPluginContext *pContext, const cell_t *params)
{
	char *admin;
	pContext->LocalToString(params[1], &admin);

	if (admin[0] == '\0')
	{
		admin = NULL;
	}

	return adminsys->CreateAdmin(admin);
}

static cell_t GetAdminUsername(IPluginContext *pContext, const cell_t *params)
{
	AdminId id = params[1];
	size_t written;
	const char *name = adminsys->GetAdminName(id);

	if (!name)
	{
		return 0;
	}

	pContext->StringToLocalUTF8(params[2], params[3], name, &written);

	return written;
}

static cell_t BindAdminIdentity(IPluginContext *pContext, const cell_t *params)
{
	AdminId id = params[1];
	char *auth, *ident;

	pContext->LocalToString(params[2], &auth);
	pContext->LocalToString(params[3], &ident);

	return adminsys->BindAdminIdentity(id, auth, ident);
}

static cell_t SetAdminFlag(IPluginContext *pContext, const cell_t *params)
{
	AdminId id = params[1];
	AdminFlag flag = (AdminFlag)params[2];
	bool enabled = params[3] ? true : false;

	adminsys->SetAdminFlag(id, flag, enabled);

	return 1;
}

static cell_t GetAdminFlag(IPluginContext *pContext, const cell_t *params)
{
	AdminId id = params[1];
	AdminFlag flag = (AdminFlag)params[2];
	AccessMode mode = (AccessMode)params[3];

	return adminsys->GetAdminFlag(id, flag, mode);
}

static cell_t GetAdminFlags(IPluginContext *pContext, const cell_t *params)
{
	AdminId id = params[1];
	AccessMode mode = (AccessMode)params[2];

	return adminsys->GetAdminFlags(id, mode);
}

static cell_t AdminInheritGroup(IPluginContext *pContext, const cell_t *params)
{
	AdminId id = params[1];
	GroupId gid = params[2];

	return adminsys->AdminInheritGroup(id, gid);
}

static cell_t GetAdminGroupCount(IPluginContext *pContext, const cell_t *params)
{
	AdminId id = params[1];

	return adminsys->GetAdminGroupCount(id);
}

static cell_t GetAdminGroup(IPluginContext *pContext, const cell_t *params)
{
	AdminId id = params[1];
	unsigned int index = params[2];
	const char *name;
	GroupId gid;

	if ((gid=adminsys->GetAdminGroup(id, index, &name)) == INVALID_GROUP_ID)
	{
		return gid;
	}

	if (name == NULL)
	{
		name = "";
	}

	pContext->StringToLocalUTF8(params[3], params[4], name, NULL);

	return gid;
}

static cell_t SetAdminPassword(IPluginContext *pContext, const cell_t *params)
{
	AdminId id = params[1];
	char *password;

	pContext->LocalToString(params[2], &password);

	adminsys->SetAdminPassword(id, password);

	return 1;
}

static cell_t GetAdminPassword(IPluginContext *pContext, const cell_t *params)
{
	AdminId id = params[1];
	const char *password = adminsys->GetAdminPassword(id);

	if (!password)
	{
		return 0;
	}

	pContext->StringToLocalUTF8(params[2], params[3], password, NULL);

	return 1;
}

static cell_t FindAdminByIdentity(IPluginContext *pContext, const cell_t *params)
{
	char *auth, *ident;

	pContext->LocalToString(params[1], &auth);
	pContext->LocalToString(params[2], &ident);

	return adminsys->FindAdminByIdentity(auth, ident);
}

static cell_t RemoveAdmin(IPluginContext *pContext, const cell_t *params)
{
	AdminId id = params[1];

	return adminsys->InvalidateAdmin(id);
}

static cell_t FlagBitsToBitArray(IPluginContext *pContext, const cell_t *params)
{
	FlagBits bits = (FlagBits)params[1];
	bool array[AdminFlags_TOTAL];
	cell_t *addr;
	unsigned int numWr = adminsys->FlagBitsToBitArray(bits, array, AdminFlags_TOTAL);

	pContext->LocalToPhysAddr(params[2], &addr);

	unsigned int i;
	for (i=0; i<numWr && i<(unsigned int)params[3]; i++)
	{
		addr[i] = array[i] ? 1 : 0;
	}

	return i;
}

static cell_t FlagBitArrayToBits(IPluginContext *pContext, const cell_t *params)
{
	bool array[AdminFlags_TOTAL];
	unsigned int num = ((unsigned int)params[2] > AdminFlags_TOTAL ? params[2] : AdminFlags_TOTAL);
	cell_t *addr;

	pContext->LocalToPhysAddr(params[1], &addr);

	for (unsigned int i=0; i<num; i++)
	{
		array[i] = addr[i] ? true : false;
	}

	return adminsys->FlagBitArrayToBits(array, num);
}

static cell_t FlagArrayToBits(IPluginContext *pContext, const cell_t *params)
{
	cell_t *addr;
	pContext->LocalToPhysAddr(params[1], &addr);

	if (sizeof(AdminFlag) == sizeof(cell_t))
	{
		return adminsys->FlagArrayToBits((const AdminFlag *)addr, params[2]);
	} else {
		AdminFlag flags[AdminFlags_TOTAL];
		unsigned int num = ((unsigned)params[2] > AdminFlags_TOTAL ? AdminFlags_TOTAL : params[2]);

		for (unsigned int i=0; i<num; i++)
		{
			flags[i] = (AdminFlag)addr[i];
		}
		return adminsys->FlagArrayToBits(flags, num);
	}
}

static cell_t FlagBitsToArray(IPluginContext *pContext, const cell_t *params)
{
	cell_t *addr;
	pContext->LocalToPhysAddr(params[2], &addr);

	if (sizeof(AdminFlag) == sizeof(cell_t))
	{
		return adminsys->FlagBitsToArray(params[1], (AdminFlag *)addr, params[3]);
	} else {
		AdminFlag flags[AdminFlags_TOTAL];
		unsigned int num = ((unsigned)params[2] > AdminFlags_TOTAL ? AdminFlags_TOTAL : params[2]);
		num = adminsys->FlagBitsToArray(params[1], flags, num);

		for (unsigned int i=0; i<num; i++)
		{
			addr[i] = flags[i];
		}

		return num;
	}
}

static cell_t CanAdminTarget(IPluginContext *pContext, const cell_t *params)
{
	return adminsys->CanAdminTarget(params[1], params[2]) ? 1 : 0;
}

static cell_t CreateAuthMethod(IPluginContext *pContext, const cell_t *params)
{
	char *name;
	pContext->LocalToString(params[1], &name);
	adminsys->RegisterAuthIdentType(name);

	return 1;
}

static cell_t SetAdmGroupImmunityLevel(IPluginContext *pContext, const cell_t *params)
{
	return adminsys->SetGroupImmunityLevel(params[1], params[2]);
}

static cell_t GetAdmGroupImmunityLevel(IPluginContext *pContext, const cell_t *params)
{
	return adminsys->GetGroupImmunityLevel(params[1]);
}

static cell_t SetAdminImmunityLevel(IPluginContext *pContext, const cell_t *params)
{
	return adminsys->SetAdminImmunityLevel(params[1], params[2]);
}

static cell_t GetAdminImmunityLevel(IPluginContext *pContext, const cell_t *params)
{
	return adminsys->GetAdminImmunityLevel(params[1]);
}

static cell_t FindFlagByName(IPluginContext *pContext, const cell_t *params)
{
	char *flag;
	pContext->LocalToString(params[1], &flag);

	cell_t *addr;
	pContext->LocalToPhysAddr(params[2], &addr);

	AdminFlag admflag;
	if (!adminsys->FindFlag(flag, &admflag))
	{
		return 0;
	}

	*addr = (cell_t)admflag;

	return 1;
}

static cell_t FindFlagByChar(IPluginContext *pContext, const cell_t *params)
{
	cell_t *addr;
	pContext->LocalToPhysAddr(params[2], &addr);

	AdminFlag admflag;
	if (!adminsys->FindFlag((char)params[1], &admflag))
	{
		return 0;
	}

	*addr = (cell_t)admflag;

	return 1;
}

static cell_t FindFlagChar(IPluginContext *pContext, const cell_t *params)
{
	cell_t *addr;
	pContext->LocalToPhysAddr(params[2], &addr);

	char flagchar;
	if (!adminsys->FindFlagChar((AdminFlag)params[1], &flagchar))
	{
		return 0;
	}

	*addr = (cell_t)flagchar;

	return 1;
}

static cell_t ReadFlagString(IPluginContext *pContext, const cell_t *params)
{
	char *flag;
	pContext->LocalToString(params[1], &flag);

	cell_t *addr;
	pContext->LocalToPhysAddr(params[2], &addr);

	const char *end = flag;
	FlagBits bits = adminsys->ReadFlagString(flag, &end);

	*addr = end - flag;

	return bits;
}

REGISTER_NATIVES(adminNatives)
{
	{"DumpAdminCache",			DumpAdminCache},
	{"AddCommandOverride",		AddCommandOverride},
	{"GetCommandOverride",		GetCommandOverride},
	{"UnsetCommandOverride",	UnsetCommandOverride},
	{"CreateAdmGroup",			CreateAdmGroup},
	{"FindAdmGroup",			FindAdmGroup},
	{"SetAdmGroupAddFlag",		SetAdmGroupAddFlag},
	{"GetAdmGroupAddFlag",		GetAdmGroupAddFlag},
	{"SetAdmGroupImmunity",		SetAdmGroupImmunity},
	{"GetAdmGroupImmunity",		GetAdmGroupImmunity},
	{"SetAdmGroupImmuneFrom",	SetAdmGroupImmuneFrom},
	{"GetAdmGroupImmuneFrom",	GetAdmGroupImmuneFrom},
	{"GetAdmGroupImmuneCount",	GetAdmGroupImmuneCount},
	{"AddAdmGroupCmdOverride",	AddAdmGroupCmdOverride},
	{"GetAdmGroupCmdOverride",	GetAdmGroupCmdOverride},
	{"GetAdmGroupAddFlags",		GetAdmGroupAddFlags},
	{"RegisterAuthIdentType",	RegisterAuthIdentType},
	{"CreateAdmin",				CreateAdmin},
	{"GetAdminUsername",		GetAdminUsername},
	{"BindAdminIdentity",		BindAdminIdentity},
	{"SetAdminFlag",			SetAdminFlag},
	{"GetAdminFlag",			GetAdminFlag},
	{"GetAdminFlags",			GetAdminFlags},
	{"AdminInheritGroup",		AdminInheritGroup},
	{"GetAdminGroupCount",		GetAdminGroupCount},
	{"GetAdminGroup",			GetAdminGroup},
	{"SetAdminPassword",		SetAdminPassword},
	{"GetAdminPassword",		GetAdminPassword},
	{"FindAdminByIdentity",		FindAdminByIdentity},
	{"RemoveAdmin",				RemoveAdmin},
	{"FlagBitsToBitArray",		FlagBitsToBitArray},
	{"FlagBitArrayToBits",		FlagBitArrayToBits},
	{"FlagArrayToBits",			FlagArrayToBits},
	{"FlagBitsToArray",			FlagBitsToArray},
	{"CanAdminTarget",			CanAdminTarget},
	{"CreateAuthMethod",		CreateAuthMethod},
	{"FindFlagByName",			FindFlagByName},
	{"FindFlagByChar",			FindFlagByChar},
	{"FindFlagChar",			FindFlagChar},
	{"ReadFlagString",			ReadFlagString},
	{"GetAdmGroupImmunityLevel",GetAdmGroupImmunityLevel},
	{"SetAdmGroupImmunityLevel",SetAdmGroupImmunityLevel},
	{"GetAdminImmunityLevel",	GetAdminImmunityLevel},
	{"SetAdminImmunityLevel",	SetAdminImmunityLevel},
	{"AdminId.GetUsername",		GetAdminUsername},
	{"AdminId.BindIdentity",	BindAdminIdentity},
	{"AdminId.SetFlag",			SetAdminFlag},
	{"AdminId.HasFlag",			GetAdminFlag},
	{"AdminId.GetFlags",		GetAdminFlags},
	{"AdminId.InheritGroup",	AdminInheritGroup},
	{"AdminId.GetGroup",		GetAdminGroup},
	{"AdminId.SetPassword",		SetAdminPassword},
	{"AdminId.GetPassword",		GetAdminPassword},
	{"AdminId.CanTarget",		CanAdminTarget},
	{"AdminId.GroupCount.get",	GetAdminGroupCount},
	{"AdminId.ImmunityLevel.get",	GetAdminImmunityLevel},
	{"AdminId.ImmunityLevel.set",	SetAdminImmunityLevel},
	{"GroupId.HasFlag",			GetAdmGroupAddFlag},
	{"GroupId.SetFlag",			SetAdmGroupAddFlag},
	{"GroupId.GetFlags",		GetAdmGroupAddFlags},
	{"GroupId.GetGroupImmunity",	GetAdmGroupImmuneFrom},
	{"GroupId.AddGroupImmunity",	SetAdmGroupImmuneFrom},
	{"GroupId.GetCommandOverride",	GetAdmGroupCmdOverride},
	{"GroupId.AddCommandOverride",  AddAdmGroupCmdOverride},
	{"GroupId.GroupImmunitiesCount.get",	GetAdmGroupImmuneCount},
	{"GroupId.ImmunityLevel.get",	GetAdmGroupImmunityLevel},
	{"GroupId.ImmunityLevel.set",   SetAdmGroupImmunityLevel},
	/* -------------------------------------------------- */
	{NULL,						NULL},
};

