#pragma once

#include "capsule.hpp"

namespace dhooks::abi {

void JIT_Align(AsmJit& jit) {
	auto realign = (16 - (jit.get_outputpos() % 16)) % 16;
	for (;realign; realign--) {
		jit.breakpoint();
	}
}

bool AreParamsEquivalent(const std::vector<Variable>& params_lh, const std::vector<Variable>& params_rh, const ReturnVariable& return_lh, const ReturnVariable& return_rh) {
	return true;
}

bool Proccess(sp::CallingConvention conv, std::vector<Variable>& params, ReturnVariable& ret, size_t& stack_size);
void JIT_CallMemberFunction(AsmJit& jit, bool save_general_register[MAX_GENERAL_REGISTERS], bool save_float_register[MAX_FLOAT_REGISTERS], void* this_ptr, const void* mfp, bool post);
void JIT_MakeReturn(AsmJit& jit, ReturnVariable& ret);
void JIT_CallOriginal(AsmJit& jit, ReturnVariable& ret, std::uintptr_t* original_function, size_t stack_size, std::uintptr_t* jit_start);
void JIT_Recall(AsmJit& jit, bool save_general_register[MAX_GENERAL_REGISTERS], bool save_float_register[MAX_FLOAT_REGISTERS], size_t stack_size, std::uintptr_t* call_function);

}