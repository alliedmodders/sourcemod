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

#include "extension.h"
#include "natives.h"
#include <compat_wrappers.h>
#include <IBinTools.h>
#include <sm_argbuffer.h>

using namespace SourceMod;

SH_DECL_MANUALEXTERN1(OnTakeDamage, int, CTakeDamageInfoHack &);
SH_DECL_MANUALEXTERN3_void(Weapon_Drop, CBaseCombatWeapon *, const Vector *, const Vector *);

cell_t Native_Hook(IPluginContext *pContext, const cell_t *params)
{
	int entity = (int)params[1];
	SDKHookType type = (SDKHookType)params[2];
	IPluginFunction *callback = pContext->GetFunctionById(params[3]);
	HookReturn ret = g_Interface.Hook(entity, type, callback);
	switch (ret)
	{
		case HookRet_InvalidEntity:
			pContext->ThrowNativeError("Entity %d is invalid", entity);
			break;
		case HookRet_InvalidHookType:
			pContext->ThrowNativeError("Invalid hook type specified");
			break;
		case HookRet_NotSupported:
			pContext->ThrowNativeError("Hook type not supported on this game");
			break;
		case HookRet_BadEntForHookType:
		{
			CBaseEntity *pEnt = gamehelpers->ReferenceToEntity(params[1]);
			const char *pClassname = pEnt ? gamehelpers->GetEntityClassname(pEnt) : NULL;
			if (!pClassname)
			{
				pContext->ThrowNativeError("Hook type not valid for this type of entity (%i).", entity);
			}
			else
			{
				pContext->ThrowNativeError("Hook type not valid for this type of entity (%i/%s)", entity, pClassname);
			}
			
			break;
		}
	}

	return 0;
}

cell_t Native_HookEx(IPluginContext *pContext, const cell_t *params)
{
	int entity = (int)params[1];
	SDKHookType type = (SDKHookType)params[2];
	IPluginFunction *callback = pContext->GetFunctionById(params[3]);
	HookReturn ret = g_Interface.Hook(entity, type, callback);
	if (ret == HookRet_Successful)
		return true;

	return false;
}

cell_t Native_Unhook(IPluginContext *pContext, const cell_t *params)
{
	int entity = (int)params[1];
	SDKHookType type = (SDKHookType)params[2];
	IPluginFunction *callback = pContext->GetFunctionById(params[3]);
	g_Interface.Unhook(entity, type, callback);

	return 0;
}

cell_t Native_TakeDamage(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pVictim = gamehelpers->ReferenceToEntity(params[1]);
	if (!pVictim)
		return pContext->ThrowNativeError("Invalid entity index %d for victim", params[1]);
	
	CBaseEntity *pInflictor = gamehelpers->ReferenceToEntity(params[2]);
	if (!pInflictor)
		return pContext->ThrowNativeError("Invalid entity index %d for inflictor", params[2]);

	CBaseEntity *pAttacker;
	if (params[3] != -1)
	{
		pAttacker = gamehelpers->ReferenceToEntity(params[3]);
		if (!pAttacker)
		{
			return pContext->ThrowNativeError("Invalid entity index %d for attackerr", params[3]);
		}
	}
	else
	{
		pAttacker = NULL;
	}

	float flDamage = sp_ctof(params[4]);
	int iDamageType = params[5];

	CBaseEntity *pWeapon;
	if (params[6] != -1)
	{
		pWeapon = gamehelpers->ReferenceToEntity(params[6]);
		if (!pWeapon)
		{
			return pContext->ThrowNativeError("Invalid entity index %d for weapon", params[6]);
		}
	}
	else
	{
		pWeapon = NULL;
	}

	cell_t *addr;
	int err;
	if ((err = pContext->LocalToPhysAddr(params[7], &addr)) != SP_ERROR_NONE)
	{
		return pContext->ThrowNativeError("Could not read damageForce vector");
	}

	Vector vecDamageForce;
	if (addr != pContext->GetNullRef(SP_NULL_VECTOR))
	{
		vecDamageForce.Init(sp_ctof(addr[0]), sp_ctof(addr[1]), sp_ctof(addr[2]));
	}
	else
	{
		vecDamageForce.Init();
	}

	if ((err = pContext->LocalToPhysAddr(params[8], &addr)) != SP_ERROR_NONE)
	{
		return pContext->ThrowNativeError("Could not read damagePosition vector");
	}

	Vector vecDamagePosition;
	if (addr != pContext->GetNullRef(SP_NULL_VECTOR))
	{
		vecDamagePosition.Init(sp_ctof(addr[0]), sp_ctof(addr[1]), sp_ctof(addr[2]));
	}
	else
	{
		 vecDamagePosition = vec3_origin;
	}

	CTakeDamageInfoHack info(pInflictor, pAttacker, flDamage, iDamageType, pWeapon, vecDamageForce, vecDamagePosition);

	if (params[0] < 9 || params[9] != 0)
	{
		SH_MCALL(pVictim, OnTakeDamage)((CTakeDamageInfoHack&)info);
	}
	else
	{
		static ICallWrapper *pCall = nullptr;
		if (!pCall)
		{
			int offset;
			if (!g_pGameConf->GetOffset("OnTakeDamage", &offset))
			{
				return pContext->ThrowNativeError("Could not find OnTakeDamage offset");
			}			

			PassInfo pass[2];
			pass[0].type = PassType_Object;
			pass[0].size = sizeof(CTakeDamageInfoHack &);
			pass[0].flags = PASSFLAG_BYREF | PASSFLAG_OCTOR;
			pass[1].type = PassType_Basic;
			pass[1].size = sizeof(int);
			pass[1].flags = PASSFLAG_BYVAL;

			pCall = g_pBinTools->CreateVCall(offset, 0, 0, &pass[1], &pass[0], 1);
		}

		// Can't ArgBuffer here until we upgrade our Clang version on the Linux builder
		unsigned char vstk[sizeof(CBaseEntity *) + sizeof(CTakeDamageInfoHack &)];
		unsigned char* vptr = vstk;

		*(CBaseEntity **)vptr = pVictim;
		vptr += sizeof(CBaseEntity *);
		*(CTakeDamageInfoHack *&)vptr = info;

		int ret;
		pCall->Execute(vstk, &ret);
	}

	return 0;
}

