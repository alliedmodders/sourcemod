#pragma once

#include <sp_vm_types.h>
#include <vector>

namespace dhooks::natives::dhookreturn {
void init(std::vector<sp_nativeinfo_t>& natives);
}