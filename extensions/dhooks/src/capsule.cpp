#include "capsule.hpp"
#include "sdk_types.hpp"

namespace dhooks {

using namespace KHook::Asm;

namespace abi {

void JIT_Align(AsmJit& jit) {
	auto realign = (16 - (jit.get_outputpos() % 16)) % 16;
	for (;realign; realign--) {
		jit.breakpoint();
	}
}

bool Proccess(sp::CallingConvention conv, std::vector<Variable>& params, ReturnVariable& ret, size_t& stack_size);
void JIT_CallMemberFunction(AsmJit& jit, bool save_general_register[MAX_GENERAL_REGISTERS], bool save_float_register[MAX_FLOAT_REGISTERS], void* this_ptr, const void* mfp, bool post);
void JIT_MakeReturn(AsmJit& jit, ReturnVariable& ret);
void JIT_CallOriginal(AsmJit& jit, ReturnVariable& ret, std::uintptr_t* original_function, std::uintptr_t* call_function);
void JIT_Recall(AsmJit& jit, bool save_general_register[MAX_GENERAL_REGISTERS], bool save_float_register[MAX_FLOAT_REGISTERS], size_t stack_size, std::uintptr_t* call_function);
}

Capsule::Capsule(void* address, sp::CallingConvention conv, const std::vector<Variable>& params, const ReturnVariable& ret) :
	_parameters(params),
	_return(ret),
	_call_conv(conv) {
	if (!abi::Proccess(conv, _parameters, _return, _stack_size)) {
		return;
	}

	// For every parameter found, and return value, save the associated registers
	for (auto& param : params) {
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
	abi::JIT_CallOriginal(_jit, _return, &_original_function, &_jit_start);

	abi::JIT_Align(_jit);
	auto offset_to_recall = _jit.get_outputpos();
	abi::JIT_Recall(_jit, _save_general_register, _save_float_register, _stack_size, &_jit_start);

	_jit.SetRE();
	_jit_start = reinterpret_cast<std::uintptr_t>(_jit.GetData());

	auto id = KHook::SetupHook(
		address,
		this,
		KHook::ExtractMFP(&Capsule::_KHook_RemovedHook),
		reinterpret_cast<void*>(_jit_start + offset_to_pre_callback),
		reinterpret_cast<void*>(_jit_start + offset_to_post_callback),
		reinterpret_cast<void*>(_jit_start + offset_to_make_return),
		reinterpret_cast<void*>(_jit_start + offset_to_call_original),
		true
	);

	if (id != KHook::INVALID_HOOK) {
		_original_function = reinterpret_cast<std::uintptr_t>(KHook::GetOriginal(address));
		_recall_function = reinterpret_cast<decltype(_recall_function)>(_jit_start + offset_to_recall);
	}
}

}