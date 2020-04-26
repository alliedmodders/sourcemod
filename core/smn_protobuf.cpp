/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2013 AlliedModders LLC.  All rights reserved.
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

#include "logic_bridge.h"
#include "UserMessages.h"
#include "UserMessagePBHelpers.h"
#include "smn_usermsgs.h"
#include <IHandleSys.h>

// Assumes pbuf message handle is param 1, gets message as msg
#define GET_MSG_FROM_HANDLE_OR_ERR()                  \
	Handle_t hndl = static_cast<Handle_t>(params[1]); \
	HandleError herr;                                 \
	HandleSecurity sec;                               \
	SMProtobufMessage *msg;                           \
	                                                  \
	sec.pOwner = NULL;                                \
	sec.pIdentity = g_pCoreIdent;                     \
	                                                  \
	if ((herr=handlesys->ReadHandle(hndl, g_ProtobufType, &sec, (void **)&msg)) \
		!= HandleError_None)                          \
	{                                                 \
		return pCtx->ThrowNativeError("Invalid protobuf message handle %x (error %d)", hndl, herr); \
	}

// Assumes message field name is param 2, gets as strField
#define GET_FIELD_NAME_OR_ERR()                                           \
	char *strField;                                                       \
	pCtx->LocalToString(params[2], &strField);

static cell_t smn_PbReadInt(IPluginContext *pCtx, const cell_t *params)
{
	GET_MSG_FROM_HANDLE_OR_ERR();
	GET_FIELD_NAME_OR_ERR();

	int ret;

	int index = params[0] >= 3 ? params[3] : -1;
	if (index < 0)
	{
		if (!msg->GetInt32OrUnsignedOrEnum(strField, &ret))
		{
			return pCtx->ThrowNativeError("Invalid field \"%s\" for message \"%s\"", strField, msg->GetProtobufMessage()->GetTypeName().c_str());
		}
	}
	else
	{
		if (!msg->GetRepeatedInt32OrUnsignedOrEnum(strField, index, &ret))
		{
			return pCtx->ThrowNativeError("Invalid field \"%s\"[%d] for message \"%s\"", strField, index, msg->GetProtobufMessage()->GetTypeName().c_str());
		}
	}

	return ret;
}

static cell_t smn_PbReadInt64(IPluginContext *pCtx, const cell_t *params)
{
	GET_MSG_FROM_HANDLE_OR_ERR();
	GET_FIELD_NAME_OR_ERR();
	
	cell_t *ret;
	pCtx->LocalToPhysAddr(params[3], &ret);
	int64 temp;
	((cell_t *)&temp)[0] = ret[0];
	((cell_t *)&temp)[1] = ret[1];
	
	if (params[4] < 0)
	{
		if (!msg->GetInt64OrUnsigned(strField, &temp))
		{
			return pCtx->ThrowNativeError("Invalid field \"%s\" for message \"%s\"", strField, msg->GetProtobufMessage()->GetTypeName().c_str());
		}
	}
	else
	{
		if (!msg->GetRepeatedInt64OrUnsigned(strField, params[4], &temp))
		{
			return pCtx->ThrowNativeError("Invalid field \"%s\"[%d] for message \"%s\"", strField, params[4], msg->GetProtobufMessage()->GetTypeName().c_str());
		}
	}
	
	ret[0] = ((cell_t *)&temp)[0];
	ret[1] = ((cell_t *)&temp)[1];
	
	return 1;
}

static cell_t smn_PbReadFloat(IPluginContext *pCtx, const cell_t *params)
{
	GET_MSG_FROM_HANDLE_OR_ERR();
	GET_FIELD_NAME_OR_ERR();

	float ret;

	int index = params[0] >= 3 ? params[3] : -1;
	if (index < 0)
	{
		if (!msg->GetFloatOrDouble(strField, &ret))
		{
			return pCtx->ThrowNativeError("Invalid field \"%s\" for message \"%s\"", strField, msg->GetProtobufMessage()->GetTypeName().c_str());
		}
	}
	else
	{
		if (!msg->GetRepeatedFloatOrDouble(strField, index, &ret))
		{
			return pCtx->ThrowNativeError("Invalid field \"%s\"[%d] for message \"%s\"", strField, index, msg->GetProtobufMessage()->GetTypeName().c_str());
		}
	}

	return sp_ftoc(ret);
}

static cell_t smn_PbReadBool(IPluginContext *pCtx, const cell_t *params)
{
	GET_MSG_FROM_HANDLE_OR_ERR();
	GET_FIELD_NAME_OR_ERR();

	bool ret;

	int index = params[0] >= 3 ? params[3] : -1;
	if (index < 0)
	{
		if (!msg->GetBool(strField, &ret))
		{
			return pCtx->ThrowNativeError("Invalid field \"%s\" for message \"%s\"", strField, msg->GetProtobufMessage()->GetTypeName().c_str());
		}
	}
	else
	{
		if (!msg->GetRepeatedBool(strField, index, &ret))
		{
			return pCtx->ThrowNativeError("Invalid field \"%s\"[%d] for message \"%s\"", strField, index, msg->GetProtobufMessage()->GetTypeName().c_str());
		} 
	}

	return ret ? 1 : 0;
}

