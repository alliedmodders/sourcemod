/**
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

#include <time.h>
#include "sm_globals.h"
#include "sourcemod.h"

static cell_t ThrowError(IPluginContext *pContext, const cell_t *params)
{
	char buffer[512];

	g_SourceMod.FormatString(buffer, sizeof(buffer), pContext, params, 1);

	if (pContext->GetContext()->n_err == SP_ERROR_NONE)
	{
		pContext->ThrowNativeErrorEx(SP_ERROR_ABORTED, "%s", buffer);
	}

	return 0;
}

static cell_t GetTime(IPluginContext *pContext, const cell_t *params)
{
	time_t t = time(NULL);
	cell_t *addr;
	pContext->LocalToPhysAddr(params[1], &addr);

	*(time_t *)addr = t;

	return static_cast<cell_t>(t);
}

REGISTER_NATIVES(coreNatives)
{
	{"GetTime",				GetTime},
	{"ThrowError",			ThrowError},
	{NULL,					NULL},
};
