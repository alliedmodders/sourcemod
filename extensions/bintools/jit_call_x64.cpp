/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod BinTools Extension
 * Copyright (C) 2004-2017 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 */

#include <sm_platform.h>
#include "extension.h"
#include <jit_helpers.h>
#include <x86_macros.h>
#include "x64_macros.h"
#include "jit_compile.h"

#if defined PLATFORM_WINDOWS
jit_uint32_t g_StackUsage = 32;	// Shadow space
#else
jit_uint32_t g_StackUsage = 0;
#endif
jit_uint32_t g_StackAlign = 0;
jit_uint32_t g_RegDecoder = 0;
jit_uint32_t g_FloatRegDecoder = 0;
jit_uint32_t g_PODCount = 0;
jit_uint32_t g_FloatCount = 0;
jit_uint32_t g_ParamCount = 0;
jit_uint32_t g_StackOffset = 0;
jit_uint8_t g_ThisPtrReg;

const jit_uint8_t STACK_PARAM = 16;
const jit_uint8_t INVALID_REG = 255;

inline jit_uint8_t NextScratchReg()
{
	switch (g_RegDecoder++ % 3)
	{
		case 0:
			return kREG_RAX;
		case 1:
			return kREG_R10;
		case 2:
			return kREG_R11;
		default:
			return INVALID_REG;
	}
}
 
#ifdef PLATFORM_POSIX
const int MAX_CLASSES = 2;
const int INT_REG_MAX = 6;
inline jit_uint8_t NextPODReg(size_t size)
{
	switch (g_PODCount++)
	{
		case 0:
			return kREG_RDI;
		case 1:
			return kREG_RSI;
		case 2:
			return kREG_RDX;
		case 3:
			return kREG_RCX;
		case 4:
			return kREG_R8;
		case 5:
			if (size == 16)
			{
				g_PODCount--;
				return STACK_PARAM;
			}
			return kREG_R9;
		default:
			return STACK_PARAM;
	}
}

const int FLOAT_REG_MAX = 8;
inline jit_uint8_t NextFloatReg(size_t size)
{
	switch (g_FloatCount++)
	{
		case 0:
			return kREG_XMM0;
		case 1:
			return kREG_XMM1;
		case 2:
			return kREG_XMM2;
		case 3:
			return kREG_XMM3;
		case 4:
			return kREG_XMM4;
		case 5:
			return kREG_XMM5;
		case 6:
			return kREG_XMM6;
		case 7:
			if (size == 16)
			{
				g_FloatCount--;
				return STACK_PARAM;
			}
			return kREG_XMM7;
		default:
			return STACK_PARAM;
	}
}

inline jit_uint8_t NextScratchFloatReg()
{
	switch (g_FloatRegDecoder++ % 8)
	{
		case 0:
			return kREG_XMM8;
		case 1:
			return kREG_XMM9;
		case 2:
			return kREG_XMM10;
		case 3:
			return kREG_XMM11;
		case 4:
			return kREG_XMM12;
		case 5:
			return kREG_XMM13;
		case 6:
			return kREG_XMM14;
		case 7:
			return kREG_XMM15;
		default:
			return INVALID_REG;
	}
}
#elif defined PLATFORM_WINDOWS
const int NUM_ARG_REGS = 4;
inline jit_uint8_t NextPODReg(size_t size)
{
	switch (g_ParamCount++)
	{
		case 0:
			return kREG_RCX;
		case 1:
			return kREG_RDX;
		case 2:
			return kREG_R8;
		case 3:
			return kREG_R9;
		default:
			return STACK_PARAM;
	}
}

inline jit_uint8_t NextFloatReg(size_t size)
{
	switch (g_ParamCount++)
	{
		case 0:
			return kREG_XMM0;
		case 1:
			return kREG_XMM1;
		case 2:
			return kREG_XMM2;
		case 3:
			return kREG_XMM3;
		default:
			return STACK_PARAM;
	}
}

inline jit_uint8_t NextScratchFloatReg()
{
	switch (g_FloatRegDecoder++ % 2)
	{
		case 0:
			return kREG_XMM4;
		case 1:
			return kREG_XMM5;
		default:
			return INVALID_REG;
	}
}
#endif

/********************
 * Assembly Opcodes *
 ********************/
