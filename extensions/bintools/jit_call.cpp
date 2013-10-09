/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod BinTools Extension
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
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
 *
 * Version: $Id$
 */

#include <sm_platform.h>
#include "extension.h"
#include <jit_helpers.h>
#include <x86_macros.h>
#include "jit_compile.h"

jit_uint32_t g_StackUsage = 0;
jit_uint32_t g_StackAlign = 0;
jit_uint32_t g_RegDecoder = 0;

/********************
 * Assembly Opcodes *
 ********************/

inline void Write_Execution_Prologue(JitWriter *jit, bool is_void, bool has_params)
{
	//push ebp
	//mov ebp, esp
	//if !is_void
	// push edi
	// mov edi, [ebp+12]
	//if has_params
	// push ebx
	// mov ebx, [ebp+8]
	//push esi
	//mov esi, esp
	//and esp, 0xFFFFFFF0
	//sub esp, <alignment>
	IA32_Push_Reg(jit, kREG_EBP);
	IA32_Mov_Reg_Rm(jit, kREG_EBP, kREG_ESP, MOD_REG);
	if (!is_void)
	{
		IA32_Push_Reg(jit, kREG_EDI);
		IA32_Mov_Reg_Rm_Disp8(jit, kREG_EDI, kREG_EBP, 12);
	}
	if (has_params)
	{
		IA32_Push_Reg(jit, kREG_EBX);
		IA32_Mov_Reg_Rm_Disp8(jit, kREG_EBX, kREG_EBP, 8);
	}
	IA32_Push_Reg(jit, kREG_ESI);
	IA32_Mov_Reg_Rm(jit, kREG_ESI, kREG_ESP, MOD_REG);
	IA32_And_Rm_Imm8(jit, kREG_ESP, MOD_REG, -16);

	if (!jit->outbase)
	{
		/* Alloc this instruction before knowing the real stack usage */
		IA32_Sub_Rm_Imm32(jit, kREG_ESP, 1337, MOD_REG);
	} else {
		if (g_StackAlign)
		{
			IA32_Sub_Rm_Imm32(jit, kREG_ESP, g_StackAlign, MOD_REG);
		}
	}
}

inline void Write_Function_Epilogue(JitWriter *jit, bool is_void, bool has_params)
{
	//mov esp, esi
	//pop esi
	//if has_params
	// pop ebx
	//if !is_void
	// pop edi
	//mov esp, ebp
	//pop ebp
	//ret
	IA32_Mov_Reg_Rm(jit, kREG_ESP, kREG_ESI, MOD_REG);
	IA32_Pop_Reg(jit, kREG_ESI);
	if (has_params)
	{
		IA32_Pop_Reg(jit, kREG_EBX);
	}
	if (!is_void)
	{
		IA32_Pop_Reg(jit, kREG_EDI);
	}
	IA32_Mov_Reg_Rm(jit, kREG_ESP, kREG_EBP, MOD_REG);
	IA32_Pop_Reg(jit, kREG_EBP);
	IA32_Return(jit);
}

