/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod SDKTools Extension
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

#include "tempents.h"
#include "CellRecipientFilter.h"
#include <IForwardSys.h>

SH_DECL_HOOK5_void(IVEngineServer, PlaybackTempEntity, SH_NOATTRIB, 0, IRecipientFilter &, float, const void *, const SendTable *, int);

CellRecipientFilter g_TERecFilter;
TempEntityInfo *g_CurrentTE = NULL;
int g_TEPlayers[SM_MAXPLAYERS+1];
bool tenatives_initialized = false;

/*************************
*                        *
* Temp Entity Hook Class *
*                        *
**************************/

void TempEntHooks::Initialize()
{
	m_TEHooks = adtfactory->CreateBasicTrie();
	plsys->AddPluginsListener(this);
	tenatives_initialized = true;
}

void TempEntHooks::Shutdown()
{
	if (!tenatives_initialized)
	{
		return;
	}

	plsys->RemovePluginsListener(this);
	SourceHook::List<TEHookInfo *>::iterator iter;
	for (iter=m_HookInfo.begin(); iter!=m_HookInfo.end(); iter++)
	{
		delete (*iter);
	}
	if (m_HookCount)
	{
		m_HookCount = 1;
		_DecRefCounter();
	}
	m_TEHooks->Destroy();
	tenatives_initialized = false;
}

void TempEntHooks::OnPluginUnloaded(IPlugin *plugin)
{
	SourceHook::List<TEHookInfo *>::iterator iter = m_HookInfo.begin();
	IPluginContext *pContext = plugin->GetBaseContext();

	/* For each hook list... */
	while (iter != m_HookInfo.end())
	{
		SourceHook::List<IPluginFunction *>::iterator f_iter = (*iter)->lst.begin();

		/* Find the hooks on the given temp entity */
		while (f_iter != (*iter)->lst.end())
		{
			/* If it matches, remove it and dec the ref count */
			if ((*f_iter)->GetParentContext() == pContext)
			{
				f_iter = (*iter)->lst.erase(f_iter);
				_DecRefCounter();
			}
			else
			{
				f_iter++;
			}
		}

		/* If there are no more hooks left, we can safely 
		 * remove it from the cache and remove its list.
		 */
		if ((*iter)->lst.size() == 0)
		{
			m_TEHooks->Delete((*iter)->te->GetName());
			delete (*iter);
			iter = m_HookInfo.erase(iter);
		}
		else
		{
			iter++;
		}
	}
}

void TempEntHooks::_IncRefCounter()
{
	if (m_HookCount++ == 0)
	{
		SH_ADD_HOOK(IVEngineServer, PlaybackTempEntity, engine, SH_MEMBER(this, &TempEntHooks::OnPlaybackTempEntity), false);
	}
}

void TempEntHooks::_DecRefCounter()
{
	if (--m_HookCount == 0)
	{
		SH_REMOVE_HOOK(IVEngineServer, PlaybackTempEntity, engine, SH_MEMBER(this, &TempEntHooks::OnPlaybackTempEntity), false);
	}
}

size_t TempEntHooks::_FillInPlayers(int *pl_array, IRecipientFilter *pFilter)
{
	size_t size = static_cast<size_t>(pFilter->GetRecipientCount());

	for (size_t i=0; i<size; i++)
	{
		pl_array[i] = pFilter->GetRecipientIndex(i);
	}

	return size;
}

bool TempEntHooks::AddHook(const char *name, IPluginFunction *pFunc)
{
	TEHookInfo *pInfo;

	if (m_TEHooks->Retrieve(name, reinterpret_cast<void **>(&pInfo)))
	{
		pInfo->lst.push_back(pFunc);
	} else {
		TempEntityInfo *te;
		if (!(te=g_TEManager.GetTempEntityInfo(name)))
		{
			return false;
		}

		pInfo = new TEHookInfo;
		pInfo->te = te;
		pInfo->lst.push_back(pFunc);

		m_TEHooks->Insert(name, reinterpret_cast<void *>(pInfo));
		m_HookInfo.push_back(pInfo);
	}

	_IncRefCounter();

	return true;
}

