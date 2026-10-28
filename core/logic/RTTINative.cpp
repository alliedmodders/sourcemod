/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2026 AlliedModders LLC.  All rights reserved.
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
 */

#include "RTTINative.h"

#include <stdint.h>
#include <string.h>
#include <string>
#include <utility>
#include <vector>

#include <IPluginSys.h>
#include <smx/smx-legacy-typeinfo.h>
#include <smx/smx-typeinfo.h>
#include <sourcepawn/vm/base-runtime.h>
#include <sourcepawn/vm/environment.h>
#include <sourcepawn/vm/rtti.h>
#include <sourcepawn/vm/smx-image.h>
#include <sourcepawn/vm/v2/runtime.h>

#include "Logger.h"
#include "Native.h"
#include "PluginSys.h"
#include "ShareSys.h"
#include "common_logic.h"

using namespace SourceMod;
using namespace SourcePawn;

extern sp::Environment *g_pPawnEnv;

namespace {

enum class RTTIParamKind
{
	Cell,
	CellByRef,
	Int64,
	Int64ByRef,
	StringConst,
	StringMutable,
	Array
};

struct RTTIParamDesc
{
	RTTIParamKind kind = RTTIParamKind::Cell;
	bool is_const = false;
	uint32_t fixed_size = 0;
	uint32_t element_cells = 1;
};

struct RTTINativeSignature
{
	bool is_legacy_nativecall = false;
	bool is_variadic = false;
	bool returns_int64 = false;
	std::vector<RTTIParamDesc> params;
};

struct LegacyCallerAddressSlots
{
	bool found = false;
	bool caller_has_return = false;
	uint32_t caller_argc = 0;
	bool return_is_address = false;
	bool param_is_address[SP_MAX_EXEC_PARAMS] = {};
};

struct Int64Copyback
{
	void *caller_phys;
	int64_t *callee_phys;
	bool caller_is_cell32;
	bool is_return_slot;
};

static bool UsesInt64Address(IPluginContext *pContext)
{
	return pContext->GetRuntime()->FindPubvarByName("__Int64_Address__", nullptr) == SP_ERROR_NONE;
}

static bool IsNamedEnumType(sp::SmxImage *image, uint32_t enum_index, const char *target_name)
{
	if (!image || !image->rtti_enums() || enum_index >= image->rtti_enums()->row_count)
		return false;
	const sp::smx_rtti_enum *en = image->getRttiRow<sp::smx_rtti_enum>(image->rtti_enums(), enum_index);
	return en && strcmp(image->names() + en->name, target_name) == 0;
}

static uint32_t ComputeEnumStructCells(sp::BaseRuntime *runtime, sp::SmxImage *image, uint32_t idx)
{
	if (!image)
		return 0;

	if (const sp::smx_rtti_table_header *es_table = image->findRttiSection("rtti.enumstructs"))
	{
		if (idx < es_table->row_count)
			return image->getRttiRow<sp::smx_rtti_enumstruct>(es_table, idx)->size;
	}

	if (runtime)
	{
		if (sp::v2::Runtime *v2_rt = runtime->AsV2())
		{
			if (const sp::smx_rtti_classdef *classdef = image->getClassdef(idx))
			{
				if (const sp::TypeDesc *td = v2_rt->GetClassdefType(classdef))
					return td->slot_size() / sizeof(cell_t);
			}
		}
	}

	return 0;
}

static bool ParseSingleParam(sp::BaseRuntime *runtime,
                             sp::SmxImage *image,
                             sp::debug::FastRtti &parser,
                             RTTIParamDesc *out_param,
                             bool *out_is_handle_enum = nullptr,
                             bool *out_is_address_enum = nullptr)
{
	if (out_is_handle_enum)
		*out_is_handle_enum = false;
	if (out_is_address_enum)
		*out_is_address_enum = false;

	RTTIParamDesc desc;
	bool is_by_ref = false;
	uint8_t b = 0;

	for (;;)
	{
		if (!parser.GetByte(&b))
			return false;
		if (b == sp::cb::kConst)
		{
			desc.is_const = true;
			parser.NextByte();
			continue;
		}
		if (b == sp::cb::kByRef)
		{
			is_by_ref = true;
			parser.NextByte();
			continue;
		}
		break;
	}

	if (!parser.GetNextByte(&b))
		return false;

	switch (b)
	{
	case sp::cb::kBool:
	case sp::cb::kInt8:
	case sp::cb::kInt16:
	case sp::cb::kInt32:
	case sp::cb::kFloat32:
	case sp::cb::kChar8:
	case sp::cb::kAny:
	case sp::cb::kTopObject:
	case sp::cb::kTopFunction:
		desc.kind = is_by_ref ? RTTIParamKind::CellByRef : RTTIParamKind::Cell;
		break;

	case sp::cb::kInt64:
	case sp::cb::kIntPtr:
	case sp::cb::kFloat64:
		desc.kind = is_by_ref ? RTTIParamKind::Int64ByRef : RTTIParamKind::Int64;
		break;

	case sp::cb::kEnum:
	{
		uint32_t idx = 0;
		if (!parser.ReadUint32_Leb128(&idx))
			return false;
		if (out_is_handle_enum && !is_by_ref && IsNamedEnumType(image, idx, "Handle"))
			*out_is_handle_enum = true;
		if (out_is_address_enum && IsNamedEnumType(image, idx, "Address"))
			*out_is_address_enum = true;
		desc.kind = is_by_ref ? RTTIParamKind::CellByRef : RTTIParamKind::Cell;
		break;
	}

	case sp::cb::kTypeset:
	case sp::cb::kClass:
	case sp::cb::kFunctionPtr:
	{
		uint32_t idx = 0;
		if (!parser.ReadUint32_Leb128(&idx))
			return false;
		desc.kind = is_by_ref ? RTTIParamKind::CellByRef : RTTIParamKind::Cell;
		break;
	}

	case sp::cb::kEnumStruct:
	case sp::cb::kClassDef:
	{
		uint32_t idx = 0;
		if (!parser.ReadUint32_Leb128(&idx))
			return false;
		desc.kind = RTTIParamKind::Array;
		desc.fixed_size = ComputeEnumStructCells(runtime, image, idx);
		break;
	}

	case sp::cb::kArray:
	case sp::cb::kFixedArray:
	case sp::cb::kFlatArray:
	{
		uint32_t total_fixed_size = 0;
		bool has_fixed = false;

		uint8_t arr_op = b;
		for (;;)
		{
			if (arr_op == sp::cb::kFixedArray || arr_op == sp::cb::kFlatArray)
			{
				uint32_t dim_size = 0;
				if (!parser.ReadUint32_Leb128(&dim_size))
					return false;
				total_fixed_size = has_fixed ? (total_fixed_size * dim_size) : dim_size;
				has_fixed = true;
			}
			else
			{
				has_fixed = false;
				total_fixed_size = 0;
			}

			uint8_t inner_b = 0;
			if (!parser.GetByte(&inner_b))
				return false;
			if (inner_b == sp::cb::kConst)
			{
				desc.is_const = true;
				parser.NextByte();
				if (!parser.GetByte(&inner_b))
					return false;
			}

			if (inner_b == sp::cb::kArray || inner_b == sp::cb::kFixedArray || inner_b == sp::cb::kFlatArray)
			{
				parser.NextByte();
				arr_op = inner_b;
				continue;
			}
			break;
		}

		uint8_t elem_b = 0;
		if (!parser.GetNextByte(&elem_b))
			return false;
		if (elem_b == sp::cb::kConst)
		{
			desc.is_const = true;
			if (!parser.GetNextByte(&elem_b))
				return false;
		}

		desc.fixed_size = has_fixed ? total_fixed_size : 0;
		if (elem_b == sp::cb::kChar8)
		{
			desc.kind = desc.is_const ? RTTIParamKind::StringConst : RTTIParamKind::StringMutable;
		}
		else
		{
			desc.kind = RTTIParamKind::Array;
			if (elem_b == sp::cb::kInt64 || elem_b == sp::cb::kIntPtr || elem_b == sp::cb::kFloat64)
			{
				desc.element_cells = 2;
			}
			else if (elem_b == sp::cb::kEnumStruct || elem_b == sp::cb::kClassDef)
			{
				uint32_t idx = 0;
				if (!parser.ReadUint32_Leb128(&idx))
					return false;
				uint32_t es_cells = ComputeEnumStructCells(runtime, image, idx);
				desc.element_cells = es_cells ? es_cells : 1;
			}
			else if (elem_b == sp::cb::kEnum || elem_b == sp::cb::kTypeset ||
			         elem_b == sp::cb::kClass || elem_b == sp::cb::kFunctionPtr)
			{
				uint32_t idx = 0;
				if (!parser.ReadUint32_Leb128(&idx))
					return false;
				if (elem_b == sp::cb::kEnum && out_is_address_enum && IsNamedEnumType(image, idx, "Address"))
					*out_is_address_enum = true;
			}
		}
		break;
	}

	default:
		return false;
	}

	*out_param = desc;
	return true;
}

static bool ReadSignatureHeader(sp::BaseRuntime *runtime,
                                sp::SmxImage *image,
                                sp::debug::FastRtti &parser,
                                uint32_t *out_argc,
                                bool *out_is_variadic,
                                bool *out_has_return,
                                RTTIParamDesc *out_ret_desc,
                                bool *out_ret_is_address)
{
	if (!parser.ReadFunctionSignatureArgCount(out_argc))
		return false;

	uint8_t next_byte = 0;
	if (!parser.GetByte(&next_byte))
		return false;
	*out_is_variadic = (next_byte == sp::cb::kLegacyVariadic);
	if (*out_is_variadic)
		parser.NextByte();

	uint8_t ret_byte = 0;
	if (!parser.GetByte(&ret_byte))
		return false;
	if (ret_byte == sp::cb::kVoid)
	{
		parser.NextByte();
		*out_has_return = false;
		if (out_ret_is_address)
			*out_ret_is_address = false;
		return true;
	}

	*out_has_return = true;
	return ParseSingleParam(runtime, image, parser, out_ret_desc, nullptr, out_ret_is_address);
}

static bool HasExplicitFirstArgInDebugLocals(sp::SmxImage *image, const sp::smx_rtti_method *method)
{
	if (!image || !method || !image->rtti_methods())
		return false;

	const sp::smx_rtti_table_header *dbg_methods = image->findRttiSection(".dbg.methods");
	const sp::smx_rtti_table_header *dbg_locals = image->findRttiSection(".dbg.locals");
	if (!dbg_methods || !dbg_locals)
		return false;

	uint32_t method_index = image->GetIndexOfMethod(method);
	for (uint32_t i = 0; i < dbg_methods->row_count; i++)
	{
		const sp::smx_rtti_debug_method *dbg_method =
			image->getRttiRow<sp::smx_rtti_debug_method>(dbg_methods, i);
		if (!dbg_method || dbg_method->method_index != method_index)
			continue;

		uint32_t start = dbg_method->first_local;
		uint32_t end = dbg_locals->row_count;
		if (i + 1 < dbg_methods->row_count)
		{
			const sp::smx_rtti_debug_method *next_dbg =
				image->getRttiRow<sp::smx_rtti_debug_method>(dbg_methods, i + 1);
			if (next_dbg && next_dbg->first_local < end)
				end = next_dbg->first_local;
		}

		for (uint32_t j = start; j < end && j < dbg_locals->row_count; j++)
		{
			const sp::smx_rtti_debug_var *var =
				image->getRttiRow<sp::smx_rtti_debug_var>(dbg_locals, j);
			if (var && var->vclass == sp::kVarClass_Arg && var->address == -1)
				return true;
		}
		break;
	}
	return false;
}

static bool ParseRTTINativeSignature(sp::BaseRuntime *runtime,
                                     sp::SmxImage *image,
                                     const sp::smx_rtti_method *method,
                                     RTTINativeSignature *out_sig)
{
	if (!image || !method || !image->rttidata() || !out_sig)
		return false;

	*out_sig = RTTINativeSignature();
	sp::debug::FastRtti parser = image->GetTypeParser(method->signature);

	uint32_t argc = 0;
	bool has_return = false;
	RTTIParamDesc ret_desc;
	if (!ReadSignatureHeader(runtime, image, parser, &argc, &out_sig->is_variadic, &has_return, &ret_desc, nullptr))
		return false;

	out_sig->params.reserve(argc);
	bool first_param_is_handle = false;
	for (uint32_t i = 0; i < argc; i++)
	{
		RTTIParamDesc param_desc;
		bool is_handle = false;
		if (!ParseSingleParam(runtime, image, parser, &param_desc, (i == 0) ? &is_handle : nullptr))
			return false;
		if (i == 0)
			first_param_is_handle = is_handle;
		out_sig->params.push_back(param_desc);
	}

	// Functions returning int64 in SourcePawn 1.13+ have a void return type in RTTI and
	// take the caller's hidden return slot as their first by-reference argument (which is
	// omitted from .dbg.locals, shifting explicit arguments to start at address -2).
	if (!has_return &&
	    !out_sig->params.empty() &&
	    out_sig->params[0].kind == RTTIParamKind::Int64ByRef &&
	    !HasExplicitFirstArgInDebugLocals(image, method))
	{
		out_sig->returns_int64 = true;
	}

	if (!out_sig->is_variadic &&
	    has_return &&
	    ret_desc.kind == RTTIParamKind::Cell &&
	    out_sig->params.size() == 2 &&
	    first_param_is_handle &&
	    out_sig->params[0].kind == RTTIParamKind::Cell &&
	    out_sig->params[1].kind == RTTIParamKind::Cell)
	{
		out_sig->is_legacy_nativecall = true;
	}

	return true;
}

// In SourceMod <= 1.12, Address was declared as `enum Address`. Inspect the caller's
// `rtti.natives` entry for this native so we only bridge 32-bit <-> 64-bit values when
// the caller compiled that slot as `enum Address` rather than `int`.
static bool InspectLegacyCallerAddressSlots(IPluginContext *pCallerCtx,
                                            const char *native_name,
                                            LegacyCallerAddressSlots *out_slots)
{
	*out_slots = LegacyCallerAddressSlots();
	if (!pCallerCtx || !native_name)
		return false;

	sp::BaseRuntime *pRuntime = pCallerCtx->GetBaseRuntime();
	if (!pRuntime)
		return false;

	sp::SmxImage *image = pRuntime->image();
	if (!image || !image->HasRtti() || !image->rtti_enums() || !image->rttidata())
		return false;

	const sp::smx_rtti_table_header *rtti_natives = image->findRttiSection("rtti.natives");
	if (!rtti_natives)
		return false;

	const sp::smx_rtti_native *ntv_row = nullptr;
	uint32_t ntv_idx = 0;
	if (pRuntime->FindNativeByName(native_name, &ntv_idx) == SP_ERROR_NONE &&
	    ntv_idx < rtti_natives->row_count)
	{
		const sp::smx_rtti_native *candidate = image->getRttiRow<sp::smx_rtti_native>(rtti_natives, ntv_idx);
		if (candidate && strcmp(image->names() + candidate->name, native_name) == 0)
			ntv_row = candidate;
	}
	if (!ntv_row)
	{
		for (uint32_t i = 0; i < rtti_natives->row_count; i++)
		{
			const sp::smx_rtti_native *candidate = image->getRttiRow<sp::smx_rtti_native>(rtti_natives, i);
			if (candidate && strcmp(image->names() + candidate->name, native_name) == 0)
			{
				ntv_row = candidate;
				break;
			}
		}
	}
	if (!ntv_row)
		return false;

	sp::debug::FastRtti parser = image->GetTypeParser(ntv_row->signature);
	uint32_t argc = 0;
	bool is_variadic = false;
	bool has_return = false;
	RTTIParamDesc ret_desc;
	if (!ReadSignatureHeader(pRuntime, image, parser, &argc, &is_variadic, &has_return, &ret_desc, &out_slots->return_is_address))
		return false;

	for (uint32_t i = 0; i < argc && i < SP_MAX_EXEC_PARAMS; i++)
	{
		RTTIParamDesc param_desc;
		if (!ParseSingleParam(pRuntime, image, parser, &param_desc, nullptr, &out_slots->param_is_address[i]))
			return false;
	}

	out_slots->found = true;
	out_slots->caller_has_return = has_return;
	out_slots->caller_argc = argc;
	return true;
}

static bool AllocTempInt64Slot(IPluginContext *pCallerCtx,
                               IPluginFunction *pTargetFn,
                               int64_t initial_val,
                               uint32_t *out_local,
                               int64_t **out_phys)
{
	uint32_t slot_local = g_pPawnEnv->sp();
	if (!g_pPawnEnv->addStack(sizeof(int64_t)))
	{
		pTargetFn->Cancel();
		pCallerCtx->ThrowNativeErrorEx(SP_ERROR_STACKLOW, "Out of stack space marshaling int64 reference");
		return false;
	}
	int64_t *slot_phys = g_pPawnEnv->heap().ToPhysAddr<int64_t *>(slot_local);
	memcpy(slot_phys, &initial_val, sizeof(int64_t));
	*out_local = slot_local;
	*out_phys = slot_phys;
	return true;
}

static cell_t InvokeRTTINative(IPluginContext *pCallerCtx,
                               const cell_t *params,
                               FakeNative *pNative,
                               const RTTINativeSignature &sig)
{
	if (!pNative)
		return pCallerCtx->ThrowNativeError("Invalid RTTI native descriptor");

	if (params[0] > SP_MAX_EXEC_PARAMS)
		return pCallerCtx->ThrowNativeError("Called native with too many parameters (%d>%d)", params[0], SP_MAX_EXEC_PARAMS);

	if (pNative->ctx->GetRuntime()->IsPaused())
		return pCallerCtx->ThrowNativeError("Plugin owning this native is currently paused.");

	bool caller_int64_addr = UsesInt64Address(pCallerCtx);
	LegacyCallerAddressSlots legacy_addr_slots;
	if (!caller_int64_addr)
		InspectLegacyCallerAddressSlots(pCallerCtx, pNative->name.c_str(), &legacy_addr_slots);

	bool effective_returns_int64 = sig.returns_int64;
	if (effective_returns_int64 &&
	    !caller_int64_addr &&
	    legacy_addr_slots.found &&
	    !legacy_addr_slots.caller_has_return &&
	    legacy_addr_slots.caller_argc == sig.params.size())
	{
		effective_returns_int64 = false;
	}

	cell_t scope_offset = 0;
	if (effective_returns_int64)
	{
		if (caller_int64_addr)
		{
			scope_offset = 1;
		}
		else if (!legacy_addr_slots.return_is_address)
		{
			return pCallerCtx->ThrowNativeError("Native '%s' signature mismatch: caller expects 32-bit cell, provider uses int64",
			                                    pNative->name.c_str());
		}
	}

	ScopedFakeNativeCall fakeScope(pNative, pCallerCtx, params, scope_offset);

	IPluginFunction *pTargetFn = pNative->call;
	size_t paramIdx = 1;
	size_t passedCount = static_cast<size_t>(params[0]);

	Int64Copyback i64_copybacks[SP_MAX_EXEC_PARAMS];
	size_t num_i64_copybacks = 0;

	ke::SaveRestore<uint32_t> save_sp(g_pPawnEnv->sp());

	for (size_t i = 0; i < sig.params.size(); i++)
	{
		const RTTIParamDesc &desc = sig.params[i];

		// If a pre-1.13 caller declared the return type as `enum Address` and the provider returns
		// `Address` via hidden arg 0, allocate a temporary 64-bit return slot for the callee.
		if (i == 0 && effective_returns_int64 && !caller_int64_addr)
		{
			uint32_t slot_local = 0;
			int64_t *slot_phys = nullptr;
			if (!AllocTempInt64Slot(pCallerCtx, pTargetFn, 0, &slot_local, &slot_phys))
				return 0;
			i64_copybacks[num_i64_copybacks++] = { nullptr, slot_phys, true, true };
			pTargetFn->PushCell(static_cast<cell_t>(slot_local));
			continue;
		}

		if (paramIdx > passedCount)
		{
			pTargetFn->Cancel();
			size_t expected = effective_returns_int64 ? (sig.params.size() - 1) : sig.params.size();
			size_t provided = (passedCount >= static_cast<size_t>(scope_offset)) ? (passedCount - scope_offset) : 0;
			return pCallerCtx->ThrowNativeError("Native \"%s\" expected %zu parameters, but only %zu were provided",
			                                    pNative->name.c_str(), expected, provided);
		}

		size_t caller_param_idx = effective_returns_int64 ? (i - 1) : i;
		bool legacy_slot_is_addr = (!caller_int64_addr &&
		                            caller_param_idx < SP_MAX_EXEC_PARAMS &&
		                            legacy_addr_slots.param_is_address[caller_param_idx]);

		switch (desc.kind)
		{
		case RTTIParamKind::Cell:
		{
			pTargetFn->PushCell(params[paramIdx++]);
			break;
		}

		case RTTIParamKind::CellByRef:
		{
			cell_t *pAddr = nullptr;
			int err = pCallerCtx->LocalToPhysAddr(params[paramIdx++], &pAddr);
			if (err != SP_ERROR_NONE)
			{
				pTargetFn->Cancel();
				return pCallerCtx->ThrowNativeErrorEx(err, "Invalid byref address (%d)", err);
			}
			pTargetFn->PushCellByRef(pAddr, desc.is_const ? 0 : SM_PARAM_COPYBACK);
			break;
		}

		case RTTIParamKind::Int64:
		{
			if (!caller_int64_addr)
			{
				if (!legacy_slot_is_addr)
				{
					pTargetFn->Cancel();
					return pCallerCtx->ThrowNativeError("Native '%s' signature mismatch: caller expects 32-bit cell, provider uses int64",
					                                    pNative->name.c_str());
				}
				int64_t val = static_cast<int64_t>(static_cast<ucell_t>(params[paramIdx++]));
				pTargetFn->PushInt64(val);
				break;
			}

			cell_t *pAddr = nullptr;
			int err = pCallerCtx->LocalToPhysAddr(params[paramIdx++], &pAddr);
			if (err != SP_ERROR_NONE)
			{
				pTargetFn->Cancel();
				return pCallerCtx->ThrowNativeErrorEx(err, "Invalid int64 address (%d)", err);
			}
			int64_t val = 0;
			memcpy(&val, pAddr, sizeof(int64_t));
			pTargetFn->PushInt64(val);
			break;
		}

		case RTTIParamKind::Int64ByRef:
		{
			if (!caller_int64_addr && !legacy_slot_is_addr)
			{
				pTargetFn->Cancel();
				return pCallerCtx->ThrowNativeError("Native '%s' signature mismatch: caller expects 32-bit cell, provider uses int64",
				                                    pNative->name.c_str());
			}

			int64_t init_val = 0;
			void *caller_phys = nullptr;
			bool caller_is_cell32 = false;

			if (!caller_int64_addr)
			{
				cell_t *pAddr = nullptr;
				if (!desc.is_const && pCallerCtx->LocalToPhysAddr(params[paramIdx], &pAddr) == SP_ERROR_NONE)
				{
					paramIdx++;
					init_val = static_cast<int64_t>(static_cast<ucell_t>(*pAddr));
					caller_phys = pAddr;
					caller_is_cell32 = true;
				}
				else
				{
					init_val = static_cast<int64_t>(static_cast<ucell_t>(params[paramIdx++]));
				}
			}
			else
			{
				cell_t *pAddr = nullptr;
				int err = pCallerCtx->LocalToPhysAddr(params[paramIdx++], &pAddr);
				if (err != SP_ERROR_NONE)
				{
					pTargetFn->Cancel();
					return pCallerCtx->ThrowNativeErrorEx(err, "Invalid int64 byref address (%d)", err);
				}
				memcpy(&init_val, pAddr, sizeof(int64_t));
				if (!desc.is_const)
					caller_phys = pAddr;
			}

			uint32_t slot_local = 0;
			int64_t *slot_phys = nullptr;
			if (!AllocTempInt64Slot(pCallerCtx, pTargetFn, init_val, &slot_local, &slot_phys))
				return 0;

			if ((caller_phys || (i == 0 && effective_returns_int64)) && num_i64_copybacks < SP_MAX_EXEC_PARAMS)
			{
				i64_copybacks[num_i64_copybacks++] = {
					caller_phys,
					slot_phys,
					caller_is_cell32,
					(i == 0 && effective_returns_int64)
				};
			}

			pTargetFn->PushCell(static_cast<cell_t>(slot_local));
			break;
		}

		case RTTIParamKind::StringConst:
		{
			char *str = nullptr;
			int err = pCallerCtx->LocalToString(params[paramIdx++], &str);
			if (err != SP_ERROR_NONE)
			{
				pTargetFn->Cancel();
				return pCallerCtx->ThrowNativeErrorEx(err, "Invalid string address (%d)", err);
			}
			pTargetFn->PushString(str);
			break;
		}

		case RTTIParamKind::StringMutable:
		{
			char *str = nullptr;
			int err = pCallerCtx->LocalToString(params[paramIdx++], &str);
			if (err != SP_ERROR_NONE)
			{
				pTargetFn->Cancel();
				return pCallerCtx->ThrowNativeErrorEx(err, "Invalid mutable string address (%d)", err);
			}

			size_t maxLen = desc.fixed_size;
			if (maxLen == 0)
			{
				if (i + 1 < sig.params.size() &&
				    sig.params[i + 1].kind == RTTIParamKind::Cell &&
				    paramIdx <= passedCount &&
				    params[paramIdx] > 0 &&
				    params[paramIdx] <= 65536)
				{
					maxLen = static_cast<size_t>(params[paramIdx]);
				}
				else
				{
					maxLen = strlen(str) + 1;
				}
			}

			pTargetFn->PushStringEx(str, maxLen, SM_PARAM_STRING_COPY, SM_PARAM_COPYBACK);
			break;
		}

		case RTTIParamKind::Array:
		{
			cell_t *pArray = nullptr;
			int err = pCallerCtx->LocalToPhysAddr(params[paramIdx++], &pArray);
			if (err != SP_ERROR_NONE)
			{
				pTargetFn->Cancel();
				return pCallerCtx->ThrowNativeErrorEx(err, "Invalid array address (%d)", err);
			}

			uint32_t elemCount = desc.fixed_size;
			if (elemCount == 0)
			{
				if (i + 1 < sig.params.size() &&
				    sig.params[i + 1].kind == RTTIParamKind::Cell &&
				    paramIdx <= passedCount &&
				    params[paramIdx] >= 0 &&
				    params[paramIdx] <= 65536)
				{
					elemCount = static_cast<uint32_t>(params[paramIdx]);
				}
				else
				{
					elemCount = 1;
				}
			}

			uint32_t cellCount = elemCount * desc.element_cells;
			int copyback = desc.is_const ? 0 : SM_PARAM_COPYBACK;
			pTargetFn->PushArray(pArray, cellCount, copyback);
			break;
		}
		}
	}

	// Variadic functions expect a trailing argument pointing to `{ count, arg0, arg1, ... }`.
	if (sig.is_variadic)
	{
		uint32_t vararg_count = (passedCount >= paramIdx) ? static_cast<uint32_t>(passedCount - paramIdx + 1) : 0;
		uint32_t pkg_local = g_pPawnEnv->sp();
		if (!g_pPawnEnv->addStack((vararg_count + 1) * sizeof(cell_t)))
		{
			pTargetFn->Cancel();
			return pCallerCtx->ThrowNativeErrorEx(SP_ERROR_STACKLOW, "Out of stack space marshaling variadic arguments");
		}
		cell_t *pkg_phys = g_pPawnEnv->heap().ToPhysAddr<cell_t *>(pkg_local);
		pkg_phys[0] = static_cast<cell_t>(vararg_count);
		for (uint32_t v = 0; v < vararg_count; v++)
		{
			pkg_phys[1 + v] = params[paramIdx + v];
		}
		pTargetFn->PushCell(static_cast<cell_t>(pkg_local));
	}

	cell_t result = 0;
	if (!pTargetFn->Invoke(&result))
	{
		return 0;
	}

	for (size_t i = 0; i < num_i64_copybacks; i++)
	{
		int64_t val = 0;
		memcpy(&val, i64_copybacks[i].callee_phys, sizeof(int64_t));
		if (i64_copybacks[i].caller_is_cell32 && static_cast<uint64_t>(val) > UINT32_MAX)
		{
			return pCallerCtx->ThrowNativeError("Native '%s' returned 64-bit Address 0x%llx that does not fit in a 32-bit cell",
			                                    pNative->name.c_str(), static_cast<unsigned long long>(val));
		}
		if (i64_copybacks[i].caller_phys)
		{
			if (i64_copybacks[i].caller_is_cell32)
				*reinterpret_cast<cell_t *>(i64_copybacks[i].caller_phys) = static_cast<cell_t>(val);
			else
				memcpy(i64_copybacks[i].caller_phys, &val, sizeof(int64_t));
		}
		if (i64_copybacks[i].is_return_slot)
		{
			result = static_cast<cell_t>(val);
		}
	}

	return result;
}

class RTTINativeCallback final : public INativeCallback
{
public:
	RTTINativeCallback(FakeNative *fake, RTTINativeSignature &&sig)
		: fake_(fake),
		  sig_(std::move(sig))
	{
	}

