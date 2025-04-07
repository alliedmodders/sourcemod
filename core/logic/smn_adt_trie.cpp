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

#include <memory>

#include "common_logic.h"
#include <am-refcounting.h>
#include <sm_hashmap.h>
#include "sm_memtable.h"
#include <IHandleSys.h>

HandleType_t htCellTrie;
HandleType_t htSnapshot;
HandleType_t htIntCellTrie;
HandleType_t htIntSnapshot;

enum EntryType
{
	EntryType_Cell,
	EntryType_CellArray,
	EntryType_String,
};

class Entry
{
	struct ArrayInfo
	{
		size_t length;
		size_t maxbytes;

		void *base() {
			return this + 1;
		}
	};

public:
	Entry()
		: control_(0)
	{
	}
	Entry(Entry &&other)
	{
		control_ = other.control_;
		data_ = other.data_;
		other.control_ = 0;
	}

	~Entry()
	{
		free(raw());
	}

	void setCell(cell_t value) {
		setType(EntryType_Cell);
		data_ = value;
	}
	void setArray(cell_t *cells, size_t length) {
		ArrayInfo *array = ensureArray(length * sizeof(cell_t));
		array->length = length;
		memcpy(array->base(), cells, length * sizeof(cell_t));
		setTypeAndPointer(EntryType_CellArray, array);
	}
	void setString(const char *str) {
		size_t length = strlen(str);
		ArrayInfo *array = ensureArray(length + 1);
		array->length = length;
		strcpy((char *)array->base(), str);
		setTypeAndPointer(EntryType_String, array);
	}

	size_t arrayLength() const {
		assert(isArray());
		return raw()->length;
	}
	cell_t *array() const {
		assert(isArray());
		return reinterpret_cast<cell_t *>(raw()->base());
	}
	char *c_str() const {
		assert(isString());
		return reinterpret_cast<char *>(raw()->base());
	}
	cell_t cell() const {
		assert(isCell());
		return data_;
	}

	bool isCell() const {
		return type() == EntryType_Cell;
	}
	bool isArray() const {
		return type() == EntryType_CellArray;
	}
	bool isString() const {
		return type() == EntryType_String;
	}

private:
	Entry(const Entry &other) = delete;

	ArrayInfo *ensureArray(size_t bytes) {
		ArrayInfo *array = raw();
		if (array && array->maxbytes >= bytes)
			return array;
		array = (ArrayInfo *)realloc(array, bytes + sizeof(ArrayInfo));
		if (!array)
		{
			fprintf(stderr, "Out of memory!\n");
			abort();
		}
		array->maxbytes = bytes;
		return array;
	}

	// Pointer and type are overlaid, so we have some accessors.
	ArrayInfo *raw() const {
		return reinterpret_cast<ArrayInfo *>(control_ & ~uintptr_t(0x3));
	}
	void setType(EntryType aType) {
		control_ = uintptr_t(raw()) | uintptr_t(aType);
		assert(type() == aType);
	}
	void setTypeAndPointer(EntryType aType, ArrayInfo *ptr) {
		// malloc() should guarantee 8-byte alignment at worst
		assert((uintptr_t(ptr) & 0x3) == 0);
		control_ = uintptr_t(ptr) | uintptr_t(aType);
		assert(type() == aType);
	}
	EntryType type() const {
		return (EntryType)(control_ & 0x3);
	}

private:
	// Contains the bits for the type, and an array pointer, if one is set.
	uintptr_t control_;

	// Contains data for cell-only entries.
	cell_t data_;
};

struct CellTrie
{
	StringHashMap<Entry> map;
};

struct IntCellTrie
{
	IntHashMap<Entry> map;
};

struct TrieSnapshot
{
	TrieSnapshot()
		: strings(128)
	{ }

	size_t mem_usage()
	{
		return length * sizeof(int) + strings.GetMemTable()->GetMemUsage();
	}

	size_t length;
	std::unique_ptr<int[]> keys;
	BaseStringTable strings;
};

