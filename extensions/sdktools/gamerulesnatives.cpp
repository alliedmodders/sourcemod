/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod SDKTools Extension
 * Copyright (C) 2004-2011 AlliedModders LLC.  All rights reserved.
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
#include "gamerulesnatives.h"
#include "vglobals.h"

const char *g_szGameRulesProxy;

void GameRulesNativesInit()
{
	g_szGameRulesProxy = g_pGameConf->GetKeyValue("GameRulesProxy");
}

static CBaseEntity *FindEntityByNetClass(int start, const char *classname)
{
	int maxEntities = gpGlobals->maxEntities;
	for (int i = start; i < maxEntities; i++)
	{
		edict_t *current = gamehelpers->EdictOfIndex(i);
		if (current == NULL || current->IsFree())
			continue;

		IServerNetworkable *network = current->GetNetworkable();
		if (network == NULL)
			continue;

		IHandleEntity *pHandleEnt = network->GetEntityHandle();
		if (pHandleEnt == NULL)
			continue;

		ServerClass *sClass = network->GetServerClass();
		const char *name = sClass->GetName();

		if (!strcmp(name, classname))
			return gamehelpers->ReferenceToEntity(gamehelpers->IndexOfEdict(current));		
	}

	return NULL;
}

static CBaseEntity* GetGameRulesProxyEnt()
{
	static cell_t proxyEntRef = -1;
	CBaseEntity *pProxy;
	if (proxyEntRef == -1 || (pProxy = gamehelpers->ReferenceToEntity(proxyEntRef)) == NULL)
	{
		pProxy = FindEntityByNetClass(playerhelpers->GetMaxClients(), g_szGameRulesProxy);
		if (pProxy)
			proxyEntRef = gamehelpers->EntityToReference(pProxy);
	}
	
	return pProxy;
}

enum PropFieldType
{
	PropField_Unsupported,		/**< The type is unsupported. */
	PropField_Integer,			/**< Valid for SendProp and Data fields */
	PropField_Float,			/**< Valid for SendProp and Data fields */
	PropField_Entity,			/**< Valid for Data fields only (SendProp shows as int) */
	PropField_Vector,			/**< Valid for SendProp and Data fields */
	PropField_String,			/**< Valid for SendProp and Data fields */
	PropField_String_T,			/**< Valid for Data fields.  Read only! */
};

#define FIND_PROP_SEND(type, type_name) \
	sm_sendprop_info_t info;\
	SendProp *pProp; \
	if (!gamehelpers->FindSendPropInfo(g_szGameRulesProxy, prop, &info)) \
	{ \
		return pContext->ThrowNativeError("Property \"%s\" not found on the gamerules proxy", prop); \
	} \
	\
	offset = info.actual_offset; \
	pProp = info.prop; \
	bit_count = pProp->m_nBits; \
	\
	switch (pProp->GetType()) \
	{ \
	case type: \
		{ \
			if (element != 0) \
			{ \
				return pContext->ThrowNativeError("SendProp %s is not an array. Element %d is invalid.", \
					prop, \
					element); \
			} \
			break; \
		} \
	case DPT_Array: \
		{ \
			int elementCount = pProp->GetNumElements(); \
			int elementStride = pProp->GetElementStride(); \
			if (element < 0 || element >= elementCount) \
			{ \
				return pContext->ThrowNativeError("Element %d is out of bounds (Prop %s has %d elements).", \
					element, \
					prop, \
					elementCount); \
			} \
			\
			pProp = pProp->GetArrayProp(); \
			if (!pProp) { \
				return pContext->ThrowNativeError("Error looking up ArrayProp for prop %s", \
					prop); \
			} \
			\
			if (pProp->GetType() != type) \
			{ \
				return pContext->ThrowNativeError("SendProp %s type is not " type_name " ([%d,%d] != %d)", \
					prop, \
					pProp->GetType(), \
					pProp->m_nBits, \
					type); \
			} \
			\
			offset += pProp->GetOffset() + (elementStride * element); \
			bit_count = pProp->m_nBits; \
			break; \
		} \
	case DPT_DataTable: \
		{ \
			FIND_PROP_SEND_IN_SENDTABLE(info, pProp, element, type, type_name); \
			\
			offset += pProp->GetOffset(); \
			bit_count = pProp->m_nBits; \
			break; \
		} \
	default: \
		{ \
			return pContext->ThrowNativeError("SendProp %s type is not " type_name " (%d != %d)", \
				prop, \
				pProp->GetType(), \
				type); \
		} \
	} \

