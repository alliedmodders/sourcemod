#pragma once

#ifndef SAFETYHOOK_USE_CXXMODULES
#include <algorithm>
#include <cstdint>
#include <optional>
#include <type_traits>
#else
import std.compat;
#endif

namespace safetyhook {
template <typename T> constexpr void store(uint8_t* address, const T& value) {
    std::copy_n(reinterpret_cast<const uint8_t*>(&value), sizeof(T), address);
}

template <typename T>
concept FnPtr = requires(T f) { std::is_pointer_v<T>&& std::is_function_v<std::remove_pointer_t<T>>; };

bool is_executable(uint8_t* address);

class UnprotectMemory {
public:
    UnprotectMemory() = delete;
    ~UnprotectMemory();
    UnprotectMemory(const UnprotectMemory&) = delete;
    UnprotectMemory(UnprotectMemory&& other) noexcept;
    UnprotectMemory& operator=(const UnprotectMemory&) = delete;
    UnprotectMemory& operator=(UnprotectMemory&& other) noexcept;

private:
    friend std::optional<UnprotectMemory> unprotect(uint8_t*, size_t);

    UnprotectMemory(uint8_t* address, size_t size, uint32_t original_protection)
        : m_address{address}, m_size{size}, m_original_protection{original_protection} {}

    uint8_t* m_address{};
    size_t m_size{};
    uint32_t m_original_protection{};
};

[[nodiscard]] std::optional<UnprotectMemory> unprotect(uint8_t* address, size_t size);

template <typename T> constexpr T align_up(T address, size_t align) {
    const auto unaligned_address = (uintptr_t)address;
    const auto aligned_address = (unaligned_address + align - 1) & ~(align - 1);
    return (T)aligned_address;
}

template <typename T> constexpr T align_down(T address, size_t align) {
    const auto unaligned_address = (uintptr_t)address;
    const auto aligned_address = unaligned_address & ~(align - 1);
    return (T)aligned_address;
}
} // namespace safetyhook
