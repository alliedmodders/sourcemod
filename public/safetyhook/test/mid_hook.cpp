#include <boost/ut.hpp>
#include <safetyhook.hpp>

using namespace boost::ut;

static suite<"mid hook"> mid_hook_tests = [] {
    "Mid hook to change a register"_test = [] {
        struct Target {
            SAFETYHOOK_NOINLINE static int SAFETYHOOK_FASTCALL add_42(int a) { return a + 42; }
        };

        expect(Target::add_42(0) == 42_i);

        static SafetyHookMid hook;

        struct Hook {
            static void add_42(SafetyHookContext& ctx) {
#if SAFETYHOOK_OS_WINDOWS
#if SAFETYHOOK_ARCH_X86_64
                ctx.rcx = 1337 - 42;
#elif SAFETYHOOK_ARCH_X86_32
                ctx.ecx = 1337 - 42;
#endif
#elif SAFETYHOOK_OS_LINUX
#if SAFETYHOOK_ARCH_X86_64
                ctx.rdi = 1337 - 42;
#elif SAFETYHOOK_ARCH_X86_32
                ctx.edi = 1337 - 42;
#endif
#endif
            }
        };

        auto hook_result = SafetyHookMid::create(Target::add_42, Hook::add_42);

        expect(hook_result.has_value());

        hook = std::move(*hook_result);

        expect(Target::add_42(1) == 1337_i);

        hook.reset();

        expect(Target::add_42(2) == 44_i);
    };

#if SAFETYHOOK_ARCH_X86_64
    "Mid hook to change an XMM register"_test = [] {
        struct Target {
            SAFETYHOOK_NOINLINE static float SAFETYHOOK_FASTCALL add_42(float a) { return a + 0.42f; }
        };

        expect(Target::add_42(0.0f) == 0.42_f);

        static SafetyHookMid hook;

        struct Hook {
            static void add_42(SafetyHookContext& ctx) { ctx.xmm0.f32[0] = 1337.0f - 0.42f; }
        };

        auto hook_result = SafetyHookMid::create(Target::add_42, Hook::add_42);

        expect(hook_result.has_value());

        hook = std::move(*hook_result);

        expect(Target::add_42(1.0f) == 1337.0_f);

        hook.reset();

        expect(Target::add_42(2.0f) == 2.42_f);
    };
#endif

    "Mid hook enable and disable"_test = [] {
        struct Target {
            SAFETYHOOK_NOINLINE static int SAFETYHOOK_FASTCALL add_42(int a) {
                volatile int b = a;
                return b + 42;
            }
        };

        expect(Target::add_42(0) == 42_i);
        expect(Target::add_42(1) == 43_i);
        expect(Target::add_42(2) == 44_i);

        static SafetyHookMid hook;

        struct Hook {
            static void add_42(SafetyHookContext& ctx) {
#if SAFETYHOOK_OS_WINDOWS
#if SAFETYHOOK_ARCH_X86_64
                ctx.rcx = 1337 - 42;
#elif SAFETYHOOK_ARCH_X86_32
                ctx.ecx = 1337 - 42;
#endif
#elif SAFETYHOOK_OS_LINUX
#if SAFETYHOOK_ARCH_X86_64
                ctx.rdi = 1337 - 42;
#elif SAFETYHOOK_ARCH_X86_32
                ctx.edi = 1337 - 42;
#endif
#endif
            }
        };

        auto hook_result = SafetyHookMid::create(Target::add_42, Hook::add_42, SafetyHookMid::StartDisabled);

        expect(hook_result.has_value());

        hook = std::move(*hook_result);

        expect(Target::add_42(0) == 42_i);
        expect(Target::add_42(1) == 43_i);
        expect(Target::add_42(2) == 44_i);

        expect(hook.enable().has_value());

        expect(Target::add_42(1) == 1337_i);
        expect(Target::add_42(2) == 1337_i);
        expect(Target::add_42(3) == 1337_i);

        expect(hook.disable().has_value());

        expect(Target::add_42(0) == 42_i);
        expect(Target::add_42(1) == 43_i);
        expect(Target::add_42(2) == 44_i);

        hook.reset();

        expect(Target::add_42(0) == 42_i);
        expect(Target::add_42(1) == 43_i);
        expect(Target::add_42(2) == 44_i);
    };
};