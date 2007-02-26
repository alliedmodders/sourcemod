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

#include "sm_globals.h"
#include "sourcemm_api.h"

static cell_t SetRandomSeed(IPluginContext *pContext, const cell_t *params)
{
	engrandom->SetSeed(params[1]);

	return 1;
}

static cell_t GetRandomFloat(IPluginContext *pContext, const cell_t *params)
{
	float fMin = sp_ctof(params[1]);
	float fMax = sp_ctof(params[2]);

	float fRandom = engrandom->RandomFloat(fMin, fMax);

	return sp_ftoc(fRandom);
}

static cell_t GetRandomInt(IPluginContext *pContext, const cell_t *params)
{
	return engrandom->RandomInt(params[1], params[2]);
}

REGISTER_NATIVES(halflifeNatives)
{
	{"SetRandomSeed",			SetRandomSeed},
	{"GetRandomFloat",			GetRandomFloat},
	{"GetRandomInt",			GetRandomInt},
	{NULL,						NULL},
};
