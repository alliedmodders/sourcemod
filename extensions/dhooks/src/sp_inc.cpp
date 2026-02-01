#include "sp_inc.hpp"
#include "sdk_types.hpp"

namespace dhooks::sp {

std::optional<std::size_t> GetParamTypeSize(HookParamType type) {
	switch (type) {
		case HookParamType_Int:
			return sizeof(int);
		case HookParamType_Bool:
			return sizeof(bool);
		case HookParamType_Float:
			return sizeof(float);
		case HookParamType_String:
			return sizeof(sdk::string_t);
		case HookParamType_StringPtr:
			return sizeof(sdk::string_t*);
		case HookParamType_CharPtr:
			return sizeof(const char*);
		case HookParamType_VectorPtr:
			return sizeof(sdk::Vector*);
		case HookParamType_CBaseEntity:
			return sizeof(sdk::CBaseEntity*);
		case HookParamType_ObjectPtr:
			return sizeof(void*);
		case HookParamType_Edict:
			return sizeof(sdk::edict_t*);
	}
	return {};
}

std::optional<std::size_t> GetReturnTypeSize(ReturnType type) {
	switch (type) {
		case sp::ReturnType_Void:
			return 0;
		case sp::ReturnType_Int:
			return sizeof(int);
		case sp::ReturnType_Bool:
			return sizeof(bool);
		case sp::ReturnType_Float:
			return sizeof(float);
		case sp::ReturnType_String:
			return sizeof(sdk::string_t);
		case sp::ReturnType_StringPtr:
			return sizeof(sdk::string_t*);
		case sp::ReturnType_CharPtr:
			return sizeof(const char*);
		case sp::ReturnType_Vector:
			return sizeof(sdk::Vector);
		case sp::ReturnType_VectorPtr:
			return sizeof(sdk::Vector*);
		case sp::ReturnType_CBaseEntity:
			return sizeof(sdk::CBaseEntity*);
		case sp::ReturnType_Edict:
			return sizeof(sdk::edict_t*);
	}
	return {};
}

}