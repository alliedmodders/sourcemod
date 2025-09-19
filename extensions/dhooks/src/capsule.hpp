#pragma once

#include "register.hpp"
#include "sp_inc.hpp"
#include "globals.hpp"
#include "handle.hpp"
#include "hook.hpp"

#if defined(DHOOKS_X86_64)
#include <khook/asm/x86_64.hpp>
#elif defined(DHOOKS_X86)
#include <khook/asm/x86.hpp>
#endif
#include <khook.hpp>

#include <vector>
#include <optional>
#include <memory>
#include <stack>
#include <unordered_set>

namespace dhooks {

#if defined(DHOOKS_X86_64)
using AsmJit = KHook::Asm::x86_64_Jit;
#elif defined(DHOOKS_X86)
using AsmJit = KHook::Asm::x86_Jit;
#endif

struct Variable {
public:
    enum Alignment {
        OneByte = 1,
        TwoBytes = 2,
        FourBytes = 4,
        EightBytes = 8,
        SixteenBytes = 16
    };

    sp::HookParamType dhook_type;
    size_t dhook_size;
    sp::DHookPassFlag dhook_pass_flags;
    sp::DHookRegister dhook_custom_register;

    // Provided by the ABI, used by the JIT
    // If DHook is to ever support complex type, transform those fields
    // into an array so that the SP natives know where to look when offsetting into an object
    std::optional<AsmRegCode> reg_index;
    std::optional<AsmFloatRegCode> float_reg_index;
    std::optional<size_t> reg_offset;
    Alignment alignment;
    // Atm it's exactly the same as _dhook_size
    // We differentiate it anyways, to avoid having to refactor the JIT
    size_t type_size;
};

struct ReturnVariable {
public:
    enum Alignment {
        OneByte = 1,
        TwoBytes = 2,
        FourBytes = 4,
        EightBytes = 8,
        SixteenBytes = 16
    };

    sp::ReturnType dhook_type;
    size_t dhook_size;
    sp::DHookPassFlag dhook_pass_flags;
    sp::DHookRegister dhook_custom_register;

    std::optional<AsmRegCode> reg_index;
    std::optional<AsmFloatRegCode> float_reg_index;
    size_t reg_offset;
};

struct HookCallback;
class Capsule {
public:
    Capsule(void* address, sp::CallingConvention conv, const std::vector<Variable>& params, const ReturnVariable& ret);

    const std::vector<Variable>& GetParameters() const { return _parameters; };
    const ReturnVariable& GetReturn() const { return _return; };
    const sp::CallingConvention GetCallConv() const { return _call_conv; };
protected:
    //void JIT_SaveRegisters(AsmJit& jit);
    void JIT_RestoreRegisters(AsmJit& jit);

    template<typename T>
    void PrePostHookLoop(std::uint8_t* saved_register, bool post) const;
protected:
    void _KHook_RemovedHook(KHook::HookID_t id);

    // Parameters for the Capsule
    std::vector<Variable> _parameters;
    ReturnVariable _return;

#if defined(DHOOKS_X86_64)
    static_assert(KHook::Asm::R15 == (MAX_GENERAL_REGISTERS - 1), "Mismatch in register index");
    static_assert(KHook::Asm::XMM15 == (MAX_FLOAT_REGISTERS - 1), "Mismatch in register index");
#elif defined(DHOOKS_X86)
    static_assert(KHook::Asm::RSP == (MAX_GENERAL_REGISTERS - 1), "Mismatch in register index");
    static_assert(KHook::Asm::XMM7 == (MAX_FLOAT_REGISTERS - 1), "Mismatch in register index");
#endif

    // Registers we should save upon function entry
    bool _save_general_register[MAX_GENERAL_REGISTERS];
    // Registers we should save upon function entry
    bool _save_float_register[MAX_FLOAT_REGISTERS];

    // First is general registers, second is float registers
    std::stack<std::pair<std::unique_ptr<std::uint8_t[]>, std::unique_ptr<std::uint8_t[]>>> _saved_registers;

    AsmJit _jit;
    size_t _stack_size;
    std::uintptr_t _original_function;
    std::uintptr_t _jit_start;
    void (*_recall_function)(void* recall_func, std::uint8_t* saved_register);

    std::unordered_set<HookCallback*> _pre_hooks;
    std::unordered_set<HookCallback*> _post_hooks;

    // Only for dhook natives
    sp::CallingConvention _call_conv;
};

template<typename RETURN>
void Capsule::PrePostHookLoop(std::uint8_t* saved_register, bool post) const {
	RETURN* return_ptr = nullptr;
	RETURN* temp_ptr = nullptr;
	void* init_op = nullptr;
	void* delete_op = nullptr;
	size_t return_size = 0;

    if constexpr(!std::is_same<RETURN, void>::value) {	
		return_ptr = new RETURN;
        temp_ptr = new RETURN;
		init_op = reinterpret_cast<void*>(::KHook::init_operator<RETURN>);
		delete_op = reinterpret_cast<void*>(::KHook::delete_operator<RETURN>);
		return_size = sizeof(RETURN);
	}

	KHook::Action final_action = KHook::Action::Ignore;
	// Only detours executing on the main thread can fire dhooks' callback
	// Most notably because the SP VM isn't thread safe
	if (std::this_thread::get_id() == globals::main_thread) {
        handle::ParamReturn paramret(
            this,
            reinterpret_cast<GeneralRegister*>(saved_register),
            reinterpret_cast<FloatRegister*>(saved_register + sizeof(GeneralRegister) * MAX_GENERAL_REGISTERS),
            return_ptr
        );

        // Save some time and pre-save a this pointer (if it even exists)
        void* this_ptr = nullptr;
        std::optional<cell_t> entity_index;
        if (_parameters.size() != 0) {
            this_ptr = *(paramret.Get<void*>(0));
        }

        // Make deep-copy to allow deletion of hooks under callbacks
        auto hooks = (post) ? _post_hooks : _pre_hooks;
        for (auto hook : hooks) {
            if (hook->callback->IsRunnable()) {
                if (hook->this_pointer_type != sp::ThisPointer_Ignore) {
                    // Push the associated this pointer
                    if (hook->this_pointer_type == sp::ThisPointer_CBaseEntity) {
                        if (!entity_index) {
                            entity_index = globals::gamehelpers->EntityToBCompatRef(reinterpret_cast<CBaseEntity*>(this_ptr));
                        }
                        hook->callback->PushCell(entity_index.value_or(-1));
                    } else if (hook->this_pointer_type == sp::ThisPointer_Address) {
                        hook->callback->PushCell(globals::sourcemod->ToPseudoAddress(this_ptr));
                    }
                }

                if (_return.dhook_type != sp::ReturnType_Void) {
                    hook->callback->PushCell(paramret);
                }

                if (_parameters.size() != 0) {
                    hook->callback->PushCell(paramret);
                }

                cell_t result = (cell_t)sp::MRES_Ignored;
                hook->callback->Execute(&result);

                if (result == (cell_t)sp::MRES_Supercede) {
                    final_action = KHook::Action::Supersede;
                } else if (result != (cell_t)sp::MRES_Ignored) {
                    final_action = KHook::Action::Override;
                }
                // Params change, perform recall
                if (result == (cell_t)sp::MRES_ChangedHandled || result == (cell_t)sp::MRES_ChangedOverride) {
                    void* recall_func = KHook::DoRecall(final_action, return_ptr, return_size, init_op, delete_op);
                    (*_recall_function)(recall_func, saved_register);
                }
            }
        }
	}
	KHook::SaveReturnValue(final_action, return_ptr, return_size, init_op, delete_op, false);
}

}