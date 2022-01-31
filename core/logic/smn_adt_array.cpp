/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * =============================================================================
 * SourceMod
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

#include <stdlib.h>
#include "common_logic.h"
#include "CellArray.h"
#include "stringutil.h"
#include <IHandleSys.h>

HandleType_t htCellArray;

class CellArrayHelpers :
	public SMGlobalClass,
	public IHandleTypeDispatch
{
public: //SMGlobalClass
	void OnSourceModAllInitialized()
	{
		htCellArray = handlesys->CreateType("CellArray", this, 0, NULL, NULL, g_pCoreIdent, NULL);
	}
	void OnSourceModShutdown()
	{
		handlesys->RemoveType(htCellArray, g_pCoreIdent);
	}
public: //IHandleTypeDispatch
	void OnHandleDestroy(HandleType_t type, void *object)
	{
		CellArray *array = (CellArray *)object;
		delete array;
	}
	bool GetHandleApproxSize(HandleType_t type, void *object, unsigned int *pSize)
	{
		CellArray *pArray = (CellArray *)object;
		*pSize = sizeof(CellArray) + pArray->mem_usage();
		return true;
	}
} s_CellArrayHelpers;

static cell_t CreateArray(IPluginContext *pContext, const cell_t *params)
{
	if (!params[1])
	{
		return pContext->ThrowNativeError("Invalid block size (must be > 0)");
	}

	CellArray *array = new CellArray(params[1]);

	if (params[2])
	{
		if (!array->resize(params[2]))
		{
			delete array;
			return pContext->ThrowNativeError("Failed to resize array to startsize \"%u\".", params[2]);
		}
	}

	Handle_t hndl = handlesys->CreateHandle(htCellArray, array, pContext->GetIdentity(), g_pCoreIdent, NULL);
	if (!hndl)
	{
		delete array;
	}

	return hndl;
}

static cell_t ClearArray(IPluginContext *pContext, const cell_t *params)
{
	CellArray *array;
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

	if ((err = handlesys->ReadHandle(params[1], htCellArray, &sec, (void **)&array))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error: %d)", params[1], err);
	}

	array->clear();

	return 1;
}

static cell_t ResizeArray(IPluginContext *pContext, const cell_t *params)
{
	CellArray *array;
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

	if ((err = handlesys->ReadHandle(params[1], htCellArray, &sec, (void **)&array))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error: %d)", params[1], err);
	}

	if (!array->resize(params[2]))
	{
		return pContext->ThrowNativeError("Unable to resize array to \"%u\"", params[2]);
	}

	return 1;
}

static cell_t GetArraySize(IPluginContext *pContext, const cell_t *params)
{
	CellArray *array;
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

	if ((err = handlesys->ReadHandle(params[1], htCellArray, &sec, (void **)&array))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error: %d)", params[1], err);
	}

	return (cell_t)array->size();
}

static cell_t PushArrayCell(IPluginContext *pContext, const cell_t *params)
{
	CellArray *array;
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

	if ((err = handlesys->ReadHandle(params[1], htCellArray, &sec, (void **)&array))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error: %d)", params[1], err);
	}

	cell_t *blk = array->push();
	if (!blk)
	{
		return pContext->ThrowNativeError("Failed to grow array");
	}

	*blk = params[2];

	return (cell_t)(array->size() - 1);
}

static cell_t PushArrayString(IPluginContext *pContext, const cell_t *params)
{
	CellArray *array;
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

	if ((err = handlesys->ReadHandle(params[1], htCellArray, &sec, (void **)&array))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error: %d)", params[1], err);
	}

	cell_t *blk = array->push();
	if (!blk)
	{
		return pContext->ThrowNativeError("Failed to grow array");
	}

	char *str;
	pContext->LocalToString(params[2], &str);

	strncopy((char *)blk, str, array->blocksize() * sizeof(cell_t));

	return (cell_t)(array->size() - 1);
}

