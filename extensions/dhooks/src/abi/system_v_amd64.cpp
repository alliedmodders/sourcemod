#include "sp_inc.hpp"
#include "capsule.hpp"
#include "sdk_types.hpp"

#include <cstdlib>
#include <vector>
#include <optional>

// Reference: https://refspecs.linuxbase.org/elf/x86_64-abi-0.99.pdf

namespace dhooks::abi {

using namespace KHook::Asm;

enum TypeClass {
	VOID,
	INTEGER,
	SSE,
	SSEUP,
	X87,
	X87UP,
	COMPLEX_X87,
	NO_CLASS,
	MEMORY
};

// Chapter 3.2.3 Parameter Passing
std::optional<TypeClass> Classify_ParamType(sp::HookParamType type) {
	switch (type) {
	case sp::HookParamType_StringPtr:
	case sp::HookParamType_CharPtr:
	case sp::HookParamType_VectorPtr:
	case sp::HookParamType_Bool:
	case sp::HookParamType_Int:
	case sp::HookParamType_CBaseEntity:
	case sp::HookParamType_ObjectPtr:
	case sp::HookParamType_Edict:
		return INTEGER;
	case sp::HookParamType_Float:
		return SSE;
	default:
		break;
	}
	return {};
}

std::optional<TypeClass> Classify_ReturnType(const ReturnVariable& info) {
	if (info.dhook_size > (4 * 8)) {
		return MEMORY;
	}

	switch (info.dhook_type) {
	case sp::ReturnType_Int:
	case sp::ReturnType_Bool:
	case sp::ReturnType_String:
	case sp::ReturnType_StringPtr:
	case sp::ReturnType_CharPtr:
	case sp::ReturnType_VectorPtr:
	case sp::ReturnType_CBaseEntity:
	case sp::ReturnType_Edict:
		return INTEGER;
	case sp::ReturnType_Float:
	case sp::ReturnType_Vector: // Not true but it's fine... Must be reworked if dhook ever introduces complex types
		return SSE;
	case sp::ReturnType_Void:
		return VOID;
	default:
		break;
	}
	return {};
}

bool Process(sp::CallingConvention conv, std::vector<Variable>& params, ReturnVariable& ret, size_t& stack_size) {
	// 3.2.3 Parameter Passing
	static const AsmReg available_general_registers[] = { rdi, rsi, rdx, rcx, r8, r9 };
	static const AsmFloatReg available_float_registers[] = { xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7 };
	size_t general_register_available = 0;
	static constexpr size_t general_register_count = sizeof(available_general_registers) / sizeof(AsmReg);
	size_t float_register_available = 0;
	static constexpr size_t float_register_count = sizeof(available_float_registers) / sizeof(AsmFloatReg);

	if (ret.dhook_custom_register == sp::DHookRegister_Default) {
		auto cls = Classify_ReturnType(ret);
		if (!cls.has_value()) {
			return false;
		}
		if (cls.value() == MEMORY) {
			// RDI is used by the Return
			// Unsupported for now
			ret.reg_index = available_general_registers[general_register_available++];
			ret.reg_offset = 0;
			return false;
		} 
		// We don't know how to handle it
		else if (!(cls.value() == INTEGER || cls.value() == SSE)) {
			return false;
		}
	} else {
		ret.reg_index = Translate_DHookRegister(ret.dhook_custom_register);
		ret.float_reg_index = Translate_DHookRegister_Float(ret.dhook_custom_register);
		ret.reg_offset = 0;
		// Need better support
		return false;
	}

	// Calling conventions don't really exist in this ABI, however for easier user experience
	// We still use CallConv_THISCALL to figure whether or not we're dealing with a member func
	if (conv == sp::CallConv_THISCALL) {
		Variable this_ptr;
		this_ptr.dhook_type = sp::HookParamType_Int;
		this_ptr.dhook_custom_register = sp::DHookRegister_Default;
		this_ptr.dhook_pass_flags = sp::DHookPass_ByVal;
		this_ptr.dhook_size = sizeof(void*);
		params.insert(params.begin(), this_ptr);
	}

	/* Skip the return address */
	size_t stack_offset = sizeof(void*);
	for (auto& param : params) {
		if (param.dhook_custom_register == sp::DHookRegister_Default) {
			auto cls = Classify_ParamType(param.dhook_type);
			if (!cls.has_value()) {
				if (param.dhook_pass_flags & sp::DHookPass_ByRef) {
					// Its a pointer
					cls = INTEGER;
				} else {
					// Otherwise not supported, end
					// TO-DO: Update and support objects passed on the stack
					return false;
				}
			}
			switch (cls.value()) {
				case INTEGER:
					if (general_register_count == general_register_available) {
						// No regs left, its on the stack
						param.reg_index = RSP;
						param.reg_offset = stack_offset;
						stack_offset += sizeof(void*);
					} else {
						param.reg_index = available_general_registers[general_register_available++];
						param.reg_offset = 0;
					}
				break;
				case SSE:
					if (float_register_count == float_register_available) {
						// No regs left, its on the stack
						param.reg_index = RSP;
						param.reg_offset = stack_offset;
						stack_offset += sizeof(void*);
					} else {
						param.float_reg_index = available_float_registers[float_register_available++];
						param.reg_offset = 0;
						float_register_available++;
					}
				break;
				default:
				return false;
			}
		} else {
			param.reg_index = Translate_DHookRegister(param.dhook_custom_register);
			param.float_reg_index = Translate_DHookRegister_Float(param.dhook_custom_register);
			ret.reg_offset = 0;
		}
	}
	/* Stack size here only means the size the parameters occupy, so remove the space occupied by return address */
	stack_size = stack_offset - sizeof(void*);
}

void JIT_CallMemberFunction(AsmJit& jit, bool save_general_register[MAX_GENERAL_REGISTERS], bool save_float_register[MAX_FLOAT_REGISTERS], void* this_ptr, const void* mfp, bool post) {
	jit.push(rbp);

	static constexpr size_t stack_size = (MAX_GENERAL_REGISTERS * 0x8) + (MAX_FLOAT_REGISTERS * 0x10);
	jit.sub(rsp, stack_size);

	// Independently of anything else, RAX gets saved always
	jit.mov(rsp(), rax);
	for (size_t i = 1; i < MAX_GENERAL_REGISTERS; i++) {
		auto reg = AsmReg((AsmRegCode)i);
		if (!save_general_register[i]) {
			continue;
		}
		if (reg == STACK_REG) {
			// We modified the stack, so RSP no longer holds the correct value
			jit.lea(rax, rsp(stack_size));
			jit.mov(rsp(i * 0x8), rax);
		} else {
			jit.mov(rsp(i * 0x8), reg);
		}
	}

	for (size_t i = 1; i < MAX_GENERAL_REGISTERS; i++) {
		auto reg = AsmFloatReg((AsmFloatRegCode)i);
		if (!save_float_register[i]) {
			continue;
		}
		jit.movsd(rsp(i * 0x10 + (MAX_GENERAL_REGISTERS * 0x8)), reg);
	}
	// void Capsule::PrePostHookLoop(std::uint8_t* saved_register, bool post)
	jit.mov(rdx, post);
	jit.mov(rsi, rsp);
	jit.mov(rdi, reinterpret_cast<std::uintptr_t>(this_ptr));
	jit.mov(rax, reinterpret_cast<std::uintptr_t>(mfp));
	jit.call(rax);

	jit.add(rsp, stack_size);
	jit.pop(rbp);

	jit.retn();
}

void JIT_MakeReturn(AsmJit& jit, ReturnVariable& ret) {
	jit.sub(rsp, 0x8); // Re-align stack
	jit.mov(rax, reinterpret_cast<std::uintptr_t>(::KHook::GetCurrentValuePtr));
	jit.mov(rdi, true); // Pop
	jit.call(rax);
	// RAX now contains the return value ptr
	switch (Classify_ReturnType(ret).value()) {
		case MEMORY:
		// Currently unsupported
		std::abort();
		break;
		case INTEGER:
		// At the present time, dhook has trivial INTEGER types
		// However this will have to be revised to MOV on RDI too
		// if there's an INTEGER return type that exceeds 8 bytes
		jit.mov(rax, rax());
		// Save RAX, we're gonna call a function which could modify rax
		jit.mov(rsp(), rax);
		break;
		case SSE:
			// TO-DO: handle this better...
			if (ret.dhook_type == sp::ReturnType_Vector) {
				jit.sub(rsp, 0x10);
				jit.movsd(xmm0, rax());
				jit.movsd(rsp(), xmm0);
				jit.movsd(xmm1, rax(0x8));
				jit.movsd(rsp(0x8), xmm1);
			} else {
				jit.movsd(xmm0, rax());
				jit.movsd(rsp(), xmm0);
			}
		break;
		default:
		std::abort();
		return;
	}

	jit.mov(rax, reinterpret_cast<std::uintptr_t>(::KHook::DestroyReturnValue));
	jit.call(rax);

	switch (Classify_ReturnType(ret).value()) {
		case INTEGER:
		// Restore RAX
		jit.mov(rax, rsp());
		break;
		case SSE:
			// TO-DO: handle this better...
			if (ret.dhook_type == sp::ReturnType_Vector) {
				jit.movsd(xmm0, rsp());
				jit.movsd(xmm1, rsp(0x8));
				jit.add(rsp, 0x10);
			} else {
				jit.movsd(xmm0, rsp());
			}
		break;
		default:
		std::abort();
		return;
	}

	jit.add(rsp, 0x8);
	jit.retn();
}

void JIT_CallOriginal(AsmJit& jit, ReturnVariable& ret, std::uintptr_t* original_function, std::uintptr_t* call_function) {
	auto start = jit.get_outputpos();

	jit.push(rsp()); // Save return address
	jit.push(rsp());

	jit.mov(r15, reinterpret_cast<std::uintptr_t>(call_function));
	jit.mov(r15, r15());
	jit.add(r15, INT32_MAX);
	auto add = jit.get_outputpos();

	// Change return address to later in this function
	jit.mov(rsp(0x10), r15);

	jit.mov(r15, reinterpret_cast<std::uintptr_t>(original_function));
	jit.mov(r15, r15());
	jit.mov(rsp(0x8), r15);
	// Save the original return address in r15, KHook saves that register and ABI promises not to use it
	jit.pop(r15);
	// Now go to original function
	jit.retn();

	// Rewrite the add r15, X - operation
	jit.rewrite<std::int32_t>(add - sizeof(std::int32_t), jit.get_outputpos() - start);

	// Stack is currently aligned
	// Store the return value in a local variable
	auto local_size = 0;
	std::uintptr_t init_op = 0;
	std::uintptr_t delete_op = 0;
	switch (Classify_ReturnType(ret).value()) {
		case INTEGER:
		// Store RAX
		local_size = 0x10;
		jit.sub(rsp, local_size);
		jit.mov(rsp(), rax);

		init_op = reinterpret_cast<std::uintptr_t>(KHook::init_operator<std::uintptr_t>);
		delete_op = reinterpret_cast<std::uintptr_t>(KHook::delete_operator<std::uintptr_t>);
		break;
		case SSE:
			local_size = 0x10;
			jit.sub(rsp, local_size);
			// TO-DO: handle this better...
			if (ret.dhook_type == sp::ReturnType_Vector) {
				jit.movsd(rsp(), xmm0);
				jit.movsd(rsp(0x8), xmm1);

				init_op = reinterpret_cast<std::uintptr_t>(KHook::init_operator<sdk::Vector>);
				delete_op = reinterpret_cast<std::uintptr_t>(KHook::delete_operator<sdk::Vector>);
			} else {
				jit.movsd(rsp(), xmm0);

				init_op = reinterpret_cast<std::uintptr_t>(KHook::init_operator<float>);
				delete_op = reinterpret_cast<std::uintptr_t>(KHook::delete_operator<float>);
			}
		break;
		default:
		std::abort();
		return;
	}

	// KHook::SaveReturnValue(KHook::Action action, void* ptr_to_return, std::size_t return_size, void* init_op, void* delete_op, bool original)
	jit.mov(rdi, (std::uint8_t)KHook::Action::Ignore); // action
	jit.mov(rsi, rsp); // ptr_to_return
	jit.mov(rdx, ret.dhook_size); // return_size
	jit.mov(rcx, init_op); // init_op
	jit.mov(r8, delete_op); // delete_op
	jit.mov(r9, true); // original

	jit.mov(rax, reinterpret_cast<std::uintptr_t>(::KHook::SaveReturnValue));
	jit.call(rax);

	// Free local variable
	jit.add(rsp, local_size);

	// The return address should still be in r15
	jit.push(r15);
	// We don't have to worry about re-aligning the stack (though it should be fine)
	// KHook will align it on its own
	jit.retn();
}

}