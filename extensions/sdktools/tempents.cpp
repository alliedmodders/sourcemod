/**
 * vim: set ts=4 :
 * ===============================================================
 * SourceMod SDK Tools Extension
 * Copyright (C) 2004-2007 AlliedModders LLC. All rights reserved.
 * ===============================================================
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Version: $Id$
 */

#include "tempents.h"

TempEntityManager g_TEManager;
ICallWrapper *g_GetServerClass = NULL;

/*************************
*                        *
* Temp Entities Wrappers *
*                        *
**************************/

TempEntityInfo::TempEntityInfo(const char *name, void *me)
{
	m_Name.assign(name);
	m_Me = me;
	g_GetServerClass->Execute(&m_Me, &m_Sc);
}

const char *TempEntityInfo::GetName()
{
	return m_Name.c_str();
}

bool TempEntityInfo::IsValidProp(const char *name)
{
	return (g_pGameHelpers->FindInSendTable(m_Sc->GetName(), name)) ? true : false;
}

int TempEntityInfo::_FindOffset(const char *name, int *size)
{
	int offset;

	SendProp *prop = g_pGameHelpers->FindInSendTable(m_Sc->GetName(), name);
	if (!prop)
	{
		return -1;
	}

	offset = prop->GetOffset();
	if (size)
	{
		*size = prop->m_nBits;
	}

	return offset;
}

bool TempEntityInfo::TE_SetEntData(const char *name, int value)
{
	/* Search for our offset */
	int size;
	int offset = _FindOffset(name, &size);

	if (offset < 0)
	{
		return false;
	}

	if (size <= 8)
	{
		*((uint8_t *)m_Me + offset) = value;
	} else if (size <= 16) {
		*(short *)((uint8_t *)m_Me + offset) = value;
	} else if (size <= 32){
		*(int *)((uint8_t *)m_Me + offset) = value;
	} else {
		return false;
	}

	return true;
}

bool TempEntityInfo::TE_SetEntDataFloat(const char *name, float value)
{
	/* Search for our offset */
	int offset = _FindOffset(name);

	if (offset < 0)
	{
		return false;
	}

	*(float *)((uint8_t *)m_Me + offset) = value;

	return true;
}

bool TempEntityInfo::TE_SetEntDataVector(const char *name, float vector[3])
{
	/* Search for our offset */
	int offset = _FindOffset(name);

	if (offset < 0)
	{
		return false;
	}

	Vector *v = (Vector *)((uint8_t *)m_Me + offset);
	v->x = vector[0];
	v->y = vector[1];
	v->z = vector[2];

	return true;
}

bool TempEntityInfo::TE_SetEntDataFloatArray(const char *name, cell_t *array, int size)
{
	/* Search for our offset */
	int offset = _FindOffset(name);

	if (offset < 0)
	{
		return false;
	}

	float *base = (float *)((uint8_t *)m_Me + offset);
	for (int i=0; i<size; i++)
	{
		base[i] = sp_ctof(array[i]);
	}

	return true;
}

void TempEntityInfo::Send(IRecipientFilter &filter, float delay)
{
	engine->PlaybackTempEntity(filter, delay, m_Me, m_Sc->m_pTable, m_Sc->m_ClassID);
}

/**********************
*                     *
* Temp Entity Manager *
*                     *
***********************/

void TempEntityManager::Initialize()
{
	void *addr;
	int offset;
	m_Loaded = false;

	/* Read our sigs and offsets from the config file */
	if (!g_pGameConf->GetMemSig("CBaseTempEntity", &addr) || !addr)
	{
		return;
	}
	if (!g_pGameConf->GetOffset("s_pTempEntities", &offset) || !offset)
	{
		return;
	}
	if (!g_pGameConf->GetOffset("GetTEName", &m_NameOffs) || !m_NameOffs)
	{
		return;
	}
	if (!g_pGameConf->GetOffset("GetTENext", &m_NextOffs) || !m_NextOffs)
	{
		return;
	}
	if (!g_pGameConf->GetOffset("TE_GetServerClass", &m_GetClassNameOffs) || !m_GetClassNameOffs)
	{
		return;
	}

	/* Store the head of the TE linked list */
	m_ListHead = **(void ***)((unsigned char *)addr + offset);

	/* Create our trie */
	m_TempEntInfo = adtfactory->CreateBasicTrie();

	/* Create the GetServerClass call */
	PassInfo retinfo;
	retinfo.flags = PASSFLAG_BYVAL;
	retinfo.type = PassType_Basic;
	retinfo.size = sizeof(ServerClass *);
	g_GetServerClass = g_pBinTools->CreateVCall(m_GetClassNameOffs, 0, 0, &retinfo, NULL, 0);

	/* We're done */
	m_Loaded = true;
}

bool TempEntityManager::IsAvailable()
{
	return m_Loaded;
}

void TempEntityManager::Shutdown()
{
	if (!IsAvailable())
	{
		return;
	}

	List<TempEntityInfo *>::iterator iter;
	for (iter=m_TEList.begin(); iter!=m_TEList.end(); iter++)
	{
		delete (*iter);
	}
	m_TEList.clear();

	m_TempEntInfo->Destroy();
	g_GetServerClass->Destroy();
	g_GetServerClass = NULL;
	m_ListHead = NULL;
	m_NextOffs = m_NameOffs = m_GetClassNameOffs = 0;
	m_Loaded = false;
}

TempEntityInfo *TempEntityManager::GetTempEntityInfo(const char *name)
{
	/* If the system is unloaded skip the search */
	if (!IsAvailable())
	{
		return NULL;
	}

	TempEntityInfo *te = NULL;

	/* Start searching for the TE inside the engine */
	if (!m_TempEntInfo->Retrieve(name, reinterpret_cast<void **>(&te)))
	{
		void *iter = m_ListHead;
		while (iter)
		{
			const char *realname = *(const char **)((unsigned char *)iter + m_NameOffs);
			if (!realname)
			{
				continue;
			}
			if (strcmp(name, realname) == 0)
			{
				te = new TempEntityInfo(name, iter);
				m_TempEntInfo->Insert(name, (void *)te);
				m_TEList.push_back(te);
				return te;
			}
			iter = *(void **)((unsigned char *)iter + m_NextOffs);
		}
		return NULL;
	}

	return te;
}
