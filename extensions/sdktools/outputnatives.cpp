/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod SDKTools Extension
 * Copyright (C) 2004-2017 AlliedModders LLC.  All rights reserved.
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
#include "variant-t.h"
#include "output.h"
#include <sm_argbuffer.h>

ICallWrapper *g_pFireOutput = NULL;

#define ENTINDEX_TO_CBASEENTITY(ref, buffer) \
	buffer = gamehelpers->ReferenceToEntity(ref); \
	if (!buffer) \
	{ \
		return pContext->ThrowNativeError("Entity %d (%d) is not a CBaseEntity", gamehelpers->ReferenceToIndex(ref), ref); \
	}

// HookSingleEntityOutput(int ent, const char[] output, EntityOutput function, bool once);
cell_t HookSingleEntityOutput(IPluginContext *pContext, const cell_t *params)
{
	if (!g_OutputManager.IsEnabled())
	{
		return pContext->ThrowNativeError("Entity Outputs are disabled - See error logs for details");
	}

	CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(params[1]);
	if (!pEntity)
	{
		return pContext->ThrowNativeError("Invalid Entity index %i (%i)", gamehelpers->ReferenceToIndex(params[1]), params[1]);
	}

	const char *classname = gamehelpers->GetEntityClassname(pEntity);

	char *outputname;
	pContext->LocalToString(params[2], &outputname);

	OutputNameStruct *pOutputName = g_OutputManager.FindOutputPointer((const char *)classname, outputname, true);

	//Check for an existing identical hook
	SourceHook::List<omg_hooks *>::iterator _iter;

	omg_hooks *hook;

	IPluginFunction *pFunction;
	pFunction = pContext->GetFunctionById(params[3]);

	for (_iter=pOutputName->hooks.begin(); _iter!=pOutputName->hooks.end(); _iter++)
	{
		hook = (omg_hooks *)*_iter;
		if (hook->pf == pFunction && hook->entity_ref == gamehelpers->EntityToReference(pEntity))
		{
			return 0;
		}
	}

	hook = g_OutputManager.NewHook();

	hook->entity_ref = gamehelpers->EntityToReference(pEntity);
	hook->only_once= !!params[4];
	hook->pf = pFunction;
	hook->m_parent = pOutputName;
	hook->in_use = false;
	hook->delete_me = false;

	pOutputName->hooks.push_back(hook);

	g_OutputManager.OnHookAdded();

	IPlugin *pPlugin = plsys->FindPluginByContext(pContext->GetContext());
	SourceHook::List<omg_hooks *> *pList = NULL;

	if (!pPlugin->GetProperty("OutputHookList", (void **)&pList, false) || !pList)
	{
		pList = new SourceHook::List<omg_hooks *>;
		pPlugin->SetProperty("OutputHookList", pList);
	}

	pList->push_back(hook);

	return 1;
}

// HookEntityOutput(const char[] classname, const char[] output, EntityOutput function);
cell_t HookEntityOutput(IPluginContext *pContext, const cell_t *params)
{
	if (!g_OutputManager.IsEnabled())
	{
		return pContext->ThrowNativeError("Entity Outputs are disabled - See error logs for details");
	}

	//Find or create the base structures for this classname and the output
	char *classname;
	pContext->LocalToString(params[1], &classname);

	char *outputname;
	pContext->LocalToString(params[2], &outputname);

	OutputNameStruct *pOutputName = g_OutputManager.FindOutputPointer((const char *)classname, outputname, true);

	//Check for an existing identical hook
	SourceHook::List<omg_hooks *>::iterator _iter;

	omg_hooks *hook;

	IPluginFunction *pFunction;
	pFunction = pContext->GetFunctionById(params[3]);

	for (_iter=pOutputName->hooks.begin(); _iter!=pOutputName->hooks.end(); _iter++)
	{
		hook = (omg_hooks *)*_iter;
		if (hook->pf == pFunction && hook->entity_ref == -1)
		{
			//already hooked to this function...
			//throw an error or just let them get away with stupidity?
			// seems like poor coding if they dont know if something is hooked or not
			return 0;
		}
	}

	hook = g_OutputManager.NewHook();

	hook->entity_ref = -1;
	hook->pf = pFunction;
	hook->m_parent = pOutputName;
	hook->in_use = false;
	hook->delete_me = false;

	pOutputName->hooks.push_back(hook);

	g_OutputManager.OnHookAdded();

	IPlugin *pPlugin = plsys->FindPluginByContext(pContext->GetContext());
	SourceHook::List<omg_hooks *> *pList = NULL;

	if (!pPlugin->GetProperty("OutputHookList", (void **)&pList, false) || !pList)
	{
		pList = new SourceHook::List<omg_hooks *>;
		pPlugin->SetProperty("OutputHookList", pList);
	}

	pList->push_back(hook);

	return 1;
}

