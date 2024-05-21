#include "safetyhook/common.hpp"

#if SAFETYHOOK_OS_LINUX

#include <cstdio>

#include <sys/mman.h>
#include <unistd.h>

#include "safetyhook/utility.hpp"

#include "safetyhook/os.hpp"

namespace safetyhook {
std::expected<uint8_t*, OsError> vm_allocate(uint8_t* address, size_t size, VmAccess access) {
    int prot = 0;
    int flags = MAP_PRIVATE | MAP_ANONYMOUS;

    if (access == VM_ACCESS_R) {
        prot = PROT_READ;
    } else if (access == VM_ACCESS_RW) {
        prot = PROT_READ | PROT_WRITE;
    } else if (access == VM_ACCESS_RX) {
        prot = PROT_READ | PROT_EXEC;
    } else if (access == VM_ACCESS_RWX) {
        prot = PROT_READ | PROT_WRITE | PROT_EXEC;
    } else {
        return std::unexpected{OsError::FAILED_TO_ALLOCATE};
    }

    auto* result = mmap(address, size, prot, flags, -1, 0);

    if (result == MAP_FAILED) {
        return std::unexpected{OsError::FAILED_TO_ALLOCATE};
    }

    return static_cast<uint8_t*>(result);
}

void vm_free(uint8_t* address) {
    munmap(address, 0);
}

std::expected<uint32_t, OsError> vm_protect(uint8_t* address, size_t size, VmAccess access) {
    int prot = 0;

    if (access == VM_ACCESS_R) {
        prot = PROT_READ;
    } else if (access == VM_ACCESS_RW) {
        prot = PROT_READ | PROT_WRITE;
    } else if (access == VM_ACCESS_RX) {
        prot = PROT_READ | PROT_EXEC;
    } else if (access == VM_ACCESS_RWX) {
        prot = PROT_READ | PROT_WRITE | PROT_EXEC;
    } else {
        return std::unexpected{OsError::FAILED_TO_PROTECT};
    }

    return vm_protect(address, size, prot);
}

std::expected<uint32_t, OsError> vm_protect(uint8_t* address, size_t size, uint32_t protect) {
    auto mbi = vm_query(address);

    if (!mbi.has_value()) {
        return std::unexpected{OsError::FAILED_TO_PROTECT};
    }

    uint32_t old_protect = 0;

    if (mbi->access.read) {
        old_protect |= PROT_READ;
    }

    if (mbi->access.write) {
        old_protect |= PROT_WRITE;
    }

    if (mbi->access.execute) {
        old_protect |= PROT_EXEC;
    }

    auto* addr = align_down(address, static_cast<size_t>(sysconf(_SC_PAGESIZE)));

    if (mprotect(addr, size, static_cast<int>(protect)) == -1) {
        return std::unexpected{OsError::FAILED_TO_PROTECT};
    }

    return old_protect;
}

std::expected<VmBasicInfo, OsError> vm_query(uint8_t* address) {
    auto* maps = fopen("/proc/self/maps", "r");

    if (maps == nullptr) {
        return std::unexpected{OsError::FAILED_TO_QUERY};
    }

    char line[512];
    unsigned long start;
    unsigned long end;
    char perms[5];
    unsigned long offset;
    int dev_major;
    int dev_minor;
    unsigned long inode;
    char path[256];
    unsigned long last_end =
        reinterpret_cast<unsigned long>(system_info().min_address); // Track the end address of the last mapping.
    auto addr = reinterpret_cast<unsigned long>(address);
    std::optional<VmBasicInfo> info = std::nullopt;

    while (fgets(line, sizeof(line), maps) != nullptr) {
        path[0] = '\0';

        sscanf(line, "%lx-%lx %4s %lx %x:%x %lu %255[^\n]", &start, &end, perms, &offset, &dev_major, &dev_minor,
            &inode, path);

        if (last_end < start && addr >= last_end && addr < start) {
            info = {
                .address = reinterpret_cast<uint8_t*>(last_end),
                .size = start - last_end,
                .access = VmAccess{},
                .is_free = true,
            };

            break;
        }

        last_end = end;

        if (addr >= start && addr < end) {
            info = {
                .address = reinterpret_cast<uint8_t*>(start),
                .size = end - start,
                .access = VmAccess{},
                .is_free = false,
            };

            if (perms[0] == 'r') {
                info->access.read = true;
            }

            if (perms[1] == 'w') {
                info->access.write = true;
            }

            if (perms[2] == 'x') {
                info->access.execute = true;
            }

            break;
        }
    }

    fclose(maps);

    if (!info.has_value()) {
        return std::unexpected{OsError::FAILED_TO_QUERY};
    }

    return info.value();
}

bool vm_is_readable(uint8_t* address, [[maybe_unused]] size_t size) {
    return vm_query(address).value_or(VmBasicInfo{}).access.read;
}

bool vm_is_writable(uint8_t* address, [[maybe_unused]] size_t size) {
    return vm_query(address).value_or(VmBasicInfo{}).access.write;
}

bool vm_is_executable(uint8_t* address) {
    return vm_query(address).value_or(VmBasicInfo{}).access.execute;
}

SystemInfo system_info() {
    auto page_size = static_cast<uint32_t>(sysconf(_SC_PAGESIZE));

    return {
        .page_size = page_size,
        .allocation_granularity = page_size,
        .min_address = reinterpret_cast<uint8_t*>(0x10000),
        .max_address = reinterpret_cast<uint8_t*>(1ull << 47),
    };
}

void trap_threads([[maybe_unused]] uint8_t* from, [[maybe_unused]] uint8_t* to, [[maybe_unused]] size_t len,
    const std::function<void()>& run_fn) {
    auto from_protect = vm_protect(from, len, VM_ACCESS_RWX).value_or(0);
    auto to_protect = vm_protect(to, len, VM_ACCESS_RWX).value_or(0);
    run_fn();
    vm_protect(to, len, to_protect);
    vm_protect(from, len, from_protect);
}

void fix_ip([[maybe_unused]] ThreadContext ctx, [[maybe_unused]] uint8_t* old_ip, [[maybe_unused]] uint8_t* new_ip) {
}

} // namespace safetyhook

#endif