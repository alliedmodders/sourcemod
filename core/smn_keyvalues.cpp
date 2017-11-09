/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2015 AlliedModders LLC.  All rights reserved.
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

#include "smn_keyvalues.h"

#include "sourcemod.h"
#include "sourcemm_api.h"
#include "sm_stringutil.h"
#include "HalfLife2.h"
#include <KeyValues.h>
#include "logic_bridge.h"

HandleType_t g_KeyValueType;

class KeyValueNatives : 
	public SMGlobalClass,
	public IHandleTypeDispatch
{
public:
	void OnSourceModAllInitialized()
	{
		g_KeyValueType = handlesys->CreateType("KeyValues", this, 0, NULL, NULL, g_pCoreIdent, NULL);
	}
	void OnSourceModShutdown()
	{
		handlesys->RemoveType(g_KeyValueType, g_pCoreIdent);
		g_KeyValueType = 0;
	}
	void OnHandleDestroy(HandleType_t type, void *object)
	{
		IKeyValueStack *pKVStack = reinterpret_cast<IKeyValueStack *>(object);
		g_SourceMod.FreeKVStack(pKVStack);
	}
	bool GetHandleApproxSize(HandleType_t type, void *object, unsigned int *pSize)
	{
		IKeyValueStack *pKVStack = (IKeyValueStack *)object;
		*pSize = pKVStack->CalcSize();

		return true;
	}
};

Handle_t SourceModBase::CreateHandleFromKVStack(IKeyValueStack *kVStack, IdentityToken_t *owner, HandleError *err)
{
	return handlesys->CreateHandle(g_KeyValueType, kVStack, owner, g_pCoreIdent, err);
}

IKeyValueStack *SourceModBase::ReadKVStackFromHandle(Handle_t hndl, IdentityToken_t *owner, HandleError *err)
{
	HandleError hErr;
	HandleSecurity hSec;
	IKeyValueStack *pKVStack;

	hSec.pOwner = NULL;
	hSec.pIdentity = g_pCoreIdent;

	if ((hErr = handlesys->ReadHandle(hndl, g_KeyValueType, &hSec, (void **)&pKVStack))
		!= HandleError_None)
	{
		if (err)
		{
			*err = hErr;
		}
		return NULL;
	}

	if (err)
	{
		*err = HandleError_None;
	}

	return pKVStack;
}

KeyValues *SourceModBase::ReadKeyValuesHandle(Handle_t hndl, HandleError *err, bool root)
{
	IKeyValueStack* pKVStack = ReadKVStackFromHandle(hndl, NULL, err);
	if (*err == HandleError_None)
	{
		return (root) ? pKVStack->GetRootSection() : pKVStack->GetCurrentSection();
	}
	return NULL;
}

static cell_t smn_KvSetString(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	IKeyValueStack *pStk;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_KeyValueType, &sec, (void **)&pStk))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid key value handle %x (error %d)", hndl, herr);
	}

	char *key, *value;
	pCtx->LocalToStringNULL(params[2], &key);
	pCtx->LocalToString(params[3], &value);

	pStk->GetCurrentSection()->SetString(key, value);

	return 1;
}

static cell_t smn_KvSetNum(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	IKeyValueStack *pStk;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_KeyValueType, &sec, (void **)&pStk))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid key value handle %x (error %d)", hndl, herr);
	}

	char *key;
	pCtx->LocalToStringNULL(params[2], &key);

	pStk->GetCurrentSection()->SetInt(key, params[3]);

	return 1;
}

static cell_t smn_KvSetUInt64(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	IKeyValueStack *pStk;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_KeyValueType, &sec, (void **)&pStk))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid key value handle %x (error %d)", hndl, herr);
	}

	char *key;
	cell_t *addr;
	uint64 value;
	pCtx->LocalToStringNULL(params[2], &key);
	pCtx->LocalToPhysAddr(params[3], &addr);

	value = *reinterpret_cast<uint64 *>(addr);
	pStk->GetCurrentSection()->SetUint64(key, value);

	return 1;
}