bool TempEntHooks::RemoveHook(const char *name, IPluginFunction *pFunc)
{
	TEHookInfo *pInfo;

	if (m_TEHooks->Retrieve(name, reinterpret_cast<void **>(&pInfo)))
	{
		SourceHook::List<IPluginFunction *>::iterator iter;
		if ((iter=pInfo->lst.find(pFunc)) != pInfo->lst.end())
		{
			pInfo->lst.erase(iter);
			if (pInfo->lst.empty())
			{
				m_HookInfo.remove(pInfo);
				m_TEHooks->Delete(name);
				delete pInfo;
			}
			_DecRefCounter();
		} else {
			return false;
		}
	} else {
		return false;
	}

	return true;
}

void TempEntHooks::OnPlaybackTempEntity(IRecipientFilter &filter, float delay, const void *pSender, const SendTable *pST, int classID)
{
	TEHookInfo *pInfo;
	const char *name = g_TEManager.GetNameFromThisPtr(const_cast<void *>(pSender));

	if (m_TEHooks->Retrieve(name, reinterpret_cast<void **>(&pInfo)))
	{
		SourceHook::List<IPluginFunction *>::iterator iter;
		IPluginFunction *pFunc;
		size_t size;
		cell_t res = static_cast<ResultType>(Pl_Continue);

		TempEntityInfo *oldinfo = g_CurrentTE;
		g_CurrentTE = pInfo->te;
		size = _FillInPlayers(g_TEPlayers, &filter);

		for (iter=pInfo->lst.begin(); iter!=pInfo->lst.end(); iter++)
		{
			pFunc = (*iter);
			pFunc->PushString(name);
			pFunc->PushArray(g_TEPlayers, size);
			pFunc->PushCell(size);
			pFunc->PushFloat(delay);
			pFunc->Execute(&res);

			if (res != Pl_Continue)
			{
				g_CurrentTE = oldinfo;
				RETURN_META(MRES_SUPERCEDE);
			}
		}

		g_CurrentTE = oldinfo;
		RETURN_META(MRES_IGNORED);
	}
}

/**********************
*                     *
* Temp Entity Natives *
*                     *
***********************/

TempEntHooks s_TempEntHooks;

static cell_t smn_TEStart(IPluginContext *pContext, const cell_t *params)
{
	if (!g_TEManager.IsAvailable())
	{
		return pContext->ThrowNativeError("TempEntity System unsupported or not available, file a bug report");
	}

	char *name;
	pContext->LocalToString(params[1], &name);

	g_CurrentTE = g_TEManager.GetTempEntityInfo(name);
	if (!g_CurrentTE)
	{
		return pContext->ThrowNativeError("Invalid TempEntity name: \"%s\"", name);
	}

	return 1;
}

static cell_t smn_TEWriteNum(IPluginContext *pContext, const cell_t *params)
{
	if (!g_TEManager.IsAvailable())
	{
		return pContext->ThrowNativeError("TempEntity System unsupported or not available, file a bug report");
	}
	if (!g_CurrentTE)
	{
		return pContext->ThrowNativeError("No TempEntity call is in progress");
	}

	char *prop;
	pContext->LocalToString(params[1], &prop);

	if (!g_CurrentTE->TE_SetEntData(prop, params[2]))
	{
		return pContext->ThrowNativeError("Temp entity property \"%s\" not found", prop);
	}

	return 1;
}

static cell_t smn_TEReadNum(IPluginContext *pContext, const cell_t *params)
{
	if (!g_TEManager.IsAvailable())
	{
		return pContext->ThrowNativeError("TempEntity System unsupported or not available, file a bug report");
	}
	if (!g_CurrentTE)
	{
		return pContext->ThrowNativeError("No TempEntity call is in progress");
	}

	char *prop;
	int val;
	pContext->LocalToString(params[1], &prop);

	if (!g_CurrentTE->TE_GetEntData(prop, &val))
	{
		return pContext->ThrowNativeError("Temp entity property \"%s\" not found", prop);
	}

	return val;
}

