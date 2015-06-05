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

#include "UserMessages.h"

#ifndef USE_PROTOBUF_USERMESSAGES

#include "sourcemod.h"
#include <bitbuf.h>
#include <vector.h>
#include <HalfLife2.h>
#include "smn_usermsgs.h"
#include "sourcemod.h"
#include "logic_bridge.h"

static cell_t smn_BfWriteBool(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	bf_write *pBitBuf;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_WrBitBufType, &sec, (void **)&pBitBuf))
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

	if ((herr=handlesys->ReadHandle(hndl, g_WrBitBufType, &sec, (void **)&pBitBuf))
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

	if ((herr=handlesys->ReadHandle(hndl, g_WrBitBufType, &sec, (void **)&pBitBuf))
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

	if ((herr=handlesys->ReadHandle(hndl, g_WrBitBufType, &sec, (void **)&pBitBuf))
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

	if ((herr=handlesys->ReadHandle(hndl, g_WrBitBufType, &sec, (void **)&pBitBuf))
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

	if ((herr=handlesys->ReadHandle(hndl, g_WrBitBufType, &sec, (void **)&pBitBuf))
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

	if ((herr=handlesys->ReadHandle(hndl, g_WrBitBufType, &sec, (void **)&pBitBuf))
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

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_WrBitBufType, &sec, (void **)&pBitBuf))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid bit buffer handle %x (error %d)", hndl, herr);
	}

	char *str;
	pCtx->LocalToString(params[2], &str);

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

	if ((herr=handlesys->ReadHandle(hndl, g_WrBitBufType, &sec, (void **)&pBitBuf))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid bit buffer handle %x (error %d)", hndl, herr);
	}

	int index = g_HL2.ReferenceToIndex(params[2]);

	if (index == -1)
	{
		return 0;
	}

	pBitBuf->WriteShort(index);

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

	if ((herr=handlesys->ReadHandle(hndl, g_WrBitBufType, &sec, (void **)&pBitBuf))
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

	if ((herr=handlesys->ReadHandle(hndl, g_WrBitBufType, &sec, (void **)&pBitBuf))
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

	if ((herr=handlesys->ReadHandle(hndl, g_WrBitBufType, &sec, (void **)&pBitBuf))
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

	if ((herr=handlesys->ReadHandle(hndl, g_WrBitBufType, &sec, (void **)&pBitBuf))
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

	if ((herr=handlesys->ReadHandle(hndl, g_WrBitBufType, &sec, (void **)&pBitBuf))
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

	if ((herr=handlesys->ReadHandle(hndl, g_RdBitBufType, &sec, (void **)&pBitBuf))
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

	if ((herr=handlesys->ReadHandle(hndl, g_RdBitBufType, &sec, (void **)&pBitBuf))
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

	if ((herr=handlesys->ReadHandle(hndl, g_RdBitBufType, &sec, (void **)&pBitBuf))
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

	if ((herr=handlesys->ReadHandle(hndl, g_RdBitBufType, &sec, (void **)&pBitBuf))
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

	if ((herr=handlesys->ReadHandle(hndl, g_RdBitBufType, &sec, (void **)&pBitBuf))
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

	if ((herr=handlesys->ReadHandle(hndl, g_RdBitBufType, &sec, (void **)&pBitBuf))
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

	if ((herr=handlesys->ReadHandle(hndl, g_RdBitBufType, &sec, (void **)&pBitBuf))
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
	int numChars = 0;
	char *buf;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_RdBitBufType, &sec, (void **)&pBitBuf))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid bit buffer handle %x (error %d)", hndl, herr);
	}

	pCtx->LocalToPhysAddr(params[2], (cell_t **)&buf);
	pBitBuf->ReadString(buf, params[3], params[4] ? true : false, &numChars);

	if (pBitBuf->IsOverflowed())
	{
		return -numChars - 1;
	}

	return numChars;
}

static cell_t smn_BfReadEntity(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	bf_read *pBitBuf;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_RdBitBufType, &sec, (void **)&pBitBuf))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid bit buffer handle %x (error %d)", hndl, herr);
	}

	int ref = g_HL2.IndexToReference(pBitBuf->ReadShort());

	return g_HL2.ReferenceToBCompatRef(ref);
}

static cell_t smn_BfReadAngle(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	bf_read *pBitBuf;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_RdBitBufType, &sec, (void **)&pBitBuf))
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

	if ((herr=handlesys->ReadHandle(hndl, g_RdBitBufType, &sec, (void **)&pBitBuf))
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

	if ((herr=handlesys->ReadHandle(hndl, g_RdBitBufType, &sec, (void **)&pBitBuf))
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

	if ((herr=handlesys->ReadHandle(hndl, g_RdBitBufType, &sec, (void **)&pBitBuf))
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

	if ((herr=handlesys->ReadHandle(hndl, g_RdBitBufType, &sec, (void **)&pBitBuf))
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

static cell_t smn_BfGetNumBytesLeft(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	bf_read *pBitBuf;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=handlesys->ReadHandle(hndl, g_RdBitBufType, &sec, (void **)&pBitBuf))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid bit buffer handle %x (error %d)", hndl, herr);
	}

	return pBitBuf->GetNumBitsLeft() >> 3;
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
	{"BfGetNumBytesLeft",		smn_BfGetNumBytesLeft},

	// Transitional syntax support.
	{"BfWrite.WriteBool",		smn_BfWriteBool},
	{"BfWrite.WriteByte",		smn_BfWriteByte},
	{"BfWrite.WriteChar",		smn_BfWriteChar},
	{"BfWrite.WriteShort",		smn_BfWriteShort},
	{"BfWrite.WriteWord",		smn_BfWriteWord},
	{"BfWrite.WriteNum",		smn_BfWriteNum},
	{"BfWrite.WriteFloat",		smn_BfWriteFloat},
	{"BfWrite.WriteString",		smn_BfWriteString},
	{"BfWrite.WriteEntity",		smn_BfWriteEntity},
	{"BfWrite.WriteAngle",		smn_BfWriteAngle},
	{"BfWrite.WriteCoord",		smn_BfWriteCoord},
	{"BfWrite.WriteVecCoord",	smn_BfWriteVecCoord},
	{"BfWrite.WriteVecNormal",	smn_BfWriteVecNormal},
	{"BfWrite.WriteAngles",		smn_BfWriteAngles},

	{"BfRead.ReadBool",			smn_BfReadBool},
	{"BfRead.ReadByte",			smn_BfReadByte},
	{"BfRead.ReadChar",			smn_BfReadChar},
	{"BfRead.ReadShort",		smn_BfReadShort},
	{"BfRead.ReadWord",			smn_BfReadWord},
	{"BfRead.ReadNum",			smn_BfReadNum},
	{"BfRead.ReadFloat",		smn_BfReadFloat},
	{"BfRead.ReadString",		smn_BfReadString},
	{"BfRead.ReadEntity",		smn_BfReadEntity},
	{"BfRead.ReadAngle",		smn_BfReadAngle},
	{"BfRead.ReadCoord",		smn_BfReadCoord},
	{"BfRead.ReadVecCoord",		smn_BfReadVecCoord},
	{"BfRead.ReadVecNormal",	smn_BfReadVecNormal},
	{"BfRead.ReadAngles",		smn_BfReadAngles},
	{"BfRead.BytesLeft.get",	smn_BfGetNumBytesLeft},

	{NULL,						NULL}
};

#endif
