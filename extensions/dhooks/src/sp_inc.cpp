#include "sp_inc.hpp"
#include "sdk_types.hpp"

namespace dhooks::sp {

std::size_t GetParamTypeSize(HookParamType type) {
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
		default:
			return 0;
	}
}

}