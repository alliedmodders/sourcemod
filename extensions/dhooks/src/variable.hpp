#pragma once

#include "sp_inc.hpp"
#include "register.hpp"

#include <cstdint>
#include <optional>

namespace dhooks {
struct Variable {
public:
    enum Alignment {
        OneByte = 1,
        TwoBytes = 2,
        FourBytes = 4,
        EightBytes = 8,
        SixteenBytes = 16
    };

    sp::HookParamType dhook_type;
    std::size_t dhook_size;
    sp::DHookPassFlag dhook_pass_flags;
    sp::DHookRegister dhook_custom_register;

    // Provided by the ABI, used by the JIT
    // If DHook is to ever support complex type, transform those fields
    // into an array so that the SP natives know where to look when offsetting into an object
    std::optional<AsmRegCode> reg_index;
    std::optional<AsmFloatRegCode> float_reg_index;
    std::optional<size_t> reg_offset;
    Alignment alignment;
    // Atm it's exactly the same as _dhook_size
    // We differentiate it anyways, to avoid having to refactor the JIT
    std::size_t type_size;
};

struct ReturnVariable {
public:
    enum Alignment {
        OneByte = 1,
        TwoBytes = 2,
        FourBytes = 4,
        EightBytes = 8,
        SixteenBytes = 16
    };

    sp::ReturnType dhook_type;
    size_t dhook_size;
    sp::DHookPassFlag dhook_pass_flags;
    sp::DHookRegister dhook_custom_register;

    std::optional<AsmRegCode> reg_index;
    std::optional<AsmFloatRegCode> float_reg_index;
    size_t reg_offset;
};
}