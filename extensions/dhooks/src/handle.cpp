#include "handle.hpp"
#include "globals.hpp"
#include "capsule.hpp"
#include "sdk_types.hpp"

#include <cstdlib>

namespace dhooks::globals {
SourceMod::HandleType_t param_return_handle;
}

namespace dhooks::handle {

void init() {
	SourceMod::HandleAccess security;
	globals::handlesys->InitAccessDefaults(nullptr, &security);
	// Do not allow cloning, the struct self-manage its handle
	security.access[SourceMod::HandleAccess_Clone] = HANDLE_RESTRICT_IDENTITY;
	globals::param_return_handle = globals::handlesys->CreateType("DHookParamReturn", nullptr, 0, nullptr, &security, globals::myself->GetIdentity(), nullptr);
}

ParamReturn::ParamReturn(const Capsule* capsule, GeneralRegister* generalregs, FloatRegister* floatregs, void* return_ptr) :
	_handle(globals::handlesys->CreateHandle(globals::param_return_handle, this, globals::myself->GetIdentity(), globals::myself->GetIdentity(), nullptr)),
	_capsule(capsule),
	_general_registers(generalregs),
	_float_registers(floatregs),
	_return(return_ptr) {
}

ParamReturn::~ParamReturn() {
	if (_handle != BAD_HANDLE) {
		globals::handlesys->FreeHandle(_handle, nullptr);
	}
}

// This is a helper function for very trivial types (that are held by only one register)
// If dhooks becomes more complex this logic must be entirely re-written
void* ParamReturn::_Get(size_t index) const {
	const auto& params = _capsule->GetParameters();
	if (params.size() <= index) {
		// What the hell
		std::abort();
	}

	const auto& variable = params[index];
	if (variable.reg_index) {
		AsmRegCode code = variable.reg_index.value();

		if (variable.reg_offset) {
			// The register already acts as a pointer
			return reinterpret_cast<void*>(static_cast<std::uintptr_t>(_general_registers[code]) + variable.reg_offset.value());
		} else {
			// The register holds the value, return a pointer to it
			return reinterpret_cast<void*>(&_general_registers[code]);
		}
	} else if (variable.float_reg_index) {
		AsmFloatRegCode code = variable.float_reg_index.value();

		if (variable.reg_offset) {
			// Floating registers can't be pointers
			std::abort();
		} else {
			// The register holds the value, return a pointer to it
			return reinterpret_cast<void*>(&_float_registers[code]);
		}
	} else {
		// What the hell again
		std::abort();
	}
}

inline ParamReturn* Get(SourcePawn::IPluginContext* context, const cell_t param) {
	SourceMod::HandleSecurity security;
	security.pOwner = globals::myself->GetIdentity();
	security.pIdentity = globals::myself->GetIdentity();
	SourceMod::Handle_t hndl = static_cast<SourceMod::Handle_t>(param);
	ParamReturn* obj = nullptr;
	SourceMod::HandleError chnderr = globals::handlesys->ReadHandle(hndl, globals::param_return_handle, &security, (void **)&obj);
	if (chnderr != SourceMod::HandleError_None) {
		context->ThrowNativeError("Invalid Handle %x (error %i: %s)", hndl, chnderr, globals::HandleErrorToString(chnderr));
		return nullptr;
	}
	return obj;
}

cell_t DHookReturn_GetReturn(SourcePawn::IPluginContext* context, const cell_t* params) {
	auto obj = Get(context, params[1]);
	if (obj == nullptr) {
		return 0;
	}

	const auto& ret = obj->GetCapsule()->GetReturn();
	switch (ret.dhook_type) {
		case sp::ReturnType_Int:
			return *obj->GetReturn<int>();
		case sp::ReturnType_Bool:
			return (*obj->GetReturn<bool>()) ? 1 : 0;
		case sp::ReturnType_CBaseEntity:
			return globals::gamehelpers->EntityToBCompatRef(*obj->GetReturn<CBaseEntity*>());
		case sp::ReturnType_Edict:
			return globals::gamehelpers->IndexOfEdict(*obj->GetReturn<edict_t*>());
		case sp::ReturnType_Float:
			return sp_ftoc(*obj->GetReturn<float>());
		default:
			return context->ThrowNativeError("Invalid param type (%i) to get", ret.dhook_type);
	}
	return 0;
}

cell_t DHookReturn_SetReturn(SourcePawn::IPluginContext* context, const cell_t* params) {
	auto obj = Get(context, params[1]);
	if (obj == nullptr) {
		return 0;
	}

	const auto& ret = obj->GetCapsule()->GetReturn();
	switch (ret.dhook_type) {
		case sp::ReturnType_Int:
			*obj->GetReturn<int>() = params[2];
			break;
		case sp::ReturnType_Bool:
			*obj->GetReturn<bool>() = params[2] != 0;
			break;
		case sp::ReturnType_CBaseEntity: {
			CBaseEntity* entity = globals::gamehelpers->ReferenceToEntity(params[2]);
			if (entity == nullptr) {
				return context->ThrowNativeError("Invalid entity index passed for return value");
			}
			*obj->GetReturn<CBaseEntity*>() = entity;
			break;
		}
		case sp::ReturnType_Edict: {
			sdk::edict_t* edict = reinterpret_cast<sdk::edict_t*>(globals::gamehelpers->EdictOfIndex(params[2]));
			if (edict == nullptr || edict->IsFree()) {
				return context->ThrowNativeError("Invalid entity index passed for return value");
			}
			*obj->GetReturn<sdk::edict_t*>() = edict;
			break;
		}
		case sp::ReturnType_Float:
			*obj->GetReturn<float>() = sp_ctof(params[2]);
			break;
		default:
			return context->ThrowNativeError("Invalid param type (%i) to get", ret.dhook_type);
	}
	return 0;
}

cell_t DHookReturn_GetReturnVector(SourcePawn::IPluginContext* context, const cell_t* params) {
	auto obj = Get(context, params[1]);
	if (obj == nullptr) {
		return 0;
	}

	cell_t *buffer;
	context->LocalToPhysAddr(params[2], &buffer);

	const auto& ret = obj->GetCapsule()->GetReturn();

	sdk::Vector* vector = nullptr;
	if (ret.dhook_type == sp::ReturnType_Vector) {
		vector = obj->GetReturn<sdk::Vector>();
	} else if(ret.dhook_type == sp::ReturnType_VectorPtr) {
		auto pvector = obj->GetReturn<sdk::Vector*>();
		if (*pvector == nullptr) {
			return context->ThrowNativeError("Vector pointer is null");
		}
		vector = *pvector;
	} else {
		return context->ThrowNativeError("Return type is not a vector type");
	}
	buffer[0] = sp_ftoc(vector->x);
	buffer[1] = sp_ftoc(vector->y);
	buffer[2] = sp_ftoc(vector->z);
	return 0;
}

static void FreeChangedVector(void* data) {
	delete (sdk::Vector*)data;
}
cell_t Native_SetReturnVector(SourcePawn::IPluginContext* context, const cell_t* params) {
	auto obj = Get(context, params[1]);
	if (obj == nullptr) {
		return 0;
	}

	cell_t *buffer;
	context->LocalToPhysAddr(params[2], &buffer);
	const auto& ret = obj->GetCapsule()->GetReturn();

	if (ret.dhook_type == sp::ReturnType_Vector) {
		*(obj->GetReturn<sdk::Vector>()) = sdk::Vector(sp_ctof(buffer[0]), sp_ctof(buffer[1]), sp_ctof(buffer[2]));
	}
	else if (ret.dhook_type == sp::ReturnType_VectorPtr) {
		// Lazily free the vector a frame later
		auto vector = new sdk::Vector(sp_ctof(buffer[0]), sp_ctof(buffer[1]), sp_ctof(buffer[2]));
		*(obj->GetReturn<sdk::Vector*>()) = vector;
		globals::sourcemod->AddFrameAction(FreeChangedVector, vector);
	} else {
		return context->ThrowNativeError("Return type is not a vector type");
	}
	return 0;
}

/*void foo() {
	{
	{"DHookGetParam",                   DHookParam_GetParam},
	{"DHookSetParam",                   DHookParam_SetParam},
	{"DHookGetParamVector",             DHookParam_GetParamVector},
	{"DHookSetParamVector",             DHookParam_SetParamVector},
	{"DHookGetParamString",             DHookParam_GetParamString},
	{"DHookSetParamString",             DHookParam_SetParamString},
	{"DHookGetParamObjectPtrVar",       DHookParam_GetParamObjectPtrVar},
	{"DHookSetParamObjectPtrVar",       DHookParam_SetParamObjectPtrVar},
	{"DHookGetParamObjectPtrVarVector", DHookParam_GetParamObjectPtrVarVector},
	{"DHookSetParamObjectPtrVarVector", DHookParam_SetParamObjectPtrVarVector},
	{"DHookGetParamObjectPtrString",    DHookParam_GetParamObjectPtrString},
	{"DHookIsNullParam",                DHookParam_IsNullParam}
	{"DHookParam.Get",                  DHookParam_GetParam},
	{"DHookParam.GetVector",            DHookParam_GetParamVector},
	{"DHookParam.GetString",            DHookParam_GetParamString},
	{"DHookParam.Set",                  DHookParam_SetParam},
	{"DHookParam.SetVector",            DHookParam_SetParamVector},
	{"DHookParam.SetString",            DHookParam_SetParamString},
	{"DHookParam.GetObjectVar",         DHookParam_GetParamObjectPtrVar},
	{"DHookParam.GetObjectVarVector",   DHookParam_GetParamObjectPtrVarVector},
	{"DHookParam.GetObjectVarString",   DHookParam_GetParamObjectPtrString},
	{"DHookParam.SetObjectVar",         DHookParam_SetParamObjectPtrVar},
	{"DHookParam.SetObjectVarVector",   DHookParam_SetParamObjectPtrVarVector},
	{"DHookParam.IsNull",               DHookParam_IsNullParam},

	{"DHookGetReturn",                  DHookReturn_GetReturn},
	{"DHookSetReturn",                  DHookReturn_SetReturn},
	{"DHookGetReturnVector",            DHookReturn_GetReturnVector},
	{"DHookSetReturnVector",            DHookReturn_SetReturnVector},
	{"DHookGetReturnString",            DHookReturn_GetReturnString},
	{"DHookSetReturnString",            DHookReturn_SetReturnString},
	{"DHookReturn.Value.get",           DHookReturn_GetReturn},
	{"DHookReturn.Value.set",           DHookReturn_SetReturn},
	{"DHookReturn.GetVector",           DHookReturn_GetReturnVector},
	{"DHookReturn.SetVector",           DHookReturn_SetReturnVector},
	{"DHookReturn.GetString",           DHookReturn_GetReturnString},
	{"DHookReturn.SetString",           DHookReturn_SetReturnString}
	}
}*/

}