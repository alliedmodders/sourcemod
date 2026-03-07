#include "capsule.hpp"
#include "handle.hpp"
#include "abi.hpp"
#include "sdk_types.hpp"

namespace dhooks {

using namespace KHook::Asm;

namespace locals {
std::uint32_t last_hook_id = 1;
std::unordered_map<void*, std::unique_ptr<Capsule>> address_detours;
std::unordered_map<void*, std::unique_ptr<Capsule>> virtual_detours;
std::unordered_map<std::uint32_t, HookCallback> hook_callbacks;
std::unordered_map<SourcePawn::IPluginContext*, std::unordered_set<std::uint32_t>> plugin_hook_ids;

static std::unique_ptr<Capsule> nullptr_capsule(nullptr);
}

const std::unique_ptr<Capsule>& Capsule::FindOrCreate(const handle::HookSetup* setup, void** vtable) {
	auto dyndetour = dynamic_cast<const handle::DynamicDetour*>(setup);
	if (dyndetour) {
		// Let's see if a hook already exists
		auto it = locals::address_detours.find(dyndetour->GetAddress());
		if (it == locals::address_detours.end()) {
			// It does not, so let's create it
			auto hook = new Capsule(dyndetour->GetAddress(), nullptr, 0, setup->GetCallConv(), setup->GetParameters(), setup->GetReturn());
			if (!hook->IsActive()) {
				delete hook;
				return locals::nullptr_capsule;
			}
			auto insert = locals::address_detours.try_emplace(dyndetour->GetAddress(), hook);
			if (!insert.second) {
				delete hook;
				return locals::nullptr_capsule;
			}
			it = insert.first;
		}
		return it->second;
	}
	return locals::nullptr_capsule;
}

class EmptyClass {
public:
	bool AllowedToHealTarget_MakeReturn( CBaseEntity* pTarget ) {
		printf("MAKE RETURN\n");

		bool ret = *(bool*)::KHook::GetCurrentValuePtr(true);
		::KHook::DestroyReturnValue();
		return ret;
	}

