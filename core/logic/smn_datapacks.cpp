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
		delete reinterpret_cast<CDataPack *>(object);
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
	CDataPack *pDataPack = new CDataPack();

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
	CDataPack *pDataPack;

	sec.pOwner = pContext->GetIdentity();
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_DataPackType, &sec, (void **)&pDataPack))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid data pack handle %x (error %d).", hndl, herr);
	}

	bool insert = (params[0] >= 3) ? params[3] : false;
	if (!insert)
	{
		pDataPack->RemoveItem();
	}

	pDataPack->PackCell(params[2]);

	return 1;
}

static cell_t smn_WritePackFloat(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	CDataPack *pDataPack;

	sec.pOwner = pContext->GetIdentity();
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_DataPackType, &sec, (void **)&pDataPack))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid data pack handle %x (error %d).", hndl, herr);
	}

	bool insert = (params[0] >= 3) ? params[3] : false;
	if (!insert)
	{
		pDataPack->RemoveItem();
	}

	pDataPack->PackFloat(sp_ctof(params[2]));

	return 1;
}

static cell_t smn_WritePackString(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	CDataPack *pDataPack;

	sec.pOwner = pContext->GetIdentity();
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_DataPackType, &sec, (void **)&pDataPack))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid data pack handle %x (error %d).", hndl, herr);
	}

	bool insert = (params[0] >= 3) ? params[3] : false;
	if (!insert)
	{
		pDataPack->RemoveItem();
	}

	char *str;
	pContext->LocalToString(params[2], &str);
	pDataPack->PackString(str);

	return 1;
}

static cell_t smn_WritePackCellArray(IPluginContext *pContext, const cell_t *params)
{
	HandleError herr;
	HandleSecurity sec;
	sec.pOwner = pContext->GetIdentity();
	sec.pIdentity = g_pCoreIdent;

	Handle_t hndl = static_cast<Handle_t>(params[1]);
	CDataPack *pDataPack = nullptr;
	if ((herr = handlesys->ReadHandle(hndl, g_DataPackType, &sec, (void **)&pDataPack))
		!= HandleError_None)
	{
		pContext->ReportError("Invalid data pack handle %x (error %d).", hndl, herr);
		return 0;
	}

	if (!params[4])
	{
		pDataPack->RemoveItem();
	}

	cell_t *pArray;
	pContext->LocalToPhysAddr(params[2], &pArray);
	pDataPack->PackCellArray(pArray, params[3]);

	return 1;
}

static cell_t smn_WritePackFloatArray(IPluginContext *pContext, const cell_t *params)
{
	HandleError herr;
	HandleSecurity sec;
	sec.pOwner = pContext->GetIdentity();
	sec.pIdentity = g_pCoreIdent;

	Handle_t hndl = static_cast<Handle_t>(params[1]);
	CDataPack *pDataPack = nullptr;
	if ((herr = handlesys->ReadHandle(hndl, g_DataPackType, &sec, (void **)&pDataPack))
		!= HandleError_None)
	{
		pContext->ReportError("Invalid data pack handle %x (error %d).", hndl, herr);
		return 0;
	}

	if (!params[4])
	{
		pDataPack->RemoveItem();
	}

	cell_t *pArray;
	pContext->LocalToPhysAddr(params[2], &pArray);
	pDataPack->PackFloatArray(pArray, params[3]);

	return 1;
}

static cell_t smn_WritePackFunction(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	CDataPack *pDataPack;

	sec.pOwner = pContext->GetIdentity();
	sec.pIdentity = g_pCoreIdent;

	if ((herr = handlesys->ReadHandle(hndl, g_DataPackType, &sec, (void **)&pDataPack))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid data pack handle %x (error %d).", hndl, herr);
	}

	bool insert = (params[0] >= 3) ? params[3] : false;
	if (!insert)
	{
		pDataPack->RemoveItem();
	}

	pDataPack->PackFunction(params[2]);

	return 1;
}

