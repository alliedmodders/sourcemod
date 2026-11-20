#include "../sp_inc.hpp"
#include "../capsule.hpp"
#include "../sdk_types.hpp"

#include <cstdlib>
#include <vector>
#include <optional>


// Reference: https://refspecs.linuxfoundation.org/elf/abi386-4.pdf
// Scalar floats return in ST(0), independently of the calling convention.

namespace dhooks::abi {

using namespace KHook::Asm;

// Those are not official types of the ABI, but they help streamlining the process
enum class TypeClass {
	VOID,
	INTEGER,
	FLOAT,
	MEMORY,
	UNKNOWN
};

const char* TypeToString(TypeClass type) {
	switch (type) {
	case TypeClass::VOID:
	return "VOID";
	case TypeClass::INTEGER:
	return "INTEGER";
	case TypeClass::FLOAT:
	return "FLOAT";
	case TypeClass::MEMORY:
	return "MEMORY";
	default:
	return "UNKNOWN";
	}
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
		return TypeClass::FLOAT;
	case sp::ReturnType_String:
	case sp::ReturnType_Vector:
		return TypeClass::MEMORY;
	case sp::ReturnType_Void:
		return TypeClass::VOID;
	default:
		break;
	}
	return {};
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
		return TypeClass::FLOAT;
	case sp::HookParamType_String:
	case sp::HookParamType_Object:
		return TypeClass::MEMORY;
	default:
		break;
	}
	return {};
}


bool Proccess(sp::CallingConvention conv, std::vector<Variable>& params, ReturnVariable& ret, size_t& stack_size) {
	ret.reg_index.reset();
	ret.float_reg_index.reset();
	ret.reg_offset = 0;

	/* Skip the return address */
	size_t stack_offset = sizeof(void*);
	size_t fastcall_registers_used = 0;

	if (ret.dhook_custom_register == sp::DHookRegister_Default) {
		auto cls = Classify_ReturnType(ret);
		if (!cls.has_value()) {
			globals::sourcemod->LogError(globals::myself, "Couldn't classify return type!");
			return false;
		}
		if (cls.value() != TypeClass::INTEGER
		&& cls.value() != TypeClass::FLOAT
		&& cls.value() != TypeClass::VOID
		&& cls.value() != TypeClass::MEMORY) {
			globals::sourcemod->LogError(globals::myself, "ABI classified return type as \"%s\", we don't know how to handle it!", TypeToString(cls.value()));
			return false;
		}

		if (cls.value() == TypeClass::MEMORY) {
			if (ret.dhook_type != sp::ReturnType_String && ret.dhook_type != sp::ReturnType_Vector) {
				globals::sourcemod->LogError(globals::myself, "ABI currently does not support custom object return!");
				return false;
			}

			// The hidden first argument consumes ECX under GCC fastcall.
			if (conv == sp::CallConv_FASTCALL) {
				ret.reg_index = ECX;
				fastcall_registers_used = 1;
			} else {
				ret.reg_index = ESP;
				ret.reg_offset = stack_offset;
				stack_offset += sizeof(void*);
			}
		}
	} else {
		// Need better support
		globals::sourcemod->LogError(globals::myself, "Custom register for return isn't supported!");
		return false;
	}

	// Only allow cdecl to specify custom registers for its arguments
	if (conv != sp::CallConv_CDECL) {
		// Otherwise consider the whole thing ill-defined and bail
		{auto len = params.size(); for (unsigned int i = 0; i < len; i++) {
			auto& param = params[i];
			if (param.dhook_custom_register != sp::DHookRegister_Default) {
				// Bail
				globals::sourcemod->LogError(globals::myself, "For safety reasons, dhooks does not allow the user to specify custom registers for arguments outside of cdecl callconv!");
				return false;
			}
		}}
	}

	// Insert a parameter at the start if this happens to be a thiscall conv
	// Under system v this makes no sense, but it's too late to undo dhooks history
	if (conv == sp::CallConv_THISCALL) {
		Variable this_ptr{};
		this_ptr.dhook_type = sp::HookParamType_Int;
		this_ptr.dhook_custom_register = sp::DHookRegister_Default;
		this_ptr.dhook_pass_flags = sp::DHookPass_ByVal;
		this_ptr.dhook_size = sizeof(void*);
		params.insert(params.begin(), this_ptr);
	}

	// Arguments occupy whole four-byte words, including bools and references.
	for (size_t i = 0; i < params.size(); ++i) {
		auto& param = params[i];
		param.reg_index.reset();
		param.float_reg_index.reset();
		param.reg_offset.reset();

		if (param.dhook_custom_register == sp::DHookRegister_Default) {
			auto cls = Classify_ParamType(param.dhook_type);
			auto size = sp::GetParamTypeSize(param.dhook_type).value_or(param.dhook_size);

			if (param.dhook_pass_flags & sp::DHookPass_ByRef) {
				// Its a pointer
				cls = TypeClass::INTEGER;
				size = sizeof(void*);
			}
			
			if (!cls.has_value()) {
				globals::sourcemod->LogError(globals::myself, "ABI could not classify parameter (%d)!", i);
				return false;
			}

			if (size == 0) {
				globals::sourcemod->LogError(globals::myself, "Parameter (%d) has a size of 0 bytes!", i);
				return false;
			}

			switch (cls.value()) {
				case TypeClass::INTEGER:
					if (conv == sp::CallConv_FASTCALL && fastcall_registers_used < 2) {
						param.reg_index = (fastcall_registers_used == 0) ? ECX : EDX;
						fastcall_registers_used++;
					} else {
						// On the stack
						param.reg_index = ESP;
						param.reg_offset = stack_offset;
						stack_offset += sizeof(int);
					}
				break;
				case TypeClass::FLOAT:
					param.reg_index = ESP;
					param.reg_offset = stack_offset;
					stack_offset += sizeof(float);
				break;
				case TypeClass::MEMORY:
					if (conv == sp::CallConv_FASTCALL && fastcall_registers_used < 2) {
						// Memory args consume fastcall registers without actually using them
						fastcall_registers_used = 2;
					}

					param.reg_index = ESP;
					param.reg_offset = stack_offset;
					stack_offset += (size + 3) & ~3;
				break;
				default:
				globals::sourcemod->LogError(globals::myself, "ABI classified parameter (%d) as %s, we don't know how to handle it!", i, TypeToString(cls.value()));
				return false;
			}
		} else {
			param.reg_index = Translate_DHookRegister(param.dhook_custom_register);
			param.float_reg_index = Translate_DHookRegister_Float(param.dhook_custom_register);

			if ((!param.reg_index && !param.float_reg_index) || (param.reg_index && param.float_reg_index)) {
				globals::sourcemod->LogError(globals::myself, "Invalid register was provided for parameter (%d)!", i);
				return false;
			}
		}
	}

	// KHook must copy the hidden return pointer as well as explicit arguments.
	stack_size = stack_offset - sizeof(void*);
	if ((conv == sp::CallConv_STDCALL || conv == sp::CallConv_FASTCALL) &&
		stack_size > UINT16_MAX) {
		globals::sourcemod->LogError(globals::myself, "stack size is too big, dhooks can't support it!");
		return false;
	}
	return true;
}