inline void Write_PushPOD(JitWriter *jit, const SourceHook::PassInfo *info, unsigned int offset)
{
	jit_uint8_t reg = _DecodeRegister3(g_RegDecoder++);

	if (info->flags & PASSFLAG_BYVAL)
	{
		switch (info->size)
		{
		case 1:
			{
				//movzx reg, BYTE PTR [ebx+<offset>]
				//push reg
				if (offset < SCHAR_MAX)
				{
					IA32_Movzx_Reg32_Rm8_Disp8(jit, reg, kREG_EBX, (jit_int8_t)offset);
				} else if (!offset) {
					IA32_Movzx_Reg32_Rm8(jit, reg, kREG_EBX, MOD_MEM_REG);
				} else {
					IA32_Movzx_Reg32_Rm8_Disp32(jit, reg, kREG_EBX, offset);
				}
				IA32_Push_Reg(jit, reg);

				g_StackUsage += 4;
				break;
			}
		case 2:
			{
				//movzx reg, WORD PTR [ebx+<offset>]
				//push reg
				jit->write_ubyte(IA32_16BIT_PREFIX);
				if (offset < SCHAR_MAX)
				{
					IA32_Movzx_Reg32_Rm16_Disp8(jit, reg, kREG_EBX, (jit_int8_t)offset);
				} else if (!offset) {
					IA32_Movzx_Reg32_Rm16(jit, reg, kREG_EBX, MOD_MEM_REG);
				} else {
					IA32_Movzx_Reg32_Rm16_Disp32(jit, reg, kREG_EBX, offset);
				}
				IA32_Push_Reg(jit, reg);

				g_StackUsage += 4;
				break;
			}
		case 4:
			{
				//mov reg, DWORD PTR [ebx+<offset>]
				//push reg
				if (offset < SCHAR_MAX)
				{
					IA32_Mov_Reg_Rm_Disp8(jit, reg, kREG_EBX, (jit_int8_t)offset);
				} else if (!offset) {
					IA32_Mov_Reg_Rm(jit, reg, kREG_EBX, MOD_MEM_REG);
				} else {
					IA32_Mov_Reg_Rm_Disp32(jit, reg, kREG_EBX, offset);
				}
				IA32_Push_Reg(jit, reg);

				g_StackUsage += 4;
				break;
			}
		case 8:
			{
				//mov reg, DWORD PTR [ebx+<offset>+4]
				//mov reg2, DWORD PTR [ebx+<offset>]
				//push reg
				//push reg2
				jit_uint8_t reg2 = _DecodeRegister3(g_RegDecoder++);

				if (offset+4 < SCHAR_MAX)
				{
					IA32_Mov_Reg_Rm_Disp8(jit, reg, kREG_EBX, (jit_int8_t)(offset+4));
				} else {
					IA32_Mov_Reg_Rm_Disp32(jit, reg, kREG_EBX, offset+4);
				}
				if (offset < SCHAR_MAX)
				{
					IA32_Mov_Reg_Rm_Disp8(jit, reg2, kREG_EBX, (jit_int8_t)offset);
				} else if (!offset) {
					IA32_Mov_Reg_Rm(jit, reg2, kREG_EBX, MOD_MEM_REG);
				} else {
					IA32_Mov_Reg_Rm_Disp32(jit, reg2, kREG_EBX, offset);
				}
				IA32_Push_Reg(jit, reg);
				IA32_Push_Reg(jit, reg2);

				g_StackUsage += 8;
				break;
			}
		}
	} else if (info->flags & PASSFLAG_BYREF) {
		//lea reg, [ebx+<offset>]
		//push reg
		if (!offset)
		{
			IA32_Push_Reg(jit, kREG_EBX);
			g_StackUsage += 4;
			return;
		}
		if (offset < SCHAR_MAX)
		{
			IA32_Lea_DispRegImm8(jit, reg, kREG_EBX, (jit_int8_t)offset);
		} else {
			IA32_Lea_DispRegImm32(jit, reg, kREG_EBX, offset);
		}
		IA32_Push_Reg(jit, reg);

		g_StackUsage += 4;
	}
}

inline void Write_PushFloat(JitWriter *jit, const SourceHook::PassInfo *info, unsigned int offset)
{
	if (info->flags & PASSFLAG_BYVAL)
	{
		switch (info->size)
		{
		case 4:
			{
				//fld DWORD PTR [ebx+<offset>]
				//push reg
				//fstp DWORD PTR [esp]
				if (offset < SCHAR_MAX)
				{
					IA32_Fld_Mem32_Disp8(jit, kREG_EBX, (jit_int8_t)offset);
				} else if (!offset) {
					IA32_Fld_Mem32(jit, kREG_EBX);
				} else {
					IA32_Fld_Mem32_Disp32(jit, kREG_EBX, offset);
				}
				IA32_Push_Reg(jit, _DecodeRegister3(g_RegDecoder++));
				IA32_Fstp_Mem32_ESP(jit);
				g_StackUsage += 4;
				break;
			}
		case 8:
			{
				//fld QWORD PTR [ebx+<offset>]
				//sub esp, 8
				//fstp QWORD PTR [esp]
				if (offset < SCHAR_MAX)
				{
					IA32_Fld_Mem64_Disp8(jit, kREG_EBX, (jit_int8_t)offset);
				} else if (!offset) {
					IA32_Fld_Mem64(jit, kREG_EBX);
				} else {
					IA32_Fld_Mem64_Disp32(jit, kREG_EBX, offset);
				}
				IA32_Sub_Rm_Imm8(jit, kREG_ESP, 8, MOD_REG);
				IA32_Fstp_Mem64_ESP(jit);
				g_StackUsage += 8;
				break;
			}
		}
	} else if (info->flags & PASSFLAG_BYREF) {
		//lea reg, [ebx+<offset>]
		//push reg
		if (!offset)
		{
			IA32_Push_Reg(jit, kREG_EBX);
			g_StackUsage += 4;
			return;
		}

		jit_uint8_t reg = _DecodeRegister3(g_RegDecoder++);
		if (offset < SCHAR_MAX)
		{
			IA32_Lea_DispRegImm8(jit, reg, kREG_EBX, (jit_int8_t)offset);
		} else {
			IA32_Lea_DispRegImm32(jit, reg, kREG_EBX, offset);
		}
		IA32_Push_Reg(jit, reg);

		g_StackUsage += 4;
	}
}