struct IntTrieSnapshot
{
	IntTrieSnapshot() {}

	size_t mem_usage()
	{
		return length * sizeof(int);
	}

	size_t length;
	std::unique_ptr<int[]> keys;
};

class TrieHelpers : 
	public SMGlobalClass,
	public IHandleTypeDispatch
{
public: //SMGlobalClass
	void OnSourceModAllInitialized()
	{
		htCellTrie = handlesys->CreateType("Trie", this, 0, NULL, NULL, g_pCoreIdent, NULL);
		htSnapshot = handlesys->CreateType("TrieSnapshot", this, 0, NULL, NULL, g_pCoreIdent, NULL);
		htIntCellTrie = handlesys->CreateType("IntTrie", this, 0, NULL, NULL, g_pCoreIdent, NULL);
		htIntSnapshot = handlesys->CreateType("IntTrieSnapshot", this, 0, NULL, NULL, g_pCoreIdent, NULL);
	}
	void OnSourceModShutdown()
	{
		handlesys->RemoveType(htSnapshot, g_pCoreIdent);
		handlesys->RemoveType(htCellTrie, g_pCoreIdent);
		handlesys->RemoveType(htIntSnapshot, g_pCoreIdent);
		handlesys->RemoveType(htIntCellTrie, g_pCoreIdent);
	}
public: //IHandleTypeDispatch
	void OnHandleDestroy(HandleType_t type, void *object)
	{
		if (type == htCellTrie)
		{
			delete (CellTrie *)object;
		}
		else if (type == htSnapshot) 
		{
			TrieSnapshot *snapshot = (TrieSnapshot *)object;
			delete snapshot;
		}
		else if (type == htIntCellTrie)
		{
			delete (IntCellTrie *)object;
		}
		else if (type == htIntSnapshot) 
		{
			IntTrieSnapshot *snapshot = (IntTrieSnapshot *)object;
			delete snapshot;
		}
	}
	bool GetHandleApproxSize(HandleType_t type, void *object, unsigned int *pSize)
	{
		if (type == htCellTrie)
		{
			CellTrie *pArray = (CellTrie *)object;
			*pSize = sizeof(CellTrie) + pArray->map.mem_usage();
			return true;
		}
		else if (type == htSnapshot)
		{
			TrieSnapshot *snapshot = (TrieSnapshot *)object;
			*pSize = sizeof(TrieSnapshot) + snapshot->mem_usage();
			return true;
		}
		else if (type == htIntCellTrie)
		{
			IntCellTrie *pArray = (IntCellTrie *)object;
			*pSize = sizeof(IntCellTrie) + pArray->map.mem_usage();
			return true;
		}
		else if (type == htIntSnapshot)
		{
			IntTrieSnapshot *snapshot = (IntTrieSnapshot *)object;
			*pSize = sizeof(IntTrieSnapshot) + snapshot->mem_usage();
			return true;
		}
		
		return false;
	}
} s_CellTrieHelpers;

static cell_t CreateTrie(IPluginContext *pContext, const cell_t *params)
{
	CellTrie *pTrie = new CellTrie;
	Handle_t hndl;

	if ((hndl = handlesys->CreateHandle(htCellTrie, pTrie, pContext->GetIdentity(), g_pCoreIdent, NULL))
		== BAD_HANDLE)
	{
		delete pTrie;
		return BAD_HANDLE;
	}

	return hndl;
}

static cell_t CreateIntTrie(IPluginContext *pContext, const cell_t *params)
{
	IntCellTrie *pTrie = new IntCellTrie;
	Handle_t hndl;

	if ((hndl = handlesys->CreateHandle(htIntCellTrie, pTrie, pContext->GetIdentity(), g_pCoreIdent, NULL))
		== BAD_HANDLE)
	{
		delete pTrie;
		return BAD_HANDLE;
	}

	return hndl;
}