#define FIND_PROP_SEND_IN_SENDTABLE(info, pProp, element, type, type_name) \
	SendTable *pTable = pProp->GetDataTable(); \
	if (!pTable) \
	{ \
		return pContext->ThrowNativeError("Error looking up DataTable for prop %s", \
			prop); \
	} \
	\
	int elementCount = pTable->GetNumProps(); \
	if (element < 0 || element >= elementCount) \
	{ \
		return pContext->ThrowNativeError("Element %d is out of bounds (Prop %s has %d elements).", \
			element, \
			prop, \
			elementCount); \
	} \
	\
	pProp = pTable->GetProp(element); \
	if (pProp->GetType() != type) \
	{ \
		return pContext->ThrowNativeError("SendProp %s type is not " type_name " ([%d,%d] != %d)", \
			prop, \
			pProp->GetType(), \
			pProp->m_nBits, \
			type); \
	}

static cell_t GameRules_GetProp(IPluginContext *pContext, const cell_t *params)
{
	char *prop;
	int element = params[3];
	int offset;
	int bit_count;
	bool is_unsigned;

	void *pGameRules = GameRules();

	if (!pGameRules || !g_szGameRulesProxy || !strcmp(g_szGameRulesProxy, ""))
		return pContext->ThrowNativeError("Gamerules lookup failed.");

	pContext->LocalToString(params[1], &prop);

	int elementCount = 1;

	FIND_PROP_SEND(DPT_Int, "integer");
	is_unsigned = ((pProp->GetFlags() & SPROP_UNSIGNED) == SPROP_UNSIGNED);

	// This isn't in CS:S yet, but will be, doesn't hurt to add now, and will save us a build later
#if SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_HL2DM || SOURCE_ENGINE == SE_DODS || SOURCE_ENGINE == SE_TF2 \
	|| SOURCE_ENGINE == SE_SDK2013 || SOURCE_ENGINE == SE_BMS || SOURCE_ENGINE == SE_CSGO || SOURCE_ENGINE == SE_BLADE
	if (pProp->GetFlags() & SPROP_VARINT)
	{
		bit_count = sizeof(int) * 8;
	}
#endif

	if (bit_count < 1)
	{
		bit_count = params[2] * 8;
	}

	if (bit_count >= 17)
	{
		return *(int32_t *)((intptr_t)pGameRules + offset);
	}
	else if (bit_count >= 9)
	{
		if (is_unsigned)
		{
			return *(uint16_t *)((intptr_t)pGameRules + offset);
		}
		else
		{
			return *(int16_t *)((intptr_t)pGameRules + offset);
		}
	}
	else if (bit_count >= 2)
	{
		if (is_unsigned)
		{
			return *(uint8_t *)((intptr_t)pGameRules + offset);
		}
		else
		{
			return *(int8_t *)((intptr_t)pGameRules + offset);
		}
	}
	else
	{
		return *(bool *)((intptr_t)pGameRules + offset) ? 1 : 0;
	}

	return -1;
}