static cell_t PushArrayArray(IPluginContext *pContext, const cell_t *params)
{
	CellArray *array;
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

	if ((err = handlesys->ReadHandle(params[1], htCellArray, &sec, (void **)&array))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error: %d)", params[1], err);
	}

	cell_t *blk = array->push();
	if (!blk)
	{
		return pContext->ThrowNativeError("Failed to grow array");
	}

	cell_t *addr;
	pContext->LocalToPhysAddr(params[2], &addr);

	size_t indexes = array->blocksize();
	if (params[3] != -1 && (size_t)params[3] <= array->blocksize())
	{
		indexes = params[3];
	}

	memcpy(blk, addr, sizeof(cell_t) * indexes);

	return (cell_t)(array->size() - 1);
}

static cell_t GetArrayCell(IPluginContext *pContext, const cell_t *params)
{
	CellArray *array;
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

	if ((err = handlesys->ReadHandle(params[1], htCellArray, &sec, (void **)&array))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error: %d)", params[1], err);
	}

	size_t idx = (size_t)params[2];
	if (idx >= array->size())
	{
		return pContext->ThrowNativeError("Invalid index %d (count: %d)", idx, array->size());
	}

	cell_t *blk = array->at(idx);

	idx = (size_t)params[3];
	if (params[4] == 0)
	{
		if (idx >= array->blocksize())
		{
			return pContext->ThrowNativeError("Invalid block %d (blocksize: %d)", idx, array->blocksize());
		}
		return blk[idx];
	} else {
		if (idx >= array->blocksize() * 4)
		{
			return pContext->ThrowNativeError("Invalid byte %d (blocksize: %d bytes)", idx, array->blocksize() * 4);
		}
		return (cell_t)*((char *)blk + idx);
	}

	return 0;
}

static cell_t GetArrayString(IPluginContext *pContext, const cell_t *params)
{
	CellArray *array;
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

	if ((err = handlesys->ReadHandle(params[1], htCellArray, &sec, (void **)&array))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error: %d)", params[1], err);
	}

	size_t idx = (size_t)params[2];
	if (idx >= array->size())
	{
		return pContext->ThrowNativeError("Invalid index %d (count: %d)", idx, array->size());
	}

	cell_t *blk = array->at(idx);
	size_t numWritten = 0;

	pContext->StringToLocalUTF8(params[3], params[4], (char *)blk, &numWritten);

	return numWritten;
}

static cell_t GetArrayArray(IPluginContext *pContext, const cell_t *params)
{
	CellArray *array;
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

	if ((err = handlesys->ReadHandle(params[1], htCellArray, &sec, (void **)&array))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error: %d)", params[1], err);
	}

	size_t idx = (size_t)params[2];
	if (idx >= array->size())
	{
		return pContext->ThrowNativeError("Invalid index %d (count: %d)", idx, array->size());
	}

	cell_t *blk = array->at(idx);
	size_t indexes = array->blocksize();
	if (params[4] != -1 && (size_t)params[4] <= array->blocksize())
	{
		indexes = params[4];
	}

	cell_t *addr;
	pContext->LocalToPhysAddr(params[3], &addr);

	memcpy(addr, blk, sizeof(cell_t) * indexes);

	return indexes;
}

static cell_t SetArrayCell(IPluginContext *pContext, const cell_t *params)
{
	CellArray *array;
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

	if ((err = handlesys->ReadHandle(params[1], htCellArray, &sec, (void **)&array))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error: %d)", params[1], err);
	}

	size_t idx = (size_t)params[2];
	if (idx >= array->size())
	{
		return pContext->ThrowNativeError("Invalid index %d (count: %d)", idx, array->size());
	}

	cell_t *blk = array->at(idx);

	idx = (size_t)params[4];
	if (params[5] == 0)
	{
		if (idx >= array->blocksize())
		{
			return pContext->ThrowNativeError("Invalid block %d (blocksize: %d)", idx, array->blocksize());
		}
		blk[idx] = params[3];
	} else {
		if (idx >= array->blocksize() * 4)
		{
			return pContext->ThrowNativeError("Invalid byte %d (blocksize: %d bytes)", idx, array->blocksize() * 4);
		}
		*((char *)blk + idx) = (char)params[3];
	}

	return 1;
}

