/**
 * vim: set ts=4 :
 * ===============================================================
 * SourceMod (C)2004-2007 AlliedModders LLC.  All rights reserved.
 * ===============================================================
 *
 * This file is not open source and may not be copied without explicit
 * written permission of AlliedModders LLC.  This file may not be redistributed 
 * in whole or significant part.
 * For information, see LICENSE.txt or http://www.sourcemod.net/license.php
 *
 * Version: $Id$
 */

#include <malloc.h>
#include "sm_globals.h"
#include "sm_stringutil.h"
#include "CellArray.h"
#include "HandleSys.h"

HandleType_t htCellArray;

class CellArrayHelpers : 
	public SMGlobalClass,
	public IHandleTypeDispatch
{
public: //SMGlobalClass
	void OnSourceModAllInitialized()
	{
		htCellArray = g_HandleSys.CreateType("CellArray", this, 0, NULL, NULL, g_pCoreIdent, NULL);
	}
	void OnSourceModShutdown()
	{
		g_HandleSys.RemoveType(htCellArray, g_pCoreIdent);
	}
public: //IHandleTypeDispatch
	void OnHandleDestroy(HandleType_t type, void *object)
	{
		CellArray *array = (CellArray *)object;
		delete array;
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
		array->resize(params[2]);
	}
	
	Handle_t hndl = g_HandleSys.CreateHandle(htCellArray, array, pContext->GetIdentity(), g_pCoreIdent, NULL);
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

	if ((err = g_HandleSys.ReadHandle(params[1], htCellArray, &sec, (void **)&array)) 
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

	if ((err = g_HandleSys.ReadHandle(params[1], htCellArray, &sec, (void **)&array)) 
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

	if ((err = g_HandleSys.ReadHandle(params[1], htCellArray, &sec, (void **)&array)) 
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

	if ((err = g_HandleSys.ReadHandle(params[1], htCellArray, &sec, (void **)&array)) 
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

	if ((err = g_HandleSys.ReadHandle(params[1], htCellArray, &sec, (void **)&array)) 
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

	if ((err = g_HandleSys.ReadHandle(params[1], htCellArray, &sec, (void **)&array)) 
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

	if ((err = g_HandleSys.ReadHandle(params[1], htCellArray, &sec, (void **)&array)) 
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

	if ((err = g_HandleSys.ReadHandle(params[1], htCellArray, &sec, (void **)&array)) 
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

	if ((err = g_HandleSys.ReadHandle(params[1], htCellArray, &sec, (void **)&array)) 
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

	if ((err = g_HandleSys.ReadHandle(params[1], htCellArray, &sec, (void **)&array)) 
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

	if ((err = g_HandleSys.ReadHandle(params[1], htCellArray, &sec, (void **)&array)) 
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

	if ((err = g_HandleSys.ReadHandle(params[1], htCellArray, &sec, (void **)&array)) 
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

	if ((err = g_HandleSys.ReadHandle(params[1], htCellArray, &sec, (void **)&array)) 
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

	if ((err = g_HandleSys.ReadHandle(params[1], htCellArray, &sec, (void **)&array)) 
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

	if ((err = g_HandleSys.ReadHandle(params[1], htCellArray, &sec, (void **)&array)) 
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
	{NULL,							NULL},
};