// UnHookEntityOutput(const char[] classname, const char[] output, EntityOutput callback);
cell_t UnHookEntityOutput(IPluginContext *pContext, const cell_t *params)
{
	if (!g_OutputManager.IsEnabled())
	{
		return pContext->ThrowNativeError("Entity Outputs are disabled - See error logs for details");
	}

	char *classname;
	pContext->LocalToString(params[1], &classname);

	char *outputname;
	pContext->LocalToString(params[2], &outputname);

	OutputNameStruct *pOutputName = g_OutputManager.FindOutputPointer((const char *)classname, outputname, false);

	if (!pOutputName)
	{
		return 0;
	}

	//Check for an existing identical hook
	SourceHook::List<omg_hooks *>::iterator _iter;

	omg_hooks *hook;

	IPluginFunction *pFunction;
	pFunction = pContext->GetFunctionById(params[3]);

	for (_iter=pOutputName->hooks.begin(); _iter!=pOutputName->hooks.end(); _iter++)
	{
		hook = (omg_hooks *)*_iter;
		if (hook->pf == pFunction && hook->entity_ref == -1)
		{
			// remove this hook.
			if (hook->in_use)
			{
				hook->delete_me = true;
				return 1;
			}

			pOutputName->hooks.erase(_iter);
			g_OutputManager.CleanUpHook(hook);

			return 1;
		}
	}

	return 0;
}

// UnHookSingleEntityOutput(int entity, const char[] output, EntityOutput callback);
cell_t UnHookSingleEntityOutput(IPluginContext *pContext, const cell_t *params)
{
	if (!g_OutputManager.IsEnabled())
	{
		return pContext->ThrowNativeError("Entity Outputs are disabled - See error logs for details");
	}

	// Find the classname of the entity and lookup the classname and output structures
	CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(params[1]);
	if (!pEntity)
	{
		return pContext->ThrowNativeError("Invalid Entity index %i (%i)", gamehelpers->ReferenceToIndex(params[1]), params[1]);
	}

	const char *classname = gamehelpers->GetEntityClassname(pEntity);

	char *outputname;
	pContext->LocalToString(params[2], &outputname);

	OutputNameStruct *pOutputName = g_OutputManager.FindOutputPointer((const char *)classname, outputname, false);

	if (!pOutputName)
	{
		return 0;
	}

	//Check for an existing identical hook
	SourceHook::List<omg_hooks *>::iterator _iter;

	omg_hooks *hook;

	IPluginFunction *pFunction;
	pFunction = pContext->GetFunctionById(params[3]);

	for (_iter=pOutputName->hooks.begin(); _iter!=pOutputName->hooks.end(); _iter++)
	{
		hook = (omg_hooks *)*_iter;
		/* We're not serial checking and just removing by index here - This was always allowed so is left for bcompat */
		if (hook->pf == pFunction && gamehelpers->ReferenceToIndex(hook->entity_ref) == gamehelpers->ReferenceToIndex(params[1]))
		{
			// remove this hook.
			if (hook->in_use)
			{
				hook->delete_me = true;
				return 1;
			}

			pOutputName->hooks.erase(_iter);
			g_OutputManager.CleanUpHook(hook);

			return 1;
		}
	}

	return 0;
}

void *FindOutputPointerByName(CBaseEntity *pEntity, const char *outputname)
{
	datamap_t *pMap = gamehelpers->GetDataMap(pEntity);

	while (pMap)
	{
		for (int i=0; i<pMap->dataNumFields; i++)
		{
			if (pMap->dataDesc[i].flags & FTYPEDESC_OUTPUT)
			{
				if (strcmp(pMap->dataDesc[i].externalName,outputname) == 0)
				{
					return reinterpret_cast<void *>((unsigned char*)pEntity + GetTypeDescOffs(&pMap->dataDesc[i]));
				}
			}
		}
		pMap = pMap->baseMap;
	}
	return NULL;
}