inline void Write_Execution_Prologue(JitWriter *jit, bool is_void, bool has_params)
{
	//push rbp
	//mov rbp, rsp
	//if !is_void
	// push r14
	// mov r14, rsi  |  Windows: mov r14, rdx		; 2nd param (retbuf)
	//if has_params
	// push rbx
	// mov rbx, rdi  |  Windows: mov rbx, rcx		; 1st param (param stack)
	//push r15
	//mov r15, rsp
	//and rsp, 0xFFFFFFF0
	//sub rsp, <alignment>
	X64_Push_Reg(jit, kREG_RBP);
	X64_Mov_Reg_Rm(jit, kREG_RBP, kREG_RSP, MOD_REG);
	if (!is_void)
	{
		X64_Push_Reg(jit, kREG_R14);
#ifdef PLATFORM_POSIX
		X64_Mov_Reg_Rm(jit, kREG_R14, kREG_RSI, MOD_REG);
#elif defined PLATFORM_WINDOWS
		X64_Mov_Reg_Rm(jit, kREG_R14, kREG_RDX, MOD_REG);
#endif
	}
	if (has_params)
	{
		X64_Push_Reg(jit, kREG_RBX);
#ifdef PLATFORM_POSIX
		X64_Mov_Reg_Rm(jit, kREG_RBX, kREG_RDI, MOD_REG);
#elif defined PLATFORM_WINDOWS
		X64_Mov_Reg_Rm(jit, kREG_RBX, kREG_RCX, MOD_REG);
#endif
	}
	X64_Push_Reg(jit, kREG_R15);
	X64_Mov_Reg_Rm(jit, kREG_R15, kREG_RSP, MOD_REG);
	X64_And_Rm_Imm8(jit, kREG_RSP, MOD_REG, -16);

	if (!jit->outbase)
	{
		/* Alloc this instruction before knowing the real stack usage */
		X64_Sub_Rm_Imm32(jit, kREG_RSP, 1337, MOD_REG);
	} else {
		if (g_StackAlign)
			X64_Sub_Rm_Imm32(jit, kREG_RSP, g_StackAlign, MOD_REG);
	}
}

inline void Write_Function_Epilogue(JitWriter *jit, bool is_void, bool has_params)
{
	//mov rsp, r15
	//pop r15
	//if has_params
	// pop rbx
	//if !is_void
	// pop r14
	//mov rsp, rbp
	//pop rbp
	//ret
	X64_Mov_Reg_Rm(jit, kREG_RSP, kREG_R15, MOD_REG);
	X64_Pop_Reg(jit, kREG_R15);
	if (has_params)
		X64_Pop_Reg(jit, kREG_RBX);
	if (!is_void)
		X64_Pop_Reg(jit, kREG_R14);
	X64_Mov_Reg_Rm(jit, kREG_RSP, kREG_RBP, MOD_REG);
	X64_Pop_Reg(jit, kREG_RBP);
	X64_Return(jit);
}

inline jit_uint8_t Write_PushPOD(JitWriter *jit, const SourceHook::PassInfo *info, unsigned int offset)
{
	bool needStack = false;
	jit_uint8_t reg = NextPODReg(info->size);
	jit_uint8_t reg2;
	
	if (reg == STACK_PARAM)
	{
		reg = NextScratchReg();
		needStack = true;
	}

	if (info->flags & PASSFLAG_BYVAL)
	{
		switch (info->size)
		{
			case 1:
			{
				//movzx reg, BYTE PTR [ebx+<offset>]
				if (!offset)
					X64_Movzx_Reg64_Rm8(jit, reg, kREG_RBX, MOD_MEM_REG);
				else if (offset < SCHAR_MAX)
					X64_Movzx_Reg64_Rm8_Disp8(jit, reg, kREG_RBX, (jit_int8_t)offset);
				else
					X64_Movzx_Reg64_Rm8_Disp32(jit, reg, kREG_RBX, offset);

				break;
			}
			case 2:
			{
				//movzx reg, WORD PTR [ebx+<offset>]
				if (!offset)
					X64_Movzx_Reg64_Rm16(jit, reg, kREG_RBX, MOD_MEM_REG);
				else if (offset < SCHAR_MAX)
					X64_Movzx_Reg64_Rm16_Disp8(jit, reg, kREG_RBX, (jit_int8_t)offset);
				else
					X64_Movzx_Reg64_Rm16_Disp32(jit, reg, kREG_RBX, offset);

				break;
			}
			case 4:
			{
				//mov reg, DWORD PTR [ebx+<offset>]
				if (!offset)
					X64_Mov_Reg32_Rm(jit, reg, kREG_RBX, MOD_MEM_REG);
				else if (offset < SCHAR_MAX)
					X64_Mov_Reg32_Rm_Disp8(jit, reg, kREG_EBX, (jit_int8_t)offset);
				else
					X64_Mov_Reg32_Rm_Disp32(jit, reg, kREG_RBX, offset);

				break;
			}
			case 8:
			{
				//mov reg, DWORD PTR [ebx+<offset>]
				if (!offset)
					X64_Mov_Reg_Rm(jit, reg, kREG_RBX, MOD_MEM_REG);
				else if (offset < SCHAR_MAX)
					X64_Mov_Reg_Rm_Disp8(jit, reg, kREG_EBX, (jit_int8_t)offset);
				else
					X64_Mov_Reg_Rm_Disp32(jit, reg, kREG_RBX, offset);

				break;
			}
			case 16:
			{
				//mov reg, DWORD PTR [ebx+<offset>]
				//mov reg2, DWORD PTR [ebx+<offset>+8]
				reg2 = needStack ? NextScratchReg() : NextPODReg(8);

				if (!offset)
					X64_Mov_Reg_Rm(jit, reg, kREG_RBX, MOD_MEM_REG);
				else if (offset < SCHAR_MAX)
					X64_Mov_Reg_Rm_Disp8(jit, reg, kREG_RBX, (jit_int8_t)offset);
				else
					X64_Mov_Reg_Rm_Disp32(jit, reg, kREG_RBX, offset);
					
				if (offset+8 < SCHAR_MAX)
					X64_Mov_Reg_Rm_Disp8(jit, reg2, kREG_RBX, (jit_int8_t)(offset+8));
				else
					X64_Mov_Reg_Rm_Disp32(jit, reg2, kREG_RBX, offset+8);

				break;
			}
		}
	} else if (info->flags & PASSFLAG_BYREF) {
		//lea reg, [ebx+<offset>]
		if (!offset)
			X64_Mov_Reg_Rm(jit, reg, kREG_RBX, MOD_REG);
		else if (offset < SCHAR_MAX)
			X64_Lea_DispRegImm8(jit, reg, kREG_RBX, (jit_int8_t)offset);
		else
			X64_Lea_DispRegImm32(jit, reg, kREG_RBX, offset);
	}
	
	if (needStack)
	{
		// Move register value onto stack
		//if g_StackUsage == 0
		// mov [rsp], reg
		//else
		// mov [rsp+<g_StackUsage>], reg
		//if info->size == 16
		// mov [rsp+<g_StackUsage>+8], reg2
		if (g_StackUsage == 0)
			X64_Mov_RmRSP_Reg(jit, reg);
		else if (g_StackUsage < SCHAR_MAX)
			X64_Mov_RmRSP_Disp8_Reg(jit, reg, (jit_int8_t)g_StackUsage);
		else
			X64_Mov_RmRSP_Disp32_Reg(jit, reg, g_StackUsage);
			
		if (info->size == 16)
		{
			if (g_StackUsage + 8 < SCHAR_MAX)
				X64_Mov_RmRSP_Disp8_Reg(jit, reg2, (jit_int8_t)g_StackUsage + 8);
			else
				X64_Mov_RmRSP_Disp32_Reg(jit, reg2, (jit_int8_t)g_StackUsage + 8);
			g_StackUsage += 16;
		} else {
			g_StackUsage += 8;
		}
	}
	
	return reg;
}

