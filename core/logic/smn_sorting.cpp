/**
 * vim: set ts=4 :
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
#include <string.h>
#include <time.h>
#include "common_logic.h"
#include "CellArray.h"
#include <IHandleSys.h>
#include <amtl/am-raii.h>

/***********************************
 *   About the double array hack   *
     ***************************   

 Double arrays in Pawn are vectors offset by the current offset.  For example:

 new array[2][2]

   In this array, index 0 contains the offset from the current offset which
 results in the final vector [2] (at [0][2]).  Meaning, to dereference [1][2], 
 it is equivalent to:

 address = &array[1] + array[1] + 2 * sizeof(cell)

   The fact that each offset is from the _current_ position rather than the _base_
 position is very important.  It means that if you to try to swap vector positions,
 the offsets will no longer match, because their current position has changed.  A 
 simple and ingenious way around this is to back up the positions in a separate array,
 then to overwrite each position in the old array with absolute indices.  Pseudo C++ code:

 cell *array; //assumed to be set to the 2+D array
 cell *old_offsets = new cell[2];
 for (int i=0; i<2; i++)
 {
    old_offsets = array[i];
	array[i] = i;
 }

   Now, you can swap the array indices with no problem, and do a reverse-lookup to find the original addresses.
 After sorting/modification is done, you must relocate the new indices.  For example, if the two vectors in our
 demo array were swapped, array[0] would be 1 and array[1] would be 0.  This is invalid to the virtual machine.
 Luckily, this is also simple -- all the information is there.

 for (int i=0; i<2; i++)
 {
	//get the # of the vector we want to relocate in
	cell vector_index = array[i];			
	//get the real address of this vector
	char *real_address = (char *)array + (vector_index * sizeof(cell)) + old_offsets[vector_index];
	//calc and store the new distance offset
	array[i] = real_address - ( (char *)array + (vector_index + sizeof(cell)) )
 }

 Note that the inner expression can be heavily reduced; it is expanded for readability.
 **********************************/

enum SortOrder
{
	Sort_Ascending = 0,
	Sort_Descending = 1,
	Sort_Random = 2,
};

void sort_random(cell_t *array, cell_t size)
{
	for (int i = size-1; i > 0; i--)
	{
        int n = rand() % (i + 1);

		if (array[i] != array[n])
		{
			array[i] ^= array[n];
			array[n] ^= array[i];
			array[i] ^= array[n];
		}
	}
}

int sort_ints_asc(const void *int1, const void *int2)
{
	return (*(int *)int1) - (*(int *)int2);
}

int sort_ints_desc(const void *int1, const void *int2)
{
	return (*(int *)int2) - (*(int *)int1);
}

static cell_t sm_SortIntegers(IPluginContext *pContext, const cell_t *params)
{
	cell_t *array;
	cell_t array_size = params[2];
	cell_t type = params[3];

	pContext->LocalToPhysAddr(params[1], &array);

	if (type == Sort_Ascending)
	{
		qsort(array, array_size, sizeof(cell_t), sort_ints_asc);
	}
	else if (type == Sort_Descending)
	{
		qsort(array, array_size, sizeof(cell_t), sort_ints_desc);
	}
	else
	{
		sort_random(array, array_size);
	}

	return 1;
}

int sort_floats_asc(const void *float1, const void *float2)
{
	float r1 = *(float *)float1;
	float r2 = *(float *)float2;
	
	if (r1 < r2)
	{
		return -1;
	} else if (r2 < r1) {
		return 1;
	} else {
		return 0;
	}
}

int sort_floats_desc(const void *float1, const void *float2)
{
	float r1 = *(float *)float1;
	float r2 = *(float *)float2;

	if (r1 < r2)
	{
		return 1;
	} else if (r2 < r1) {
		return -1;
	} else {
		return 0;
	}
}

static cell_t sm_SortFloats(IPluginContext *pContext, const cell_t *params)
{
	cell_t *array;
	cell_t array_size = params[2];
	cell_t type = params[3];

	pContext->LocalToPhysAddr(params[1], &array);

	if (type == Sort_Ascending)
	{
		qsort(array, array_size, sizeof(cell_t), sort_floats_asc);
	}
	else if (type == Sort_Descending)
	{
		qsort(array, array_size, sizeof(cell_t), sort_floats_desc);
	}
	else
	{
		sort_random(array, array_size);
	}

	return 1;
}

