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

#include "HandleSys.h"
#include "CellRecipientFilter.h"
#include "CMsgIdPool.h"

#define USERMSG_PASSTHRU			(1<<0)
#define USERMSG_PASSTHRU_ALL		(1<<1)
#define USERMSG_RELIABLE			(1<<2)
#define USERMSG_INITMSG				(1<<3)

CMessageIdPool g_MessageIds;
CellRecipientFilter g_MsgRecFilter;
HandleType_t g_WrBitBufType;
Handle_t g_CurMsgHandle;
bool g_IsMsgInExec = false;
int g_CurMsgFlags = 0;

class UsrMessageNatives :
	public SMGlobalClass,
	public IHandleTypeDispatch
{
public:
	void OnSourceModAllInitialized()
	{
		g_WrBitBufType = g_HandleSys.CreateType("BitBufWriter", this, 0, NULL, NULL, g_pCoreIdent, NULL);
	}
	void OnSourceModShutdown()
	{
		g_HandleSys.RemoveType(g_WrBitBufType, g_pCoreIdent);
		g_WrBitBufType = 0;
	}
	void OnHandleDestroy(HandleType_t type, void *object)
	{
	}
};

static cell_t smn_GetUserMessageId(IPluginContext *pCtx, const cell_t *params)
{
	char *msgname;
	int err;
	if ((err=pCtx->LocalToString(params[1], &msgname)) != SP_ERROR_NONE)
	{
		pCtx->ThrowNativeErrorEx(err, NULL);
		return 0;
	}

	return g_MessageIds.GetMessageId(msgname);
}

static cell_t smn_StartMessage(IPluginContext *pCtx, const cell_t *params)
{
	char *msgname;
	cell_t *cl_array;
	int msgid, err;
	bf_write *pBitBuf;

	if (g_IsMsgInExec)
	{
		return pCtx->ThrowNativeError("Unable to execute a new message, there is already one in progress");
	}

	if ((err=pCtx->LocalToString(params[1], &msgname)) != SP_ERROR_NONE)
	{
		pCtx->ThrowNativeErrorEx(err, NULL);
		return 0;
	}

	if ((msgid=g_MessageIds.GetMessageId(msgname)) == INVALID_MESSAGE_ID)
	{
		return pCtx->ThrowNativeError("Invalid message name: \"%s\"", msgname);
	}

	pCtx->LocalToPhysAddr(params[2], &cl_array);
	g_CurMsgFlags = params[4];

	g_MsgRecFilter.SetRecipientPtr(cl_array, params[3]);
	if (g_CurMsgFlags & USERMSG_INITMSG)
	{
		g_MsgRecFilter.SetInitMessage(true);
	}
	if (g_CurMsgFlags & USERMSG_RELIABLE)
	{
		g_MsgRecFilter.SetReliable(true);
	}

	if (g_CurMsgFlags & (USERMSG_PASSTHRU_ALL|USERMSG_PASSTHRU)) //:TODO: change this when we can hook messages
	{
		pBitBuf = engine->UserMessageBegin(static_cast<IRecipientFilter *>(&g_MsgRecFilter), msgid);
	} else {
		pBitBuf = ENGINE_CALL(UserMessageBegin)(static_cast<IRecipientFilter *>(&g_MsgRecFilter), msgid);
	}

	g_CurMsgHandle = g_HandleSys.CreateHandle(g_WrBitBufType, pBitBuf, pCtx->GetIdentity(), g_pCoreIdent, NULL);
	g_IsMsgInExec = true;

	return g_CurMsgHandle;
}

static cell_t smn_StartMessageEx(IPluginContext *pCtx, const cell_t *params)
{
	cell_t *cl_array;
	bf_write *pBitBuf;
	int msgid = params[1];

	if (g_IsMsgInExec)
	{
		return pCtx->ThrowNativeError("Unable to execute a new message, there is already one in progress");
	}

	if (msgid < 0 || msgid > 255)
	{
		return pCtx->ThrowNativeError("Invalid message id supplied (%d)", msgid);
	}

	pCtx->LocalToPhysAddr(params[2], &cl_array);
	g_CurMsgFlags = params[4];

	g_MsgRecFilter.SetRecipientPtr(cl_array, params[3]);
	if (g_CurMsgFlags & USERMSG_INITMSG)
	{
		g_MsgRecFilter.SetInitMessage(true);
	}
	if (g_CurMsgFlags & USERMSG_RELIABLE)
	{
		g_MsgRecFilter.SetReliable(true);
	}

	if (g_CurMsgFlags & (USERMSG_PASSTHRU_ALL|USERMSG_PASSTHRU)) //:TODO: change this when we can hook messages
	{
		pBitBuf = engine->UserMessageBegin(static_cast<IRecipientFilter *>(&g_MsgRecFilter), msgid);
	} else {
		pBitBuf = ENGINE_CALL(UserMessageBegin)(static_cast<IRecipientFilter *>(&g_MsgRecFilter), msgid);
	}

	g_CurMsgHandle = g_HandleSys.CreateHandle(g_WrBitBufType, pBitBuf, pCtx->GetIdentity(), g_pCoreIdent, NULL);
	g_IsMsgInExec = true;

	return g_CurMsgHandle;
}

static cell_t smn_EndMessage(IPluginContext *pCtx, const cell_t *params)
{
	HandleSecurity sec;

	if (!g_IsMsgInExec)
	{
		return pCtx->ThrowNativeError("Unable to end message, no message is in progress");
	}

	if (g_CurMsgFlags & (USERMSG_PASSTHRU_ALL|USERMSG_PASSTHRU))
	{
		engine->MessageEnd();
	} else {
		ENGINE_CALL(MessageEnd)();
	}

	sec.pOwner = pCtx->GetIdentity();
	sec.pIdentity = g_pCoreIdent;
	g_HandleSys.FreeHandle(g_CurMsgHandle, &sec);

	g_IsMsgInExec = false;
	g_CurMsgFlags = 0;
	g_MsgRecFilter.ResetFilter();

	return 1;
}

static UsrMessageNatives s_UsrMessageNatives;

REGISTER_NATIVES(usrmsgnatives)
{
	{"GetUserMessageId",			smn_GetUserMessageId},
	{"StartMessage",				smn_StartMessage},
	{"StartMessageEx",				smn_StartMessageEx},
	{"EndMessage",					smn_EndMessage},
	{NULL,							NULL}
};
