#include <map>
#include <memory>
#include <mutex>

#include "safetyhook/common.hpp"
#include "safetyhook/utility.hpp"

#if SAFETYHOOK_OS_WINDOWS

#define NOMINMAX
#if __has_include(<Windows.h>)
#include <Windows.h>
#elif __has_include(<windows.h>)
#include <windows.h>
#else
#error "Windows.h not found"
#endif

#include "safetyhook/os.hpp"

namespace safetyhook {
std::expected<uint8_t*, OsError> vm_allocate(uint8_t* address, size_t size, VmAccess access) {
    DWORD protect = 0;

    if (access == VM_ACCESS_R) {
        protect = PAGE_READONLY;
    } else if (access == VM_ACCESS_RW) {
        protect = PAGE_READWRITE;
    } else if (access == VM_ACCESS_RX) {
        protect = PAGE_EXECUTE_READ;
    } else if (access == VM_ACCESS_RWX) {
        protect = PAGE_EXECUTE_READWRITE;
    } else {
        return std::unexpected{OsError::FAILED_TO_ALLOCATE};
    }

    auto* result = VirtualAlloc(address, size, MEM_COMMIT | MEM_RESERVE, protect);

    if (result == nullptr) {
        return std::unexpected{OsError::FAILED_TO_ALLOCATE};
    }

    return static_cast<uint8_t*>(result);
}

void vm_free(uint8_t* address) {
    VirtualFree(address, 0, MEM_RELEASE);
}

std::expected<uint32_t, OsError> vm_protect(uint8_t* address, size_t size, VmAccess access) {
    DWORD protect = 0;

    if (access == VM_ACCESS_R) {
        protect = PAGE_READONLY;
    } else if (access == VM_ACCESS_RW) {
        protect = PAGE_READWRITE;
    } else if (access == VM_ACCESS_RX) {
        protect = PAGE_EXECUTE_READ;
    } else if (access == VM_ACCESS_RWX) {
        protect = PAGE_EXECUTE_READWRITE;
    } else {
        return std::unexpected{OsError::FAILED_TO_PROTECT};
    }

    return vm_protect(address, size, protect);
}

std::expected<uint32_t, OsError> vm_protect(uint8_t* address, size_t size, uint32_t protect) {
    DWORD old_protect = 0;

    if (VirtualProtect(address, size, protect, &old_protect) == FALSE) {
        return std::unexpected{OsError::FAILED_TO_PROTECT};
    }

    return old_protect;
}

std::expected<VmBasicInfo, OsError> vm_query(uint8_t* address) {
    MEMORY_BASIC_INFORMATION mbi{};
    auto result = VirtualQuery(address, &mbi, sizeof(mbi));

    if (result == 0) {
        return std::unexpected{OsError::FAILED_TO_QUERY};
    }

    VmAccess access{
        .read = (mbi.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE)) != 0,
        .write = (mbi.Protect & (PAGE_READWRITE | PAGE_EXECUTE_READWRITE)) != 0,
        .execute = (mbi.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE)) != 0,
    };

    return VmBasicInfo{
        .address = static_cast<uint8_t*>(mbi.AllocationBase),
        .size = mbi.RegionSize,
        .access = access,
        .is_free = mbi.State == MEM_FREE,
    };
}

bool vm_is_readable(uint8_t* address, size_t size) {
    return IsBadReadPtr(address, size) == FALSE;
}

bool vm_is_writable(uint8_t* address, size_t size) {
    return IsBadWritePtr(address, size) == FALSE;
}

bool vm_is_executable(uint8_t* address) {
    LPVOID image_base_ptr;

    if (RtlPcToFileHeader(address, &image_base_ptr) == nullptr) {
        return vm_query(address).value_or(VmBasicInfo{}).access.execute;
    }

    // Just check if the section is executable.
    const auto* image_base = reinterpret_cast<uint8_t*>(image_base_ptr);
    const auto* dos_hdr = reinterpret_cast<const IMAGE_DOS_HEADER*>(image_base);

    if (dos_hdr->e_magic != IMAGE_DOS_SIGNATURE) {
        return vm_query(address).value_or(VmBasicInfo{}).access.execute;
    }

    const auto* nt_hdr = reinterpret_cast<const IMAGE_NT_HEADERS*>(image_base + dos_hdr->e_lfanew);

    if (nt_hdr->Signature != IMAGE_NT_SIGNATURE) {
        return vm_query(address).value_or(VmBasicInfo{}).access.execute;
    }

    const auto* section = IMAGE_FIRST_SECTION(nt_hdr);

    for (auto i = 0; i < nt_hdr->FileHeader.NumberOfSections; ++i, ++section) {
        if (address >= image_base + section->VirtualAddress &&
            address < image_base + section->VirtualAddress + section->Misc.VirtualSize) {
            return (section->Characteristics & IMAGE_SCN_MEM_EXECUTE) != 0;
        }
    }

    return vm_query(address).value_or(VmBasicInfo{}).access.execute;
}

SystemInfo system_info() {
    SystemInfo info{};

    SYSTEM_INFO si{};
    GetSystemInfo(&si);

    info.page_size = si.dwPageSize;
    info.allocation_granularity = si.dwAllocationGranularity;
    info.min_address = static_cast<uint8_t*>(si.lpMinimumApplicationAddress);
    info.max_address = static_cast<uint8_t*>(si.lpMaximumApplicationAddress);

    return info;
}

