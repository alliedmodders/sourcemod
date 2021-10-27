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

#include "extension.h"
#include "vhelpers.h"

static void *s_pGameRules = nullptr;
static void **g_ppGameRules = nullptr;
void *g_EntList = nullptr;
CBaseHandle g_ResourceEntity;

void *GameRules()
{
	return g_ppGameRules ? *g_ppGameRules : s_pGameRules;
}

void InitializeValveGlobals()
{
	g_EntList = gamehelpers->GetGlobalEntityList();

	/*
	 * g_pGameRules
	 *
	 * First try to lookup pointer directly for platforms with symbols.
	 * If symbols aren't present (Windows or stripped Linux/Mac), 
	 * attempt find via CreateGameRulesObject + offset
	 */

	char *addr;
	if (g_pGameConf->GetMemSig("g_pGameRules", (void **)&addr) && addr)
	{
		g_ppGameRules = reinterpret_cast<void **>(addr);
	}
	else if (g_pGameConf->GetMemSig("CreateGameRulesObject", (void **)&addr) && addr)
	{
		int offset;
		if (!g_pGameConf->GetOffset("g_pGameRules", &offset) || !offset)
		{
			return;
		}
#ifdef PLATFORM_X86
		g_ppGameRules = *reinterpret_cast<void ***>(addr + offset);
#else
		int32_t varOffset = *(int32_t *) ((unsigned char *) addr + offset);
		g_ppGameRules = *reinterpret_cast<void ***>((unsigned char *) addr + offset + sizeof(int32_t) + varOffset);
#endif
	}
}

static bool UTIL_FindDataTable(SendTable *pTable,
	const char *name,
	sm_sendprop_info_t *info,
	unsigned int offset = 0)
{
	const char *pname;
	int props = pTable->GetNumProps();
	SendProp *prop;
	SendTable *table;

	for (int i = 0; i<props; i++)
	{
		prop = pTable->GetProp(i);

		if ((table = prop->GetDataTable()) != NULL)
		{
			pname = prop->GetName();
			if (pname && strcmp(name, pname) == 0)
			{
				info->prop = prop;
				info->actual_offset = offset + info->prop->GetOffset();
				return true;
			}

			if (UTIL_FindDataTable(table,
				name,
				info,
				offset + prop->GetOffset())
				)
			{
				return true;
			}
		}
	}

	return false;
}

static ServerClass *UTIL_FindServerClass(const char *classname)
{
	ServerClass *sc = gamedll->GetAllServerClasses();
	while (sc)
	{
		if (strcmp(classname, sc->GetName()) == 0)
		{
			return sc;
		}
		sc = sc->m_pNext;
	}

	return nullptr;
}

void UpdateValveGlobals()
{
	s_pGameRules = nullptr;

	const char *pszNetClass = g_pGameConf->GetKeyValue("GameRulesProxy");
	const char *pszDTName = g_pGameConf->GetKeyValue("GameRulesDataTable");
	if (pszNetClass && pszDTName)
	{
		sm_sendprop_info_t info;
		ServerClass *sc = UTIL_FindServerClass(pszNetClass);

		if (sc && UTIL_FindDataTable(sc->m_pTable, pszDTName, &info))
		{
			auto proxyFn = info.prop->GetDataTableProxyFn();
			if (proxyFn)
			{
				CSendProxyRecipients recp;
				s_pGameRules = proxyFn(nullptr, nullptr, nullptr, &recp, 0);
			}
		}
	}
}

size_t UTIL_StringToSignature(const char *str, char buffer[], size_t maxlength)
{
	size_t real_bytes = 0;
	size_t length = strlen(str);

	for (size_t i=0; i<length; i++)
	{
		if (real_bytes >= maxlength)
		{
			break;
		}
		buffer[real_bytes++] = (unsigned char)str[i];
		if (str[i] == '\\'
			&& str[i+1] == 'x')
		{
			if (i + 3 >= length)
			{
				continue;
			}
			/* Get the hex part */
			char s_byte[3];
			int r_byte;
			s_byte[0] = str[i+2];
			s_byte[1] = str[i+3];
			s_byte[2] = '\n';
			/* Read it as an integer */
			sscanf(s_byte, "%x", &r_byte);
			/* Save the value */
			buffer[real_bytes-1] = (unsigned char)r_byte;
			/* Adjust index */
			i += 3;
		}
	}

	return real_bytes;
}

bool UTIL_VerifySignature(const void *addr, const char *sig, size_t len)
{
	unsigned char *addr1 = (unsigned char *) addr;
	unsigned char *addr2 = (unsigned char *) sig;

	for (size_t i = 0; i < len; i++)
	{
		if (addr2[i] == '*')
			continue;
		if (addr1[i] != addr2[i])
			return false;
	}

	return true;
}

#ifdef PLATFORM_X64
#define KEY_SUFFIX "64"
#else
#define KEY_SUFFIX ""
#endif

