/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Team Fortress 2 Extension
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

#include "extension.h"
#include "util.h"
#include "time.h"
#include "RegNatives.h"


// native TF2_Burn(client, target)
cell_t TF2_Burn(IPluginContext *pContext, const cell_t *params)
{
	static ICallWrapper *pWrapper = NULL;

	// CTFPlayerShared::Burn(CTFPlayer*)
	if (!pWrapper)
	{
		REGISTER_NATIVE_ADDR("Burn", 
			PassInfo pass[2]; \
			pass[0].flags = PASSFLAG_BYVAL; \
			pass[0].size = sizeof(CBaseEntity *); \
			pass[0].type = PassType_Basic; \
			pass[1].flags = PASSFLAG_BYVAL; \
			pass[1].size = sizeof(CBaseEntity *); \
			pass[1].type = PassType_Basic; \
			pWrapper = g_pBinTools->CreateCall(addr, CallConv_ThisCall, NULL, pass, 2))
	}

	CBaseEntity *pEntity;
	if (!(pEntity = UTIL_GetCBaseEntity(params[1], true)))
	{
		return pContext->ThrowNativeError("Client index %d is not valid", params[1]);
	}

	CBaseEntity *pTarget;
	if (!(pTarget = UTIL_GetCBaseEntity(params[2], true)))
	{
		return pContext->ThrowNativeError("Client index %d is not valid", params[2]);
	}

	void *obj = (void *)((uint8_t *)pEntity + playerSharedOffset->actual_offset);

	unsigned char vstk[sizeof(void *) + sizeof(CBaseEntity *)];
	unsigned char *vptr = vstk;

	*(void **)vptr = obj;
	vptr += sizeof(void *);
	*(CBaseEntity **)vptr = pTarget;
	vptr += sizeof(CBaseEntity *);
	*(CBaseEntity **)vptr = NULL;

	pWrapper->Execute(vstk, NULL);

	return 1;
}

// native TF2_Invuln(client, bool:enabled)
cell_t TF2_Invuln(IPluginContext *pContext, const cell_t *params)
{
	return pContext->ThrowNativeError("TF2_SetPlayerInvuln is no longer available");
}

cell_t TF2_Disguise(IPluginContext *pContext, const cell_t *params)
{
	static ICallWrapper *pWrapper = NULL;

	//CTFPlayerShared::Disguise(int, int)
	if (!pWrapper)
	{
		REGISTER_NATIVE_ADDR("Disguise", 
			PassInfo pass[2]; \
			pass[0].flags = PASSFLAG_BYVAL; \
			pass[0].size = sizeof(int); \
			pass[0].type = PassType_Basic; \
			pass[1].flags = PASSFLAG_BYVAL; \
			pass[1].size = sizeof(int); \
			pass[1].type = PassType_Basic; \
			pWrapper = g_pBinTools->CreateCall(addr, CallConv_ThisCall, NULL, pass, 2))
	}

	CBaseEntity *pEntity;
	if (!(pEntity = UTIL_GetCBaseEntity(params[1], true)))
	{
		return pContext->ThrowNativeError("Client index %d is not valid", params[1]);
	}

	void *obj = (void *)((uint8_t *)pEntity + playerSharedOffset->actual_offset);

	unsigned char vstk[sizeof(void *) + 2*sizeof(int)];
	unsigned char *vptr = vstk;


	*(void **)vptr = obj;
	vptr += sizeof(void *);
	*(int *)vptr = params[2];
	vptr += sizeof(int);
	*(int *)vptr = params[3];

	pWrapper->Execute(vstk, NULL);

	return 1;
}

cell_t TF2_RemoveDisguise(IPluginContext *pContext, const cell_t *params)
{
	static ICallWrapper *pWrapper = NULL;

	//CTFPlayerShared::RemoveDisguise()
	if (!pWrapper)
	{
		REGISTER_NATIVE_ADDR("RemoveDisguise", 
				pWrapper = g_pBinTools->CreateCall(addr, CallConv_ThisCall, NULL, NULL, 0))
	}

	CBaseEntity *pEntity;
	if (!(pEntity = UTIL_GetCBaseEntity(params[1], true)))
	{
		return pContext->ThrowNativeError("Client index %d is not valid", params[1]);
	}

	void *obj = (void *)((uint8_t *)pEntity + playerSharedOffset->actual_offset);

	unsigned char vstk[sizeof(void *)];
	unsigned char *vptr = vstk;


	*(void **)vptr = obj;

	pWrapper->Execute(vstk, NULL);

	return 1;
}

cell_t TF2_Respawn(IPluginContext *pContext, const cell_t *params)
{
	static ICallWrapper *pWrapper = NULL;

	//CTFPlayer::ForceRespawn()

	if (!pWrapper)
	{
		int offset;

		if (!g_pGameConf->GetOffset("ForceRespawn", &offset))
		{
			return pContext->ThrowNativeError("Failed to locate function");
		}

		pWrapper = g_pBinTools->CreateVCall(offset,
											0,
											0,
											NULL,
											NULL,
											0);

		g_RegNatives.Register(pWrapper);
	}

	CBaseEntity *pEntity;
	if (!(pEntity = UTIL_GetCBaseEntity(params[1], true)))
	{
		return pContext->ThrowNativeError("Client index %d is not valid", params[1]);
	}

	unsigned char vstk[sizeof(void *)];
	unsigned char *vptr = vstk;


	*(void **)vptr = (void *)pEntity;

	pWrapper->Execute(vstk, NULL);

	return 1;
}

cell_t TF2_GetResourceEntity(IPluginContext *pContext, const cell_t *params)
{
	return g_resourceEntity;
}

cell_t TF2_GetClass(IPluginContext *pContext, const cell_t *params)
{
	char *str;
	pContext->LocalToString(params[1], &str);

	return (cell_t)ClassnameToType(str);
}

sp_nativeinfo_t g_TFNatives[] = 
{
	{"TF2_IgnitePlayer",			TF2_Burn},
	{"TF2_SetPlayerInvuln",			TF2_Invuln},
	{"TF2_RespawnPlayer",			TF2_Respawn},
	{"TF2_DisguisePlayer",			TF2_Disguise},
	{"TF2_RemovePlayerDisguise",	TF2_RemoveDisguise},
	{"TF2_GetResourceEntity",		TF2_GetResourceEntity},
	{"TF2_GetClass",				TF2_GetClass},
	{NULL,							NULL}
};
