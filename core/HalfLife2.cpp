/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2007 AlliedModders LLC.  All rights reserved.
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

#include "HalfLife2.h"
#include "sourcemod.h"
#include "sourcemm_api.h"
#include "UserMessages.h"
#include "PlayerManager.h"

CHalfLife2 g_HL2;
bool g_IsOriginalEngine = false;
ConVar *sv_lan = NULL;

namespace SourceHook
{
	template<>
	int HashFunction<datamap_t *>(datamap_t * const &k)
	{
		return reinterpret_cast<int>(k);
	}

	template<>
	int Compare<datamap_t *>(datamap_t * const &k1, datamap_t * const &k2)
	{
		return (k1-k2);
	}
}

CHalfLife2::CHalfLife2()
{
	m_pClasses = sm_trie_create();
}

CHalfLife2::~CHalfLife2()
{
	sm_trie_destroy(m_pClasses);

	List<DataTableInfo *>::iterator iter;
	DataTableInfo *pInfo;
	for (iter=m_Tables.begin(); iter!=m_Tables.end(); iter++)
	{
		pInfo = (*iter);
		sm_trie_destroy(pInfo->lookup);
		delete pInfo;
	}

	m_Tables.clear();

	THash<datamap_t *, DataMapTrie>::iterator h_iter;
	for (h_iter=m_Maps.begin(); h_iter!=m_Maps.end(); h_iter++)
	{
		if (h_iter->val.trie)
		{
			sm_trie_destroy(h_iter->val.trie);
			h_iter->val.trie = NULL;
		}
	}

	m_Maps.clear();
}

CSharedEdictChangeInfo *g_pSharedChangeInfo = NULL;

void CHalfLife2::OnSourceModStartup(bool late)
{
	/* The Ship currently is the only known game to use an older version of the engine */
	if (strcasecmp(g_SourceMod.GetGameFolderName(), "ship") == 0)
	{
		/* :TODO: Better engine versioning - perhaps something added to SourceMM? */
		g_IsOriginalEngine = true;
	} else if (!g_pSharedChangeInfo) {
		g_pSharedChangeInfo = engine->GetSharedEdictChangeInfo();
	}
}

void CHalfLife2::OnSourceModAllInitialized()
{
	m_MsgTextMsg = g_UserMsgs.GetMessageIndex("TextMsg");
	m_HinTextMsg = g_UserMsgs.GetMessageIndex("HintText");
	m_VGUIMenu = g_UserMsgs.GetMessageIndex("VGUIMenu");
	g_ShareSys.AddInterface(NULL, this);
}

IChangeInfoAccessor *CBaseEdict::GetChangeAccessor()
{
	return engine->GetChangeAccessor( (const edict_t *)this );
}

SendProp *UTIL_FindInSendTable(SendTable *pTable, const char *name)
{
	const char *pname;
	int props = pTable->GetNumProps();
	SendProp *prop;

	for (int i=0; i<props; i++)
	{
		prop = pTable->GetProp(i);
		pname = prop->GetName();
		if (pname && strcmp(name, pname) == 0)
		{
			return prop;
		}
		if (prop->GetDataTable())
		{
			if ((prop=UTIL_FindInSendTable(prop->GetDataTable(), name)) != NULL)
			{
				return prop;
			}
		}
	}

	return NULL;
}

typedescription_t *UTIL_FindInDataMap(datamap_t *pMap, const char *name)
{
	while (pMap)
	{
		for (int i=0; i<pMap->dataNumFields; i++)
		{
			if (pMap->dataDesc[i].fieldName == NULL)
			{
				continue;
			}
			if (strcmp(name, pMap->dataDesc[i].fieldName) == 0)
			{
				return &(pMap->dataDesc[i]);
			}
			if (pMap->dataDesc[i].td)
			{
				typedescription_t *_td;
				if ((_td=UTIL_FindInDataMap(pMap->dataDesc[i].td, name)) != NULL)
				{
					return _td;
				}
			}
		}
		pMap = pMap->baseMap;
	}

	return NULL; 
}

ServerClass *CHalfLife2::FindServerClass(const char *classname)
{
	DataTableInfo *pInfo = _FindServerClass(classname);
	if (!pInfo)
	{
		return NULL;
	}

	return pInfo->sc;
}

DataTableInfo *CHalfLife2::_FindServerClass(const char *classname)
{
	DataTableInfo *pInfo = NULL;

	if (!sm_trie_retrieve(m_pClasses, classname, (void **)&pInfo))
	{
		ServerClass *sc = gamedll->GetAllServerClasses();
		while (sc)
		{
			if (strcmp(classname, sc->GetName()) == 0)
			{
				pInfo = new DataTableInfo;
				pInfo->lookup = sm_trie_create();
				pInfo->sc = sc;
				sm_trie_insert(m_pClasses, classname, pInfo);
				m_Tables.push_back(pInfo);
				break;
			}
			sc = sc->m_pNext;
		}
		if (!pInfo)
		{
			return NULL;
		}
	}

	return pInfo;
}

