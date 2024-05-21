#include "safetyhook/os.hpp"

#include "safetyhook/utility.hpp"

namespace safetyhook {
bool is_executable(uint8_t* address) {
    return vm_is_executable(address);
}

UnprotectMemory::~UnprotectMemory() {
    if (m_address != nullptr) {
        vm_protect(m_address, m_size, m_original_protection);
    }
}

UnprotectMemory::UnprotectMemory(UnprotectMemory&& other) noexcept {
    *this = std::move(other);
}

UnprotectMemory& UnprotectMemory::operator=(UnprotectMemory&& other) noexcept {
    if (this != &other) {
        m_address = other.m_address;
        m_size = other.m_size;
        m_original_protection = other.m_original_protection;
        other.m_address = nullptr;
        other.m_size = 0;
        other.m_original_protection = 0;
    }

    return *this;
}

std::optional<UnprotectMemory> unprotect(uint8_t* address, size_t size) {
    auto old_protection = vm_protect(address, size, VM_ACCESS_RWX);

    if (!old_protection) {
        return std::nullopt;
    }

    return UnprotectMemory{address, size, old_protection.value()};
}

} // namespace safetyhook