static cell_t smn_ReadPackCell(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	CDataPack *pDataPack;

	sec.pOwner = pContext->GetIdentity();
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_DataPackType, &sec, (void **)&pDataPack))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid data pack handle %x (error %d).", hndl, herr);
	}

	if (!pDataPack->IsReadable())
	{
		return pContext->ThrowNativeError("Data pack operation is out of bounds.");
	}

	if (pDataPack->GetCurrentType() != CDataPackType::Cell)
	{
		return pContext->ThrowNativeError("Invalid data pack type (got %d / expected %d).", pDataPack->GetCurrentType(), CDataPackType::Cell);
	}

	return pDataPack->ReadCell();
}

static cell_t smn_ReadPackFloat(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	CDataPack *pDataPack;

	sec.pOwner = pContext->GetIdentity();
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_DataPackType, &sec, (void **)&pDataPack))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid data pack handle %x (error %d).", hndl, herr);
	}

	if (!pDataPack->IsReadable())
	{
		return pContext->ThrowNativeError("Data pack operation is out of bounds.");
	}

	if (pDataPack->GetCurrentType() != CDataPackType::Float)
	{
		return pContext->ThrowNativeError("Invalid data pack type (got %d / expected %d).", pDataPack->GetCurrentType(), CDataPackType::Float);
	}

	return sp_ftoc(pDataPack->ReadFloat());
}

static cell_t smn_ReadPackString(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	CDataPack *pDataPack;

	sec.pOwner = pContext->GetIdentity();
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_DataPackType, &sec, (void **)&pDataPack))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid data pack handle %x (error %d).", hndl, herr);
	}

	if (!pDataPack->IsReadable())
	{
		return pContext->ThrowNativeError("Data pack operation is out of bounds.");
	}

	if (pDataPack->GetCurrentType() != CDataPackType::String)
	{
		return pContext->ThrowNativeError("Invalid data pack type (got %d / expected %d).", pDataPack->GetCurrentType(), CDataPackType::String);
	}

	const char *str = pDataPack->ReadString(NULL);
	pContext->StringToLocal(params[2], params[3], str);

	return 1;
}

static cell_t smn_ReadPackFunction(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	CDataPack *pDataPack;

	sec.pOwner = pContext->GetIdentity();
	sec.pIdentity = g_pCoreIdent;

	if ((herr = handlesys->ReadHandle(hndl, g_DataPackType, &sec, (void **)&pDataPack))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid data pack handle %x (error %d).", hndl, herr);
	}

	if (!pDataPack->IsReadable())
	{
		return pContext->ThrowNativeError("Data pack operation is out of bounds.");
	}

	if (pDataPack->GetCurrentType() != CDataPackType::Function)
	{
		return pContext->ThrowNativeError("Invalid data pack type (got %d / expected %d).", pDataPack->GetCurrentType(), CDataPackType::Function);
	}

	return pDataPack->ReadFunction();
}

static cell_t smn_ReadPackCellArray(IPluginContext *pContext, const cell_t *params)
{
	HandleError herr; 
	HandleSecurity sec;
	sec.pOwner = pContext->GetIdentity();
	sec.pIdentity = g_pCoreIdent;

	Handle_t hndl = static_cast<Handle_t>(params[1]);
	CDataPack *pDataPack = nullptr;
	if ((herr = handlesys->ReadHandle(hndl, g_DataPackType, &sec, (void **)&pDataPack))
		!= HandleError_None)
	{
		pContext->ReportError("Invalid data pack handle %x (error %d).", hndl, herr);
		return 0;
	}

	if (!pDataPack->IsReadable())
	{
		pContext->ReportError("Data pack operation is out of bounds.");
		return 0;
	}

	if (pDataPack->GetCurrentType() != CDataPackType::CellArray)
	{
		pContext->ReportError("Invalid data pack type (got %d / expected %d).", pDataPack->GetCurrentType(), CDataPackType::CellArray);
		return 0;
	}

	cell_t packCount = 0;
	cell_t *pData = pDataPack->ReadCellArray(&packCount);
	if(pData == nullptr || packCount == 0)
	{
		pContext->ReportError("Invalid data pack operation: current position isn't an array!");
		return 0;
	}

	cell_t count = params[3];
	if(packCount > count)
	{
		pContext->ReportError("Input buffer too small (needed %d, got %d).", packCount, count);
		return 0;
	}
	
	cell_t *pArray;
	pContext->LocalToPhysAddr(params[2], &pArray);

	memcpy(pArray, pData, sizeof(cell_t) * count);

	return 1;
}