static cell_t smn_TE_WriteFloat(IPluginContext *pContext, const cell_t *params)
{
	if (!g_TEManager.IsAvailable())
	{
		return pContext->ThrowNativeError("TempEntity System unsupported or not available, file a bug report");
	}
	if (!g_CurrentTE)
	{
		return pContext->ThrowNativeError("No TempEntity call is in progress");
	}

	char *prop;
	pContext->LocalToString(params[1], &prop);

	if (!g_CurrentTE->TE_SetEntDataFloat(prop, sp_ctof(params[2])))
	{
		return pContext->ThrowNativeError("Temp entity property \"%s\" not found", prop);
	}

	return 1;
}

static cell_t smn_TE_ReadFloat(IPluginContext *pContext, const cell_t *params)
{
	if (!g_TEManager.IsAvailable())
	{
		return pContext->ThrowNativeError("TempEntity System unsupported or not available, file a bug report");
	}
	if (!g_CurrentTE)
	{
		return pContext->ThrowNativeError("No TempEntity call is in progress");
	}

	char *prop;
	float val;
	pContext->LocalToString(params[1], &prop);

	if (!g_CurrentTE->TE_GetEntDataFloat(prop, &val))
	{
		return pContext->ThrowNativeError("Temp entity property \"%s\" not found", prop);
	}

	return sp_ftoc(val);
}

static cell_t smn_TEWriteVector(IPluginContext *pContext, const cell_t *params)
{
	if (!g_TEManager.IsAvailable())
	{
		return pContext->ThrowNativeError("TempEntity System unsupported or not available, file a bug report");
	}
	if (!g_CurrentTE)
	{
		return pContext->ThrowNativeError("No TempEntity call is in progress");
	}

	char *prop;
	pContext->LocalToString(params[1], &prop);

	cell_t *addr;
	pContext->LocalToPhysAddr(params[2], &addr);
	float vec[3] = {sp_ctof(addr[0]), sp_ctof(addr[1]), sp_ctof(addr[2])};

	if (!g_CurrentTE->TE_SetEntDataVector(prop, vec))
	{
		return pContext->ThrowNativeError("Temp entity property \"%s\" not found", prop);
	}

	return 1;
}

static cell_t smn_TEReadVector(IPluginContext *pContext, const cell_t *params)
{
	if (!g_TEManager.IsAvailable())
	{
		return pContext->ThrowNativeError("TempEntity System unsupported or not available, file a bug report");
	}
	if (!g_CurrentTE)
	{
		return pContext->ThrowNativeError("No TempEntity call is in progress");
	}

	char *prop;
	pContext->LocalToString(params[1], &prop);

	cell_t *addr;
	float vec[3];
	pContext->LocalToPhysAddr(params[2], &addr);

	if (!g_CurrentTE->TE_GetEntDataVector(prop, vec))
	{
		return pContext->ThrowNativeError("Temp entity property \"%s\" not found", prop);
	}

	addr[0] = sp_ftoc(vec[0]);
	addr[1] = sp_ftoc(vec[1]);
	addr[2] = sp_ftoc(vec[2]);

	return 1;
}

static cell_t smn_TEWriteFloatArray(IPluginContext *pContext, const cell_t *params)
{
	if (!g_TEManager.IsAvailable())
	{
		return pContext->ThrowNativeError("TempEntity System unsupported or not available, file a bug report");
	}
	if (!g_CurrentTE)
	{
		return pContext->ThrowNativeError("No TempEntity call is in progress");
	}

	char *prop;
	pContext->LocalToString(params[1], &prop);

	cell_t *addr;
	pContext->LocalToPhysAddr(params[2], &addr);

	if (!g_CurrentTE->TE_SetEntDataFloatArray(prop, addr, params[3]))
	{
		return pContext->ThrowNativeError("Temp entity property \"%s\" not found", prop);
	}

	return 1;
}