static cell_t smn_PbReadString(IPluginContext *pCtx, const cell_t *params)
{
	GET_MSG_FROM_HANDLE_OR_ERR();
	GET_FIELD_NAME_OR_ERR();

	char *buf;
	pCtx->LocalToPhysAddr(params[3], (cell_t **)&buf);

	int index = params[0] >= 5 ? params[5] : -1;

	if (index < 0)
	{
		if (!msg->GetString(strField, buf, params[4]))
		{
			return pCtx->ThrowNativeError("Invalid field \"%s\" for message \"%s\"", strField, msg->GetProtobufMessage()->GetTypeName().c_str());
		}
	}
	else
	{
		if (!msg->GetRepeatedString(strField, index, buf, params[4]))
		{
			return pCtx->ThrowNativeError("Invalid field \"%s\"[%d] for message \"%s\"", strField, index, msg->GetProtobufMessage()->GetTypeName().c_str());
		}
	}

	return 1;
}

static cell_t smn_PbReadColor(IPluginContext *pCtx, const cell_t *params)
{
	GET_MSG_FROM_HANDLE_OR_ERR();
	GET_FIELD_NAME_OR_ERR();

	cell_t *out;
	pCtx->LocalToPhysAddr(params[3], &out);

	Color clr;
	int index = params[0] >= 4 ? params[4] : -1;
	if (index < 0)
	{
		if (!msg->GetColor(strField, &clr))
		{
			return pCtx->ThrowNativeError("Invalid field \"%s\" for message \"%s\"", strField, msg->GetProtobufMessage()->GetTypeName().c_str());
		}
	}
	else
	{
		if (!msg->GetRepeatedColor(strField, index, &clr))
		{
			return pCtx->ThrowNativeError("Invalid field \"%s\"[%d] for message \"%s\"", strField, index, msg->GetProtobufMessage()->GetTypeName().c_str());
		}
	}

	out[0] = clr.r();
	out[1] = clr.g();
	out[2] = clr.b();
	out[3] = clr.a();

	return 1;
}

static cell_t smn_PbReadAngle(IPluginContext *pCtx, const cell_t *params)
{
	GET_MSG_FROM_HANDLE_OR_ERR();
	GET_FIELD_NAME_OR_ERR();

	cell_t *out;
	pCtx->LocalToPhysAddr(params[3], &out);

	QAngle ang;
	int index = params[0] >= 4 ? params[4] : -1;
	if (index < 0)
	{
		if (!msg->GetQAngle(strField, &ang))
		{
			return pCtx->ThrowNativeError("Invalid field \"%s\" for message \"%s\"", strField, msg->GetProtobufMessage()->GetTypeName().c_str());
		}
	}
	else
	{
		if (!msg->GetRepeatedQAngle(strField, index, &ang))
		{
		return pCtx->ThrowNativeError("Invalid field \"%s\"[%d] for message \"%s\"", strField, index, msg->GetProtobufMessage()->GetTypeName().c_str());
		}
	}

	out[0] = sp_ftoc(ang.x);
	out[1] = sp_ftoc(ang.y);
	out[2] = sp_ftoc(ang.z);

	return 1;
}

static cell_t smn_PbReadVector(IPluginContext *pCtx, const cell_t *params)
{
	GET_MSG_FROM_HANDLE_OR_ERR();
	GET_FIELD_NAME_OR_ERR();

	cell_t *out;
	pCtx->LocalToPhysAddr(params[3], &out);

	Vector vec;
	int index = params[0] >= 4 ? params[4] : -1;
	if (index < 0)
	{
		if (!msg->GetVector(strField, &vec))
		{
			return pCtx->ThrowNativeError("Invalid field \"%s\" for message \"%s\"", strField, msg->GetProtobufMessage()->GetTypeName().c_str());
		}
	}
	else
	{
		if (!msg->GetRepeatedVector(strField, index, &vec))
		{
			return pCtx->ThrowNativeError("Invalid field \"%s\"[%d] for message \"%s\"", strField, index, msg->GetProtobufMessage()->GetTypeName().c_str());
		}
	}

	out[0] = sp_ftoc(vec.x);
	out[1] = sp_ftoc(vec.y);
	out[2] = sp_ftoc(vec.z);

	return 1;
}