static cell_t GameRules_SetProp(IPluginContext *pContext, const cell_t *params)
{
	char *prop;
	int element = params[4];
	int offset;
	int bit_count;

	void *pGameRules = GameRules();

	CBaseEntity *pProxy = NULL;
	if ((pProxy = GetGameRulesProxyEnt()) == NULL)
		return pContext->ThrowNativeError("Couldn't find gamerules proxy entity");
	
	if (!pGameRules || !g_szGameRulesProxy || !strcmp(g_szGameRulesProxy, ""))
		return pContext->ThrowNativeError("Gamerules lookup failed");

	pContext->LocalToString(params[1], &prop);

#if SOURCE_ENGINE == SE_CSGO
	if (!g_SdkTools.CanSetCSGOEntProp(prop))
	{
		return pContext->ThrowNativeError("Cannot set ent prop %s with core.cfg option \"FollowCSGOServerGuidelines\" enabled.", prop);
	}
#endif

	FIND_PROP_SEND(DPT_Int, "integer");

#if SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_HL2DM || SOURCE_ENGINE == SE_DODS || SOURCE_ENGINE == SE_TF2 \
	|| SOURCE_ENGINE == SE_SDK2013 || SOURCE_ENGINE == SE_BMS || SOURCE_ENGINE == SE_CSGO || SOURCE_ENGINE == SE_BLADE
	if (pProp->GetFlags() & SPROP_VARINT)
	{
		bit_count = sizeof(int) * 8;
	}
#endif

	if (bit_count < 1)
	{
		bit_count = params[3] * 8;
	}

	if (bit_count >= 17)
	{
		*(int32_t *)((intptr_t)pGameRules + offset) = params[2];
	}
	else if (bit_count >= 9)
	{
		*(int16_t *)((intptr_t)pGameRules + offset) = (int16_t)params[2];
	}
	else if (bit_count >= 2)
	{
		*(int8_t *)((intptr_t)pGameRules + offset) = (int8_t)params[2];
	}
	else
	{
		*(bool *)((intptr_t)pGameRules + offset) = (params[2] == 0) ? false : true;
	}

	edict_t *proxyEdict = gamehelpers->EdictOfIndex(gamehelpers->EntityToBCompatRef(pProxy));
	if (proxyEdict != NULL)
		gamehelpers->SetEdictStateChanged(proxyEdict, offset);

	return 0;
}

static cell_t GameRules_GetPropFloat(IPluginContext *pContext, const cell_t *params)
{
	char *prop;
	int element = params[2];
	int offset;
	int bit_count;

	void *pGameRules = GameRules();

	if (!pGameRules || !g_szGameRulesProxy || !strcmp(g_szGameRulesProxy, ""))
		return pContext->ThrowNativeError("Gamerules lookup failed.");

	pContext->LocalToString(params[1], &prop);

	FIND_PROP_SEND(DPT_Float, "float");

	float val = *(float *)((intptr_t)pGameRules + offset);

	return sp_ftoc(val);
}

static cell_t GameRules_SetPropFloat(IPluginContext *pContext, const cell_t *params)
{
	char *prop;
	int element = params[3];
	int offset;
	int bit_count;

	void *pGameRules = GameRules();

	CBaseEntity *pProxy = NULL;
	if ((pProxy = GetGameRulesProxyEnt()) == NULL)
		return pContext->ThrowNativeError("Couldn't find gamerules proxy entity.");
	
	if (!pGameRules || !g_szGameRulesProxy || !strcmp(g_szGameRulesProxy, ""))
		return pContext->ThrowNativeError("Gamerules lookup failed.");

	pContext->LocalToString(params[1], &prop);

#if SOURCE_ENGINE == SE_CSGO
	if (!g_SdkTools.CanSetCSGOEntProp(prop))
	{
		return pContext->ThrowNativeError("Cannot set ent prop %s with core.cfg option \"FollowCSGOServerGuidelines\" enabled.", prop);
	}
#endif

	FIND_PROP_SEND(DPT_Float, "float");

	float newVal = sp_ctof(params[2]);

	*(float *)((intptr_t)pGameRules + offset) = newVal;

	edict_t *proxyEdict = gamehelpers->EdictOfIndex(gamehelpers->EntityToBCompatRef(pProxy));
	if (proxyEdict != NULL)
		gamehelpers->SetEdictStateChanged(proxyEdict, offset);

	return 0;
}

static cell_t GameRules_GetPropEnt(IPluginContext *pContext, const cell_t *params)
{
	char *prop;
	int element = params[2];
	int offset;
	int bit_count;

	void *pGameRules = GameRules();

	if (!pGameRules || !g_szGameRulesProxy || !strcmp(g_szGameRulesProxy, ""))
		return pContext->ThrowNativeError("Gamerules lookup failed.");

	pContext->LocalToString(params[1], &prop);

	FIND_PROP_SEND(DPT_Int, "Integer");

	CBaseHandle &hndl = *(CBaseHandle *)((intptr_t)pGameRules + offset);
	CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(hndl.GetEntryIndex());

	if (!pEntity || ((IServerEntity *)pEntity)->GetRefEHandle() != hndl)
	{
		return -1;
	}

	return gamehelpers->EntityToBCompatRef(pEntity);
}

