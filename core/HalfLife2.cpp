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
#include "sourcemm_api.h"

CHalfLife2 g_HL2;

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
}

#if 0
void CHalfLife2::OnSourceModStartup(bool late)
{
}

void CHalfLife2::OnSourceModAllShutdown()
{
}
#endif

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

	SendProp *pProp;
	if (!sm_trie_retrieve(pInfo->lookup, offset, (void **)&pProp))
	{
		if ((pProp = UTIL_FindInSendTable(pInfo->sc->m_pTable, offset)) != NULL)
		{
			sm_trie_insert(pInfo->lookup, offset, pProp);
		}
	}

	return pProp;
}
