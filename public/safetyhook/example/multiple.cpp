#include <print>

#include <safetyhook.hpp>

SafetyHookInline hook0, hook1, hook2, hook3;

SAFETYHOOK_NOINLINE void say_hi(const std::string& name) {
    std::println("hello {}", name);
}

void hook0_fn(const std::string& name) {
    hook0.call<void, const std::string&>(name + " and bob");
}

void hook1_fn(const std::string& name) {
    hook1.call<void, const std::string&>(name + " and alice");
}

void hook2_fn(const std::string& name) {
    hook2.call<void, const std::string&>(name + " and eve");
}

void hook3_fn(const std::string& name) {
    hook3.call<void, const std::string&>(name + " and carol");
}

int main() {
    hook0 = safetyhook::create_inline(reinterpret_cast<void*>(say_hi), reinterpret_cast<void*>(hook0_fn));
    hook1 = safetyhook::create_inline(reinterpret_cast<void*>(say_hi), reinterpret_cast<void*>(hook1_fn));
    hook2 = safetyhook::create_inline(reinterpret_cast<void*>(say_hi), reinterpret_cast<void*>(hook2_fn));
    hook3 = safetyhook::create_inline(reinterpret_cast<void*>(say_hi), reinterpret_cast<void*>(hook3_fn));

    say_hi("world");

    return 0;
}