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

	edict_t *pEdict = engine->PEntityOfEntIndex(params[1]);
	if (!pEdict)
	{
		return pContext->ThrowNativeError("Invalid Entity index %i", params[1]);
	}
	const char *classname = pEdict->GetClassName();

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
		if (hook->pf == pFunction && hook->entity_filter == pEdict->m_NetworkSerialNumber)
		{
			return 0;
		}
	}

	hook = g_OutputManager.NewHook();

	hook->entity_filter = pEdict->m_NetworkSerialNumber;
	hook->entity_index = engine->IndexOfEdict(pEdict);
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
		if (hook->pf == pFunction && hook->entity_filter == -1)
		{
			//already hooked to this function...
			//throw an error or just let them get away with stupidity?
			// seems like poor coding if they dont know if something is hooked or not
			return 0;
		}
	}

	hook = g_OutputManager.NewHook();

	hook->entity_filter = -1;
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
		if (hook->pf == pFunction && hook->entity_filter == -1)
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
	edict_t *pEdict = engine->PEntityOfEntIndex(params[1]);
	if (!pEdict)
	{
		return pContext->ThrowNativeError("Invalid Entity index %i", params[1]);
	}

	const char *classname = pEdict->GetClassName();

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
		if (hook->pf == pFunction && hook->entity_index == engine->IndexOfEdict(pEdict))
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

#if 0
static cell_t GetVariantType(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	varhandle_t *pInfo;
	HandleSecurity sec;
 
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	if ((err=handlesys->ReadHandle(hndl, g_OutputManager.VariantHandle, &sec, (void **)&pInfo))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid variant handle %x (error %d)", hndl, err);
	}

	return pInfo->crab.FieldType();
}

static cell_t GetVariantInt(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	varhandle_t *pInfo;
	HandleSecurity sec;
 
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();


	if ((err=handlesys->ReadHandle(hndl, g_OutputManager.VariantHandle, &sec, (void **)&pInfo))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid variant handle %x (error %d)", hndl, err);
	}

	if (pInfo->crab.FieldType() != FIELD_INTEGER)
	{
		return pContext->ThrowNativeError("Variant is not an integer");
	}

	return pInfo->crab.Int();
}

static cell_t GetVariantBool(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	varhandle_t *pInfo;
	HandleSecurity sec;
 
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	if ((err=handlesys->ReadHandle(hndl, g_OutputManager.VariantHandle, &sec, (void **)&pInfo))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid variant handle %x (error %d)", hndl, err);
	}

	if (pInfo->crab.FieldType() != FIELD_BOOLEAN)
	{
		return pContext->ThrowNativeError("Variant is not a boolean");
	}

	return pInfo->crab.Bool();
}

static cell_t GetVariantEntity(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	varhandle_t *pInfo;
	HandleSecurity sec;
 
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	if ((err=handlesys->ReadHandle(hndl, g_OutputManager.VariantHandle, &sec, (void **)&pInfo))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid variant handle %x (error %d)", hndl, err);
	}

	if (pInfo->crab.FieldType() != FIELD_EHANDLE)
	{
		return pContext->ThrowNativeError("Variant is not an entity");
	}

	edict_t *pEdict = g_OutputManager.BaseHandleToEdict((CBaseHandle)pInfo->crab.Entity());

	return engine->IndexOfEdict(pEdict);
}

static cell_t GetVariantFloat(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	varhandle_t *pInfo;
	HandleSecurity sec;
 
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	if ((err=handlesys->ReadHandle(hndl, g_OutputManager.VariantHandle, &sec, (void **)&pInfo))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid variant handle %x (error %d)", hndl, err);
	}

	if (pInfo->crab.FieldType() != FIELD_FLOAT)
	{
		return pContext->ThrowNativeError("Variant is not a float");
	}

	return sp_ftoc(pInfo->crab.Float());
}

static cell_t GetVariantVector(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	varhandle_t *pInfo;
	HandleSecurity sec;
 
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	if ((err=handlesys->ReadHandle(hndl, g_OutputManager.VariantHandle, &sec, (void **)&pInfo))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid variant handle %x (error %d)", hndl, err);
	}

	if (pInfo->crab.FieldType() != FIELD_VECTOR)
	{
		return pContext->ThrowNativeError("Variant is not a vector");
	}
		
	cell_t *r;
	pContext->LocalToPhysAddr(params[2], &r);

	Vector temp;
	pInfo->crab.Vector3D(temp);

	r[0] = sp_ftoc(temp[0]);
	r[1] = sp_ftoc(temp[1]);
	r[2] = sp_ftoc(temp[2]);

	return 0;
}