static cell_t smn_PbReadVector2D(IPluginContext *pCtx, const cell_t *params)
{
	GET_MSG_FROM_HANDLE_OR_ERR();
	GET_FIELD_NAME_OR_ERR();

	cell_t *out;
	pCtx->LocalToPhysAddr(params[3], &out);

	Vector2D vec;
	int index = params[0] >= 4 ? params[4] : -1;
	if (index < 0)
	{
		if (!msg->GetVector2D(strField, &vec))
		{
			return pCtx->ThrowNativeError("Invalid field \"%s\" for message \"%s\"", strField, msg->GetProtobufMessage()->GetTypeName().c_str());
		}
	}
	else
	{
		if (!msg->GetRepeatedVector2D(strField, index, &vec))
		{
			return pCtx->ThrowNativeError("Invalid field \"%s\"[%d] for message \"%s\"", strField, index, msg->GetProtobufMessage()->GetTypeName().c_str());
		}
	}

	out[0] = sp_ftoc(vec.x);
	out[1] = sp_ftoc(vec.y);

	return 1;
}

static cell_t smn_PbGetRepeatedFieldCount(IPluginContext *pCtx, const cell_t *params)
{
	GET_MSG_FROM_HANDLE_OR_ERR();
	GET_FIELD_NAME_OR_ERR();

	int cnt = msg->GetRepeatedFieldCount(strField);
	if (cnt == -1)
	{
		return pCtx->ThrowNativeError("Invalid field \"%s\" for message \"%s\"", strField, msg->GetProtobufMessage()->GetTypeName().c_str());
	}

	return cnt;
}

static cell_t smn_PbHasField(IPluginContext *pCtx, const cell_t *params)
{
	GET_MSG_FROM_HANDLE_OR_ERR();
	GET_FIELD_NAME_OR_ERR();

	return msg->HasField(strField) ? 1 : 0;
}

static cell_t smn_PbSetInt(IPluginContext *pCtx, const cell_t *params)
{
	GET_MSG_FROM_HANDLE_OR_ERR();
	GET_FIELD_NAME_OR_ERR();

	int index = params[0] >= 4 ? params[4] : -1;
	if (index < 0)
	{
		if (!msg->SetInt32OrUnsignedOrEnum(strField, params[3]))
		{
			return pCtx->ThrowNativeError("Invalid field \"%s\" for message \"%s\"", strField, msg->GetProtobufMessage()->GetTypeName().c_str());
		}
	}
	else
	{
		if (!msg->SetRepeatedInt32OrUnsignedOrEnum(strField, index, params[3]))
		{
			return pCtx->ThrowNativeError("Invalid field \"%s\"[%d] for message \"%s\"", strField, index, msg->GetProtobufMessage()->GetTypeName().c_str());
		}
	}

	return 1;
}

static cell_t smn_PbSetInt64(IPluginContext *pCtx, const cell_t *params)
{
	GET_MSG_FROM_HANDLE_OR_ERR();
	GET_FIELD_NAME_OR_ERR();
	
	cell_t *value;
	pCtx->LocalToPhysAddr(params[3], &value);
	int64 temp;
	((cell_t *)&temp)[0] = value[0];
	((cell_t *)&temp)[1] = value[1];
	
	if (params[4] < 0)
	{
		if (!msg->SetInt64OrUnsigned(strField, temp))
		{
			return pCtx->ThrowNativeError("Invalid field \"%s\" for message \"%s\"", strField, msg->GetProtobufMessage()->GetTypeName().c_str());
		}
	}
	else
	{
		if (!msg->SetRepeatedInt64OrUnsigned(strField, params[4], temp))
		{
			return pCtx->ThrowNativeError("Invalid field \"%s\"[%d] for message \"%s\"", strField, params[4], msg->GetProtobufMessage()->GetTypeName().c_str());
		}
	}

	return 1;
}

static cell_t smn_PbSetFloat(IPluginContext *pCtx, const cell_t *params)
{
	GET_MSG_FROM_HANDLE_OR_ERR();
	GET_FIELD_NAME_OR_ERR();

	int index = params[0] >= 4 ? params[4] : -1;
	if (index < 0)
	{
		if (!msg->SetFloatOrDouble(strField, sp_ctof(params[3])))
		{
			return pCtx->ThrowNativeError("Invalid field \"%s\" for message \"%s\"", strField, msg->GetProtobufMessage()->GetTypeName().c_str());
		}
	}
	else
	{
		if (!msg->SetRepeatedFloatOrDouble(strField, index, sp_ctof(params[3])))
		{
			return pCtx->ThrowNativeError("Invalid field \"%s\"[%d] for message \"%s\"", strField, index, msg->GetProtobufMessage()->GetTypeName().c_str());
		}
	}

	return 1;
}

