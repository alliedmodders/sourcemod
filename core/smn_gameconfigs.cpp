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
#include "GameConfigs.h"

HandleType_t g_GameConfigsType;

class GameConfigsNatives :
	public IHandleTypeDispatch, 
	public SourceModBase
{
public:
	void OnSourceModAllInitialized()
	{
		g_GameConfigsType = g_HandleSys.CreateType("GameConfigs", this, 0, NULL, NULL, g_pCoreIdent, NULL);
	}
	void OnSourceModShutdown()
	{
		g_HandleSys.RemoveType(g_GameConfigsType, g_pCoreIdent);
		g_GameConfigsType = 0;
	}
	void OnHandleDestroy(HandleType_t type, void *object)
	{
		g_GameConfigs.CloseGameConfigFile(reinterpret_cast<IGameConfig *>(object));
	}
};

static cell_t smn_LoadGameConfigFile(IPluginContext *pCtx, const cell_t *params)
{
	IGameConfig *gc;
	char *filename;
	char error[128];

	pCtx->LocalToString(params[1], &filename);
	if (!g_GameConfigs.LoadGameConfigFile(filename, &gc, error, sizeof(error)))
	{
		return pCtx->ThrowNativeError("Unable to open %s: %s", filename, error);
	}

	return g_HandleSys.CreateHandle(g_GameConfigsType, gc, pCtx->GetIdentity(), g_pCoreIdent, NULL);
}

static cell_t smn_GameConfGetOffset(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	IGameConfig *gc;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=g_HandleSys.ReadHandle(hndl, g_GameConfigsType, &sec, (void **)&gc))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid game config handle %x (error %d)", hndl, herr);
	}

	char *key;
	int val;
	pCtx->LocalToString(params[2], &key);

	if (!gc->GetOffset(key, &val))
	{
		return -1;
	}

	return val;
}

static cell_t smn_GameConfGetKeyValue(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	IGameConfig *gc;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=g_HandleSys.ReadHandle(hndl, g_GameConfigsType, &sec, (void **)&gc))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid game config handle %x (error %d)", hndl, herr);
	}

	char *key;
	const char *val;
	pCtx->LocalToString(params[2], &key);

	if ((val=gc->GetKeyValue(key)) == NULL)
	{
		return 0;
	}

	pCtx->StringToLocalUTF8(params[3], params[4], val, NULL);

	return 1;
}

static GameConfigsNatives s_GameConfigsNatives;

REGISTER_NATIVES(gameconfignatives)
{
	{"LoadGameConfigFile",			smn_LoadGameConfigFile},
	{"GameConfGetOffset",			smn_GameConfGetOffset},
	{"GameConfGetKeyValue",			smn_GameConfGetKeyValue},
	{NULL,							NULL}
};