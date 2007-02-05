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

#include "sm_globals.h"
#include "AdminCache.h"
#include "ForwardSys.h"

static cell_t DumpAdminCache(IPluginContext *pContext, const cell_t *params)
{
	g_Admins.DumpAdminCache(params[1], (params[2] == 1) ? true : false);
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
	{NULL,						NULL},
};