static cell_t SetArrayString(IPluginContext *pContext, const cell_t *params)
{
	CellArray *array;
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

	if ((err = handlesys->ReadHandle(params[1], htCellArray, &sec, (void **)&array))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error: %d)", params[1], err);
	}

	size_t idx = (size_t)params[2];
	if (idx >= array->size())
	{
		return pContext->ThrowNativeError("Invalid index %d (count: %d)", idx, array->size());
	}

	cell_t *blk = array->at(idx);

	char *str;
	pContext->LocalToString(params[3], &str);

	return strncopy((char *)blk, str, array->blocksize() * sizeof(cell_t));
}

static cell_t SetArrayArray(IPluginContext *pContext, const cell_t *params)
{
	CellArray *array;
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

	if ((err = handlesys->ReadHandle(params[1], htCellArray, &sec, (void **)&array))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error: %d)", params[1], err);
	}

	size_t idx = (size_t)params[2];
	if (idx >= array->size())
	{
		return pContext->ThrowNativeError("Invalid index %d (count: %d)", idx, array->size());
	}

	cell_t *blk = array->at(idx);
	size_t indexes = array->blocksize();
	if (params[4] != -1 && (size_t)params[4] <= array->blocksize())
	{
		indexes = params[4];
	}

	cell_t *addr;
	pContext->LocalToPhysAddr(params[3], &addr);

	memcpy(blk, addr, sizeof(cell_t) * indexes);

	return indexes;
}

static cell_t ShiftArrayUp(IPluginContext *pContext, const cell_t *params)
{
	CellArray *array;
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

	if ((err = handlesys->ReadHandle(params[1], htCellArray, &sec, (void **)&array))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error: %d)", params[1], err);
	}

	size_t idx = (size_t)params[2];
	if (idx >= array->size())
	{
		return pContext->ThrowNativeError("Invalid index %d (count: %d)", idx, array->size());
	}

	array->insert_at(idx);

	return 1;
}

static cell_t RemoveFromArray(IPluginContext *pContext, const cell_t *params)
{
	CellArray *array;
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

	if ((err = handlesys->ReadHandle(params[1], htCellArray, &sec, (void **)&array))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error: %d)", params[1], err);
	}

	size_t idx = (size_t)params[2];
	if (idx >= array->size())
	{
		return pContext->ThrowNativeError("Invalid index %d (count: %d)", idx, array->size());
	}

	array->remove(idx);

	return 1;
}

static cell_t SwapArrayItems(IPluginContext *pContext, const cell_t *params)
{
	CellArray *array;
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

	if ((err = handlesys->ReadHandle(params[1], htCellArray, &sec, (void **)&array))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error: %d)", params[1], err);
	}

	size_t idx1 = (size_t)params[2];
	size_t idx2 = (size_t)params[3];
	if (idx1 >= array->size())
	{
		return pContext->ThrowNativeError("Invalid index %d (count: %d)", idx1, array->size());
	}
	if (idx2 >= array->size())
	{
		return pContext->ThrowNativeError("Invalid index %d (count: %d)", idx2, array->size());
	}

	array->swap(idx1, idx2);

	return 1;
}

static cell_t CloneArray(IPluginContext *pContext, const cell_t *params)
{
	CellArray *oldArray;
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

	if ((err = handlesys->ReadHandle(params[1], htCellArray, &sec, (void **)&oldArray))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error: %d)", params[1], err);
	}

	ICellArray *array = oldArray->clone();
	if (!array)
	{
		return pContext->ThrowNativeError("Failed to clone array. Out of memory.");
	}

	Handle_t hndl = handlesys->CreateHandle(htCellArray, array, pContext->GetIdentity(), g_pCoreIdent, NULL);
	if (!hndl)
	{
		delete array;
	}

	return hndl;
}

