/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod SDKTools Extension
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
#include "output.h"

ISourcePawnEngine *spengine = NULL;
EntityOutputManager g_OutputManager;

EntityOutputManager::EntityOutputManager()
{
	info_address = NULL;
	info_callback = NULL;
	HookCount = 0;
	is_detoured = false;
	enabled = false;
}

EntityOutputManager::~EntityOutputManager()
{
	if (!enabled)
	{
		return;
	}

	EntityOutputs->Destroy();
	ClassNames->Destroy();
	ShutdownFireEventDetour();
}

void EntityOutputManager::Init()
{
	enabled = CreateFireEventDetour();

	if (!enabled)
	{
		return;
	}

	EntityOutputs = adtfactory->CreateBasicTrie();
	ClassNames = adtfactory->CreateBasicTrie();
}

bool EntityOutputManager::IsEnabled()
{
	return enabled;
}

bool EntityOutputManager::CreateFireEventDetour()
{
	if (!g_pGameConf->GetMemSig("FireOutput", &info_address) || !info_address)
	{
		g_pSM->LogError(myself, "Could not locate FireOutput - Disabling Entity Outputs");
		return false;
	}

	if (!g_pGameConf->GetOffset("FireOutputBackup", (int *)&(info_restore.bytes)))
	{
		g_pSM->LogError(myself, "Could not locate FireOutputBackup - Disabling Entity Outputs");
		return false;
	}

	/* First, save restore bits */
	for (size_t i=0; i<info_restore.bytes; i++)
	{
		info_restore.patch[i] = ((unsigned char *)info_address)[i];
	}

	info_callback = spengine->ExecAlloc(100);
	JitWriter wr;
	JitWriter *jit = &wr;
	wr.outbase = (jitcode_t)info_callback;
	wr.outptr = wr.outbase;

	/* Function we are detouring into is
	 *
	 * void FireEventDetour(CBaseEntityOutput(void *) *pOutput, CBaseEntity *pActivator, CBaseEntity *pCaller, float fDelay = 0 )
	 */

	/* push		fDelay [esp+20h]
	 * push		pCaller [esp+1Ch]
	 * push		pActivator [esp+18h]
	 * push		pOutput [ecx]
	 */


#if defined PLATFORM_WINDOWS

	IA32_Push_Rm_Disp8_ESP(jit, 32);
	IA32_Push_Rm_Disp8_ESP(jit, 32);
	IA32_Push_Rm_Disp8_ESP(jit, 32);

	IA32_Push_Reg(jit, REG_ECX);

#elif defined PLATFORM_LINUX
	IA32_Push_Rm_Disp8_ESP(jit, 20);
	IA32_Push_Rm_Disp8_ESP(jit, 20);
	IA32_Push_Rm_Disp8_ESP(jit, 20);

	IA32_Push_Rm_Disp8_ESP(jit, 16);
#endif

	jitoffs_t call = IA32_Call_Imm32(jit, 0); 
	IA32_Write_Jump32_Abs(jit, call, (void *)TempDetour);

	
#if defined PLATFORM_LINUX
	IA32_Add_Rm_Imm8(jit, REG_ESP, 4, MOD_REG);		//add esp, 4
#elif defined PLATFORM_WINDOWS
	IA32_Pop_Reg(jit, REG_ECX);
#endif

	IA32_Add_Rm_Imm8(jit, REG_ESP, 12, MOD_REG);	//add esp, 12 (0Ch)


	/* Patch old bytes in */
	for (size_t i=0; i<info_restore.bytes; i++)
	{
		jit->write_ubyte(info_restore.patch[i]);
	}

	/* Return to the original function */
	call = IA32_Jump_Imm32(jit, 0);
	IA32_Write_Jump32_Abs(jit, call, (unsigned char *)info_address + info_restore.bytes);

	return true;
}

void EntityOutputManager::InitFireEventDetour()
{
	if (!is_detoured)
	{
		DoGatePatch((unsigned char *)info_address, &info_callback);
		is_detoured = true;
	}
}

