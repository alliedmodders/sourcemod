#include "safetyhook/os.hpp"

#include "safetyhook/vmt_hook.hpp"

namespace safetyhook {
VmHook::VmHook(VmHook&& other) noexcept {
    *this = std::move(other);
}

VmHook& VmHook::operator=(VmHook&& other) noexcept {
    destroy();
    m_original_vm = other.m_original_vm;
    m_new_vm = other.m_new_vm;
    m_vmt_entry = other.m_vmt_entry;
    m_new_vmt_allocation = std::move(other.m_new_vmt_allocation);
    other.m_original_vm = nullptr;
    other.m_new_vm = nullptr;
    other.m_vmt_entry = nullptr;
    return *this;
}

VmHook::~VmHook() {
    destroy();
}

void VmHook::reset() {
    *this = {};
}

void VmHook::destroy() {
    if (m_original_vm != nullptr) {
        *m_vmt_entry = m_original_vm;
        m_original_vm = nullptr;
        m_new_vm = nullptr;
        m_vmt_entry = nullptr;
        m_new_vmt_allocation.reset();
    }
}

std::expected<VmtHook, VmtHook::Error> VmtHook::create(void* object) {
    VmtHook hook{};

    const auto original_vmt = *reinterpret_cast<uint8_t***>(object);
    hook.m_objects.emplace(object, original_vmt);

    // Count the number of virtual method pointers. We start at one to account for the RTTI pointer.
    auto num_vmt_entries = 1;

    for (auto vm = original_vmt; is_executable(*vm); ++vm) {
        ++num_vmt_entries;
    }

    // Allocate memory for the new VMT.
    auto allocation = Allocator::global()->allocate(num_vmt_entries * sizeof(uint8_t*));

    if (!allocation) {
        return std::unexpected{Error::bad_allocation(allocation.error())};
    }

    hook.m_new_vmt_allocation = std::make_shared<Allocation>(std::move(*allocation));
    hook.m_new_vmt = reinterpret_cast<uint8_t**>(hook.m_new_vmt_allocation->data());

    // Copy pointer to RTTI.
    hook.m_new_vmt[0] = original_vmt[-1];

    // Copy virtual method pointers.
    for (auto i = 0; i < num_vmt_entries - 1; ++i) {
        hook.m_new_vmt[i + 1] = original_vmt[i];
    }

    *reinterpret_cast<uint8_t***>(object) = &hook.m_new_vmt[1];

    return hook;
}

VmtHook::VmtHook(VmtHook&& other) noexcept {
    *this = std::move(other);
}

VmtHook& VmtHook::operator=(VmtHook&& other) noexcept {
    destroy();
    m_objects = std::move(other.m_objects);
    m_new_vmt_allocation = std::move(other.m_new_vmt_allocation);
    m_new_vmt = other.m_new_vmt;
    other.m_new_vmt = nullptr;
    return *this;
}

VmtHook::~VmtHook() {
    destroy();
}

void VmtHook::apply(void* object) {
    m_objects.emplace(object, *reinterpret_cast<uint8_t***>(object));
    *reinterpret_cast<uint8_t***>(object) = &m_new_vmt[1];
}

void VmtHook::remove(void* object) {
    const auto search = m_objects.find(object);

    if (search == m_objects.end()) {
        return;
    }

    const auto original_vmt = search->second;

    if (!vm_is_writable(reinterpret_cast<uint8_t*>(object), sizeof(void*))) {
        m_objects.erase(search);
        return;
    }

    if (*reinterpret_cast<uint8_t***>(object) != &m_new_vmt[1]) {
        m_objects.erase(search);
        return;
    }

    *reinterpret_cast<uint8_t***>(object) = original_vmt;

    m_objects.erase(search);
}

void VmtHook::reset() {
    *this = {};
}

void VmtHook::destroy() {
    for (const auto [object, original_vmt] : m_objects) {
        if (!vm_is_writable(reinterpret_cast<uint8_t*>(object), sizeof(void*))) {
            continue;
        }

        if (*reinterpret_cast<uint8_t***>(object) != &m_new_vmt[1]) {
            continue;
        }

        *reinterpret_cast<uint8_t***>(object) = original_vmt;
    }

    m_objects.clear();
    m_new_vmt_allocation.reset();
    m_new_vmt = nullptr;
}
} // namespace safetyhook
