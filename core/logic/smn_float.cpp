/**
 * vim: set ts=4 sw=4 tw=99 :
 * =============================================================================
 * SourceMod
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

#include <cmath>
#include <string.h>
#include <stdlib.h>
#include "common_logic.h"
#include "MersenneTwister.h"
#include <IPluginSys.h>
#include <am-utility.h>
#include <am-float.h>

/****************************************
*                                       *
* FLOATING POINT NATIVE IMPLEMENTATIONS *
*                                       *
****************************************/


static cell_t sm_float(IPluginContext *pCtx, const cell_t *params)
{
	float val = static_cast<float>(params[1]);

	return sp_ftoc(val);
}

static cell_t sm_FloatAbs(IPluginContext *pCtx, const cell_t *params)
{
	float val = sp_ctof(params[1]);
	val = (val >= 0.0f) ? val : -val;

	return sp_ftoc(val);
}

static cell_t sm_FloatAdd(IPluginContext *pCtx, const cell_t *params)
{
	float val = sp_ctof(params[1]) + sp_ctof(params[2]);

	return sp_ftoc(val);
}

static cell_t sm_FloatSub(IPluginContext *pCtx, const cell_t *params)
{
	float val = sp_ctof(params[1]) - sp_ctof(params[2]);

	return sp_ftoc(val);
}

static cell_t sm_FloatMul(IPluginContext *pCtx, const cell_t *params)
{
	float val = sp_ctof(params[1]) * sp_ctof(params[2]);

	return sp_ftoc(val);
}

static cell_t sm_FloatDiv(IPluginContext *pCtx, const cell_t *params)
{
	float val = sp_ctof(params[1]) / sp_ctof(params[2]);

	return sp_ftoc(val);
}

static cell_t sm_FloatGt(IPluginContext *pCtx, const cell_t *params)
{
	return !!(sp_ctof(params[1]) > sp_ctof(params[2]));
}

static cell_t sm_FloatGe(IPluginContext *pCtx, const cell_t *params)
{
	return !!(sp_ctof(params[1]) >= sp_ctof(params[2]));
}

static cell_t sm_FloatLt(IPluginContext *pCtx, const cell_t *params)
{
	return !!(sp_ctof(params[1]) < sp_ctof(params[2]));
}

static cell_t sm_FloatLe(IPluginContext *pCtx, const cell_t *params)
{
	return !!(sp_ctof(params[1]) <= sp_ctof(params[2]));
}

static cell_t sm_FloatEq(IPluginContext *pCtx, const cell_t *params)
{
	return !!(sp_ctof(params[1]) == sp_ctof(params[2]));
}

static cell_t sm_FloatNe(IPluginContext *pCtx, const cell_t *params)
{
	return !!(sp_ctof(params[1]) != sp_ctof(params[2]));
}

static cell_t sm_FloatNot(IPluginContext *pCtx, const cell_t *params)
{
	float val = sp_ctof(params[1]);
	if (ke::IsNaN(val))
		return 1;
	return val ? 0 : 1;
}

static cell_t sm_FloatCompare(IPluginContext *pCtx, const cell_t *params)
{
	float val1 = sp_ctof(params[1]);
	float val2 = sp_ctof(params[2]);

	if (val1 > val2)
	{
		return 1;
	} else if (val1 < val2) {
		return -1;
	}

	return 0;
}

static cell_t sm_Logarithm(IPluginContext *pCtx, const cell_t *params)
{
	float val = sp_ctof(params[1]);
	float base = sp_ctof(params[2]);

	if ((val <= 0.0f) || (base <= 0.0f))
	{
		return pCtx->ThrowNativeError("Cannot evaluate the logarithm of zero or a negative number (val:%f base:%f)", val, base);
	}
	if (base == 10.0f)
	{
		val = log10(val);
	} else {
		val = log(val) / log(base);
	}

	return sp_ftoc(val);
}

static cell_t sm_Exponential(IPluginContext *pCtx, const cell_t *params)
{
	float val = sp_ctof(params[1]);

	return sp_ftoc(exp(val));
}

static cell_t sm_Pow(IPluginContext *pCtx, const cell_t *params)
{
	float base = sp_ctof(params[1]);
	float exponent = sp_ctof(params[2]);

	return sp_ftoc(pow(base, exponent));
}

static cell_t sm_SquareRoot(IPluginContext *pCtx, const cell_t *params)
{
	float val = sp_ctof(params[1]);

	if (val < 0.0f)
	{
		return pCtx->ThrowNativeError("Cannot evaluate the square root of a negative number (val:%f)", val);
	}

	return sp_ftoc(sqrt(val));
}

static cell_t sm_RoundToNearest(IPluginContext *pCtx, const cell_t *params)
{
	float val = sp_ctof(params[1]);

	return (int)val;
}

static cell_t sm_RoundToFloor(IPluginContext *pCtx, const cell_t *params)
{
	float val = sp_ctof(params[1]);
	val = floor(val);

	return static_cast<int>(val);
}

static cell_t sm_RoundToCeil(IPluginContext *pCtx, const cell_t *params)
{
	float val = sp_ctof(params[1]);
	val = ceil(val);

	return static_cast<int>(val);
}

static cell_t sm_RoundToZero(IPluginContext *pCtx, const cell_t *params)
{
	float val = sp_ctof(params[1]);
	if (val >= 0.0f)
	{
		val = floor(val);
	} else {
		val = ceil(val);
	}

	return static_cast<int>(val);
}

