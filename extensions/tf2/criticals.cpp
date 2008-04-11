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

#include "criticals.h"

ISourcePawnEngine *spengine = NULL;
CriticalHitManager g_CriticalHitManager;
IServerGameEnts *gameents = NULL;

int g_returnvalue;

bool CriticalHitManager::CreateCriticalDetour()
{
	if (!g_pGameConf->GetMemSig("CalcCritical", &critical_address) || !critical_address)
	{
		g_pSM->LogError(myself, "Could not locate CalcCritical - Disabling Critical Hit forward");
		return false;
	}

	if (!g_pGameConf->GetOffset("CalcCriticalBackup", (int *)&(critical_restore.bytes)))
	{
		g_pSM->LogError(myself, "Could not locate CalcCriticalBackup - Disabling Critical Hit forward");
		return false;
	}

	/* First, save restore bits */
	for (size_t i=0; i<critical_restore.bytes; i++)
	{
		critical_restore.patch[i] = ((unsigned char *)critical_address)[i];
	}

	critical_callback = spengine->ExecAlloc(100);
	JitWriter wr;
	JitWriter *jit = &wr;
	wr.outbase = (jitcode_t)critical_callback;
	wr.outptr = wr.outbase;

	/* Function we are detouring into is
	 *
	 * void CriticalDetour(CTFWeaponBase(void *) *pWeapon)
	 *
	 * push		pWeapon [ecx]
	 */

#if defined PLATFORM_WINDOWS
	IA32_Push_Reg(jit, REG_ECX);
#elif defined PLATFORM_LINUX
	IA32_Push_Rm_Disp8_ESP(jit, 4);
#endif

	jitoffs_t call = IA32_Call_Imm32(jit, 0); 
	IA32_Write_Jump32_Abs(jit, call, (void *)TempDetour);

	
#if defined PLATFORM_LINUX
	IA32_Add_Rm_Imm8(jit, REG_ESP, 4, MOD_REG);		//add esp, 4
#elif defined PLATFORM_WINDOWS
	IA32_Pop_Reg(jit, REG_ECX);
#endif

	//If TempDetour returns non-zero we want to load something into eax and return this value

	//IA32_Test_Rm_Reg(jit, eax, eax, something);
	jit->write_ubyte(0x85);
	jit->write_ubyte(0xC0);

	//JNZ critical_callback+50
	jit->write_ubyte(0x75); 
	jit->write_ubyte(50-((jit->outptr+1)-jit->outbase));

	/* Patch old bytes in */
	for (size_t i=0; i<critical_restore.bytes; i++)
	{
		jit->write_ubyte(critical_restore.patch[i]);
	}

	/* Return to the original function */
	call = IA32_Jump_Imm32(jit, 0);
	IA32_Write_Jump32_Abs(jit, call, (unsigned char *)critical_address + critical_restore.bytes);
	
	wr.outbase = (jitcode_t)critical_callback+50;
	wr.outptr = wr.outbase;

	//copy g_returnvalue into eax
	jit->write_ubyte(0xA1);
	jit->write_uint32((jit_uint32_t)&g_returnvalue);

	IA32_Return(jit);

	return true;
}

