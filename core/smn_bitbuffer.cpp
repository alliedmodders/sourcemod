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

#include <bitbuf.h>
#include <vector.h>
#include "sourcemod.h"
#include "HandleSys.h"

static cell_t smn_BfWriteBool(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	bf_write *pBitBuf;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=g_HandleSys.ReadHandle(hndl, g_WrBitBufType, &sec, (void **)&pBitBuf))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid bit buffer handle %x (error %d)", hndl, herr);
	}

	pBitBuf->WriteOneBit(params[2]);

	return 1;
}

static cell_t smn_BfWriteByte(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	bf_write *pBitBuf;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=g_HandleSys.ReadHandle(hndl, g_WrBitBufType, &sec, (void **)&pBitBuf))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid bit buffer handle %x (error %d)", hndl, herr);
	}

	pBitBuf->WriteByte(params[2]);

	return 1;
}

static cell_t smn_BfWriteChar(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	bf_write *pBitBuf;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=g_HandleSys.ReadHandle(hndl, g_WrBitBufType, &sec, (void **)&pBitBuf))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid bit buffer handle %x (error %d)", hndl, herr);
	}

	pBitBuf->WriteChar(params[2]);

	return 1;
}

static cell_t smn_BfWriteShort(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	bf_write *pBitBuf;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=g_HandleSys.ReadHandle(hndl, g_WrBitBufType, &sec, (void **)&pBitBuf))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid bit buffer handle %x (error %d)", hndl, herr);
	}

	pBitBuf->WriteShort(params[2]);

	return 1;
}

static cell_t smn_BfWriteWord(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	bf_write *pBitBuf;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=g_HandleSys.ReadHandle(hndl, g_WrBitBufType, &sec, (void **)&pBitBuf))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid bit buffer handle %x (error %d)", hndl, herr);
	}

	pBitBuf->WriteWord(params[2]);

	return 1;
}

static cell_t smn_BfWriteNum(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	bf_write *pBitBuf;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=g_HandleSys.ReadHandle(hndl, g_WrBitBufType, &sec, (void **)&pBitBuf))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid bit buffer handle %x (error %d)", hndl, herr);
	}

	pBitBuf->WriteLong(static_cast<long>(params[2]));

	return 1;
}

static cell_t smn_BfWriteFloat(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	bf_write *pBitBuf;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=g_HandleSys.ReadHandle(hndl, g_WrBitBufType, &sec, (void **)&pBitBuf))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid bit buffer handle %x (error %d)", hndl, herr);
	}

	pBitBuf->WriteFloat(sp_ctof(params[2]));

	return 1;
}

static cell_t smn_BfWriteString(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	bf_write *pBitBuf;
	int err;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=g_HandleSys.ReadHandle(hndl, g_WrBitBufType, &sec, (void **)&pBitBuf))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid bit buffer handle %x (error %d)", hndl, herr);
	}

	char *str;
	if ((err=pCtx->LocalToString(params[2], &str)) != SP_ERROR_NONE)
	{
		pCtx->ThrowNativeErrorEx(err, NULL);
		return 0;
	}

	pBitBuf->WriteString(str);

	return 1;
}

static cell_t smn_BfWriteEntity(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	bf_write *pBitBuf;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=g_HandleSys.ReadHandle(hndl, g_WrBitBufType, &sec, (void **)&pBitBuf))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid bit buffer handle %x (error %d)", hndl, herr);
	}

	pBitBuf->WriteShort(params[2]);

	return 1;
}

static cell_t smn_BfWriteAngle(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	bf_write *pBitBuf;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=g_HandleSys.ReadHandle(hndl, g_WrBitBufType, &sec, (void **)&pBitBuf))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid bit buffer handle %x (error %d)", hndl, herr);
	}

	pBitBuf->WriteBitAngle(sp_ctof(params[2]), params[3]);

	return 1;
}

static cell_t smn_BfWriteCoord(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	bf_write *pBitBuf;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=g_HandleSys.ReadHandle(hndl, g_WrBitBufType, &sec, (void **)&pBitBuf))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid bit buffer handle %x (error %d)", hndl, herr);
	}

	pBitBuf->WriteBitCoord(sp_ctof(params[2]));

	return 1;
}

static cell_t smn_BfWriteVecCoord(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	bf_write *pBitBuf;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=g_HandleSys.ReadHandle(hndl, g_WrBitBufType, &sec, (void **)&pBitBuf))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid bit buffer handle %x (error %d)", hndl, herr);
	}

	cell_t *pVec;
	pCtx->LocalToPhysAddr(params[2], &pVec);
	Vector vec(sp_ctof(pVec[0]), sp_ctof(pVec[1]), sp_ctof(pVec[2]));
	pBitBuf->WriteBitVec3Coord(vec);

	return 1;
}

static cell_t smn_BfWriteVecNormal(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	bf_write *pBitBuf;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=g_HandleSys.ReadHandle(hndl, g_WrBitBufType, &sec, (void **)&pBitBuf))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid bit buffer handle %x (error %d)", hndl, herr);
	}

	cell_t *pVec;
	pCtx->LocalToPhysAddr(params[2], &pVec);
	Vector vec(sp_ctof(pVec[0]), sp_ctof(pVec[1]), sp_ctof(pVec[2]));
	pBitBuf->WriteBitVec3Normal(vec);

	return 1;
}

static cell_t smn_BfWriteAngles(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	bf_write *pBitBuf;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=g_HandleSys.ReadHandle(hndl, g_WrBitBufType, &sec, (void **)&pBitBuf))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid bit buffer handle %x (error %d)", hndl, herr);
	}

	cell_t *pAng;
	pCtx->LocalToPhysAddr(params[2], &pAng);
	QAngle ang(sp_ctof(pAng[0]), sp_ctof(pAng[1]), sp_ctof(pAng[2]));
	pBitBuf->WriteBitAngles(ang);

	return 1;
}

REGISTER_NATIVES(wrbitbufnatives)
{
	{"BfWriteBool",				smn_BfWriteBool},
	{"BfWriteByte",				smn_BfWriteByte},
	{"BfWriteChar",				smn_BfWriteChar},
	{"BfWriteShort",			smn_BfWriteShort},
	{"BfWriteWord",				smn_BfWriteWord},
	{"BfWriteNum",				smn_BfWriteNum},
	{"BfWriteFloat",			smn_BfWriteFloat},
	{"BfWriteString",			smn_BfWriteString},
	{"BfWriteEntity",			smn_BfWriteEntity},
	{"BfWriteAngle",			smn_BfWriteAngle},
	{"BfWriteCoord",			smn_BfWriteCoord},
	{"BfWriteVecCoord",			smn_BfWriteVecCoord},
	{"BfWriteVecNormal",		smn_BfWriteVecNormal},
	{"BfWriteAngles",			smn_BfWriteAngles},
	{NULL,						NULL}
};