static cell_t smn_KvSetFloat(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	IKeyValueStack *pStk;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_KeyValueType, &sec, (void **)&pStk))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid key value handle %x (error %d)", hndl, herr);
	}

	char *key;
	pCtx->LocalToStringNULL(params[2], &key);

	pStk->GetCurrentSection()->SetFloat(key, sp_ctof(params[3]));

	return 1;
}

static cell_t smn_KvSetColor(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	IKeyValueStack *pStk;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_KeyValueType, &sec, (void **)&pStk))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid key value handle %x (error %d)", hndl, herr);
	}

	char *key;
	pCtx->LocalToStringNULL(params[2], &key);

	Color color(params[3], params[4], params[5], params[6]);
	pStk->GetCurrentSection()->SetColor(key, color);

	return 1;
}

static cell_t smn_KvSetVector(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	IKeyValueStack *pStk;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_KeyValueType, &sec, (void **)&pStk))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid key value handle %x (error %d)", hndl, herr);
	}

	char *key;
	char buffer[64];
	cell_t *vector;
	pCtx->LocalToStringNULL(params[2], &key);
	pCtx->LocalToPhysAddr(params[3], &vector);

	ke::SafeSprintf(buffer, sizeof(buffer), "%f %f %f", sp_ctof(vector[0]), sp_ctof(vector[1]), sp_ctof(vector[2]));

	pStk->GetCurrentSection()->SetString(key, buffer);

	return 1;
}

static cell_t smn_KvGetString(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	IKeyValueStack *pStk;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_KeyValueType, &sec, (void **)&pStk))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid key value handle %x (error %d)", hndl, herr);
	}

	const char *value;
	char *key, *defvalue;
	pCtx->LocalToStringNULL(params[2], &key);
	pCtx->LocalToString(params[5], &defvalue);

	value = pStk->GetCurrentSection()->GetString(key, defvalue);
	pCtx->StringToLocalUTF8(params[3], params[4], value, NULL);

	return 1;
}

static cell_t smn_KvGetNum(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	IKeyValueStack *pStk;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_KeyValueType, &sec, (void **)&pStk))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid key value handle %x (error %d)", hndl, herr);
	}

	int value;
	char *key;
	pCtx->LocalToStringNULL(params[2], &key);

	value = pStk->GetCurrentSection()->GetInt(key, params[3]);

	return value;
}

static cell_t smn_KvGetFloat(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	IKeyValueStack *pStk;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_KeyValueType, &sec, (void **)&pStk))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid key value handle %x (error %d)", hndl, herr);
	}

	float value;
	char *key;
	pCtx->LocalToStringNULL(params[2], &key);

	value = pStk->GetCurrentSection()->GetFloat(key, sp_ctof(params[3]));

	return sp_ftoc(value);
}

static cell_t smn_KvGetColor(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	IKeyValueStack *pStk;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_KeyValueType, &sec, (void **)&pStk))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid key value handle %x (error %d)", hndl, herr);
	}

	Color color;
	char *key;
	cell_t *r, *g, *b, *a;
	pCtx->LocalToStringNULL(params[2], &key);
	pCtx->LocalToPhysAddr(params[3], &r);
	pCtx->LocalToPhysAddr(params[4], &g);
	pCtx->LocalToPhysAddr(params[5], &b);
	pCtx->LocalToPhysAddr(params[6], &a);

	color = pStk->GetCurrentSection()->GetColor(key);
	*r = color.r();
	*g = color.g();
	*b = color.b();
	*a = color.a();

	return 1;
}

static cell_t smn_KvGetUInt64(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	IKeyValueStack *pStk;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_KeyValueType, &sec, (void **)&pStk))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid key value handle %x (error %d)", hndl, herr);
	}

	char *key;
	cell_t *addr, *defvalue;
	uint64 value;
	pCtx->LocalToStringNULL(params[2], &key);
	pCtx->LocalToPhysAddr(params[3], &addr);
	pCtx->LocalToPhysAddr(params[4], &defvalue);

	value = pStk->GetCurrentSection()->GetUint64(key, static_cast<uint64>(*defvalue));
	*reinterpret_cast<uint64 *>(addr) = value;

	return 1;
}

