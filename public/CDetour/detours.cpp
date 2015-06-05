/**
* vim: set ts=4 :
* =============================================================================
* SourceMod
* Copyright (C) 2004-2010 AlliedModders LLC.  All rights reserved.
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
* Version: $Id: detours.cpp 248 2008-08-27 00:56:22Z pred $
*/

#include "detours.h"
#include <asm/asm.h>

ISourcePawnEngine *CDetourManager::spengine = NULL;
IGameConfig *CDetourManager::gameconf = NULL;

void CDetourManager::Init(ISourcePawnEngine *spengine, IGameConfig *gameconf)
{
	CDetourManager::spengine = spengine;
	CDetourManager::gameconf = gameconf;
}

CDetour *CDetourManager::CreateDetour(void *callbackfunction, void **trampoline, const char *signame)
{
	CDetour *detour = new CDetour(callbackfunction, trampoline, signame);
	if (detour)
	{
		if (!detour->Init(spengine, gameconf))
		{
			delete detour;
			return NULL;
		}

		return detour;
	}

	return NULL;
}

CDetour *CDetourManager::CreateDetour(void *callbackfunction, void **trampoline, void *pAddress)
{
	CDetour *detour = new CDetour(callbackfunction, trampoline, pAddress);
	if (detour)
	{
		if (!detour->Init(spengine, gameconf))
		{
			delete detour;
			return NULL;
		}

		return detour;
	}

	return NULL;
}

CDetour::CDetour(void *callbackfunction, void **trampoline, const char *signame)
{
	enabled = false;
	detoured = false;
	detour_address = NULL;
	detour_trampoline = NULL;
	this->signame = signame;
	this->detour_callback = callbackfunction;
	spengine = NULL;
	gameconf = NULL;
	this->trampoline = trampoline;
}

CDetour::CDetour(void*callbackfunction, void **trampoline, void *pAddress)
{
	enabled = false;
	detoured = false;
	detour_address = pAddress;
	detour_trampoline = NULL;
	this->signame = NULL;
	this->detour_callback = callbackfunction;
	spengine = NULL;
	gameconf = NULL;
	this->trampoline = trampoline;
}

bool CDetour::Init(ISourcePawnEngine *spengine, IGameConfig *gameconf)
{
	this->spengine = spengine;
	this->gameconf = gameconf;

	if (!CreateDetour())
	{
		enabled = false;
		return enabled;
	}

	enabled = true;

	return enabled;
}

void CDetour::Destroy()
{
	DeleteDetour();
	delete this;
}

bool CDetour::IsEnabled()
{
	return enabled;
}

bool CDetour::CreateDetour()
{
	if (signame && !gameconf->GetMemSig(signame, &detour_address))
	{
		g_pSM->LogError(myself, "Could not locate %s - Disabling detour", signame);
		return false;
	}
	else if(!detour_address)
	{
		g_pSM->LogError(myself, "Invalid detour address passed - Disabling detour to prevent crashes");
		return false;
	}

	if (!detour_address)
	{
		g_pSM->LogError(myself, "Sigscan for %s failed - Disabling detour to prevent crashes", signame);
		return false;
	}

	detour_restore.bytes = copy_bytes((unsigned char *)detour_address, NULL, OP_JMP_SIZE+1);

	/* First, save restore bits */
	for (size_t i=0; i<detour_restore.bytes; i++)
	{
		detour_restore.patch[i] = ((unsigned char *)detour_address)[i];
	}

	JitWriter wr;
	JitWriter *jit = &wr;
	jit_uint32_t CodeSize = 0;
	
	wr.outbase = NULL;
	wr.outptr = NULL;

jit_rewind:

	/* Patch old bytes in */
	if (wr.outbase != NULL)
	{
		copy_bytes((unsigned char *)detour_address, (unsigned char*)wr.outptr, detour_restore.bytes);
	}
	wr.outptr += detour_restore.bytes;

	/* Return to the original function */
	jitoffs_t call = IA32_Jump_Imm32(jit, 0);
	IA32_Write_Jump32_Abs(jit, call, (unsigned char *)detour_address + detour_restore.bytes);

	if (wr.outbase == NULL)
	{
		CodeSize = wr.get_outputpos();
		wr.outbase = (jitcode_t)spengine->AllocatePageMemory(CodeSize);
		spengine->SetReadWrite(wr.outbase);
		wr.outptr = wr.outbase;
		detour_trampoline = wr.outbase;
		goto jit_rewind;
	}

	spengine->SetReadExecute(wr.outbase);

	*trampoline = detour_trampoline;

	return true;
}

void CDetour::DeleteDetour()
{
	if (detoured)
	{
		DisableDetour();
	}

	if (detour_trampoline)
	{
		/* Free the allocated trampoline memory */
		spengine->FreePageMemory(detour_trampoline);
		detour_trampoline = NULL;
	}
}

void CDetour::EnableDetour()
{
	if (!detoured)
	{
		DoGatePatch((unsigned char *)detour_address, &detour_callback);
		detoured = true;
	}
}

void CDetour::DisableDetour()
{
	if (detoured)
	{
		/* Remove the patch */
		ApplyPatch(detour_address, 0, &detour_restore, NULL);
		detoured = false;
	}
}
