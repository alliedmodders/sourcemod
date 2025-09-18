#include "register.hpp"

namespace dhooks {

std::optional<AsmRegCode> Translate_DHookRegister(sp::DHookRegister reg) {
	switch (reg) {
		case sp::DHookRegister_AL:
		case sp::DHookRegister_AH:
		case sp::DHookRegister_EAX:
#ifdef DHOOKS_X86_64
		return KHook::Asm::rax;
#else
		return KHook::Asm::eax;
#endif
		case sp::DHookRegister_CL:
		case sp::DHookRegister_CH:
		case sp::DHookRegister_ECX:
#ifdef DHOOKS_X86_64
		return KHook::Asm::rcx;
#else
		return KHook::Asm::ecx;
#endif
		case sp::DHookRegister_DL:
		case sp::DHookRegister_DH:
		case sp::DHookRegister_EDX:
#ifdef DHOOKS_X86_64
		return KHook::Asm::rdx;
#else
		return KHook::Asm::edx;
#endif
		case sp::DHookRegister_BL:
		case sp::DHookRegister_BH:
		case sp::DHookRegister_EBX:
#ifdef DHOOKS_X86_64
		return KHook::Asm::rbx;
#else
		return KHook::Asm::ebx;
#endif
		case sp::DHookRegister_ESP:
#ifdef DHOOKS_X86_64
		return KHook::Asm::rsp;
#else
		return KHook::Asm::esp;
#endif
		case sp::DHookRegister_EBP:
#ifdef DHOOKS_X86_64
		return KHook::Asm::rbp;
#else
		return KHook::Asm::ebp;
#endif
		case sp::DHookRegister_ESI:
#ifdef DHOOKS_X86_64
		return KHook::Asm::rsi;
#else
		return KHook::Asm::esi;
#endif
		case sp::DHookRegister_EDI:
#ifdef DHOOKS_X86_64
		return KHook::Asm::rdi;
#else
		return KHook::Asm::edi;
#endif
    }
	return {};
}

std::optional<AsmFloatReg> Translate_DHookRegister_Float(sp::DHookRegister reg) {
	switch (reg) {
		case sp::DHookRegister_XMM0:
			return KHook::Asm::xmm0;
		case sp::DHookRegister_XMM1:
			return KHook::Asm::xmm1;
		case sp::DHookRegister_XMM2:
			return KHook::Asm::xmm2;
		case sp::DHookRegister_XMM3:
			return KHook::Asm::xmm3;
		case sp::DHookRegister_XMM4:
			return KHook::Asm::xmm4;
		case sp::DHookRegister_XMM5:
			return KHook::Asm::xmm5;
		case sp::DHookRegister_XMM6:
			return KHook::Asm::xmm6;
		case sp::DHookRegister_XMM7:
			return KHook::Asm::xmm7;
	}
	return {};
}

}