static cell_t smn_PbSetBool(IPluginContext *pCtx, const cell_t *params)
{
	GET_MSG_FROM_HANDLE_OR_ERR();
	GET_FIELD_NAME_OR_ERR();

	bool value = (params[3] == 0 ? false : true);
	int index = params[0] >= 4 ? params[4] : -1;
	if (index < 0)
	{
		if (!msg->SetBool(strField, value))
		{
			return pCtx->ThrowNativeError("Invalid field \"%s\" for message \"%s\"", strField, msg->GetProtobufMessage()->GetTypeName().c_str());
		}
	}
	else
	{
		if (!msg->SetRepeatedBool(strField, index, value))
		{
			return pCtx->ThrowNativeError("Invalid field \"%s\"[%d] for message \"%s\"", strField, index, msg->GetProtobufMessage()->GetTypeName().c_str());
		}
	}

	return 1;
}

static cell_t smn_PbSetString(IPluginContext *pCtx, const cell_t *params)
{
	GET_MSG_FROM_HANDLE_OR_ERR();
	GET_FIELD_NAME_OR_ERR();

	char *strValue;
	pCtx->LocalToString(params[3], &strValue);

	int index = params[0] >= 4 ? params[4] : -1;
	if (index < 0)
	{
		if (!msg->SetString(strField, strValue))
		{
			return pCtx->ThrowNativeError("Invalid field \"%s\" for message \"%s\"", strField, msg->GetProtobufMessage()->GetTypeName().c_str());
		}
	}
	else
	{
		if (!msg->SetRepeatedString(strField, index, strValue))
		{
			return pCtx->ThrowNativeError("Invalid field \"%s\"[%d] for message \"%s\"", strField, index, msg->GetProtobufMessage()->GetTypeName().c_str());
		}
	}

	return 1;
}

static cell_t smn_PbSetColor(IPluginContext *pCtx, const cell_t *params)
{
	GET_MSG_FROM_HANDLE_OR_ERR();
	GET_FIELD_NAME_OR_ERR();

	cell_t *clrParams;
	pCtx->LocalToPhysAddr(params[3], &clrParams);

	Color clr(
		clrParams[0],
		clrParams[1],
		clrParams[2],
		clrParams[3]);

	int index = params[0] >= 4 ? params[4] : -1;
	if (index < 0)
	{
		if (!msg->SetColor(strField, clr))
		{
			return pCtx->ThrowNativeError("Invalid field \"%s\" for message \"%s\"", strField, msg->GetProtobufMessage()->GetTypeName().c_str());
		}
	}
	else
	{
		if (!msg->SetRepeatedColor(strField, index, clr))
		{
			return pCtx->ThrowNativeError("Invalid field \"%s\"[%d] for message \"%s\"", strField, index, msg->GetProtobufMessage()->GetTypeName().c_str());
		}
	}

	return 1;
}

static cell_t smn_PbSetAngle(IPluginContext *pCtx, const cell_t *params)
{
	GET_MSG_FROM_HANDLE_OR_ERR();
	GET_FIELD_NAME_OR_ERR();

	cell_t *angParams;
	pCtx->LocalToPhysAddr(params[3], &angParams);

	QAngle ang(
		sp_ctof(angParams[0]),
		sp_ctof(angParams[1]),
		sp_ctof(angParams[2]));

	int index = params[0] >= 4 ? params[4] : -1;
	if (index < 0)
	{
		if (!msg->SetQAngle(strField, ang))
		{
			return pCtx->ThrowNativeError("Invalid field \"%s\" for message \"%s\"", strField, msg->GetProtobufMessage()->GetTypeName().c_str());
		}
	}
	else
	{
		if (!msg->SetRepeatedQAngle(strField, index, ang))
		{
			return pCtx->ThrowNativeError("Invalid field \"%s\"[%d] for message \"%s\"", strField, index, msg->GetProtobufMessage()->GetTypeName().c_str());
		}
	}

	return 1;
}

static cell_t smn_PbSetVector(IPluginContext *pCtx, const cell_t *params)
{
	GET_MSG_FROM_HANDLE_OR_ERR();
	GET_FIELD_NAME_OR_ERR();

	cell_t *vecParams;
	pCtx->LocalToPhysAddr(params[3], &vecParams);

	Vector vec(
		sp_ctof(vecParams[0]),
		sp_ctof(vecParams[1]),
		sp_ctof(vecParams[2]));

	int index = params[0] >= 4 ? params[4] : -1;
	if (index < 0)
	{
		if (!msg->SetVector(strField, vec))
		{
			return pCtx->ThrowNativeError("Invalid field \"%s\" for message \"%s\"", strField, msg->GetProtobufMessage()->GetTypeName().c_str());
		}
	}
	else
	{
		if (!msg->SetRepeatedVector(strField, index, vec))
		{
			return pCtx->ThrowNativeError("Invalid field \"%s\"[%d] for message \"%s\"", strField, index, msg->GetProtobufMessage()->GetTypeName().c_str());
		}
	}

	return 1;
}