static cell_t *g_CurStringArray = NULL;
static cell_t *g_CurRebaseMap = NULL;

int sort_strings_asc_legacy(const void *blk1, const void *blk2)
{
	cell_t reloc1 = *(cell_t *)blk1;
	cell_t reloc2 = *(cell_t *)blk2;
	
	char *str1 = ((char *)(&g_CurStringArray[reloc1]) + g_CurRebaseMap[reloc1]);
	char *str2 = ((char *)(&g_CurStringArray[reloc2]) + g_CurRebaseMap[reloc2]);

	return strcmp(str1, str2);
}

int sort_strings_desc_legacy(const void *blk1, const void *blk2)
{
	cell_t reloc1 = *(cell_t *)blk1;
	cell_t reloc2 = *(cell_t *)blk2;

	char *str1 = ((char *)(&g_CurStringArray[reloc1]) + g_CurRebaseMap[reloc1]);
	char *str2 = ((char *)(&g_CurStringArray[reloc2]) + g_CurRebaseMap[reloc2]);

	return strcmp(str2, str1);
}

static cell_t sm_SortStrings_Legacy(IPluginContext *pContext, const cell_t *params)
{
	cell_t *array;
	cell_t array_size = params[2];
	cell_t type = params[3];

	pContext->LocalToPhysAddr(params[1], &array);

	/** HACKHACK - back up the old indices, replace the indices with something easier */
	cell_t amx_addr, *phys_addr;
	int err;
	if ((err=pContext->HeapAlloc(array_size, &amx_addr, &phys_addr)) != SP_ERROR_NONE)
	{
		pContext->ThrowNativeErrorEx(err, "Ran out of memory to sort");
		return 0;
	}

	g_CurStringArray = array;
	g_CurRebaseMap = phys_addr;

	for (int i=0; i<array_size; i++)
	{
		phys_addr[i] = array[i];
		array[i] = i;
	}

	if (type == Sort_Ascending)
	{
		qsort(array, array_size, sizeof(cell_t), sort_strings_asc_legacy);
	}
	else if (type == Sort_Descending)
	{
		qsort(array, array_size, sizeof(cell_t), sort_strings_desc_legacy);
	}
	else
	{
		sort_random(array, array_size);
	}

	/* END HACKHACK - restore what we damaged so Pawn doesn't throw up.
	 * We'll browse through each index of the array and patch up the distance.
	 */
	for (int i=0; i<array_size; i++)
	{
		/* Compute the final address of the old array and subtract the new location.
		 * This is the fixed up distance.
		 */
		array[i] = ((char *)&array[array[i]] + phys_addr[array[i]]) - (char *)&array[i];
	}

	pContext->HeapPop(amx_addr);

	g_CurStringArray = NULL;
	g_CurRebaseMap = NULL;

	return 1;
}

static IPluginContext* sSortContext = nullptr;

int sort_strings_asc(const void *blk1, const void *blk2)
{
	cell_t str_addr1 = *(cell_t *)blk1;
	cell_t str_addr2 = *(cell_t *)blk2;

	char *str1;
	char *str2;
	if (sSortContext->LocalToString(str_addr1, &str1) != SP_ERROR_NONE ||
		sSortContext->LocalToString(str_addr2, &str2) != SP_ERROR_NONE)
	{
		return 0;
	}

	return strcmp(str1, str2);
}

int sort_strings_desc(const void *blk1, const void *blk2)
{
	cell_t str_addr1 = *(cell_t *)blk1;
	cell_t str_addr2 = *(cell_t *)blk2;

	char *str1;
	char *str2;
	if (sSortContext->LocalToString(str_addr1, &str1) != SP_ERROR_NONE ||
		sSortContext->LocalToString(str_addr2, &str2) != SP_ERROR_NONE)
	{
		return 0;
	}

	return strcmp(str2, str1);
}

