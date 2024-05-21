#include <boost/ut.hpp>
#include <safetyhook.hpp>
#include <xbyak/xbyak.h>

#if SAFETYHOOK_ARCH_X86_64

using namespace std::literals;
using namespace boost::ut;
using namespace Xbyak::util;

void asciiz(Xbyak::CodeGenerator& cg, const char* str) {
    while (*str) {
        cg.db(*str++);
    }

    cg.db(0);
}

static suite<"inline hook (x64)"> inline_hook_x64_tests = [] {
    "Function with RIP-relative operand is hooked"_test = [] {
        Xbyak::CodeGenerator cg{};
        Xbyak::Label str_label{};

        cg.lea(rax, ptr[rip + str_label]);
        cg.ret();

        for (auto i = 0; i < 10; ++i) {
            cg.nop(10, false);
        }

        cg.L(str_label);
        asciiz(cg, "Hello");

        const auto fn = cg.getCode<const char* (*)()>();

        expect(eq(fn(), "Hello"sv));

        static SafetyHookInline hook;

        struct Hook {
            static const char* fn() { return "Hello, world!"; }
        };

        auto hook_result = SafetyHookInline::create(fn, Hook::fn);

        expect(hook_result.has_value());

        hook = std::move(*hook_result);

        expect(eq(fn(), "Hello, world!"sv));

        hook.reset();

        expect(eq(fn(), "Hello"sv));
    };

    "Function with no nearby memory is hooked"_test = [] {
        Xbyak::CodeGenerator cg{5'000'000'000}; // 5 GB
        Xbyak::Label start{};

#if SAFETYHOOK_OS_WINDOWS
        constexpr auto param = ecx;
#elif SAFETYHOOK_OS_LINUX
        constexpr auto param = edi;
#endif

        cg.nop(2'500'000'000, false); // 2.5 GB
        cg.L(start);
        cg.mov(dword[rsp + 8], param);
        cg.mov(eax, dword[rsp + 8]);
        cg.imul(eax, dword[rsp + 8]);
        cg.ret();

        auto fn = reinterpret_cast<int (*)(int)>(const_cast<uint8_t*>(start.getAddress()));

        expect(fn(2) == 4_i);
        expect(fn(3) == 9_i);
        expect(fn(4) == 16_i);

        static SafetyHookInline hook;

        struct Hook {
            static int fn(int a) { return hook.call<int>(a) * a; }
        };

        auto hook_result = SafetyHookInline::create(fn, Hook::fn);

        expect(hook_result.has_value());

        hook = std::move(*hook_result);

        expect(fn(2) == 8_i);
        expect(fn(3) == 27_i);
        expect(fn(4) == 64_i);

        hook.reset();

        expect(fn(2) == 4_i);
        expect(fn(3) == 9_i);
        expect(fn(4) == 16_i);
    };
};

#endif