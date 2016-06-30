/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2010 AlliedModders LLC.  All rights reserved.
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

#include "common_logic.h"
#include <IHandleSys.h>
#include <ISourceMod.h>
#include "CDataPack.h"

HandleType_t g_DataPackType;

class DataPackNatives : 
	public SMGlobalClass,
	public IHandleTypeDispatch
{
public:
	void OnSourceModAllInitialized()
	{
		HandleAccess hacc;
		TypeAccess tacc;

		handlesys->InitAccessDefaults(&tacc, &hacc);
		tacc.access[HTypeAccess_Create] = true;
		tacc.access[HTypeAccess_Inherit] = true;
		tacc.ident = g_pCoreIdent;
		hacc.access[HandleAccess_Read] = HANDLE_RESTRICT_OWNER;

		g_DataPackType = handlesys->CreateType("DataPack", this, 0, &tacc, &hacc, g_pCoreIdent, NULL);
	}
	void OnSourceModShutdown()
	{
		handlesys->RemoveType(g_DataPackType, g_pCoreIdent);
		g_DataPackType = 0;
	}
	void OnHandleDestroy(HandleType_t type, void *object)
	{
		CDataPack::Free(reinterpret_cast<IDataPack *>(object));
	}
	bool GetHandleApproxSize(HandleType_t type, void *object, unsigned int *pSize)
	{
		CDataPack *pack = reinterpret_cast<CDataPack *>(object);
		*pSize = sizeof(CDataPack) + pack->GetCapacity();
		return true;
	}
};

static cell_t smn_CreateDataPack(IPluginContext *pContext, const cell_t *params)
{
	IDataPack *pDataPack = CDataPack::New();

	if (!pDataPack)
	{
		return 0;
	}

	return handlesys->CreateHandle(g_DataPackType, pDataPack, pContext->GetIdentity(), g_pCoreIdent, NULL);
}

static cell_t smn_WritePackCell(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	IDataPack *pDataPack;

	sec.pOwner = pContext->GetIdentity();
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_DataPackType, &sec, (void **)&pDataPack))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid data pack handle %x (error %d)", hndl, herr);
	}

	pDataPack->PackCell(params[2]);

	return 1;
}

static cell_t smn_WritePackFloat(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	IDataPack *pDataPack;

	sec.pOwner = pContext->GetIdentity();
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_DataPackType, &sec, (void **)&pDataPack))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid data pack handle %x (error %d)", hndl, herr);
	}

	pDataPack->PackFloat(sp_ctof(params[2]));

	return 1;
}

static cell_t smn_WritePackString(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	IDataPack *pDataPack;

	sec.pOwner = pContext->GetIdentity();
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_DataPackType, &sec, (void **)&pDataPack))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid data pack handle %x (error %d)", hndl, herr);
	}

	char *str;
	pContext->LocalToString(params[2], &str);
	pDataPack->PackString(str);

	return 1;
}

static cell_t smn_WritePackFunction(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	IDataPack *pDataPack;

	sec.pOwner = pContext->GetIdentity();
	sec.pIdentity = g_pCoreIdent;

	if ((herr = handlesys->ReadHandle(hndl, g_DataPackType, &sec, (void **)&pDataPack))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid data pack handle %x (error %d)", hndl, herr);
	}

	pDataPack->PackFunction(params[2]);

	return 1;
}

static cell_t smn_ReadPackCell(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	IDataPack *pDataPack;

	sec.pOwner = pContext->GetIdentity();
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_DataPackType, &sec, (void **)&pDataPack))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid data pack handle %x (error %d)", hndl, herr);
	}

	if (!pDataPack->IsReadable(sizeof(char) + sizeof(size_t) + sizeof(cell_t)))
	{
		return pContext->ThrowNativeError("DataPack operation is out of bounds.");
	}

	return pDataPack->ReadCell();
}

static cell_t smn_ReadPackFloat(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	IDataPack *pDataPack;

	sec.pOwner = pContext->GetIdentity();
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_DataPackType, &sec, (void **)&pDataPack))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid data pack handle %x (error %d)", hndl, herr);
	}

	if (!pDataPack->IsReadable(sizeof(char) + sizeof(size_t) + sizeof(float)))
	{
		return pContext->ThrowNativeError("DataPack operation is out of bounds.");
	}

	return sp_ftoc(pDataPack->ReadFloat());
}

