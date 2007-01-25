#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "sm_autonatives.h"
#include "sp_vm_api.h"
#include "sp_typeutil.h"

using namespace SourcePawn;

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
	val = (val >= 0) ? val : -val;

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

	if ((val <= 0) || (base <= 0))
	{
		return pCtx->ThrowNativeError("Cannot evaluate the logarithm of zero or a negative number (val:%f base:%f).", val, base);
	}
	if (base == 10.0)
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

	if (val < 0.0)
	{
		return pCtx->ThrowNativeError("Cannot evaluate the square root of a negative number (val:%f).", val);
	}

	return sp_ftoc(sqrt(val));
}

static cell_t sm_FloatRound(IPluginContext *pCtx, const cell_t *params)
{
	float val = sp_ctof(params[1]);

	switch (params[2])
	{
	case 1:
		{
			val = floor(val);
			break;
		}
	case 2:
		{
			val = ceil(val);
			break;
		}
	case 3:
		{
			if (val >= 0.0)
			{
				val = floor(val);
			} else {
				val = ceil(val);
			}
			break;
		}
	default:
		{
			val = (float)floor(val + 0.5);
			break;
		}
	}

	return static_cast<int>(val);
}

static cell_t sm_FloatStr(IPluginContext *pCtx, const cell_t *params)
{
	char *str;

	pCtx->LocalToString(params[1], &str);
	if (!strlen(str))
	{
		return 0;
	}

	return sp_ftoc((float)atof(str));
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

REGISTER_NATIVES(floatnatives)
{
	{"float",			sm_float},
	{"FloatStr",		sm_FloatStr},
	{"FloatMul",		sm_FloatMul},
	{"FloatDiv",		sm_FloatDiv},
	{"FloatAdd",		sm_FloatAdd},
	{"FloatSub",		sm_FloatSub},
	{"FloatFraction",	sm_FloatFraction},
	{"FloatRound",		sm_FloatRound},
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
	{NULL,				NULL}
};
