#include "../handle.hpp"
#include "../globals.hpp"

namespace dhooks::natives::dhooksetup {

inline handle::DynamicDetour* Get(SourcePawn::IPluginContext* context, const cell_t param) {
	SourceMod::HandleSecurity security;
	security.pOwner = globals::myself->GetIdentity();
	security.pIdentity = globals::myself->GetIdentity();
	SourceMod::Handle_t hndl = static_cast<SourceMod::Handle_t>(param);
	handle::DynamicDetour* obj = nullptr;
	SourceMod::HandleError chnderr = globals::handlesys->ReadHandle(hndl, handle::DynamicDetour::HANDLE_TYPE, &security, (void **)&obj);
	if (chnderr != SourceMod::HandleError_None) {
		context->ThrowNativeError("Invalid Handle %x (error %i: %s)", hndl, chnderr, globals::HandleErrorToString(chnderr));
		return nullptr;
	}
	return obj;
}

cell_t DynamicDetour_DynamicDetour(SourcePawn::IPluginContext* context, const cell_t* params) {
	auto addr = globals::cell_to_ptr(context, params[1]);
	auto callconv = static_cast<sp::CallingConvention>(params[2]);
	auto returntype = static_cast<sp::ReturnType>(params[3]);
	auto thisptrtype = static_cast<sp::ThisPointerType>(params[4]);

	ReturnInfo info;
	info.custom_register = sp::DHookRegister_Default;
	info.flags = sp::DHookPass_ByVal;
	info.type = returntype;
	info.size = sp::GetReturnTypeSize(returntype).value_or(0);
	if (info.size == 0 && returntype != sp::ReturnType_Void) {
		return context->ThrowNativeError("Return type size cannot be 0 bytes");
	}

	auto hndl = new handle::DynamicDetour(thisptrtype, callconv, addr, {}, info);
	return hndl->GetHandle();
}

cell_t DynamicDetour_Enable(SourcePawn::IPluginContext* context, const cell_t* params) {
	auto setup = Get(context, params[1]);
	if (setup == nullptr) {
		return 0;
	}

	SourcePawn::IPluginFunction* callback = context->GetFunctionById(params[3]);
	if (!callback) {
		return context->ThrowNativeError("Failed to retrieve function by id");
	}

	auto mode = static_cast<sp::HookMode>(params[2]);

	// Check if we already detoured that function.
	CHookManager *pDetourManager = GetHookManager();
	CHook* pDetour = pDetourManager->FindHook(setup->funcAddr);

	// If there is no detour on this function yet, create it.
	if (!pDetour) {
		ICallingConvention *pCallConv = ConstructCallingConvention(setup);
		pDetour = pDetourManager->HookFunction(setup->funcAddr, pCallConv);
		if (!UpdateRegisterArgumentSizes(pDetour, setup))
			return pContext->ThrowNativeError("A custom register for a parameter isn't supported.");
	}

	// Register our pre/post handler.
	pDetour->AddCallback(hookType, (HookHandlerFn *)&HandleDetour);

	// Add the plugin callback to the map.
	return AddDetourPluginHook(hookType, pDetour, setup, callback);
}

void init(std::vector<sp_nativeinfo_t>& natives) {
	sp_nativeinfo_t list[] = {
		{"DHookCreateDetour",                  DynamicDetour_DynamicDetour},
		{"DynamicDetour.DynamicDetour",        DynamicDetour_DynamicDetour},
		/* DynamicDetour.FromConf is implemented in dhooksetup.cpp */
		{"DynamicDetour.Enable",               DynamicDetour_Enable},
		{"DynamicDetour.Disable",              DynamicDetour_Disable},
	};
	natives.insert(natives.end(), std::begin(list), std::end(list));
}

}