static cell_t sm_SortStrings(IPluginContext *pContext, const cell_t *params)
{
	auto rt = pContext->GetRuntime();
	if (!rt->UsesDirectArrays())
		return sm_SortStrings_Legacy(pContext, params);

	cell_t *array;
	cell_t array_size = params[2];
	cell_t type = params[3];

	pContext->LocalToPhysAddr(params[1], &array);

	ke::SaveAndSet<IPluginContext*> set_context(&sSortContext, pContext);

	if (type == Sort_Ascending)
	{
		qsort(array, array_size, sizeof(cell_t), sort_strings_asc);
	}
	else if (type == Sort_Descending)
	{
		qsort(array, array_size, sizeof(cell_t), sort_strings_desc);
	}
	else
	{
		sort_random(array, array_size);
	}
	return 1;
}

struct sort_info
{
	IPluginFunction *pFunc;
	Handle_t hndl;
	cell_t array_addr;
	cell_t *array_base;
	cell_t *array_remap;
	ExceptionHandler *eh;
};

sort_info g_SortInfo;

int sort1d_amx_custom(const void *elem1, const void *elem2)
{
	if (g_SortInfo.eh->HasException())
		return 0;

	cell_t c1 = *(cell_t *)elem1;
	cell_t c2 = *(cell_t *)elem2;

	cell_t result = 0;
	IPluginFunction *pf = g_SortInfo.pFunc;
	pf->PushCell(c1);
	pf->PushCell(c2);
	pf->PushCell(g_SortInfo.array_addr);
	pf->PushCell(g_SortInfo.hndl);
	pf->Invoke(&result);

	return result;
}

static cell_t sm_SortCustom1D(IPluginContext *pContext, const cell_t *params)
{
	cell_t *array;
	cell_t array_size = params[2];

	IPluginFunction *pFunction = pContext->GetFunctionById(params[3]);
	if (!pFunction)
	{
		return pContext->ThrowNativeError("Function %x is not a valid function", params[3]);
	}

	pContext->LocalToPhysAddr(params[1], &array);

	sort_info oldinfo = g_SortInfo;

	DetectExceptions eh(pContext);
	g_SortInfo.hndl = params[4];
	g_SortInfo.array_addr = params[1];
	g_SortInfo.array_remap = NULL;
	g_SortInfo.array_base = NULL;
	g_SortInfo.pFunc = pFunction;
	g_SortInfo.eh = &eh;

	qsort(array, array_size, sizeof(cell_t), sort1d_amx_custom);

	g_SortInfo = oldinfo;
	return 1;
}

static int sort2d_amx_custom_legacy(const void *elem1, const void *elem2)
{
	if (g_SortInfo.eh->HasException())
		return 0;

	cell_t c1 = *(cell_t *)elem1;
	cell_t c2 = *(cell_t *)elem2;

	cell_t c1_addr = g_SortInfo.array_addr + (c1 * sizeof(cell_t)) + g_SortInfo.array_remap[c1];
	cell_t c2_addr = g_SortInfo.array_addr + (c2 * sizeof(cell_t)) + g_SortInfo.array_remap[c2];

	IPluginContext *pContext = g_SortInfo.pFunc->GetParentContext();
	cell_t *c1_r, *c2_r;
	pContext->LocalToPhysAddr(c1_addr, &c1_r);
	pContext->LocalToPhysAddr(c2_addr, &c2_r);

	cell_t result = 0;
	g_SortInfo.pFunc->PushCell(c1_addr);
	g_SortInfo.pFunc->PushCell(c2_addr);
	g_SortInfo.pFunc->PushCell(g_SortInfo.array_addr);
	g_SortInfo.pFunc->PushCell(g_SortInfo.hndl);
	g_SortInfo.pFunc->Invoke(&result);

	return result;
}

