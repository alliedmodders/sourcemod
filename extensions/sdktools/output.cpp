/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod SDKTools Extension
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
 * Version: $Id$
 */

#include "extension.h"
#include "output.h"
#include "am-string.h"

ISourcePawnEngine *spengine = NULL;
EntityOutputManager g_OutputManager;
CDetour *fireOutputDetour = NULL;

EntityOutputManager::EntityOutputManager()
{
	info_address = NULL;
	info_callback = NULL;
	HookCount = 0;
	enabled = false;
}

void EntityOutputManager::Shutdown()
{
	if (!enabled)
	{
		return;
	}

	ClassNames->Destroy();
	fireOutputDetour->Destroy();
}

void EntityOutputManager::Init()
{
	enabled = CreateFireEventDetour();

	if (!enabled)
	{
		return;
	}

	ClassNames = adtfactory->CreateBasicTrie();
}

bool EntityOutputManager::IsEnabled()
{
	return enabled;
}

#ifdef PLATFORM_WINDOWS
DETOUR_DECL_MEMBER8(FireOutput, void, int, what, int, the, int, hell, int, msvc, void *, variant_t, CBaseEntity *, pActivator, CBaseEntity *, pCaller, float, fDelay)
{
	bool fireOutput = g_OutputManager.FireEventDetour((void *)this, pActivator, pCaller, fDelay);

	if (!fireOutput)
	{
		return;
	}

	DETOUR_MEMBER_CALL(FireOutput)(what, the, hell, msvc, variant_t, pActivator, pCaller, fDelay);
}
#else
DETOUR_DECL_MEMBER4(FireOutput, void, void *, variant_t, CBaseEntity *, pActivator, CBaseEntity *, pCaller, float, fDelay)
{
	bool fireOutput = g_OutputManager.FireEventDetour((void *)this, pActivator, pCaller, fDelay);

	if (!fireOutput)
	{
		return;
	}

	DETOUR_MEMBER_CALL(FireOutput)(variant_t, pActivator, pCaller, fDelay);
}
#endif

bool EntityOutputManager::CreateFireEventDetour()
{
	fireOutputDetour = DETOUR_CREATE_MEMBER(FireOutput, "FireOutput");

	if (fireOutputDetour)
	{
		return true;
	}

	return false;
}

bool EntityOutputManager::FireEventDetour(void *pOutput, CBaseEntity *pActivator, CBaseEntity *pCaller, float fDelay)
{
	if (!pCaller)
	{
		return true;
	}

	// attempt to directly lookup a hook using the pOutput pointer
	OutputNameStruct *pOutputName = NULL;

	const char *classname;
	const char *outputname = FindOutputName(pOutput, pActivator, pCaller, &classname);

	if (!outputname || !classname)
	{
		return true;
	}

	pOutputName = FindOutputPointer(classname, outputname, false);

	if (!pOutputName)
	{
		return true;
	}

	if (!pOutputName->hooks.empty())
	{
		SourceHook::List<omg_hooks *>::iterator _iter;

		omg_hooks *hook;

		_iter = pOutputName->hooks.begin();

		// by default we'll call the game's output func, unless a plugin overrides it
		bool fireOriginal = true;

		while (_iter != pOutputName->hooks.end())
		{
			hook = (omg_hooks *)*_iter;

			hook->in_use = true;

			cell_t ref = gamehelpers->EntityToReference(pCaller);
			
			if (hook->entity_ref != -1 
					&& gamehelpers->ReferenceToIndex(hook->entity_ref) == gamehelpers->ReferenceToIndex(ref)
					&& ref != hook->entity_ref)
			{
				// same entity index but different reference. Entity has changed, kill the hook.
				_iter = pOutputName->hooks.erase(_iter);
				CleanUpHook(hook);

				continue;
			}

			if (hook->entity_ref == -1 || hook->entity_ref == ref) // Global classname hook
			{
				cell_t result;

				//fire the forward to hook->pf
				hook->pf->PushString(pOutputName->Name);
				hook->pf->PushCell(gamehelpers->ReferenceToBCompatRef(ref));
				hook->pf->PushCell(gamehelpers->EntityToBCompatRef(pActivator));

				//hook->pf->PushCell(handle);
				hook->pf->PushFloat(fDelay);
				hook->pf->Execute(&result);

				if (result > Pl_Continue)
				{
					// a hook doesn't want the output to be called
					fireOriginal = false;
				}

				if ((hook->entity_ref != -1) && hook->only_once)
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
			}

			hook->in_use = false;
			_iter++;
		}

		return fireOriginal;
	}

	return true;
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
		fireOutputDetour->EnableDetour();
	}
}

void EntityOutputManager::OnHookRemoved()
{
	HookCount--;

	if (HookCount == 0)
	{
		fireOutputDetour->DisableDetour();
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

// Iterate the datamap of pCaller/pActivator and look for output pointers with the same address as pOutput.
// Store the classname of the entity we found the output on in |entity_classname| if provided.
//
// TODO: It turns out this logic isn't very sane, and it relies heavily on convention how most entities call
//       FireOutput rather than explicitly conforming to the design of the engine's output system. We need a
//       big refactor here to lookup the underlying per-entity CBaseEntityOutput instances and introduce an
//       explicit concept of the entity owning the output being triggered, rather than assuming it is also at
//       least one of the caller or activator entity.
const char *EntityOutputManager::FindOutputName(void *pOutput, CBaseEntity *pActivator, CBaseEntity *pCaller, const char **entity_classname)
{
	datamap_t *pMap = gamehelpers->GetDataMap(pCaller);

	while (pMap)
	{
		for (int i=0; i<pMap->dataNumFields; i++)
		{
			if (pMap->dataDesc[i].flags & FTYPEDESC_OUTPUT)
			{
				if ((char *)pCaller + GetTypeDescOffs(&pMap->dataDesc[i]) == pOutput)
				{
					if (entity_classname)
					{
						*entity_classname = gamehelpers->GetEntityClassname(pCaller);
					}

					return pMap->dataDesc[i].externalName;
				}
			}
		}
		pMap = pMap->baseMap;
	}

	// HACK: Generally, the game passes the entity that triggered the output as pCaller, but occasionally (because the
	//       param order is confusing), the entity gets passed in as pActivator instead. We do a 2nd pass over
	//       pActivator looking for the output if we couldn't find it on pCaller.
	if (pActivator)
	{
		pMap = gamehelpers->GetDataMap(pActivator);

		while (pMap)
		{
			for (int i=0; i<pMap->dataNumFields; i++)
			{
				if (pMap->dataDesc[i].flags & FTYPEDESC_OUTPUT)
				{
					if ((char *)pActivator + GetTypeDescOffs(&pMap->dataDesc[i]) == pOutput)
					{
						if (entity_classname)
						{
							*entity_classname = gamehelpers->GetEntityClassname(pActivator);
						}

						return pMap->dataDesc[i].externalName;
					}
				}
			}
			pMap = pMap->baseMap;
		}
	}

	if(entity_classname)
	{
		*entity_classname = nullptr;
	}

	return NULL;
}