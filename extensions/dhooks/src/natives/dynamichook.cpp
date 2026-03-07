#include "../natives.hpp"
#include "../handle.hpp"
#include "../globals.hpp"
#include "dynamicdetour.hpp"

namespace dhooks::natives::dynamichook {

inline handle::DynamicHook* Get(SourcePawn::IPluginContext* context, const cell_t param) {
	SourceMod::HandleSecurity security;
	security.pOwner = globals::myself->GetIdentity();
	security.pIdentity = globals::myself->GetIdentity();
	SourceMod::Handle_t hndl = static_cast<SourceMod::Handle_t>(param);
	handle::DynamicHook* obj = nullptr;
	SourceMod::HandleError chnderr = globals::handlesys->ReadHandle(hndl, handle::DynamicHook::HANDLE_TYPE, &security, (void **)&obj);
	if (chnderr != SourceMod::HandleError_None) {
		context->ThrowNativeError("Invalid DynamicHook Handle %x (error %i: %s)", hndl, chnderr, globals::HandleErrorToString(chnderr));
		return nullptr;
	}
	return obj;
}

cell_t DynamicHook_DynamicHook(SourcePawn::IPluginContext* context, const cell_t* params) {
	auto offset = params[1];
	auto type = static_cast<sp::HookType>(params[2]);
	auto returntype = static_cast<sp::ReturnType>(params[3]);
	auto thisptrtype = static_cast<sp::ThisPointerType>(params[4]);

	ReturnInfo ret_info;
	ret_info.custom_register = sp::DHookRegister_Default;
	ret_info.flags = sp::DHookPass_ByVal;
	ret_info.type = returntype;
	ret_info.size = sp::GetReturnTypeSize(returntype).value_or(0);
	if (ret_info.size == 0 && returntype != sp::ReturnType_Void) {
		return context->ThrowNativeError("Return type size cannot be 0 bytes");
	}

	auto hndl = new handle::DynamicHook(context->GetIdentity(), thisptrtype, offset, type, {}, ret_info, nullptr);
	return hndl->GetHandle();
}

cell_t DynamicHook_Create(SourcePawn::IPluginContext* context, const cell_t* params) {
	auto offset = params[1];
	auto type = static_cast<sp::HookType>(params[2]);
	auto returntype = static_cast<sp::ReturnType>(params[3]);
	auto thisptrtype = static_cast<sp::ThisPointerType>(params[4]);
	auto callback = context->GetFunctionById(params[5]);

	ReturnInfo ret_info;
	ret_info.custom_register = sp::DHookRegister_Default;
	ret_info.flags = sp::DHookPass_ByVal;
	ret_info.type = returntype;
	ret_info.size = sp::GetReturnTypeSize(returntype).value_or(0);
	if (ret_info.size == 0 && returntype != sp::ReturnType_Void) {
		return context->ThrowNativeError("Return type size cannot be 0 bytes");
	}

	auto hndl = new handle::DynamicHook(context->GetIdentity(), thisptrtype, offset, type, {}, ret_info, callback);
	return hndl->GetHandle();
}

cell_t DynamicHook_HookEntity(SourcePawn::IPluginContext* context, const cell_t* params) {
	auto dynhook = Get(context, params[1]);
	if (dynhook == nullptr) {
		return 0;
	}

	if (dynhook->GetType() != sp::HookType::HookType_Entity) {
		return context->ThrowNativeError("DynamicHook isn't an entity hook!");
	}

	auto mode = static_cast<sp::HookMode>(params[2]);

	auto entity = globals::gamehelpers->ReferenceToEntity(params[3]);
	if (entity == nullptr) {
		return context->ThrowNativeError("Invalid entity index/reference %d!", params[3]);
	}

	auto callback = context->GetFunctionById(params[4]);
	if (callback == nullptr) {
		callback = dynhook->GetDefaultCallback();
	}

	if (callback == nullptr) {
		return context->ThrowNativeError("A callback must be provided to the hook!");
	}

	auto removal_callback = context->GetFunctionById(params[5]);

	// TO-DO : Hook code
	return context->ThrowNativeError("TO-DO IMPLEMENT!!");
}

cell_t DynamicHook_DHookEntity(SourcePawn::IPluginContext* context, const cell_t* params) {
	auto dynhook = Get(context, params[1]);
	if (dynhook == nullptr) {
		return 0;
	}

	if (dynhook->GetType() != sp::HookType::HookType_Entity) {
		return context->ThrowNativeError("DynamicHook isn't an entity hook!");
	}

	bool post = (params[2] == 0) ? false : true;

	auto entity = globals::gamehelpers->ReferenceToEntity(params[3]);
	if (entity == nullptr) {
		return context->ThrowNativeError("Invalid entity index/reference %d!", params[3]);
	}

	auto removal_callback = context->GetFunctionById(params[4]);

	auto callback = context->GetFunctionById(params[5]);
	if (callback == nullptr) {
		callback = dynhook->GetDefaultCallback();
	}

	if (callback == nullptr) {
		return context->ThrowNativeError("A callback must be provided to the hook!");
	}

	// TO-DO : Hook code
	return context->ThrowNativeError("TO-DO IMPLEMENT!!");
}

cell_t DynamicHook_HookGamerules(SourcePawn::IPluginContext* context, const cell_t* params) {
	auto dynhook = Get(context, params[1]);
	if (dynhook == nullptr) {
		return 0;
	}

	if (dynhook->GetType() != sp::HookType::HookType_GameRules) {
		return context->ThrowNativeError("DynamicHook isn't a gamerules hook!");
	}

	auto mode = static_cast<sp::HookMode>(params[2]);

	auto callback = context->GetFunctionById(params[3]);
	if (callback == nullptr) {
		callback = dynhook->GetDefaultCallback();
	}

	if (callback == nullptr) {
		return context->ThrowNativeError("A callback must be provided to the hook!");
	}

	auto removal_callback = context->GetFunctionById(params[4]);

	// TO-DO : Hook code
	return context->ThrowNativeError("TO-DO IMPLEMENT!!");
}

cell_t DynamicHook_DHookGamerules(SourcePawn::IPluginContext* context, const cell_t* params) {
	auto dynhook = Get(context, params[1]);
	if (dynhook == nullptr) {
		return 0;
	}

	if (dynhook->GetType() != sp::HookType::HookType_GameRules) {
		return context->ThrowNativeError("DynamicHook isn't a gamerules hook!");
	}

	auto post = (params[2] == 0) ? false : true;

	auto removal_callback = context->GetFunctionById(params[3]);

	auto callback = context->GetFunctionById(params[4]);
	if (callback == nullptr) {
		callback = dynhook->GetDefaultCallback();
	}

	if (callback == nullptr) {
		return context->ThrowNativeError("A callback must be provided to the hook!");
	}

	// TO-DO : Hook code
	return context->ThrowNativeError("TO-DO IMPLEMENT!!");
}

cell_t DynamicHook_HookRaw(SourcePawn::IPluginContext* context, const cell_t* params) {
	auto dynhook = Get(context, params[1]);
	if (dynhook == nullptr) {
		return 0;
	}

	if (dynhook->GetType() != sp::HookType::HookType_Raw) {
		return context->ThrowNativeError("DynamicHook isn't a raw hook!");
	}

	auto mode = static_cast<sp::HookMode>(params[2]);

	auto addr = globals::cell_to_ptr(context, params[3]);

	auto callback = context->GetFunctionById(params[4]);
	if (callback == nullptr) {
		callback = dynhook->GetDefaultCallback();
	}

	// TO-DO : Hook code
	return context->ThrowNativeError("TO-DO IMPLEMENT!!");
}

cell_t DynamicHook_DHookRaw(SourcePawn::IPluginContext* context, const cell_t* params) {
	auto dynhook = Get(context, params[1]);
	if (dynhook == nullptr) {
		return 0;
	}

	if (dynhook->GetType() != sp::HookType::HookType_Raw) {
		return context->ThrowNativeError("DynamicHook isn't a raw hook!");
	}

	auto post = (params[2] == 0) ? false : true;

	auto addr = globals::cell_to_ptr(context, params[3]);

	auto removal_callback = context->GetFunctionById(params[4]);

	auto callback = context->GetFunctionById(params[5]);
	if (callback == nullptr) {
		callback = dynhook->GetDefaultCallback();
	}

	if (callback == nullptr) {
		return context->ThrowNativeError("A callback must be provided to the hook!");
	}

	// TO-DO : Hook code
	return context->ThrowNativeError("TO-DO IMPLEMENT!!");
}

cell_t DynamicHook_RemoveHook(SourcePawn::IPluginContext* context, const cell_t* params) {
	// TO-DO : Hook removal
	return context->ThrowNativeError("TO-DO IMPLEMENT!!");
}

void init(std::vector<sp_nativeinfo_t>& natives) {
	sp_nativeinfo_t list[] = {
		{"DynamicHook.DynamicHook",       DynamicHook_DynamicHook},
		{"DynamicHook.HookEntity",        DynamicHook_HookEntity},
		{"DynamicHook.HookGamerules",     DynamicHook_HookGamerules},
		{"DynamicHook.HookRaw",           DynamicHook_HookRaw},
		{"DynamicHook.RemoveHook",        DynamicHook_RemoveHook},

		{"DHookCreate",                   DynamicHook_Create},
		{"DHookEntity",                   DynamicHook_DHookEntity},
		{"DHookGamerules",                DynamicHook_DHookGamerules},
		{"DHookRaw",                      DynamicHook_DHookRaw},
		{"DHookRemoveHookID",             DynamicHook_RemoveHook}
	};

	natives.insert(natives.end(), std::begin(list), std::end(list));
}
}