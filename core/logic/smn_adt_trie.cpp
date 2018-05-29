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
#include <am-moveable.h>
#include <am-refcounting.h>
#include <sm_hashmap.h>
#include "sm_memtable.h"
#include <IHandleSys.h>

HandleType_t htStringCellTrie;
HandleType_t htStringSnapshot;
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
	char *chars() const {
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

struct StringCellTrie
{
	StringHashMap<Entry> map;
};

struct IntCellTrie
{
	IntHashMap<Entry> map;
};

struct StringTrieSnapshot
{
	StringTrieSnapshot()
		: strings(128)
	{ }

	size_t mem_usage()
	{
		return length * sizeof(int) + strings.GetMemTable()->GetMemUsage();
	}

	size_t length;
	ke::AutoPtr<int[]> keys;
	BaseStringTable strings;
};

struct IntTrieSnapshot
{
	IntTrieSnapshot()
		: ints(128)
	{ }

	size_t mem_usage()
	{
		return length * sizeof(int) + ints.GetMemTable()->GetMemUsage();
	}

	size_t length;
	ke::AutoPtr<int[]> keys;
	BaseIntTable ints;
};

class TrieHelpers : 
	public SMGlobalClass,
	public IHandleTypeDispatch
{
public: //SMGlobalClass
	void OnSourceModAllInitialized()
	{
		htStringCellTrie = handlesys->CreateType("Trie", this, 0, NULL, NULL, g_pCoreIdent, NULL);
		htStringSnapshot = handlesys->CreateType("TrieSnapshot", this, 0, NULL, NULL, g_pCoreIdent, NULL);
		htIntCellTrie = handlesys->CreateType("IntTrie", this, 0, NULL, NULL, g_pCoreIdent, NULL);
		htIntSnapshot = handlesys->CreateType("IntTrieSnapshot", this, 0, NULL, NULL, g_pCoreIdent, NULL);
	}
	void OnSourceModShutdown()
	{
		handlesys->RemoveType(htStringSnapshot, g_pCoreIdent);
		handlesys->RemoveType(htStringCellTrie, g_pCoreIdent);
		handlesys->RemoveType(htIntSnapshot, g_pCoreIdent);
		handlesys->RemoveType(htIntCellTrie, g_pCoreIdent);
	}
public: //IHandleTypeDispatch
	void OnHandleDestroy(HandleType_t type, void *object)
	{
		if (type == htStringCellTrie)
		{
			delete (StringCellTrie *)object;
			return;
		}

		if (type == htStringSnapshot)
		{
			delete (StringTrieSnapshot *)object;
			return;
		}

		if (type == htIntCellTrie)
		{
			delete (IntCellTrie *)object;
			return;
		}

		if (type == htIntSnapshot)
		{
			delete (IntTrieSnapshot *)object;
			return;
		}
	}
	bool GetHandleApproxSize(HandleType_t type, void *object, unsigned int *pSize)
	{
		if (type == htStringCellTrie)
		{
			StringCellTrie *pArray = (StringCellTrie *)object;
			*pSize = sizeof(StringCellTrie) + pArray->map.mem_usage();
			return true;
		}

		if (type == htStringSnapshot)
		{
			StringTrieSnapshot *snapshot = (StringTrieSnapshot *)object;
			*pSize = sizeof(StringTrieSnapshot) + snapshot->mem_usage();
			return true;
		}

		if (type == htIntCellTrie)
		{
			IntCellTrie *pArray = (IntCellTrie *)object;
			*pSize = sizeof(IntCellTrie) + pArray->map.mem_usage();
			return true;
		}

		if (type == htIntSnapshot)
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
	StringCellTrie *pTrie = new StringCellTrie;
	Handle_t hndl;

	if ((hndl = handlesys->CreateHandle(htStringCellTrie, pTrie, pContext->GetIdentity(), g_pCoreIdent, NULL))
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
	StringCellTrie *pTrie;
	HandleError err;
	HandleSecurity sec = HandleSecurity(pContext->GetIdentity(), g_pCoreIdent);

	Handle_t hndl = params[1];

	if ((err = handlesys->ReadHandle(hndl, htStringCellTrie, &sec, (void **)&pTrie))
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

	const int key = params[2];

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
	StringCellTrie *pTrie;
	HandleError err;
	HandleSecurity sec = HandleSecurity(pContext->GetIdentity(), g_pCoreIdent);

	Handle_t hndl = params[1];

	if ((err = handlesys->ReadHandle(hndl, htStringCellTrie, &sec, (void **)&pTrie))
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

	const int key = params[2];
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
	StringCellTrie *pTrie;
	HandleError err;
	HandleSecurity sec = HandleSecurity(pContext->GetIdentity(), g_pCoreIdent);

	Handle_t hndl = params[1];

	if ((err = handlesys->ReadHandle(hndl, htStringCellTrie, &sec, (void **)&pTrie))
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

	const int key = params[2];
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


static cell_t RemoveFromTrie(IPluginContext *pContext, const cell_t *params)
{
	StringCellTrie *pTrie;
	HandleError err;
	HandleSecurity sec = HandleSecurity(pContext->GetIdentity(), g_pCoreIdent);

	Handle_t hndl = params[1];

	if ((err = handlesys->ReadHandle(hndl, htStringCellTrie, &sec, (void **)&pTrie))
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

	const int key = params[2];

	IntHashMap<Entry>::Result r = pTrie->map.find(key);
	if (!r.found())
		return 0;

	pTrie->map.remove(r);
	return 1;
}


static cell_t ClearTrie(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl;
	StringCellTrie *pTrie;
	HandleError err;
	HandleSecurity sec = HandleSecurity(pContext->GetIdentity(), g_pCoreIdent);

	hndl = params[1];

	if ((err = handlesys->ReadHandle(hndl, htStringCellTrie, &sec, (void **)&pTrie))
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
	StringCellTrie *pTrie;
	HandleError err;
	HandleSecurity sec = HandleSecurity(pContext->GetIdentity(), g_pCoreIdent);

	hndl = params[1];

	if ((err = handlesys->ReadHandle(hndl, htStringCellTrie, &sec, (void **)&pTrie))
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

	const char key = params[2];
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
	StringCellTrie *pTrie;
	HandleError err;
	HandleSecurity sec = HandleSecurity(pContext->GetIdentity(), g_pCoreIdent);

	hndl = params[1];

	if ((err = handlesys->ReadHandle(hndl, htStringCellTrie, &sec, (void **)&pTrie))
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

	const int key = params[2];
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
	StringCellTrie *pTrie;
	HandleError err;
	HandleSecurity sec = HandleSecurity(pContext->GetIdentity(), g_pCoreIdent);

	hndl = params[1];

	if ((err = handlesys->ReadHandle(hndl, htStringCellTrie, &sec, (void **)&pTrie))
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
	pContext->StringToLocalUTF8(params[3], params[4], r->value.chars(), &written);

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

	const int key = params[2];
	cell_t *pSize;
	pContext->LocalToPhysAddr(params[5], &pSize);

	IntHashMap<Entry>::Result r = pTrie->map.find(key);
	if (!r.found() || !r->value.isString())
		return 0;

	size_t written;
	pContext->StringToLocalUTF8(params[3], params[4], r->value.chars(), &written);

	*pSize = (cell_t)written;
	return 1;
}


static cell_t GetTrieSize(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl;
	StringCellTrie *pTrie;
	HandleError err;
	HandleSecurity sec = HandleSecurity(pContext->GetIdentity(), g_pCoreIdent);

	hndl = params[1];

	if ((err = handlesys->ReadHandle(hndl, htStringCellTrie, &sec, (void **)&pTrie))
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

	StringCellTrie *pTrie;
	if ((err = handlesys->ReadHandle(hndl, htStringCellTrie, &sec, (void **)&pTrie))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", hndl, err);
	}

	StringTrieSnapshot *snapshot = new StringTrieSnapshot;
	snapshot->length = pTrie->map.elements();
	snapshot->keys = ke::MakeUnique<int[]>(snapshot->length);
	size_t i = 0;
	for (StringHashMap<Entry>::iterator iter = pTrie->map.iter(); !iter.empty(); iter.next(), i++)
		snapshot->keys[i] = snapshot->strings.AddString(iter->key.chars(), iter->key.length());
	assert(i == snapshot->length);

	if ((hndl = handlesys->CreateHandle(htStringSnapshot, snapshot, pContext->GetIdentity(), g_pCoreIdent, NULL))
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
	snapshot->keys = ke::MakeUnique<int[]>(snapshot->length);
	size_t i = 0;
	for (IntHashMap<Entry>::iterator iter = pTrie->map.iter(); !iter.empty(); iter.next(), i++)
		snapshot->keys[i] = snapshot->ints.AddInt(iter->key);
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

	StringTrieSnapshot *snapshot;
	if ((err = handlesys->ReadHandle(hndl, htStringSnapshot, &sec, (void **)&snapshot))
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

	StringTrieSnapshot *snapshot;
	if ((err = handlesys->ReadHandle(hndl, htStringSnapshot, &sec, (void **)&snapshot))
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

	StringTrieSnapshot *snapshot;
	if ((err = handlesys->ReadHandle(hndl, htStringSnapshot, &sec, (void **)&snapshot))
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

	return snapshot->ints.GetInt(snapshot->keys[index]);
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

	{"ClearIntTrie",			ClearIntTrie},
	{"CreateIntTrie",			CreateIntTrie},
	{"GetIntTrieArray",			GetIntTrieArray},
	{"GetIntTrieString",			GetIntTrieString},
	{"GetIntTrieValue",			GetIntTrieValue},
	{"RemoveFromIntTrie",			RemoveFromIntTrie},
	{"SetIntTrieArray",			SetIntTrieArray},
	{"SetIntTrieString",			SetIntTrieString},
	{"SetIntTrieValue",			SetIntTrieValue},
	{"GetIntTrieSize",			GetIntTrieSize},

	{"CreateTrieSnapshot",			CreateTrieSnapshot},
	{"TrieSnapshotLength",			TrieSnapshotLength},
	{"TrieSnapshotKeyBufferSize",		TrieSnapshotKeyBufferSize},
	{"GetTrieSnapshotKey",			GetTrieSnapshotKey},

	{"CreateIntTrieSnapshot",		CreateIntTrieSnapshot},
	{"IntTrieSnapshotLength",		IntTrieSnapshotLength},
	{"GetIntTrieSnapshotKey",		GetIntTrieSnapshotKey},

	// Transitional syntax support.
	{"StringMap.StringMap",			CreateTrie},
	{"StringMap.Clear",			ClearTrie},
	{"StringMap.GetArray",			GetTrieArray},
	{"StringMap.GetString",			GetTrieString},
	{"StringMap.GetValue",			GetTrieValue},
	{"StringMap.Remove",			RemoveFromTrie},
	{"StringMap.SetArray",			SetTrieArray},
	{"StringMap.SetString",			SetTrieString},
	{"StringMap.SetValue",			SetTrieValue},
	{"StringMap.Size.get",			GetTrieSize},
	{"StringMap.Snapshot",			CreateTrieSnapshot},

	{"IntMap.IntMap",			CreateIntTrie},
	{"IntMap.Clear",			ClearIntTrie},
	{"IntMap.GetArray",			GetIntTrieArray},
	{"IntMap.GetString",			GetIntTrieString},
	{"IntMap.GetValue",			GetIntTrieValue},
	{"IntMap.Remove",			RemoveFromIntTrie},
	{"IntMap.SetArray",			SetIntTrieArray},
	{"IntMap.SetString",			SetIntTrieString},
	{"IntMap.SetValue",			SetIntTrieValue},
	{"IntMap.Size.get",			GetIntTrieSize},
	{"IntMap.Snapshot",			CreateIntTrieSnapshot},

	{"StringMapSnapshot.Length.get",	TrieSnapshotLength},
	{"StringMapSnapshot.KeyBufferSize",	TrieSnapshotKeyBufferSize},
	{"StringMapSnapshot.GetKey",		GetTrieSnapshotKey},

	{"IntMapSnapshot.Length.get",		IntTrieSnapshotLength},
	{"IntMapSnapshot.GetKey",		GetIntTrieSnapshotKey},

	{NULL,					NULL},
};