static cell_t smn_PbSetVector2D(IPluginContext *pCtx, const cell_t *params)
{
	GET_MSG_FROM_HANDLE_OR_ERR();
	GET_FIELD_NAME_OR_ERR();

	cell_t *vecParams;
	pCtx->LocalToPhysAddr(params[3], &vecParams);

	Vector2D vec(
		sp_ctof(vecParams[0]),
		sp_ctof(vecParams[1]));

	int index = params[0] >= 4 ? params[4] : -1;
	if (index < 0)
	{
		if (!msg->SetVector2D(strField, vec))
		{
			return pCtx->ThrowNativeError("Invalid field \"%s\" for message \"%s\"", strField, msg->GetProtobufMessage()->GetTypeName().c_str());
		}
	}
	else
	{
		if (!msg->SetRepeatedVector2D(strField, index, vec))
		{
			return pCtx->ThrowNativeError("Invalid field \"%s\"[%d] for message \"%s\"", strField, index, msg->GetProtobufMessage()->GetTypeName().c_str());
		}
	}

	return 1;
}

static cell_t smn_PbAddInt(IPluginContext *pCtx, const cell_t *params)
{
	GET_MSG_FROM_HANDLE_OR_ERR();
	GET_FIELD_NAME_OR_ERR();

	if (!msg->AddInt32OrUnsignedOrEnum(strField, params[3]))
	{
		return pCtx->ThrowNativeError("Invalid field \"%s\" for message \"%s\"", strField, msg->GetProtobufMessage()->GetTypeName().c_str());
	}

	return 1;
}

static cell_t smn_PbAddInt64(IPluginContext *pCtx, const cell_t *params)
{
	GET_MSG_FROM_HANDLE_OR_ERR();
	GET_FIELD_NAME_OR_ERR();
	
	cell_t *value;
	pCtx->LocalToPhysAddr(params[3], &value);
	int64 temp;
	((cell_t *)&temp)[0] = value[0];
	((cell_t *)&temp)[1] = value[1];

	if (!msg->AddInt64OrUnsigned(strField, temp))
	{
		return pCtx->ThrowNativeError("Invalid field \"%s\" for message \"%s\"", strField, msg->GetProtobufMessage()->GetTypeName().c_str());
	}

	return 1;
}

static cell_t smn_PbAddFloat(IPluginContext *pCtx, const cell_t *params)
{
	GET_MSG_FROM_HANDLE_OR_ERR();
	GET_FIELD_NAME_OR_ERR();

	if (!msg->AddFloatOrDouble(strField, sp_ctof(params[3])))
	{
		return pCtx->ThrowNativeError("Invalid field \"%s\" for message \"%s\"", strField, msg->GetProtobufMessage()->GetTypeName().c_str());
	}

	return 1;
}

static cell_t smn_PbAddBool(IPluginContext *pCtx, const cell_t *params)
{
	GET_MSG_FROM_HANDLE_OR_ERR();
	GET_FIELD_NAME_OR_ERR();

	bool value = (params[3] == 0 ? false : true);
	if (!msg->AddBool(strField, value))
	{
		return pCtx->ThrowNativeError("Invalid field \"%s\" for message \"%s\"", strField, msg->GetProtobufMessage()->GetTypeName().c_str());
	}

	return 1;
}

static cell_t smn_PbAddString(IPluginContext *pCtx, const cell_t *params)
{
	GET_MSG_FROM_HANDLE_OR_ERR();
	GET_FIELD_NAME_OR_ERR();

	char *strValue;
	pCtx->LocalToString(params[3], &strValue);

	if (!msg->AddString(strField, strValue))
	{
		return pCtx->ThrowNativeError("Invalid field \"%s\" for message \"%s\"", strField, msg->GetProtobufMessage()->GetTypeName().c_str());
	}

	return 1;
}

static cell_t smn_PbAddColor(IPluginContext *pCtx, const cell_t *params)
{
	GET_MSG_FROM_HANDLE_OR_ERR();
	GET_FIELD_NAME_OR_ERR();

	cell_t *clrParams;
	pCtx->LocalToPhysAddr(params[3], &clrParams);

	Color clr(
		clrParams[0],
		clrParams[1],
		clrParams[2],
		clrParams[3]);

	if (!msg->AddColor(strField, clr))
	{
		return pCtx->ThrowNativeError("Invalid field \"%s\" for message \"%s\"", strField, msg->GetProtobufMessage()->GetTypeName().c_str());
	}

	return 1;
}

