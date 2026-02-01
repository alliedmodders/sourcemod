#include "../handle.hpp"
#include "../globals.hpp"
#include "../sp_inc.hpp"
#include "../signatures.hpp"

namespace dhooks::natives::dhooksetup {

inline handle::HookSetup* Get(SourcePawn::IPluginContext* context, const cell_t param) {
	SourceMod::HandleSecurity security;
	security.pOwner = globals::myself->GetIdentity();
	security.pIdentity = globals::myself->GetIdentity();
	SourceMod::Handle_t hndl = static_cast<SourceMod::Handle_t>(param);
	handle::HookSetup* obj = nullptr;
	SourceMod::HandleError chnderr = globals::handlesys->ReadHandle(hndl, handle::HookSetup::HANDLE_TYPE, &security, (void **)&obj);
	if (chnderr != SourceMod::HandleError_None) {
		context->ThrowNativeError("Invalid Handle %x (error %i: %s)", hndl, chnderr, globals::HandleErrorToString(chnderr));
		return nullptr;
	}
	return obj;
}

cell_t DHookSetup_AddParam(SourcePawn::IPluginContext* context, const cell_t* params) {
	auto setup = Get(context, params[1]);
	if (setup == nullptr) {
		return 0;
	}

	if (setup->IsImmutable()) {
		return context->ThrowNativeError("This setup handle can no longer be modified."); 
	}

	sp::HookParamType type = static_cast<sp::HookParamType>(params[2]);
	if (type == sp::HookParamType_Object) {
		return context->ThrowNativeError("HookParamType_Object is currently unsupported.");
	}
	if (type == sp::HookParamType_Unknown) {
		return context->ThrowNativeError("HookParamType_Unknown isn't a valid param type.");
	}

	sp::DHookPassFlag flags = sp::DHookPass_ByVal;
	if (params[0] >= 4) {
		flags = static_cast<sp::DHookPassFlag>(params[4]);
	}

	sp::DHookRegister param_register = sp::DHookRegister_Default;
	if (params[0] >= 5) {
		param_register = static_cast<sp::DHookRegister>(params[5]);
	}

	std::size_t size = sp::GetParamTypeSize(type).value_or(0);
	if (params[0] >= 3 && params[3] != -1) {
		size = params[3];
	}
	
	if (size == 0) {
		return context->ThrowNativeError("Param type size cannot be 0 bytes.");
	}

	Variable var;
	var.dhook_custom_register = param_register;
	var.dhook_pass_flags = flags;
	var.dhook_size = size;
	var.dhook_type = type;

	setup->AddParam(var);
	return 0;
}

cell_t DHookSetup_SetFromConf(SourcePawn::IPluginContext* context, const cell_t* params) {
	auto setup = Get(context, params[1]);
	if (setup == nullptr) {
		return 0;
	}

	if (setup->IsImmutable()) {
		return context->ThrowNativeError("This setup handle can no longer be modified."); 
	}

	SourceMod::IGameConfig* conf = nullptr;
	SourceMod::HandleError chnderr = SourceMod::HandleError_None;
	if ((conf = globals::gameconfs->ReadHandle(params[2], context->GetIdentity(), &chnderr)) == nullptr || chnderr != SourceMod::HandleError_None) {
		return context->ThrowNativeError("Invalid GameData handle %x (error %i: %s)", params[2], chnderr, globals::HandleErrorToString(chnderr));
	}

	sp::SDKFuncConfSource source = static_cast<sp::SDKFuncConfSource>(params[3]);
	
	char* key;
	context->LocalToString(params[4], &key);

	// Only change offset if HookSetup is a virtual hook
	auto virtual_setup = dynamic_cast<handle::DynamicHook*>(setup);
	if (virtual_setup) {
		int offset = 0;
		if (source == sp::SDKConf_Virtual && conf->GetOffset(key, &offset)) {
			virtual_setup->SetOffset(offset);
			return 0;
		}
		return context->ThrowNativeError("Invalid SDKFuncConfSource: %d", source);
	}

	// Only change address if HookSetup is an address hook
	auto detour_setup = dynamic_cast<handle::DynamicDetour*>(setup);
	if (detour_setup) {
		void* addr = nullptr;
		if (source == sp::SDKConf_Signature && conf->GetMemSig(key, &addr)) {
			detour_setup->SetAddress(addr);
			return 0;
		}
		if (source == sp::SDKConf_Address && conf->GetAddress(key, &addr)) {
			detour_setup->SetAddress(addr);
			return 0;
		}
		return context->ThrowNativeError("Invalid SDKFuncConfSource: %d", source);
	}

	return context->ThrowNativeError("Unknown HookSetup type.");
}

enum class DHookCreateFromConf {
	ANY,
	VIRTUAL,
	ADDRESS
};

cell_t DHookCreateFromConf_Ex(SourcePawn::IPluginContext* context, const cell_t* params, DHookCreateFromConf hook_type) {
	SourceMod::IGameConfig* conf = nullptr;
	SourceMod::HandleError chnderr = SourceMod::HandleError_None;
	if ((conf = globals::gameconfs->ReadHandle(params[1], context->GetIdentity(), &chnderr)) == nullptr || chnderr != SourceMod::HandleError_None) {
		return context->ThrowNativeError("Invalid GameData handle %x (error %i: %s)", params[1], chnderr, globals::HandleErrorToString(chnderr));
	}

	char* function;
	context->LocalToString(params[2], &function);

	SignatureWrapper* sig = globals::dhooks_config->GetFunctionSignature(function);
	if (!sig) {
		return context->ThrowNativeError("Hook definition \"%s\" was not found.", function);
	}

	// Validate the params
	for (const auto& arg: sig->args) {
		if (arg.info.type == sp::HookParamType_Unknown) {
			return context->ThrowNativeError("Undefined type for parameter \"%s\".", arg.name.c_str());
		}
		if (arg.info.size == 0) {
			return context->ThrowNativeError("Undefined size for parameter \"%s\".", arg.name.c_str());
		}
		if (arg.info.flags != sp::DHookPass_ByVal) {
			return context->ThrowNativeError("Parameters can only be passed byval in this version.");
		}
	}

	if (sig->ret.type == sp::ReturnType_Unknown) {
		return context->ThrowNativeError("Return type cannot be unknown.");
	}

	if (sig->ret.type != sp::ReturnType_Void && sig->ret.size == 0) {
		return context->ThrowNativeError("Return type cannot have a size 0 bytes.");
	}

	if (sig->ret.flags != sp::DHookPass_ByVal) {
		return context->ThrowNativeError("Return type can only be passed byval in this version.");
	}

	// Setup a virtual hook
	if (hook_type == DHookCreateFromConf::ANY || hook_type == DHookCreateFromConf::VIRTUAL) {
		int offset = 0;
		if (conf->GetOffset(sig->offset.c_str(), &offset)) {
			auto setup = new handle::DynamicHook(sig->thisType, offset, sig->args, sig->ret);
			return setup->GetHandle();
		}
		if (hook_type == DHookCreateFromConf::VIRTUAL) {
			return context->ThrowNativeError("Failed to retrieve offset \"%s\" for hook.", sig->offset.c_str());
		}
	}

	if (hook_type == DHookCreateFromConf::ANY || hook_type == DHookCreateFromConf::ADDRESS) {
		void* addr = nullptr;
		if (conf->GetAddress(sig->address.c_str(), &addr) || conf->GetMemSig(sig->signature.c_str(), &addr)) {
			auto setup = new handle::DynamicDetour(sig->thisType, sig->callConv, addr, sig->args, sig->ret);
			return setup->GetHandle();
		}
		if (hook_type == DHookCreateFromConf::ADDRESS) {
			return context->ThrowNativeError("Failed to retrieve offset \"%s\" for hook.", sig->offset.c_str());
		}
	}

	return context->ThrowNativeError("Failed to create hook");
}

cell_t DHookCreateFromConf(SourcePawn::IPluginContext* context, const cell_t* params) {
	return DHookCreateFromConf_Ex(context, params, DHookCreateFromConf::ANY);
}

cell_t DynamicHook_FromConf(SourcePawn::IPluginContext* context, const cell_t* params) {
	return DHookCreateFromConf_Ex(context, params, DHookCreateFromConf::VIRTUAL);
}

cell_t DynamicDetour_FromConf(SourcePawn::IPluginContext* context, const cell_t* params) {
	return DHookCreateFromConf_Ex(context, params, DHookCreateFromConf::ADDRESS);
}

void init(std::vector<sp_nativeinfo_t>& natives) {
	sp_nativeinfo_t list[] = {
		{"DHookSetFromConf",			  DHookSetup_SetFromConf},
		{"DHookAddParam",				  DHookSetup_AddParam},
		{"DHookCreateFromConf",           DHookCreateFromConf},
		{"DHookSetup.AddParam",           DHookSetup_AddParam},
		{"DHookSetup.SetFromConf",        DHookSetup_SetFromConf},

		{"DynamicHook.FromConf",          DynamicHook_FromConf},
		{"DynamicDetour.FromConf",        DynamicDetour_FromConf},
	};
	natives.insert(natives.end(), std::begin(list), std::end(list));
}

}