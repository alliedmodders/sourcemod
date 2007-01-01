#include "CPlayerManager.h"

//:TODO: register these and use the same capitalization conventions as the natives

static cell_t sm_getclientcount(IPluginContext *pCtx, const cell_t *params)
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

static cell_t sm_getmaxclients(IPluginContext *pCtx, const cell_t *params)
{
	return g_PlayerManager.GetMaxClients();
}

static cell_t sm_getclientname(IPluginContext *pCtx, const cell_t *params)
{
	int index = params[1];
	if ((index < 1) || (index > g_PlayerManager.GetMaxClients()))
	{
		//:TODO: runtime error
	}

	pCtx->StringToLocalUTF8(params[2], static_cast<size_t>(params[3]), g_PlayerManager.GetPlayerByIndex(index)->PlayerName(), NULL);
	return 1;
}

static cell_t sm_getclientip(IPluginContext *pCtx, const cell_t *params)
{
	int index = params[1];
	if ((index < 1) || (index > g_PlayerManager.GetMaxClients()))
	{
		//:TODO: runtime error
	}

	pCtx->StringToLocal(params[2], static_cast<size_t>(params[3]), g_PlayerManager.GetPlayerByIndex(index)->PlayerIP());
	return 1;
}

static cell_t sm_getclientauthstr(IPluginContext *pCtx, const cell_t *params)
{
	int index = params[1];
	if ((index < 1) || (index > g_PlayerManager.GetMaxClients()))
	{
		//:TODO: runtime error
	}

	pCtx->StringToLocal(params[2], static_cast<size_t>(params[3]), g_PlayerManager.GetPlayerByIndex(index)->PlayerAuthString());
	return 1;
}

static cell_t sm_isplayerconnected(IPluginContext *pCtx, const cell_t *params)
{
	int index = params[1];
	if ((index < 1) || (index > g_PlayerManager.GetMaxClients()))
	{
		//:TODO: runtime error
	}

	return (g_PlayerManager.GetPlayerByIndex(index)->IsPlayerConnected()) ? 1 : 0;
}

static cell_t sm_isplayeringame(IPluginContext *pCtx, const cell_t *params)
{
	int index = params[1];
	if ((index < 1) || (index > g_PlayerManager.GetMaxClients()))
	{
		//:TODO: runtime error
	}

	return (g_PlayerManager.GetPlayerByIndex(index)->IsPlayerInGame()) ? 1 : 0;
}

static cell_t sm_isplayerauthorized(IPluginContext *pCtx, const cell_t *params)
{
	int index = params[1];
	if ((index < 1) || (index > g_PlayerManager.GetMaxClients()))
	{
		//:TODO: runtime error
	}

	return (g_PlayerManager.GetPlayerByIndex(index)->IsPlayerAuthorized()) ? 1 : 0;
}

static cell_t sm_isplayerfakeclient(IPluginContext *pCtx, const cell_t *params)
{
	int index = params[1];
	if ((index < 1) || (index > g_PlayerManager.GetMaxClients()))
	{
		//:TODO: runtime error
	}

	return (g_PlayerManager.GetPlayerByIndex(index)->IsPlayerFakeClient()) ? 1 : 0;
}