void EntityOutputManager::DeleteFireEventDetour()
{
	if (is_detoured)
	{
		ShutdownFireEventDetour();
	}

	if (info_callback)
	{
		/* Free the gate */
		spengine->ExecFree(info_callback);
		info_callback = NULL;
	}
}

void TempDetour(void *pOutput, CBaseEntity *pActivator, CBaseEntity *pCaller, float fDelay)
{
	g_OutputManager.FireEventDetour(pOutput, pActivator, pCaller, fDelay);
}

void EntityOutputManager::ShutdownFireEventDetour()
{
	if (info_callback)
	{
		/* Remove the patch */
		ApplyPatch(info_address, 0, &info_restore, NULL);
		is_detoured = false;
	}
}

void EntityOutputManager::FireEventDetour(void *pOutput, CBaseEntity *pActivator, CBaseEntity *pCaller, float fDelay)
{
	char sOutput[20];
	Q_snprintf(sOutput, sizeof(sOutput), "%x", pOutput);

	// attempt to directly lookup a hook using the pOutput pointer
	OutputNameStruct *pOutputName = NULL;

	edict_t *pEdict = gameents->BaseEntityToEdict(pCaller);

	/* TODO: Add support for entities without an edict */
	if (pEdict == NULL)
	{
		return;
	}

	bool fastLookup = false;
	
	// Fast lookup failed - check the slow way for hooks that havn't fired yet
	if ((fastLookup = EntityOutputs->Retrieve(sOutput, (void **)&pOutputName)) == false)
	{
		const char *classname = pEdict->GetClassName();
		const char *outputname = FindOutputName(pOutput, pCaller);

		pOutputName = FindOutputPointer(classname, outputname, false);

		if (!pOutputName)
		{
			return;
		}
	}

	if (!pOutputName->hooks.empty())
	{
		if (!fastLookup)
		{
			// hook exists on this classname and output - map it into our quick find trie
			EntityOutputs->Insert(sOutput, pOutputName);
		}

		SourceHook::List<omg_hooks *>::iterator _iter;

		omg_hooks *hook;

		_iter = pOutputName->hooks.begin();

		while (_iter != pOutputName->hooks.end())
		{
			hook = (omg_hooks *)*_iter;

			hook->in_use = true;

			int serial = pEdict->m_NetworkSerialNumber;
			
			if (serial != hook->entity_filter && hook->entity_index == engine->IndexOfEdict(pEdict))
			{
				// same entity index but different serial number. Entity has changed, kill the hook.
				_iter = pOutputName->hooks.erase(_iter);
				CleanUpHook(hook);

				continue;
			}

			if (hook->entity_filter == -1 || hook->entity_filter == serial) // Global classname hook
			{
				//fire the forward to hook->pf
				hook->pf->PushString(pOutputName->Name);
				hook->pf->PushCell(engine->IndexOfEdict(pEdict));
				
				edict_t *pEdictActivator = gameents->BaseEntityToEdict(pActivator);
				if (!pEdictActivator)
				{
					hook->pf->PushCell(-1);
				}
				else
				{
					hook->pf->PushCell(engine->IndexOfEdict(pEdictActivator));
				}
				//hook->pf->PushCell(handle);
				hook->pf->PushFloat(fDelay);
				hook->pf->Execute(NULL);

				if ((hook->entity_filter != -1) && hook->only_once)
				{
					_iter = pOutputName->hooks.erase(_iter);
					CleanUpHook(hook);

					continue;
				}

				if (hook->delete_me)
				{
					_iter = pOutputName->hooks.erase(_iter);
					CleanUpHook(hook);
					continue;
				}

				hook->in_use = false;
				_iter++;

				continue;
			}
		}
	}
}

omg_hooks *EntityOutputManager::NewHook()
{
	omg_hooks *hook;

	if (FreeHooks.empty())
	{
		hook = new omg_hooks;
	}
	else
	{
		hook = g_OutputManager.FreeHooks.front();
		g_OutputManager.FreeHooks.pop();
	}

	return hook;
}