void JIT_CallMemberFunction(AsmJit& jit, bool save_general_register[MAX_GENERAL_REGISTERS], bool save_float_register[MAX_FLOAT_REGISTERS], void* this_ptr, const void* mfp, bool post) {
	jit.push(ebp);
	jit.mov(ebp, esp);

	constexpr size_t stack_size = MAX_GENERAL_REGISTERS * sizeof(int) + MAX_FLOAT_REGISTERS * sizeof(FloatRegister) + 8;
	static_assert(stack_size % 16 == 8); // System-V requires the stack to be aligned for any call operation
	// The stack size needs to be mis-aligned by 8 bytes so that we can re-align it
	jit.sub(esp, stack_size);

	for (size_t i = 0; i < MAX_GENERAL_REGISTERS; ++i) {
		auto reg = AsmReg((AsmRegCode)i);
		if (!save_general_register[i]) {
			continue;
		}
		if (reg == STACK_REG) {
			// We modified the stack, so ESP no longer holds the correct value
			jit.lea(eax, esp(stack_size + 4));
			jit.mov(esp(i * sizeof(GeneralRegister)), eax);
		} else if (reg == ebp) {
			// EBP has been modified above, so fetch its real value
			jit.mov(ebp, esp(stack_size));
			jit.mov(esp(i * sizeof(GeneralRegister)), ebp);
			jit.lea(ebp, esp(stack_size));
		}
		else {
			jit.mov(esp(i * sizeof(GeneralRegister)), reg);
		}
	}

	for (size_t i = 0; i < MAX_FLOAT_REGISTERS; i++) {
		auto reg = AsmFloatReg((AsmFloatRegCode)i);
		if (!save_float_register[i]) {
			continue;
		}
		jit.movsd(esp(i * sizeof(FloatRegister) + (MAX_GENERAL_REGISTERS * sizeof(GeneralRegister))), reg);
	}

	// void Capsule::PrePostHookLoop(std::uint8_t* saved_register, bool post)
	jit.mov(eax, esp);
	jit.sub(esp, 4);
	jit.push(post);
	jit.push(eax);
	jit.push(reinterpret_cast<std::uintptr_t>(this_ptr));
	jit.mov(eax, reinterpret_cast<std::uintptr_t>(mfp));
	jit.call(eax);
	jit.add(esp, 16);

	jit.add(esp, stack_size);
	jit.pop(ebp);
}

