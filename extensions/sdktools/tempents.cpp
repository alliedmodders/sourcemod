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

TempEntityManager g_TEManager;
ICallWrapper *g_GetServerClass = NULL;

CON_COMMAND(sm_print_telist, "Prints the temp entity list")
{
	if (!g_TEManager.IsAvailable())
	{
		META_CONPRINT("The tempent portion of SDKTools failed to load.\n");
		META_CONPRINT("Check that you have the latest sdktools.games.txt file!\n");
		return;
	}
	g_TEManager.DumpList();
}

CON_COMMAND(sm_dump_teprops, "Dumps tempentity props to a file")
{
#if SOURCE_ENGINE <= SE_DARKMESSIAH
	CCommand args;
#endif
	if (!g_TEManager.IsAvailable())
	{
		META_CONPRINT("The tempent portion of SDKTools failed to load.\n");
		META_CONPRINT("Check that you have the latest sdktools.games.txt file!\n");
		return;
	}
	int argc = args.ArgC();
	if (argc < 2)
	{
		META_CONPRINT("Usage: sm_dump_teprops <file>\n");
		return;
	}

	const char *arg = args.Arg(1);

	if (!arg || arg[0] == '\0')
	{
		META_CONPRINTF("Usage: sm_dump_teprops <file>\n");
		return;
	}

	char path[PLATFORM_MAX_PATH];
	g_pSM->BuildPath(Path_Game, path, sizeof(path), "%s", arg);

	FILE *fp = NULL;
	if ((fp = fopen(path, "wt")) == NULL)
	{
		META_CONPRINTF("Could not open file \"%s\"\n", path);
		return;
	}

	g_TEManager.DumpProps(fp);

	fclose(fp);
}

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

ServerClass *TempEntityInfo::GetServerClass()
{
	return m_Sc;
}

bool TempEntityInfo::IsValidProp(const char *name)
{
	return (g_pGameHelpers->FindInSendTable(m_Sc->GetName(), name)) ? true : false;
}

int TempEntityInfo::_FindOffset(const char *name, int *size)
{
	int offset;

	sm_sendprop_info_t info;
	if (!g_pGameHelpers->FindSendPropInfo(m_Sc->GetName(), name, &info))
	{
		return -1;
	}

	offset = info.actual_offset;
	if (size)
	{
		*size = info.prop->m_nBits;
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
	} else if (size <= 32) {
		*(int *)((uint8_t *)m_Me + offset) = value;
	} else {
		return false;
	}

	return true;
}