bool CriticalHitManager::CreateCriticalMeleeDetour()
{
	if (!g_pGameConf->GetMemSig("CalcCriticalMelee", &melee_address) || !melee_address)
	{
		g_pSM->LogError(myself, "Could not locate CalcCriticalMelee - Disabling Critical Hit forward");
		return false;
	}

	if (!g_pGameConf->GetOffset("CalcCriticalMeleeBackup", (int *)&(melee_restore.bytes)))
	{
		g_pSM->LogError(myself, "Could not locate CalcCriticalMeleeBackup - Disabling Critical Hit forward");
		return false;
	}

	/* First, save restore bits */
	for (size_t i=0; i<melee_restore.bytes; i++)
	{
		melee_restore.patch[i] = ((unsigned char *)melee_address)[i];
	}

	melee_callback = spengine->ExecAlloc(100);
	JitWriter wr;
	JitWriter *jit = &wr;
	wr.outbase = (jitcode_t)melee_callback;
	wr.outptr = wr.outbase;

	/* Function we are detouring into is
	 *
	 * void CriticalDetour(CTFWeaponBase(void *) *pWeapon)
	 *
	 * push		pWeapon [ecx]
	 */

#if defined PLATFORM_WINDOWS
	IA32_Push_Reg(jit, REG_ECX);
#elif defined PLATFORM_LINUX
	IA32_Push_Rm_Disp8_ESP(jit, 4);
#endif

	jitoffs_t call = IA32_Call_Imm32(jit, 0); 
	IA32_Write_Jump32_Abs(jit, call, (void *)TempDetour);

	
#if defined PLATFORM_LINUX
	IA32_Add_Rm_Imm8(jit, REG_ESP, 4, MOD_REG);		//add esp, 4
#elif defined PLATFORM_WINDOWS
	IA32_Pop_Reg(jit, REG_ECX);
#endif

	//If TempDetour returns non-zero we want to load something into eax and return this value

	//IA32_Test_Rm_Reg(jit, eax, eax, something);
	jit->write_ubyte(0x85);
	jit->write_ubyte(0xC0);

	//JNZ critical_callback+50
	jit->write_ubyte(0x75); 
	jit->write_ubyte(50-((jit->outptr+1)-jit->outbase));

	/* Patch old bytes in */
	for (size_t i=0; i<melee_restore.bytes; i++)
	{
		jit->write_ubyte(melee_restore.patch[i]);
	}

	/* Return to the original function */
	call = IA32_Jump_Imm32(jit, 0);
	IA32_Write_Jump32_Abs(jit, call, (unsigned char *)melee_address + melee_restore.bytes);
	
	wr.outbase = (jitcode_t)melee_callback+50;
	wr.outptr = wr.outbase;

	//copy g_returnvalue into eax
	jit->write_ubyte(0xA1);
	jit->write_uint32((jit_uint32_t)&g_returnvalue);

	IA32_Return(jit);

	return true;
}

bool CriticalHitManager::CreateCriticalKnifeDetour()
{
	if (!g_pGameConf->GetMemSig("CalcCriticalKnife", &knife_address) || !knife_address)
	{
		g_pSM->LogError(myself, "Could not locate CalcCriticalKnife - Disabling Critical Hit forward");
		return false;
	}

	if (!g_pGameConf->GetOffset("CalcCriticalMeleeBackup", (int *)&(knife_restore.bytes)))
	{
		g_pSM->LogError(myself, "Could not locate CalcCriticalKnifeBackup - Disabling Critical Hit forward");
		return false;
	}

	/* First, save restore bits */
	for (size_t i=0; i<knife_restore.bytes; i++)
	{
		knife_restore.patch[i] = ((unsigned char *)knife_address)[i];
	}

	knife_callback = spengine->ExecAlloc(100);
	JitWriter wr;
	JitWriter *jit = &wr;
	wr.outbase = (jitcode_t)knife_callback;
	wr.outptr = wr.outbase;

	/* Function we are detouring into is
	 *
	 * void CriticalDetour(CTFWeaponBase(void *) *pWeapon)
	 *
	 * push		pWeapon [ecx]
	 */

#if defined PLATFORM_WINDOWS
	IA32_Push_Reg(jit, REG_ECX);
#elif defined PLATFORM_LINUX
	IA32_Push_Rm_Disp8_ESP(jit, 4);
#endif

	jitoffs_t call = IA32_Call_Imm32(jit, 0); 
	IA32_Write_Jump32_Abs(jit, call, (void *)TempDetour);

	
#if defined PLATFORM_LINUX
	IA32_Add_Rm_Imm8(jit, REG_ESP, 4, MOD_REG);		//add esp, 4
#elif defined PLATFORM_WINDOWS
	IA32_Pop_Reg(jit, REG_ECX);
#endif

	//If TempDetour returns non-zero we want to load something into eax and return this value

	//IA32_Test_Rm_Reg(jit, eax, eax, something);
	jit->write_ubyte(0x85);
	jit->write_ubyte(0xC0);

	//JNZ critical_callback+50
	jit->write_ubyte(0x75); 
	jit->write_ubyte(50-((jit->outptr+1)-jit->outbase));

	/* Patch old bytes in */
	for (size_t i=0; i<knife_restore.bytes; i++)
	{
		jit->write_ubyte(knife_restore.patch[i]);
	}

	/* Return to the original function */
	call = IA32_Jump_Imm32(jit, 0);
	IA32_Write_Jump32_Abs(jit, call, (unsigned char *)knife_address + knife_restore.bytes);
	
	wr.outbase = (jitcode_t)knife_callback+50;
	wr.outptr = wr.outbase;

	//copy g_returnvalue into eax
	jit->write_ubyte(0xA1);
	jit->write_uint32((jit_uint32_t)&g_returnvalue);

	IA32_Return(jit);

	return true;
}