SendProp *CHalfLife2::FindInSendTable(const char *classname, const char *offset)
{
	DataTableInfo *pInfo = _FindServerClass(classname);

	if (!pInfo)
	{
		return NULL;
	}

	SendProp *pProp = NULL;
	if (!sm_trie_retrieve(pInfo->lookup, offset, (void **)&pProp))
	{
		if ((pProp = UTIL_FindInSendTable(pInfo->sc->m_pTable, offset)) != NULL)
		{
			sm_trie_insert(pInfo->lookup, offset, pProp);
		}
	}

	return pProp;
}

typedescription_t *CHalfLife2::FindInDataMap(datamap_t *pMap, const char *offset)
{
	typedescription_t *td = NULL;
	DataMapTrie &val = m_Maps[pMap];

	if (!val.trie)
	{
		val.trie = sm_trie_create();
	}
	if (!sm_trie_retrieve(val.trie, offset, (void **)&td))
	{
		if ((td = UTIL_FindInDataMap(pMap, offset)) != NULL)
		{
			sm_trie_insert(val.trie, offset, td);
		}
	}

	return td;
}

void CHalfLife2::SetEdictStateChanged(edict_t *pEdict, unsigned short offset)
{
	if (!g_IsOriginalEngine)
	{
		if (offset)
		{
			pEdict->StateChanged(offset);
		} else {
			pEdict->StateChanged();
		}
	} else {
		pEdict->m_fStateFlags |= FL_EDICT_CHANGED;
	}
}

bool CHalfLife2::TextMsg(int client, int dest, const char *msg)
{
	bf_write *pBitBuf = NULL;
	cell_t players[] = {client};

	if ((pBitBuf = g_UserMsgs.StartMessage(m_MsgTextMsg, players, 1, USERMSG_RELIABLE)) == NULL)
	{
		return false;
	}

	pBitBuf->WriteByte(dest);
	pBitBuf->WriteString(msg);
	g_UserMsgs.EndMessage();

	return true;
}

bool CHalfLife2::HintTextMsg(int client, const char *msg)
{
	bf_write *pBitBuf = NULL;
	cell_t players[] = {client};

	if ((pBitBuf = g_UserMsgs.StartMessage(m_HinTextMsg, players, 1, USERMSG_RELIABLE)) == NULL)
	{
		return false;
	}

	pBitBuf->WriteByte(1);
	pBitBuf->WriteString(msg);
	g_UserMsgs.EndMessage();

	return true;
}

bool CHalfLife2::ShowVGUIMenu(int client, const char *name, KeyValues *data, bool show)
{
	bf_write *pBitBuf = NULL;
	KeyValues *SubKey = NULL;
	int count = 0;
	cell_t players[] = {client};

	if ((pBitBuf = g_UserMsgs.StartMessage(m_VGUIMenu, players, 1, USERMSG_RELIABLE)) == NULL)
	{
		return false;
	}

	if (data)
	{
		SubKey = data->GetFirstSubKey();
		while (SubKey)
		{
			count++;
			SubKey = SubKey->GetNextKey();
		}
		SubKey = data->GetFirstSubKey();
	}

	pBitBuf->WriteString(name);
	pBitBuf->WriteByte((show) ? 1 : 0);
	pBitBuf->WriteByte(count);
	while (SubKey)
	{
		pBitBuf->WriteString(SubKey->GetName());
		pBitBuf->WriteString(SubKey->GetString());
		SubKey = SubKey->GetNextKey();
	}
	g_UserMsgs.EndMessage();

	return true;
}

void CHalfLife2::AddToFakeCliCmdQueue(int client, int userid, const char *cmd)
{
	DelayedFakeCliCmd *pFake;

	if (m_FreeCmds.empty())
	{
		pFake = new DelayedFakeCliCmd;
	} else {
		pFake = m_FreeCmds.front();
		m_FreeCmds.pop();
	}

	pFake->client = client;
	pFake->userid = userid;
	pFake->cmd.assign(cmd);

	m_CmdQueue.push(pFake);
}

void CHalfLife2::ProcessFakeCliCmdQueue()
{
	while (!m_CmdQueue.empty())
	{
		DelayedFakeCliCmd *pFake = m_CmdQueue.first();

		if (g_Players.GetClientOfUserId(pFake->userid) == pFake->client)
		{
			CPlayer *pPlayer = g_Players.GetPlayerByIndex(pFake->client);
			serverpluginhelpers->ClientCommand(pPlayer->GetEdict(), pFake->cmd.c_str());
		}

		m_CmdQueue.pop();
	}
}

bool CHalfLife2::IsLANServer()
{
	sv_lan = icvar->FindVar("sv_lan");

	if (!sv_lan)
	{
		return false;
	}

	return (sv_lan->GetInt() != 0);
}