static cell_t smn_KvGetVector(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	IKeyValueStack *pStk;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_KeyValueType, &sec, (void **)&pStk))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid key value handle %x (error %d)", hndl, herr);
	}

	char *key;
	const char *value;
	cell_t *defvector, *outvector;
	char buffer[64];
	pCtx->LocalToStringNULL(params[2], &key);
	pCtx->LocalToPhysAddr(params[3], &outvector);
	pCtx->LocalToPhysAddr(params[4], &defvector);

	ke::SafeSprintf(buffer, sizeof(buffer), "%f %f %f", sp_ctof(defvector[0]), sp_ctof(defvector[1]), sp_ctof(defvector[2]));

	value = pStk->GetCurrentSection()->GetString(key, buffer);

	float out;
	int components = 0;
	while (*value && components < 3)
	{
		while ((*value) && (*value == ' '))
		{
			value++;
		}

		out = 0.0f;
		bool isnegative;
		if (*value == '-')
		{
			isnegative = true;
			value++;
		}
		else
		{
			isnegative = false;
		}

		for (; *value && isdigit(*value); ++value)
		{
			out *= 10.0f;
			out += *value - '0';
		}

		if (*value == '.')
		{
			value++;
			float factor = 0.1f;
			for (; *value && isdigit(*value); ++value)
			{
				out += (*value - '0') * factor;
				factor *= 0.1f;
			}
		}

		out = (isnegative) ? -out : out;
		outvector[components++] = sp_ftoc(out);
	}

	return 1;
}

static cell_t smn_CreateKeyValues(IPluginContext *pCtx, const cell_t *params)
{
	IKeyValueStack *pStk;
	char *name, *firstkey, *firstvalue;
	bool is_empty;

	pCtx->LocalToString(params[1], &name);
	pCtx->LocalToString(params[2], &firstkey);
	pCtx->LocalToString(params[3], &firstvalue);

	is_empty = (firstkey[0] == '\0');
	KeyValues *pKV = new KeyValues(name, is_empty ? NULL : firstkey, (is_empty || (firstvalue[0] == '\0')) ? NULL : firstvalue);
	pStk = g_SourceMod.CreateKVStack(pKV);

	return g_SourceMod.CreateHandleFromKVStack(pStk, pCtx->GetIdentity(), NULL);
}

static cell_t smn_KvJumpToKey(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	char *name;	
	IKeyValueStack *pStk;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_KeyValueType, &sec, (void **)&pStk))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid key value handle %x (error %d)", hndl, herr);
	}

	pCtx->LocalToString(params[2], &name);
	bool createKey = (params[3]) ? true : false;

	return pStk->JumpToKey(name, createKey);
}

static cell_t smn_KvJumpToKeySymbol(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	int symbol = params[2];
	HandleError herr;
	HandleSecurity sec;
	IKeyValueStack *pStk;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_KeyValueType, &sec, (void **)&pStk))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid key value handle %x (error %d)", hndl, herr);
	}

	return pStk->JumpToKeySymbol(symbol);
}

static cell_t smn_KvGotoFirstSubKey(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	bool keyOnly = params[2];
	HandleError herr;
	HandleSecurity sec;
	IKeyValueStack *pStk;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_KeyValueType, &sec, (void **)&pStk))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid key value handle %x (error %d)", hndl, herr);
	}

	return pStk->GotoFirstSubKey(keyOnly);
}

static cell_t smn_KvGotoNextKey(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	bool keyOnly = params[2];
	HandleError herr;
	HandleSecurity sec;
	IKeyValueStack *pStk;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_KeyValueType, &sec, (void **)&pStk))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid key value handle %x (error %d)", hndl, herr);
	}

	return pStk->GotoNextKey(keyOnly);
}

static cell_t smn_KvGoBack(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	IKeyValueStack *pStk;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_KeyValueType, &sec, (void **)&pStk))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid key value handle %x (error %d)", hndl, herr);
	}

	return pStk->GoBack();
}

static cell_t smn_KvRewind(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	IKeyValueStack *pStk;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_KeyValueType, &sec, (void **)&pStk))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid key value handle %x (error %d)", hndl, herr);
	}

	pStk->Rewind();

	return 1;
}

