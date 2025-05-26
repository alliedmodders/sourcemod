/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod SDKTools Extension
 * Copyright (C) 2004-2017 AlliedModders LLC.  All rights reserved.
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
#include "variant-t.h"
#include <datamap.h>

variant_t g_Variant_t;
char g_Variant_str_Value[128];

// copy this definition as the original file includes cbase.h which explodes in a shower of compile errors
void variant_t::SetEntity( CBaseEntity *val ) 
{ 
	eVal = val;
	fieldType = FIELD_EHANDLE; 
}

#define ENTINDEX_TO_CBASEENTITY(ref, buffer) \
	buffer = gamehelpers->ReferenceToEntity(ref); \
	if (!buffer) \
	{ \
		return pContext->ThrowNativeError("Entity %d (%d) is not a CBaseEntity", gamehelpers->ReferenceToIndex(ref), ref); \
	}

static cell_t SetVariantBool(IPluginContext *pContext, const cell_t *params)
{
	g_Variant_t.SetBool((params[1]) ? true : false);
	return 1;
}

static cell_t SetVariantString(IPluginContext *pContext, const cell_t *params)
{
	char *str;
	pContext->LocalToString(params[1], &str);
	strncpy(g_Variant_str_Value, str, sizeof(g_Variant_str_Value));
	g_Variant_str_Value[sizeof(g_Variant_str_Value) - 1] = '\0';
	g_Variant_t.SetString(MAKE_STRING(g_Variant_str_Value));
	return 1;
}

static cell_t SetVariantInt(IPluginContext *pContext, const cell_t *params)
{
	g_Variant_t.SetInt(params[1]);
	return 1;
}

static cell_t SetVariantFloat(IPluginContext *pContext, const cell_t *params)
{
	g_Variant_t.SetFloat(sp_ctof(params[1]));
	return 1;
}

static cell_t SetVariantVector3D(IPluginContext *pContext, const cell_t *params)
{
	cell_t *val;
	pContext->LocalToPhysAddr(params[1], &val);

	Vector v(sp_ctof(val[0]), sp_ctof(val[1]), sp_ctof(val[2]));
	g_Variant_t.SetVector3D(v);
	return 1;
}

static cell_t SetVariantPosVector3D(IPluginContext *pContext, const cell_t *params)
{
	cell_t *val;
	pContext->LocalToPhysAddr(params[1], &val);

	Vector v(sp_ctof(val[0]), sp_ctof(val[1]), sp_ctof(val[2]));
	g_Variant_t.SetPositionVector3D(v);
	return 1;
}

static cell_t SetVariantColor(IPluginContext *pContext, const cell_t *params)
{
	cell_t *val;
	pContext->LocalToPhysAddr(params[1], &val);

	g_Variant_t.SetColor32(val[0], val[1], val[2], val[3]);
	return 1;
}

static cell_t SetVariantEntity(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity;
	ENTINDEX_TO_CBASEENTITY(params[1], pEntity);
	g_Variant_t.SetEntity(pEntity);

	return 1;
}

sp_nativeinfo_t g_VariantTNatives[] =
{
	{"SetVariantBool",				SetVariantBool},
	{"SetVariantString",			SetVariantString},
	{"SetVariantInt",				SetVariantInt},
	{"SetVariantFloat",				SetVariantFloat},
	{"SetVariantVector3D",			SetVariantVector3D},
	{"SetVariantPosVector3D",		SetVariantPosVector3D},
	{"SetVariantColor",				SetVariantColor},
	{"SetVariantEntity",			SetVariantEntity},
	{NULL,							NULL},
};
