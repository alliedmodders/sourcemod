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

	int strindex = pTable->FindStringIndex(str);

	// INVALID_STRING_INDEX is 65535 at time of writing, but already defined in sp inc files as -1
	if (strindex == INVALID_STRING_INDEX)
	{
		return -1;
	}

	return strindex;
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
	int datalen = 0;

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

	char *addr;
	pContext->LocalToString(params[3], &addr);

	int maxBytes = params[4];
	size_t numBytes = MIN(maxBytes, datalen);

	if (userdata)
	{
		memcpy(addr, userdata, numBytes);
	}
	else if (maxBytes > 0)
	{
		addr[0] = '\0';
		numBytes = 0;
	}

	return numBytes;
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

#if SOURCE_ENGINE >= SE_ORANGEBOX
	pTable->AddString(true, str, params[4], userdata);
#else
	pTable->AddString(str, params[4], userdata);
#endif

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
