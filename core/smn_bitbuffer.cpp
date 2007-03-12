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

#include "sourcemod.h"
#include "HandleSys.h"
#include <bitbuf.h>
#include <vector.h>

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

static cell_t smn_BfReadBool(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	bf_read *pBitBuf;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=g_HandleSys.ReadHandle(hndl, g_RdBitBufType, &sec, (void **)&pBitBuf))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid bit buffer handle %x (error %d)", hndl, herr);
	}

	return pBitBuf->ReadOneBit() ? 1 : 0;
}

static cell_t smn_BfReadByte(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	bf_read *pBitBuf;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=g_HandleSys.ReadHandle(hndl, g_RdBitBufType, &sec, (void **)&pBitBuf))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid bit buffer handle %x (error %d)", hndl, herr);
	}

	return pBitBuf->ReadByte();
}

static cell_t smn_BfReadChar(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	bf_read *pBitBuf;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=g_HandleSys.ReadHandle(hndl, g_RdBitBufType, &sec, (void **)&pBitBuf))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid bit buffer handle %x (error %d)", hndl, herr);
	}

	return pBitBuf->ReadChar();
}

static cell_t smn_BfReadShort(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	bf_read *pBitBuf;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=g_HandleSys.ReadHandle(hndl, g_RdBitBufType, &sec, (void **)&pBitBuf))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid bit buffer handle %x (error %d)", hndl, herr);
	}

	return pBitBuf->ReadShort();
}

static cell_t smn_BfReadWord(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	bf_read *pBitBuf;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=g_HandleSys.ReadHandle(hndl, g_RdBitBufType, &sec, (void **)&pBitBuf))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid bit buffer handle %x (error %d)", hndl, herr);
	}

	return pBitBuf->ReadWord();
}

static cell_t smn_BfReadNum(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	bf_read *pBitBuf;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=g_HandleSys.ReadHandle(hndl, g_RdBitBufType, &sec, (void **)&pBitBuf))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid bit buffer handle %x (error %d)", hndl, herr);
	}

	return static_cast<cell_t>(pBitBuf->ReadLong());
}

static cell_t smn_BfReadFloat(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	bf_read *pBitBuf;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=g_HandleSys.ReadHandle(hndl, g_RdBitBufType, &sec, (void **)&pBitBuf))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid bit buffer handle %x (error %d)", hndl, herr);
	}

	return sp_ftoc(pBitBuf->ReadFloat());
}

static cell_t smn_BfReadString(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	bf_read *pBitBuf;
	char *buf;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=g_HandleSys.ReadHandle(hndl, g_RdBitBufType, &sec, (void **)&pBitBuf))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid bit buffer handle %x (error %d)", hndl, herr);
	}

	pCtx->LocalToPhysAddr(params[2], (cell_t **)&buf);
	if (!pBitBuf->ReadString(buf, params[3], params[4] ? true : false))
	{
		return pCtx->ThrowNativeError("Destination string buffer is too short, try increasing its size");
	}

	return 1;
}

static cell_t smn_BfReadEntity(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	bf_read *pBitBuf;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=g_HandleSys.ReadHandle(hndl, g_RdBitBufType, &sec, (void **)&pBitBuf))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid bit buffer handle %x (error %d)", hndl, herr);
	}

	return pBitBuf->ReadShort();
}

static cell_t smn_BfReadAngle(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	bf_read *pBitBuf;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=g_HandleSys.ReadHandle(hndl, g_RdBitBufType, &sec, (void **)&pBitBuf))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid bit buffer handle %x (error %d)", hndl, herr);
	}

	return sp_ftoc(pBitBuf->ReadBitAngle(params[2]));
}

static cell_t smn_BfReadCoord(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	bf_read *pBitBuf;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=g_HandleSys.ReadHandle(hndl, g_RdBitBufType, &sec, (void **)&pBitBuf))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid bit buffer handle %x (error %d)", hndl, herr);
	}

	return sp_ftoc(pBitBuf->ReadBitCoord());
}

static cell_t smn_BfReadVecCoord(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	bf_read *pBitBuf;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=g_HandleSys.ReadHandle(hndl, g_RdBitBufType, &sec, (void **)&pBitBuf))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid bit buffer handle %x (error %d)", hndl, herr);
	}

	cell_t *pVec;
	pCtx->LocalToPhysAddr(params[2], &pVec);

	Vector vec;
	pBitBuf->ReadBitVec3Coord(vec);

	pVec[0] = sp_ftoc(vec.x);
	pVec[1] = sp_ftoc(vec.y);
	pVec[2] = sp_ftoc(vec.z);

	return 1;
}

static cell_t smn_BfReadVecNormal(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	bf_read *pBitBuf;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=g_HandleSys.ReadHandle(hndl, g_RdBitBufType, &sec, (void **)&pBitBuf))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid bit buffer handle %x (error %d)", hndl, herr);
	}

	cell_t *pVec;
	pCtx->LocalToPhysAddr(params[2], &pVec);

	Vector vec;
	pBitBuf->ReadBitVec3Normal(vec);

	pVec[0] = sp_ftoc(vec.x);
	pVec[1] = sp_ftoc(vec.y);
	pVec[2] = sp_ftoc(vec.z);

	return 1;
}

static cell_t smn_BfReadAngles(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	bf_read *pBitBuf;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=g_HandleSys.ReadHandle(hndl, g_RdBitBufType, &sec, (void **)&pBitBuf))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid bit buffer handle %x (error %d)", hndl, herr);
	}

	cell_t *pAng;
	pCtx->LocalToPhysAddr(params[2], &pAng);

	QAngle ang;
	pBitBuf->ReadBitAngles(ang);

	pAng[0] = sp_ftoc(ang.x);
	pAng[1] = sp_ftoc(ang.y);
	pAng[2] = sp_ftoc(ang.z);

	return 1;
}

REGISTER_NATIVES(bitbufnatives)
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
	{"BfReadBool",				smn_BfReadBool},
	{"BfReadByte",				smn_BfReadByte},
	{"BfReadChar",				smn_BfReadChar},
	{"BfReadShort",				smn_BfReadShort},
	{"BfReadWord",				smn_BfReadWord},
	{"BfReadNum",				smn_BfReadNum},
	{"BfReadFloat",				smn_BfReadFloat},
	{"BfReadString",			smn_BfReadString},
	{"BfReadEntity",			smn_BfReadEntity},
	{"BfReadAngle",				smn_BfReadAngle},
	{"BfReadCoord",				smn_BfReadCoord},
	{"BfReadVecCoord",			smn_BfReadVecCoord},
	{"BfReadVecNormal",			smn_BfReadVecNormal},
	{"BfReadAngles",			smn_BfReadAngles},
	{NULL,						NULL}
};