void CriticalHitManager::EnableCriticalDetour()
{
	if (!detoured)
	{
		DoGatePatch((unsigned char *)critical_address, &critical_callback);
		DoGatePatch((unsigned char *)melee_address, &melee_callback);
		DoGatePatch((unsigned char *)knife_address, &knife_callback);
		detoured = true;
	}
}

void CriticalHitManager::DeleteCriticalDetour()
{
	if (detoured)
	{
		DisableCriticalDetour();
	}

	if (critical_callback)
	{
		/* Free the gate */
		spengine->ExecFree(critical_callback);
		critical_callback = NULL;
	}

	if (melee_callback)
	{
		/* Free the gate */
		spengine->ExecFree(melee_callback);
		melee_callback = NULL;
	}

	if (knife_callback)
	{
		/* Free the gate */
		spengine->ExecFree(knife_callback);
		knife_callback = NULL;
	}
}

bool TempDetour(void *pWeapon)
{
	return g_CriticalHitManager.CriticalDetour(pWeapon);
}

void CriticalHitManager::DisableCriticalDetour()
{
	if (critical_callback)
	{
		/* Remove the patch */
		ApplyPatch(critical_address, 0, &critical_restore, NULL);
		detoured = false;
	}

	if (melee_callback)
	{
		/* Remove the patch */
		ApplyPatch(melee_address, 0, &melee_restore, NULL);
		detoured = false;
	}

	if (melee_callback)
	{
		/* Remove the patch */
		ApplyPatch(knife_address, 0, &knife_restore, NULL);
		detoured = false;
	}
}

int CheckBaseHandle(CBaseHandle &hndl)
{
	if (!hndl.IsValid())
	{
		return -1;
	}

	int index = hndl.GetEntryIndex();

	edict_t *pStoredEdict;

	pStoredEdict = engine->PEntityOfEntIndex(index);

	if (pStoredEdict == NULL)
	{
		return -1;
	}

	IServerEntity *pSE = pStoredEdict->GetIServerEntity();

	if (pSE == NULL)
	{
		return -1;
	}

	if (pSE->GetRefEHandle() != hndl)
	{
		return -1;
	}

	return index;
}

bool CriticalHitManager::CriticalDetour(void *pWeapon)
{
	edict_t *pEdict = gameents->BaseEntityToEdict((CBaseEntity *)pWeapon);
	
	if (!pEdict)
	{
		g_pSM->LogMessage(myself, "Entity Error");
		return false;
	}

	sm_sendprop_info_t info;

	if (!gamehelpers->FindSendPropInfo(pEdict->GetNetworkable()->GetServerClass()->GetName(), "m_hOwnerEntity", &info))
	{
		g_pSM->LogMessage(myself, "Offset Error");
		return false;
	}

	if (!forward)
	{
		g_pSM->LogMessage(myself, "Invalid Forward");
		return false;
	}
	
	CBaseHandle &hndl = *(CBaseHandle *)((uint8_t *)pWeapon + info.actual_offset);
	int index = CheckBaseHandle(hndl);

	forward->PushCell(index); //Client index
	forward->PushCell(engine->IndexOfEdict(pEdict)); // Weapon index
	forward->PushString(pEdict->GetClassName()); //Weapon classname
	forward->PushCellByRef(&g_returnvalue); //return value

	cell_t result = 0;

	forward->Execute(&result);

	return !!result;
}
