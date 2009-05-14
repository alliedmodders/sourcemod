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
#include "common_logic.h"
#include <sm_trie_tpl.h>

HandleType_t htCellTrie;

enum TrieNodeType
{
	TrieNode_Cell,
	TrieNode_CellArray,
	TrieNode_String,
};

struct SmartTrieNode
{
	SmartTrieNode()
	{
		ptr = NULL;
		type = TrieNode_Cell;
	}
	SmartTrieNode(const SmartTrieNode &obj)
	{
		type = obj.type;
		ptr = obj.ptr;
		data = obj.data;
		data_len = obj.data_len;
	}
	SmartTrieNode & operator =(const SmartTrieNode &src)
	{
		type = src.type;
		ptr = src.ptr;
		data = src.data;
		data_len = src.data_len;
		return *this;
	}
	TrieNodeType type;
	cell_t *ptr;
	cell_t data;
	cell_t data_len;
};

struct CellTrie
{
	KTrie<SmartTrieNode> trie;
	cell_t mem_usage;
};

class TrieHelpers : 
	public SMGlobalClass,
	public IHandleTypeDispatch
{
public: //SMGlobalClass
	void OnSourceModAllInitialized()
	{
		htCellTrie = handlesys->CreateType("Trie", this, 0, NULL, NULL, g_pCoreIdent, NULL);
	}
	void OnSourceModShutdown()
	{
		handlesys->RemoveType(htCellTrie, g_pCoreIdent);
	}
public: //IHandleTypeDispatch
	static void DestroySmartTrieNode(SmartTrieNode *pNode)
	{
		free(pNode->ptr);
	}
	void OnHandleDestroy(HandleType_t type, void *object)
	{
		CellTrie *pTrie = (CellTrie *)object;

		pTrie->trie.run_destructor(DestroySmartTrieNode);

		delete pTrie;
	}
	bool GetHandleApproxSize(HandleType_t type, void *object, unsigned int *pSize)
	{
		CellTrie *pArray = (CellTrie *)object;
		*pSize = sizeof(CellTrie) + pArray->mem_usage + pArray->trie.mem_usage();
		return true;
	}
} s_CellTrieHelpers;

static cell_t CreateTrie(IPluginContext *pContext, const cell_t *params)
{
	CellTrie *pTrie = new CellTrie;
	Handle_t hndl;

	pTrie->mem_usage = 0;

	if ((hndl = handlesys->CreateHandle(htCellTrie, pTrie, pContext->GetIdentity(), g_pCoreIdent, NULL))
		== BAD_HANDLE)
	{
		delete pTrie;
		return BAD_HANDLE;
	}

	return hndl;
}

static void UpdateNodeCells(CellTrie *pTrie, SmartTrieNode *pData, const cell_t *cells, cell_t num_cells)
{
	if (num_cells == 1)
	{
		pData->data = *cells;
		pData->type = TrieNode_Cell;
	}
	else
	{
		pData->type = TrieNode_CellArray;
		if (pData->ptr == NULL)
		{
			pData->ptr = (cell_t *)malloc(num_cells * sizeof(cell_t));
			pData->data_len = num_cells;
			pTrie->mem_usage += (pData->data_len * sizeof(cell_t));
		}
		else if (pData->data_len < num_cells)
		{
			pData->ptr = (cell_t *)realloc(pData->ptr, num_cells * sizeof(cell_t));
			pTrie->mem_usage += (num_cells - pData->data_len) * sizeof(cell_t);
			pData->data_len = num_cells;
		}
		if (num_cells != 0)
		{
			memcpy(pData->ptr, cells, sizeof(cell_t) * num_cells);
		}
		pData->data = num_cells;
	}
}

static void UpdateNodeString(CellTrie *pTrie, SmartTrieNode *pData, const char *str)
{
	size_t len = strlen(str);
	cell_t num_cells = (len + sizeof(cell_t)) / sizeof(cell_t);

	if (pData->ptr == NULL)
	{
		pData->ptr = (cell_t *)malloc(num_cells * sizeof(cell_t));
		pData->data_len = num_cells;
		pTrie->mem_usage += (pData->data_len * sizeof(cell_t));
	}
	else if (pData->data_len < num_cells)
	{
		pData->ptr = (cell_t *)realloc(pData->ptr, num_cells * sizeof(cell_t));
		pTrie->mem_usage += (num_cells - pData->data_len) * sizeof(cell_t);
		pData->data_len = num_cells;
	}

	strcpy((char *)pData->ptr, str);
	pData->data = len;
	pData->type = TrieNode_String;
}

