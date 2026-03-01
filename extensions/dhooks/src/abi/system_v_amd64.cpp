#include "../sp_inc.hpp"
#include "../capsule.hpp"
#include "../sdk_types.hpp"

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

bool Proccess(sp::CallingConvention conv, std::vector<Variable>& params, ReturnVariable& ret, size_t& stack_size) {
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
			ret.reg_offset = {};
			return false;
		} 
		// We don't know how to handle it
		else if (!(cls.value() == INTEGER || cls.value() == SSE)) {
			return false;
		}
	} else {
		ret.reg_index = Translate_DHookRegister(ret.dhook_custom_register);
		ret.float_reg_index = Translate_DHookRegister_Float(ret.dhook_custom_register);
		ret.reg_offset = {};
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
						param.reg_offset = {};
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
						param.reg_offset = {};
						float_register_available++;
					}
				break;
				default:
				return false;
			}
		} else {
			param.reg_index = Translate_DHookRegister(param.dhook_custom_register);
			param.float_reg_index = Translate_DHookRegister_Float(param.dhook_custom_register);
			ret.reg_offset = {};
		}
	}
	/* Stack size here only means the size the parameters occupy, so remove the space occupied by return address */
	stack_size = stack_offset - sizeof(void*);
	return true;
}

void JIT_CallMemberFunction(AsmJit& jit, bool save_general_register[MAX_GENERAL_REGISTERS], bool save_float_register[MAX_FLOAT_REGISTERS], void* this_ptr, const void* mfp, bool post) {
	jit.push(rbp);
	jit.mov(rbp, rsp);

	static constexpr size_t stack_size = (MAX_GENERAL_REGISTERS * 0x8) + (MAX_FLOAT_REGISTERS * 0x10);
	static_assert(stack_size % 16 == 0); // System-V requires the stack to be aligned for any call operation
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

	for (size_t i = 1; i < MAX_FLOAT_REGISTERS; i++) {
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
}

void JIT_MakeReturn(AsmJit& jit, ReturnVariable& ret) {
	jit.push(rbp); // Re-align stack
	jit.sub(rsp, 0x10); // Make space to save data
	jit.mov(rbp, rsp);

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
			} else {
				jit.movsd(xmm0, rsp());
			}
		break;
		default:
		std::abort();
		return;
	}

	jit.add(rsp, 0x10);
	jit.pop(rbp);
	jit.retn();
}

void JIT_Recall(AsmJit& jit, bool save_general_register[MAX_GENERAL_REGISTERS], bool save_float_register[MAX_FLOAT_REGISTERS], size_t stack_size, std::uintptr_t* jit_start) {
	jit.push(rbp);
	jit.mov(rbp, rsp);

	// 1st - rdi - is ptr to function to call
	// 2nd - rsi - is ptr to registers

	if (stack_size != 0) {
		if (save_general_register[STACK_REG] == false) {
			// What the hell
			std::abort();
		}
		// Have RAX act as the previous stack
		jit.mov(rax, rsi(sizeof(GeneralRegister) * STACK_REG));

		// Prepare the stack
		jit.sub(rsp, stack_size + (16 - (stack_size % 16)) % 16);

		// Save the 2 parameters
		jit.push(rdi);
		jit.push(rsi);

		// Skip the two parameters we just saved
		jit.lea(rdi, rsp(0x8 * 2));
		// Skip return value contained in the stack
		jit.lea(rsi, rax(0x8));
		jit.mov(rdx, stack_size);

		jit.mov(rax, reinterpret_cast<std::uintptr_t>(memcpy));
		jit.call(rax);

		jit.pop(rsi);
		jit.pop(rdi);
	}

	// Prepare function to call
	jit.push(rdi);
	jit.push(rdi);

	// Figure out the return address
	jit.mov(rdi, reinterpret_cast<std::uintptr_t>(jit_start));
	jit.mov(rdi, rdi());
	jit.add(rdi, INT32_MAX);
	auto add = jit.get_outputpos();
	jit.mov(rsp(0x8), rdi);

	// Restore the registers
	for (size_t i = 1; i < MAX_FLOAT_REGISTERS; i++) {
		auto reg = AsmFloatReg((AsmFloatRegCode)i);
		if (!save_float_register[i]) {
			continue;
		}
		jit.movsd(reg, rsi(i * 0x10 + (MAX_GENERAL_REGISTERS * 0x8)));
	}

	for (size_t i = 1; i < MAX_GENERAL_REGISTERS; i++) {
		auto reg = AsmReg((AsmRegCode)i);
		if (!save_general_register[i]) {
			continue;
		}
		// Stack isn't a register to restore
		if (reg != STACK_REG && reg != rsi) {
			jit.mov(reg, rsi(sizeof(GeneralRegister) * i));
		}
	}
	// RSI is restored last
	if (save_general_register[RSI]) {
		jit.mov(rsi, rsi(sizeof(GeneralRegister) * RSI));
	}

	// Call the recall
	jit.retn();
	// Rewrite the add value
	jit.rewrite<std::int32_t>(add - sizeof(std::int32_t), jit.get_outputpos());

	jit.mov(rsp, rbp);
	jit.pop(rbp);
	jit.retn();
}

