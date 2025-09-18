#pragma once

#include "sp_inc.hpp"

#include <cstdint>
#include <optional>

#ifdef DHOOKS_X86_64
#include <khook/asm/x86_64.hpp>
#else
#include <khook/asm/x86.hpp>
#endif

namespace dhooks
{

#ifdef DHOOKS_X86_64
const auto STACK_REG = KHook::Asm::rsp;
using AsmRegCode = KHook::Asm::x8664Reg;
using AsmReg = KHook::Asm::x86_64_Reg;
using AsmFloatRegCode = KHook::Asm::x8664FloatReg;
using AsmFloatReg = KHook::Asm::x86_64_FloatReg;
static constexpr const std::size_t MAX_GENERAL_REGISTERS = 16;
static constexpr const std::size_t MAX_FLOAT_REGISTERS = 16;
static_assert(KHook::Asm::RAX == 0);
#elif DHOOKS_X86
const auto STACK_REG = KHook::Asm::esp;
using AsmRegCode = KHook::Asm::x86Reg;
using AsmReg = KHook::Asm::x86_Reg;
using AsmFloatRegCode = KHook::Asm::x86FloatReg;
using AsmFloatReg = KHook::Asm::x86_Float_Reg;
static constexpr const std::size_t MAX_GENERAL_REGISTERS = 8;
static constexpr const std::size_t MAX_FLOAT_REGISTERS = 8;
static_assert(KHook::Asm::EAX == 0);
#endif

class GeneralRegister {
public:
    operator float() { return (float)_value; }
    operator bool() { return _value != 0; }
#if defined(DHOOKS_X86_64) /* || defined() */
    operator std::int64_t() { return (std::int64_t)_value; }
    operator std::uint64_t() { return (std::uint64_t)_value; }
#endif
    operator std::int32_t() { return (std::int32_t)_value; }
    operator std::uint32_t() { return (std::uint32_t)_value; }
    operator std::int16_t() { return (std::int16_t)_value; }
    operator std::uint16_t() { return (std::uint16_t)_value; }
    operator std::int8_t() { return (std::int8_t)_value; }
    operator std::uint8_t() { return (std::int8_t)_value; }
protected:
    std::uintptr_t _value;
};

class FloatRegister {
public:
    operator float() { return *(float*)&_value; }
    operator double() { return *(double*)&_value; }
protected:
    std::uint8_t _value[16];
};

static_assert(sizeof(GeneralRegister) == sizeof(void*), "GeneralRegister class size isn't the size of a pointer!");
static_assert(sizeof(FloatRegister) == 16, "FloatRegister class size isn't 128 bits!");

std::optional<AsmRegCode> Translate_DHookRegister(sp::DHookRegister reg);
std::optional<AsmFloatReg> Translate_DHookRegister_Float(sp::DHookRegister reg);
}