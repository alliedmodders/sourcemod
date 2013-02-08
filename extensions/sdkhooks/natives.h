/**
 * vim: set ts=4 :
 * =============================================================================
 * Source SDK Hooks Extension
 * Copyright (C) 2010-2012 Nicholas Hastings
 * Copyright (C) 2009-2010 Erik Minekus
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

#ifndef _INCLUDE_SOURCEMOD_NATIVES_PROPER_H_
#define _INCLUDE_SOURCEMOD_NATIVES_PROPER_H_

#include "util.h"

cell_t Native_Hook(IPluginContext *pContext, const cell_t *params);
cell_t Native_HookEx(IPluginContext *pContext, const cell_t *params);
cell_t Native_Unhook(IPluginContext *pContext, const cell_t *params);
cell_t Native_TakeDamage(IPluginContext *pContext, const cell_t *params);
cell_t Native_DropWeapon(IPluginContext *pContext, const cell_t *params);

const sp_nativeinfo_t g_Natives[] = 
{
	{"SDKHook",		Native_Hook},
	{"SDKHookEx",		Native_HookEx},
	{"SDKUnhook",		Native_Unhook},
	{"SDKHooks_TakeDamage",	Native_TakeDamage},
	{"SDKHooks_DropWeapon",	Native_DropWeapon},
	{NULL,					NULL},
};

extern SDKHooks g_Interface;

#endif // _INCLUDE_SOURCEMOD_NATIVES_PROPER_H_