struct TrapInfo {
    uint8_t* from_page_start;
    uint8_t* from_page_end;
    uint8_t* from;
    uint8_t* to_page_start;
    uint8_t* to_page_end;
    uint8_t* to;
    size_t len;
};

class TrapManager final {
public:
    static std::mutex mutex;
    static std::unique_ptr<TrapManager> instance;

    TrapManager() { m_trap_veh = AddVectoredExceptionHandler(1, trap_handler); }
    ~TrapManager() {
        if (m_trap_veh != nullptr) {
            RemoveVectoredExceptionHandler(m_trap_veh);
        }
    }

    TrapInfo* find_trap(uint8_t* address) {
        auto search = std::find_if(m_traps.begin(), m_traps.end(), [address](auto& trap) {
            return address >= trap.second.from && address < trap.second.from + trap.second.len;
        });

        if (search == m_traps.end()) {
            return nullptr;
        }

        return &search->second;
    }

    TrapInfo* find_trap_page(uint8_t* address) {
        auto search = std::find_if(m_traps.begin(), m_traps.end(), [address](auto& trap) {
            return address >= trap.second.from_page_start && address < trap.second.from_page_end;
        });

        if (search != m_traps.end()) {
            return &search->second;
        }

        search = std::find_if(m_traps.begin(), m_traps.end(), [address](auto& trap) {
            return address >= trap.second.to_page_start && address < trap.second.to_page_end;
        });

        if (search != m_traps.end()) {
            return &search->second;
        }

        return nullptr;
    }

    void add_trap(uint8_t* from, uint8_t* to, size_t len) {
        m_traps.insert_or_assign(from, TrapInfo{.from_page_start = align_down(from, 0x1000),
                                           .from_page_end = align_up(from + len, 0x1000),
                                           .from = from,
                                           .to_page_start = align_down(to, 0x1000),
                                           .to_page_end = align_up(to + len, 0x1000),
                                           .to = to,
                                           .len = len});
    }

private:
    std::map<uint8_t*, TrapInfo> m_traps;
    PVOID m_trap_veh{};

    static LONG CALLBACK trap_handler(PEXCEPTION_POINTERS exp) {
        auto exception_code = exp->ExceptionRecord->ExceptionCode;

        if (exception_code != EXCEPTION_ACCESS_VIOLATION) {
            return EXCEPTION_CONTINUE_SEARCH;
        }

        std::scoped_lock lock{mutex};
        auto* faulting_address = reinterpret_cast<uint8_t*>(exp->ExceptionRecord->ExceptionInformation[1]);
        auto* trap = instance->find_trap(faulting_address);

        if (trap == nullptr) {
            if (instance->find_trap_page(faulting_address) != nullptr) {
                return EXCEPTION_CONTINUE_EXECUTION;
            } else {
                return EXCEPTION_CONTINUE_SEARCH;
            }
        }

        auto* ctx = exp->ContextRecord;

        for (size_t i = 0; i < trap->len; i++) {
            fix_ip(ctx, trap->from + i, trap->to + i);
        }

        return EXCEPTION_CONTINUE_EXECUTION;
    }
};

std::mutex TrapManager::mutex;
std::unique_ptr<TrapManager> TrapManager::instance;

void find_me() {
}

void trap_threads(uint8_t* from, uint8_t* to, size_t len, const std::function<void()>& run_fn) {
    MEMORY_BASIC_INFORMATION find_me_mbi{};
    MEMORY_BASIC_INFORMATION from_mbi{};
    MEMORY_BASIC_INFORMATION to_mbi{};

    VirtualQuery(reinterpret_cast<void*>(find_me), &find_me_mbi, sizeof(find_me_mbi));
    VirtualQuery(from, &from_mbi, sizeof(from_mbi));
    VirtualQuery(to, &to_mbi, sizeof(to_mbi));

    auto new_protect = PAGE_READWRITE;

    if (from_mbi.AllocationBase == find_me_mbi.AllocationBase || to_mbi.AllocationBase == find_me_mbi.AllocationBase) {
        new_protect = PAGE_EXECUTE_READWRITE;
    }

    std::scoped_lock lock{TrapManager::mutex};

    if (TrapManager::instance == nullptr) {
        TrapManager::instance = std::make_unique<TrapManager>();
    }

    TrapManager::instance->add_trap(from, to, len);

    DWORD from_protect;
    DWORD to_protect;

    VirtualProtect(from, len, new_protect, &from_protect);
    VirtualProtect(to, len, new_protect, &to_protect);

    if (run_fn) {
        run_fn();
    }

    VirtualProtect(to, len, to_protect, &to_protect);
    VirtualProtect(from, len, from_protect, &from_protect);
}

void fix_ip(ThreadContext thread_ctx, uint8_t* old_ip, uint8_t* new_ip) {
    auto* ctx = reinterpret_cast<CONTEXT*>(thread_ctx);

#if SAFETYHOOK_ARCH_X86_64
    auto ip = ctx->Rip;
#elif SAFETYHOOK_ARCH_X86_32
    auto ip = ctx->Eip;
#endif

    if (ip == reinterpret_cast<uintptr_t>(old_ip)) {
        ip = reinterpret_cast<uintptr_t>(new_ip);
    }

#if SAFETYHOOK_ARCH_X86_64
    ctx->Rip = ip;
#elif SAFETYHOOK_ARCH_X86_32
    ctx->Eip = ip;
#endif
}

} // namespace safetyhook

#endif