cell_t Native_DropWeapon(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pPlayer = gamehelpers->ReferenceToEntity(params[1]);
	if (!pPlayer)
		return pContext->ThrowNativeError("Invalid client index %d", params[1]);

	IGamePlayer *pGamePlayer = playerhelpers->GetGamePlayer(gamehelpers->ReferenceToIndex(params[1]));
	if (!pGamePlayer || !pGamePlayer->IsInGame())
		return pContext->ThrowNativeError("Client index %d not in game", params[1]);
	
	CBaseEntity *pWeapon = gamehelpers->ReferenceToEntity(params[2]);
	if (!pWeapon)
		return pContext->ThrowNativeError("Invalid entity index %d for weapon", params[2]);

	IServerUnknown *pUnk = (IServerUnknown *)pWeapon;
	IServerNetworkable *pNet = pUnk->GetNetworkable();

	if (!UTIL_ContainsDataTable(pNet->GetServerClass()->m_pTable, "DT_BaseCombatWeapon"))
		return pContext->ThrowNativeError("Entity index %d is not a weapon", params[2]);

	sm_sendprop_info_t spi;
	if (!gamehelpers->FindSendPropInfo("CBaseCombatWeapon", "m_hOwnerEntity", &spi))
		return pContext->ThrowNativeError("Invalid entity index %d for weapon", params[2]);

	CBaseHandle &hndl = *(CBaseHandle *)((uint8_t *)pWeapon + spi.actual_offset);
	if (params[1] != hndl.GetEntryIndex())
		return pContext->ThrowNativeError("Weapon %d is not owned by client %d", params[2], params[1]);

	Vector vecTarget;
	cell_t *addr;
	int err;
	if ((err = pContext->LocalToPhysAddr(params[3], &addr)) != SP_ERROR_NONE)
	{
		return pContext->ThrowNativeError("Could not read vecTarget vector");
	}

	if (addr != pContext->GetNullRef(SP_NULL_VECTOR))
	{
		vecTarget = Vector(
			sp_ctof(addr[0]),
			sp_ctof(addr[1]),
			sp_ctof(addr[2]));
	}
	else
	{
		SH_MCALL(pPlayer, Weapon_Drop)((CBaseCombatWeapon *)pWeapon, NULL, NULL);
		return 0;
	}

	Vector vecVelocity;
	Vector *pVecVelocity = nullptr;
	if ((err = pContext->LocalToPhysAddr(params[4], &addr)) != SP_ERROR_NONE)
	{
		return pContext->ThrowNativeError("Could not read vecVelocity vector");
	}

	if (addr != pContext->GetNullRef(SP_NULL_VECTOR))
	{
		vecVelocity = Vector(
			sp_ctof(addr[0]),
			sp_ctof(addr[1]),
			sp_ctof(addr[2]));
		pVecVelocity = &vecVelocity;
	}

	if (params[0] < 5 || params[5] != 0)
	{
		SH_MCALL(pPlayer, Weapon_Drop)((CBaseCombatWeapon*)pWeapon, &vecTarget, pVecVelocity);
	}
	else
	{
		static ICallWrapper* pCall = nullptr;
		if (!pCall)
		{
			int offset;
			if (!g_pGameConf->GetOffset("Weapon_Drop", &offset))
			{
				return pContext->ThrowNativeError("Could not find Weapon_Drop offset");
			}

			PassInfo pass[3];
			pass[0].type = PassType_Basic;
			pass[0].size = sizeof(CBaseEntity *);
			pass[0].flags = PASSFLAG_BYVAL;
			pass[1].type = PassType_Basic;
			pass[1].size = sizeof(Vector *);
			pass[1].flags = PASSFLAG_BYVAL;
			pass[2].type = PassType_Basic;
			pass[2].size = sizeof(Vector *);
			pass[2].flags = PASSFLAG_BYVAL;

			pCall = g_pBinTools->CreateVCall(offset, 0, 0, nullptr, pass, 3);
		}

		pCall->Execute(ArgBuffer<CBaseEntity *, CBaseEntity *, Vector *, Vector *>(pPlayer, pWeapon, &vecTarget, pVecVelocity), nullptr);
	}

	return 0;
}
