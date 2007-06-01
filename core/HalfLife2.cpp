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

#include "HalfLife2.h"
#include "sourcemod.h"
#include "sourcemm_api.h"
#include "UserMessages.h"

CHalfLife2 g_HL2;
bool g_IsOriginalEngine = false;

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

void CHalfLife2::TextMsg(int client, int dest, const char *msg)
{
	bf_write *pBitBuf = NULL;
	cell_t players[] = {client};

	pBitBuf = g_UserMsgs.StartMessage(m_MsgTextMsg, players, 1, USERMSG_RELIABLE);
	pBitBuf->WriteByte(dest);
	pBitBuf->WriteString(msg);
	g_UserMsgs.EndMessage();
}
