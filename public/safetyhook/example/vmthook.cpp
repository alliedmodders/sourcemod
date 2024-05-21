#include <memory>
#include <print>

#include <safetyhook.hpp>

class Interface {
public:
    virtual ~Interface() = default;
    virtual int add_42(int a) = 0;
};

class Target : public Interface {
public:
    int add_42(int a) override { return a + 42; }
};

SafetyHookVmt g_target_hook;
SafetyHookVm g_add_42_hook;

class Hook : public Target {
public:
    int hooked_add_42(int a) { return g_add_42_hook.thiscall<int>(this, a) + 1337; }
};

int main() {
    auto target = std::make_unique<Target>();

    std::println("unhooked target->add_42(1) = {}", target->add_42(1));

    g_target_hook = safetyhook::create_vmt(target.get());

#if SAFETYHOOK_OS_WINDOWS
    g_add_42_hook = safetyhook::create_vm(g_target_hook, 1, &Hook::hooked_add_42);
#elif SAFETYHOOK_OS_LINUX
    g_add_42_hook = safetyhook::create_vm(g_target_hook, 2, &Hook::hooked_add_42);
#endif

    std::println("hooked target->add_42(2) = {}", target->add_42(2));

    g_target_hook = {};

    std::println("unhooked target->add_42(3) = {}", target->add_42(3));

    return 0;
}