inline void Write_PushObject(JitWriter *jit, const SourceHook::PassInfo *info, unsigned int offset)
{
	if (info->flags & PASSFLAG_BYVAL)
	{
#ifdef PLATFORM_POSIX
		if (info->flags & PASSFLAG_ODTOR)
		{
			goto push_byref;
		}
#endif
		jit_uint32_t dwords = info->size >> 2;
		jit_uint32_t bytes = info->size & 0x3;

		//sub esp, <size>
		//cld
		//push edi
		//push esi
		//lea edi, [esp+8]
		//lea esi, [ebx+<offs>]
		//if dwords
		// mov ecx, <dwords>
		// rep movsd
		//if bytes
		// mov ecx, <bytes>
		// rep movsb
		//pop esi
		//pop edi
		if (info->size < SCHAR_MAX)
		{
			IA32_Sub_Rm_Imm8(jit, kREG_ESP, (jit_int8_t)info->size, MOD_REG);
		} else {
			IA32_Sub_Rm_Imm32(jit, kREG_ESP, info->size, MOD_REG);
		}
		IA32_Cld(jit);
		IA32_Push_Reg(jit, kREG_EDI);
		IA32_Push_Reg(jit, kREG_ESI);
		IA32_Lea_Reg_DispRegMultImm8(jit, kREG_EDI, kREG_NOIDX, kREG_ESP, NOSCALE, 8);
		if (offset < SCHAR_MAX)
		{
			IA32_Lea_DispRegImm8(jit, kREG_ESI, kREG_EBX, (jit_int8_t)offset);
		} else if (!offset) {
			IA32_Mov_Reg_Rm(jit, kREG_ESI, kREG_EBX, MOD_REG);
		} else {
			IA32_Lea_DispRegImm32(jit, kREG_ESI, kREG_EBX, offset);
		}
		if (dwords)
		{
			IA32_Mov_Reg_Imm32(jit, kREG_ECX, dwords);
			IA32_Rep(jit);
			IA32_Movsd(jit);
		}
		if (bytes)
		{
			IA32_Mov_Reg_Imm32(jit, kREG_ECX, bytes);
			IA32_Rep(jit);
			IA32_Movsb(jit);
		}
		IA32_Pop_Reg(jit, kREG_ESI);
		IA32_Pop_Reg(jit, kREG_EDI);

		g_StackUsage += info->size;
	} else if (info->flags & PASSFLAG_BYREF) {
#ifdef PLATFORM_POSIX
push_byref:
#endif
		if (!offset)
		{
			IA32_Push_Reg(jit, kREG_EBX);
			g_StackUsage += 4;
			return;
		}

		//lea reg, [ebx+<offset>]
		//push reg
		jit_uint8_t reg = _DecodeRegister3(g_RegDecoder++);
		if (offset < SCHAR_MAX)
		{
			IA32_Lea_DispRegImm8(jit, reg, kREG_EBX, (jit_int8_t)offset);
		} else {
			IA32_Lea_DispRegImm32(jit, reg, kREG_EBX, offset);
		}
		IA32_Push_Reg(jit, reg);

		g_StackUsage += 4;
	}
}