static cell_t smn_ReadPackFloatArray(IPluginContext *pContext, const cell_t *params)
{
	HandleError herr;
	HandleSecurity sec;
	sec.pOwner = pContext->GetIdentity();
	sec.pIdentity = g_pCoreIdent;

	Handle_t hndl = static_cast<Handle_t>(params[1]);
	CDataPack *pDataPack = nullptr;
	if ((herr = handlesys->ReadHandle(hndl, g_DataPackType, &sec, (void **)&pDataPack))
		!= HandleError_None)
	{
		pContext->ReportError("Invalid data pack handle %x (error %d).", hndl, herr);
		return 0;
	}

	if (!pDataPack->IsReadable())
	{
		pContext->ReportError("Data pack operation is out of bounds.");
		return 0;
	}

	if (pDataPack->GetCurrentType() != CDataPackType::FloatArray)
	{
		pContext->ReportError("Invalid data pack type (got %d / expected %d).", pDataPack->GetCurrentType(), CDataPackType::FloatArray);
		return 0;
	}

	cell_t packCount = 0;
	cell_t *pData = pDataPack->ReadFloatArray(&packCount);
	if(pData == nullptr || packCount == 0)
	{
		pContext->ReportError("Invalid data pack operation: current position isn't an array!");
		return 0;
	}

	cell_t count = params[3];
	if(packCount > count)
	{
		pContext->ReportError("Input buffer too small (needed %d, got %d).", packCount, count);
		return 0;
	}

	cell_t *pArray;
	pContext->LocalToPhysAddr(params[2], &pArray);

	memcpy(pArray, pData, sizeof(cell_t) * count);

	return 1;
}

static cell_t smn_ResetPack(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	CDataPack *pDataPack;

	sec.pOwner = pContext->GetIdentity();
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_DataPackType, &sec, (void **)&pDataPack))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid data pack handle %x (error %d).", hndl, herr);
	}

	if (params[2])
	{
		pDataPack->ResetSize();
	}
	else
	{
		pDataPack->Reset();
	}

	return 1;
}

static cell_t smn_GetPackPosition(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	CDataPack *pDataPack;

	sec.pOwner = pContext->GetIdentity();
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_DataPackType, &sec, (void **)&pDataPack))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid data pack handle %x (error %d).", hndl, herr);
	}

	return static_cast<cell_t>(pDataPack->GetPosition());
}

static cell_t smn_SetPackPosition(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	CDataPack *pDataPack;

	sec.pOwner = pContext->GetIdentity();
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_DataPackType, &sec, (void **)&pDataPack))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid data pack handle %x (error %d).", hndl, herr);
	}

	if (!pDataPack->SetPosition(params[2]))
	{
		return pContext->ThrowNativeError("Invalid data pack position, %d is out of bounds (%d)", params[2], pDataPack->GetCapacity());
	}

	return 1;
}

static cell_t smn_IsPackReadable(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	CDataPack *pDataPack;

	sec.pOwner = pContext->GetIdentity();
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_DataPackType, &sec, (void **)&pDataPack))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid data pack handle %x (error %d).", hndl, herr);
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
	{"DataPack.WriteCellArray",		smn_WritePackCellArray},
	{"DataPack.WriteFloatArray",	smn_WritePackFloatArray},
	{"DataPack.ReadCell",			smn_ReadPackCell},
	{"DataPack.ReadFloat",			smn_ReadPackFloat},
	{"DataPack.ReadString",			smn_ReadPackString},
	{"DataPack.ReadFunction",		smn_ReadPackFunction},
	{"DataPack.ReadCellArray",		smn_ReadPackCellArray},
	{"DataPack.ReadFloatArray",		smn_ReadPackFloatArray},
	{"DataPack.Reset",				smn_ResetPack},
	{"DataPack.Position.get",		smn_GetPackPosition},
	{"DataPack.Position.set",		smn_SetPackPosition},
	{"DataPack.IsReadable",			smn_IsPackReadable},
	{NULL,							NULL}
};
