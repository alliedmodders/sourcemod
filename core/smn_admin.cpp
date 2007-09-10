/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2007 AlliedModders LLC.  All rights reserved.
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

#include "sm_globals.h"
#include "AdminCache.h"
#include "ForwardSys.h"

static cell_t DumpAdminCache(IPluginContext *pContext, const cell_t *params)
{
	g_Admins.DumpAdminCache((AdminCachePart)params[1], (params[2] == 1) ? true : false);
	return 1;
}

static cell_t AddCommandOverride(IPluginContext *pContext, const cell_t *params)
{
	char *str;

	pContext->LocalToString(params[1], &str);
	g_Admins.AddCommandOverride(str, (OverrideType)params[2], (AdminFlag)params[3]);

	return 1;
}

static cell_t GetCommandOverride(IPluginContext *pContext, const cell_t *params)
{
	char *cmd;
	FlagBits flags;

	pContext->LocalToString(params[1], &cmd);

	if (!g_Admins.GetCommandOverride(cmd, (OverrideType)params[2], &flags))
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

	g_Admins.UnsetCommandOverride(cmd, (OverrideType)params[2]);

	return 1;
}

static cell_t CreateAdmGroup(IPluginContext *pContext, const cell_t *params)
{
	char *str;

	pContext->LocalToString(params[1], &str);

	return g_Admins.AddGroup(str);
}

static cell_t FindAdmGroup(IPluginContext *pContext, const cell_t *params)
{
	char *str;

	pContext->LocalToString(params[1], &str);

	return g_Admins.FindGroupByName(str);
}

static cell_t SetAdmGroupAddFlag(IPluginContext *pContext, const cell_t *params)
{
	GroupId id = (GroupId)params[1];
	AdminFlag flag = (AdminFlag)params[2];
	bool enabled = params[3] ? 1 : 0;

	g_Admins.SetGroupAddFlag(id, flag, enabled);

	return 1;
}

static cell_t GetAdmGroupAddFlag(IPluginContext *pContext, const cell_t *params)
{
	GroupId id = (GroupId)params[1];
	AdminFlag flag = (AdminFlag)params[2];
	
	return g_Admins.GetGroupAddFlag(id, flag) ? 1 : 0;
}

static cell_t SetAdmGroupImmunity(IPluginContext *pContext, const cell_t *params)
{
	GroupId id = (GroupId)params[1];
	ImmunityType type = (ImmunityType)params[2];
	bool enabled = params[3] ? 1 : 0;

	g_Admins.SetGroupGenericImmunity(id, type, enabled);

	return 1;
}

static cell_t GetAdmGroupImmunity(IPluginContext *pContext, const cell_t *params)
{
	GroupId id = (GroupId)params[1];
	ImmunityType type = (ImmunityType)params[2];

	return g_Admins.GetGroupGenericImmunity(id, type) ? 1 : 0;
}

static cell_t SetAdmGroupImmuneFrom(IPluginContext *pContext, const cell_t *params)
{
	GroupId id = (GroupId)params[1];
	GroupId other_id = (GroupId)params[2];

	g_Admins.AddGroupImmunity(id, other_id);

	return 1;
}

static cell_t GetAdmGroupImmuneCount(IPluginContext *pContext, const cell_t *params)
{
	GroupId id = (GroupId)params[1];

	return g_Admins.GetGroupImmunityCount(id);
}

static cell_t GetAdmGroupImmuneFrom(IPluginContext *pContext, const cell_t *params)
{
	GroupId id = (GroupId)params[1];

	return g_Admins.GetGroupImmunity(id, params[2]);
}

static cell_t AddAdmGroupCmdOverride(IPluginContext *pContext, const cell_t *params)
{
	GroupId id = (GroupId)params[1];
	OverrideType type = (OverrideType)params[3];
	OverrideRule rule = (OverrideRule)params[4];
	char *cmd;

	pContext->LocalToString(params[2], &cmd);

	g_Admins.AddGroupCommandOverride(id, cmd, type, rule);

	return 1;
}

static cell_t GetAdmGroupCmdOverride(IPluginContext *pContext, const cell_t *params)
{
	GroupId id = (GroupId)params[1];
	OverrideType type = (OverrideType)params[3];
	OverrideRule rule;
	char *cmd;

	pContext->LocalToString(params[2], &cmd);

	if (!g_Admins.GetGroupCommandOverride(id, cmd, type, &rule))
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
	return g_Admins.GetGroupAddFlags(id);
}

static cell_t RegisterAuthIdentType(IPluginContext *pContext, const cell_t *params)
{
	char *type;
	pContext->LocalToString(params[1], &type);

	g_Admins.RegisterAuthIdentType(type);

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

	return g_Admins.CreateAdmin(admin);
}

static cell_t GetAdminUsername(IPluginContext *pContext, const cell_t *params)
{
	AdminId id = params[1];
	size_t written;
	const char *name = g_Admins.GetAdminName(id);

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

	return g_Admins.BindAdminIdentity(id, auth, ident);
}

static cell_t SetAdminFlag(IPluginContext *pContext, const cell_t *params)
{
	AdminId id = params[1];
	AdminFlag flag = (AdminFlag)params[2];
	bool enabled = params[3] ? true : false;

	g_Admins.SetAdminFlag(id, flag, enabled);

	return 1;
}

static cell_t GetAdminFlag(IPluginContext *pContext, const cell_t *params)
{
	AdminId id = params[1];
	AdminFlag flag = (AdminFlag)params[2];
	AccessMode mode = (AccessMode)params[3];

	return g_Admins.GetAdminFlag(id, flag, mode);
}