inline void Write_PushFloat(JitWriter *jit, const SourceHook::PassInfo *info, unsigned int offset, uint8_t *floatRegs)
{
	bool needStack = false;
	jit_uint8_t floatReg = NextFloatReg(info->size);
	jit_uint8_t floatReg2;
	
	if (floatReg == STACK_PARAM)
	{
		floatReg = NextScratchFloatReg();
		needStack = true;
	}
	
	if (info->flags & PASSFLAG_BYVAL)
	{
		switch (info->size)
		{
			case 4:
			{
				//if offset % 16 == 0
				// movaps floatReg, [ebx+<offset>]
				//else
				// movups floatReg, [ebx+<offset>]
				if (!offset) {
					X64_Movaps_Rm(jit, floatReg, kREG_EBX);
				} else if (offset < SCHAR_MAX) {
					if (offset % 16 == 0)
						X64_Movaps_Rm_Disp8(jit, floatReg, kREG_EBX, (jit_int8_t)offset);
					else
						X64_Movups_Rm_Disp8(jit, floatReg, kREG_EBX, (jit_int8_t)offset);
				} else {
					if (offset % 16 == 0)
						X64_Movaps_Rm_Disp32(jit, floatReg, kREG_EBX, offset);
					else
						X64_Movups_Rm_Disp32(jit, floatReg, kREG_EBX, offset);
				}
				break;
			}
			case 8:
			{
				//if offset % 16 == 0
				// movapd floatReg, [ebx+<offset>]
				//else
				// movupd floatReg, [ebx+<offset>]
				if (!offset) {
					X64_Movapd_Rm(jit, floatReg, kREG_EBX);
				} else if (offset < SCHAR_MAX) {
					if (offset % 16 == 0)
						X64_Movapd_Rm_Disp8(jit, floatReg, kREG_EBX, (jit_int8_t)offset);
					else
						X64_Movupd_Rm_Disp8(jit, floatReg, kREG_EBX, (jit_int8_t)offset);
				} else {
					if (offset % 16 == 0)
						X64_Movapd_Rm_Disp32(jit, floatReg, kREG_EBX, offset);
					else
						X64_Movupd_Rm_Disp32(jit, floatReg, kREG_EBX, offset);
				}
				break;
			}
			case 16:
				//if offset % 16 == 0
				// movaps floatReg, [ebx+<offset>]
				//else
				// movads floatReg, [ebx+<offset>]
				//if (offset+8) % 16 == 0
				// movaps floatReg2, [ebx+<offset>+8]
				//else
				// movads floatReg2, [ebx+<offset>+8]
				floatReg2 = needStack ? NextScratchFloatReg() : NextFloatReg(8);

				if (!offset) {
					X64_Movaps_Rm(jit, floatReg, kREG_EBX);
					X64_Movups_Rm_Disp8(jit, floatReg2, kREG_EBX, offset+8);
				} else if (offset < SCHAR_MAX) {
					if (offset % 16 == 0)
						X64_Movaps_Rm_Disp8(jit, floatReg, kREG_EBX, (jit_int8_t)offset);
					else
						X64_Movups_Rm_Disp8(jit, floatReg, kREG_EBX, (jit_int8_t)offset);
					if ((offset + 8) % 16 == 0)
						X64_Movaps_Rm_Disp8(jit, floatReg2, kREG_EBX, (jit_int8_t)offset+8);
					else
						X64_Movups_Rm_Disp8(jit, floatReg2, kREG_EBX, (jit_int8_t)offset+8);
				} else {
					if (offset % 16 == 0)
						X64_Movaps_Rm_Disp32(jit, floatReg, kREG_EBX, offset);
					else
						X64_Movups_Rm_Disp32(jit, floatReg, kREG_EBX, offset);
					if ((offset + 8) % 16 == 0)
						X64_Movaps_Rm_Disp32(jit, floatReg2, kREG_EBX, offset+8);
					else
						X64_Movups_Rm_Disp32(jit, floatReg2, kREG_EBX, offset+8);
				}
		}
	} else if (info->flags & PASSFLAG_BYREF) {
		//lea reg, [ebx+<offset>]
		Write_PushPOD(jit, info, offset);
		return;
	}
	
	if (needStack)
	{
		// Move register value onto stack
		//if g_StackUsage == 0
		// movaps [rsp], floatReg
		//else
		// movaps [rsp+<g_StackUsage>], floatReg
		//if info->size == 16
		// movaps [rsp+<g_StackUsage>+8], floatReg2
		if (g_StackUsage == 0) {
			X64_Movaps_Rm_Reg(jit, kREG_RSP, floatReg);
		} else if (g_StackUsage < SCHAR_MAX) {
			if (g_StackUsage % 16 == 0)
				X64_Movaps_Rm_Disp8_Reg(jit, kREG_RSP, floatReg, (jit_int8_t)g_StackUsage);
			else
				X64_Movups_Rm_Disp8_Reg(jit, kREG_RSP, floatReg, (jit_int8_t)g_StackUsage);
		} else {
			if (g_StackUsage % 16 == 0)
				X64_Movaps_Rm_Disp32_Reg(jit, kREG_RSP, floatReg, g_StackUsage);
			else
				X64_Movups_Rm_Disp32_Reg(jit, kREG_RSP, floatReg, g_StackUsage);
		}
			
		if (info->size == 16)
		{
			if (g_StackUsage + 8 < SCHAR_MAX) {
				if ((g_StackUsage + 8) % 16 == 0)
					X64_Movaps_Rm_Disp8_Reg(jit, kREG_RSP, floatReg2, (jit_int8_t)g_StackUsage+8);
				else
					X64_Movups_Rm_Disp8_Reg(jit, kREG_RSP, floatReg2, (jit_int8_t)g_StackUsage+8);
			} else {
				if ((g_StackUsage + 8) % 16 == 0)
					X64_Movaps_Rm_Disp32_Reg(jit, kREG_RSP, floatReg, g_StackUsage+8);
				else
					X64_Movups_Rm_Disp32_Reg(jit, kREG_RSP, floatReg, g_StackUsage+8);
			}
			g_StackUsage += 16;
		} else {
			g_StackUsage += 8;
		}
	}
	
	if (floatRegs)
	{
		floatRegs[0] = floatReg;
		floatRegs[1] = info->size == 16 ? floatReg2 : INVALID_REG;
	}
}

