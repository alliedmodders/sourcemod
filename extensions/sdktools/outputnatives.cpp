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

// HookSingleEntityOutput(ent, const String:output[], function, bool:once);
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

// HookEntityOutput(const String:classname[], const String:output[], function);
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

// UnHookEntityOutput(const String:classname[], const String:output[], EntityOutput:callback);
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

// UnHookSingleEntityOutput(entity, const String:output[], EntityOutput:callback);
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

sp_nativeinfo_t g_EntOutputNatives[] =
{
	{"HookEntityOutput",			HookEntityOutput},
	{"UnhookEntityOutput",			UnHookEntityOutput},
	{"HookSingleEntityOutput",		HookSingleEntityOutput},
	{"UnhookSingleEntityOutput",	UnHookSingleEntityOutput},
	{NULL,							NULL},
};