static cell_t SetTrieValue(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl;
	CellTrie *pTrie;
	HandleError err;
	HandleSecurity sec = HandleSecurity(pContext->GetIdentity(), g_pCoreIdent);

	hndl = params[1];

	if ((err = handlesys->ReadHandle(hndl, htCellTrie, &sec, (void **)&pTrie))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", hndl, err);
	}

	char *key;
	pContext->LocalToString(params[2], &key);

	SmartTrieNode *pNode;
	if ((pNode = pTrie->trie.retrieve(key)) == NULL)
	{
		SmartTrieNode node;
		UpdateNodeCells(pTrie, &node, &params[3], 1);
		return pTrie->trie.insert(key, node) ? 1 : 0;
	}

	if (!params[4])
	{
		return 0;
	}

	UpdateNodeCells(pTrie, pNode, &params[3], 1);

	return 1;
}

static cell_t SetTrieArray(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl;
	CellTrie *pTrie;
	HandleError err;
	HandleSecurity sec = HandleSecurity(pContext->GetIdentity(), g_pCoreIdent);

	hndl = params[1];

	if ((err = handlesys->ReadHandle(hndl, htCellTrie, &sec, (void **)&pTrie))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", hndl, err);
	}

	if (params[4] < 0)
	{
		return pContext->ThrowNativeError("Invalid array size: %d", params[4]);
	}

	char *key;
	cell_t *array;
	pContext->LocalToString(params[2], &key);
	pContext->LocalToPhysAddr(params[3], &array);

	SmartTrieNode *pNode;
	if ((pNode = pTrie->trie.retrieve(key)) == NULL)
	{
		SmartTrieNode node;
		UpdateNodeCells(pTrie, &node, array, params[4]);
		if (!pTrie->trie.insert(key, node))
		{
			free(node.ptr);
			return 0;
		}
		return 1;
	}

	if (!params[4])
	{
		return 0;
	}

	UpdateNodeCells(pTrie, pNode, array, params[4]);

	return 1;
}

static cell_t SetTrieString(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl;
	CellTrie *pTrie;
	HandleError err;
	HandleSecurity sec = HandleSecurity(pContext->GetIdentity(), g_pCoreIdent);

	hndl = params[1];

	if ((err = handlesys->ReadHandle(hndl, htCellTrie, &sec, (void **)&pTrie))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", hndl, err);
	}

	char *key, *val;
	pContext->LocalToString(params[2], &key);
	pContext->LocalToString(params[3], &val);

	SmartTrieNode *pNode;
	if ((pNode = pTrie->trie.retrieve(key)) == NULL)
	{
		SmartTrieNode node;
		UpdateNodeString(pTrie, &node, val);
		if (!pTrie->trie.insert(key, node))
		{
			free(node.ptr);
			return 0;
		}
		return 1;
	}

	if (!params[4])
	{
		return 0;
	}

	UpdateNodeString(pTrie, pNode, val);

	return 1;
}

static cell_t RemoveFromTrie(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl;
	CellTrie *pTrie;
	HandleError err;
	HandleSecurity sec = HandleSecurity(pContext->GetIdentity(), g_pCoreIdent);

	hndl = params[1];

	if ((err = handlesys->ReadHandle(hndl, htCellTrie, &sec, (void **)&pTrie))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", hndl, err);
	}

	char *key;
	pContext->LocalToString(params[2], &key);

	SmartTrieNode *pNode;
	if ((pNode = pTrie->trie.retrieve(key)) == NULL)
	{
		return 0;
	}

	free(pNode->ptr);
	pNode->ptr = NULL;

	return pTrie->trie.remove(key) ? 1 : 0;
}

static cell_t ClearTrie(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl;
	CellTrie *pTrie;
	HandleError err;
	HandleSecurity sec = HandleSecurity(pContext->GetIdentity(), g_pCoreIdent);

	hndl = params[1];

	if ((err = handlesys->ReadHandle(hndl, htCellTrie, &sec, (void **)&pTrie))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", hndl, err);
	}

	pTrie->trie.run_destructor(TrieHelpers::DestroySmartTrieNode);
	pTrie->trie.clear();

	return 1;
}