#ifdef PLATFORM_WINDOWS
inline uint8_t MapFloatToIntReg(uint8_t floatReg)
{
	switch (floatReg)
	{
		case kREG_XMM0:
			return kREG_RCX;
		case kREG_XMM1:
			return kREG_RDX;
		case kREG_XMM2:
			return kREG_R8;
		case kREG_XMM3:
			return kREG_R9;
		default:
			return INVALID_REG;
	}
}

inline void Write_VarArgFloatCopy(JitWriter *jit, const SourceHook::PassInfo *info, uint8_t *floatRegs)
{
	if (!floatRegs || floatRegs[0] == STACK_PARAM)
		return;
		
	uint8_t intReg1 = MapFloatToIntReg(floatRegs[0]);

	switch (info->size)
	{
		case 4:
			//movd intReg1, floatReg[0]
			X64_Movd_Reg32_Xmm(jit, intReg1, floatRegs[0]);
			break;
		case 8:
			//movq intReg1, floatReg[0]
			X64_Movq_Reg_Xmm(jit, intReg1, floatRegs[0]);
			break;
		case 16:
			//movq intReg1, floatReg[0]
			//movq intReg2, floatReg[1]
			X64_Movq_Reg_Xmm(jit, intReg1, floatRegs[0]);
			if (floatRegs[1] != STACK_PARAM)
			{
				int intReg2 = MapFloatToIntReg(floatRegs[1]);
				X64_Movq_Reg_Xmm(jit, intReg2, floatRegs[1]);
			} 
			break;
	}
}
#endif // PLATFORM_WINDOWS


enum class ObjectClass
{
	None,
	Integer,
	Pointer,	// Special case of Integer
	SSE,
	SSEUp,
	X87,		// TODO? Really only applies to long doubles which Source doesn't use
	X87Up,		// TODO?
	X87Complex,	// TODO?
	Memory
};