	void AddRef() override
	{
		refcount_++;
	}

	void Release() override
	{
		assert(refcount_ > 0);
		if (--refcount_ == 0)
			delete this;
	}

	int Invoke(IPluginContext *ctx, const cell_t *params) override
	{
		return InvokeRTTINative(ctx, params, fake_, sig_);
	}

private:
	size_t refcount_ = 0;
	FakeNative *fake_;
	RTTINativeSignature sig_;
};

static bool HasClassdefWithName(sp::SmxImage *image, const std::string &class_name)
{
	if (!image)
		return false;

	if (image->rtti_classdefs())
	{
		for (uint32_t i = 0; i < image->rtti_classdefs()->row_count; i++)
		{
			const sp::smx_rtti_classdef *cd = image->getClassdef(i);
			if (cd && class_name == (image->names() + cd->name))
				return true;
		}
	}

	if (image->rtti_enums())
	{
		for (uint32_t i = 0; i < image->rtti_enums()->row_count; i++)
		{
			const sp::smx_rtti_enum *en = image->getRttiRow<sp::smx_rtti_enum>(image->rtti_enums(), i);
			if (en && class_name == (image->names() + en->name))
				return true;
		}
	}

	return false;
}

} // anonymous namespace

void AutoRegisterPluginRTTINatives(CPlugin *pPlugin)
{
	if (!pPlugin)
		return;

	IPluginContext *pBase = pPlugin->GetBaseContext();
	if (!pBase)
		return;

	sp::BaseRuntime *pRuntime = pBase->GetBaseRuntime();
	if (!pRuntime)
		return;

	sp::SmxImage *image = pRuntime->image();
	if (!image || !image->HasRtti() || !image->rtti_methods())
		return;

	static constexpr const char *kNativePrefix = "__native_";
	static constexpr size_t kNativePrefixLen = 9;

	uint32_t numPublics = pRuntime->GetPublicsNum();
	for (uint32_t i = 0; i < numPublics; i++)
	{
		sp_public_t *pub = nullptr;
		if (pRuntime->GetPublicByIndex(i, &pub) != SP_ERROR_NONE || !pub || !pub->name)
			continue;

		if (strncmp(pub->name, kNativePrefix, kNativePrefixLen) != 0)
			continue;

		const char *strippedName = pub->name + kNativePrefixLen;
		if (*strippedName == '\0')
			continue;

		const sp::smx_rtti_method *rttiMethod = nullptr;
		if (auto methodIdx = image->FindRttiMethod(pub->name))
		{
			rttiMethod = image->GetMethod(*methodIdx);
		}
		else
		{
			rttiMethod = image->GetMethodRttiByOffset(pub->code_offs);
		}

		if (!rttiMethod)
		{
			g_Logger.LogError("[SM] Plugin \"%s\": Could not find RTTI method entry for \"%s\"",
			                  pPlugin->GetFilename(), pub->name);
			continue;
		}

		RTTINativeSignature sig;
		if (!ParseRTTINativeSignature(pRuntime, image, rttiMethod, &sig))
		{
			g_Logger.LogError("[SM] Plugin \"%s\": Failed to parse RTTI signature for \"%s\"",
			                  pPlugin->GetFilename(), pub->name);
			continue;
		}

		IPluginFunction *pFunc = pRuntime->GetFunctionById(pub->funcid);
		if (!pFunc)
			pFunc = pRuntime->GetFunctionByName(pub->name);
		if (!pFunc)
			continue;

		std::string nativeName = strippedName;
		const char *doubleUnderscore = strstr(strippedName, "__");
		if (doubleUnderscore && doubleUnderscore > strippedName && *(doubleUnderscore + 2) != '\0')
		{
			std::string clsName(strippedName, static_cast<size_t>(doubleUnderscore - strippedName));
			if (HasClassdefWithName(image, clsName))
			{
				nativeName = clsName + "." + (doubleUnderscore + 2);
			}
		}

		if (!pPlugin->AddFakeNative(pFunc, nativeName.c_str(), FakeNativeRouter))
		{
			g_Logger.LogError("[SM] Plugin \"%s\": Native \"%s\" is already registered; skipping RTTI auto-registration",
			                  pPlugin->GetFilename(), nativeName.c_str());
			continue;
		}

		if (!sig.is_legacy_nativecall)
		{
			if (ke::RefPtr<Native> entry = g_ShareSys.FindNative(nativeName.c_str()))
				entry->fake->wrapper = new RTTINativeCallback(entry->fake.get(), std::move(sig));
		}
	}
}