static cell_t smn_PbAddAngle(IPluginContext *pCtx, const cell_t *params)
{
	GET_MSG_FROM_HANDLE_OR_ERR();
	GET_FIELD_NAME_OR_ERR();

	cell_t *angParams;
	pCtx->LocalToPhysAddr(params[3], &angParams);

	QAngle ang(
		sp_ctof(angParams[0]),
		sp_ctof(angParams[1]),
		sp_ctof(angParams[2]));

	if (!msg->AddQAngle(strField, ang))
	{
		return pCtx->ThrowNativeError("Invalid field \"%s\" for message \"%s\"", strField, msg->GetProtobufMessage()->GetTypeName().c_str());
	}

	return 1;
}

static cell_t smn_PbAddVector(IPluginContext *pCtx, const cell_t *params)
{
	GET_MSG_FROM_HANDLE_OR_ERR();
	GET_FIELD_NAME_OR_ERR();

	cell_t *vecParams;
	pCtx->LocalToPhysAddr(params[3], &vecParams);

	Vector vec(
		sp_ctof(vecParams[0]),
		sp_ctof(vecParams[1]),
		sp_ctof(vecParams[2]));

	if (!msg->AddVector(strField, vec))
	{
		return pCtx->ThrowNativeError("Invalid field \"%s\" for message \"%s\"", strField, msg->GetProtobufMessage()->GetTypeName().c_str());
	}

	return 1;
}

static cell_t smn_PbAddVector2D(IPluginContext *pCtx, const cell_t *params)
{
	GET_MSG_FROM_HANDLE_OR_ERR();
	GET_FIELD_NAME_OR_ERR();

	cell_t *vecParams;
	pCtx->LocalToPhysAddr(params[3], &vecParams);

	Vector2D vec(
		sp_ctof(vecParams[0]),
		sp_ctof(vecParams[1]));

	if (!msg->AddVector2D(strField, vec))
	{
		return pCtx->ThrowNativeError("Invalid field \"%s\" for message \"%s\"", strField, msg->GetProtobufMessage()->GetTypeName().c_str());
	}

	return 1;
}

static cell_t smn_PbRemoveRepeatedFieldValue(IPluginContext *pCtx, const cell_t *params)
{
	GET_MSG_FROM_HANDLE_OR_ERR();
	GET_FIELD_NAME_OR_ERR();

	if (!msg->RemoveRepeatedFieldValue(strField, params[3]))
	{
		return pCtx->ThrowNativeError("Invalid field \"%s\" for message \"%s\"", strField, msg->GetProtobufMessage()->GetTypeName().c_str());
	}

	return 1;
}

static cell_t smn_PbReadMessage(IPluginContext *pCtx, const cell_t *params)
{
	GET_MSG_FROM_HANDLE_OR_ERR();
	GET_FIELD_NAME_OR_ERR();

	protobuf::Message *innerMsg;
	if (!msg->GetMessage(strField, &innerMsg))
	{
		return pCtx->ThrowNativeError("Invalid field \"%s\" for message \"%s\"", strField, msg->GetProtobufMessage()->GetTypeName().c_str());
	}

	Handle_t outHndl = handlesys->CreateHandle(g_ProtobufType, new SMProtobufMessage(innerMsg), NULL, g_pCoreIdent, NULL);
	msg->AddChildHandle(outHndl);

	return outHndl;
}

static cell_t smn_PbReadRepeatedMessage(IPluginContext *pCtx, const cell_t *params)
{
	GET_MSG_FROM_HANDLE_OR_ERR();
	GET_FIELD_NAME_OR_ERR();

	const protobuf::Message *innerMsg;
	if (!msg->GetRepeatedMessage(strField, params[3], &innerMsg))
	{
		return pCtx->ThrowNativeError("Invalid field \"%s\"[%d] for message \"%s\"", strField, params[3], msg->GetProtobufMessage()->GetTypeName().c_str());
	}

	Handle_t outHndl = handlesys->CreateHandle(g_ProtobufType, new SMProtobufMessage(const_cast<protobuf::Message *>(innerMsg)), NULL, g_pCoreIdent, NULL);
	msg->AddChildHandle(outHndl);

	return outHndl;
}

static cell_t smn_PbAddMessage(IPluginContext *pCtx, const cell_t *params)
{
	GET_MSG_FROM_HANDLE_OR_ERR();
	GET_FIELD_NAME_OR_ERR();

	protobuf::Message *innerMsg;
	if (!msg->AddMessage(strField, &innerMsg))
	{
		return pCtx->ThrowNativeError("Invalid field \"%s\" for message \"%s\"", strField, msg->GetProtobufMessage()->GetTypeName().c_str());
	}

	Handle_t outHndl = handlesys->CreateHandle(g_ProtobufType, new SMProtobufMessage(innerMsg), NULL, g_pCoreIdent, NULL);
	msg->AddChildHandle(outHndl);

	return outHndl;
}