inline ObjectClass ClassifyType(ObjectField field)
{
	switch (field)
	{
		case ObjectField::Boolean:
		case ObjectField::Int8:
		case ObjectField::Int16:
		case ObjectField::Int32:
		case ObjectField::Int64:
		case ObjectField::Pointer:
			return ObjectClass::Integer;
		case ObjectField::Float:
		case ObjectField::Double:
			return ObjectClass::SSE;
		default:
			return ObjectClass::None;
	}
}

inline size_t TypeSize(ObjectField field)
{
	switch (field)
	{
		case ObjectField::Boolean:
		case ObjectField::Int8:
			return 1;
		case ObjectField::Int16:
			return 2;
		case ObjectField::Int32:
		case ObjectField::Float:
			return 4;
		case ObjectField::Int64:
		case ObjectField::Pointer:
		case ObjectField::Double:
			return 8;
		default:
			assert(false);
			return 0;
	}
}

inline ObjectClass MergeClasses(ObjectClass class1, ObjectClass class2)
{
	if (class1 == class2)
		return class1;
	else if (class1 == ObjectClass::None)
		return class2;
	else if (class2 == ObjectClass::None)
		return class1;
	else if (class1 == ObjectClass::Integer || class2 == ObjectClass::Integer)
		return ObjectClass::Integer;
		
	return ObjectClass::SSE;
}

inline int ClassifyObject(const PassInfo *info, ObjectClass *classes)
{
	ObjectField *fields = info->fields;
	unsigned int numFields = info->numFields;
	int numWords = 1;

	if (info->size > 16 || info->flags & PASSFLAG_OUNALIGN)
		classes[0] = ObjectClass::Memory;
	else if (info->flags & (PASSFLAG_ODTOR|PASSFLAG_OCOPYCTOR))
		classes[0] = ObjectClass::Pointer;
	else if (info->size > 8)
		classes[0] = ObjectClass::None;
	else if (numFields == 1 || numFields == 2)
		classes[0] = ClassifyType(fields[0]);
	else
		classes[0] = ObjectClass::Integer;

	if (classes[0] == ObjectClass::None)
	{
		numWords = int((info->size + 7) / 8);

		unsigned int j = 0;
		for (int i = 0; i < numWords; i++)
		{
			classes[i] = ObjectClass::None;
			size_t sizeSoFar = 0;
			for (int k = 0; j < numFields; j++, k++)
			{
				size_t sz = TypeSize(fields[j]);
				if (sizeSoFar + sz > 8)
					break;
				else
					sizeSoFar += sz;

				if (k == 0 && sizeSoFar == 8) {
					classes[i] = ClassifyType(fields[j++]);
					break;
				} else if (j + 1 >= numFields) {
					break;
				}

				const ObjectField &field1 = fields[j];
				const ObjectField &field2 = fields[j + 1];
				
				classes[i] = MergeClasses(ClassifyType(field1), ClassifyType(field2));
			}
		}
	}
		
	return numWords;
}