static cell_t smn_KvGetSectionName(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	IKeyValueStack *pStk;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_KeyValueType, &sec, (void **)&pStk))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid key value handle %x (error %d)", hndl, herr);
	}

	const char *name = pStk->GetCurrentSection()->GetName();
	if (!name)
	{
		return 0;
	}
	pCtx->StringToLocalUTF8(params[2], params[3], name, NULL);

	return 1;
}

static cell_t smn_KvSetSectionName(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	char *name;
	IKeyValueStack *pStk;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_KeyValueType, &sec, (void **)&pStk))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid key value handle %x (error %d)", hndl, herr);
	}

	pCtx->LocalToString(params[2], &name);

	pStk->GetCurrentSection()->SetName(name);

	return 1;
}

static cell_t smn_KvGetDataType(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	char *name;
	IKeyValueStack *pStk;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_KeyValueType, &sec, (void **)&pStk))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid key value handle %x (error %d)", hndl, herr);
	}

	pCtx->LocalToString(params[2], &name);

	return pStk->GetCurrentSection()->GetDataType(name);
}

static cell_t smn_KeyValuesToFile(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	char *path;
	IKeyValueStack *pStk;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_KeyValueType, &sec, (void **)&pStk))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid key value handle %x (error %d)", hndl, herr);
	}

	pCtx->LocalToString(params[2], &path);

	return pStk->GetCurrentSection()->SaveToFile(basefilesystem, path);
}

static cell_t smn_FileToKeyValues(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	char *path;
	IKeyValueStack *pStk;
	KeyValues *kv;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr = handlesys->ReadHandle(hndl, g_KeyValueType, &sec, (void **)&pStk))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid key value handle %x (error %d)", hndl, herr);
	}

	pCtx->LocalToString(params[2], &path);

	return g_HL2.KVLoadFromFile(pStk->GetCurrentSection(), basefilesystem, path);
}

static cell_t smn_StringToKeyValues(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	IKeyValueStack *pStk;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_KeyValueType, &sec, (void **)&pStk))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid key value handle %x (error %d)", hndl, herr);
	}

	char *buffer;
	char *resourceName;
	pCtx->LocalToString(params[2], &buffer);
	pCtx->LocalToString(params[3], &resourceName);

	return pStk->GetCurrentSection()->LoadFromBuffer(resourceName, buffer);
}

static cell_t smn_KvSetEscapeSequences(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	IKeyValueStack *pStk;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_KeyValueType, &sec, (void **)&pStk))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid key value handle %x (error %d)", hndl, herr);
	}

	pStk->GetCurrentSection()->UsesEscapeSequences(params[2] ? true : false);

	return 1;
}

static cell_t smn_KvNodesInStack(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	IKeyValueStack *pStk;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_KeyValueType, &sec, (void **)&pStk))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid key value handle %x (error %d)", hndl, herr);
	}

	return pStk->GetNodeCount();
}

static cell_t smn_KvDeleteThis(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	IKeyValueStack *pStk;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_KeyValueType, &sec, (void **)&pStk))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid key value handle %x (error %d)", hndl, herr);
	}

	return pStk->DeleteThis();
}

static cell_t smn_KvDeleteKey(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	IKeyValueStack *pStk;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_KeyValueType, &sec, (void **)&pStk))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid key value handle %x (error %d)", hndl, herr);
	}

	if (pStk->GetNodeCount() == 0)
	{
		return 0;
	}

	char *keyName;
	pContext->LocalToString(params[2], &keyName);

	KeyValues *pRoot = pStk->GetCurrentSection();
	KeyValues *pValues = pRoot->FindKey(keyName);
	if (!pValues)
	{
		return 0;
	}

	pRoot->RemoveSubKey(pValues);
	pValues->deleteThis();

	return 1;
}

static cell_t smn_KvSavePosition(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	IKeyValueStack *pStk;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_KeyValueType, &sec, (void **)&pStk))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid key value handle %x (error %d)", hndl, herr);
	}

	return pStk->SavePosition();
}