static cell_t smn_ReadPackString(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	IDataPack *pDataPack;

	sec.pOwner = pContext->GetIdentity();
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_DataPackType, &sec, (void **)&pDataPack))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid data pack handle %x (error %d)", hndl, herr);
	}

	const char *str;
	if (!(str=pDataPack->ReadString(NULL)))
	{
		return pContext->ThrowNativeError("DataPack operation is out of bounds.");
	}

	pContext->StringToLocal(params[2], params[3], str);

	return 1;
}

static cell_t smn_ReadPackFunction(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	IDataPack *pDataPack;

	sec.pOwner = pContext->GetIdentity();
	sec.pIdentity = g_pCoreIdent;

	if ((herr = handlesys->ReadHandle(hndl, g_DataPackType, &sec, (void **)&pDataPack))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid data pack handle %x (error %d)", hndl, herr);
	}

	if (!pDataPack->IsReadable(sizeof(char) + sizeof(size_t) + sizeof(cell_t)))
	{
		return pContext->ThrowNativeError("DataPack operation is out of bounds.");
	}

	return pDataPack->ReadFunction();
}

static cell_t smn_ResetPack(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	IDataPack *pDataPack;

	sec.pOwner = pContext->GetIdentity();
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_DataPackType, &sec, (void **)&pDataPack))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid data pack handle %x (error %d)", hndl, herr);
	}

	pDataPack->Reset();
	if (params[2])
	{
		pDataPack->ResetSize();
	}

	return 1;
}

static cell_t smn_GetPackPosition(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	IDataPack *pDataPack;

	sec.pOwner = pContext->GetIdentity();
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_DataPackType, &sec, (void **)&pDataPack))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid data pack handle %x (error %d)", hndl, herr);
	}

	return static_cast<cell_t>(pDataPack->GetPosition());
}

static cell_t smn_SetPackPosition(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	IDataPack *pDataPack;

	sec.pOwner = pContext->GetIdentity();
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_DataPackType, &sec, (void **)&pDataPack))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid data pack handle %x (error %d)", hndl, herr);
	}

	if (!pDataPack->SetPosition(params[2]))
	{
		return pContext->ThrowNativeError("Invalid DataPack position, %d is out of bounds", params[2]);
	}

	return 1;
}

static cell_t smn_IsPackReadable(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	IDataPack *pDataPack;

	sec.pOwner = pContext->GetIdentity();
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_DataPackType, &sec, (void **)&pDataPack))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid data pack handle %x (error %d)", hndl, herr);
	}

	return pDataPack->IsReadable(params[2]) ? 1 : 0;
}

static DataPackNatives s_DataPackNatives;

REGISTER_NATIVES(datapacknatives)
{
	{"CreateDataPack",			smn_CreateDataPack},
	{"WritePackCell",			smn_WritePackCell},
	{"WritePackFloat",			smn_WritePackFloat},
	{"WritePackString",			smn_WritePackString},
	{"WritePackFunction",		smn_WritePackFunction},
	{"ReadPackCell",			smn_ReadPackCell},
	{"ReadPackFloat",			smn_ReadPackFloat},
	{"ReadPackString",			smn_ReadPackString},
	{"ReadPackFunction",		smn_ReadPackFunction},
	{"ResetPack",				smn_ResetPack},
	{"GetPackPosition",			smn_GetPackPosition},
	{"SetPackPosition",			smn_SetPackPosition},
	{"IsPackReadable",			smn_IsPackReadable},

	// Methodmap versions.
	{"DataPack.DataPack",			smn_CreateDataPack},
	{"DataPack.WriteCell",			smn_WritePackCell},
	{"DataPack.WriteFloat",			smn_WritePackFloat},
	{"DataPack.WriteString",		smn_WritePackString},
	{"DataPack.WriteFunction",		smn_WritePackFunction},
	{"DataPack.ReadCell",			smn_ReadPackCell},
	{"DataPack.ReadFloat",			smn_ReadPackFloat},
	{"DataPack.ReadString",			smn_ReadPackString},
	{"DataPack.ReadFunction",		smn_ReadPackFunction},
	{"DataPack.Reset",				smn_ResetPack},
	{"DataPack.Position.get",		smn_GetPackPosition},
	{"DataPack.Position.set",		smn_SetPackPosition},
	{"DataPack.IsReadable",			smn_IsPackReadable},
	{NULL,							NULL}
};
