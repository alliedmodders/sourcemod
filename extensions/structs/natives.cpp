/**
* vim: set ts=4 :
* =============================================================================
* SourceMod Sample Extension
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
#include "utldict.h"

class FileWeaponInfo_t;
typedef unsigned short WEAPON_FILE_INFO_HANDLE;

inline edict_t *GetEdict(cell_t num)
{
	edict_t *pEdict = engine->PEntityOfEntIndex(num);
	if (!pEdict || pEdict->IsFree())
	{
		return NULL;
	}

	return pEdict;
}

inline edict_t *GetEntity(cell_t num, CBaseEntity **pData)
{
	edict_t *pEdict = engine->PEntityOfEntIndex(num);
	if (!pEdict || pEdict->IsFree())
	{
		return NULL;
	}

	IServerUnknown *pUnk;
	if ((pUnk=pEdict->GetUnknown()) == NULL)
	{
		return NULL;
	}
	*pData = pUnk->GetBaseEntity();
	return pEdict;
}

int CheckBaseHandle(CBaseHandle &hndl)
{
	if (!hndl.IsValid())
	{
		return -1;
	}

	int index = hndl.GetEntryIndex();

	edict_t *pStoredEdict;
	CBaseEntity *pStoredEntity;

	pStoredEdict = GetEntity(index, &pStoredEntity);

	if (pStoredEdict == NULL || pStoredEntity == NULL)
	{
		return -1;
	}

	IServerEntity *pSE = pStoredEdict->GetIServerEntity();

	if (pSE == NULL)
	{
		return -1;
	}

	if (pSE->GetRefEHandle() != hndl)
	{
		return -1;
	}

	return index;
}

//native GetStructInt(Handle:struct, const String:member[]);
static cell_t GetStructInt(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	HandleSecurity sec;

	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	StructHandle *pHandle;
	if ((err = g_pHandleSys->ReadHandle(hndl, g_StructHandle, &sec, (void **)&pHandle))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid struct handle %x (error %d)", hndl, err);
	}

	char *member;
	pContext->LocalToString(params[2], &member);

	int value;

	if (!pHandle->GetInt(member, &value))
	{
		return pContext->ThrowNativeError("Invalid member, or incorrect data type");
	}

	return value;
}

//native SetStructInt(Handle:struct, const String:member[], value);
static cell_t SetStructInt(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	HandleSecurity sec;

	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	StructHandle *pHandle;
	if ((err = handlesys->ReadHandle(hndl, g_StructHandle, &sec, (void **)&pHandle))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid struct handle %x (error %d)", hndl, err);
	}

	char *member;
	pContext->LocalToString(params[2], &member);

	if (!pHandle->SetInt(member, params[3]))
	{
		return pContext->ThrowNativeError("Invalid member, or incorrect data type");
	}

	return 1;
}

//native GetStructFloat(Handle:struct, const String:member[], &Float:value);
static cell_t GetStructFloat(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	HandleSecurity sec;

	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	StructHandle *pHandle;
	if ((err = g_pHandleSys->ReadHandle(hndl, g_StructHandle, &sec, (void **)&pHandle))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid struct handle %x (error %d)", hndl, err);
	}

	char *member;
	pContext->LocalToString(params[2], &member);

	float value;

	if (!pHandle->GetFloat(member, &value))
	{
		return pContext->ThrowNativeError("Invalid member, or incorrect data type");
	}

	return sp_ftoc(value);
}	

//native SetStructFloat(Handle:struct, const String:member[], Float:value);
static cell_t SetStructFloat(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	HandleSecurity sec;

	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	StructHandle *pHandle;
	if ((err = g_pHandleSys->ReadHandle(hndl, g_StructHandle, &sec, (void **)&pHandle))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid struct handle %x (error %d)", hndl, err);
	}

	char *member;
	pContext->LocalToString(params[2], &member);

	if (!pHandle->SetFloat(member, sp_ctof(params[3])))
	{
		return pContext->ThrowNativeError("Invalid member, or incorrect data type");
	}

	return 1;
}

//native GetStructVector(Handle:struct, const String:member[], Float:vec[3]);
static cell_t GetStructVector(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	HandleSecurity sec;

	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	StructHandle *pHandle;
	if ((err = g_pHandleSys->ReadHandle(hndl, g_StructHandle, &sec, (void **)&pHandle))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid struct handle %x (error %d)", hndl, err);
	}

	char *member;
	pContext->LocalToString(params[2], &member);

	cell_t *addr;
	pContext->LocalToPhysAddr(params[3], &addr);

	Vector value;

	if (!pHandle->GetVector(member, &value))
	{
		return pContext->ThrowNativeError("Invalid member, or incorrect data type");
	}

	addr[0] = sp_ftoc(value.x);
	addr[1] = sp_ftoc(value.y);
	addr[2] = sp_ftoc(value.z);

	return 1;
}

//native SetStructVector(Handle:struct, const String:member[], Float:vec[3]);
static cell_t SetStructVector(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	HandleSecurity sec;

	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	StructHandle *pHandle;
	if ((err = g_pHandleSys->ReadHandle(hndl, g_StructHandle, &sec, (void **)&pHandle))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid struct handle %x (error %d)", hndl, err);
	}

	char *member;
	pContext->LocalToString(params[2], &member);

	Vector value;

	cell_t *addr;
	pContext->LocalToPhysAddr(params[3], &addr);

	value.x = sp_ctof(addr[0]);
	value.y = sp_ctof(addr[1]);
	value.z = sp_ctof(addr[2]);

	if (!pHandle->SetVector(member, value))
	{
		return pContext->ThrowNativeError("Invalid member, or incorrect data type");
	}

	return 1;
}

//native GetStructString(Handle:struct, const String:member[], String:value[], maxlen);
static cell_t GetStructString(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	HandleSecurity sec;

	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	StructHandle *pHandle;
	if ((err = g_pHandleSys->ReadHandle(hndl, g_StructHandle, &sec, (void **)&pHandle))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid struct handle %x (error %d)", hndl, err);
	}

	char *member;
	pContext->LocalToString(params[2], &member);

	char *string;
	if (!pHandle->GetString(member, &string))
	{
		return pContext->ThrowNativeError("Invalid member, or incorrect data type");
	}

	pContext->StringToLocal(params[3], params[4], string);

	return 1;
}

//native SetStructString(Handle:struct, const String:member[], String:value[]);
static cell_t SetStructString(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	HandleSecurity sec;

	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	StructHandle *pHandle;
	if ((err = g_pHandleSys->ReadHandle(hndl, g_StructHandle, &sec, (void **)&pHandle))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid struct handle %x (error %d)", hndl, err);
	}

	char *member;
	pContext->LocalToString(params[2], &member);

	char *string;
	pContext->LocalToString(params[3], &string);

	if (!pHandle->SetString(member, string))
	{
		return pContext->ThrowNativeError("Invalid member, or incorrect data type");
	}

	return 1;
}

//native GetStructEnt(Handle:struct, const String:member[], &ent);
static cell_t GetStructEnt(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	HandleSecurity sec;

	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	StructHandle *pHandle;
	if ((err = g_pHandleSys->ReadHandle(hndl, g_StructHandle, &sec, (void **)&pHandle))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid struct handle %x (error %d)", hndl, err);
	}

	char *member;
	pContext->LocalToString(params[2], &member);

	CBaseHandle *value;
	if (!pHandle->GetEHandle(member, &value))
	{
		return pContext->ThrowNativeError("Invalid member, or incorrect data type");
	}

	return CheckBaseHandle(*value);
}

//native SetStructEnt(Handle:struct, const String:member[], ent);
static cell_t SetStructEnt(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	HandleSecurity sec;

	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	StructHandle *pHandle;
	if ((err = g_pHandleSys->ReadHandle(hndl, g_StructHandle, &sec, (void **)&pHandle))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid struct handle %x (error %d)", hndl, err);
	}

	char *member;
	pContext->LocalToString(params[2], &member);

	CBaseHandle *value;
	if (!pHandle->GetEHandle(member, &value))
	{
		return pContext->ThrowNativeError("Invalid member, or incorrect data type");
	}

	edict_t *pEdict = GetEdict(params[3]);

	if (pEdict == NULL)
	{
		return pContext->ThrowNativeError("Invalid entity %i", params[3]);
	}

	IServerEntity *pEntOther = pEdict->GetIServerEntity();
	value->Set(pEntOther);

	if (!pHandle->SetEHandle(member, value))
	{
		return pContext->ThrowNativeError("Invalid member, or incorrect data type");
	}

	return 1;
}

static cell_t GetWeaponStruct(IPluginContext *pContext, const cell_t *params)
{
	char *weapon;
	pContext->LocalToString(params[1], &weapon);

	void **func;

#if defined WIN32
	void *addr = NULL;
	func = &addr;
#else
	WEAPON_FILE_INFO_HANDLE (*LookupWeaponInfoSlot)(const char *) = NULL;
	func = (void **)&LookupWeaponInfoSlot;
#endif

	FileWeaponInfo_t *(*GetFileWeaponInfoFromHandle)( WEAPON_FILE_INFO_HANDLE handle ) = NULL;

	if (!conf->GetMemSig("LookupWeaponInfoSlot", func) || *func == NULL)
	{
		return pContext->ThrowNativeError("Failed to locate signature LookupWeaponInfoSlot");
	}

	if (!conf->GetMemSig("GetFileWeaponInfoFromHandle", (void **)&GetFileWeaponInfoFromHandle) || GetFileWeaponInfoFromHandle == NULL)
	{
		return pContext->ThrowNativeError("Failed to locate signature GetFileWeaponInfoFromHandle");
	}

#if defined WIN32
	int offset;

	if (!conf->GetOffset("m_WeaponInfoDatabase", &offset))
	{
		return pContext->ThrowNativeError("Failed to locate offset m_WeaponInfoDatabase");
	}

	addr = (unsigned char *)addr + offset;

	CUtlDict< FileWeaponInfo_t*, unsigned short > *m_WeaponInfoDatabase = *(CUtlDict< FileWeaponInfo_t*, unsigned short > **)addr;

	WEAPON_FILE_INFO_HANDLE handle = m_WeaponInfoDatabase->Find(weapon);

	if (handle == -1 || handle == m_WeaponInfoDatabase->InvalidIndex())
	{
		return pContext->ThrowNativeError("Could not find weapon %s", weapon);
	}
#else
	WEAPON_FILE_INFO_HANDLE handle = LookupWeaponInfoSlot(weapon);
#endif
	FileWeaponInfo_t *info = GetFileWeaponInfoFromHandle(handle);

	//g_pSM->LogMessage(myself, "%i %i %x", handle, GetInvalidWeaponInfoHandle(), info);

	if (!info)
	{
		return pContext->ThrowNativeError("Weapon does not exist!");
	}

	/* Offsets! */
	/*
	g_pSM->LogMessage(myself, "Offsets for FileWeaponInfo_t");
	g_pSM->LogMessage(myself, "bParsed: %i", (unsigned char *)&(info->bParsedScript) - (unsigned char *)info);
	g_pSM->LogMessage(myself, "bLoadedHudElements: %i", (unsigned char *)&(info->bLoadedHudElements) - (unsigned char *)info);
	g_pSM->LogMessage(myself, "szClassName: %i", (unsigned char *)&(info->szClassName) - (unsigned char *)info);
	g_pSM->LogMessage(myself, "szPrintName: %i", (unsigned char *)&(info->szPrintName) - (unsigned char *)info);
	g_pSM->LogMessage(myself, "szViewModel: %i", (unsigned char *)&(info->szViewModel) - (unsigned char *)info);
	g_pSM->LogMessage(myself, "szWorldModel: %i", (unsigned char *)&(info->szWorldModel) - (unsigned char *)info);
	g_pSM->LogMessage(myself, "szAnimationPrefix: %i", (unsigned char *)&(info->szAnimationPrefix) - (unsigned char *)info);
	g_pSM->LogMessage(myself, "iSlot: %i", (unsigned char *)&(info->iSlot) - (unsigned char *)info);
	g_pSM->LogMessage(myself, "iPosition: %i", (unsigned char *)&(info->iPosition) - (unsigned char *)info);
	g_pSM->LogMessage(myself, "iMaxClip1: %i", (unsigned char *)&(info->iMaxClip1) - (unsigned char *)info);
	g_pSM->LogMessage(myself, "iMaxClip2: %i", (unsigned char *)&(info->iMaxClip2) - (unsigned char *)info);
	g_pSM->LogMessage(myself, "iDefaultClip1: %i", (unsigned char *)&(info->iDefaultClip1) - (unsigned char *)info);
	g_pSM->LogMessage(myself, "iDefaultClip2: %i", (unsigned char *)&(info->iDefaultClip2) - (unsigned char *)info);
	g_pSM->LogMessage(myself, "iWeight: %i", (unsigned char *)&(info->iWeight) - (unsigned char *)info);
	g_pSM->LogMessage(myself, "iRumbleEffect: %i", (unsigned char *)&(info->iRumbleEffect) - (unsigned char *)info);
	g_pSM->LogMessage(myself, "bAutoSwitchTo: %i", (unsigned char *)&(info->bAutoSwitchTo) - (unsigned char *)info);
	g_pSM->LogMessage(myself, "bAutoSwitchFrom: %i", (unsigned char *)&(info->bAutoSwitchFrom) - (unsigned char *)info);
	g_pSM->LogMessage(myself, "iFlags: %i", (unsigned char *)&(info->iFlags) - (unsigned char *)info);
	g_pSM->LogMessage(myself, "szAmmo1: %i", (unsigned char *)&(info->szAmmo1) - (unsigned char *)info);
	g_pSM->LogMessage(myself, "szAmmo2: %i", (unsigned char *)&(info->szAmmo2) - (unsigned char *)info);
	g_pSM->LogMessage(myself, "aShootSounds: %i", (unsigned char *)&(info->aShootSounds) - (unsigned char *)info);
	g_pSM->LogMessage(myself, "iAmmoType: %i", (unsigned char *)&(info->iAmmoType) - (unsigned char *)info);
	g_pSM->LogMessage(myself, "iAmmo2Type: %i", (unsigned char *)&(info->iAmmo2Type) - (unsigned char *)info);
	g_pSM->LogMessage(myself, "m_bMeleeWeapon: %i", (unsigned char *)&(info->m_bMeleeWeapon) - (unsigned char *)info);
	g_pSM->LogMessage(myself, "m_bBuiltRightHanded: %i", (unsigned char *)&(info->m_bBuiltRightHanded) - (unsigned char *)info);
	g_pSM->LogMessage(myself, "m_bAllowFlipping: %i", (unsigned char *)&(info->m_bAllowFlipping) - (unsigned char *)info);
	*/
	return g_StructManager.CreateStructHandle("FileWeaponInfo_t", info);
};

const sp_nativeinfo_t MyNatives[] = 
{
	{"GetStructInt",	GetStructInt},
	{"SetStructInt",	SetStructInt},
	{"GetStructFloat",	GetStructFloat},
	{"SetStructFloat",	SetStructFloat},
	{"GetStructVector",	GetStructVector},
	{"SetStructVector",	SetStructVector},
	{"GetStructString",	GetStructString},
	{"SetStructString",	SetStructString},
	{"GetStructEnt",	GetStructEnt},
	{"SetStructEnt",	SetStructEnt},
	{"GetWeaponStruct",	GetWeaponStruct},
	{NULL,				NULL},
};
