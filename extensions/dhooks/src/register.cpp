#include "register.hpp"

namespace dhooks {

std::optional<AsmRegCode> Translate_DHookRegister(sp::DHookRegister reg) {
	switch (reg) {
		case sp::DHookRegister_AL:
		case sp::DHookRegister_AH:
		case sp::DHookRegister_EAX:
#ifdef DHOOKS_X86_64
		case sp::DHookRegister_RAX:
		return KHook::Asm::rax;
#else
		return KHook::Asm::eax;
#endif
		case sp::DHookRegister_CL:
		case sp::DHookRegister_CH:
		case sp::DHookRegister_ECX:
#ifdef DHOOKS_X86_64
		case sp::DHookRegister_RCX:
		return KHook::Asm::rcx;
#else
		return KHook::Asm::ecx;
#endif
		case sp::DHookRegister_DL:
		case sp::DHookRegister_DH:
		case sp::DHookRegister_EDX:
#ifdef DHOOKS_X86_64
		case sp::DHookRegister_RDX:
		return KHook::Asm::rdx;
#else
		return KHook::Asm::edx;
#endif
		case sp::DHookRegister_BL:
		case sp::DHookRegister_BH:
		case sp::DHookRegister_EBX:
#ifdef DHOOKS_X86_64
		case sp::DHookRegister_RBX:
		return KHook::Asm::rbx;
#else
		return KHook::Asm::ebx;
#endif
		case sp::DHookRegister_ESP:
#ifdef DHOOKS_X86_64
		case sp::DHookRegister_RSP:
		return KHook::Asm::rsp;
#else
		return KHook::Asm::esp;
#endif
		case sp::DHookRegister_EBP:
#ifdef DHOOKS_X86_64
		case sp::DHookRegister_RBP:
		return KHook::Asm::rbp;
#else
		return KHook::Asm::ebp;
#endif
		case sp::DHookRegister_ESI:
#ifdef DHOOKS_X86_64
		case sp::DHookRegister_RSI:
		return KHook::Asm::rsi;
#else
		return KHook::Asm::esi;
#endif
		case sp::DHookRegister_EDI:
#ifdef DHOOKS_X86_64
		case sp::DHookRegister_RDI:
		return KHook::Asm::rdi;
#else
		return KHook::Asm::edi;
#endif
#ifdef DHOOKS_X86_64
		case sp::DHookRegister_R8:
		return KHook::Asm::r8;
		case sp::DHookRegister_R9:
		return KHook::Asm::r9;
		case sp::DHookRegister_R10:
		return KHook::Asm::r10;
		case sp::DHookRegister_R11:
		return KHook::Asm::r11;
		case sp::DHookRegister_R12:
		return KHook::Asm::r12;
		case sp::DHookRegister_R13:
		return KHook::Asm::r13;
		case sp::DHookRegister_R14:
		return KHook::Asm::r14;
		case sp::DHookRegister_R15:
		return KHook::Asm::r15;
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
#ifdef DHOOKS_X86_64
		case sp::DHookRegister_XMM8:
			return KHook::Asm::xmm8;
		case sp::DHookRegister_XMM9:
			return KHook::Asm::xmm9;
		case sp::DHookRegister_XMM10:
			return KHook::Asm::xmm10;
		case sp::DHookRegister_XMM11:
			return KHook::Asm::xmm11;
		case sp::DHookRegister_XMM12:
			return KHook::Asm::xmm12;
		case sp::DHookRegister_XMM13:
			return KHook::Asm::xmm13;
		case sp::DHookRegister_XMM14:
			return KHook::Asm::xmm14;
		case sp::DHookRegister_XMM15:
			return KHook::Asm::xmm15;
#endif
	}
	return {};
}

}