static cell_t sm_FloatFraction(IPluginContext *pCtx, const cell_t *params)
{
	float val = sp_ctof(params[1]);
	val = val - floor(val);

	return sp_ftoc(val);
}

static cell_t sm_Sine(IPluginContext *pCtx, const cell_t *params)
{
	float val = sp_ctof(params[1]);
	val = sin(val);

	return sp_ftoc(val);
}

static cell_t sm_Cosine(IPluginContext *pCtx, const cell_t *params)
{
	float val = sp_ctof(params[1]);
	val = cos(val);

	return sp_ftoc(val);
}

static cell_t sm_Tangent(IPluginContext *pCtx, const cell_t *params)
{
	float val = sp_ctof(params[1]);
	val = tan(val);

	return sp_ftoc(val);
}

static cell_t sm_ArcSine(IPluginContext *pCtx, const cell_t *params)
{
	float val = sp_ctof(params[1]);
	val = asin(val);

	return sp_ftoc(val);
}

static cell_t sm_ArcCosine(IPluginContext *pCtx, const cell_t *params)
{
	float val = sp_ctof(params[1]);
	val = acos(val);

	return sp_ftoc(val);
}

static cell_t sm_ArcTangent(IPluginContext *pCtx, const cell_t *params)
{
	float val = sp_ctof(params[1]);
	val = atan(val);

	return sp_ftoc(val);
}

static cell_t sm_ArcTangent2(IPluginContext *pCtx, const cell_t *params)
{
	float val1 = sp_ctof(params[1]);
	float val2 = sp_ctof(params[2]);
	val1 = atan2(val1, val2);

	return sp_ftoc(val1);
}

class RandomHelpers :
	public SMGlobalClass,
	public IPluginsListener
{
public:
	void OnSourceModAllInitialized()
	{
		pluginsys->AddPluginsListener(this);
	}

	void OnSourceModShutdown()
	{
		pluginsys->RemovePluginsListener(this);
	}

	void OnPluginDestroyed(IPlugin *plugin)
	{
		MTRand *mtrand;
		if (plugin->GetProperty("core.logic.mtrand", (void**)&mtrand, true))
		{
			delete mtrand;
		}
	}
	
	MTRand *RandObjForPlugin(IPluginContext *ctx)
	{
		IPlugin *plugin = pluginsys->FindPluginByContext(ctx->GetContext());
		MTRand *mtrand;	
		if (!plugin->GetProperty("core.logic.mtrand", (void**)&mtrand))
		{
			mtrand = new MTRand();
			plugin->SetProperty("core.logic.mtrand", mtrand);
		}
		return mtrand;
	}
} s_RandHelpers;

static cell_t GetURandomInt(IPluginContext *ctx, const cell_t *params)
{
	MTRand *randobj = s_RandHelpers.RandObjForPlugin(ctx);
	/* Note the sign bit must be stripped off because cell_t is signed,
	 * and this guarantees a range of [0,max_int]
	 */
	return randobj->randInt() & 0x7FFFFFFF;
}

static cell_t GetURandomFloat(IPluginContext *ctx, const cell_t *params)
{
	MTRand *randobj = s_RandHelpers.RandObjForPlugin(ctx);
	return sp_ftoc((float)randobj->rand());
}

static cell_t SetURandomSeed(IPluginContext *ctx, const cell_t *params)
{
	MTRand *randobj = s_RandHelpers.RandObjForPlugin(ctx);
	cell_t *addr;
	ctx->LocalToPhysAddr(params[1], &addr);
	/* We're 32-bit only. */
	randobj->seed((MTRand::uint32*)addr, params[2]);
	return 1;
}

REGISTER_NATIVES(floatnatives)
{
	{"float",			sm_float},
	{"FloatMul",		sm_FloatMul},
	{"FloatDiv",		sm_FloatDiv},
	{"FloatAdd",		sm_FloatAdd},
	{"FloatSub",		sm_FloatSub},
	{"FloatFraction",	sm_FloatFraction},
	{"RoundToZero",		sm_RoundToZero},
	{"RoundToCeil",		sm_RoundToCeil},
	{"RoundToFloor",	sm_RoundToFloor},
	{"RoundToNearest",	sm_RoundToNearest},
	{"__FLOAT_GT__",	sm_FloatGt},
	{"__FLOAT_GE__",	sm_FloatGe},
	{"__FLOAT_LT__",	sm_FloatLt},
	{"__FLOAT_LE__",	sm_FloatLe},
	{"__FLOAT_EQ__",	sm_FloatEq},
	{"__FLOAT_NE__",	sm_FloatNe},
	{"__FLOAT_NOT__",	sm_FloatNot},
	{"FloatCompare",	sm_FloatCompare},
	{"SquareRoot",		sm_SquareRoot},
	{"Pow",				sm_Pow},
	{"Exponential",		sm_Exponential},
	{"Logarithm",		sm_Logarithm},
	{"Sine",			sm_Sine},
	{"Cosine",			sm_Cosine},
	{"Tangent",			sm_Tangent},
	{"FloatAbs",		sm_FloatAbs},
	{"ArcTangent",		sm_ArcTangent},
	{"ArcCosine",		sm_ArcCosine},
	{"ArcSine",			sm_ArcSine},
	{"ArcTangent2",		sm_ArcTangent2},
	{"GetURandomInt",	GetURandomInt},
	{"GetURandomFloat",	GetURandomFloat},
	{"SetURandomSeed",	SetURandomSeed},
	{NULL,				NULL}
};

