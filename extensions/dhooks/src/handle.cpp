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

inline cell_t GetParam(SourcePawn::IPluginContext* context, ParamReturn* obj, const cell_t param) {
	int index = param - 1;
	if (obj->GetCapsule()->GetCallConv() == sp::CallConv_THISCALL) {
		// Skip the |this| ptr parameter
		index += 1;
	}

	if (param <= 0 || index >= obj->GetCapsule()->GetParameters().size()) {
		context->ThrowNativeError("Invalid param number %i max params is %i", param, obj->GetCapsule()->GetParameters().size() - ((obj->GetCapsule()->GetCallConv() == sp::CallConv_THISCALL) ? 1 : 0));
		return -1;
	}
	return index;
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
cell_t DHookReturn_SetReturnVector(SourcePawn::IPluginContext* context, const cell_t* params) {
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

cell_t DHookReturn_GetReturnString(SourcePawn::IPluginContext* context, const cell_t* params) {
	auto obj = Get(context, params[1]);
	if (obj == nullptr) {
		return 0;
	}

	const auto& ret = obj->GetCapsule()->GetReturn();
	switch (ret.dhook_type) {
		case sp::ReturnType_String:
			context->StringToLocal(params[2], params[3], obj->GetReturn<sdk::string_t>()->ToCStr());
			return 1;
		case sp::ReturnType_StringPtr:
			context->StringToLocal(params[2], params[3], (obj->GetReturn<sdk::string_t*>() != nullptr) ? (*obj->GetReturn<sdk::string_t*>())->ToCStr() : "");
			return 1;
		case sp::ReturnType_CharPtr:
			context->StringToLocal(params[2], params[3], (*(obj->GetReturn<const char*>()) == nullptr) ? "" : *(obj->GetReturn<const char*>()));
			return 1;
		default:
			return context->ThrowNativeError("Invalid param type to get. Param is not a string.");
	}
}

static void FreeChangedCharPtr(void* data) {
	delete[] (const char*)data;
}
cell_t DHookReturn_SetReturnString(SourcePawn::IPluginContext* context, const cell_t* params) {
	auto obj = Get(context, params[1]);
	if (obj == nullptr) {
		return 0;
	}

	const auto& ret = obj->GetCapsule()->GetReturn();

	char *value;
	context->LocalToString(params[2], &value);

	switch (ret.dhook_type) {
		case sp::ReturnType_CharPtr: {
			auto new_str = new char[strlen(value) + 1];
			strcpy(new_str, value);
			*(obj->GetReturn<const char*>()) = new_str;
			// Free it later (cheaply) after the function returned.
			globals::sourcemod->AddFrameAction(FreeChangedCharPtr, new_str);
			return 1;
		}
		default:
			return context->ThrowNativeError("Invalid param type to get. Param is not a char pointer.");
	}
}

cell_t DHookParam_GetParam(SourcePawn::IPluginContext* context, const cell_t* params) {
	auto obj = Get(context, params[1]);
	if (obj == nullptr) {
		return 0;
	}

	if (params[2] == 0) {
		return obj->GetCapsule()->GetParameters().size();
	}

	int index = GetParam(context, obj, params[2]);
	if (index == -1) {
		return 0;
	}

	const auto& var = obj->GetCapsule()->GetParameters()[index];
	if (sp::HookParamTypeIsPtr(var.dhook_type) && *obj->Get<void*>(index) == nullptr) {
		return context->ThrowNativeError("Trying to get value for null pointer.");
	}

	switch(var.dhook_type) {
		case sp::HookParamType_Int:
			return *obj->Get<int>(index);
		case sp::HookParamType_Bool:
			return (*obj->Get<bool>(index)) ? 1 : 0;
		case sp::HookParamType_CBaseEntity:
			return globals::gamehelpers->EntityToBCompatRef(*obj->Get<CBaseEntity*>(index));
		case sp::HookParamType_Edict:
			return globals::gamehelpers->IndexOfEdict(*obj->Get<edict_t*>(index));
		case sp::HookParamType_Float:
			return sp_ftoc(*obj->Get<float>(index));
		default:
			return context->ThrowNativeError("Invalid param type (%i) to get", var.dhook_type);
	}
}

cell_t DHookParam_SetParam(SourcePawn::IPluginContext* context, const cell_t* params) {
	auto obj = Get(context, params[1]);
	if (obj == nullptr) {
		return 0;
	}

	int index = GetParam(context, obj, params[2]);
	if (index == -1) {
		return 0;
	}

	const auto& var = obj->GetCapsule()->GetParameters()[index];
	switch (var.dhook_type) {
		case sp::HookParamType_Int:
			*obj->Get<int>(index) = params[3];
			break;
		case sp::HookParamType_Bool:
			*obj->Get<bool>(index) = (params[3] ? true : false);
			break;
		case sp::HookParamType_CBaseEntity: {
			CBaseEntity* entity = globals::gamehelpers->ReferenceToEntity(params[3]);

			if (params[3] != -1 && entity == nullptr) {
				return context->ThrowNativeError("Invalid entity reference passed for param value");
			}

			*obj->Get<CBaseEntity*>(index) = entity;
			break;
		}
		case sp::HookParamType_Edict: {
			sdk::edict_t* edict = reinterpret_cast<sdk::edict_t*>(globals::gamehelpers->EdictOfIndex(params[3]));

			if (params[3] != -1 && (edict == nullptr || edict->IsFree())) {
				return context->ThrowNativeError("Invalid edict index passed for param value");
			}

			*obj->Get<sdk::edict_t*>(index) = edict;
			break;
		}
		case sp::HookParamType_Float:
			*obj->Get<float>(index) = sp_ctof(params[3]);
			break;
		default:
			return context->ThrowNativeError("Invalid param type (%i) to set", var.dhook_type);
	}
	return 0;
}

cell_t DHookParam_GetParamVector(SourcePawn::IPluginContext* context, const cell_t* params) {
	auto obj = Get(context, params[1]);
	if (obj == nullptr) {
		return 0;
	}

	int index = GetParam(context, obj, params[2]);
	if (index == -1) {
		return 0;
	}

	const auto& var = obj->GetCapsule()->GetParameters()[index];
	if (var.dhook_type == sp::HookParamType_VectorPtr) {
		cell_t* buffer;
		context->LocalToPhysAddr(params[3], &buffer);
		
		auto vec = *obj->Get<sdk::Vector*>(index);
		if (vec == nullptr) {
			return context->ThrowNativeError("Trying to get value for null pointer.");
		}
		buffer[0] = sp_ftoc(vec->x);
		buffer[1] = sp_ftoc(vec->y);
		buffer[2] = sp_ftoc(vec->z);
	} else {
		return context->ThrowNativeError("Invalid param type to get. Param is not a vector.");
	}
	return 0;
}

cell_t DHookParam_SetParamVector(SourcePawn::IPluginContext* context, const cell_t* params) {
	auto obj = Get(context, params[1]);
	if (obj == nullptr) {
		return 0;
	}

	int index = GetParam(context, obj, params[2]);
	if (index == -1) {
		return 0;
	}

	const auto& var = obj->GetCapsule()->GetParameters()[index];
	if (var.dhook_type == sp::HookParamType_VectorPtr) {
		cell_t* buffer;
		context->LocalToPhysAddr(params[3], &buffer);
		
		auto vec = *obj->Get<sdk::Vector*>(index);
		if (vec == nullptr) {
			vec = new sdk::Vector;
			globals::sourcemod->AddFrameAction(FreeChangedVector, vec);
			*obj->Get<sdk::Vector*>(index) = vec;
		}

		vec->x = sp_ctof(buffer[0]);
		vec->y = sp_ctof(buffer[1]);
		vec->z = sp_ctof(buffer[2]);
	} else {
		return context->ThrowNativeError("Invalid param type to set. Param is not a vector.");
	}
	return 0;
}

cell_t DHookParam_GetParamString(SourcePawn::IPluginContext* context, const cell_t* params) {
	auto obj = Get(context, params[1]);
	if (obj == nullptr) {
		return 0;
	}

	int index = GetParam(context, obj, params[2]);
	if (index == -1) {
		return 0;
	}

	const auto& var = obj->GetCapsule()->GetParameters()[index];
	if (sp::HookParamTypeIsPtr(var.dhook_type) && *obj->Get<void*>(index) == nullptr) {
		return context->ThrowNativeError("Trying to get value for null pointer.");
	}

	switch (var.dhook_type) {
		case sp::HookParamType_CharPtr: {
			auto str = *obj->Get<const char*>(index);
			context->StringToLocal(params[3], params[4], str);
			break;
		}
		case sp::HookParamType_String: {
			auto str = *obj->Get<sdk::string_t>(index);
			context->StringToLocal(params[3], params[4], str.ToCStr());
			break;
		}
		case sp::HookParamType_StringPtr: {
			auto str = *obj->Get<sdk::string_t*>(index);
			context->StringToLocal(params[3], params[4], str->ToCStr());
			break;
		}
		default:
			return context->ThrowNativeError("Invalid param type to get. Param is not a string.");
	}

	return 0;
}

cell_t DHookParam_SetParamString(SourcePawn::IPluginContext* context, const cell_t* params) {
	auto obj = Get(context, params[1]);
	if (obj == nullptr) {
		return 0;
	}

	int index = GetParam(context, obj, params[2]);
	if (index == -1) {
		return 0;
	}

	char* value;
	context->LocalToString(params[3], &value);

	const auto& var = obj->GetCapsule()->GetParameters()[index];
	if (var.dhook_type == sp::HookParamType_CharPtr) {
		auto str = new char[strlen(value) + 1];
		strcpy(str, value);
		*obj->Get<const char*>(index) = str;
		// Free it later (cheaply) after the function returned.
		globals::sourcemod->AddFrameAction(FreeChangedCharPtr, str);
	} else if (var.dhook_type == sp::HookParamType_String || var.dhook_type == sp::HookParamType_StringPtr) {
		return context->ThrowNativeError("DHooks currently lacks support for setting string_t. Pull requests are welcome!");
	} else {
		return context->ThrowNativeError("Invalid param type to set. Param is not a string.");
	}
	return 0;
}

cell_t Native_GetParamObjectPtrVar(SourcePawn::IPluginContext* context, const cell_t* params) {
	auto obj = Get(context, params[1]);
	if (obj == nullptr) {
		return 0;
	}

	int index = GetParam(context, obj, params[2]);
	if (index == -1) {
		return 0;
	}

	const auto& var = obj->GetCapsule()->GetParameters()[index];
	if (var.dhook_type != sp::HookParamType_ObjectPtr && var.dhook_type != sp::HookParamType_Object) {
		return context->ThrowNativeError("Invalid object value type %i", var.dhook_type);
	}

	std::uint8_t* object = obj->Get<std::uint8_t>(index);
	if (var.dhook_type == sp::HookParamType_ObjectPtr) {
		object = *(std::uint8_t**)object;
	}

	switch((sp::ObjectValueType)params[4]) {
		case sp::ObjectValueType_Int:
			return *(int *)(object + params[3]);
		case sp::ObjectValueType_Bool:
			return (*(bool *)(object + params[3])) ? 1 : 0;
		case sp::ObjectValueType_Ehandle: {
			auto edict = reinterpret_cast<sdk::edict_t*>(globals::gamehelpers->GetHandleEntity(*(CBaseHandle *)(object + params[3])));
			if (edict == nullptr || edict->IsFree()) {
				return -1;
			}
			return globals::gamehelpers->IndexOfEdict(reinterpret_cast<edict_t*>(edict));
		}
		case sp::ObjectValueType_EhandlePtr: {
			auto edict = reinterpret_cast<sdk::edict_t*>(globals::gamehelpers->GetHandleEntity(**(CBaseHandle **)(object + params[3])));
			if (edict == nullptr || edict->IsFree()) {
				return -1;
			}
			return globals::gamehelpers->IndexOfEdict(reinterpret_cast<edict_t*>(edict));
		}
		case sp::ObjectValueType_Float:
			return sp_ftoc(*(float *)(object + params[3]));
		// This is technically incorrect and doesn't follow the "convention" that CBaseEntity is always a pointer for plugins
		// Therefore this should be a pointer of pointer (i.e a triple pointer when offseted to it). But it isn't....
		case sp::ObjectValueType_CBaseEntityPtr:
			return globals::gamehelpers->EntityToBCompatRef(*(CBaseEntity **)(object + params[3]));
		case sp::ObjectValueType_IntPtr:
			return **(int **)(object + params[3]);
		case sp::ObjectValueType_BoolPtr:
			return (**(bool **)(object + params[3])) ? 1 : 0;
		case sp::ObjectValueType_FloatPtr:
			return sp_ftoc(**(float **)(object + params[3]));
		default:
			return context->ThrowNativeError("Invalid Object value type");
	}
}

cell_t DHookParam_SetParamObjectPtrVar(SourcePawn::IPluginContext* context, const cell_t* params) {
	auto obj = Get(context, params[1]);
	if (obj == nullptr) {
		return 0;
	}

	int index = GetParam(context, obj, params[2]);
	if (index == -1) {
		return 0;
	}

	const auto& var = obj->GetCapsule()->GetParameters()[index];
	if (var.dhook_type != sp::HookParamType_ObjectPtr && var.dhook_type != sp::HookParamType_Object) {
		return context->ThrowNativeError("Invalid object value type %i", var.dhook_type);
	}

	std::uint8_t* object = obj->Get<std::uint8_t>(index);
	if (var.dhook_type == sp::HookParamType_ObjectPtr) {
		object = *(std::uint8_t**)object;
	}

	switch((sp::ObjectValueType)params[4]) {
		case sp::ObjectValueType_Int:
			*(int *)(object + params[3]) = params[5];
			break;
		case sp::ObjectValueType_Bool:
			*(bool *)(object + params[3]) = params[5] != 0;
			break;
		case sp::ObjectValueType_Ehandle:
			globals::gamehelpers->SetHandleEntity(*(CBaseHandle *)(object + params[3]), globals::gamehelpers->EdictOfIndex(params[5]));
			break;
		case sp::ObjectValueType_EhandlePtr:
			globals::gamehelpers->SetHandleEntity(**(CBaseHandle **)(object + params[3]), globals::gamehelpers->EdictOfIndex(params[5]));
			break;
		case sp::ObjectValueType_Float:
			*(float *)(object + params[3]) = sp_ctof(params[5]);
			break;
		// This is technically incorrect and doesn't follow the "convention" that CBaseEntity is always a pointer for plugins
		// Therefore this should be a pointer of pointer (i.e a triple pointer when offseted to it). But it isn't....
		case sp::ObjectValueType_CBaseEntityPtr: {
			CBaseEntity* entity = globals::gamehelpers->ReferenceToEntity(params[5]);

			if (params[5] != -1 && entity == nullptr) {
				return context->ThrowNativeError("Invalid entity passed");
			}

			*(CBaseEntity **)(object + params[3]) = entity;
			break;
		}
		case sp::ObjectValueType_IntPtr:
			**(int **)(object + params[3]) = params[5];
			break;
		case sp::ObjectValueType_BoolPtr:
			**(int **)(object + params[3]) = params[5] != 0;
			break;
		case sp::ObjectValueType_FloatPtr:
			**(float **)(object + params[3]) = sp_ctof(params[5]);
			break;
		default:
			return context->ThrowNativeError("Invalid Object value type");
	}
	return 0;
}

cell_t DHookParam_GetParamObjectPtrVarVector(SourcePawn::IPluginContext* context, const cell_t* params) {
	auto obj = Get(context, params[1]);
	if (obj == nullptr) {
		return 0;
	}

	int index = GetParam(context, obj, params[2]);
	if (index == -1) {
		return 0;
	}

	const auto& var = obj->GetCapsule()->GetParameters()[index];
	if (var.dhook_type != sp::HookParamType_ObjectPtr && var.dhook_type != sp::HookParamType_Object) {
		return context->ThrowNativeError("Invalid object value type %i", var.dhook_type);
	}

	std::uint8_t* object = obj->Get<std::uint8_t>(index);
	if (var.dhook_type == sp::HookParamType_ObjectPtr) {
		object = *(std::uint8_t**)object;
	}

	cell_t *buffer;
	context->LocalToPhysAddr(params[5], &buffer);

	if ((sp::ObjectValueType)params[4] == sp::ObjectValueType_VectorPtr || (sp::ObjectValueType)params[4] == sp::ObjectValueType_Vector) {
		sdk::Vector* vec = (sdk::Vector*)(object + params[3]);

		if ((sp::ObjectValueType)params[4] == sp::ObjectValueType_VectorPtr) {
			vec = *(sdk::Vector**)vec;
			if (vec == nullptr) {
				return context->ThrowNativeError("Trying to get value for null pointer.");
			}
		}

		buffer[0] = sp_ftoc(vec->x);
		buffer[1] = sp_ftoc(vec->y);
		buffer[2] = sp_ftoc(vec->z);
		return 1;
	}

	return context->ThrowNativeError("Invalid Object value type (not a type of vector)");
}

cell_t DHookParam_SetParamObjectPtrVarVector(SourcePawn::IPluginContext* context, const cell_t* params) {
	auto obj = Get(context, params[1]);
	if (obj == nullptr) {
		return 0;
	}

	int index = GetParam(context, obj, params[2]);
	if (index == -1) {
		return 0;
	}

	const auto& var = obj->GetCapsule()->GetParameters()[index];
	if (var.dhook_type != sp::HookParamType_ObjectPtr && var.dhook_type != sp::HookParamType_Object) {
		return context->ThrowNativeError("Invalid object value type %i", var.dhook_type);
	}

	std::uint8_t* object = obj->Get<std::uint8_t>(index);
	if (var.dhook_type == sp::HookParamType_ObjectPtr) {
		object = *(std::uint8_t**)object;
	}

	cell_t *buffer;
	context->LocalToPhysAddr(params[5], &buffer);

	if((sp::ObjectValueType)params[4] == sp::ObjectValueType_VectorPtr || (sp::ObjectValueType)params[4] == sp::ObjectValueType_Vector) {
		sdk::Vector* vec = (sdk::Vector*)(object + params[3]);

		if ((sp::ObjectValueType)params[4] == sp::ObjectValueType_VectorPtr) {
			vec = *(sdk::Vector**)vec;
			if (vec == nullptr) {
				return context->ThrowNativeError("Trying to set value for null pointer.");
			}
		}

		vec->x = sp_ctof(buffer[0]);
		vec->y = sp_ctof(buffer[1]);
		vec->z = sp_ctof(buffer[2]);
		return 0;
	}
	return context->ThrowNativeError("Invalid Object value type (not a type of vector)");
}

cell_t DHookParam_GetParamObjectPtrString(SourcePawn::IPluginContext* context, const cell_t* params) {
	auto obj = Get(context, params[1]);
	if (obj == nullptr) {
		return 0;
	}

	int index = GetParam(context, obj, params[2]);
	if (index == -1) {
		return 0;
	}

	const auto& var = obj->GetCapsule()->GetParameters()[index];
	if (var.dhook_type != sp::HookParamType_ObjectPtr && var.dhook_type != sp::HookParamType_Object) {
		return context->ThrowNativeError("Invalid object value type %i", var.dhook_type);
	}

	std::uint8_t* object = obj->Get<std::uint8_t>(index);
	if (var.dhook_type == sp::HookParamType_ObjectPtr) {
		object = *(std::uint8_t**)object;
	}


	switch((sp::ObjectValueType)params[4]) {
		case sp::ObjectValueType_CharPtr: {
			char *ptr = *(char **)(object + params[3]);
			context->StringToLocal(params[5], params[6], ptr == NULL ? "" : (const char *)ptr);
			break;
		}
		case sp::ObjectValueType_String: {
			auto str = *(sdk::string_t *)(object + params[3]);
			context->StringToLocal(params[5], params[6], str.ToCStr());
			break;
		}
		default:
			return context->ThrowNativeError("Invalid Object value type (not a type of string)");
	}
	return 0;
}

cell_t DHookParam_IsNullParam(SourcePawn::IPluginContext* context, const cell_t* params) {
	auto obj = Get(context, params[1]);
	if (obj == nullptr) {
		return 0;
	}

	int index = GetParam(context, obj, params[2]);
	if (index == -1) {
		return 0;
	}

	const auto& var = obj->GetCapsule()->GetParameters()[index];
	if (sp::HookParamTypeIsPtr(var.dhook_type)) {
		return (*obj->Get<void*>(index) != nullptr) ? 1 : 0;
	}
	return context->ThrowNativeError("Param is not a pointer!");
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
	{"DHookParam.Set",                  DHookParam_SetParam},
	{"DHookParam.GetVector",            DHookParam_GetParamVector},
	{"DHookParam.SetVector",            DHookParam_SetParamVector},
	{"DHookParam.GetString",            DHookParam_GetParamString},
	{"DHookParam.SetString",            DHookParam_SetParamString},
	{"DHookParam.GetObjectVar",         DHookParam_GetParamObjectPtrVar},
	{"DHookParam.SetObjectVar",         DHookParam_SetParamObjectPtrVar},
	{"DHookParam.GetObjectVarVector",   DHookParam_GetParamObjectPtrVarVector},
	{"DHookParam.SetObjectVarVector",   DHookParam_SetParamObjectPtrVarVector},
	{"DHookParam.GetObjectVarString",   DHookParam_GetParamObjectPtrString},
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