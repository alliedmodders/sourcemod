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

#include <math.h>
#include <string>
#include <stdlib.h>
#include "common_logic.h"
#include "MersenneTwister.h"
#include <ISourceMod.h>
#include <IPluginSys.h>
#include <ITranslator.h>
#include <am-utility.h>
#include <am-float.h>
#include <am-vector.h>
#include <sm_stringhashmap.h>

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
	val = (float)floor(val + 0.5f);

	return static_cast<int>(val);
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

StringHashMap<IPluginFunction*> funcs;

class FunctionValidator :
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
		StringHashMap<IPluginFunction*>::iterator iter = funcs.iter();
		while (!iter.empty())
		{
			if (plugin->GetRuntime() == iter->value->GetParentRuntime())
				iter.erase();
			else
				iter.next();
		}
	}
};

static cell_t sm_AddVariable(IPluginContext *pCtx, const cell_t *params)
{
	char *var;
	pCtx->LocalToString(params[1], &var);
	if (strlen(var) != 2)
		pCtx->ReportError("Variable %s is too long(expected size of 2, got %i)", var, strlen(var));
	IPluginFunction *func = pCtx->GetFunctionById(static_cast<funcid_t>(params[2]));
	if (!func)
		pCtx->ReportError("Unable to find function.");
	if (funcs.contains(var))
	{
		pCtx->ReportError("Variable %s already exists... Sorry.", var);
		return 0;
	}
	else if (!funcs.insert(var, func))
		pCtx->ReportError("Unable to insert variable %s.", var);
	return 0;
}

enum Operators {
	Operator_None = -1,
	Operator_Add,
	Operator_Subtract,
	Operator_Multiply,
	Operator_Divide,
	Operator_Exponent,
};

inline void Operate(float &sum, float v2, Operators &_operator)
{
	switch (_operator)
	{
	case Operator_Add:
		sum = sum + v2;
		break;
	case Operator_Subtract:
		sum = sum - v2;
		break;
	case Operator_Multiply:
		sum = sum * v2;
		break;
	case Operator_Divide:
		if (fabs(v2) < 0.000001)
			return;
		sum = sum / v2;
		break;
	case Operator_Exponent:
		sum = pow(sum, v2);
		break;
	default:
		sum = v2;
		break;
	}
	_operator = Operator_None;
}

inline void OperateOnString(float &sum, std::string &sValue, Operators &_operator)
{
	if (sValue.length() == 0)
		return;
	Operate(sum, stof(sValue), _operator);
	sValue = "";
}

IPhraseCollection *GetPhrases(IPluginContext *pCtx)
{
	IPluginIterator *iter = pluginsys->GetPluginIterator();
	while (iter->MorePlugins())
	{
		IPlugin *plugin = iter->GetPlugin();
		IPluginContext *ctx = plugin->GetBaseContext();
		if (!ctx)
			return nullptr;
		if (ctx == pCtx)
			return plugin->GetPhrases();
		iter->NextPlugin();
	}
	return nullptr;
}

static cell_t sm_ParseFormula(IPluginContext *pCtx, const cell_t *params)
{
	char *formula;
	size_t bracket = 0;
	ke::Vector<float> sum;
	sum.resize(1);
	ke::Vector<Operators> _operator;
	_operator.resize(1);
	std::string sValue = "";
	pCtx->LocalToString(params[1], &formula);
	IPluginFunction *func = pCtx->GetFunctionById(static_cast<funcid_t>(params[2]));
	if (!func)
		pCtx->ReportError("Unable to find function.");
	IPhraseCollection *phrases = GetPhrases(pCtx);
	if (!phrases)
		pCtx->ReportError("Unable to find phrases.");
	for (size_t i = 0; i <= strlen(formula); i++)
	{
		switch (*(formula + i))
		{
		case '(':
			++bracket;
			if (bracket + 1 > sum.length())
			{
				assert(sum.resize(bracket + 1));
				assert(_operator.resize(bracket + 1));
			}
			sum[bracket] = 0.0f;
			_operator[bracket] = Operator_None;
			break;
		case ')':
			OperateOnString(sum[bracket], sValue, _operator[bracket]);
			assert(--bracket >= 0);
			sum[bracket] = sum[bracket + 1];
			break;
		case '\0':
			if (sValue.length() > 0)
				OperateOnString(sum[bracket], sValue, _operator[bracket]);
			break;
		case '+':
		case '-':
		case '*':
		case '/':
		case '^':
			OperateOnString(sum[bracket], sValue, _operator[bracket]);
			switch (*(formula + i))
			{
			case '+':
				_operator[bracket] = Operator_Add;
				break;
			case '-':
				_operator[bracket] = Operator_Subtract;
				break;
			case '*':
				_operator[bracket] = Operator_Multiply;
				break;
			case '/':
				_operator[bracket] = Operator_Divide;
				break;
			case '^':
				_operator[bracket] = Operator_Exponent;
				break;
			}
			break;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case '.':
			sValue.append((formula + i), 1);
			break;
		default:
			char variable[2];
			g_pSM->Format(variable, sizeof(variable), "%c", *(formula + i));
			IPluginFunction *vFunc = nullptr;
			if (!funcs.retrieve(variable, &func))
			{
				char trans[2048];
				std::string* str = new std::string(variable);
				void* ptr = { str };
				phrases->FormatString(trans, sizeof(trans), "Unknown Variable", &ptr, 1, nullptr, nullptr);
				func->PushString(trans);
				func->Invoke();
				delete str;
				return 0;
			}
			else
			{
				cell_t result;
				vFunc->PushString(variable);
				if (!vFunc->Invoke(&result))
					Operate(sum[bracket], sp_ctof(result), _operator[bracket]);
				else
				{
					char trans[2048];
					void* ptr = {};
					phrases->FormatString(trans, sizeof(trans), "Unknown Error", &ptr, 0, nullptr, nullptr);
					func->PushString(trans);
					func->Invoke();
					return 0;
				}
			}
		}
	}
	if (sValue.length() > 0)
		sum[0] = stof(sValue);
	return sp_ftoc(sum[0]);
}

#if 0
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
			if (val >= 0.0f)
			{
				val = floor(val);
			} else {
				val = ceil(val);
			}
			break;
		}
	default:
		{
			val = (float)floor(val + 0.5f);
			break;
		}
	}

	return static_cast<int>(val);
}
#endif

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
	{"AddVariable",		sm_AddVariable},
	{"ParseFormula",	sm_ParseFormula},
	{"GetURandomInt",	GetURandomInt},
	{"GetURandomFloat",	GetURandomFloat},
	{"SetURandomSeed",	SetURandomSeed},
	{NULL,				NULL}
};