static cell_t GetTrieValue(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl;
	CellTrie *pTrie;
	HandleError err;
	HandleSecurity sec = HandleSecurity(pContext->GetIdentity(), g_pCoreIdent);

	hndl = params[1];

	if ((err = handlesys->ReadHandle(hndl, htCellTrie, &sec, (void **)&pTrie))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", hndl, err);
	}

	char *key;
	cell_t *pValue;
	pContext->LocalToString(params[2], &key);
	pContext->LocalToPhysAddr(params[3], &pValue);

	SmartTrieNode *pNode;
	if ((pNode = pTrie->trie.retrieve(key)) == NULL)
	{
		return 0;
	}

	if (pNode->type == TrieNode_Cell)
	{
		*pValue = pNode->data;
		return 1;
	}
	
	return 0;
}

static cell_t GetTrieArray(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl;
	CellTrie *pTrie;
	HandleError err;
	HandleSecurity sec = HandleSecurity(pContext->GetIdentity(), g_pCoreIdent);

	hndl = params[1];

	if ((err = handlesys->ReadHandle(hndl, htCellTrie, &sec, (void **)&pTrie))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", hndl, err);
	}

	if (params[4] < 0)
	{
		return pContext->ThrowNativeError("Invalid array size: %d", params[4]);
	}

	char *key;
	cell_t *pValue, *pSize;
	pContext->LocalToString(params[2], &key);
	pContext->LocalToPhysAddr(params[3], &pValue);
	pContext->LocalToPhysAddr(params[5], &pSize);

	SmartTrieNode *pNode;
	if ((pNode = pTrie->trie.retrieve(key)) == NULL
		|| pNode->type != TrieNode_CellArray)
	{
		return 0;
	}

	if (pNode->ptr == NULL)
	{
		*pSize = 0;
		return 1;
	}

	if (pNode->data > params[4])
	{
		*pSize = params[4];
	}
	else if (params[4] != 0)
	{
		*pSize = pNode->data;
	}
	else
	{
		return 1;
	}

	memcpy(pValue, pNode->ptr, sizeof(cell_t) * pSize[0]);

	return 1;
}

static cell_t GetTrieString(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl;
	CellTrie *pTrie;
	HandleError err;
	HandleSecurity sec = HandleSecurity(pContext->GetIdentity(), g_pCoreIdent);

	hndl = params[1];

	if ((err = handlesys->ReadHandle(hndl, htCellTrie, &sec, (void **)&pTrie))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", hndl, err);
	}

	if (params[4] < 0)
	{
		return pContext->ThrowNativeError("Invalid buffer size: %d", params[4]);
	}

	char *key;
	cell_t *pSize;
	pContext->LocalToString(params[2], &key);
	pContext->LocalToPhysAddr(params[5], &pSize);

	SmartTrieNode *pNode;
	if ((pNode = pTrie->trie.retrieve(key)) == NULL
		|| pNode->type != TrieNode_String)
	{
		return 0;
	}

	if (pNode->ptr == NULL)
	{
		*pSize = 0;
		pContext->StringToLocal(params[3], params[4], "");
		return 1;
	}

	size_t written;
	pContext->StringToLocalUTF8(params[3], params[4], (char *)pNode->ptr, &written);

	*pSize = (cell_t)written;

	return 1;
}

static cell_t GetTrieSize(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl;
	CellTrie *pTrie;
	HandleError err;
	HandleSecurity sec = HandleSecurity(pContext->GetIdentity(), g_pCoreIdent);

	hndl = params[1];

	if ((err = handlesys->ReadHandle(hndl, htCellTrie, &sec, (void **)&pTrie))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", hndl, err);
	}

	return pTrie->trie.size();
}

REGISTER_NATIVES(trieNatives)
{
	{"ClearTrie",				ClearTrie},
	{"CreateTrie",				CreateTrie},
	{"GetTrieArray",			GetTrieArray},
	{"GetTrieString",			GetTrieString},
	{"GetTrieValue",			GetTrieValue},
	{"RemoveFromTrie",			RemoveFromTrie},
	{"SetTrieArray",			SetTrieArray},
	{"SetTrieString",			SetTrieString},
	{"SetTrieValue",			SetTrieValue},
	{"GetTrieSize",				GetTrieSize},
	{NULL,						NULL},
};