static cell_t FindStringInArray(IPluginContext *pContext, const cell_t *params)
{
	CellArray *array;
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

	if ((err = handlesys->ReadHandle(params[1], htCellArray, &sec, (void **)&array))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error: %d)", params[1], err);
	}

	char *str;
	pContext->LocalToString(params[2], &str);

	for (unsigned int i = 0; i < array->size(); i++)
	{
		const char *array_str = (const char *)array->at(i);
		if (strcmp(str, array_str) == 0)
		{
			return (cell_t) i;
		}
	}

	return -1;
}

static cell_t FindValueInArray(IPluginContext *pContext, const cell_t *params)
{
	CellArray *array;
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

	if ((err = handlesys->ReadHandle(params[1], htCellArray, &sec, (void **)&array))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error: %d)", params[1], err);
	}

	// the blocknumber is not guaranteed to always be passed
	size_t blocknumber = 0;
	if (params[0] >= 3)
	{
		blocknumber = (size_t) params[3];
	}

	if (blocknumber >= array->blocksize())
	{
		return pContext->ThrowNativeError("Invalid block %d (blocksize: %d)", blocknumber, array->blocksize());
	}

	for (unsigned int i = 0; i < array->size(); i++)
	{
		cell_t *blk = array->at(i);
		if (params[2] == blk[blocknumber])
		{
			return (cell_t) i;
		}
	}

	return -1;
}

static cell_t GetArrayBlockSize(IPluginContext *pContext, const cell_t *params)
{
	CellArray *array;
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

	if ((err = handlesys->ReadHandle(params[1], htCellArray, &sec, (void **)&array))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error: %d)", params[1], err);
	}

	return array->blocksize();
}

REGISTER_NATIVES(cellArrayNatives)
{
	{"ClearArray",					ClearArray},
	{"CreateArray",					CreateArray},
	{"GetArrayArray",				GetArrayArray},
	{"GetArrayCell",				GetArrayCell},
	{"GetArraySize",				GetArraySize},
	{"GetArrayString",				GetArrayString},
	{"ResizeArray",					ResizeArray},
	{"PushArrayArray",				PushArrayArray},
	{"PushArrayCell",				PushArrayCell},
	{"PushArrayString",				PushArrayString},
	{"RemoveFromArray",				RemoveFromArray},
	{"SetArrayCell",				SetArrayCell},
	{"SetArrayString",				SetArrayString},
	{"SetArrayArray",				SetArrayArray},
	{"ShiftArrayUp",				ShiftArrayUp},
	{"SwapArrayItems",				SwapArrayItems},
	{"CloneArray",					CloneArray},
	{"FindStringInArray",			FindStringInArray},
	{"FindValueInArray",			FindValueInArray},
	{"GetArrayBlockSize",			GetArrayBlockSize},

	// Transitional syntax support.
	{"ArrayList.ArrayList",			CreateArray},
	{"ArrayList.Clear",				ClearArray},
	{"ArrayList.Length.get",		GetArraySize},
	{"ArrayList.Resize",			ResizeArray},
	{"ArrayList.Get",				GetArrayCell},
	{"ArrayList.GetString",			GetArrayString},
	{"ArrayList.GetArray",			GetArrayArray},
	{"ArrayList.Push",				PushArrayCell},
	{"ArrayList.PushString",		PushArrayString},
	{"ArrayList.PushArray",			PushArrayArray},
	{"ArrayList.Set",				SetArrayCell},
	{"ArrayList.SetString",			SetArrayString},
	{"ArrayList.SetArray",			SetArrayArray},
	{"ArrayList.Erase",				RemoveFromArray},
	{"ArrayList.ShiftUp",			ShiftArrayUp},
	{"ArrayList.SwapAt",			SwapArrayItems},
	{"ArrayList.Clone",				CloneArray},
	{"ArrayList.FindString",		FindStringInArray},
	{"ArrayList.FindValue",			FindValueInArray},
	{"ArrayList.BlockSize.get",		GetArrayBlockSize},

	{NULL,							NULL},
};
