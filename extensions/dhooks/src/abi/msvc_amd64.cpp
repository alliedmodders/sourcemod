#include "../sp_inc.hpp"
#include "../capsule.hpp"
#include "../sdk_types.hpp"

#include <cstdlib>
#include <vector>
#include <optional>

// Reference: https://learn.microsoft.com/en-us/cpp/build/x64-calling-convention
// KHook copies stack_size bytes from entry rsp + 8, shadow space included

namespace dhooks::abi {

using namespace KHook::Asm;

// windows.h defines VOID, get rid of it
#undef VOID

enum class TypeClass {
	VOID,
	INTEGER,
	SSE,
	MEMORY
};

const char* TypeToString(TypeClass type) {
	switch (type) {
	case TypeClass::VOID:
		return "VOID";
	case TypeClass::INTEGER:
		return "INTEGER";
	case TypeClass::SSE:
		return "SSE";
	case TypeClass::MEMORY:
		return "MEMORY";
	default:
		return "UNKNOWN";
	}
}

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
		return TypeClass::INTEGER;
	case sp::HookParamType_Float:
		return TypeClass::SSE;
	default:
		break;
	}
	return {};
}

std::optional<TypeClass> Classify_ReturnType(const ReturnVariable& info) {
	switch (info.dhook_type) {
	case sp::ReturnType_Int:
	case sp::ReturnType_Bool:
	case sp::ReturnType_StringPtr:
	case sp::ReturnType_CharPtr:
	case sp::ReturnType_VectorPtr:
	case sp::ReturnType_CBaseEntity:
	case sp::ReturnType_Edict:
		return TypeClass::INTEGER;
	case sp::ReturnType_Float:
		return TypeClass::SSE;
	case sp::ReturnType_String:
		// string_t isn't a POD, MSVC hands it back through a hidden pointer
		return TypeClass::MEMORY;
	case sp::ReturnType_Vector:
		// 12 bytes, too big to come back in RAX
		return TypeClass::MEMORY;
	case sp::ReturnType_Void:
		return TypeClass::VOID;
	default:
		break;
	}
	return {};
}

// Args are positional, ints and floats share the same 4 slots
static const AsmReg available_general_registers[] = { rcx, rdx, r8, r9 };
static const AsmFloatReg available_float_registers[] = { xmm0, xmm1, xmm2, xmm3 };
static constexpr size_t positional_register_count = sizeof(available_general_registers) / sizeof(AsmReg);

static constexpr size_t SHADOW_SPACE = 32;

