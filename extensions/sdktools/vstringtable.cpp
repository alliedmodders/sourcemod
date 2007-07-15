/**
 * vim: set ts=4 :
 * ===============================================================
 * SourceMod SDK Tools Extension
 * Copyright (C) 2004-2007 AlliedModders LLC. All rights reserved.
 * ===============================================================
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Version: $Id$
 */

#include "extension.h"

static cell_t LockStringTables(IPluginContext *pContext, const cell_t *params)
{
	bool lock = params[1] ? true : false;

	return engine->LockNetworkStringTables(lock) ? 1 : 0;
}

static cell_t FindStringTable(IPluginContext *pContext, const cell_t *params)
{
	char *name;
	pContext->LocalToString(params[1], &name);

	INetworkStringTable *pTable = netstringtables->FindTable(name);

	if (!pTable)
	{
		return INVALID_STRING_TABLE;
	}

	return pTable->GetTableId();
}

static cell_t GetNumStringTables(IPluginContext *pContext, const cell_t *params)
{
	return netstringtables->GetNumTables();
}

static cell_t GetStringTableNumStrings(IPluginContext *pContext, const cell_t *params)
{
	TABLEID idx = static_cast<TABLEID>(params[1]);
	INetworkStringTable *pTable = netstringtables->GetTable(idx);

	if (!pTable)
	{
		return pContext->ThrowNativeError("Invalid string table index %d", idx);
	}

	return pTable->GetNumStrings();
}

static cell_t GetStringTableMaxStrings(IPluginContext *pContext, const cell_t *params)
{
	TABLEID idx = static_cast<TABLEID>(params[1]);
	INetworkStringTable *pTable = netstringtables->GetTable(idx);

	if (!pTable)
	{
		return pContext->ThrowNativeError("Invalid string table index %d", idx);
	}

	return pTable->GetMaxStrings();
}

static cell_t GetStringTableName(IPluginContext *pContext, const cell_t *params)
{
	TABLEID idx = static_cast<TABLEID>(params[1]);
	INetworkStringTable *pTable = netstringtables->GetTable(idx);
	size_t numBytes;

	if (!pTable)
	{
		return pContext->ThrowNativeError("Invalid string table index %d", idx);
	}

	pContext->StringToLocalUTF8(params[2], params[3], pTable->GetTableName(), &numBytes);

	return numBytes;
}

static cell_t FindStringIndex(IPluginContext *pContext, const cell_t *params)
{
	TABLEID idx = static_cast<TABLEID>(params[1]);
	INetworkStringTable *pTable = netstringtables->GetTable(idx);
	char *str;

	if (!pTable)
	{
		return pContext->ThrowNativeError("Invalid string table index %d", idx);
	}

	pContext->LocalToString(params[2], &str);

	return pTable->FindStringIndex(str);
}

static cell_t ReadStringTable(IPluginContext *pContext, const cell_t *params)
{
	TABLEID idx = static_cast<TABLEID>(params[1]);
	INetworkStringTable *pTable = netstringtables->GetTable(idx);
	int stringidx;
	const char *value;
	size_t numBytes;

	if (!pTable)
	{
		return pContext->ThrowNativeError("Invalid string table index %d", idx);
	}
	
	stringidx = params[2];
	value = pTable->GetString(stringidx);

	if (!value)
	{
		return pContext->ThrowNativeError("Invalid string index specified for table (index %d) (table \"%s\")", stringidx, pTable->GetTableName());
	}

	pContext->StringToLocalUTF8(params[3], params[4], value, &numBytes);

	return numBytes;
}

static cell_t GetStringTableDataLength(IPluginContext *pContext, const cell_t *params)
{
	TABLEID idx = static_cast<TABLEID>(params[1]);
	INetworkStringTable *pTable = netstringtables->GetTable(idx);
	int stringidx;
	const void *userdata;
	int datalen;

	if (!pTable)
	{
		return pContext->ThrowNativeError("Invalid string table index %d", idx);
	}

	stringidx = params[2];
	if (stringidx < 0 || stringidx >= pTable->GetNumStrings())
	{
		return pContext->ThrowNativeError("Invalid string index specified for table (index %d) (table \"%s\")", stringidx, pTable->GetTableName());
	}

	userdata = pTable->GetStringUserData(stringidx, &datalen);
	if (!userdata)
	{
		datalen = 0;
	}

	return datalen;
}

static cell_t GetStringTableData(IPluginContext *pContext, const cell_t *params)
{
	TABLEID idx = static_cast<TABLEID>(params[1]);
	INetworkStringTable *pTable = netstringtables->GetTable(idx);
	int stringidx;
	const char *userdata;
	int datalen;
	size_t numBytes;

	if (!pTable)
	{
		return pContext->ThrowNativeError("Invalid string table index %d", idx);
	}

	stringidx = params[2];
	if (stringidx < 0 || stringidx >= pTable->GetNumStrings())
	{
		return pContext->ThrowNativeError("Invalid string index specified for table (index %d) (table \"%s\")", stringidx, pTable->GetTableName());
	}

	userdata = (const char *)pTable->GetStringUserData(stringidx, &datalen);
	if (!userdata)
	{
		userdata = "";
	}

	pContext->StringToLocalUTF8(params[3], params[4], userdata, &numBytes);

	return numBytes;
}

CON_COMMAND(sm_lockstate, "Lock state")
{
	bool save = engine->LockNetworkStringTables(false);
	Msg("save: %d\n", save);
	engine->LockNetworkStringTables(save);
}

static cell_t SetStringTableData(IPluginContext *pContext, const cell_t *params)
{
	TABLEID idx = static_cast<TABLEID>(params[1]);
	INetworkStringTable *pTable = netstringtables->GetTable(idx);
	int stringidx;
	char *userdata;

	if (!pTable)
	{
		return pContext->ThrowNativeError("Invalid string table index %d", idx);
	}

	stringidx = params[2];
	if (stringidx < 0 || stringidx >= pTable->GetNumStrings())
	{
		return pContext->ThrowNativeError("Invalid string index specified for table (index %d) (table \"%s\")", stringidx, pTable->GetTableName());
	}

	pContext->LocalToString(params[3], &userdata);
	pTable->SetStringUserData(stringidx, params[4], userdata);

	return 1;
}

static cell_t AddToStringTable(IPluginContext *pContext, const cell_t *params)
{
	TABLEID idx = static_cast<TABLEID>(params[1]);
	INetworkStringTable *pTable = netstringtables->GetTable(idx);
	char *str, *userdata;

	if (!pTable)
	{
		return pContext->ThrowNativeError("Invalid string table index %d", idx);
	}

	pContext->LocalToString(params[2], &str);
	pContext->LocalToString(params[3], &userdata);

	pTable->AddString(str, params[4], userdata);

	return 1;
}

sp_nativeinfo_t g_StringTableNatives[] = 
{
	{"LockStringTables",			LockStringTables},
	{"FindStringTable",				FindStringTable},
	{"GetNumStringTables",			GetNumStringTables},
	{"GetStringTableNumStrings",	GetStringTableNumStrings},
	{"GetStringTableMaxStrings",	GetStringTableMaxStrings},
	{"GetStringTableName",			GetStringTableName},
	{"FindStringIndex",				FindStringIndex},
	{"ReadStringTable",				ReadStringTable},
	{"GetStringTableDataLength",	GetStringTableDataLength},
	{"GetStringTableData",			GetStringTableData},
	{"SetStringTableData",			SetStringTableData},
	{"AddToStringTable",			AddToStringTable},
	{NULL,							NULL},
};