static cell_t smn_CopySubkeys(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl_copied = static_cast<Handle_t>(params[1]);
	Handle_t hndl_parent = static_cast<Handle_t>(params[2]);
	HandleError herr;
	HandleSecurity sec;
	IKeyValueStack *pStk_copied, *pStk_parent;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl_copied, g_KeyValueType, &sec, (void **)&pStk_copied))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid key value handle %x (error %d)", hndl_copied, herr);
	}
	if ((herr=handlesys->ReadHandle(hndl_parent, g_KeyValueType, &sec, (void **)&pStk_parent))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid key value handle %x (error %d)", hndl_parent, herr);
	}

	pStk_copied->GetCurrentSection()->CopySubkeys(pStk_parent->GetCurrentSection());

	return 1;
}

static cell_t smn_GetNameSymbol(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	IKeyValueStack *pStk;
	cell_t *val;
	char *key;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_KeyValueType, &sec, (void **)&pStk))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid key value handle %x (error %d)", hndl, herr);
	}

	if (pStk->GetNodeCount() == 0)
	{
		return 0;
	}

	pContext->LocalToString(params[2], &key);

	KeyValues *pKv = pStk->GetCurrentSection()->FindKey(key);
	if (!pKv)
	{
		return 0;
	}
	pContext->LocalToPhysAddr(params[3], &val);
	*val = pKv->GetNameSymbol();

	return 1;
}

static cell_t smn_FindKeyById(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	IKeyValueStack *pStk;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_KeyValueType, &sec, (void **)&pStk))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid key value handle %x (error %d)", hndl, herr);
	}

	KeyValues *pKv = pStk->GetCurrentSection()->FindKey(params[2]);
	if (!pKv)
	{
		return 0;
	}

	pContext->StringToLocalUTF8(params[3], params[4], pKv->GetName(), NULL);

	return 1;
}

static cell_t smn_KvGetSectionSymbol(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	IKeyValueStack *pStk;
	cell_t *val;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_KeyValueType, &sec, (void **)&pStk))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid key value handle %x (error %d)", hndl, herr);
	}

	KeyValues *pSection = pStk->GetCurrentSection();

	pCtx->LocalToPhysAddr(params[2], &val);
	*val = pSection->GetNameSymbol();

	if (!*val)
	{
		return 0;
	}

	return 1;
}

static cell_t KeyValues_Import(IPluginContext *pContext, const cell_t *params)
{
	// This version takes (dest, src). The original is (src, dest).
	cell_t new_params[3] = {
		2,
		params[2],
		params[1],
	};

	return smn_CopySubkeys(pContext, new_params);
}

static cell_t smn_KeyValuesToString(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	IKeyValueStack *pStk;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_KeyValueType, &sec, (void **)&pStk))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid key value handle %x (error %d)", hndl, herr);
	}
	KeyValues *kv;
	CUtlBuffer buffer;
	
	kv = pStk->GetCurrentSection();

	kv->RecursiveSaveToFile(buffer, 0);
	
	char* outStr;
	pContext->LocalToString(params[2], &outStr);
	size_t maxlen = static_cast<size_t>(params[3]);
	
	buffer.GetString(outStr, maxlen);
	return buffer.TellPut();
}

static cell_t smn_KeyValuesExportLength(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	IKeyValueStack *pStk;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_KeyValueType, &sec, (void **)&pStk))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid key value handle %x (error %d)", hndl, herr);
	}
	KeyValues *kv;
	CUtlBuffer buffer;
	
	kv = pStk->GetCurrentSection();

	kv->RecursiveSaveToFile(buffer, 0);
	
	return (cell_t)buffer.TellPut();
}

static KeyValueNatives s_KeyValueNatives;