void JIT_CallOriginal(AsmJit& jit, ReturnVariable& ret, std::uintptr_t* original_function, size_t stack_size, std::uintptr_t* jit_start) {
	auto start = jit.get_outputpos();

	// save rax (and re-align stack)
	jit.push(rax);

	// Ensure stack size is aligned on 16 bytes
	stack_size = (stack_size + 15) & ~15;
	jit.sub(rsp, stack_size);

	// Now copy stack over
	for (int i = 0; i < stack_size; i += sizeof(void*)) {
		// Skip saved RAX, skip return value
		jit.mov(rax, rsp(stack_size + 0x8 + 0x8 + i));
		jit.mov(rsp(i), rax);
	}

	// Setup the return address to later in this function
	jit.mov(rax, reinterpret_cast<std::uintptr_t>(jit_start));
	jit.mov(rax, rax());
	jit.add(rax, INT32_MAX);
	auto add = jit.get_outputpos();

	// Place the return address
	jit.push(rax);

	jit.mov(rax, reinterpret_cast<std::uintptr_t>(original_function));
	jit.mov(rax, rax());

	// Place the call address
	jit.push(rax);
	// Restore rax (call addr, return addr) + stack_size
	jit.mov(rax, rsp(0x8 + 0x8 + stack_size));
	jit.retn();

	jit.rewrite<std::int32_t>(add - sizeof(std::int32_t), jit.get_outputpos());

	// Free up the stack
	jit.add(rsp, stack_size);
	jit.sub(rsp, 0x10); // Make some space for local save data

	// Stack is currently aligned
	// Store the return value in a local variable
	std::uintptr_t init_op = 0;
	std::uintptr_t deinit_op = 0;
	switch (Classify_ReturnType(ret).value()) {
		case INTEGER:
		// Store RAX
		jit.mov(rsp(), rax);

		init_op = reinterpret_cast<std::uintptr_t>(KHook::init_operator<std::uintptr_t>);
		deinit_op = reinterpret_cast<std::uintptr_t>(KHook::deinit_operator<std::uintptr_t>);
		break;
		case SSE:
			// TO-DO: handle this better...
			if (ret.dhook_type == sp::ReturnType_Vector) {
				jit.movsd(rsp(), xmm0);
				jit.movsd(rsp(0x8), xmm1);

				init_op = reinterpret_cast<std::uintptr_t>(KHook::init_operator<sdk::Vector>);
				deinit_op = reinterpret_cast<std::uintptr_t>(KHook::deinit_operator<sdk::Vector>);
			} else {
				jit.movsd(rsp(), xmm0);

				init_op = reinterpret_cast<std::uintptr_t>(KHook::init_operator<float>);
				deinit_op = reinterpret_cast<std::uintptr_t>(KHook::deinit_operator<float>);
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
	jit.mov(r8, deinit_op); // deinit_op
	jit.mov(r9, true); // original

	jit.mov(rax, reinterpret_cast<std::uintptr_t>(::KHook::SaveReturnValue));
	jit.call(rax);

	// Free local save data + RAX
	jit.add(rsp, 0x10 + 0x8);

	jit.retn();
}

}