static cell_t SetTrieValue(IPluginContext *pContext, const cell_t *params)
{
	CellTrie *pTrie;
	HandleError err;
	HandleSecurity sec = HandleSecurity(pContext->GetIdentity(), g_pCoreIdent);

	Handle_t hndl = params[1];

	if ((err = handlesys->ReadHandle(hndl, htCellTrie, &sec, (void **)&pTrie))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", hndl, err);
	}

	char *key;
	pContext->LocalToString(params[2], &key);

	StringHashMap<Entry>::Insert i = pTrie->map.findForAdd(key);
	if (!i.found())
	{
		if (!pTrie->map.add(i, key))
			return 0;
		i->value.setCell(params[3]);
		return 1;
	}

	if (!params[4])
		return 0;

	i->value.setCell(params[3]);
	return 1;
}

static cell_t SetIntTrieValue(IPluginContext *pContext, const cell_t *params)
{
	IntCellTrie *pTrie;
	HandleError err;
	HandleSecurity sec = HandleSecurity(pContext->GetIdentity(), g_pCoreIdent);

	Handle_t hndl = params[1];

	if ((err = handlesys->ReadHandle(hndl, htIntCellTrie, &sec, (void **)&pTrie))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", hndl, err);
	}

	int32_t key = params[2];

	IntHashMap<Entry>::Insert i = pTrie->map.findForAdd(key);
	if (!i.found())
	{
		if (!pTrie->map.add(i, key))
			return 0;
		i->value.setCell(params[3]);
		return 1;
	}

	if (!params[4])
		return 0;

	i->value.setCell(params[3]);
	return 1;
}

static cell_t SetTrieArray(IPluginContext *pContext, const cell_t *params)
{
	CellTrie *pTrie;
	HandleError err;
	HandleSecurity sec = HandleSecurity(pContext->GetIdentity(), g_pCoreIdent);

	Handle_t hndl = params[1];

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

	StringHashMap<Entry>::Insert i = pTrie->map.findForAdd(key);
	if (!i.found())
	{
		if (!pTrie->map.add(i, key))
			return 0;
		i->key = key;
		i->value.setArray(array, params[4]);
		return 1;
	}

	if (!params[5])
		return 0;

	i->value.setArray(array, params[4]);
	return 1;
}

static cell_t SetIntTrieArray(IPluginContext *pContext, const cell_t *params)
{
	IntCellTrie *pTrie;
	HandleError err;
	HandleSecurity sec = HandleSecurity(pContext->GetIdentity(), g_pCoreIdent);

	Handle_t hndl = params[1];

	if ((err = handlesys->ReadHandle(hndl, htIntCellTrie, &sec, (void **)&pTrie))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", hndl, err);
	}

	if (params[4] < 0)
	{
		return pContext->ThrowNativeError("Invalid array size: %d", params[4]);
	}

	int32_t key = params[2];
	cell_t *array;
	pContext->LocalToPhysAddr(params[3], &array);

	IntHashMap<Entry>::Insert i = pTrie->map.findForAdd(key);
	if (!i.found())
	{
		if (!pTrie->map.add(i, key))
			return 0;
		i->key = key;
		i->value.setArray(array, params[4]);
		return 1;
	}

	if (!params[5])
		return 0;

	i->value.setArray(array, params[4]);
	return 1;
}

static cell_t SetTrieString(IPluginContext *pContext, const cell_t *params)
{
	CellTrie *pTrie;
	HandleError err;
	HandleSecurity sec = HandleSecurity(pContext->GetIdentity(), g_pCoreIdent);

	Handle_t hndl = params[1];

	if ((err = handlesys->ReadHandle(hndl, htCellTrie, &sec, (void **)&pTrie))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", hndl, err);
	}

	char *key, *val;
	pContext->LocalToString(params[2], &key);
	pContext->LocalToString(params[3], &val);

	StringHashMap<Entry>::Insert i = pTrie->map.findForAdd(key);
	if (!i.found())
	{
		if (!pTrie->map.add(i, key))
			return 0;
		i->value.setString(val);
		return 1;
	}

	if (!params[4])
		return 0;

	i->value.setString(val);
	return 1;
}

