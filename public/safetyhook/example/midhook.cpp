#include <print>

#if __has_include("Zydis/Zydis.h")
#include "Zydis/Zydis.h"
#elif __has_include("Zydis.h")
#include "Zydis.h"
#else
#error "Zydis not found"
#endif

#include <safetyhook.hpp>

SAFETYHOOK_NOINLINE int add_42(int a) {
    return a + 42;
}

void hooked_add_42(SafetyHookContext& ctx) {
#if SAFETYHOOK_ARCH_X86_64
    ctx.rax = 1337;
#elif SAFETYHOOK_ARCH_X86_32
    ctx.eax = 1337;
#endif
}

SafetyHookMid g_hook{};

int main() {
    std::println("unhooked add_42(2) = {}", add_42(2));

    // Let's disassemble add_42 and hook its RET.
    // NOTE: On Linux we should specify -falign-functions=32 to add some padding to the function otherwise we'll
    // end up hooking the next function's prologue. This is pretty hacky in general but it's just an example.
    ZydisDecoder decoder{};

#if SAFETYHOOK_ARCH_X86_64
    ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);
#elif SAFETYHOOK_ARCH_X86_32
    ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LEGACY_32, ZYDIS_STACK_WIDTH_32);
#endif

    auto ip = reinterpret_cast<uint8_t*>(add_42);

    while (*ip != 0xC3) {
        ZydisDecodedInstruction ix{};

        ZydisDecoderDecodeInstruction(&decoder, nullptr, reinterpret_cast<void*>(ip), 15, &ix);

        // Follow JMPs
        if (ix.opcode == 0xE9) {
            ip += ix.length + (int32_t)ix.raw.imm[0].value.s;
        } else {
            ip += ix.length;
        }
    }

    g_hook = safetyhook::create_mid(ip, hooked_add_42);

    std::println("hooked add_42(3) = {}", add_42(3));

    g_hook = {};

    std::println("unhooked add_42(4) = {}", add_42(4));

    return 0;
}