static cell_t GameRules_SetPropEnt(IPluginContext *pContext, const cell_t *params)
{
	char *prop;
	int element = params[3];
	int offset;
	int bit_count;

	void *pGameRules = GameRules();

	CBaseEntity *pProxy = NULL;
	if ((pProxy = GetGameRulesProxyEnt()) == NULL)
		return pContext->ThrowNativeError("Couldn't find gamerules proxy entity.");
	
	if (!pGameRules || !g_szGameRulesProxy || !strcmp(g_szGameRulesProxy, ""))
		return pContext->ThrowNativeError("Gamerules lookup failed.");

	pContext->LocalToString(params[1], &prop);

#if SOURCE_ENGINE == SE_CSGO
	if (!g_SdkTools.CanSetCSGOEntProp(prop))
	{
		return pContext->ThrowNativeError("Cannot set ent prop %s with core.cfg option \"FollowCSGOServerGuidelines\" enabled.", prop);
	}
#endif

	FIND_PROP_SEND(DPT_Int, "integer");

	CBaseHandle &hndl = *(CBaseHandle *)((intptr_t)pGameRules + offset);
	CBaseEntity *pOther;

	if (params[2] == -1)
	{
		hndl.Set(NULL);
	}
	else
	{
		pOther = gamehelpers->ReferenceToEntity(params[2]);

		if (!pOther)
		{
			return pContext->ThrowNativeError("Entity %d (%d) is invalid", gamehelpers->ReferenceToIndex(params[4]), params[4]);
		}

		IHandleEntity *pHandleEnt = (IHandleEntity *)pOther;
		hndl.Set(pHandleEnt);
	}

	edict_t *proxyEdict = gamehelpers->EdictOfIndex(gamehelpers->EntityToBCompatRef(pProxy));
	if (proxyEdict != NULL)
		gamehelpers->SetEdictStateChanged(proxyEdict, offset);

	return 0;
}

static cell_t GameRules_GetPropVector(IPluginContext *pContext, const cell_t *params)
{
	char *prop;
	int element = params[3];
	int offset;
	int bit_count;

	void *pGameRules = GameRules();

	if (!pGameRules || !g_szGameRulesProxy || !strcmp(g_szGameRulesProxy, ""))
		return pContext->ThrowNativeError("Gamerules lookup failed.");

	pContext->LocalToString(params[1], &prop);

	FIND_PROP_SEND(DPT_Vector, "vector");

	Vector *v = (Vector *)((intptr_t)pGameRules + offset);

	cell_t *vec;
	pContext->LocalToPhysAddr(params[2], &vec);

	vec[0] = sp_ftoc(v->x);
	vec[1] = sp_ftoc(v->y);
	vec[2] = sp_ftoc(v->z);

	return 1;
}

static cell_t GameRules_SetPropVector(IPluginContext *pContext, const cell_t *params)
{
	char *prop;
	int element = params[3];
	int offset;
	int bit_count;

	void *pGameRules = GameRules();

	CBaseEntity *pProxy = NULL;
	if ((pProxy = GetGameRulesProxyEnt()) == NULL)
		return pContext->ThrowNativeError("Couldn't find gamerules proxy entity.");
	
	if (!pGameRules || !g_szGameRulesProxy || !strcmp(g_szGameRulesProxy, ""))
		return pContext->ThrowNativeError("Gamerules lookup failed.");

	pContext->LocalToString(params[1], &prop);

#if SOURCE_ENGINE == SE_CSGO
	if (!g_SdkTools.CanSetCSGOEntProp(prop))
	{
		return pContext->ThrowNativeError("Cannot set ent prop %s with core.cfg option \"FollowCSGOServerGuidelines\" enabled.", prop);
	}
#endif

	FIND_PROP_SEND(DPT_Vector, "vector");

	Vector *v = (Vector *)((intptr_t)pGameRules + offset);

	cell_t *vec;
	pContext->LocalToPhysAddr(params[2], &vec);

	v->x = sp_ctof(vec[0]);
	v->y = sp_ctof(vec[1]);
	v->z = sp_ctof(vec[2]);

	edict_t *proxyEdict = gamehelpers->EdictOfIndex(gamehelpers->EntityToBCompatRef(pProxy));
	if (proxyEdict != NULL)
		gamehelpers->SetEdictStateChanged(proxyEdict, offset);

	return 1;
}

