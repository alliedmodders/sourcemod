/**
* vim: set ts=4 sw=4 tw=99 noet :
* =============================================================================
* SourceMod
* Copyright (C) 2004-2014 AlliedModders LLC.  All rights reserved.
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
#include "handle_helpers.h"
#include "stringutil.h"
#include <IHandleSys.h>

HandleType_t htCellStack;

class CellStackHelpers : 
	public SMGlobalClass,
	public IHandleTypeDispatch
{
public: //SMGlobalClass
	void OnSourceModAllInitialized()
	{
		htCellStack = handlesys->CreateType("CellStack", this, 0, NULL, NULL, g_pCoreIdent, NULL);
	}
	void OnSourceModShutdown()
	{
		handlesys->RemoveType(htCellStack, g_pCoreIdent);
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
} s_CellStackHelpers;

static cell_t CreateStack(IPluginContext *pContext, const cell_t *params)
{
	if (params[1] < 1)
	{
		return pContext->ThrowNativeError("Invalid block size (must be > 0)");
	}

	CellArray *array = new CellArray(params[1]);

	Handle_t hndl = handlesys->CreateHandle(htCellStack, array, pContext->GetIdentity(), g_pCoreIdent, NULL);
	if (!hndl)
	{
		delete array;
	}

	return hndl;
}

static cell_t ClearStack(IPluginContext *pContext, const cell_t *params)
{
	CellArray *array;
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

	if ((err = handlesys->ReadHandle(params[1], htCellStack, &sec, (void **)&array)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error: %d)", params[1], err);
	}

	array->clear();

	return 1;
}

static cell_t CloneStack(IPluginContext *pContext, const cell_t *params)
{
	CellArray *oldArray;
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

	if ((err = handlesys->ReadHandle(params[1], htCellStack, &sec, (void **)&oldArray)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error: %d)", params[1], err);
	}

	ICellArray *array = oldArray->clone();
	if (!array)
	{
		return pContext->ThrowNativeError("Failed to clone stack. Out of memory.");
	}

	Handle_t hndl = handlesys->CreateHandle(htCellStack, array, pContext->GetIdentity(), g_pCoreIdent, NULL);
	if (!hndl)
	{
		delete array;
	}

	return hndl;
}

static cell_t PushStackCell(IPluginContext *pContext, const cell_t *params)
{
	CellArray *array;
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

	if ((err = handlesys->ReadHandle(params[1], htCellStack, &sec, (void **)&array)) 
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error: %d)", params[1], err);
	}

	cell_t *blk = array->push();
	if (!blk)
	{
		return pContext->ThrowNativeError("Failed to grow stack");
	}

	*blk = params[2];

	return 1;
}

static cell_t PushStackString(IPluginContext *pContext, const cell_t *params)
{
	CellArray *array;
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

	if ((err = handlesys->ReadHandle(params[1], htCellStack, &sec, (void **)&array)) 
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error: %d)", params[1], err);
	}

	cell_t *blk = array->push();
	if (!blk)
	{
		return pContext->ThrowNativeError("Failed to grow stack");
	}

	char *str;
	pContext->LocalToString(params[2], &str);

	strncopy((char *)blk, str, array->blocksize() * sizeof(cell_t));

	return 1;
}

static cell_t PushStackArray(IPluginContext *pContext, const cell_t *params)
{
	CellArray *array;
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

	if ((err = handlesys->ReadHandle(params[1], htCellStack, &sec, (void **)&array)) 
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

	return 1;
}

static cell_t PopStackCell(IPluginContext *pContext, const cell_t *params)
{
	size_t idx;
	HandleError err;
	CellArray *array;
	cell_t *blk, *buffer;
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

	if ((err = handlesys->ReadHandle(params[1], htCellStack, &sec, (void **)&array)) 
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error: %d)", params[1], err);
	}

	if (array->size() == 0)
	{
		return 0;
	}

	pContext->LocalToPhysAddr(params[2], &buffer);

	blk = array->at(array->size() - 1);
	idx = (size_t)params[3];

	if (params[4] == 0)
	{
		if (idx >= array->blocksize())
		{
			return pContext->ThrowNativeError("Invalid block %d (blocksize: %d)", idx, array->blocksize());
		}
		*buffer = blk[idx];
	}
	else
	{
		if (idx >= array->blocksize() * 4)
		{
			return pContext->ThrowNativeError("Invalid byte %d (blocksize: %d bytes)", idx, array->blocksize() * 4);
		}
		*buffer = (cell_t)*((char *)blk + idx);
	}

	array->remove(array->size() - 1);

	return 1;
}

static cell_t PopStackString(IPluginContext *pContext, const cell_t *params)
{
	HandleError err;
	CellArray *array;
	size_t idx, numWritten;
	cell_t *blk, *pWritten;
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

	if ((err = handlesys->ReadHandle(params[1], htCellStack, &sec, (void **)&array)) 
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error: %d)", params[1], err);
	}

	if (array->size() == 0)
	{
		return 0;
	}

	idx = array->size() - 1;
	blk = array->at(idx);
	pContext->StringToLocalUTF8(params[2], params[3], (char *)blk, &numWritten);
	pContext->LocalToPhysAddr(params[4], &pWritten);
	*pWritten = (cell_t)numWritten;
	array->remove(idx);

	return 1;
}

static cell_t PopStackArray(IPluginContext *pContext, const cell_t *params)
{
	size_t idx;
	cell_t *blk;
	cell_t *addr;
	size_t indexes;
	HandleError err;
	CellArray *array;
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

	if ((err = handlesys->ReadHandle(params[1], htCellStack, &sec, (void **)&array)) 
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error: %d)", params[1], err);
	}

	if (array->size() == 0)
	{
		return 0;
	}

	idx = array->size() - 1;
	blk = array->at(idx);
	indexes = array->blocksize();

	if (params[3] != -1 && (size_t)params[3] <= array->blocksize())
	{
		indexes = params[3];
	}

	pContext->LocalToPhysAddr(params[2], &addr);
	memcpy(addr, blk, sizeof(cell_t) * indexes);
	array->remove(idx);

	return indexes;
}

static cell_t IsStackEmpty(IPluginContext *pContext, const cell_t *params)
{
	HandleError err;
	CellArray *array;
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

	if ((err = handlesys->ReadHandle(params[1], htCellStack, &sec, (void **)&array)) 
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error: %d)", params[1], err);
	}

	if (array->size() == 0)
	{
		return 1;
	}

	return 0;
}

static cell_t ArrayStack_Pop(IPluginContext *pContext, const cell_t *params)
{
	OpenHandle<CellArray> array(pContext, params[1], htCellStack); 
	if (!array.Ok())
		return 0;

	if (array->size() == 0)
		return pContext->ThrowNativeError("stack is empty");

	cell_t *blk = array->at(array->size() - 1);
	size_t idx = (size_t)params[2];

	cell_t rval;
	if (params[3] == 0) {
		if (idx >= array->blocksize())
			return pContext->ThrowNativeError("Invalid block %d (blocksize: %d)", idx, array->blocksize());
		rval = blk[idx];
	} else {
		if (idx >= array->blocksize() * 4)
			return pContext->ThrowNativeError("Invalid byte %d (blocksize: %d bytes)", idx, array->blocksize() * 4);
		rval = (cell_t)*((char *)blk + idx);
	}

	array->remove(array->size() - 1);
	return rval;
}

static cell_t ArrayStack_PopString(IPluginContext *pContext, const cell_t *params)
{
	OpenHandle<CellArray> array(pContext, params[1], htCellStack); 
	if (!array.Ok())
		return 0;

	if (array->size() == 0)
		return pContext->ThrowNativeError("stack is empty");

	size_t idx = array->size() - 1;
	cell_t *blk = array->at(idx);

	cell_t *pWritten;
	pContext->LocalToPhysAddr(params[4], &pWritten);

	size_t numWritten;
	pContext->StringToLocalUTF8(params[2], params[3], (char *)blk, &numWritten);
	*pWritten = (cell_t)numWritten;
	array->remove(idx);
	return 1;
}

static cell_t ArrayStack_PopArray(IPluginContext *pContext, const cell_t *params)
{
	OpenHandle<CellArray> array(pContext, params[1], htCellStack); 
	if (!array.Ok())
		return 0;

	if (array->size() == 0)
		return pContext->ThrowNativeError("stack is empty");

	cell_t *addr;
	pContext->LocalToPhysAddr(params[2], &addr);

	size_t idx = array->size() - 1;
	cell_t *blk = array->at(idx);
	size_t indexes = array->blocksize();

	if (params[3] != -1 && (size_t)params[3] <= array->blocksize())
		indexes = params[3];

	memcpy(addr, blk, sizeof(cell_t) * indexes);
	array->remove(idx);
	return 0;
}

static cell_t GetStackBlockSize(IPluginContext *pContext, const cell_t *params)
{
	HandleError err;
	CellArray *array;
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

	if ((err = handlesys->ReadHandle(params[1], htCellStack, &sec, (void **)&array))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error: %d)", params[1], err);
	}

	return array->blocksize();
}

REGISTER_NATIVES(cellStackNatives)
{
	{"CreateStack",					CreateStack},
	{"CloneStack",					CloneStack},
	{"IsStackEmpty",				IsStackEmpty},
	{"PopStackArray",				PopStackArray},
	{"PopStackCell",				PopStackCell},
	{"PopStackString",				PopStackString},
	{"PushStackArray",				PushStackArray},
	{"PushStackCell",				PushStackCell},
	{"PushStackString",				PushStackString},
	{"GetStackBlockSize",			GetStackBlockSize},

	// Transitional syntax support.
	{"ArrayStack.ArrayStack",		CreateStack},
	{"ArrayStack.Clear",			ClearStack},
	{"ArrayStack.Clone",			CloneStack},
	{"ArrayStack.Pop",				ArrayStack_Pop},
	{"ArrayStack.PopString",		ArrayStack_PopString},
	{"ArrayStack.PopArray",			ArrayStack_PopArray},
	{"ArrayStack.Push",				PushStackCell},
	{"ArrayStack.PushString",		PushStackString},
	{"ArrayStack.PushArray",		PushStackArray},
	{"ArrayStack.Empty.get",		IsStackEmpty},
	{"ArrayStack.BlockSize.get",	GetStackBlockSize},

	{NULL,							NULL},
};
