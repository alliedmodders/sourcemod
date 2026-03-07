#include "handle.hpp"
#include "globals.hpp"
#include "capsule.hpp"

#include <cstdlib>

namespace dhooks::handle {

SourceMod::HandleType_t ParamReturn::HANDLE_TYPE = 0;
SourceMod::HandleType_t HookSetup::HANDLE_TYPE = 0;
SourceMod::HandleType_t DynamicHook::HANDLE_TYPE = 0;
SourceMod::HandleType_t DynamicDetour::HANDLE_TYPE = 0;

ParamReturn::ParamReturn(const Capsule* capsule, GeneralRegister* generalregs, FloatRegister* floatregs, void* return_ptr) :
	_handle(globals::handlesys->CreateHandle(handle::ParamReturn::HANDLE_TYPE, this, globals::myself->GetIdentity(), globals::myself->GetIdentity(), nullptr)),
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
		//printf("Reg code: %d Offset: %lu (%d)\n", variable.reg_index.value(), variable.reg_offset.value_or(0), variable.reg_offset.has_value());
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

bool DynamicDetour::Enable(SourcePawn::IPluginFunction* callback, sp::HookMode mode) {
	auto& detours = (mode == sp::HookMode::Hook_Post) ? _post_detours : _pre_detours;
	if (detours.find(callback) != detours.end()) {
		return false;
	}

	const auto& capsule = Capsule::FindOrCreate(this);
	if (capsule.get() == nullptr) {
		return false;
	}

	auto id = capsule->AddCallback(callback, nullptr, mode, this->_this_pointer, nullptr);
	if (detours.try_emplace(callback, id).second == false) {
		Capsule::RemoveCallbackById(id);
		return false;
	}
	return true;
}

bool DynamicDetour::Disable(SourcePawn::IPluginFunction* callback, sp::HookMode mode) {
	auto& detours = (mode == sp::HookMode::Hook_Post) ? _post_detours : _pre_detours;
	auto it = detours.find(callback);
	if (it != detours.end()) {
		return false;
	}

	Capsule::RemoveCallbackById(it->second);
	detours.erase(it);
	return true;
}

HookSetup::HookSetup(
	dhooks::sp::ThisPointerType thisptr_type,
	dhooks::sp::CallingConvention callconv,
	std::vector<dhooks::ArgumentInfo> const& args,
	dhooks::ReturnInfo const& ret
	) : 
	_handle(BAD_HANDLE),
	_immutable(false),
	_this_pointer(thisptr_type),
	_dhook_call_conv(callconv) {
	
	for (const auto& arg : args) {
		Variable var;
		var.dhook_type = arg.info.type;
		var.dhook_size = arg.info.size;
		var.dhook_pass_flags = arg.info.flags;
		var.dhook_custom_register = arg.info.custom_register;

		_dhook_params.push_back(var);
	}

	_dhook_return.dhook_type = ret.type;
    _dhook_return.dhook_size = ret.size;
    _dhook_return.dhook_pass_flags = ret.flags;
    _dhook_return.dhook_custom_register = ret.custom_register;
}

DynamicDetour::DynamicDetour(
	SourceMod::IdentityToken_t* plugin_ident,
	dhooks::sp::ThisPointerType thisptr_type,
	dhooks::sp::CallingConvention callconv,
	void* address,
	std::vector<dhooks::ArgumentInfo> const& params,
	dhooks::ReturnInfo const& ret) :
	HookSetup(thisptr_type, callconv, params, ret),
	_address(address)
	{
	SourceMod::HandleError err = SourceMod::HandleError_None;
	_handle = globals::handlesys->CreateHandle(
		DynamicDetour::HANDLE_TYPE,
		this,
		plugin_ident,
		globals::myself->GetIdentity(),
		&err
	);
	if (_handle == BAD_HANDLE) {
		globals::sourcemod->LogError(globals::myself, "Failed to create DynamicDetour: \"%s\" (%d)", globals::HandleErrorToString(err), err);
	}
}

DynamicHook::DynamicHook(
	SourceMod::IdentityToken_t* plugin_ident,
	sp::ThisPointerType thisptr_type,
	std::uint32_t offset,
	sp::HookType type,
	const std::vector<ArgumentInfo>& params,
	const ReturnInfo& ret,
    SourcePawn::IPluginFunction* default_callbakc) :
	HookSetup(thisptr_type, sp::CallingConvention::CallConv_THISCALL, params, ret),
	_offset(offset),
	_hook_type(type),
	_default_callback(default_callbakc) {
	_handle = globals::handlesys->CreateHandle(
		DynamicHook::HANDLE_TYPE,
		this,
		plugin_ident,
		globals::myself->GetIdentity(),
		nullptr
	);
}

class HookSetupDispatch : public SourceMod::IHandleTypeDispatch {
	virtual void OnHandleDestroy(SourceMod::HandleType_t type, void* object) override {
		if (type == DynamicDetour::HANDLE_TYPE) {
			delete (DynamicDetour*)object;
		}
		if (type == DynamicHook::HANDLE_TYPE) {
			delete (DynamicHook*)object;
		}
	}
};
HookSetupDispatch gHookSetupDispatcher;

void init() {
	SourceMod::HandleAccess security;
	globals::handlesys->InitAccessDefaults(nullptr, &security);
	// Do not allow cloning, the struct self-manage its handle
	security.access[SourceMod::HandleAccess_Clone] = HANDLE_RESTRICT_IDENTITY;
	ParamReturn::HANDLE_TYPE = globals::handlesys->CreateType("DHookParamReturn", &gHookSetupDispatcher, 0, nullptr, &security, globals::myself->GetIdentity(), nullptr);
	HookSetup::HANDLE_TYPE = globals::handlesys->CreateType("DHookSetup", &gHookSetupDispatcher, 0, nullptr, &security, globals::myself->GetIdentity(), nullptr);
	DynamicHook::HANDLE_TYPE = globals::handlesys->CreateType("DynamicHook", &gHookSetupDispatcher, HookSetup::HANDLE_TYPE, nullptr, &security, globals::myself->GetIdentity(), nullptr);
	DynamicDetour::HANDLE_TYPE = globals::handlesys->CreateType("DynamicDetour", &gHookSetupDispatcher, HookSetup::HANDLE_TYPE, nullptr, &security, globals::myself->GetIdentity(), nullptr);
}

}