bool Proccess(sp::CallingConvention conv, std::vector<Variable>& params, ReturnVariable& ret, size_t& stack_size) {
	bool return_in_memory = false;

	if (ret.dhook_custom_register == sp::DHookRegister_Default) {
		auto cls = Classify_ReturnType(ret);
		if (!cls.has_value()) {
			globals::sourcemod->LogError(globals::myself, "Couldn't classify return type!");
			return false;
		}
		if (cls.value() == TypeClass::MEMORY) {
			return_in_memory = true;
		} else if (cls.value() != TypeClass::INTEGER
			&& cls.value() != TypeClass::SSE
			&& cls.value() != TypeClass::VOID) {
			globals::sourcemod->LogError(globals::myself, "ABI classified return type as \"%s\", we don't know how to handle it!", TypeToString(cls.value()));
			return false;
		}
	} else {
		ret.reg_index = Translate_DHookRegister(ret.dhook_custom_register);
		ret.float_reg_index = Translate_DHookRegister_Float(ret.dhook_custom_register);
		ret.reg_offset = {};
		// Need better support
		globals::sourcemod->LogError(globals::myself, "Custom register for return isn't supported!");
		return false;
	}

	size_t position = 0;

	// Calling conventions don't really exist in this ABI, however for easier user experience
	// We still use CallConv_THISCALL to figure whether or not we're dealing with a member func
	bool has_this = (conv == sp::CallConv_THISCALL);
	if (has_this) {
		Variable this_ptr;
		this_ptr.dhook_type = sp::HookParamType_Int;
		this_ptr.dhook_custom_register = sp::DHookRegister_Default;
		this_ptr.dhook_pass_flags = sp::DHookPass_ByVal;
		this_ptr.dhook_size = sizeof(void*);
		params.insert(params.begin(), this_ptr);
	}

	// this comes first, then the hidden return pointer
	if (has_this) {
		Variable& this_ptr = params[0];
		this_ptr.reg_index = available_general_registers[position];
		this_ptr.float_reg_index = {};
		this_ptr.reg_offset = {};
		position++;
	}
	if (return_in_memory) {
		ret.reg_index = available_general_registers[position];
		ret.float_reg_index = {};
		ret.reg_offset = 0;
		position++;
	} else {
		ret.reg_offset = 0;
	}

	size_t stack_args = 0;

	{auto len = params.size(); for (unsigned int i = (has_this ? 1 : 0); i < len; i++) {
		auto& param = params[i];
		if (param.dhook_custom_register == sp::DHookRegister_Default) {
			auto cls = Classify_ParamType(param.dhook_type);
			if (!cls.has_value()) {
				if (param.dhook_pass_flags & sp::DHookPass_ByRef) {
					// Its a pointer
					cls = TypeClass::INTEGER;
				} else {
					// Otherwise not supported, end
					// TO-DO: Update and support objects passed on the stack
					globals::sourcemod->LogError(globals::myself, "ABI could not classify parameter (%d)!", i);
					return false;
				}
			}

			if (cls.value() != TypeClass::INTEGER && cls.value() != TypeClass::SSE) {
				globals::sourcemod->LogError(globals::myself, "ABI classified parameter (%d) as %s, we don't know how to handle it!", i, TypeToString(cls.value()));
				return false;
			}

			if (position < positional_register_count) {
				if (cls.value() == TypeClass::INTEGER) {
					param.reg_index = available_general_registers[position];
					param.float_reg_index = {};
				} else {
					param.float_reg_index = available_float_registers[position];
					param.reg_index = {};
				}
				param.reg_offset = {};
			} else {
				// No regs left, its on the stack
				param.reg_index = RSP;
				// Skip the saved stack pointer, the return address and the shadow space
				param.reg_offset = (2 * sizeof(void*)) + SHADOW_SPACE + (stack_args * sizeof(void*));
				param.float_reg_index = {};
				stack_args++;
			}
			position++;
		} else {
			param.reg_index = Translate_DHookRegister(param.dhook_custom_register);
			param.float_reg_index = Translate_DHookRegister_Float(param.dhook_custom_register);
			param.reg_offset = {};
		}
	}}

	stack_size = SHADOW_SPACE + (stack_args * sizeof(void*));
	return true;
}

void JIT_CallMemberFunction(AsmJit& jit, bool save_general_register[MAX_GENERAL_REGISTERS], bool save_float_register[MAX_FLOAT_REGISTERS], void* this_ptr, const void* mfp, bool post) {
	jit.push(rbp);
	jit.mov(rbp, rsp);

	static constexpr size_t save_area = (MAX_GENERAL_REGISTERS * 0x8) + (MAX_FLOAT_REGISTERS * 0x10);
	static_assert(save_area % 16 == 0); // Windows requires the stack to be aligned for any call operation
	jit.sub(rsp, save_area);

	// Independently of anything else, RAX gets saved always
	jit.mov(rsp(), rax);
	for (size_t i = 1; i < MAX_GENERAL_REGISTERS; i++) {
		auto reg = AsmReg((AsmRegCode)i);
		if (!save_general_register[i]) {
			continue;
		}
		if (reg == STACK_REG) {
			// We modified the stack, so RSP no longer holds the correct value
			// RBP is entry RSP - 8, which is what we want anyway
			jit.mov(rsp(i * 0x8), rbp);
		} else {
			jit.mov(rsp(i * 0x8), reg);
		}
	}

	for (size_t i = 0; i < MAX_FLOAT_REGISTERS; i++) {
		auto reg = AsmFloatReg((AsmFloatRegCode)i);
		if (!save_float_register[i]) {
			continue;
		}
		jit.movsd(rsp(i * 0x10 + (MAX_GENERAL_REGISTERS * 0x8)), reg);
	}

	// void Capsule::PrePostHookLoop(std::uint8_t* saved_register, bool post)
	jit.mov(r8, post);
	jit.mov(rdx, rsp);
	jit.mov(rcx, reinterpret_cast<std::uintptr_t>(this_ptr));
	jit.sub(rsp, SHADOW_SPACE);
	jit.mov(rax, reinterpret_cast<std::uintptr_t>(mfp));
	jit.call(rax);

	jit.mov(rsp, rbp);
	jit.pop(rbp);
}