bool TempEntityInfo::TE_GetEntData(const char *name, int *value)
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
		*value = *((uint8_t *)m_Me + offset);
	} else if (size <= 16) {
		*value = *(short *)((uint8_t *)m_Me + offset);
	} else if (size <= 32) {
		*value = *(int *)((uint8_t *)m_Me + offset);
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

bool TempEntityInfo::TE_GetEntDataFloat(const char *name, float *value)
{
	/* Search for our offset */
	int offset = _FindOffset(name);

	if (offset < 0)
	{
		return false;
	}

	*value = *(float *)((uint8_t *)m_Me + offset);

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

bool TempEntityInfo::TE_GetEntDataVector(const char *name, float vector[3])
{
	/* Search for our offset */
	int offset = _FindOffset(name);

	if (offset < 0)
	{
		return false;
	}

	Vector *v = (Vector *)((uint8_t *)m_Me + offset);
	vector[0] = v->x;
	vector[1] = v->y;
	vector[2] = v->z;

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

#if SOURCE_ENGINE == SE_TF2        \
	|| SOURCE_ENGINE == SE_DODS    \
	|| SOURCE_ENGINE == SE_HL2DM   \
	|| SOURCE_ENGINE == SE_CSS     \
	|| SOURCE_ENGINE == SE_SDK2013 \
	|| SOURCE_ENGINE == SE_BMS     \
	|| SOURCE_ENGINE == SE_BLADE   \
	|| SOURCE_ENGINE == SE_NUCLEARDAWN

	if (g_SMAPI->GetServerFactory(false)("VSERVERTOOLS003", nullptr))
	{
		m_ListHead = servertools->GetTempEntList();
	}
	else
#endif
	{
		/*
		 * First try to lookup s_pTempEntities directly for platforms with symbols.
		 * If symbols aren't present (Windows or stripped Linux/Mac),
		 * attempt find via CBaseTempEntity ctor + offset
		 */

		/* Read our sigs and offsets from the config file */
		if (g_pGameConf->GetMemSig("s_pTempEntities", &addr) && addr)
		{

			/* Store the head of the TE linked list */
			m_ListHead = *(void **) addr;
		}
		else if (g_pGameConf->GetMemSig("CBaseTempEntity", &addr) && addr)
		{
			if (!g_pGameConf->GetOffset("s_pTempEntities", &offset))
			{
				return;
			}
			/* Store the head of the TE linked list */
#ifdef PLATFORM_X86
			m_ListHead = **(void ***) ((unsigned char *) addr + offset);
#else
			int32_t varOffset = *(int32_t *) ((unsigned char *) addr + offset);
			m_ListHead = **(void ***) ((unsigned char *) addr + offset + sizeof(int32_t) + varOffset);
#endif
		}
		else
		{
			return;
		}
	}

	if (!g_pGameConf->GetOffset("GetTEName", &m_NameOffs))
	{
		return;
	}
	if (!g_pGameConf->GetOffset("GetTENext", &m_NextOffs))
	{
		return;
	}
	if (!g_pGameConf->GetOffset("TE_GetServerClass", &m_GetClassNameOffs))
	{
		return;
	}

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

	SourceHook::List<TempEntityInfo *>::iterator iter;
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

const char *TempEntityManager::GetNameFromThisPtr(void *me)
{
	return *(const char **)((unsigned char *)me + m_NameOffs);
}

void TempEntityManager::DumpList()
{
	unsigned int index = 0;
	META_CONPRINT("Listing temp entities:\n");
	void *iter = m_ListHead;
	while (iter)
	{
		const char *realname = *(const char **)((unsigned char *)iter + m_NameOffs);
		if (!realname)
		{
			break;
		}
		TempEntityInfo *info = GetTempEntityInfo(realname);
		if (!info)
		{
			continue;
		}
		ServerClass *sc = info->GetServerClass();
		META_CONPRINTF("[%02d] %s (%s)\n", index++, realname, sc->GetName());
		iter = *(void **)((unsigned char *)iter + m_NextOffs);
	}
	META_CONPRINTF("%d tempent%s found.\n", index, (index == 1) ? " was" : "s were");
}

const char *SendPropTypeToString(SendPropType type)
{
	if (type == DPT_Int)
	{
		return "int";
	} else if (type == DPT_Float) {
		return "float";
	} else if (type == DPT_Vector) {
		return "vector";
	} else if (type == DPT_String) {
		return "string";
	} else if (type == DPT_Array) {
		return "array";
	} else if (type == DPT_DataTable) {
		return "datatable";
	} else {
		return "unknown";
	}
}

void _DumpProps(FILE *fp, SendTable *pTable)
{
	SendTable *pOther;
	for (int i=0; i<pTable->GetNumProps(); i++)
	{
		SendProp *prop = pTable->GetProp(i);
		if ((pOther = prop->GetDataTable()) != NULL)
		{
			_DumpProps(fp, pOther);
		} else {
			fprintf(fp, "\t\t\t\"%s\"\t\t\"%s\"\n", 
				prop->GetName() ? prop->GetName() : "unknown",
				SendPropTypeToString(prop->GetType()));
		}
	}
}

void TempEntityManager::DumpProps(FILE *fp)
{
	unsigned int index = 0;
	void *iter = m_ListHead;
	fprintf(fp, "\"TempEnts\"\n{\n");
	while (iter)
	{
		const char *realname = *(const char **)((unsigned char *)iter + m_NameOffs);
		if (!realname)
		{
			break;
		}
		TempEntityInfo *info = GetTempEntityInfo(realname);
		if (!info)
		{
			continue;
		}
		ServerClass *sc = info->GetServerClass();
		fprintf(fp, "\t\"%s\"\n", sc->GetName());
		fprintf(fp, "\t{\n");
		fprintf(fp, "\t\t\"name\"\t\t\"%s\"\n", realname);
		fprintf(fp, "\t\t\"index\"\t\t\"%d\"\n", index++);
		fprintf(fp, "\t\t\"SendTable\"\n\t\t{\n");
		_DumpProps(fp, sc->m_pTable);
		fprintf(fp, "\t\t}\n\t}\n");
		iter = *(void **)((unsigned char *)iter + m_NextOffs);
	}
	fprintf(fp, "}\n");
	META_CONPRINTF("%d tempent%s written to file.\n", index, (index == 1) ? " was" : "s were");
}
