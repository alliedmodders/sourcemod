#include <print>

#include <safetyhook.hpp>

SAFETYHOOK_NOINLINE int add(int x, int y) {
    return x + y;
}

SafetyHookInline g_add_hook{};

int hook_add(int x, int y) {
    return g_add_hook.call<int>(x * 2, y * 2);
}

int main() {
    std::println("unhooked add(2, 3) = {}", add(2, 3));

    // Create a hook on add.
    g_add_hook = safetyhook::create_inline(reinterpret_cast<void*>(add), reinterpret_cast<void*>(hook_add));

    std::println("hooked add(3, 4) = {}", add(3, 4));

    g_add_hook = {};

    std::println("unhooked add(5, 6) = {}", add(5, 6));

    return 0;
}