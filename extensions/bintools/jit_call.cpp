/**
 * vim: set ts=4 :
 * ===============================================================
 * SourceMod BinTools Extension
 * Copyright (C) 2004-2007 AlliedModders LLC. All rights reserved.
 * ===============================================================
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Version: $Id$
 */

#include <sm_platform.h>
#include "extension.h"
#include <jit_helpers.h>
#include <x86_macros.h>
#include "jit_call.h"

jit_uint32_t g_StackUsage = 0;
jit_uint32_t g_RegDecoder = 0;

/********************
 * Assembly Helpers *
 ********************/

inline jit_uint8_t _DecodeRegister3(jit_uint32_t val)
{
	switch (val % 3)
	{
	case 0:
		{
			return REG_EAX;
		}
	case 1:
		{
			return REG_EDX;
		}
	case 2:
		{
			return REG_ECX;
		}
	}

	/* Should never happen */
	return 0xFF;
}

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
	IA32_Push_Reg(jit, REG_EBP);
	IA32_Mov_Reg_Rm(jit, REG_EBP, REG_ESP, MOD_REG);
	if (!is_void)
	{
		IA32_Push_Reg(jit, REG_EDI);
		IA32_Mov_Reg_Rm_Disp8(jit, REG_EDI, REG_EBP, 12);
	}
	if (has_params)
	{
		IA32_Push_Reg(jit, REG_EBX);
		IA32_Mov_Reg_Rm_Disp8(jit, REG_EBX, REG_EBP, 8);
	}
}

inline void Write_Function_Epilogue(JitWriter *jit, bool is_void, bool has_params)
{
	//if has_params
	// pop ebx
	//if !is_void
	// pop edi
	//mov esp, ebp
	//pop ebp
	//ret
	if (has_params)
	{
		IA32_Pop_Reg(jit, REG_EBX);
	}
	if (!is_void)
	{
		IA32_Pop_Reg(jit, REG_EDI);
	}
	IA32_Mov_Reg_Rm(jit, REG_ESP, REG_EBP, MOD_REG);
	IA32_Pop_Reg(jit, REG_EBP);
	IA32_Return(jit);
}