#if defined PLATFORM_WINDOWS
#define FAKECLIENT_KEY "CreateFakeClient_Windows" KEY_SUFFIX
#elif defined PLATFORM_LINUX
#define FAKECLIENT_KEY "CreateFakeClient_Linux" KEY_SUFFIX
#elif defined PLATFORM_APPLE
#define FAKECLIENT_KEY "CreateFakeClient_Mac" KEY_SUFFIX
#else
#error "Unsupported platform"
#endif

void GetIServer()
{
#if SOURCE_ENGINE == SE_TF2        \
	|| SOURCE_ENGINE == SE_DODS    \
	|| SOURCE_ENGINE == SE_HL2DM   \
	|| SOURCE_ENGINE == SE_CSS     \
	|| SOURCE_ENGINE == SE_SDK2013 \
	|| SOURCE_ENGINE == SE_BMS     \
	|| SOURCE_ENGINE == SE_DOI     \
	|| SOURCE_ENGINE == SE_BLADE   \
	|| SOURCE_ENGINE == SE_INSURGENCY

#if SOURCE_ENGINE == SE_SDK2013
	if (g_SMAPI->GetEngineFactory(false)("VEngineServer022", nullptr))
#endif // SE_SDK2013
	{
		iserver = engine->GetIServer();
		return;
	}
#endif

	void *addr;
	const char *sigstr;
	char sig[32];
	size_t siglen;
	int offset;
	void *vfunc = NULL;

	/* Use the symbol if it exists */
	if (g_pGameConf->GetMemSig("sv", &addr) && addr)
	{
		iserver = reinterpret_cast<IServer *>(addr);
		return;
	}

	/* Get the CreateFakeClient function pointer */
	if (!(vfunc=SH_GET_ORIG_VFNPTR_ENTRY(engine, &IVEngineServer::CreateFakeClient)))
	{
		return;
	}

	/* Get signature string for IVEngineServer::CreateFakeClient() */
	sigstr = g_pGameConf->GetKeyValue(FAKECLIENT_KEY);

	if (!sigstr)
	{
		return;
	}

	/* Convert signature string to signature bytes */
	siglen = UTIL_StringToSignature(sigstr, sig, sizeof(sig));

	/* Check if we're on the expected function */
	if (!UTIL_VerifySignature(vfunc, sig, siglen))
	{
		return;
	}

	/* Get the offset into CreateFakeClient */
	if (!g_pGameConf->GetOffset("sv", &offset))
	{
		return;
	}

	/* Finally we have the interface we were looking for */
#ifdef PLATFORM_X86
	iserver = *reinterpret_cast<IServer **>(reinterpret_cast<unsigned char *>(vfunc) + offset);
#elif defined PLATFORM_X64
	int32_t varOffset = *reinterpret_cast<int32_t *>(reinterpret_cast<unsigned char *>(vfunc) + offset);
	iserver = reinterpret_cast<IServer *>(reinterpret_cast<unsigned char *>(vfunc) + offset + sizeof(int32_t) + varOffset);
#endif
}

void GetResourceEntity()
{
	g_ResourceEntity.Term();
	
#if SOURCE_ENGINE >= SE_ORANGEBOX
	const char *classname = g_pGameConf->GetKeyValue("ResourceEntityClassname");
	if (classname != NULL)
	{
		for (CBaseEntity *pEntity = (CBaseEntity *)servertools->FirstEntity(); pEntity; pEntity = (CBaseEntity *)servertools->NextEntity(pEntity))
		{
			if (!strcmp(gamehelpers->GetEntityClassname(pEntity), classname))
			{
				g_ResourceEntity = ((IHandleEntity *)pEntity)->GetRefEHandle();
				break;
			}
		}
	}
	else
#endif
	{
		int edictCount = gpGlobals->maxEntities;

		for (int i=0; i<edictCount; i++)
		{
			edict_t *pEdict = PEntityOfEntIndex(i);
			if (!pEdict || pEdict->IsFree())
			{
				continue;
			}
			if (!pEdict->GetNetworkable())
			{
				continue;
			}

			IHandleEntity *pHandleEnt = pEdict->GetNetworkable()->GetEntityHandle();
			if (!pHandleEnt)
			{
				continue;
			}

			ServerClass *pClass = pEdict->GetNetworkable()->GetServerClass();
			if (FindNestedDataTable(pClass->m_pTable, "DT_PlayerResource"))
			{
				g_ResourceEntity = pHandleEnt->GetRefEHandle();
				break;
			}
		}
	}
}

const char *GetDTTypeName(int type)
{
	switch (type)
	{
	case DPT_Int:
		{
			return "integer";
		}
	case DPT_Float:
		{
			return "float";
		}
	case DPT_Vector:
		{
			return "vector";
		}
	case DPT_String:
		{
			return "string";
		}
	case DPT_Array:
		{
			return "array";
		}
	case DPT_DataTable:
		{
			return "datatable";
		}
#if SOURCE_ENGINE >= SE_ALIENSWARM
	case DPT_Int64:
		{
			return "int64";
		}
#endif
	default:
		{
			return NULL;
		}
	}

	return NULL;
}