void JIT_MakeReturn(sp::CallingConvention conv, size_t stack_size, AsmJit& jit, ReturnVariable& ret) {
	// Re-align the stack
	jit.push(ebp);
	jit.mov(ebp, esp);

	// Make space to save data
	constexpr size_t local_data_size = 40;
	jit.sub(esp, local_data_size);

	// Grab the return pointer before it disappears from the subsequent calls
	const auto cls = Classify_ReturnType(ret).value();
	if (cls == TypeClass::MEMORY) {
		if (conv == sp::CallConv_FASTCALL) {
			jit.mov(esp(local_data_size - sizeof(void*)), ecx);
		}
		else {
			// Skip return address and ebp, grab the first param in the stack
			jit.mov(eax, esp(local_data_size + 4 + sizeof(void*)));
			jit.mov(esp(local_data_size - sizeof(void*)), eax);
		}
	}

	jit.mov(eax, reinterpret_cast<std::uintptr_t>(KHook::GetCurrentValuePtr));
	jit.mov(esp(), true); // pop = true
	jit.call(eax);

	switch (cls) {
		case TypeClass::VOID:
		// Do nothing
		break;
		case TypeClass::INTEGER:
		jit.mov(eax, eax());
		// Save EAX, we're gonna call a function which could modify EAX
		jit.mov(esp(local_data_size - sizeof(void*)), eax);
		break;
		case TypeClass::FLOAT:
		// Save EAX, we're gonna call a function which could modify EAX
		// ST(0) will be set on function exit
		jit.mov(eax, eax());
		jit.mov(esp(local_data_size - sizeof(void*)), eax);
		break;
		case TypeClass::MEMORY:
		// Presently we only support vector and string_t, custom object will have to wait
		if (ret.dhook_type != sp::ReturnType_String && ret.dhook_type != sp::ReturnType_Vector) {
			std::abort();
		}
		// We can already write the return now
		// ECX and EDX are volatile under all callconvs
		jit.mov(ecx, esp(local_data_size - sizeof(void*)));
		if (ret.dhook_type == sp::ReturnType_Vector) {
			for (size_t i = 0; i < sizeof(sdk::Vector); i += sizeof(int)) {
				jit.mov(edx, eax(i));
				jit.mov(ecx(i), edx);
			}
		} else if (ret.dhook_type == sp::ReturnType_String) {
			for (size_t i = 0; i < sizeof(sdk::string_t); i += sizeof(int)) {
				jit.mov(edx, eax(i));
				jit.mov(ecx(i), edx);
			}
		}
		break;
	default:
		std::abort();
	}


	jit.mov(eax, reinterpret_cast<std::uintptr_t>(KHook::DestroyReturnValue));
	jit.call(eax);

	if (cls == TypeClass::FLOAT) {
		jit.fld(esp(local_data_size - sizeof(void*)), x87Size::Float32);
	} else {
		jit.mov(eax, esp(local_data_size - sizeof(void*)));
	}

	// Save the return address
	jit.mov(ecx, esp(local_data_size + 4));
	jit.add(esp, local_data_size);
	jit.pop(ebp);

	if (conv == sp::CallConv_STDCALL || conv == sp::CallConv_FASTCALL) {
		jit.add(esp, stack_size);
	} else {
		// System V makes the callee pop the hidden structure-return argument
		if (cls == TypeClass::MEMORY) {
			jit.add(esp, sizeof(void*));
		}
	}

	// There's still 4 bytes of stack left, that we can use for the return address
	jit.mov(esp(), ecx);
	jit.retn();
}

void JIT_CallOriginal(sp::CallingConvention conv, AsmJit& jit, ReturnVariable& ret, std::uintptr_t* original_function, size_t stack_size, std::uintptr_t* jit_start) {
	auto start = jit.get_outputpos();

	// save eax (and re-align stack)
	jit.push(eax);

	// Ensure stack size is 8 mod 16
	// When we will push the return address it will be
	// 12 mod 16, which is how the stack should be aligned
	stack_size = ((stack_size + 15) & ~15) + 8;
	jit.sub(esp, stack_size);

	// Now copy stack over
	for (int i = 0; i < stack_size; i += sizeof(void*)) {
		// Skip saved EAX, skip return address
		jit.mov(eax, esp(stack_size + sizeof(void*) + sizeof(void*) + i));
		jit.mov(esp(i), eax);
	}

	// Setup the return address to later in this function
	jit.mov(eax, reinterpret_cast<std::uintptr_t>(jit_start));
	jit.mov(eax, eax());
	jit.add(eax, INT32_MAX);
	auto add = jit.get_outputpos();

	// Place the return address
	jit.push(eax);

	jit.mov(eax, reinterpret_cast<std::uintptr_t>(original_function));
	jit.mov(eax, eax());

	// Place the call address
	jit.push(eax);

	// Restore eax
	jit.mov(rax, esp(sizeof(void*) + sizeof(void*) + stack_size));

	// Jump to original function
	jit.retn();

	jit.rewrite<std::int32_t>(add - sizeof(std::int32_t), jit.get_outputpos());

	if (conv != sp::CallConv_STDCALL && conv != sp::CallConv_FASTCALL) {
		// Free up the stack, the provided stack_size already takes into account
		// the hidden return pointer if it exists
		jit.add(rsp, stack_size);
	}

	const auto cls = Classify_ReturnType(ret).value();
	// TO-DO
}

void JIT_Recall(sp::CallingConvention conv, AsmJit& jit, bool save_general_register[MAX_GENERAL_REGISTERS], bool save_float_register[MAX_FLOAT_REGISTERS], size_t stack_size, std::uintptr_t* jit_start) {
	// TO-DO
}

}