static cell_t SetIntTrieString(IPluginContext *pContext, const cell_t *params)
{
	IntCellTrie *pTrie;
	HandleError err;
	HandleSecurity sec = HandleSecurity(pContext->GetIdentity(), g_pCoreIdent);

	Handle_t hndl = params[1];

	if ((err = handlesys->ReadHandle(hndl, htIntCellTrie, &sec, (void **)&pTrie))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", hndl, err);
	}

	int32_t key = params[2];
	char *val;
	pContext->LocalToString(params[3], &val);

	IntHashMap<Entry>::Insert i = pTrie->map.findForAdd(key);
	if (!i.found())
	{
		if (!pTrie->map.add(i, key))
			return 0;
		i->value.setString(val);
		return 1;
	}

	if (!params[4])
		return 0;

	i->value.setString(val);
	return 1;
}

static cell_t ContainsKeyInTrie(IPluginContext *pContext, const cell_t *params)
{
	CellTrie *pTrie;
	HandleError err;
	HandleSecurity sec = HandleSecurity(pContext->GetIdentity(), g_pCoreIdent);

	Handle_t hndl = params[1];

	if ((err = handlesys->ReadHandle(hndl, htCellTrie, &sec, (void **)&pTrie)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", hndl, err);
	}

	char *key;
	pContext->LocalToString(params[2], &key);

	StringHashMap<Entry>::Result r = pTrie->map.find(key);

	return r.found() ? 1 : 0;
}

static cell_t ContainsKeyInIntTrie(IPluginContext *pContext, const cell_t *params)
{
	IntCellTrie *pTrie;
	HandleError err;
	HandleSecurity sec = HandleSecurity(pContext->GetIdentity(), g_pCoreIdent);

	Handle_t hndl = params[1];

	if ((err = handlesys->ReadHandle(hndl, htIntCellTrie, &sec, (void **)&pTrie)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", hndl, err);
	}

	int32_t key = params[2];

	IntHashMap<Entry>::Result r = pTrie->map.find(key);

	return r.found() ? 1 : 0;
}

static cell_t RemoveFromTrie(IPluginContext *pContext, const cell_t *params)
{
	CellTrie *pTrie;
	HandleError err;
	HandleSecurity sec = HandleSecurity(pContext->GetIdentity(), g_pCoreIdent);

	Handle_t hndl = params[1];

	if ((err = handlesys->ReadHandle(hndl, htCellTrie, &sec, (void **)&pTrie))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", hndl, err);
	}

	char *key;
	pContext->LocalToString(params[2], &key);

	StringHashMap<Entry>::Result r = pTrie->map.find(key);
	if (!r.found())
		return 0;

	pTrie->map.remove(r);
	return 1;
}

static cell_t RemoveFromIntTrie(IPluginContext *pContext, const cell_t *params)
{
	IntCellTrie *pTrie;
	HandleError err;
	HandleSecurity sec = HandleSecurity(pContext->GetIdentity(), g_pCoreIdent);

	Handle_t hndl = params[1];

	if ((err = handlesys->ReadHandle(hndl, htIntCellTrie, &sec, (void **)&pTrie))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", hndl, err);
	}

	int32_t key = params[2];

	IntHashMap<Entry>::Result r = pTrie->map.find(key);
	if (!r.found())
		return 0;

	pTrie->map.remove(r);
	return 1;
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

	pTrie->map.clear();
	return 1;
}

static cell_t ClearIntTrie(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl;
	IntCellTrie *pTrie;
	HandleError err;
	HandleSecurity sec = HandleSecurity(pContext->GetIdentity(), g_pCoreIdent);

	hndl = params[1];

	if ((err = handlesys->ReadHandle(hndl, htIntCellTrie, &sec, (void **)&pTrie))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", hndl, err);
	}

	pTrie->map.clear();
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

	StringHashMap<Entry>::Result r = pTrie->map.find(key);
	if (!r.found())
		return 0;

	if (r->value.isCell())
	{
		*pValue = r->value.cell();
		return 1;
	}

	// Maintain compatibility with an old bug. If an array was set with one
	// cell, it was stored internally as a single cell. We now store as an
	// actual array, but we make GetTrieValue() still work for this case.
	if (r->value.isArray() && r->value.arrayLength() == 1)
	{
		*pValue = r->value.array()[0];
		return 1;
	}
	
	return 0;
}

static cell_t GetIntTrieValue(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl;
	IntCellTrie *pTrie;
	HandleError err;
	HandleSecurity sec = HandleSecurity(pContext->GetIdentity(), g_pCoreIdent);

	hndl = params[1];

	if ((err = handlesys->ReadHandle(hndl, htIntCellTrie, &sec, (void **)&pTrie))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", hndl, err);
	}

	int32_t key = params[2];
	cell_t *pValue;
	pContext->LocalToPhysAddr(params[3], &pValue);

	IntHashMap<Entry>::Result r = pTrie->map.find(key);
	if (!r.found())
		return 0;

	if (r->value.isCell())
	{
		*pValue = r->value.cell();
		return 1;
	}

	// Maintain compatibility with an old bug. If an array was set with one
	// cell, it was stored internally as a single cell. We now store as an
	// actual array, but we make GetTrieValue() still work for this case.
	if (r->value.isArray() && r->value.arrayLength() == 1)
	{
		*pValue = r->value.array()[0];
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


	StringHashMap<Entry>::Result r = pTrie->map.find(key);
	if (!r.found() || !r->value.isArray())
		return 0;

	if (!r->value.array())
	{
		*pSize = 0;
		return 1;
	}

	if (!params[4])
		return 1;

	size_t length = r->value.arrayLength();
	cell_t *base = r->value.array();

	if (length > size_t(params[4]))
		*pSize = params[4];
	else
		*pSize = length;

	memcpy(pValue, base, sizeof(cell_t) * pSize[0]);
	return 1;
}

static cell_t GetIntTrieArray(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl;
	IntCellTrie *pTrie;
	HandleError err;
	HandleSecurity sec = HandleSecurity(pContext->GetIdentity(), g_pCoreIdent);

	hndl = params[1];

	if ((err = handlesys->ReadHandle(hndl, htIntCellTrie, &sec, (void **)&pTrie))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", hndl, err);
	}

	if (params[4] < 0)
	{
		return pContext->ThrowNativeError("Invalid array size: %d", params[4]);
	}

	int32_t key = params[2];
	cell_t *pValue, *pSize;
	pContext->LocalToPhysAddr(params[3], &pValue);
	pContext->LocalToPhysAddr(params[5], &pSize);

	IntHashMap<Entry>::Result r = pTrie->map.find(key);
	if (!r.found() || !r->value.isArray())
		return 0;

	if (!r->value.array())
	{
		*pSize = 0;
		return 1;
	}

	if (!params[4])
		return 1;

	size_t length = r->value.arrayLength();
	cell_t *base = r->value.array();

	if (length > size_t(params[4]))
		*pSize = params[4];
	else
		*pSize = length;

	memcpy(pValue, base, sizeof(cell_t) * pSize[0]);
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

	StringHashMap<Entry>::Result r = pTrie->map.find(key);
	if (!r.found() || !r->value.isString())
		return 0;

	size_t written;
	pContext->StringToLocalUTF8(params[3], params[4], r->value.c_str(), &written);

	*pSize = (cell_t)written;
	return 1;
}

static cell_t GetIntTrieString(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl;
	IntCellTrie *pTrie;
	HandleError err;
	HandleSecurity sec = HandleSecurity(pContext->GetIdentity(), g_pCoreIdent);

	hndl = params[1];

	if ((err = handlesys->ReadHandle(hndl, htIntCellTrie, &sec, (void **)&pTrie))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", hndl, err);
	}

	if (params[4] < 0)
	{
		return pContext->ThrowNativeError("Invalid buffer size: %d", params[4]);
	}

	int32_t key = params[2];
	cell_t *pSize;
	pContext->LocalToPhysAddr(params[5], &pSize);

	IntHashMap<Entry>::Result r = pTrie->map.find(key);
	if (!r.found() || !r->value.isString())
		return 0;

	size_t written;
	pContext->StringToLocalUTF8(params[3], params[4], r->value.c_str(), &written);

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

	return pTrie->map.elements();
}

static cell_t GetIntTrieSize(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl;
	IntCellTrie *pTrie;
	HandleError err;
	HandleSecurity sec = HandleSecurity(pContext->GetIdentity(), g_pCoreIdent);

	hndl = params[1];

	if ((err = handlesys->ReadHandle(hndl, htIntCellTrie, &sec, (void **)&pTrie))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", hndl, err);
	}

	return pTrie->map.elements();
}

static cell_t CreateTrieSnapshot(IPluginContext *pContext, const cell_t *params)
{
	HandleError err;
	HandleSecurity sec = HandleSecurity(pContext->GetIdentity(), g_pCoreIdent);

	Handle_t hndl = params[1];

	CellTrie *pTrie;
	if ((err = handlesys->ReadHandle(hndl, htCellTrie, &sec, (void **)&pTrie))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", hndl, err);
	}

	TrieSnapshot *snapshot = new TrieSnapshot;
	snapshot->length = pTrie->map.elements();
	snapshot->keys = std::make_unique<int[]>(snapshot->length);
	size_t i = 0;
	for (StringHashMap<Entry>::iterator iter = pTrie->map.iter(); !iter.empty(); iter.next(), i++)
		 snapshot->keys[i] = snapshot->strings.AddString(iter->key.c_str(), iter->key.length());
	assert(i == snapshot->length);

	if ((hndl = handlesys->CreateHandle(htSnapshot, snapshot, pContext->GetIdentity(), g_pCoreIdent, NULL))
		== BAD_HANDLE)
	{
		delete snapshot;
		return BAD_HANDLE;
	}

	return hndl;
}

static cell_t CreateIntTrieSnapshot(IPluginContext *pContext, const cell_t *params)
{
	HandleError err;
	HandleSecurity sec = HandleSecurity(pContext->GetIdentity(), g_pCoreIdent);

	Handle_t hndl = params[1];

	IntCellTrie *pTrie;
	if ((err = handlesys->ReadHandle(hndl, htIntCellTrie, &sec, (void **)&pTrie))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", hndl, err);
	}

	IntTrieSnapshot *snapshot = new IntTrieSnapshot;
	snapshot->length = pTrie->map.elements();
	snapshot->keys = std::make_unique<int[]>(snapshot->length);
	size_t i = 0;
	for (IntHashMap<Entry>::iterator iter = pTrie->map.iter(); !iter.empty(); iter.next(), i++)
		snapshot->keys[i] = iter->key;
	assert(i == snapshot->length);

	if ((hndl = handlesys->CreateHandle(htIntSnapshot, snapshot, pContext->GetIdentity(), g_pCoreIdent, NULL))
		== BAD_HANDLE)
	{
		delete snapshot;
		return BAD_HANDLE;
	}

	return hndl;
}

static cell_t TrieSnapshotLength(IPluginContext *pContext, const cell_t *params)
{
	HandleError err;
	HandleSecurity sec = HandleSecurity(pContext->GetIdentity(), g_pCoreIdent);

	Handle_t hndl = params[1];

	TrieSnapshot *snapshot;
	if ((err = handlesys->ReadHandle(hndl, htSnapshot, &sec, (void **)&snapshot))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", hndl, err);
	}

	return snapshot->length;
}

static cell_t IntTrieSnapshotLength(IPluginContext *pContext, const cell_t *params)
{
	HandleError err;
	HandleSecurity sec = HandleSecurity(pContext->GetIdentity(), g_pCoreIdent);

	Handle_t hndl = params[1];

	IntTrieSnapshot *snapshot;
	if ((err = handlesys->ReadHandle(hndl, htIntSnapshot, &sec, (void **)&snapshot))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", hndl, err);
	}

	return snapshot->length;
}

static cell_t TrieSnapshotKeyBufferSize(IPluginContext *pContext, const cell_t *params)
{
	HandleError err;
	HandleSecurity sec = HandleSecurity(pContext->GetIdentity(), g_pCoreIdent);

	Handle_t hndl = params[1];

	TrieSnapshot *snapshot;
	if ((err = handlesys->ReadHandle(hndl, htSnapshot, &sec, (void **)&snapshot))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", hndl, err);
	}

	unsigned index = params[2];
	if (index >= snapshot->length)
		return pContext->ThrowNativeError("Invalid index %d", index);

	return strlen(snapshot->strings.GetString(snapshot->keys[index])) + 1;
}

static cell_t GetTrieSnapshotKey(IPluginContext *pContext, const cell_t *params)
{
	HandleError err;
	HandleSecurity sec = HandleSecurity(pContext->GetIdentity(), g_pCoreIdent);

	Handle_t hndl = params[1];

	TrieSnapshot *snapshot;
	if ((err = handlesys->ReadHandle(hndl, htSnapshot, &sec, (void **)&snapshot))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", hndl, err);
	}

	unsigned index = params[2];
	if (index >= snapshot->length)
		return pContext->ThrowNativeError("Invalid index %d", index);

	size_t written;
	const char *str = snapshot->strings.GetString(snapshot->keys[index]);
	pContext->StringToLocalUTF8(params[3], params[4], str, &written);
	return written;
}

static cell_t GetIntTrieSnapshotKey(IPluginContext *pContext, const cell_t *params)
{
	HandleError err;
	HandleSecurity sec = HandleSecurity(pContext->GetIdentity(), g_pCoreIdent);

	Handle_t hndl = params[1];

	IntTrieSnapshot *snapshot;
	if ((err = handlesys->ReadHandle(hndl, htIntSnapshot, &sec, (void **)&snapshot))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", hndl, err);
	}

	unsigned index = params[2];
	if (index >= snapshot->length)
		return pContext->ThrowNativeError("Invalid index %d", index);

	return snapshot->keys[index];
}

static cell_t CloneTrie(IPluginContext *pContext, const cell_t *params)
{
	HandleError err;
	HandleSecurity sec = HandleSecurity(pContext->GetIdentity(), g_pCoreIdent);

	CellTrie *pOldTrie;
	if ((err = handlesys->ReadHandle(params[1], htCellTrie, &sec, (void **)&pOldTrie))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", params[1], err);
	}

	CellTrie *pNewTrie = new CellTrie;
	Handle_t hndl = handlesys->CreateHandle(htCellTrie, pNewTrie, pContext->GetIdentity(), g_pCoreIdent, NULL);
	if (!hndl)
	{
		delete pNewTrie;
		return hndl;
	}

	for (StringHashMap<Entry>::iterator it = pOldTrie->map.iter(); !it.empty(); it.next())
	{
		const char *key = it->key.c_str();
		StringHashMap<Entry>::Insert insert = pNewTrie->map.findForAdd(key);
		if (pNewTrie->map.add(insert, key))
		{
			StringHashMap<Entry>::Result result = pOldTrie->map.find(key);
			if (result->value.isCell())
			{
				insert->value.setCell(result->value.cell());
			}
			else if (result->value.isString())
			{
				insert->value.setString(result->value.c_str());
			}
			else if (result->value.isArray())
			{
				insert->value.setArray(result->value.array(), result->value.arrayLength());
			}
			else
			{
				handlesys->FreeHandle(hndl, NULL);
				return pContext->ThrowNativeError("Unhandled data type encountered, file a bug and reference pr #852");
			}
		}
	}

	return hndl;
}

static cell_t CloneIntTrie(IPluginContext *pContext, const cell_t *params)
{
	HandleError err;
	HandleSecurity sec = HandleSecurity(pContext->GetIdentity(), g_pCoreIdent);

	IntCellTrie *pOldTrie;
	if ((err = handlesys->ReadHandle(params[1], htIntCellTrie, &sec, (void **)&pOldTrie))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", params[1], err);
	}

	IntCellTrie *pNewTrie = new IntCellTrie;
	Handle_t hndl = handlesys->CreateHandle(htIntCellTrie, pNewTrie, pContext->GetIdentity(), g_pCoreIdent, NULL);
	if (!hndl)
	{
		delete pNewTrie;
		return hndl;
	}

	for (IntHashMap<Entry>::iterator it = pOldTrie->map.iter(); !it.empty(); it.next())
	{
		int32_t key = it->key;
		IntHashMap<Entry>::Insert insert = pNewTrie->map.findForAdd(key);
		if (pNewTrie->map.add(insert, key))
		{
			IntHashMap<Entry>::Result result = pOldTrie->map.find(key);
			if (result->value.isCell())
			{
				insert->value.setCell(result->value.cell());
			}
			else if (result->value.isString())
			{
				insert->value.setString(result->value.c_str());
			}
			else if (result->value.isArray())
			{
				insert->value.setArray(result->value.array(), result->value.arrayLength());
			}
			else
			{
				handlesys->FreeHandle(hndl, NULL);
				return pContext->ThrowNativeError("Unhandled data type encountered, file a bug and reference pr #852");
			}
		}
	}

	return hndl;
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

	{"CreateTrieSnapshot",		CreateTrieSnapshot},
	{"TrieSnapshotLength",		TrieSnapshotLength},
	{"TrieSnapshotKeyBufferSize", TrieSnapshotKeyBufferSize},
	{"GetTrieSnapshotKey",		GetTrieSnapshotKey},

	// Transitional syntax support.
	{"StringMap.StringMap",		CreateTrie},
	{"StringMap.Clear",			ClearTrie},
	{"StringMap.GetArray",		GetTrieArray},
	{"StringMap.GetString",		GetTrieString},
	{"StringMap.GetValue",		GetTrieValue},
	{"StringMap.ContainsKey",	ContainsKeyInTrie},
	{"StringMap.Remove",		RemoveFromTrie},
	{"StringMap.SetArray",		SetTrieArray},
	{"StringMap.SetString",		SetTrieString},
	{"StringMap.SetValue",		SetTrieValue},
	{"StringMap.Size.get",		GetTrieSize},
	{"StringMap.Snapshot",		CreateTrieSnapshot},
	{"StringMap.Clone",			CloneTrie},

	{"IntMap.IntMap",			CreateIntTrie},
	{"IntMap.Clear",			ClearIntTrie},
	{"IntMap.GetArray",			GetIntTrieArray},
	{"IntMap.GetString",		GetIntTrieString},
	{"IntMap.GetValue",			GetIntTrieValue},
	{"IntMap.ContainsKey",		ContainsKeyInIntTrie},
	{"IntMap.Remove",			RemoveFromIntTrie},
	{"IntMap.SetArray",			SetIntTrieArray},
	{"IntMap.SetString",		SetIntTrieString},
	{"IntMap.SetValue",			SetIntTrieValue},
	{"IntMap.Size.get",			GetIntTrieSize},
	{"IntMap.Snapshot",			CreateIntTrieSnapshot},
	{"IntMap.Clone",			CloneIntTrie},

	{"StringMapSnapshot.Length.get",	TrieSnapshotLength},
	{"StringMapSnapshot.KeyBufferSize", TrieSnapshotKeyBufferSize},
	{"StringMapSnapshot.GetKey",		GetTrieSnapshotKey},

	{"IntMapSnapshot.Length.get",	IntTrieSnapshotLength},
	{"IntMapSnapshot.GetKey",		GetIntTrieSnapshotKey},

	{NULL,						NULL},
};