void EntityOutputManager::OnHookAdded()
{
	HookCount++;

	if (HookCount == 1)
	{
		// This is the first hook created
		InitFireEventDetour();
	}
}

void EntityOutputManager::OnHookRemoved()
{
	HookCount--;

	if (HookCount == 0)
	{
		ShutdownFireEventDetour();
	}
}

void EntityOutputManager::CleanUpHook(omg_hooks *hook)
{
	FreeHooks.push(hook);

	OnHookRemoved();

	IPlugin *pPlugin = plsys->FindPluginByContext(hook->pf->GetParentContext()->GetContext());
	SourceHook::List<omg_hooks *> *pList = NULL;

	if (!pPlugin->GetProperty("OutputHookList", (void **)&pList, false) || !pList)
	{
		return;
	}

	SourceHook::List<omg_hooks *>::iterator p_iter = pList->begin();

	omg_hooks *pluginHook;

	while (p_iter != pList->end())
	{
		pluginHook = (omg_hooks *)*p_iter;
		if (pluginHook == hook)
		{
			p_iter = pList->erase(p_iter);
		}
		else
		{
			p_iter++;
		}
	}
}

void EntityOutputManager::OnPluginDestroyed(IPlugin *plugin)
{
	SourceHook::List<omg_hooks *> *pList = NULL;

	if (plugin->GetProperty("OutputHookList", (void **)&pList, true))
	{
		SourceHook::List<omg_hooks *>::iterator p_iter = pList->begin();
		omg_hooks *hook;

		while (p_iter != pList->end())
		{
			hook = (omg_hooks *)*p_iter;
		
			p_iter = pList->erase(p_iter); //remove from this plugins list
			hook->m_parent->hooks.remove(hook); // remove from the y's list

			FreeHooks.push(hook); //save the omg_hook

			OnHookRemoved();
		}		
	}
}

OutputNameStruct *EntityOutputManager::FindOutputPointer(const char *classname, const char *outputname, bool create)
{
	ClassNameStruct *pClassname;
	
	if (!ClassNames->Retrieve(classname, (void **)&pClassname))
	{
		if (create)
		{
			pClassname = new ClassNameStruct;
			ClassNames->Insert(classname, pClassname);
		}
		else
		{
			return NULL;
		}
	}

	OutputNameStruct *pOutputName;

	if (!pClassname->OutputList->Retrieve(outputname, (void **)&pOutputName))
	{
		if (create)
		{
			pOutputName = new OutputNameStruct;
			pClassname->OutputList->Insert(outputname, pOutputName);
			strncpy(pOutputName->Name, outputname, sizeof(pOutputName->Name));
			pOutputName->Name[49] = 0;
		}
		else
		{
			return NULL;
		}
	}

	return pOutputName;
}

// Iterate the datamap of pCaller and look for output pointers with the same address as pOutput
const char *EntityOutputManager::FindOutputName(void *pOutput, CBaseEntity *pCaller)
{
	datamap_t *pMap = gamehelpers->GetDataMap(pCaller);

	while (pMap)
	{
		for (int i=0; i<pMap->dataNumFields; i++)
		{
			if (pMap->dataDesc[i].flags & FTYPEDESC_OUTPUT)
			{
				if ((char *)pCaller + pMap->dataDesc[i].fieldOffset[0] == pOutput)
				{
					return pMap->dataDesc[i].externalName;
				}
			}
		}
		pMap = pMap->baseMap;
	}

	return NULL;
}

// Thanks SM core
edict_t *EntityOutputManager::BaseHandleToEdict(CBaseHandle &hndl)
{
	if (!hndl.IsValid())
	{
		return NULL;
	}

	int index = hndl.GetEntryIndex();

	edict_t *pStoredEdict;

	pStoredEdict = engine->PEntityOfEntIndex(index);

	if (pStoredEdict == NULL)
	{
		return NULL;
	}

	IServerEntity *pSE = pStoredEdict->GetIServerEntity();

	if (pSE == NULL)
	{
		return NULL;
	}

	if (pSE->GetRefEHandle() != hndl)
	{
		return NULL;
	}

	return pStoredEdict;
}