static cell_t GetVariantColour(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	varhandle_t *pInfo;
	HandleSecurity sec;
 
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	if ((err=handlesys->ReadHandle(hndl, g_OutputManager.VariantHandle, &sec, (void **)&pInfo))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid variant handle %x (error %d)", hndl, err);
	}

	if (pInfo->crab.FieldType() != FIELD_COLOR32)
	{
		return pContext->ThrowNativeError("Variant is not a colour");
	}
		
	cell_t *r;
	pContext->LocalToPhysAddr(params[2], &r);

	color32 temp = pInfo->crab.Color32();

	r[0] = temp.r;
	r[1] = temp.g;
	r[2] = temp.b;
	r[4] = temp.a;

	return 0;
}

static cell_t GetVariantString(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	varhandle_t *pInfo;
	HandleSecurity sec;
 
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	if ((err=handlesys->ReadHandle(hndl, g_OutputManager.VariantHandle, &sec, (void **)&pInfo))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid variant handle %x (error %d)", hndl, err);
	}
		
	char *dest;
	const char *src = pInfo->crab.String();
	size_t len;
 
	pContext->LocalToString(params[2], &dest);
 
	/* Perform bounds checking */
	len = strlen(src);
	if (len >= (unsigned)params[3])
	{
		len = params[3] - 1;
	} else {
		len = params[3];
	}
 
	/* Copy */
	memmove(dest, src, len);
 
	dest[len] = '\0';
 
	return 0;
}
#endif

sp_nativeinfo_t g_EntOutputNatives[] =
{
	{"HookEntityOutput",			HookEntityOutput},
	{"UnhookEntityOutput",			UnHookEntityOutput},
	{"HookSingleEntityOutput",		HookSingleEntityOutput},
	{"UnhookSingleEntityOutput",	UnHookSingleEntityOutput},
#if 0 //Removed because we don't need them
	{"GetVariantType",				GetVariantType},
	{"GetVariantInt",				GetVariantInt},
	{"GetVariantFloat",				GetVariantFloat},
	{"GetVariantBool",				GetVariantBool},
	{"GetVariantString",			GetVariantString},
	{"GetVariantEntity",			GetVariantEntity},
	{"GetVariantVector",			GetVariantVector},
	{"GetVariantColour",			GetVariantColour},
#endif
	{NULL,							NULL},
};

#if 0
//Include file stuff that wasn't needed:

enum FieldType
{
	FieldType_Float = 1,
	FieldType_String = 2,
	FieldType_Vector = 3,
	FieldType_Int = 5,
	FieldType_Bool = 6,
	FieldType_Colour = 9,
	FieldType_Entity = 13
}

/**
 * Gets the current type of a variant handle
 *
 * @param variant		A variant handle.
 * @return				Current type of the variant.
 */
native FieldType:GetVariantType(Handle:variant);

/**
 * Retreives a Float from a variant handle.
 *
 * @param variant		A variant handle.
 * @return				Float value.
 * @error				Variant handle is not type FieldType_Float
 */
native Float:GetVariantFloat(Handle:variant);

/**
 * Retreives an Integer from a variant handle.
 *
 * @param variant		A variant handle.
 * @return				Int value.
 * @error				Variant handle is not type FieldType_Int
 */
native GetVariantInt(Handle:variant);

/**
 * Retreives a bool from a variant handle.
 *
 * @param variant		A variant handle.
 * @return				bool value.
 * @error				Variant handle is not type FieldType_Bool
 */
native bool:GetVariantBool(Handle:variant);

/**
 * Retreives an entity from a variant handle.
 *
 * @param variant		A variant handle.
 * @return				Entity Index.
 * @error				Variant handle is not type FieldType_Entity
 */
native GetVariantEntity(Handle:variant);

/**
 * Retreives a Vector from a variant handle.
 *
 * @param variant		A variant handle.
 * @param vec			buffer to store the Vector.
 * @noreturn
 * @error				Variant handle is not type FieldType_Vector
 */
native GetVariantVector(Handle:variant, Float:vec[3]);

/**
 * Retreives a String from a variant handle.
 *
 * @note 	This native does not require the variant to be FieldType_String,
 *			It can convert any type into a string representation.
 *
 * @param variant		A variant handle.
 * @param buffer		buffer to store the string in
 * @param maxlen		Maximum length of buffer.
 * @noreturn
 */
native GetVariantString(Handle:variant, String:buffer[], maxlen);

/**
 * Retreives a Colour from a variant handle.
 *
 * @note Colour is in format r,g,b,a
 *
 * @param variant		A variant handle.
 * @param colour		buffer array to store the colour in.
 * @noreturn
 * @error				Variant handle is not type FieldType_Colour
 */
native GetVariantColour(Handle:variant, colour[4]);
#endif
