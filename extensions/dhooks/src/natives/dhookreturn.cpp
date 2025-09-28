#include "../globals.hpp"
#include "../sdk_types.hpp"
#include "../handle.hpp"
#include "../capsule.hpp"

namespace dhooks::natives::dhookreturn {

static void FreeChangedVector(void* data) {
	delete (sdk::Vector*)data;
}

static void FreeChangedCharPtr(void* data) {
	delete[] (const char*)data;
}

inline handle::ParamReturn* Get(SourcePawn::IPluginContext* context, const cell_t param) {
	SourceMod::HandleSecurity security;
	security.pOwner = globals::myself->GetIdentity();
	security.pIdentity = globals::myself->GetIdentity();
	SourceMod::Handle_t hndl = static_cast<SourceMod::Handle_t>(param);
	handle::ParamReturn* obj = nullptr;
	SourceMod::HandleError chnderr = globals::handlesys->ReadHandle(hndl, handle::ParamReturn::HANDLE_TYPE, &security, (void **)&obj);
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

void init(std::vector<sp_nativeinfo_t>& natives) {
	sp_nativeinfo_t list[] = {
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
	};
	natives.insert(natives.end(), std::begin(list), std::end(list));
}

}