// FireEntityOutput(int ent, const char[] output, int activator, float delay);
static cell_t FireEntityOutput(IPluginContext *pContext, const cell_t *params)
{
	if (!g_pFireOutput)
	{
		void *addr;
		if (!g_pGameConf->GetMemSig("FireOutput", &addr) || !addr)
		{
			return pContext->ThrowNativeError("\"FireEntityOutput\" not supported by this mod");
		}
#ifdef PLATFORM_WINDOWS
		int iMaxParam = 8;
		//Instead of being one param, MSVC broke variant_t param into 5 params..
		PassInfo pass[8];
		pass[0].flags = PASSFLAG_BYVAL;
		pass[0].type = PassType_Basic;
		pass[0].size = sizeof(int);
		pass[1].flags = PASSFLAG_BYVAL;
		pass[1].type = PassType_Basic;
		pass[1].size = sizeof(int);
		pass[2].flags = PASSFLAG_BYVAL;
		pass[2].type = PassType_Basic;
		pass[2].size = sizeof(int);
		pass[3].flags = PASSFLAG_BYVAL;
		pass[3].type = PassType_Basic;
		pass[3].size = sizeof(int);
		pass[4].flags = PASSFLAG_BYVAL;
		pass[4].type = PassType_Basic;
		pass[4].size = sizeof(int);
		pass[5].flags = PASSFLAG_BYVAL;
		pass[5].type = PassType_Basic;
		pass[5].size = sizeof(CBaseEntity *);
		pass[6].flags = PASSFLAG_BYVAL;
		pass[6].type = PassType_Basic;
		pass[6].size = sizeof(CBaseEntity *);
		pass[7].flags = PASSFLAG_BYVAL;
		pass[7].type = PassType_Float;
		pass[7].size = sizeof(float);
#else
		int iMaxParam = 4;

		PassInfo pass[4];
		pass[0].type = PassType_Object;
		pass[0].flags = PASSFLAG_BYVAL|PASSFLAG_OCTOR|PASSFLAG_ODTOR|PASSFLAG_OASSIGNOP;
		pass[0].size = SIZEOF_VARIANT_T;
		pass[1].flags = PASSFLAG_BYVAL;
		pass[1].type = PassType_Basic;
		pass[1].size = sizeof(CBaseEntity *);
		pass[2].flags = PASSFLAG_BYVAL;
		pass[2].type = PassType_Basic;
		pass[2].size = sizeof(CBaseEntity *);
		pass[3].flags = PASSFLAG_BYVAL;
		pass[3].type = PassType_Float;
		pass[3].size = sizeof(float);
#endif
		if (!(g_pFireOutput = g_pBinTools->CreateCall(addr, CallConv_ThisCall, NULL, pass, iMaxParam)))
		{
			return pContext->ThrowNativeError("\"FireEntityOutput\" wrapper failed to initialize.");
		}
	}

	CBaseEntity *pActivator, *pCaller;
	void *pOutput = NULL;
	
	char *outputname;
	ENTINDEX_TO_CBASEENTITY(params[1], pCaller);
	pContext->LocalToString(params[2], &outputname);
	
	if ((pOutput = FindOutputPointerByName(pCaller,outputname)))
	{
		if (params[3] == -1)
		{
			pActivator = NULL;
		}
		else
		{
			ENTINDEX_TO_CBASEENTITY(params[3], pActivator);
		}

		ArgBuffer<void*, decltype(g_Variant_t), CBaseEntity*, CBaseEntity*, float> vstk(pOutput, g_Variant_t, pActivator, pCaller, sp_ctof(params[4]));

		g_pFireOutput->Execute(vstk, nullptr);

		_init_variant_t();
		return 1;
	}
	return pContext->ThrowNativeError("Couldn't find %s output on %i entity!", outputname, params[1]);
}

sp_nativeinfo_t g_EntOutputNatives[] =
{
	{"HookEntityOutput",			HookEntityOutput},
	{"UnhookEntityOutput",			UnHookEntityOutput},
	{"HookSingleEntityOutput",		HookSingleEntityOutput},
	{"UnhookSingleEntityOutput",	UnHookSingleEntityOutput},
	{"FireEntityOutput",			FireEntityOutput},
	{NULL,							NULL},
};