static cell_t GetAdminFlags(IPluginContext *pContext, const cell_t *params)
{
	AdminId id = params[1];
	AccessMode mode = (AccessMode)params[2];

	return g_Admins.GetAdminFlags(id, mode);
}

static cell_t AdminInheritGroup(IPluginContext *pContext, const cell_t *params)
{
	AdminId id = params[1];
	GroupId gid = params[2];

	return g_Admins.AdminInheritGroup(id, gid);
}

static cell_t GetAdminGroupCount(IPluginContext *pContext, const cell_t *params)
{
	AdminId id = params[1];

	return g_Admins.GetAdminGroupCount(id);
}

static cell_t GetAdminGroup(IPluginContext *pContext, const cell_t *params)
{
	AdminId id = params[1];
	unsigned int index = params[2];
	const char *name;
	GroupId gid;

	if ((gid=g_Admins.GetAdminGroup(id, index, &name)) == INVALID_GROUP_ID)
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

	g_Admins.SetAdminPassword(id, password);

	return 1;
}

static cell_t GetAdminPassword(IPluginContext *pContext, const cell_t *params)
{
	AdminId id = params[1];
	const char *password = g_Admins.GetAdminPassword(id);

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

	return g_Admins.FindAdminByIdentity(auth, ident);
}

static cell_t RemoveAdmin(IPluginContext *pContext, const cell_t *params)
{
	AdminId id = params[1];

	return g_Admins.InvalidateAdmin(id);
}

static cell_t FlagBitsToBitArray(IPluginContext *pContext, const cell_t *params)
{
	FlagBits bits = (FlagBits)params[1];
	bool array[AdminFlags_TOTAL];
	cell_t *addr;
	unsigned int numWr = g_Admins.FlagBitsToBitArray(bits, array, AdminFlags_TOTAL);

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

	return g_Admins.FlagBitArrayToBits(array, num);
}

static cell_t FlagArrayToBits(IPluginContext *pContext, const cell_t *params)
{
	cell_t *addr;
	pContext->LocalToPhysAddr(params[1], &addr);

	if (sizeof(AdminFlag) == sizeof(cell_t))
	{
		return g_Admins.FlagArrayToBits((const AdminFlag *)addr, params[2]);
	} else {
		AdminFlag flags[AdminFlags_TOTAL];
		unsigned int num = ((unsigned)params[2] > AdminFlags_TOTAL ? AdminFlags_TOTAL : params[2]);

		for (unsigned int i=0; i<num; i++)
		{
			flags[i] = (AdminFlag)addr[i];
		}
		return g_Admins.FlagArrayToBits(flags, num);
	}
}

static cell_t FlagBitsToArray(IPluginContext *pContext, const cell_t *params)
{
	cell_t *addr;
	pContext->LocalToPhysAddr(params[2], &addr);

	if (sizeof(AdminFlag) == sizeof(cell_t))
	{
		return g_Admins.FlagBitsToArray(params[1], (AdminFlag *)addr, params[3]);
	} else {
		AdminFlag flags[AdminFlags_TOTAL];
		unsigned int num = ((unsigned)params[2] > AdminFlags_TOTAL ? AdminFlags_TOTAL : params[2]);
		num = g_Admins.FlagBitsToArray(params[1], flags, num);

		for (unsigned int i=0; i<num; i++)
		{
			addr[i] = flags[i];
		}

		return num;
	}
}

static cell_t CanAdminTarget(IPluginContext *pContext, const cell_t *params)
{
	return g_Admins.CanAdminTarget(params[1], params[2]) ? 1 : 0;
}

static cell_t CreateAuthMethod(IPluginContext *pContext, const cell_t *params)
{
	char *name;
	pContext->LocalToString(params[1], &name);
	g_Admins.RegisterAuthIdentType(name);

	return 1;
}

static cell_t SetAdmGroupImmunityLevel(IPluginContext *pContext, const cell_t *params)
{
	return g_Admins.SetGroupImmunityLevel(params[1], params[2]);
}

static cell_t GetAdmGroupImmunityLevel(IPluginContext *pContext, const cell_t *params)
{
	return g_Admins.GetGroupImmunityLevel(params[1]);
}

static cell_t SetAdminImmunityLevel(IPluginContext *pContext, const cell_t *params)
{
	return g_Admins.SetAdminImmunityLevel(params[1], params[2]);
}

static cell_t GetAdminImmunityLevel(IPluginContext *pContext, const cell_t *params)
{
	return g_Admins.GetAdminImmunityLevel(params[1]);
}

static cell_t FindFlagByName(IPluginContext *pContext, const cell_t *params)
{
	char *flag;
	pContext->LocalToString(params[1], &flag);

	cell_t *addr;
	pContext->LocalToPhysAddr(params[2], &addr);

	AdminFlag admflag;
	if (!g_Admins.FindFlag(flag, &admflag))
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
	if (!g_Admins.FindFlag((char)params[1], &admflag))
	{
		return 0;
	}

	*addr = (cell_t)admflag;

	return 1;
}

static cell_t ReadFlagString(IPluginContext *pContext, const cell_t *params)
{
	char *flag;
	pContext->LocalToString(params[1], &flag);

	cell_t *addr;
	pContext->LocalToPhysAddr(params[2], &addr);

	const char *end = flag;
	FlagBits bits = g_Admins.ReadFlagString(flag, &end);

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
	{"ReadFlagString",			ReadFlagString},
	{"GetAdmGroupImmunityLevel",GetAdmGroupImmunityLevel},
	{"SetAdmGroupImmunityLevel",SetAdmGroupImmunityLevel},
	{"GetAdminImmunityLevel",	GetAdminImmunityLevel},
	{"SetAdminImmunityLevel",	SetAdminImmunityLevel},
	/* -------------------------------------------------- */
	{NULL,						NULL},
};