inline void Write_PushPOD(JitWriter *jit, const PassEncode *pEnc)
{
	jit_uint8_t reg = _DecodeRegister3(g_RegDecoder++);

	if (pEnc->info.flags & PASSFLAG_BYVAL)
	{
		switch (pEnc->info.size)
		{
		case 1:
			{
				//xor reg, reg
				//mov reg, BYTE PTR [ebx+<offset>]
				//push reg
				IA32_Xor_Reg_Rm(jit, reg, reg, MOD_REG);
				if (pEnc->offset < SCHAR_MAX)
				{
					IA32_Mov_Reg8_Rm8_Disp8(jit, reg, REG_EBX, (jit_int8_t)pEnc->offset);
				} else if (!pEnc->offset) {
					IA32_Mov_Reg8_Rm8(jit, reg, REG_EBX, MOD_MEM_REG);
				} else {
					IA32_Mov_Reg8_Rm8_Disp32(jit, reg, REG_EBX, pEnc->offset);
				}
				IA32_Push_Reg(jit, reg);

				g_StackUsage += 4;
				break;
			}
		case 2:
			{
				//xor reg, reg
				//mov reg, WORD PTR [ebx+<offset>]
				//push reg
				IA32_Xor_Reg_Rm(jit, reg, reg, MOD_REG);
				jit->write_ubyte(IA32_16BIT_PREFIX);
				if (pEnc->offset < SCHAR_MAX)
				{
					IA32_Mov_Reg_Rm_Disp8(jit, reg, REG_EBX, (jit_int8_t)pEnc->offset);
				} else if (!pEnc->offset) {
					IA32_Mov_Reg_Rm(jit, reg, REG_EBX, MOD_MEM_REG);
				} else {
					IA32_Mov_Reg_Rm_Disp32(jit, reg, REG_EBX, pEnc->offset);
				}
				IA32_Push_Reg(jit, reg);

				g_StackUsage += 4;
				break;
			}
		case 4:
			{
				//mov reg, DWORD PTR [ebx+<offset>]
				//push reg
				if (pEnc->offset < SCHAR_MAX)
				{
					IA32_Mov_Reg_Rm_Disp8(jit, reg, REG_EBX, (jit_int8_t)pEnc->offset);
				} else if (!pEnc->offset) {
					IA32_Mov_Reg_Rm(jit, reg, REG_EBX, MOD_MEM_REG);
				} else {
					IA32_Mov_Reg_Rm_Disp32(jit, reg, REG_EBX, pEnc->offset);
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

				if (pEnc->offset+4 < SCHAR_MAX)
				{
					IA32_Mov_Reg_Rm_Disp8(jit, reg, REG_EBX, (jit_int8_t)(pEnc->offset+4));
				} else {
					IA32_Mov_Reg_Rm_Disp32(jit, reg, REG_EBX, pEnc->offset+4);
				}
				if (pEnc->offset < SCHAR_MAX)
				{
					IA32_Mov_Reg_Rm_Disp8(jit, reg2, REG_EBX, (jit_int8_t)pEnc->offset);
				} else if (!pEnc->offset) {
					IA32_Mov_Reg_Rm(jit, reg, REG_EBX, MOD_MEM_REG);
				} else {
					IA32_Mov_Reg_Rm_Disp32(jit, reg2, REG_EBX, pEnc->offset);
				}
				IA32_Push_Reg(jit, reg);
				IA32_Push_Reg(jit, reg2);

				g_StackUsage += 8;
				break;
			}
		}
	} else if (pEnc->info.flags & PASSFLAG_BYREF) {
		//lea reg, [ebx+<offset>]
		//push reg
		if (!pEnc->offset)
		{
			IA32_Push_Reg(jit, REG_EBX);
			g_StackUsage += 4;
			return;
		}
		if (pEnc->offset < SCHAR_MAX)
		{
			IA32_Lea_DispRegImm8(jit, reg, REG_EBX, (jit_int8_t)pEnc->offset);
		} else {
			IA32_Lea_DispRegImm32(jit, reg, REG_EBX, pEnc->offset);
		}
		IA32_Push_Reg(jit, reg);

		g_StackUsage += 4;
	}
}

inline void Write_PushFloat(JitWriter *jit, const PassEncode *pEnc)
{
	if (pEnc->info.flags & PASSFLAG_BYVAL)
	{
		switch (pEnc->info.size)
		{
		case 4:
			{
				//fld DWORD PTR [ebx+<offset>]
				//push reg
				//fstp DWORD PTR [esp]
				if (pEnc->offset < SCHAR_MAX)
				{
					IA32_Fld_Mem32_Disp8(jit, REG_EBX, (jit_int8_t)pEnc->offset);
				} else {
					IA32_Fld_Mem32_Disp32(jit, REG_EBX, pEnc->offset);
				}
				IA32_Push_Reg(jit, _DecodeRegister3(g_RegDecoder++));
				IA32_Fstp_Mem32(jit, REG_ESP, REG_NOIDX, NOSCALE);
				g_StackUsage += 4;
				break;
			}
		case 8:
			{
				//fld QWORD PTR [ebx+<offset>]
				//sub esp, 8
				//fstp QWORD PTR [esp]
				if (pEnc->offset < SCHAR_MAX)
				{
					IA32_Fld_Mem64_Disp8(jit, REG_EBX, (jit_int8_t)pEnc->offset);
				} else {
					IA32_Fld_Mem64_Disp32(jit, REG_EBX, pEnc->offset);
				}
				IA32_Sub_Rm_Imm8(jit, REG_ESP, 8, MOD_REG);
				IA32_Fstp_Mem64(jit, REG_ESP, REG_NOIDX, NOSCALE);
				g_StackUsage += 8;
				break;
			}
		}
	} else if (pEnc->info.flags & PASSFLAG_BYREF) {
		//lea reg, [ebx+<offset>]
		//push reg
		if (!pEnc->offset)
		{
			IA32_Push_Reg(jit, REG_EBX);
			g_StackUsage += 4;
			return;
		}

		jit_uint8_t reg = _DecodeRegister3(g_RegDecoder++);
		if (pEnc->offset < SCHAR_MAX)
		{
			IA32_Lea_DispRegImm8(jit, reg, REG_EBX, (jit_int8_t)pEnc->offset);
		} else {
			IA32_Lea_DispRegImm32(jit, reg, REG_EBX, pEnc->offset);
		}
		IA32_Push_Reg(jit, reg);

		g_StackUsage += 4;
	}
}

inline void Write_PushObject(JitWriter *jit, const PassEncode *pEnc)
{
	if (pEnc->info.flags & PASSFLAG_BYVAL)
	{
#ifdef PLATFORM_POSIX
		if (pEnc->info.flags & PASSFLAG_ODTOR)
		{
			goto push_byref;
		}
#endif
		jit_uint32_t dwords = pEnc->info.size >> 2;
		jit_uint32_t bytes = pEnc->info.size & 0x3;

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
		if (pEnc->info.size < SCHAR_MAX)
		{
			IA32_Sub_Rm_Imm8(jit, REG_ESP, (jit_int8_t)pEnc->info.size, MOD_REG);
		} else {
			IA32_Sub_Rm_Imm32(jit, REG_ESP, pEnc->info.size, MOD_REG);
		}
		IA32_Cld(jit);
		IA32_Push_Reg(jit, REG_EDI);
		IA32_Push_Reg(jit, REG_ESI);
		IA32_Lea_Reg_DispRegMultImm8(jit, REG_EDI, REG_NOIDX, REG_ESP, NOSCALE, 8);
		if (pEnc->offset < SCHAR_MAX)
		{
			IA32_Lea_DispRegImm8(jit, REG_ESI, REG_EBX, (jit_int8_t)pEnc->offset);
		} else if (!pEnc->offset) {
			IA32_Mov_Reg_Rm(jit, REG_ESI, REG_EBX, MOD_REG);
		} else {
			IA32_Lea_DispRegImm32(jit, REG_ESI, REG_EBX, pEnc->offset);
		}
		if (dwords)
		{
			IA32_Mov_Reg_Imm32(jit, REG_ECX, dwords);
			IA32_Rep(jit);
			IA32_Movsd(jit);
		}
		if (bytes)
		{
			IA32_Mov_Reg_Imm32(jit, REG_ECX, bytes);
			IA32_Rep(jit);
			IA32_Movsb(jit);
		}
		IA32_Pop_Reg(jit, REG_ESI);
		IA32_Pop_Reg(jit, REG_EDI);

		g_StackUsage += pEnc->info.size;
	} else if (pEnc->info.flags & PASSFLAG_BYREF) {
#ifdef PLATFORM_POSIX
push_byref:
#endif
		if (!pEnc->offset)
		{
			IA32_Push_Reg(jit, REG_EBX);
			g_StackUsage += 4;
			return;
		}

		//lea reg, [ebx+<offset>]
		//push reg
		jit_uint8_t reg = _DecodeRegister3(g_RegDecoder++);
		if (pEnc->offset < SCHAR_MAX)
		{
			IA32_Lea_DispRegImm8(jit, reg, REG_EBX, (jit_int8_t)pEnc->offset);
		} else {
			IA32_Lea_DispRegImm32(jit, reg, REG_EBX, pEnc->offset);
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

	IA32_Mov_Reg_Rm(jit, reg, REG_EBX, MOD_MEM_REG);
	IA32_Push_Reg(jit, reg);

	g_StackUsage += 4;
#elif defined PLATFORM_WINDOWS
	//mov ecx, [ebx]
	IA32_Mov_Reg_Rm(jit, REG_ECX, REG_EBX, MOD_MEM_REG);
#endif
}

inline void Write_PushRetBuffer(JitWriter *jit)
{
	//push edi
	IA32_Push_Reg(jit, REG_EDI);
}

inline void Write_CallFunction(JitWriter *jit, FuncAddrMethod method, CallWrapper *pWrapper)
{
	if (method == FuncAddr_Direct)
	{
		//call <addr>
		jitoffs_t call = IA32_Call_Imm32(jit, 0);
		IA32_Write_Jump32_Abs(jit, call, pWrapper->m_Addrs[ADDR_CALLEE]);
	} else if (method == FuncAddr_VTable) {
		//*(this + thisOffs + vtblOffs)[vtblIdx]
		//mov edx, [ebx]
		//mov eax, [edx+<thisOffs>+<vtblOffs>]
		//mov edx, [eax+<vtblIdx>*4]
		//call edx
		jit_uint32_t total_offs = pWrapper->m_VtInfo.thisOffs + pWrapper->m_VtInfo.vtblOffs;
		jit_uint32_t vfunc_pos = pWrapper->m_VtInfo.vtblIdx * 4;

		IA32_Mov_Reg_Rm(jit, REG_EDX, REG_EBX, MOD_MEM_REG);
		if (total_offs < SCHAR_MAX)
		{
			IA32_Mov_Reg_Rm_Disp8(jit, REG_EAX, REG_EDX, (jit_int8_t)total_offs);
		} else if (!total_offs) {
			IA32_Mov_Reg_Rm(jit, REG_EAX, REG_EDX, MOD_MEM_REG);
		} else {
			IA32_Mov_Reg_Rm_Disp32(jit, REG_EAX, REG_EDX, total_offs);
		}
		if (vfunc_pos < SCHAR_MAX)
		{
			IA32_Mov_Reg_Rm_Disp8(jit, REG_EDX, REG_EAX, (jit_int8_t)vfunc_pos);
		} else if (!vfunc_pos) {
			IA32_Mov_Reg_Rm(jit, REG_EDX, REG_EAX, MOD_MEM_REG);
		} else {
			IA32_Mov_Reg_Rm_Disp32(jit, REG_EDX, REG_EAX, vfunc_pos);
		}
		IA32_Call_Reg(jit, REG_EDX);
	}
}

inline void Write_RectifyStack(JitWriter *jit, jit_uint32_t value)
{
	//add esp, <value>
	if (value < SCHAR_MAX)
	{
		IA32_Add_Rm_Imm8(jit, REG_ESP, (jit_int8_t)value, MOD_REG);
	} else {
		IA32_Add_Rm_Imm32(jit, REG_ESP, value, MOD_REG);
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
				IA32_Fstp_Mem32(jit, REG_EDI, REG_NOIDX, NOSCALE);
				break;
			}
		case 8:
			{
				//fstp QWORD PTR [edi]
				IA32_Fstp_Mem64(jit, REG_EDI, REG_NOIDX, NOSCALE);
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
			IA32_Mov_Rm8_Reg8(jit, REG_EDI, REG_EAX, MOD_MEM_REG);
			break;
		}
	case 2:
		{
			//mov WORD PTR [edi], ax
			jit->write_ubyte(IA32_16BIT_PREFIX);
			IA32_Mov_Rm_Reg(jit, REG_EDI, REG_EAX, MOD_MEM_REG);
			break;
		}
	case 4:
		{
			//mov DWORD PTR [edi], eax
			IA32_Mov_Rm_Reg(jit, REG_EDI, REG_EAX, MOD_MEM_REG);
			break;
		}
	case 8:
		{
			//mov DWORD PTR [edi], eax
			//mov DWORD PTR [edi+4], edx
			IA32_Mov_Rm_Reg(jit, REG_EDI, REG_EAX, MOD_MEM_REG);
			IA32_Mov_Rm_Reg_Disp8(jit, REG_EDI, REG_EDX, 4);
			break;
		}
	}
}

/******************************
 * Assembly Compiler Function *
 ******************************/

void JIT_Compile(CallWrapper *pWrapper, FuncAddrMethod method)
{
	JitWriter writer;
	JitWriter *jit = &writer;

	jit_uint32_t CodeSize = 0;
	bool Needs_Retbuf = false;
	CallConvention Convention = pWrapper->GetCallConvention();
	jit_uint32_t ParamCount = pWrapper->GetParamCount();
	const PassInfo *pRet = pWrapper->GetReturnInfo();
	bool hasParams = (ParamCount || Convention == CallConv_ThisCall);

	writer.outbase = NULL;
	writer.outptr = NULL;

jit_rewind:
	/* Write function prologue */
	Write_Execution_Prologue(jit, (pRet) ? false : true, hasParams);

	/* Write parameter push code */
	for (jit_int32_t i=ParamCount-1; i>=0; i--)
	{
		const PassEncode *pEnc = pWrapper->GetParamInfo(i);
		switch (pEnc->info.type)
		{
		case PassType_Basic:
			{
				Write_PushPOD(jit, pEnc);
				break;
			}
		case PassType_Float:
			{
				Write_PushFloat(jit, pEnc);
				break;
			}
		case PassType_Object:
			{
				Write_PushObject(jit, pEnc);
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
#ifdef PLATFORM_POSIX
		Needs_Retbuf = true;
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
	if (ParamCount)
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
		writer.outbase = (jitcode_t)g_SPEngine->ExecAlloc(CodeSize);
		writer.outptr = writer.outbase;
		pWrapper->m_Addrs[ADDR_CODEBASE] = writer.outbase;
		g_StackUsage = 0;
		g_RegDecoder = 0;
		Needs_Retbuf = false;
		goto jit_rewind;
	}
}
