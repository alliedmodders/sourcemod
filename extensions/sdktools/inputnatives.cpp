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
#include "variant-t.h"
#include <datamap.h>
#include <sm_argbuffer.h>

ICallWrapper *g_pAcceptInput = NULL;

#define ENTINDEX_TO_CBASEENTITY(ref, buffer) \
	buffer = gamehelpers->ReferenceToEntity(ref); \
	if (!buffer) \
	{ \
		return pContext->ThrowNativeError("Entity %d (%d) is not a CBaseEntity", gamehelpers->ReferenceToIndex(ref), ref); \
	}


static cell_t AcceptEntityInput(IPluginContext *pContext, const cell_t *params)
{
	if (!g_pAcceptInput)
	{
		int offset;
		if (!g_pGameConf->GetOffset("AcceptInput", &offset))
		{
			return pContext->ThrowNativeError("\"AcceptEntityInput\" not supported by this mod");
		}

		PassInfo pass[6];
		pass[0].type = PassType_Basic;
		pass[0].flags = PASSFLAG_BYVAL;
		pass[0].size = sizeof(const char *);
		pass[1].type = pass[2].type = PassType_Basic;
		pass[1].flags = pass[2].flags = PASSFLAG_BYVAL;
		pass[1].size = pass[2].size = sizeof(CBaseEntity *);
		pass[3].type = PassType_Object;
		pass[3].flags = PASSFLAG_BYVAL|PASSFLAG_OCTOR|PASSFLAG_ODTOR|PASSFLAG_OASSIGNOP;
		pass[3].size = SIZEOF_VARIANT_T;
		pass[4].type = PassType_Basic;
		pass[4].flags = PASSFLAG_BYVAL;
		pass[4].size = sizeof(int);
		pass[5].type = PassType_Basic;
		pass[5].flags = PASSFLAG_BYVAL;
		pass[5].size = sizeof(bool);

		if (!(g_pAcceptInput=g_pBinTools->CreateVCall(offset, 0, 0, &pass[5], pass, 5)))
		{
			pContext->ThrowNativeError("\"AcceptEntityInput\" wrapper failed to initialized");
		}
	}

	CBaseEntity *pActivator, *pCaller, *pDest;

	char *inputname;
	ENTINDEX_TO_CBASEENTITY(params[1], pDest);
	pContext->LocalToString(params[2], &inputname);
	if (params[3] == -1)
	{
		pActivator = NULL;
	} else {
		ENTINDEX_TO_CBASEENTITY(params[3], pActivator);
	}
	if (params[4] == -1)
	{
		pCaller = NULL;
	} else {
		ENTINDEX_TO_CBASEENTITY(params[4], pCaller);
	}

	ArgBuffer<void*, const char*, CBaseEntity*, CBaseEntity*, decltype(g_Variant_t), int> vstk(pDest, inputname, pActivator, pCaller, g_Variant_t, params[5]);

	bool ret = false;
	g_pAcceptInput->Execute(vstk, &ret);

	_init_variant_t();

	return (ret) ? 1 : 0;
}

sp_nativeinfo_t g_EntInputNatives[] =
{
	{"AcceptEntityInput",			AcceptEntityInput},
	{NULL,							NULL},
};