void JIT_MakeReturn(AsmJit& jit, ReturnVariable& ret) {
	auto cls = Classify_ReturnType(ret).value();

	jit.push(rbp);
	jit.sub(rsp, 0x40); // 0x20 shadow + 0x20 locals
	jit.mov(rbp, rsp);

	// KHook restored the entry registers for us, the return buffer is still where it was
	if (cls == TypeClass::MEMORY) {
		AsmReg sret_reg = AsmReg((AsmRegCode)ret.reg_index.value());
		jit.mov(rbp(0x30), sret_reg);
	}

	jit.mov(rcx, true); // Pop
	jit.mov(rax, reinterpret_cast<std::uintptr_t>(::KHook::GetCurrentValuePtr));
	jit.call(rax);
	// RAX now contains the return value ptr

	switch (cls) {
		case TypeClass::VOID:
		break;
		case TypeClass::INTEGER:
		jit.mov(rax, rax());
		// Save RAX, we're gonna call a function which could modify rax
		jit.mov(rbp(0x20), rax);
		break;
		case TypeClass::SSE:
		jit.movsd(xmm0, rax());
		jit.movsd(rbp(0x20), xmm0);
		break;
		case TypeClass::MEMORY:
		// Copy the object into the caller's buffer
		jit.mov(rcx, rbp(0x30));
		jit.movsd(xmm0, rax());
		jit.movsd(rcx(), xmm0);
		if (ret.dhook_size > 8) {
			jit.movsd(xmm1, rax(ret.dhook_size - 8));
			jit.movsd(rcx(ret.dhook_size - 8), xmm1);
		}
		break;
		default:
		std::abort();
		return;
	}

	jit.mov(rax, reinterpret_cast<std::uintptr_t>(::KHook::DestroyReturnValue));
	jit.call(rax);

	switch (cls) {
		case TypeClass::VOID:
		break;
		case TypeClass::INTEGER:
		jit.mov(rax, rbp(0x20));
		break;
		case TypeClass::SSE:
		jit.movsd(xmm0, rbp(0x20));
		break;
		case TypeClass::MEMORY:
		jit.mov(rax, rbp(0x30)); // Hand the buffer back in RAX
		break;
		default:
		std::abort();
		return;
	}

	jit.add(rsp, 0x40);
	jit.pop(rbp);
	jit.retn();
}