	bool AllowedToHealTarget_CallOriginal( CBaseEntity* pTarget ) {
		printf("CALLED ORIGINAL 0x%lX\n", (uintptr_t)this);

		auto ptr = KHook::BuildMFP<EmptyClass, bool, CBaseEntity*>(::KHook::GetOriginalFunction());
		bool ret = (((EmptyClass*)this)->*ptr)(pTarget);
		::KHook::__internal__savereturnvalue(KHook::Return<bool>{ KHook::Action::Ignore, ret }, true);
		return ret;
	}
};

std::uint32_t Capsule::AddCallback(SourcePawn::IPluginFunction* callback, SourcePawn::IPluginFunction* remove_callback, sp::HookMode mode, sp::ThisPointerType this_ptr, void* associated_this) {
	auto id = ++locals::last_hook_id;
	
	HookCallback cb;
	cb.this_pointer_type = this_ptr;
	cb.associated_this = associated_this;
	cb.associated_capsule = this;
	cb.callback = callback;
	cb.remove_callback = remove_callback;

	auto plugin_it = locals::plugin_hook_ids.find(callback->GetParentRuntime()->GetDefaultContext());
	if (plugin_it == locals::plugin_hook_ids.end()) {
		auto plugin_insert = locals::plugin_hook_ids.try_emplace(callback->GetParentRuntime()->GetDefaultContext());
		if (!plugin_insert.second) {
			return 0;
		}
		plugin_it = plugin_insert.first;
	}
	if (plugin_it->second.find(id) != plugin_it->second.end()) {
		return 0;
	}
	if (plugin_it->second.insert(id).second == false) {
		return 0;
	}

	auto it = locals::hook_callbacks.find(id);
	if (it == locals::hook_callbacks.end()) {
		auto it_insert = locals::hook_callbacks.try_emplace(id, cb);
		if (!it_insert.second) {
			plugin_it->second.erase(id);
			return 0;
		}
		it = it_insert.first;
	} else {
		plugin_it->second.erase(id);
		return 0;
	}

	auto& callbacks = (mode == sp::HookMode::Hook_Post) ? this->_post_hooks : this->_pre_hooks;
	auto capsule_it = callbacks.find(id);
	if (capsule_it == callbacks.end()) {
		auto capsule_emplace = callbacks.try_emplace(id, cb);
		if (!capsule_emplace.second) {
			locals::hook_callbacks.erase(id);
			plugin_it->second.erase(id);
			return 0;
		}
		capsule_it = capsule_emplace.first;
	} else {
		locals::hook_callbacks.erase(id);
		plugin_it->second.erase(id);
		return 0;
	}
	return id;
}

void Capsule::RemoveCallbackById(std::uint32_t id) {
	auto it = locals::hook_callbacks.find(id);
	if (locals::hook_callbacks.end() == it) {
		return;
	}
	const auto& cb = it->second;
	auto rm_callback = cb.remove_callback;

	if (cb.associated_capsule != nullptr) {
		cb.associated_capsule->_pre_hooks.erase(id);
		cb.associated_capsule->_post_hooks.erase(id);
	}
	auto plugin_hooks_it = locals::plugin_hook_ids.find(cb.callback->GetParentRuntime()->GetDefaultContext());
	if (locals::plugin_hook_ids.end() != plugin_hooks_it) {
		plugin_hooks_it->second.erase(id);
	}
	if (cb.remove_callback != nullptr && cb.remove_callback->IsRunnable()) {
		cb.remove_callback->PushCell(id);
		cell_t ignore;
		cb.remove_callback->Execute(&ignore);
	}
	locals::hook_callbacks.erase(id);

	if (rm_callback != nullptr && rm_callback->IsRunnable()) {
		rm_callback->PushCell((cell_t)id);
		cell_t result;
		rm_callback->Execute(&result);
	}
}

void Capsule::RemoveCallbackByPlugin(SourcePawn::IPluginContext* default_context) {
	auto plugin_hooks_it = locals::plugin_hook_ids.find(default_context);
	if (locals::plugin_hook_ids.end() == plugin_hooks_it) {
		return;
	}
	for (auto id : plugin_hooks_it->second) {
		auto it = locals::hook_callbacks.find(id);
		if (locals::hook_callbacks.end() == it) {
			continue;
		}
		const auto& cb = it->second;
		auto rm_callback = cb.remove_callback;
		
		if (cb.associated_capsule != nullptr) {
			cb.associated_capsule->_pre_hooks.erase(id);
			cb.associated_capsule->_post_hooks.erase(id);
		}
		if (cb.remove_callback != nullptr && cb.remove_callback->IsRunnable()) {
			cb.remove_callback->PushCell(id);
			cell_t ignore;
			cb.remove_callback->Execute(&ignore);
		}
		locals::hook_callbacks.erase(id);

		if (rm_callback != nullptr && rm_callback->IsRunnable()) {
			rm_callback->PushCell((cell_t)id);
			cell_t result;
			rm_callback->Execute(&result);
		}
	}
	locals::plugin_hook_ids.erase(default_context);
}

Capsule::Capsule(void* address, void** vtable, std::uint32_t vtable_index, sp::CallingConvention conv, const std::vector<Variable>& params, const ReturnVariable& ret) :
	_parameters(params),
	_return(ret),
	_call_conv(conv) {
	if (!abi::Proccess(conv, _parameters, _return, _stack_size)) {
		return;
	}

	// Well we need to know what we're detouring here
	if (address == nullptr && vtable == nullptr) {
		return;
	}

	// For every parameter found, and return value, save the associated registers
	for (auto& param : _parameters) {
		if (param.float_reg_index) {
			_save_float_register[param.float_reg_index.value()] = true;
		}
		if (param.reg_index) {
			_save_general_register[param.reg_index.value()] = true;
		}

		if (!param.float_reg_index && !param.reg_index) {
			// A parameter must be associated with a register
			// If a parameter is on the stack, then it must be
			// associated with the stack and the offset into it
			globals::sourcemod->LogError(globals::myself, "Param isn't associated with register. Kill server");
			std::abort();
		}
	}
	if (_return.float_reg_index) {
		_save_float_register[_return.float_reg_index.value()] = true;
	}
	if (_return.reg_index) {
		_save_general_register[_return.reg_index.value()] = true;
	}

	const void* mfp = nullptr;
	switch (ret.dhook_type) {
		case sp::ReturnType_Void:
		mfp = KHook::ExtractMFP(&Capsule::PrePostHookLoop<void>);
		break;
		case sp::ReturnType_Int:
		mfp = KHook::ExtractMFP(&Capsule::PrePostHookLoop<int>);
		break;
		case sp::ReturnType_Bool:
		mfp = KHook::ExtractMFP(&Capsule::PrePostHookLoop<bool>);
		break;
		case sp::ReturnType_Float:
		mfp = KHook::ExtractMFP(&Capsule::PrePostHookLoop<float>);
		break;
		case sp::ReturnType_String:
		mfp = KHook::ExtractMFP(&Capsule::PrePostHookLoop<sdk::string_t>);
		break;
		case sp::ReturnType_StringPtr:
		mfp = KHook::ExtractMFP(&Capsule::PrePostHookLoop<sdk::string_t*>);
		break;
		case sp::ReturnType_CharPtr:
		mfp = KHook::ExtractMFP(&Capsule::PrePostHookLoop<char*>);
		break;
		case sp::ReturnType_Vector:
		mfp = KHook::ExtractMFP(&Capsule::PrePostHookLoop<sdk::Vector>);
		break;
		case sp::ReturnType_VectorPtr:
		mfp = KHook::ExtractMFP(&Capsule::PrePostHookLoop<sdk::Vector*>);
		break;
		case sp::ReturnType_CBaseEntity:
		mfp = KHook::ExtractMFP(&Capsule::PrePostHookLoop<sdk::CBaseEntity*>);
		break;
		case sp::ReturnType_Edict:
		mfp = KHook::ExtractMFP(&Capsule::PrePostHookLoop<sdk::edict_t*>);
		break;
		default:
		globals::sourcemod->LogError(globals::myself, "Invalid return type. Kill server");
		std::abort();
		break;
	}

	abi::JIT_Align(_jit);
	auto offset_to_pre_callback = _jit.get_outputpos();
	// PRE Callback
	{
		abi::JIT_CallMemberFunction(_jit, _save_general_register, _save_float_register, this, mfp, false);
		// KHook doesn't care whether or not we've set the return registers/cleaned the stack
		_jit.retn();
	}

	abi::JIT_Align(_jit);
	auto offset_to_post_callback = _jit.get_outputpos();
	// POST Callback
	{
		abi::JIT_CallMemberFunction(_jit, _save_general_register, _save_float_register, this, mfp, true);
		// KHook doesn't care whether or not we've set the return registers/cleaned the stack
		_jit.retn();
	}

	abi::JIT_Align(_jit);
	auto offset_to_make_return = _jit.get_outputpos();
	abi::JIT_MakeReturn(_jit, _return);

	abi::JIT_Align(_jit);
	auto offset_to_call_original = _jit.get_outputpos();
	abi::JIT_CallOriginal(_jit, _return, &_original_function, _stack_size, &_jit_start);

	abi::JIT_Align(_jit);
	auto offset_to_recall = _jit.get_outputpos();
	abi::JIT_Recall(_jit, _save_general_register, _save_float_register, _stack_size, &_jit_start);

	_jit.SetRE();
	_jit_start = reinterpret_cast<std::uintptr_t>(_jit.GetData());

	_linked_hook = (address != nullptr) ?
		KHook::SetupHook(
			address,
			this,
			KHook::ExtractMFP(&Capsule::_KHook_RemovedHook),
			reinterpret_cast<void*>(_jit_start + offset_to_pre_callback),
			reinterpret_cast<void*>(_jit_start + offset_to_post_callback),
			reinterpret_cast<void*>(_jit_start + offset_to_make_return), // KHook::ExtractMFP(&EmptyClass::AllowedToHealTarget_MakeReturn), //reinterpret_cast<void*>(_jit_start + offset_to_make_return),
			reinterpret_cast<void*>(_jit_start + offset_to_call_original), // KHook::ExtractMFP(&EmptyClass::AllowedToHealTarget_CallOriginal), //reinterpret_cast<void*>(_jit_start + offset_to_call_original),
			true
		)
	:
		KHook::SetupVirtualHook(
			vtable,
			vtable_index,
			this,
			KHook::ExtractMFP(&Capsule::_KHook_RemovedHook),
			reinterpret_cast<void*>(_jit_start + offset_to_pre_callback),
			reinterpret_cast<void*>(_jit_start + offset_to_post_callback),
			reinterpret_cast<void*>(_jit_start + offset_to_make_return),
			reinterpret_cast<void*>(_jit_start + offset_to_call_original),
			true
		);

	if (_linked_hook != KHook::INVALID_HOOK) {
		_original_function = (address != nullptr) ?
				reinterpret_cast<std::uintptr_t>(KHook::FindOriginal(address))
			:
				reinterpret_cast<std::uintptr_t>(KHook::FindOriginalVirtual(vtable, vtable_index));
		_recall_function = reinterpret_cast<decltype(_recall_function)>(_jit_start + offset_to_recall);
	}
}

void Capsule::_KHook_RemovedHook(unsigned int) {
}

Capsule::~Capsule() {
	if (_linked_hook != KHook::INVALID_HOOK) {
		KHook::RemoveHook(_linked_hook, false);
	}
	for (const auto& it : _pre_hooks) {
		locals::hook_callbacks.erase(it.first);
		auto default_ctx = it.second.callback->GetParentRuntime()->GetDefaultContext();
		auto plugin_it = locals::plugin_hook_ids.find(default_ctx);
		if (plugin_it != locals::plugin_hook_ids.end()) {
			plugin_it->second.erase(it.first);
		}
	}
	for (const auto& it : _post_hooks) {
		locals::hook_callbacks.erase(it.first);
		auto default_ctx = it.second.callback->GetParentRuntime()->GetDefaultContext();
		auto plugin_it = locals::plugin_hook_ids.find(default_ctx);
		if (plugin_it != locals::plugin_hook_ids.end()) {
			plugin_it->second.erase(it.first);
		}
	}
}

}