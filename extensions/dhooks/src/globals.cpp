#include "globals.hpp"

namespace dhooks::globals {

std::unordered_map<void*, std::unique_ptr<class Capsule>> address_detours;
std::unordered_map<void*, std::unique_ptr<class Capsule>> virtual_detours;
std::unordered_map<std::uint32_t, std::unique_ptr<class HookCallback>> hook_callbacks;

void init() {
}

}