inline void Write_PushThisPtr(JitWriter *jit)
{
#ifdef PLATFORM_POSIX
	//mov reg, [ebx]
	//push reg
	jit_uint8_t reg = _DecodeRegister3(g_RegDecoder++);

	IA32_Mov_Reg_Rm(jit, reg, kREG_EBX, MOD_MEM_REG);
	IA32_Push_Reg(jit, reg);

	g_StackUsage += 4;
#elif defined PLATFORM_WINDOWS
	//mov ecx, [ebx]
	IA32_Mov_Reg_Rm(jit, kREG_ECX, kREG_EBX, MOD_MEM_REG);
#endif
}

inline void Write_PushRetBuffer(JitWriter *jit)
{
	//push edi
	IA32_Push_Reg(jit, kREG_EDI);
}

inline void Write_CallFunction(JitWriter *jit, FuncAddrMethod method, CallWrapper *pWrapper)
{
	if (method == FuncAddr_Direct)
	{
		//call <addr>
		jitoffs_t call = IA32_Call_Imm32(jit, 0);
		IA32_Write_Jump32_Abs(jit, call, pWrapper->GetCalleeAddr());
	} else if (method == FuncAddr_VTable) {
		//*(this + thisOffs + vtblOffs)[vtblIdx]
		//mov edx, [ebx]
		//mov eax, [edx+<thisOffs>+<vtblOffs>]
		//mov edx, [eax+<vtblIdx>*4]
		//call edx
		SourceHook::MemFuncInfo *funcInfo = pWrapper->GetMemFuncInfo();
		jit_uint32_t total_offs = funcInfo->thisptroffs + funcInfo->vtbloffs;
		jit_uint32_t vfunc_pos = funcInfo->vtblindex * 4;

		IA32_Mov_Reg_Rm(jit, kREG_EDX, kREG_EBX, MOD_MEM_REG);
		if (total_offs < SCHAR_MAX)
		{
			IA32_Mov_Reg_Rm_Disp8(jit, kREG_EAX, kREG_EDX, (jit_int8_t)total_offs);
		} else if (!total_offs) {
			IA32_Mov_Reg_Rm(jit, kREG_EAX, kREG_EDX, MOD_MEM_REG);
		} else {
			IA32_Mov_Reg_Rm_Disp32(jit, kREG_EAX, kREG_EDX, total_offs);
		}
		if (vfunc_pos < SCHAR_MAX)
		{
			IA32_Mov_Reg_Rm_Disp8(jit, kREG_EDX, kREG_EAX, (jit_int8_t)vfunc_pos);
		} else if (!vfunc_pos) {
			IA32_Mov_Reg_Rm(jit, kREG_EDX, kREG_EAX, MOD_MEM_REG);
		} else {
			IA32_Mov_Reg_Rm_Disp32(jit, kREG_EDX, kREG_EAX, vfunc_pos);
		}
		IA32_Call_Reg(jit, kREG_EDX);
	}
}

inline void Write_RectifyStack(JitWriter *jit, jit_uint32_t value)
{
	//add esp, <value>
	if (value < SCHAR_MAX)
	{
		IA32_Add_Rm_Imm8(jit, kREG_ESP, (jit_int8_t)value, MOD_REG);
	} else {
		IA32_Add_Rm_Imm32(jit, kREG_ESP, value, MOD_REG);
	}
}