REGISTER_NATIVES(keyvaluenatives)
{
	{"KvSetString",				smn_KvSetString},
	{"KvSetNum",				smn_KvSetNum},
	{"KvSetUInt64",				smn_KvSetUInt64},
	{"KvSetFloat",				smn_KvSetFloat},
	{"KvSetColor",				smn_KvSetColor},
	{"KvGetString",				smn_KvGetString},
	{"KvGetNum",				smn_KvGetNum},
	{"KvGetFloat",				smn_KvGetFloat},
	{"KvGetColor",				smn_KvGetColor},
	{"KvGetUInt64",				smn_KvGetUInt64},
	{"CreateKeyValues",			smn_CreateKeyValues},
	{"KvJumpToKey",				smn_KvJumpToKey},
	{"KvJumpToKeySymbol",		smn_KvJumpToKeySymbol},
	{"KvGotoNextKey",			smn_KvGotoNextKey},
	{"KvGotoFirstSubKey",		smn_KvGotoFirstSubKey},
	{"KvGoBack",				smn_KvGoBack},
	{"KvRewind",				smn_KvRewind},
	{"KvGetSectionName",		smn_KvGetSectionName},
	{"KvSetSectionName",		smn_KvSetSectionName},
	{"KvGetDataType",			smn_KvGetDataType},
	{"KeyValuesToFile",			smn_KeyValuesToFile},
	{"FileToKeyValues",			smn_FileToKeyValues},
	{"StringToKeyValues",		smn_StringToKeyValues},
	{"KvSetEscapeSequences",	smn_KvSetEscapeSequences},
	{"KvDeleteThis",			smn_KvDeleteThis},
	{"KvDeleteKey",				smn_KvDeleteKey},
	{"KvNodesInStack",			smn_KvNodesInStack},
	{"KvSavePosition",			smn_KvSavePosition},
	{"KvCopySubkeys",			smn_CopySubkeys},
	{"KvFindKeyById",			smn_FindKeyById},
	{"KvGetNameSymbol",			smn_GetNameSymbol},
	{"KvGetSectionSymbol",		smn_KvGetSectionSymbol},
	{"KvGetVector",				smn_KvGetVector},
	{"KvSetVector",				smn_KvSetVector},

	// Transitional syntax support.
	{"KeyValues.KeyValues",				smn_CreateKeyValues},
	{"KeyValues.SetString",				smn_KvSetString},
	{"KeyValues.SetNum",				smn_KvSetNum},
	{"KeyValues.SetUInt64",				smn_KvSetUInt64},
	{"KeyValues.SetFloat",				smn_KvSetFloat},
	{"KeyValues.SetColor",				smn_KvSetColor},
	{"KeyValues.GetString",				smn_KvGetString},
	{"KeyValues.GetNum",				smn_KvGetNum},
	{"KeyValues.GetFloat",				smn_KvGetFloat},
	{"KeyValues.GetColor",				smn_KvGetColor},
	{"KeyValues.GetUInt64",				smn_KvGetUInt64},
	{"KeyValues.JumpToKey",				smn_KvJumpToKey},
	{"KeyValues.JumpToKeySymbol",		smn_KvJumpToKeySymbol},
	{"KeyValues.GotoNextKey",			smn_KvGotoNextKey},
	{"KeyValues.GotoFirstSubKey",		smn_KvGotoFirstSubKey},
	{"KeyValues.GoBack",				smn_KvGoBack},
	{"KeyValues.Rewind",				smn_KvRewind},
	{"KeyValues.GetSectionName",		smn_KvGetSectionName},
	{"KeyValues.SetSectionName",		smn_KvSetSectionName},
	{"KeyValues.GetDataType",			smn_KvGetDataType},
	{"KeyValues.SetEscapeSequences",	smn_KvSetEscapeSequences},
	{"KeyValues.DeleteThis",			smn_KvDeleteThis},
	{"KeyValues.DeleteKey",				smn_KvDeleteKey},
	{"KeyValues.NodesInStack",			smn_KvNodesInStack},
	{"KeyValues.SavePosition",			smn_KvSavePosition},
	{"KeyValues.FindKeyById",			smn_FindKeyById},
	{"KeyValues.GetNameSymbol",			smn_GetNameSymbol},
	{"KeyValues.GetSectionSymbol",		smn_KvGetSectionSymbol},
	{"KeyValues.GetVector",				smn_KvGetVector},
	{"KeyValues.SetVector",				smn_KvSetVector},
	{"KeyValues.Import",				KeyValues_Import},
	{"KeyValues.ImportFromFile",		smn_FileToKeyValues},
	{"KeyValues.ImportFromString",		smn_StringToKeyValues},
	{"KeyValues.ExportToFile",			smn_KeyValuesToFile},
	{"KeyValues.ExportToString",		smn_KeyValuesToString},
	{"KeyValues.ExportLength.get",		smn_KeyValuesExportLength},

	{NULL,						NULL}
};