static cell_t GameRules_GetPropString(IPluginContext *pContext, const cell_t *params)
{
	char *prop;
	int offset;
	int bit_count;

	int element = 0;
	if (params[0] >= 4)
	{
		element = params[4];
	}

	void *pGameRules = GameRules();

	if (!pGameRules || !g_szGameRulesProxy || !strcmp(g_szGameRulesProxy, ""))
		return pContext->ThrowNativeError("Gamerules lookup failed.");

	pContext->LocalToString(params[1], &prop);

	FIND_PROP_SEND(DPT_String, "string");

	const char *src;
	if (pProp->GetProxyFn())
	{
		DVariant var;
		pProp->GetProxyFn()(pProp, pGameRules, (const void *)((intptr_t)pGameRules + offset), &var, element, 0 /* TODO */);
		src = var.m_pString;
	}
	else
	{
		src = *(char **)((uint8_t *)pGameRules + offset);
	}

	if (src)
	{
		size_t len;
		pContext->StringToLocalUTF8(params[2], params[3], src, &len);
		return len;
	}

	pContext->StringToLocal(params[2], params[3], "");
	return 0;
}

static cell_t GameRules_SetPropString(IPluginContext *pContext, const cell_t *params)
{
	char *prop;
	int offset;
	int maxlen;
	int bit_count;

	int element = 0;
	if (params[0] >= 4)
	{
		element = params[4];
	}

	void *pGameRules = GameRules();

	CBaseEntity *pProxy = NULL;
	if ((pProxy = GetGameRulesProxyEnt()) == NULL)
		return pContext->ThrowNativeError("Couldn't find gamerules proxy entity.");
	
	if (!pGameRules || !g_szGameRulesProxy || !strcmp(g_szGameRulesProxy, ""))
		return pContext->ThrowNativeError("Gamerules lookup failed.");

	pContext->LocalToString(params[1], &prop);

#if SOURCE_ENGINE == SE_CSGO
	if (!g_SdkTools.CanSetCSGOEntProp(prop))
	{
		return pContext->ThrowNativeError("Cannot set ent prop %s with core.cfg option \"FollowCSGOServerGuidelines\" enabled.", prop);
	}
#endif

	FIND_PROP_SEND(DPT_String, "string");

	bool bIsStringIndex = false;
	if (pProp->GetProxyFn())
	{
		DVariant var;
		pProp->GetProxyFn()(pProp, pGameRules, (const void *)((intptr_t)pGameRules + offset), &var, element, 0 /* TODO */);
		if (var.m_pString == ((string_t *)((intptr_t)pGameRules + offset))->ToCStr())
		{
			bIsStringIndex = true;
		}
	}

	// Only used if not string index.
	// TODO: If we're writing to a DPT_Array, we should use the element stride here.
	maxlen = DT_MAX_STRING_BUFFERSIZE;

	char *src;
	size_t len;
	pContext->LocalToString(params[2], &src);

	if (bIsStringIndex)
	{
		return pContext->ThrowNativeError("Setting string_t gamerules prop %s not supported yet.", prop);

		// *(string_t *)((intptr_t)pGameRules + offset) = g_HL2.AllocPooledString(src);
		// len = strlen(src);
	}
	else
	{
		char *dest = (char *)((uint8_t *)pGameRules + offset);
		len = ke::SafeStrcpy(dest, maxlen, src);
	}

	edict_t *proxyEdict = gamehelpers->EdictOfIndex(gamehelpers->EntityToBCompatRef(pProxy));
	if (proxyEdict != NULL)
		gamehelpers->SetEdictStateChanged(proxyEdict, offset);

	return len;
}

sp_nativeinfo_t g_GameRulesNatives[] = 
{
	{"GameRules_GetProp",			GameRules_GetProp},
	{"GameRules_SetProp",			GameRules_SetProp},
	{"GameRules_GetPropFloat",		GameRules_GetPropFloat},
	{"GameRules_SetPropFloat",		GameRules_SetPropFloat},
	{"GameRules_GetPropEnt",		GameRules_GetPropEnt},
	{"GameRules_SetPropEnt",		GameRules_SetPropEnt},
	{"GameRules_GetPropVector",		GameRules_GetPropVector},
	{"GameRules_SetPropVector",		GameRules_SetPropVector},
	{"GameRules_GetPropString",		GameRules_GetPropString},
	{"GameRules_SetPropString",		GameRules_SetPropString},
	{NULL,							NULL},
};

