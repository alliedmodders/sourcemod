#include "handle.hpp"
#include "globals.hpp"
#include "capsule.hpp"

#include <cstdlib>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace dhooks::handle {

SourceMod::HandleType_t ParamReturn::HANDLE_TYPE = 0;
SourceMod::HandleType_t HookSetup::HANDLE_TYPE = 0;
SourceMod::HandleType_t DynamicHook::HANDLE_TYPE = 0;
SourceMod::HandleType_t DynamicDetour::HANDLE_TYPE = 0;

class CGenericClass;
namespace locals {
std::unordered_map<std::uint32_t, cell_t> associated_handle;
std::unordered_map<CGenericClass*, std::vector<std::uint32_t>> class_dynamichooks;
std::unordered_set<void**> class_vtables;
SourceMod::HandleSecurity* security = nullptr;
}

ParamReturn::ParamReturn(const Capsule* capsule, GeneralRegister* generalregs, FloatRegister* floatregs, void* return_ptr) :
	_handle(globals::handlesys->CreateHandle(handle::ParamReturn::HANDLE_TYPE, this, globals::myself->GetIdentity(), globals::myself->GetIdentity(), nullptr)),
	_capsule(capsule),
	_general_registers(generalregs),
	_float_registers(floatregs),
	_return(return_ptr) {
}

ParamReturn::~ParamReturn() {
	if (_handle != BAD_HANDLE) {
		if (globals::handlesys->FreeHandle(_handle, locals::security) != SourceMod::HandleError_None) {
			globals::sourcemod->LogError(globals::myself, "Failed to delete ParamReturn handle. Contact a Sourcemod dev!");
		}
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

#if defined(_LINUX)
#define DTOR_PARAMS void
#define DTOR_PARAMS_NONAME
#define DTOR_PARAMS_ARGS
#define DTOR_VTABLE_INDEX 1
#elif defined(_WIN32)
#define DTOR_PARAMS unsigned int arg
#define DTOR_PARAMS_NONAME , unsigned int
#define DTOR_PARAMS_ARGS arg
#define DTOR_VTABLE_INDEX 0
#else
static_assert(false, "You forgort to provide a platform define!");
#endif

class CGenericClass {
public:
	void KHook_Detour_PRE(DTOR_PARAMS) {
		auto it = locals::class_dynamichooks.find(this);
		if (it != locals::class_dynamichooks.end()) {
			const auto& ids = it->second;
			for (auto id : ids) {
				auto hndl = DynamicHook::FindByHookID(id);
				if (hndl != BAD_HANDLE) {
					SourceMod::HandleSecurity security;
					security.pOwner = globals::myself->GetIdentity();
					security.pIdentity = globals::myself->GetIdentity();
					handle::DynamicHook* obj = nullptr;
					SourceMod::HandleError chnderr = globals::handlesys->ReadHandle(hndl, DynamicHook::HANDLE_TYPE, &security, (void **)&obj);
					if (chnderr != SourceMod::HandleError_None) {
						obj = nullptr;
					}

					if (obj) {
						obj->RemoveHook(id);
					} else {
						Capsule::RemoveCallbackById(id);
					}
				} else {
					Capsule::RemoveCallbackById(id);
				}
			}
			locals::class_dynamichooks.erase(it);
		}

		::KHook::SaveReturnValue(KHook::Action::Ignore, nullptr, 0, nullptr, nullptr, false);
		return;
	}
	void KHook_Make_CallOriginal(DTOR_PARAMS) {
		void (CGenericClass::*ptr)(DTOR_PARAMS) = ::KHook::BuildMFP<CGenericClass, void DTOR_PARAMS_NONAME>(::KHook::GetOriginalFunction());
		(this->*ptr)(DTOR_PARAMS_ARGS);
		::KHook::SaveReturnValue(KHook::Action::Ignore, nullptr, 0, nullptr, nullptr, true);
		return;
	}
	void KHook_Make_Return(DTOR_PARAMS) {
		::KHook::DestroyReturnValue();
		return;
	}
};

bool DynamicDetour::Enable(SourcePawn::IPluginFunction* callback, sp::HookMode mode) {
	if (!this->IsImmutable()) {
		return false;
	}

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

std::uint32_t DynamicHook::AddHook(SourcePawn::IPluginFunction* callback, SourcePawn::IPluginFunction* rm_callback, sp::HookMode mode, void* obj, bool dtor_cleanup) {
	if (!this->IsImmutable()) {
		return 0;
	}
	
	auto vtable = *(void***)obj;
	const auto& capsule = Capsule::FindOrCreate(this, vtable);
	if (capsule.get() == nullptr) {
		globals::sourcemod->LogError(globals::myself, "Failed to create capsule");
		return 0;
	}

	auto id = capsule->AddCallback(callback, rm_callback, mode, this->_this_pointer, obj);
	if (_associated_hook.insert(id).second == false || locals::associated_handle.try_emplace(id, this->GetHandle()).second == false) {
		Capsule::RemoveCallbackById(id);
		globals::sourcemod->LogError(globals::myself, "Failed to insert callback");
		return 0;
	}

	if (dtor_cleanup) {
		auto it = locals::class_dynamichooks.find((CGenericClass*)obj);
		if (it == locals::class_dynamichooks.end()) {
			auto insert = locals::class_dynamichooks.emplace((CGenericClass*)obj, std::vector<std::uint32_t>());
			if (insert.second == false) {
				return id;
			}
			it = insert.first;

			if (locals::class_vtables.find(vtable) == locals::class_vtables.end()) {
				// Hook the virtual destructor, and perform hook cleaning actions under there
				KHook::SetupVirtualHook(
					vtable,
					DTOR_VTABLE_INDEX,
					nullptr,
					nullptr,
					KHook::ExtractMFP(&CGenericClass::KHook_Detour_PRE),
					nullptr,
					KHook::ExtractMFP(&CGenericClass::KHook_Make_Return),
					KHook::ExtractMFP(&CGenericClass::KHook_Make_CallOriginal),
					true
				);
				locals::class_vtables.insert(vtable);
			}
		}
		it->second.push_back(id);
	}
	return id;
}

bool DynamicHook::RemoveHook(std::uint32_t id) {
	if (_associated_hook.find(id) == _associated_hook.end()) {
		return false;
	}
	Capsule::RemoveCallbackById(id);
	_associated_hook.erase(id);
	return true;
}

cell_t DynamicHook::FindByHookID(std::uint32_t id) {
	auto it = locals::associated_handle.find(id);
	if (locals::associated_handle.end() == it) {
		return BAD_HANDLE;
	}
	return it->second;
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

DynamicDetour::~DynamicDetour() {
	for (const auto& it : _post_detours) {
		Capsule::RemoveCallbackById(it.second);
	}
	for (const auto& it : _pre_detours) {
		Capsule::RemoveCallbackById(it.second);
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
	locals::security = new SourceMod::HandleSecurity(globals::myself->GetIdentity(), globals::myself->GetIdentity());

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