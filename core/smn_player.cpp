#include "CPlayerManager.h"
#include "sm_stringutil.h"

static cell_t sm_GetClientCount(IPluginContext *pCtx, const cell_t *params)
{
	if (params[1])
	{
		return g_PlayerManager.GetPlayerCount();
	}

	int maxplayers = g_PlayerManager.GetMaxClients();
	int count = 0;
	for (int i=1; i<=maxplayers; ++i)
	{
		CPlayer *pPlayer = g_PlayerManager.GetPlayerByIndex(i);
		if ((pPlayer->IsPlayerConnected()) && !(pPlayer->IsPlayerInGame()))
		{
			count++;
		}
	}

	return (g_PlayerManager.GetPlayerCount() + count);
}

static cell_t sm_GetMaxClients(IPluginContext *pCtx, const cell_t *params)
{
	return g_PlayerManager.GetMaxClients();
}

static cell_t sm_GetClientName(IPluginContext *pCtx, const cell_t *params)
{
	int index = params[1];
	if ((index < 1) || (index > g_PlayerManager.GetMaxClients()))
	{
		return pCtx->ThrowNativeError("Invalid client index %d.", index);
	}

	CPlayer *pPlayer = g_PlayerManager.GetPlayerByIndex(index);
	if (!pPlayer->IsPlayerConnected())
	{
		return pCtx->ThrowNativeError("Client %d is not connected.", index);
	}

	pCtx->StringToLocalUTF8(params[2], static_cast<size_t>(params[3]), pPlayer->PlayerName(), NULL);
	return 1;
}

static cell_t sm_GetClientIP(IPluginContext *pCtx, const cell_t *params)
{
	int index = params[1];
	if ((index < 1) || (index > g_PlayerManager.GetMaxClients()))
	{
		return pCtx->ThrowNativeError("Invalid client index %d.", index);
	}

	CPlayer *pPlayer = g_PlayerManager.GetPlayerByIndex(index);
	if (!pPlayer->IsPlayerConnected())
	{
		return pCtx->ThrowNativeError("Client %d is not connected.", index);
	}

	char buf[64], *ptr;
	strcpy(buf, pPlayer->PlayerIP());

	if (params[4] && (ptr = strchr(buf, ':')))
	{
		*ptr = '\0';
	}

	pCtx->StringToLocal(params[2], static_cast<size_t>(params[3]), buf);
	return 1;
}

static cell_t sm_GetClientAuthStr(IPluginContext *pCtx, const cell_t *params)
{
	int index = params[1];
	if ((index < 1) || (index > g_PlayerManager.GetMaxClients()))
	{
		return pCtx->ThrowNativeError("Invalid client index %d.", index);
	}

	CPlayer *pPlayer = g_PlayerManager.GetPlayerByIndex(index);
	if (!pPlayer->IsPlayerConnected())
	{
		return pCtx->ThrowNativeError("Client %d is not connected.", index);
	}

	pCtx->StringToLocal(params[2], static_cast<size_t>(params[3]), pPlayer->PlayerAuthString());
	return 1;
}

static cell_t sm_IsPlayerConnected(IPluginContext *pCtx, const cell_t *params)
{
	int index = params[1];
	if ((index < 1) || (index > g_PlayerManager.GetMaxClients()))
	{
		return pCtx->ThrowNativeError("Invalid client index %d.", index);
	}

	return (g_PlayerManager.GetPlayerByIndex(index)->IsPlayerConnected()) ? 1 : 0;
}

static cell_t sm_IsPlayerIngame(IPluginContext *pCtx, const cell_t *params)
{
	int index = params[1];
	if ((index < 1) || (index > g_PlayerManager.GetMaxClients()))
	{
		return pCtx->ThrowNativeError("Invalid client index %d.", index);
	}

	return (g_PlayerManager.GetPlayerByIndex(index)->IsPlayerInGame()) ? 1 : 0;
}

static cell_t sm_IsPlayerAuthorized(IPluginContext *pCtx, const cell_t *params)
{
	int index = params[1];
	if ((index < 1) || (index > g_PlayerManager.GetMaxClients()))
	{
		return pCtx->ThrowNativeError("Invalid client index %d.", index);
	}

	return (g_PlayerManager.GetPlayerByIndex(index)->IsPlayerAuthorized()) ? 1 : 0;
}

static cell_t sm_IsPlayerFakeClient(IPluginContext *pCtx, const cell_t *params)
{
	int index = params[1];
	if ((index < 1) || (index > g_PlayerManager.GetMaxClients()))
	{
		return pCtx->ThrowNativeError("Invalid client index %d.", index);
	}

	CPlayer *pPlayer = g_PlayerManager.GetPlayerByIndex(index);
	if (!pPlayer->IsPlayerConnected())
	{
		return pCtx->ThrowNativeError("Client %d is not connected.", index);
	}

	return (pPlayer->IsPlayerFakeClient()) ? 1 : 0;
}

static cell_t sm_PrintToServer(IPluginContext *pCtx, const cell_t *params)
{
	char buffer[1024];
	char *fmt;
	int arg = 2;

	pCtx->LocalToString(params[1], &fmt);
	size_t res = atcprintf(buffer, sizeof(buffer)-2, fmt, pCtx, params, &arg);

	buffer[res++] = '\n';
	buffer[res] = '\0';

	META_CONPRINT(buffer);

	return 1;
}

static cell_t sm_PrintToConsole(IPluginContext *pCtx, const cell_t *params)
{
	int index = params[1];
	if ((index < 1) || (index > g_PlayerManager.GetMaxClients()))
	{
		return pCtx->ThrowNativeError("Invalid client index %d.", index);
	}

	CPlayer *pPlayer = g_PlayerManager.GetPlayerByIndex(index);
	if (!pPlayer->IsPlayerInGame())
	{
		return pCtx->ThrowNativeError("Client %d is not in game.", index);
	}

	char buffer[1024];
	char *fmt;
	int arg = 3;

	pCtx->LocalToString(params[2], &fmt);
	size_t res = atcprintf(buffer, sizeof(buffer)-2, fmt, pCtx, params, &arg);

	buffer[res++] = '\n';
	buffer[res] = '\0';

	engine->ClientPrintf(pPlayer->GetPlayerEdict(), buffer);

	return 1;
}

REGISTER_NATIVES(playernatives)
{
	{"GetMaxClients",			sm_GetMaxClients},
	{"GetClientCount",			sm_GetClientCount},
	{"GetClientName",			sm_GetClientName},
	{"GetClientIP",				sm_GetClientIP},
	{"GetClientAuthString",		sm_GetClientAuthStr},
	{"IsPlayerConnected",		sm_IsPlayerConnected},
	{"IsPlayerInGame",			sm_IsPlayerIngame},
	{"IsPlayerAuthorized",		sm_IsPlayerAuthorized},
	{"IsPlayerFakeClient",		sm_IsPlayerFakeClient},
	{"PrintToServer",			sm_PrintToServer},
	{"PrintToConsole",			sm_PrintToConsole},
	{NULL,						NULL}
};