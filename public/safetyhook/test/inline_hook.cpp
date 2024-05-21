#include <thread>

#include <boost/ut.hpp>
#include <safetyhook.hpp>
#include <xbyak/xbyak.h>

using namespace std::literals;
using namespace boost::ut;
using namespace Xbyak::util;

static suite<"inline hook"> inline_hook_tests = [] {
    "Function hooked multiple times"_test = [] {
        struct Target {
            SAFETYHOOK_NOINLINE static std::string fn(std::string name) { return "hello " + name; }
        };

        expect(eq(Target::fn("world"), "hello world"sv));

        // First hook.
        static SafetyHookInline hook0;

        struct Hook0 {
            static std::string fn(std::string name) { return hook0.call<std::string>(name + " and bob"); }
        };

        auto hook0_result = SafetyHookInline::create(Target::fn, Hook0::fn);

        expect(hook0_result.has_value());

        hook0 = std::move(*hook0_result);

        expect(eq(Target::fn("world"), "hello world and bob"sv));

        // Second hook.
        static SafetyHookInline hook1;

        struct Hook1 {
            static std::string fn(std::string name) { return hook1.call<std::string>(name + " and alice"); }
        };

        auto hook1_result = SafetyHookInline::create(Target::fn, Hook1::fn);

        expect(hook1_result.has_value());

        hook1 = std::move(*hook1_result);

        expect(eq(Target::fn("world"), "hello world and alice and bob"sv));

        // Third hook.
        static SafetyHookInline hook2;

        struct Hook2 {
            static std::string fn(std::string name) { return hook2.call<std::string>(name + " and eve"); }
        };

        auto hook2_result = SafetyHookInline::create(Target::fn, Hook2::fn);

        expect(hook2_result.has_value());

        hook2 = std::move(*hook2_result);

        expect(eq(Target::fn("world"), "hello world and eve and alice and bob"sv));

        // Fourth hook.
        static SafetyHookInline hook3;

        struct Hook3 {
            static std::string fn(std::string name) { return hook3.call<std::string>(name + " and carol"); }
        };

        auto hook3_result = SafetyHookInline::create(Target::fn, Hook3::fn);

        expect(hook3_result.has_value());

        hook3 = std::move(*hook3_result);

        expect(eq(Target::fn("world"), "hello world and carol and eve and alice and bob"sv));

        // Unhook.
        hook3.reset();
        hook2.reset();
        hook1.reset();
        hook0.reset();
    };

    "Function with multiple args hooked"_test = [] {
        struct Target {
            SAFETYHOOK_NOINLINE static int add(int x, int y) { return x + y; }
        };

        expect(Target::add(2, 3) == 5_i);

        static SafetyHookInline add_hook;

        struct AddHook {
            static int add(int x, int y) { return add_hook.call<int>(x * 2, y * 2); }
        };

        auto add_hook_result = SafetyHookInline::create(Target::add, AddHook::add);

        expect(add_hook_result.has_value());

        add_hook = std::move(*add_hook_result);

        expect(Target::add(3, 4) == 14_i);

        add_hook.reset();

        expect(Target::add(5, 6) == 11_i);
    };

#if SAFETYHOOK_OS_WINDOWS
    "Active function is hooked and unhooked"_test = [] {
        static int count = 0;
        static bool is_running = true;

        struct Target {
            SAFETYHOOK_NOINLINE static std::string say_hello(int times) { return "Hello #" + std::to_string(times); }

            static void say_hello_infinitely() {
                while (is_running) {
                    say_hello(count++);
                }
            }
        };

        std::thread t{Target::say_hello_infinitely};

        std::this_thread::sleep_for(1s);

        static SafetyHookInline hook;

        struct Hook {
            static std::string say_hello(int times [[maybe_unused]]) { return hook.call<std::string>(1337); }
        };

        auto hook_result = SafetyHookInline::create(Target::say_hello, Hook::say_hello);

        expect(hook_result.has_value());

        hook = std::move(*hook_result);

        expect(eq(Target::say_hello(0), "Hello #1337"sv));

        std::this_thread::sleep_for(1s);
        hook.reset();

        is_running = false;
        t.join();

        expect(eq(Target::say_hello(0), "Hello #0"sv));
        expect(count > 0_i);
    };
#endif

    "Function with short unconditional branch is hooked"_test = [] {
        static SafetyHookInline hook;

        struct Hook {
            static int SAFETYHOOK_FASTCALL fn() { return hook.fastcall<int>() + 42; };
        };

        Xbyak::CodeGenerator cg{};

        cg.jmp("@f");
        cg.mov(eax, 0);
        cg.ret();
        cg.nop(10, false);
        cg.L("@@");
        cg.mov(eax, 1);
        cg.ret();
        cg.nop(10, false);

        const auto fn = cg.getCode<int(SAFETYHOOK_FASTCALL*)()>();

        expect(fn() == 1_i);

        hook = safetyhook::create_inline(fn, Hook::fn);

        expect(fn() == 43_i);

        hook.reset();

        expect(fn() == 1_i);
    };

    "Function with short conditional branch is hooked"_test = [] {
        static SafetyHookInline hook;

        struct Hook {
            static int SAFETYHOOK_FASTCALL fn(int x) { return hook.fastcall<int>(x) + 42; };
        };

        Xbyak::CodeGenerator cg{};
        Xbyak::Label label{};
        const auto finalize = [&cg, &label] {
            cg.mov(eax, 0);
            cg.ret();
            cg.nop(10, false);
            cg.L(label);
            cg.mov(eax, 1);
            cg.ret();
            cg.nop(10, false);
            return cg.getCode<int(SAFETYHOOK_FASTCALL*)(int)>();
        };

#if SAFETYHOOK_OS_WINDOWS
        constexpr auto param = ecx;
#elif SAFETYHOOK_OS_LINUX
        constexpr auto param = edi;
#endif

        "JB"_test = [&] {
            cg.cmp(param, 8);
            cg.jb(label);
            const auto fn = finalize();

            expect(fn(7) == 1_i);
            expect(fn(8) == 0_i);
            expect(fn(9) == 0_i);

            hook = safetyhook::create_inline(fn, Hook::fn);

            expect(fn(7) == 43_i);
            expect(fn(8) == 42_i);
            expect(fn(9) == 42_i);

            hook.reset();

            expect(fn(7) == 1_i);
            expect(fn(8) == 0_i);
            expect(fn(9) == 0_i);

            cg.reset();
        };

        "JBE"_test = [&] {
            cg.cmp(param, 8);
            cg.jbe(label);
            const auto fn = finalize();

            expect(fn(7) == 1_i);
            expect(fn(8) == 1_i);
            expect(fn(9) == 0_i);

            hook = safetyhook::create_inline(fn, Hook::fn);

            expect(fn(7) == 43_i);
            expect(fn(8) == 43_i);
            expect(fn(9) == 42_i);

            hook.reset();

            expect(fn(7) == 1_i);
            expect(fn(8) == 1_i);
            expect(fn(9) == 0_i);

            cg.reset();
        };

        "JL"_test = [&] {
            cg.cmp(param, 8);
            cg.jl(label);
            const auto fn = finalize();

            expect(fn(7) == 1_i);
            expect(fn(8) == 0_i);
            expect(fn(9) == 0_i);

            hook = safetyhook::create_inline(fn, Hook::fn);

            expect(fn(7) == 43_i);
            expect(fn(8) == 42_i);
            expect(fn(9) == 42_i);

            hook.reset();

            expect(fn(7) == 1_i);
            expect(fn(8) == 0_i);
            expect(fn(9) == 0_i);

            cg.reset();
        };

        "JLE"_test = [&] {
            cg.cmp(param, 8);
            cg.jle(label);
            const auto fn = finalize();

            expect(fn(7) == 1_i);
            expect(fn(8) == 1_i);
            expect(fn(9) == 0_i);

            hook = safetyhook::create_inline(fn, Hook::fn);

            expect(fn(7) == 43_i);
            expect(fn(8) == 43_i);
            expect(fn(9) == 42_i);

            hook.reset();

            expect(fn(7) == 1_i);
            expect(fn(8) == 1_i);
            expect(fn(9) == 0_i);

            cg.reset();
        };

        "JNB"_test = [&] {
            cg.cmp(param, 8);
            cg.jnb(label);
            const auto fn = finalize();

            expect(fn(7) == 0_i);
            expect(fn(8) == 1_i);
            expect(fn(9) == 1_i);

            hook = safetyhook::create_inline(fn, Hook::fn);

            expect(fn(7) == 42_i);
            expect(fn(8) == 43_i);
            expect(fn(9) == 43_i);

            hook.reset();

            expect(fn(7) == 0_i);
            expect(fn(8) == 1_i);
            expect(fn(9) == 1_i);

            cg.reset();
        };

        "JNBE"_test = [&] {
            cg.cmp(param, 8);
            cg.jnbe(label);
            const auto fn = finalize();

            expect(fn(7) == 0_i);
            expect(fn(8) == 0_i);
            expect(fn(9) == 1_i);

            hook = safetyhook::create_inline(fn, Hook::fn);

            expect(fn(7) == 42_i);
            expect(fn(8) == 42_i);
            expect(fn(9) == 43_i);

            hook.reset();

            expect(fn(7) == 0_i);
            expect(fn(8) == 0_i);
            expect(fn(9) == 1_i);

            cg.reset();
        };

        "JNL"_test = [&] {
            cg.cmp(param, 8);
            cg.jnl(label);
            const auto fn = finalize();

            expect(fn(7) == 0_i);
            expect(fn(8) == 1_i);
            expect(fn(9) == 1_i);

            hook = safetyhook::create_inline(fn, Hook::fn);

            expect(fn(7) == 42_i);
            expect(fn(8) == 43_i);
            expect(fn(9) == 43_i);

            hook.reset();

            expect(fn(7) == 0_i);
            expect(fn(8) == 1_i);
            expect(fn(9) == 1_i);

            cg.reset();
        };

        "JNLE"_test = [&] {
            cg.cmp(param, 8);
            cg.jnle(label);
            const auto fn = finalize();

            expect(fn(7) == 0_i);
            expect(fn(8) == 0_i);
            expect(fn(9) == 1_i);

            hook = safetyhook::create_inline(fn, Hook::fn);

            expect(fn(7) == 42_i);
            expect(fn(8) == 42_i);
            expect(fn(9) == 43_i);

            hook.reset();

            expect(fn(7) == 0_i);
            expect(fn(8) == 0_i);
            expect(fn(9) == 1_i);

            cg.reset();
        };

        "JNO"_test = [&] {
            cg.cmp(param, 8);
            cg.jno(label);
            const auto fn = finalize();

            expect(fn(7) == 1_i);
            expect(fn(8) == 1_i);
            expect(fn(9) == 1_i);

            hook = safetyhook::create_inline(fn, Hook::fn);

            expect(fn(7) == 43_i);
            expect(fn(8) == 43_i);
            expect(fn(9) == 43_i);

            hook.reset();

            expect(fn(7) == 1_i);
            expect(fn(8) == 1_i);
            expect(fn(9) == 1_i);

            cg.reset();
        };

        "JNP"_test = [&] {
            cg.cmp(param, 8);
            cg.jnp(label);
            const auto fn = finalize();

            expect(fn(7) == 0_i);
            expect(fn(8) == 0_i);
            expect(fn(9) == 1_i);

            hook = safetyhook::create_inline(fn, Hook::fn);

            expect(fn(7) == 42_i);
            expect(fn(8) == 42_i);
            expect(fn(9) == 43_i);

            hook.reset();

            expect(fn(7) == 0_i);
            expect(fn(8) == 0_i);
            expect(fn(9) == 1_i);

            cg.reset();
        };

        "JNS"_test = [&] {
            cg.cmp(param, 8);
            cg.jns(label);
            const auto fn = finalize();

            expect(fn(7) == 0_i);
            expect(fn(8) == 1_i);
            expect(fn(9) == 1_i);

            hook = safetyhook::create_inline(fn, Hook::fn);

            expect(fn(7) == 42_i);
            expect(fn(8) == 43_i);
            expect(fn(9) == 43_i);

            hook.reset();

            expect(fn(7) == 0_i);
            expect(fn(8) == 1_i);
            expect(fn(9) == 1_i);

            cg.reset();
        };

        "JNZ"_test = [&] {
            cg.cmp(param, 8);
            cg.jnz(label);
            const auto fn = finalize();

            expect(fn(7) == 1_i);
            expect(fn(8) == 0_i);
            expect(fn(9) == 1_i);

            hook = safetyhook::create_inline(fn, Hook::fn);

            expect(fn(7) == 43_i);
            expect(fn(8) == 42_i);
            expect(fn(9) == 43_i);

            hook.reset();

            expect(fn(7) == 1_i);
            expect(fn(8) == 0_i);
            expect(fn(9) == 1_i);

            cg.reset();
        };

        "JO"_test = [&] {
            cg.cmp(param, 8);
            cg.jo(label);
            const auto fn = finalize();

            expect(fn(7) == 0_i);
            expect(fn(8) == 0_i);
            expect(fn(9) == 0_i);

            hook = safetyhook::create_inline(fn, Hook::fn);

            expect(fn(7) == 42_i);
            expect(fn(8) == 42_i);
            expect(fn(9) == 42_i);

            hook.reset();

            expect(fn(7) == 0_i);
            expect(fn(8) == 0_i);
            expect(fn(9) == 0_i);

            cg.reset();
        };

        "JP"_test = [&] {
            cg.cmp(param, 8);
            cg.jp(label);
            const auto fn = finalize();

            expect(fn(7) == 1_i);
            expect(fn(8) == 1_i);
            expect(fn(9) == 0_i);

            hook = safetyhook::create_inline(fn, Hook::fn);

            expect(fn(7) == 43_i);
            expect(fn(8) == 43_i);
            expect(fn(9) == 42_i);

            hook.reset();

            expect(fn(7) == 1_i);
            expect(fn(8) == 1_i);
            expect(fn(9) == 0_i);

            cg.reset();
        };

        "JS"_test = [&] {
            cg.cmp(param, 8);
            cg.js(label);
            const auto fn = finalize();

            expect(fn(7) == 1_i);
            expect(fn(8) == 0_i);
            expect(fn(9) == 0_i);

            hook = safetyhook::create_inline(fn, Hook::fn);

            expect(fn(7) == 43_i);
            expect(fn(8) == 42_i);
            expect(fn(9) == 42_i);

            hook.reset();

            expect(fn(7) == 1_i);
            expect(fn(8) == 0_i);
            expect(fn(9) == 0_i);

            cg.reset();
        };

        "JZ"_test = [&] {
            cg.cmp(param, 8);
            cg.jz(label);
            const auto fn = finalize();

            expect(fn(7) == 0_i);
            expect(fn(8) == 1_i);
            expect(fn(9) == 0_i);

            hook = safetyhook::create_inline(fn, Hook::fn);

            expect(fn(7) == 42_i);
            expect(fn(8) == 43_i);
            expect(fn(9) == 42_i);

            hook.reset();

            expect(fn(7) == 0_i);
            expect(fn(8) == 1_i);
            expect(fn(9) == 0_i);

            cg.reset();
        };
    };

    "Function with short jump inside trampoline"_test = [] {
        Xbyak::CodeGenerator cg{};

        cg.jmp("@f");
        cg.ret();
        cg.L("@@");
        cg.mov(eax, 42);
        cg.ret();
        cg.nop(10, false);

        const auto fn = cg.getCode<int (*)()>();

        expect(fn() == 42_i);

        static SafetyHookInline hook;

        struct Hook {
            static int fn() { return hook.call<int>() + 1; }
        };

        hook = safetyhook::create_inline(fn, Hook::fn);

        expect(fn() == 43_i);

        hook.reset();

        expect(fn() == 42_i);
    };

    "Function hook can be enable and disabled"_test = [] {
        struct Target {
            SAFETYHOOK_NOINLINE static int fn(int a) {
                volatile int b = a;
                return b * 2;
            }
        };

        expect(Target::fn(1) == 2_i);
        expect(Target::fn(2) == 4_i);
        expect(Target::fn(3) == 6_i);

        static SafetyHookInline hook;

        struct Hook {
            static int fn(int a) { return hook.call<int>(a + 1); }
        };

        auto hook0_result = SafetyHookInline::create(Target::fn, Hook::fn, SafetyHookInline::StartDisabled);

        expect(hook0_result.has_value());

        hook = std::move(*hook0_result);

        expect(Target::fn(1) == 2_i);
        expect(Target::fn(2) == 4_i);
        expect(Target::fn(3) == 6_i);

        expect(hook.enable().has_value());

        expect(Target::fn(1) == 4_i);
        expect(Target::fn(2) == 6_i);
        expect(Target::fn(3) == 8_i);

        expect(hook.disable().has_value());

        expect(Target::fn(1) == 2_i);
        expect(Target::fn(2) == 4_i);
        expect(Target::fn(3) == 6_i);

        hook.reset();

        expect(Target::fn(1) == 2_i);
        expect(Target::fn(2) == 4_i);
        expect(Target::fn(3) == 6_i);
    };
};