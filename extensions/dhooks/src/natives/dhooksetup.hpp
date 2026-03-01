#pragma once

#include <sp_vm_types.h>
#include <vector>

namespace dhooks::natives::dhooksetup {
void init(std::vector<sp_nativeinfo_t>& natives);
}