inline void Write_PushObject(JitWriter *jit, const SourceHook::PassInfo *info, unsigned int offset,
                             const PassInfo *smInfo)
{
	if (info->flags & PASSFLAG_BYVAL)
	{
#ifdef PLATFORM_POSIX
		ObjectClass classes[MAX_CLASSES];
		int numWords = ClassifyObject(smInfo, classes);

		if (classes[0] == ObjectClass::Pointer)
			goto push_byref;

		int neededIntRegs = 0;
		int neededFloatRegs = 0;
		for (int i = 0; i < numWords; i++)
		{
			switch (classes[i])
			{
				case ObjectClass::Integer:
					neededIntRegs++;
					break;
				case ObjectClass::SSE:
					neededFloatRegs++;
					break;
				default:
					assert(false);
					break;
			}
		}
			
		if (neededIntRegs + g_PODCount > INT_REG_MAX || 
			 neededFloatRegs + g_FloatCount > FLOAT_REG_MAX)
			classes[0] = ObjectClass::Memory;

		if (classes[0] != ObjectClass::Memory)
		{
			size_t sizeLeft = info->size;
			for (int i = 0; i < numWords; i++)
			{
				switch (classes[i])
				{
					case ObjectClass::Integer:
					{
						SourceHook::PassInfo podInfo;
						podInfo.size = (sizeLeft > 8) ? 8 : sizeLeft;
						podInfo.type = SourceHook::PassInfo::PassType_Basic;
						podInfo.flags = SourceHook::PassInfo::PassFlag_ByVal;
						Write_PushPOD(jit, &podInfo, offset + (i * 8));
						break;
					}
					case ObjectClass::SSE:
					{
						SourceHook::PassInfo floatInfo;
						floatInfo.size = (sizeLeft > 8) ? 8 : sizeLeft;
						floatInfo.type = SourceHook::PassInfo::PassType_Float;
						floatInfo.flags = SourceHook::PassInfo::PassFlag_ByVal;
						Write_PushFloat(jit, &floatInfo, offset + (i * 8), nullptr);
						break;
					}
					default:
						assert(false);
						break;
				}
				if (sizeLeft > 8)
					sizeLeft -= 8;
			}
		}

		return;

#elif defined PLATFORM_WINDOWS
		if (info->size < 64 && (info->size & (info->size - 1)) == 0)
			goto push_byref;
		else {
			SourceHook::PassInfo podInfo;
			podInfo.size = info->size;
			podInfo.type = SourceHook::PassInfo::PassType_Basic;
			podInfo.flags = SourceHook::PassInfo::PassFlag_ByVal;
			Write_PushPOD(jit, &podInfo, offset);
		}
		
		return;
#endif

		jit_uint32_t qwords = info->size >> 3;
		jit_uint32_t bytes = info->size & 0x7;

		//sub rsp, <size>
		//cld
		//push rdi
		//push rsi
		//lea rdi, [rsp+16]
		//lea rsi, [rbx+<offs>]
		//if dwords
		// mov rcx, <dwords>
		// rep movsq
		//if bytes
		// mov rcx, <bytes>
		// rep movsb
		//pop rsi
		//pop rdi
		
		//if (info->size < SCHAR_MAX)
		//{
		//	X64_Sub_Rm_Imm8(jit, kREG_RSP, (jit_int8_t)info->size, MOD_REG);
		//} else {
		//	X64_Sub_Rm_Imm32(jit, kREG_RSP, info->size, MOD_REG);
		///}
		X64_Cld(jit);
		X64_Push_Reg(jit, kREG_RDI);
		X64_Push_Reg(jit, kREG_RSI);
		if (g_StackUsage + 16 < SCHAR_MAX)
			X64_Lea_Reg_DispRegMultImm8(jit, kREG_RDI, kREG_NOIDX, kREG_RSP, NOSCALE, g_StackUsage+16);
		else
			X64_Lea_Reg_DispRegMultImm32(jit, kREG_RDI, kREG_NOIDX, kREG_RSP, NOSCALE, g_StackUsage+16);

		if (!offset)
			X64_Mov_Reg_Rm(jit, kREG_RSI, kREG_RBX, MOD_REG);
		else if (offset < SCHAR_MAX)
			X64_Lea_DispRegImm8(jit, kREG_RSI, kREG_RBX, (jit_int8_t)offset);
		else
			X64_Lea_DispRegImm32(jit, kREG_RSI, kREG_RBX, offset);

		if (qwords)
		{
			X64_Mov_Reg_Imm32(jit, kREG_RCX, qwords);
			X64_Rep(jit);
			X64_Movsq(jit);
		}
		if (bytes)
		{
			X64_Mov_Reg_Imm32(jit, kREG_RCX, bytes);
			X64_Rep(jit);
			X64_Movsb(jit);
		}
		X64_Pop_Reg(jit, kREG_RSI);
		X64_Pop_Reg(jit, kREG_RDI);

		g_StackUsage += info->size;
	} else if (info->flags & PASSFLAG_BYREF) {
push_byref:
		//lea reg, [ebx+<offset>]
		SourceHook::PassInfo podInfo;
		podInfo.size = sizeof(void *);
		podInfo.type = SourceHook::PassInfo::PassType_Basic;
		podInfo.flags = SourceHook::PassInfo::PassFlag_ByRef;
		Write_PushPOD(jit, &podInfo, offset);
	}
}

inline void Write_PushThisPtr(JitWriter *jit)
{
	SourceHook::PassInfo podInfo;
	podInfo.size = 8;
	podInfo.type = SourceHook::PassInfo::PassType_Basic;
	podInfo.flags = SourceHook::PassInfo::PassFlag_ByVal;
	g_ThisPtrReg = Write_PushPOD(jit, &podInfo, 0);
}

inline void Write_PushRetBuffer(JitWriter *jit)
{
	jit_uint8_t reg = NextPODReg(8);
	
	//mov reg, r14
	X64_Mov_Reg_Rm(jit, reg, kREG_R14, MOD_REG);
}

inline void Write_CallFunction(JitWriter *jit, FuncAddrMethod method, CallWrapper *pWrapper)
{
	if (method == FuncAddr_Direct)
	{
		int64_t diff = (intptr_t)pWrapper->GetCalleeAddr() - ((intptr_t)jit->outbase + jit->get_outputpos() + 5);
		int32_t upperBits = (diff >> 32);
		if (upperBits == 0 || upperBits == -1) {
			//call <addr>
			jitoffs_t call = X64_Call_Imm32(jit, 0);
			X64_Write_Jump32_Abs(jit, call, pWrapper->GetCalleeAddr());
		} else {
			//mov rax, <addr>
			//call rax
			X64_Mov_Reg_Imm64(jit, kREG_RAX, (jit_int64_t)pWrapper->GetCalleeAddr());
			X64_Call_Reg(jit, kREG_RAX);
		}
	} else if (method == FuncAddr_VTable) {
		//*(this + thisOffs + vtblOffs)[vtblIdx]		
		// mov r10, [g_ThisPtrReg+<thisOffs>+<vtblOffs>]
		// mov r11, [r10+<vtablIdx>*8]
		// call r11
		SourceHook::MemFuncInfo *funcInfo = pWrapper->GetMemFuncInfo();
		jit_uint32_t total_offs = funcInfo->thisptroffs + funcInfo->vtbloffs;
		jit_uint32_t vfunc_pos = funcInfo->vtblindex * 8;

		//X64_Mov_Reg_Rm(jit, kREG_RAX, kREG_RBX, MOD_MEM_REG);
		if (total_offs < SCHAR_MAX)
		{
			X64_Mov_Reg_Rm_Disp8(jit, kREG_R10, g_ThisPtrReg, (jit_int8_t)total_offs);
		} else if (!total_offs) {
			X64_Mov_Reg_Rm(jit, kREG_R10, g_ThisPtrReg, MOD_MEM_REG);
		} else {
			X64_Mov_Reg_Rm_Disp32(jit, kREG_R10, g_ThisPtrReg, total_offs);
		}
		if (vfunc_pos < SCHAR_MAX)
		{
			X64_Mov_Reg_Rm_Disp8(jit, kREG_R11, kREG_R10, (jit_int8_t)vfunc_pos);
		} else if (!vfunc_pos) {
			X64_Mov_Reg_Rm(jit, kREG_R11, kREG_R10, MOD_MEM_REG);
		} else {
			X64_Mov_Reg_Rm_Disp32(jit, kREG_R11, kREG_R10, vfunc_pos);
		}
		X64_Call_Reg(jit, kREG_R11);
	}
}