static cell_t sm_SortCustom2D_Legacy(IPluginContext *pContext, const cell_t *params)
{
	cell_t *array;
	cell_t array_size = params[2];
	IPluginFunction *pFunction;

	pContext->LocalToPhysAddr(params[1], &array);

	if ((pFunction=pContext->GetFunctionById(params[3])) == NULL)
	{
		return pContext->ThrowNativeError("Function %x is not a valid function", params[3]);
	}

	/** back up the old indices, replace the indices with something easier */
	cell_t amx_addr, *phys_addr;
	int err;
	if ((err=pContext->HeapAlloc(array_size, &amx_addr, &phys_addr)) != SP_ERROR_NONE)
	{
		pContext->ThrowNativeErrorEx(err, "Ran out of memory to sort");
		return 0;
	}

	sort_info oldinfo = g_SortInfo;

	DetectExceptions eh(pContext);
	g_SortInfo.pFunc = pFunction;
	g_SortInfo.hndl = params[4];
	g_SortInfo.array_addr = params[1];
	g_SortInfo.eh = &eh;
	
	/** Same process as in strings, back up the old indices for later fixup */
	g_SortInfo.array_base = array;
	g_SortInfo.array_remap = phys_addr;
	
	for (int i=0; i<array_size; i++)
	{
		phys_addr[i] = array[i];
		array[i] = i;
	}

	qsort(array, array_size, sizeof(cell_t), sort2d_amx_custom_legacy);

	/** Fixup process! */
	for (int i=0; i<array_size; i++)
	{
		/* Compute the final address of the old array and subtract the new location.
		 * This is the fixed up distance.
		 */
		array[i] = ((char *)&array[array[i]] + phys_addr[array[i]]) - (char *)&array[i];
	}

	pContext->HeapPop(amx_addr);

	g_SortInfo = oldinfo;
	
	return 1;
}

static int sort2d_amx_custom(const void *elem1, const void *elem2)
{
	if (g_SortInfo.eh->HasException())
		return 0;

	cell_t iv1 = *(cell_t *)elem1;
	cell_t iv2 = *(cell_t *)elem2;

	cell_t result = 0;
	g_SortInfo.pFunc->PushCell(iv1);
	g_SortInfo.pFunc->PushCell(iv2);
	g_SortInfo.pFunc->PushCell(g_SortInfo.array_addr);
	g_SortInfo.pFunc->PushCell(g_SortInfo.hndl);
	g_SortInfo.pFunc->Invoke(&result);

	return result;
}

static cell_t sm_SortCustom2D(IPluginContext *pContext, const cell_t *params)
{
	auto rt = pContext->GetRuntime();
	if (!rt->UsesDirectArrays())
		return sm_SortCustom2D_Legacy(pContext, params);

	cell_t *array;
	cell_t array_size = params[2];
	IPluginFunction *pFunction;

	pContext->LocalToPhysAddr(params[1], &array);

	if ((pFunction=pContext->GetFunctionById(params[3])) == NULL)
	{
		return pContext->ThrowNativeError("Function %x is not a valid function", params[3]);
	}

	sort_info oldinfo = g_SortInfo;

	DetectExceptions eh(pContext);
	g_SortInfo.pFunc = pFunction;
	g_SortInfo.hndl = params[4];
	g_SortInfo.array_addr = params[1];
	g_SortInfo.eh = &eh;
	
	qsort(array, array_size, sizeof(cell_t), sort2d_amx_custom);

	g_SortInfo = oldinfo;
	return 1;
}

enum SortType
{
	Sort_Integer = 0,
	Sort_Float,
	Sort_String,
};

int sort_adtarray_strings_asc(const void *str1, const void *str2)
{
	return strcmp((char *) str1, (char *) str2);
}

int sort_adtarray_strings_desc(const void *str1, const void *str2)
{
	return strcmp((char *) str2, (char *) str1);
}

void sort_adt_random(CellArray *cArray)
{
	size_t arraysize = cArray->size();

	for (int i = arraysize-1; i > 0; i--)
	{
        int n = rand() % (i + 1);

		cArray->swap(i, n);
	}
}

