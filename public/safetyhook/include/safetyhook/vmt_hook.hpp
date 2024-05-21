/// @file safetyhook/vmt_hook.hpp
/// @brief VMT hooking classes

#pragma once

#ifndef SAFETYHOOK_USE_CXXMODULES
#include <cstdint>
#include <expected>
#include <unordered_map>
#else
import std.compat;
#endif

#include "safetyhook/allocator.hpp"
#include "safetyhook/common.hpp"
#include "safetyhook/utility.hpp"

namespace safetyhook {
/// @brief A hook class that allows for hooking a single method in a VMT.
class VmHook final {
public:
    VmHook() = default;
    VmHook(const VmHook&) = delete;
    VmHook(VmHook&& other) noexcept;
    VmHook& operator=(const VmHook&) = delete;
    VmHook& operator=(VmHook&& other) noexcept;
    ~VmHook();

    /// @brief Removes the hook.
    void reset();

    /// @brief Gets the original method pointer.
    template <typename T> [[nodiscard]] T original() const { return reinterpret_cast<T>(m_original_vm); }

    /// @brief Calls the original method.
    /// @tparam RetT The return type of the method.
    /// @tparam Args The argument types of the method.
    /// @param args The arguments to pass to the method.
    /// @return The return value of the method.
    /// @note This will call the original method with the default calling convention.
    template <typename RetT = void, typename... Args> RetT call(Args... args) {
        return original<RetT (*)(Args...)>()(args...);
    }

    /// @brief Calls the original method with the __cdecl calling convention.
    /// @tparam RetT The return type of the method.
    /// @tparam Args The argument types of the method.
    /// @param args The arguments to pass to the method.
    /// @return The return value of the method.
    template <typename RetT = void, typename... Args> RetT ccall(Args... args) {
        return original<RetT(SAFETYHOOK_CCALL*)(Args...)>()(args...);
    }

    /// @brief Calls the original method with the __thiscall calling convention.
    /// @tparam RetT The return type of the method.
    /// @tparam Args The argument types of the method.
    /// @param args The arguments to pass to the method.
    /// @return The return value of the method.
    template <typename RetT = void, typename... Args> RetT thiscall(Args... args) {
        return original<RetT(SAFETYHOOK_THISCALL*)(Args...)>()(args...);
    }

    /// @brief Calls the original method with the __stdcall calling convention.
    /// @tparam RetT The return type of the method.
    /// @tparam Args The argument types of the method.
    /// @param args The arguments to pass to the method.
    /// @return The return value of the method.
    template <typename RetT = void, typename... Args> RetT stdcall(Args... args) {
        return original<RetT(SAFETYHOOK_STDCALL*)(Args...)>()(args...);
    }

    /// @brief Calls the original method with the __fastcall calling convention.
    /// @tparam RetT The return type of the method.
    /// @tparam Args The argument types of the method.
    /// @param args The arguments to pass to the method.
    /// @return The return value of the method.
    template <typename RetT = void, typename... Args> RetT fastcall(Args... args) {
        return original<RetT(SAFETYHOOK_FASTCALL*)(Args...)>()(args...);
    }

private:
    friend class VmtHook;

    uint8_t* m_original_vm{};
    uint8_t* m_new_vm{};
    uint8_t** m_vmt_entry{};

    // This keeps the allocation alive until the hook is destroyed.
    std::shared_ptr<Allocation> m_new_vmt_allocation{};

    void destroy();
};

/// @brief A hook class that copies an entire VMT for a given object and replaces it.
class VmtHook final {
public:
    /// @brief Error type for VmtHook.
    struct Error {
        /// @brief The type of error.
        enum : uint8_t {
            BAD_ALLOCATION, ///< An error occurred while allocating memory.
        } type;

        /// @brief Extra error information.
        union {
            Allocator::Error allocator_error; ///< Allocator error information.
        };

        /// @brief Create a BAD_ALLOCATION error.
        /// @param err The Allocator::Error that failed.
        /// @return The new BAD_ALLOCATION error.
        [[nodiscard]] static Error bad_allocation(Allocator::Error err) {
            return {.type = BAD_ALLOCATION, .allocator_error = err};
        }
    };

    /// @brief Creates a new VmtHook object. Will clone the VMT of the given object and replace it.
    /// @param object The object to hook.
    /// @return The VmtHook object or a VmtHook::Error if an error occurred.
    [[nodiscard]] static std::expected<VmtHook, Error> create(void* object);

    VmtHook() = default;
    VmtHook(const VmtHook&) = delete;
    VmtHook(VmtHook&& other) noexcept;
    VmtHook& operator=(const VmtHook&) = delete;
    VmtHook& operator=(VmtHook&& other) noexcept;
    ~VmtHook();

    /// @brief Applies the hook.
    /// @param object The object to apply the hook to.
    /// @note This will replace the VMT of the object with the new VMT. You can apply the hook to multiple objects.
    void apply(void* object);

    /// @brief Removes the hook.
    /// @param object The object to remove the hook from.
    void remove(void* object);

    /// @brief Removes the hook from all objects.
    void reset();

    /// @brief Hooks a method in the VMT.
    /// @param index The index of the method to hook.
    /// @param new_function The new function to use.
    [[nodiscard]] std::expected<VmHook, Error> hook_method(size_t index, FnPtr auto new_function) {
        VmHook hook{};

        ++index; // Skip RTTI pointer.
        hook.m_original_vm = m_new_vmt[index];
        store(reinterpret_cast<uint8_t*>(&hook.m_new_vm), new_function);
        hook.m_vmt_entry = &m_new_vmt[index];
        hook.m_new_vmt_allocation = m_new_vmt_allocation;
        m_new_vmt[index] = hook.m_new_vm;

        return hook;
    }

private:
    // Map of object instance to their original VMT.
    std::unordered_map<void*, uint8_t**> m_objects{};

    // The allocation is a shared_ptr, so it can be shared with VmHooks to ensure the memory is kept alive.
    std::shared_ptr<Allocation> m_new_vmt_allocation{};
    uint8_t** m_new_vmt{};

    void destroy();
};
} // namespace safetyhook