inline void Write_VarArgFloatCount(JitWriter *jit)
{
	//mov al, g_FloatCount
	X64_Mov_Reg8_Imm8(jit, kREG_RAX, (jit_int8_t)g_FloatCount);
}

inline void Write_RectifyStack(JitWriter *jit, jit_uint32_t value)
{
	//add rsp, <value>
	if (value < SCHAR_MAX)
	{
		X64_Add_Rm_Imm8(jit, kREG_RSP, (jit_int8_t)value, MOD_REG);
	} else {
		X64_Add_Rm_Imm32(jit, kREG_RSP, value, MOD_REG);
	}
}

inline void Write_MovRet2Buf(JitWriter *jit, const PassInfo *pRet, ObjectClass *classes, int numWords)
{
	if (pRet->type == PassType_Float)
	{
		switch (pRet->size)
		{
		case 4:
			{
				//movups xmmword ptr [r14], xmm0
				X64_Movups_Rm_Reg(jit, kREG_R14, kREG_XMM0);
				break;
			}
		case 8:
			{
				//movupd xmmword ptr [r14], xmm0
				X64_Movupd_Rm_Reg(jit, kREG_R14, kREG_XMM0);
				break;
			}
		}
		return;
	} 
	else if (pRet->type == PassType_Basic)
	{
		switch (pRet->size)
		{
		case 1:
			{
				//mov BYTE PTR [r14], al
				X64_Mov_Rm8_Reg8(jit, kREG_R14, kREG_RAX, MOD_MEM_REG);
				break;
			}
		case 2:
			{
				//mov WORD PTR [r14], ax
				X64_Mov_Rm16_Reg16(jit, kREG_R14, kREG_RAX, MOD_MEM_REG);
				break;
			}
		case 4:
			{
				//mov DWORD PTR [r14], rax
				X64_Mov_Rm32_Reg32(jit, kREG_R14, kREG_RAX, MOD_MEM_REG);
				break;
			}
		case 8:
			{
				//mov QWORD PTR [r14], rax
				X64_Mov_Rm_Reg(jit, kREG_R14, kREG_RAX, MOD_MEM_REG);
				break;
			}
		}
	}
#ifdef PLATFORM_POSIX
	else
	{
		// Return value registers
		jit_uint8_t intRetReg = kREG_RAX;
		jit_uint8_t floatRetReg = kREG_XMM0;
		jit_int8_t offset = 0;

		assert(numWords <= 2);

		for (int i = 0; i < numWords; i++)
		{
			ObjectClass &cls = classes[i];
		
			if (cls == ObjectClass::Integer)
			{
				//mov QWORD PTR [r14+offset], intRetReg 		; rax or rdx
				if (!offset)
					X64_Mov_Rm_Reg(jit, kREG_R14, intRetReg, MOD_MEM_REG);
				else
					X64_Mov_Rm_Reg_Disp8(jit, kREG_R14, intRetReg, offset);
				intRetReg = kREG_RDX;
			}
			else if (cls == ObjectClass::SSE)
			{
				//movupd xmmword ptr [r14+offset], floatRetReg 	; xmm0 or xmm1
				if (!offset)
					X64_Movupd_Rm_Reg(jit, kREG_R14, floatRetReg);
				else
					X64_Movupd_Rm_Disp8_Reg(jit, kREG_R14, floatRetReg, offset);
				floatRetReg = kREG_XMM1;
			}
			offset += 8;
		}
	}
#endif
}

/******************************
 * Assembly Compiler Function *
 ******************************/