static cell_t sm_SortADTArray(IPluginContext *pContext, const cell_t *params)
{
	CellArray *cArray;
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

	if ((err = handlesys->ReadHandle(params[1], htCellArray, &sec, (void **)&cArray))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error: %d)", params[1], err);
	}

	cell_t order = params[2];
	
	if (order == Sort_Random)
	{
		sort_adt_random(cArray);

		return 1;
	}

	cell_t type = params[3];
	size_t arraysize = cArray->size();
	size_t blocksize = cArray->blocksize();
	cell_t *array = cArray->base();

	if (type == Sort_Integer)
	{
		if (order == Sort_Ascending)
		{
			qsort(array, arraysize, blocksize * sizeof(cell_t), sort_ints_asc);
		}
		else
		{
			qsort(array, arraysize, blocksize * sizeof(cell_t), sort_ints_desc);
		}
	}
	else if (type == Sort_Float)
	{
		if (order == Sort_Ascending)
		{
			qsort(array, arraysize, blocksize * sizeof(cell_t), sort_floats_asc);
		}
		else
		{
			qsort(array, arraysize, blocksize * sizeof(cell_t), sort_floats_desc);
		}
	}
	else if (type == Sort_String)
	{
		if (order == Sort_Ascending)
		{
			qsort(array, arraysize, blocksize * sizeof(cell_t), sort_adtarray_strings_asc);
		}
		else
		{
			qsort(array, arraysize, blocksize * sizeof(cell_t), sort_adtarray_strings_desc);
		}
	}

	return 1;
}

struct sort_infoADT
{
	IPluginFunction *pFunc;
	cell_t *array_base;
	cell_t array_bsize;
	Handle_t array_hndl;
	Handle_t hndl;
	ExceptionHandler *eh;
};

sort_infoADT g_SortInfoADT;

int sort_adtarray_custom(const void *elem1, const void *elem2)
{
	if (g_SortInfoADT.eh->HasException())
		return 0;

	cell_t result = 0;
	IPluginFunction *pf = g_SortInfoADT.pFunc;
	pf->PushCell(((cell_t) ((cell_t *) elem1 - g_SortInfoADT.array_base)) / g_SortInfoADT.array_bsize);
	pf->PushCell(((cell_t) ((cell_t *) elem2 - g_SortInfoADT.array_base)) / g_SortInfoADT.array_bsize);
	pf->PushCell(g_SortInfoADT.array_hndl);
	pf->PushCell(g_SortInfoADT.hndl);
	pf->Invoke(&result);

	return result;
}

static cell_t sm_SortADTArrayCustom(IPluginContext *pContext, const cell_t *params)
{
	CellArray *cArray;
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), g_pCoreIdent);

	if ((err = handlesys->ReadHandle(params[1], htCellArray, &sec, (void **)&cArray))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error: %d)", params[1], err);
	}

	IPluginFunction *pFunction = pContext->GetFunctionById(params[2]);
	if (!pFunction)
	{
		return pContext->ThrowNativeError("Function %x is not a valid function", params[2]);
	}

	size_t arraysize = cArray->size();
	size_t blocksize = cArray->blocksize();
	cell_t *array = cArray->base();

	sort_infoADT oldinfo = g_SortInfoADT;

	DetectExceptions eh(pContext);
	g_SortInfoADT.pFunc = pFunction;
	g_SortInfoADT.array_base = array;
	g_SortInfoADT.array_bsize = (cell_t) blocksize;
	g_SortInfoADT.array_hndl = params[1];
	g_SortInfoADT.hndl = params[3];
	g_SortInfoADT.eh = &eh;
	
	qsort(array, arraysize, blocksize * sizeof(cell_t), sort_adtarray_custom);

	g_SortInfoADT = oldinfo;

	return 1;
}

REGISTER_NATIVES(sortNatives)
{
	{"SortIntegers",            sm_SortIntegers},
	{"SortFloats",              sm_SortFloats},
	{"SortStrings",             sm_SortStrings},
	{"SortCustom1D",            sm_SortCustom1D},
	{"SortCustom2D",            sm_SortCustom2D},
	{"SortADTArray",            sm_SortADTArray},
	{"SortADTArrayCustom",      sm_SortADTArrayCustom},
	
	{"ArrayList.Sort",          sm_SortADTArray},
	{"ArrayList.SortCustom",    sm_SortADTArrayCustom},
	
	{NULL,                      NULL},
};