REGISTER_NATIVES(protobufnatives)
{
	{"PbReadInt",					smn_PbReadInt},
	{"PbReadFloat",					smn_PbReadFloat},
	{"PbReadBool",					smn_PbReadBool},
	{"PbReadString",				smn_PbReadString},
	{"PbReadColor",					smn_PbReadColor},
	{"PbReadAngle",					smn_PbReadAngle},
	{"PbReadVector",				smn_PbReadVector},
	{"PbReadVector2D",				smn_PbReadVector2D},
	{"PbGetRepeatedFieldCount",		smn_PbGetRepeatedFieldCount},
	{"PbSetInt",					smn_PbSetInt},
	{"PbSetFloat",					smn_PbSetFloat},
	{"PbSetBool",					smn_PbSetBool},
	{"PbSetString",					smn_PbSetString},
	{"PbSetColor",					smn_PbSetColor},
	{"PbSetAngle",					smn_PbSetAngle},
	{"PbSetVector",					smn_PbSetVector},
	{"PbSetVector2D",				smn_PbSetVector2D},
	{"PbAddInt",					smn_PbAddInt},
	{"PbAddFloat",					smn_PbAddFloat},
	{"PbAddBool",					smn_PbAddBool},
	{"PbAddString",					smn_PbAddString},
	{"PbAddColor",					smn_PbAddColor},
	{"PbAddAngle",					smn_PbAddAngle},
	{"PbAddVector",					smn_PbAddVector},
	{"PbAddVector2D",				smn_PbAddVector2D},
	{"PbRemoveRepeatedFieldValue",	smn_PbRemoveRepeatedFieldValue},
	{"PbReadMessage",				smn_PbReadMessage},
	{"PbReadRepeatedMessage",		smn_PbReadRepeatedMessage},
	{"PbAddMessage",				smn_PbAddMessage},

	// Transitional syntax.
	{"Protobuf.ReadInt",					smn_PbReadInt},
	{"Protobuf.ReadInt64",					smn_PbReadInt64},
	{"Protobuf.ReadFloat",					smn_PbReadFloat},
	{"Protobuf.ReadBool",					smn_PbReadBool},
	{"Protobuf.ReadString",					smn_PbReadString},
	{"Protobuf.ReadColor",					smn_PbReadColor},
	{"Protobuf.ReadAngle",					smn_PbReadAngle},
	{"Protobuf.ReadVector",					smn_PbReadVector},
	{"Protobuf.ReadVector2D",				smn_PbReadVector2D},
	{"Protobuf.GetRepeatedFieldCount",		smn_PbGetRepeatedFieldCount},
	{"Protobuf.HasField",					smn_PbHasField},
	{"Protobuf.SetInt",						smn_PbSetInt},
	{"Protobuf.SetInt64",					smn_PbSetInt64},
	{"Protobuf.SetFloat",					smn_PbSetFloat},
	{"Protobuf.SetBool",					smn_PbSetBool},
	{"Protobuf.SetString",					smn_PbSetString},
	{"Protobuf.SetColor",					smn_PbSetColor},
	{"Protobuf.SetAngle",					smn_PbSetAngle},
	{"Protobuf.SetVector",					smn_PbSetVector},
	{"Protobuf.SetVector2D",				smn_PbSetVector2D},
	{"Protobuf.AddInt",						smn_PbAddInt},
	{"Protobuf.AddInt64",					smn_PbAddInt64},
	{"Protobuf.AddFloat",					smn_PbAddFloat},
	{"Protobuf.AddBool",					smn_PbAddBool},
	{"Protobuf.AddString",					smn_PbAddString},
	{"Protobuf.AddColor",					smn_PbAddColor},
	{"Protobuf.AddAngle",					smn_PbAddAngle},
	{"Protobuf.AddVector",					smn_PbAddVector},
	{"Protobuf.AddVector2D",				smn_PbAddVector2D},
	{"Protobuf.RemoveRepeatedFieldValue",	smn_PbRemoveRepeatedFieldValue},
	{"Protobuf.ReadMessage",				smn_PbReadMessage},
	{"Protobuf.ReadRepeatedMessage",		smn_PbReadRepeatedMessage},
	{"Protobuf.AddMessage",					smn_PbAddMessage},

	{NULL,							NULL}
};

