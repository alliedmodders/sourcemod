#include "CPlayerManager.h"
#include "ForwardSys.h"

SH_DECL_HOOK5(IServerGameClients, ClientConnect, SH_NOATTRIB, 0, bool, edict_t *, const char *, const char *, char *, int);
SH_DECL_HOOK2_void(IServerGameClients, ClientPutInServer, SH_NOATTRIB, 0, edict_t *, const char *);
SH_DECL_HOOK1_void(IServerGameClients, ClientDisconnect, SH_NOATTRIB, 0, edict_t *);
SH_DECL_HOOK1_void(IServerGameClients, ClientCommand, SH_NOATTRIB, 0, edict_t *);

void CPlayerManager::OnSourceModAllInitialized()
{
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientConnect, serverClients, this, &CPlayerManager::OnClientConnect, false);
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientPutInServer, serverClients, this, &CPlayerManager::OnClientPutInServer, true);
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientDisconnect, serverClients, this, &CPlayerManager::OnClientDisconnect, false);
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientDisconnect, serverClients, this, &CPlayerManager::OnClientDisconnect_Post, true);
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientCommand, serverClients, this, &CPlayerManager::OnClientCommand, false);

	/* Register OnClientConnect */
	ParamType p1[] = {Param_Cell, Param_String, Param_Cell};
	m_clconnect = g_Forwards.CreateForward("OnClientConnect", ET_Event, 3, p1);

	/* Register OnClientPutInServer */
	ParamType p2[] = {Param_Cell};
	m_clputinserver = g_Forwards.CreateForward("OnClientPutInServer", ET_Ignore, 1, p2);

	/* Register OnClientDisconnect */
	m_cldisconnect = g_Forwards.CreateForward("OnClientDisconnect", ET_Ignore, 1, p2);

	/* Register OnClientDisconnect_Post */
	m_cldisconnect_post = g_Forwards.CreateForward("OnClientDisconnect_Post", ET_Ignore, 1, p2);

	/* Register OnClientCommand */
	m_clcommand = g_Forwards.CreateForward("OnClientCommand", ET_Hook, 1, p2);

	/* Register OnClientAuthorized */
	//:TODO:
}

void CPlayerManager::OnSourceModShutdown()
{
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientConnect, serverClients, this, &CPlayerManager::OnClientConnect, false);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientPutInServer, serverClients, this, &CPlayerManager::OnClientPutInServer, true);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientDisconnect, serverClients, this, &CPlayerManager::OnClientDisconnect, false);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientDisconnect, serverClients, this, &CPlayerManager::OnClientDisconnect_Post, true);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientCommand, serverClients, this, &CPlayerManager::OnClientCommand, false);

	/* Release forwards */
	g_Forwards.ReleaseForward(m_clconnect);
	g_Forwards.ReleaseForward(m_clputinserver);
	g_Forwards.ReleaseForward(m_cldisconnect);
	g_Forwards.ReleaseForward(m_cldisconnect_post);
	g_Forwards.ReleaseForward(m_clcommand);
}

bool CPlayerManager::OnClientConnect(edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen)
{
	cell_t res = 1;

	m_clconnect->PushCell(engine->IndexOfEdict(pEntity));
	m_clconnect->PushStringEx(reject, maxrejectlen, SM_PARAM_STRING_UTF8, SM_PARAM_COPYBACK);
	m_clconnect->PushCell(maxrejectlen);
	m_clconnect->Execute(&res, NULL);

	return (res) ? true : false;
}

void CPlayerManager::OnClientPutInServer(edict_t *pEntity, const char *playername)
{
	cell_t res;

	m_clputinserver->PushCell(engine->IndexOfEdict(pEntity));
	m_clputinserver->Execute(&res, NULL);
}

void CPlayerManager::OnClientAuthorized()
{
	//:TODO:
}

void CPlayerManager::OnClientDisconnect(edict_t *pEntity)
{
	cell_t res;

	m_cldisconnect->PushCell(engine->IndexOfEdict(pEntity));
	m_cldisconnect->Execute(&res, NULL);
}

void CPlayerManager::OnClientDisconnect_Post(edict_t *pEntity)
{
	cell_t res;

	m_cldisconnect_post->PushCell(engine->IndexOfEdict(pEntity));
	m_cldisconnect_post->Execute(&res, NULL);
}

void CPlayerManager::OnClientCommand(edict_t *pEntity)
{
	cell_t res;

	m_clcommand->PushCell(engine->IndexOfEdict(pEntity));
	m_clcommand->Execute(&res, NULL);
}