void *JIT_CallCompile(CallWrapper *pWrapper, FuncAddrMethod method)
{
	JitWriter writer;
	JitWriter *jit = &writer;

	jit_uint32_t CodeSize = 0;
	bool Needs_Retbuf = false;
	CallConvention Convention = pWrapper->GetCallConvention();
	jit_uint32_t ParamCount = pWrapper->GetParamCount();
	const PassInfo *pRet = pWrapper->GetReturnInfo();
	bool hasParams = (ParamCount || Convention == CallConv_ThisCall);
#ifdef PLATFORM_POSIX
	ObjectClass classes[MAX_CLASSES];
	int numWords;
#endif

	g_StackUsage = 0;

	writer.outbase = NULL;
	writer.outptr = NULL;

jit_rewind:
	/* Write function prologue */
	Write_Execution_Prologue(jit, (pRet) ? false : true, hasParams);
	
#if defined PLATFORM_WINDOWS
	/* This ptr is always first on Windows */
	if (Convention == CallConv_ThisCall)
	{
		Write_PushThisPtr(jit);
	}
#endif
	
	/* Skip the return buffer stuff if this is a void function */
	if (!pRet)
	{
		goto skip_retbuffer;
	}

	if ((pRet->type == PassType_Object) && (pRet->flags & PASSFLAG_BYVAL))
	{
#ifdef PLATFORM_POSIX
		numWords = ClassifyObject(pRet, classes);
		
		if (classes[0] == ObjectClass::Memory || classes[0] == ObjectClass::Pointer)
			Needs_Retbuf = true;
#elif defined PLATFORM_WINDOWS
	if (pRet->size > 8 || (pRet->size & (pRet->size - 1)) != 0 || 
	   (pRet->flags & PASSFLAG_ODTOR|PASSFLAG_OCTOR|PASSFLAG_OASSIGNOP))
		Needs_Retbuf = true;
#endif
	}

	/* Prepare the return buffer in case we are returning objects by value. */
	if (Needs_Retbuf)
	{
		Write_PushRetBuffer(jit);
	}
	
skip_retbuffer:
#if defined PLATFORM_POSIX
	/* This ptr comes after retbuf ptr on Linux/macOS */
	if (Convention == CallConv_ThisCall)
	{
		Write_PushThisPtr(jit);
	}
#endif

	/* Write parameter push code */
	for (jit_uint32_t i = 0; i < ParamCount; i++)
	{
		unsigned int offset = pWrapper->GetParamOffset(i);
		const SourceHook::PassInfo *info = pWrapper->GetSHParamInfo(i);
		assert(info != NULL);

		switch (info->type)
		{
		case SourceHook::PassInfo::PassType_Basic:
			{
				Write_PushPOD(jit, info, offset);
				break;
			}
		case SourceHook::PassInfo::PassType_Float:
			{
#ifdef PLATFORM_WINDOWS
				if ((info->flags & PASSFLAG_BYVAL) && (pWrapper->GetFunctionFlags() & FNFLAG_VARARGS))
				{
					uint8_t floatRegs[2];
					Write_PushFloat(jit, info, offset, floatRegs);
					Write_VarArgFloatCopy(jit, info, floatRegs);
				}
				else
#endif
				{
					Write_PushFloat(jit, info, offset, nullptr);
				}
				break;
			}
		case SourceHook::PassInfo::PassType_Object:
			{
				const PassEncode *paramInfo = pWrapper->GetParamInfo(i);
				Write_PushObject(jit, info, offset, &paramInfo->info);
				break;
			}
		}
	}

	/* Write the calling code */
	Write_CallFunction(jit, method, pWrapper);
	
#ifdef PLATFORM_POSIX
	if (pWrapper->GetFunctionFlags() & FNFLAG_VARARGS)
		Write_VarArgFloatCount(jit);
#endif

	/* Clean up the calling stack */
	if (hasParams && g_StackUsage)
		Write_RectifyStack(jit, g_StackAlign);

	/* Copy the return type to the return buffer if the function is not void */
	if (pRet && !Needs_Retbuf)
	{
#ifdef PLATFORM_POSIX
		Write_MovRet2Buf(jit, pRet, classes, numWords);
#elif defined PLATFORM_WINDOWS
		Write_MovRet2Buf(jit, pRet, nullptr, 0);
#endif
	}

	/* Write Function Epilogue */
	Write_Function_Epilogue(jit, (pRet) ? false : true, hasParams);

	if (writer.outbase == NULL)
	{
		CodeSize = writer.get_outputpos();
		writer.outbase = (jitcode_t)g_SPEngine->AllocatePageMemory(CodeSize);
		g_SPEngine->SetReadWrite(writer.outbase);
		writer.outptr = writer.outbase;
		pWrapper->SetCodeBaseAddr(writer.outbase);
		g_StackAlign = (g_StackUsage) ? ((g_StackUsage & 0xFFFFFFF0) + 16) - g_StackUsage : 0;
		g_StackAlign = (g_StackAlign == 16) ? g_StackUsage : g_StackUsage + g_StackAlign;
#ifdef PLATFORM_POSIX
		g_StackUsage = 0;
#elif defined PLATFORM_WINDOWS
		g_StackUsage = 32;	// Shadow space
#endif
		g_RegDecoder = 0;
		g_FloatRegDecoder = 0;
		g_PODCount = 0;
		g_FloatCount = 0;
		g_ParamCount = 0;
		Needs_Retbuf = false;
		goto jit_rewind;
	}
	g_SPEngine->SetReadExecute(writer.outbase);
	return writer.outbase;
}