/*
                                       @@@@@@@@@@@@@ 
                                     @@@@@@@@@@@@@@@@@@
                                     @@@@@@@@@@@@@@@@@@@@@      
                                    @@@@@@@@@@@@@@@@@@@@@@@@               
                                   @@@@@@@@    @@@@@@@@@@@@@
                                   @@@@@@@@ o  @@@@@@@@@@@@@          
                                      @@@@@@  @@@@@@@@@@@@@@@             
                                 0000   @@@@@@@@@@@@@@@@@@@@@                
                              000000000   @@@@@@@@@@@@@@@@@@@           
                       0000000000000000000   @@@@@@@@@@@@@@@          
                       00000000000000000000  @@@@@@@@@@@@@@@            
                            000000000       @@@@@@@@@@@@@@@           
                                         @@@@@@@@@@@@@@@@@               
                                        @@@@@@@@@@@@@@@@@@      
                                       @@@@@@@@@@@@@@@@@@       
                                       @@@@@@@@@@@@@@@@@@                                                                @@@@
                                      @@@@@@@@@@@@@@@@@@@                                                             @@@@@@@  
                                     @@@   @@@@    @@@                                                             @@@@@@@@@@     
                                    @@@@@@@@@@@@@@@@@@@@@@@@@@                                                 @@@@@@@@@@  @@@@@    
                                   @@@@@@@@@@@@@@@@@@@@@@@@@@ @@@   @@@@@@@@                                @@  @@@@@@@@@@@@@@@ 
                                  @@@@@@@@@@@@@@@@@@@@@@@@@@ @@   @@@@@@@@@@@@@   @@@@@@@@@@@@@@@@@@@@@@@@@  @@  @@@@@@@@@        
                                  @@@@@@@@@@@@@@@@@@@@@@@ @@@@   @@@@@@@@@@@@@@@@  @@@@@@@@@@@@@@@@@@@@@@@@@  @@  @@@@@@@@@@@@@
                                  @@@@@@@@@@@@@@@@@@@@@ @@@@    @@@@@@@@@@@@@@@@@    @@@@@@@@@@@@@@@@@@@@@@  @@@  @@@@@@@@@@@@     
                                 @@@@@@@@@@@@@@@@@@@@ @@@@@@   @@@@@@@@@@@@@@@@@@   @@@@@@@@@@@@@@@@@@@@@   @@@@  @@@@@@@@@@@     
                                 @@@@@@@@@@@@@@@@@  @@@@@@    @@@@@@@@@@@@@@@@@@@  @@@@@@@@@@@@@@@@@@@      @@@@@ @@@@@@      
                                 @@@@@@@@@       @@@@@@@@@   @@@@@@@@@@@@@@@@@@   @@@@@@@@@@@@@@@@@     @@  @@@@@ @@@@@@@@   
                                           @@@@@@@@@@@@@@   @@@@@@@@@@@@@@@@@@@@   @@@@@@@@@@@@@@   @@@@@@  @@@@  @@@@@@  
                                  @@@@@@@@@@@@@@@@@@@@@@    @@@@@@@@@@@@@@@@@@@@@  @@@@@@         @@@@@@  @@@@@@
                                 @@@@@@@@@@@@@@@@@@@@@@@    @@@@@@@@@@@@@@@@@@@@@              @@@@@@@@   @@@@
                                  @@@@@@@@@@@@@@@@@@@@@@   @@@@@@@@@@@@@@@@@@@@       @@@@@@@@@@@@@@  @  @@@@@  
                                   @@@@@@@@@@@@@@@@@@@@@@   @@@@@@@@@@@@@@@@@@@@@ @@@@@@@@@@@    @@@@@  @@@@    ////
                     ////////////   @@@@@@@@@@@ @@@@@@       @@@@@@@@@@@@@@@@@@@@  @@      @@@@@@@@@  @@@@    //// //////////// 
              ///////////////// ///    @@@@@          @@@@    @@@@@@@@@@@@@@@@@@@   @@@@@@@@@@@@@     @@@@   ///// ///////  /////////    
        /////////////////////  /////         @@@@@@@@@@@@@@   @@@@@@@@@@@@@@@@@@@   @@@@@@@      @  @@@@    ////// ///////  ///////////////
    ///////////////////////// ///////    @@@@@@@@@@@@@@@@@@@    @@@@@@@@@@@@@@@           @@@@@   @@@@    //////  ////////  //////////////////// 
 ////////////////////////////  //////////   @@@@@@@@@@@@@@@@@@    @@@@@@@@@@@@   @@@@@@@@@     @@@      //////  ////////// ////////////////////////
//////////////////// //////////  ///////////   @@@@@@@@@@@@@@@@@     @@@@@@@@   @@@      @@@@@@      ////// ////////////  //////////////////////////
//////////////////// ////////////   ///////////   @@@@@@@@@@@@@@@@               @@@@            /////////////////////  /////////////////////////////
/////////////////////  //////////////       //////              @@@@@@@@@                ///////////////////////////////////////////////////////////
   ////////////////////    /////////////////////////////////////          ///////////////////////////////////////////////////////////////////////
       /////////////////////     ////////////////////////////////////////////////////////////////////////////////////////////////////////////  
            ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                   //////////////////////////////////////////////////////////////////////////////////////////////////////////////   
                             ///////////////////////////////////////////////////////////////////////////////////////// 
                                             //////////////////////////////////////////////////////////
*/
