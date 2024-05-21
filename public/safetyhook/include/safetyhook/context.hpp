/// @file safetyhook/context.hpp
/// @brief Context structure for MidHook.

#pragma once

#ifndef SAFETYHOOK_USE_CXXMODULES
#include <cstdint>
#else
import std.compat;
#endif

#include "safetyhook/common.hpp"

namespace safetyhook {
union Xmm {
    uint8_t u8[16];
    uint16_t u16[8];
    uint32_t u32[4];
    uint64_t u64[2];
    float f32[4];
    double f64[2];
};

/// @brief Context structure for 64-bit MidHook.
/// @details This structure is used to pass the context of the hooked function to the destination allowing full access
/// to the 64-bit registers at the moment the hook is called.
/// @note rip will point to a trampoline containing the replaced instruction(s).
/// @note rsp is read-only. Modifying it will have no effect. Use trampoline_rsp to modify rsp if needed but make sure
/// the top of the stack is the rip you want to resume at.
struct Context64 {
    Xmm xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm8, xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15;
    uintptr_t rflags, r15, r14, r13, r12, r11, r10, r9, r8, rdi, rsi, rdx, rcx, rbx, rax, rbp, rsp, trampoline_rsp, rip;
};

/// @brief Context structure for 32-bit MidHook.
/// @details This structure is used to pass the context of the hooked function to the destination allowing full access
/// to the 32-bit registers at the moment the hook is called.
/// @note eip will point to a trampoline containing the replaced instruction(s).
/// @note esp is read-only. Modifying it will have no effect. Use trampoline_esp to modify esp if needed but make sure
/// the top of the stack is the eip you want to resume at.
struct Context32 {
    Xmm xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7;
    uintptr_t eflags, edi, esi, edx, ecx, ebx, eax, ebp, esp, trampoline_esp, eip;
};

/// @brief Context structure for MidHook.
/// @details This structure is used to pass the context of the hooked function to the destination allowing full access
/// to the registers at the moment the hook is called.
/// @note The structure is different depending on architecture.
/// @note The structure only provides access to integer registers.
#if SAFETYHOOK_ARCH_X86_64
using Context = Context64;
#elif SAFETYHOOK_ARCH_X86_32
using Context = Context32;
#endif

} // namespace safetyhook