static cell_t smn_TESend(IPluginContext *pContext, const cell_t *params)
{
	if (!g_TEManager.IsAvailable())
	{
		return pContext->ThrowNativeError("TempEntity System unsupported or not available, file a bug report");
	}
	if (!g_CurrentTE)
	{
		return pContext->ThrowNativeError("No TempEntity call is in progress");
	}

	cell_t *cl_array;
	unsigned int numClients;
	int client;
	IGamePlayer *pPlayer = NULL;

	pContext->LocalToPhysAddr(params[1], &cl_array);
	numClients = params[2];

	/* Client validation */
	for (unsigned int i = 0; i < numClients; i++)
	{
		client = cl_array[i];
		pPlayer = playerhelpers->GetGamePlayer(client);

		if (!pPlayer)
		{
			return pContext->ThrowNativeError("Client index %d is invalid", client);
		} else if (!pPlayer->IsInGame()) {
			return pContext->ThrowNativeError("Client %d is not in game", client);
		}
	}

	g_TERecFilter.Reset();
	g_TERecFilter.Initialize(cl_array, numClients);

	g_CurrentTE->Send(g_TERecFilter, sp_ctof(params[3]));
	g_CurrentTE = NULL;

	return 1;
}

static cell_t smn_TEIsValidProp(IPluginContext *pContext, const cell_t *params)
{
	if (!g_TEManager.IsAvailable())
	{
		return pContext->ThrowNativeError("TempEntity System unsupported or not available, file a bug report");
	}
	if (!g_CurrentTE)
	{
		return pContext->ThrowNativeError("No TempEntity call is in progress");
	}

	char *prop;
	pContext->LocalToString(params[1], &prop);

	return g_CurrentTE->IsValidProp(prop) ? 1 : 0;
}

static cell_t smn_AddTempEntHook(IPluginContext *pContext, const cell_t *params)
{
	char *name;
	IPluginFunction *pFunc;

	if (!g_TEManager.IsAvailable())
	{
		return pContext->ThrowNativeError("TempEntity System unsupported or not available, file a bug report");
	}

	pContext->LocalToString(params[1], &name);
	pFunc = pContext->GetFunctionById(params[2]);
	if (!pFunc)
	{
		return pContext->ThrowNativeError("Invalid function id (%X)", params[2]);
	}

	if (!s_TempEntHooks.AddHook(name, pFunc))
	{
		return pContext->ThrowNativeError("Invalid TempEntity name: \"%s\"", name);
	}

	return 1;
}

static cell_t smn_RemoveTempEntHook(IPluginContext *pContext, const cell_t *params)
{
	char *name;
	IPluginFunction *pFunc;

	if (!g_TEManager.IsAvailable())
	{
		return pContext->ThrowNativeError("TempEntity System unsupported or not available, file a bug report");
	}

	pContext->LocalToString(params[1], &name);
	pFunc = pContext->GetFunctionById(params[2]);
	if (!pFunc)
	{
		return pContext->ThrowNativeError("Invalid function id (%X)", params[2]);
	}

	if (!s_TempEntHooks.RemoveHook(name, pFunc))
	{
		return pContext->ThrowNativeError("Invalid hooked TempEntity name or function");
	}

	return 1;
}

sp_nativeinfo_t g_TENatives[] = 
{
	{"TE_Start",				smn_TEStart},
	{"TE_WriteNum",				smn_TEWriteNum},
	{"TE_ReadNum",				smn_TEReadNum},
	{"TE_WriteFloat",			smn_TE_WriteFloat},
	{"TE_ReadFloat",			smn_TE_ReadFloat},
	{"TE_WriteVector",			smn_TEWriteVector},
	{"TE_ReadVector",			smn_TEReadVector},
	{"TE_WriteAngles",			smn_TEWriteVector},
	{"TE_Send",					smn_TESend},
	{"TE_IsValidProp",			smn_TEIsValidProp},
	{"TE_WriteFloatArray",		smn_TEWriteFloatArray},
	{"AddTempEntHook",			smn_AddTempEntHook},
	{"RemoveTempEntHook",		smn_RemoveTempEntHook},
	{NULL,						NULL}
};