inline void Write_MovRet2Buf(JitWriter *jit, const PassInfo *pRet)
{
	if (pRet->type == PassType_Float)
	{
		switch (pRet->size)
		{
		case 4:
			{
				//fstp DWORD PTR [edi]
				IA32_Fstp_Mem32(jit, kREG_EDI);
				break;
			}
		case 8:
			{
				//fstp QWORD PTR [edi]
				IA32_Fstp_Mem64(jit, kREG_EDI);
				break;
			}
		}
		return;
	}

	switch (pRet->size)
	{
	case 1:
		{
			//mov BYTE PTR [edi], al
			IA32_Mov_Rm8_Reg8(jit, kREG_EDI, kREG_EAX, MOD_MEM_REG);
			break;
		}
	case 2:
		{
			//mov WORD PTR [edi], ax
			jit->write_ubyte(IA32_16BIT_PREFIX);
			IA32_Mov_Rm_Reg(jit, kREG_EDI, kREG_EAX, MOD_MEM_REG);
			break;
		}
	case 4:
		{
			//mov DWORD PTR [edi], eax
			IA32_Mov_Rm_Reg(jit, kREG_EDI, kREG_EAX, MOD_MEM_REG);
			break;
		}
	case 8:
		{
			//mov DWORD PTR [edi], eax
			//mov DWORD PTR [edi+4], edx
			IA32_Mov_Rm_Reg(jit, kREG_EDI, kREG_EAX, MOD_MEM_REG);
			IA32_Mov_Rm_Reg_Disp8(jit, kREG_EDI, kREG_EDX, 4);
			break;
		}
	}
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

	g_StackUsage = 0;

	writer.outbase = NULL;
	writer.outptr = NULL;

jit_rewind:
	/* Write function prologue */
	Write_Execution_Prologue(jit, (pRet) ? false : true, hasParams);

	/* Write parameter push code */
	for (jit_int32_t i=ParamCount-1; i>=0; i--)
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
				Write_PushFloat(jit, info, offset);
				break;
			}
		case SourceHook::PassInfo::PassType_Object:
			{
				Write_PushObject(jit, info, offset);
				break;
			}
		}
	}

	/* Prepare the this ptr if applicable */
	if (Convention == CallConv_ThisCall)
	{
		Write_PushThisPtr(jit);
	}

	/* Skip the return buffer stuff if this is a void function */
	if (!pRet)
	{
		goto skip_retbuffer;
	}

	if ((pRet->type == PassType_Object) && (pRet->flags & PASSFLAG_BYVAL))
	{
#ifdef PLATFORM_LINUX
		Needs_Retbuf = true;
#elif defined PLATFORM_APPLE
		/*
		 * On OS X, need retbuf if size > 8 or not power of 2.
		 *
		 * See Mac OS X ABI Function Call Guide:
		 * http://developer.apple.com/mac/library/DOCUMENTATION/DeveloperTools/Conceptual/LowLevelABI/130-IA-32_Function_Calling_Conventions/IA32.html#//apple_ref/doc/uid/TP40002492-SW5
		 */
		if (pRet->size > 8 || (pRet->size & (pRet->size - 1)) != 0)
		{
			Needs_Retbuf = true;
		}
#elif defined PLATFORM_WINDOWS
		if ((Convention == CallConv_ThisCall) ||
			((Convention == CallConv_Cdecl) &&
			((pRet->size > 8) || (pRet->flags & PASSFLAG_ODTOR|PASSFLAG_OCTOR|PASSFLAG_OASSIGNOP))))
		{
			Needs_Retbuf = true;
		}
#endif
	}

	/* Prepare the return buffer in case we are returning objects by value. */
	if (Needs_Retbuf)
	{
		Write_PushRetBuffer(jit);
	}

skip_retbuffer:
	/* Write the calling code */
	Write_CallFunction(jit, method, pWrapper);

	/* Clean up the calling stack */
#ifdef PLATFORM_WINDOWS
	if ((ParamCount || Needs_Retbuf) && (Convention == CallConv_Cdecl))
	{
		/* Pop all parameters from the stack + hidden return pointer */
		jit_uint32_t total = (Needs_Retbuf) ? g_StackUsage + sizeof(void *) : g_StackUsage;
		Write_RectifyStack(jit, total);
#elif defined PLATFORM_POSIX
	if (hasParams)
	{
		/* Pop all parameters from the stack */
		Write_RectifyStack(jit, g_StackUsage);
#endif
	}

	/* Copy the return type to the return buffer if the function is not void */
	if (pRet && !Needs_Retbuf)
	{
		Write_MovRet2Buf(jit, pRet);
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
		g_StackUsage = 0;
		g_RegDecoder = 0;
		Needs_Retbuf = false;
		goto jit_rewind;
	}
	g_SPEngine->SetReadExecute(writer.outbase);
	return writer.outbase;
}