void JIT_Recall(AsmJit& jit, bool save_general_register[MAX_GENERAL_REGISTERS], bool save_float_register[MAX_FLOAT_REGISTERS], size_t stack_size, std::uintptr_t* jit_start) {
	jit.push(rbp);
	jit.mov(rbp, rsp);

	// 1st - rcx - is ptr to function to call
	// 2nd - rdx - is ptr to registers
	// Stash both, we're about to overwrite the arg registers
	jit.mov(r10, rcx);
	jit.mov(r11, rdx);

	jit.sub(rsp, stack_size + (16 - (stack_size % 16)) % 16);

	// Only worth rebuilding the stack if something actually spilled
	if (stack_size > SHADOW_SPACE) {
		if (save_general_register[STACK_REG] == false) {
			// What the hell
			std::abort();
		}
		// Have RAX act as the previous stack
		jit.mov(rax, r11(sizeof(GeneralRegister) * STACK_REG));

		// Save the 2 parameters, then open the shadow space
		jit.push(r10);
		jit.push(r11);
		jit.sub(rsp, SHADOW_SPACE);

		// Skip the two parameters we just saved and the shadow space
		jit.lea(rcx, rsp(SHADOW_SPACE + (2 * sizeof(void*))));
		// Skip return value contained in the stack
		jit.lea(rdx, rax(2 * sizeof(void*)));
		jit.mov(r8, stack_size);

		jit.mov(rax, reinterpret_cast<std::uintptr_t>(memcpy));
		jit.call(rax);

		jit.add(rsp, SHADOW_SPACE);
		jit.pop(r11);
		jit.pop(r10);
	}

	// Prepare function to call
	jit.push(r10);
	jit.push(r10);

	// Figure out the return address
	jit.mov(rax, reinterpret_cast<std::uintptr_t>(jit_start));
	jit.mov(rax, rax());
	jit.add(rax, INT32_MAX);
	auto add = jit.get_outputpos();
	jit.mov(rsp(0x8), rax);

	// Restore the registers
	for (size_t i = 0; i < MAX_FLOAT_REGISTERS; i++) {
		auto reg = AsmFloatReg((AsmFloatRegCode)i);
		if (!save_float_register[i]) {
			continue;
		}
		jit.movsd(reg, r11(i * 0x10 + (MAX_GENERAL_REGISTERS * 0x8)));
	}

	for (size_t i = 1; i < MAX_GENERAL_REGISTERS; i++) {
		auto reg = AsmReg((AsmRegCode)i);
		if (!save_general_register[i]) {
			continue;
		}
		// Stack isn't a register to restore, R11 still holds the buffer
		if (reg != STACK_REG && reg != r11) {
			jit.mov(reg, r11(sizeof(GeneralRegister) * i));
		}
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
	auto cls = Classify_ReturnType(ret).value();

	// save rax (and re-align stack)
	jit.push(rax);

	// Ensure stack size is aligned on 16 bytes
	stack_size = (stack_size + 15) & ~15;
	jit.sub(rsp, stack_size);

	// Now copy stack over
	for (size_t i = 0; i < stack_size; i += sizeof(void*)) {
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
	// Make some space for local save data, the stack args and the shadow space
	jit.sub(rsp, 0x40);

	// Store the return value in a local variable
	std::uintptr_t init_op = 0;
	std::uintptr_t deinit_op = 0;
	switch (cls) {
		case TypeClass::VOID:
		break;
		case TypeClass::INTEGER:
		// Store RAX
		jit.mov(rsp(0x30), rax);
		init_op = reinterpret_cast<std::uintptr_t>(KHook::init_operator<std::uintptr_t>);
		deinit_op = reinterpret_cast<std::uintptr_t>(KHook::deinit_operator<std::uintptr_t>);
		break;
		case TypeClass::SSE:
		jit.movsd(rsp(0x30), xmm0);
		init_op = reinterpret_cast<std::uintptr_t>(KHook::init_operator<float>);
		deinit_op = reinterpret_cast<std::uintptr_t>(KHook::deinit_operator<float>);
		break;
		case TypeClass::MEMORY:
		// RAX points at the returned object
		jit.movsd(xmm0, rax());
		jit.movsd(rsp(0x30), xmm0);
		if (ret.dhook_size > 8) {
			jit.movsd(xmm1, rax(ret.dhook_size - 8));
			jit.movsd(rsp(0x30 + (ret.dhook_size - 8)), xmm1);
		}
		if (ret.dhook_type == sp::ReturnType_String) {
			init_op = reinterpret_cast<std::uintptr_t>(KHook::init_operator<sdk::string_t>);
			deinit_op = reinterpret_cast<std::uintptr_t>(KHook::deinit_operator<sdk::string_t>);
		} else {
			init_op = reinterpret_cast<std::uintptr_t>(KHook::init_operator<sdk::Vector>);
			deinit_op = reinterpret_cast<std::uintptr_t>(KHook::deinit_operator<sdk::Vector>);
		}
		break;
		default:
		std::abort();
		return;
	}

	// KHook::SaveReturnValue(KHook::Action action, void* ptr_to_return, std::size_t return_size, void* init_op, void* delete_op, bool original)
	// 5th and 6th arguments go on the stack
	jit.mov(rcx, (std::uint8_t)KHook::Action::Ignore); // action
	if (cls == TypeClass::VOID) {
		jit.mov(rdx, 0x0); // ptr_to_return
		jit.mov(r8, 0x0);  // return_size
		jit.mov(r9, 0x0);  // init_op
		jit.mov(rsp(0x20), 0x0); // deinit_op
	} else {
		jit.lea(rdx, rsp(0x30));      // ptr_to_return
		jit.mov(r8, ret.dhook_size);  // return_size
		jit.mov(r9, init_op);         // init_op
		jit.mov(rax, deinit_op);
		jit.mov(rsp(0x20), rax);      // deinit_op
	}
	jit.mov(rax, 1);
	jit.mov(rsp(0x28), rax); // original

	jit.mov(rax, reinterpret_cast<std::uintptr_t>(::KHook::SaveReturnValue));
	jit.call(rax);

	// Free local save data + RAX
	jit.add(rsp, 0x40 + 0x8);

	jit.retn();
}

}
