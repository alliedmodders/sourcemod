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
 * Version: $Id: CallMaker.h 1964 2008-03-27 04:54:56Z damagedsoul $
 */

#if defined HOOKING_ENABLED

#include "jit_compile.h"
#include "sm_platform.h"
#include "extension.h"

/********************
 * Assembly Opcodes *
 ********************/

inline void Write_Function_Prologue(JitWriter *jit, bool RetInMemory)
{
	//push ebp
	//push ebx
	//mov ebp, esp
	IA32_Push_Reg(jit, kREG_EBP);
	IA32_Push_Reg(jit, kREG_EBX);
	IA32_Mov_Reg_Rm(jit, kREG_EBP, kREG_ESP, MOD_REG);
#if defined PLATFORM_WINDOWS
	//mov ebx, ecx
	IA32_Mov_Reg_Rm(jit, kREG_EBX, kREG_ECX, MOD_REG);
#elif defined PLATFORM_LINUX || defined PLATFORM_APPLE
	//mov ebx, [ebp+12+(RetInMemory)?4:0]
	IA32_Mov_Reg_Rm_Disp8(jit, kREG_EBX, kREG_EBP, 12+((RetInMemory)?4:0));
#endif
}

inline void Write_Function_Epilogue(JitWriter *jit, unsigned short size)
{
	//mov esp, ebp
	//pop ebx
	//pop ebp
	//ret <value>
	IA32_Mov_Reg_Rm(jit, kREG_ESP, kREG_EBP, MOD_REG);
	IA32_Pop_Reg(jit, kREG_EBX);
	IA32_Pop_Reg(jit, kREG_EBP);
	if (size == 0)
	{
		IA32_Return(jit);
	}
	else
	{
		IA32_Return_Popstack(jit, size);
	}
}

inline void Write_Stack_Alloc(JitWriter *jit, jit_uint32_t size)
{
	//sub esp, <value>
	if (size <= SCHAR_MAX)
	{
		IA32_Sub_Rm_Imm8(jit, kREG_ESP, (jit_int8_t)size, MOD_REG);
	}
	else
	{
		IA32_Sub_Rm_Imm32(jit, kREG_ESP, size, MOD_REG);
	}
}

inline void Write_Copy_Params(JitWriter *jit, bool RetInMemory, jit_uint32_t retsize, jit_uint32_t paramsize)
{
	//:TODO: inline this memcpy!! - For small numbers of params mov's (with clever reg allocation?) would be faster

	//cld
	//push edi
	//push esi
	//lea edi, [ebp-<retsize>-<paramsize>]
	//lea esi, [ebp+12+(RetInMemory)?4:0]
	//if dwords
	// mov ecx, <dwords>
	// rep movsd
	//if bytes
	// mov ecx, <bytes>
	// rep movsb
	//pop esi
	//pop edi

	jit_int32_t offs;
	jit_uint32_t dwords = paramsize >> 2;
	jit_uint32_t bytes = paramsize & 0x3;

	IA32_Cld(jit);
	IA32_Push_Reg(jit, kREG_EDI);
	IA32_Push_Reg(jit, kREG_ESI);
	offs = -(jit_int32_t)retsize - paramsize;

	if (offs > SCHAR_MIN)
	{
		IA32_Lea_DispRegImm8(jit, kREG_EDI, kREG_EBP, (jit_int8_t)offs);
	}
	else
	{
		IA32_Lea_DispRegImm32(jit, kREG_EDI, kREG_EBP, offs);
	}
	offs = 12 + ((RetInMemory) ? sizeof(void *) : 0);

#if defined PLATFORM_LINUX || defined PLATFORM_APPLE
	offs += 4;
#endif

	if (offs < SCHAR_MAX)
	{
		IA32_Lea_DispRegImm8(jit, kREG_ESI, kREG_EBP, (jit_int8_t)offs);
	}
	else
	{
		IA32_Lea_DispRegImm32(jit, kREG_ESI, kREG_EBP, offs);
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
}

inline void Write_Push_Params(JitWriter *jit, 
							  bool isretvoid, 
							  bool isvoid, 
							  jit_uint32_t retsize, 
							  jit_uint32_t paramsize, 
							  HookWrapper *pWrapper)
{
	//and esp, 0xFFFFFFF0
	IA32_And_Rm_Imm8(jit, kREG_ESP, MOD_REG, -16);

	//if retvoid
	// push 0
	//else
	// lea reg, [ebp-<retsize>]
	// push reg
	if (isretvoid)
	{
		IA32_Push_Imm8(jit, 0);
	}
	else
	{
		jit_int32_t offs = -(jit_int32_t)retsize;
		if (offs >= SCHAR_MIN)
		{
			IA32_Lea_DispRegImm8(jit, kREG_EAX, kREG_EBP, (jit_int8_t)offs);
		}
		else
		{
			IA32_Lea_DispRegImm32(jit, kREG_EAX, kREG_EBP, offs);
		}
		IA32_Push_Reg(jit, kREG_EAX);
	}

	//if void
	// push 0
	//else
	// lea reg, [ebp-<retsize>-<paramsize>]
	// push reg
	if (isvoid)
	{
		IA32_Push_Imm8(jit, 0);
	}
	else
	{
		jit_int32_t offs = -(jit_int32_t)retsize - paramsize;
		if (offs > SCHAR_MIN)
		{
			IA32_Lea_DispRegImm8(jit, kREG_EDX, kREG_EBP, (jit_int8_t)offs);
		} else {
			IA32_Lea_DispRegImm32(jit, kREG_EDX, kREG_EBP, offs);
		}
		IA32_Push_Reg(jit, kREG_EDX);
	}

	//push eax (thisptr)
	//IA32_Push_Reg(jit, kREG_ECX);

	//push ebx
	IA32_Push_Reg(jit, kREG_EBX);

	//push <pWrapper>
	IA32_Push_Imm32(jit, (jit_int32_t)(intptr_t)pWrapper);
}

inline void Write_Call_Handler(JitWriter *jit, void *addr)
{
	//call <addr>
	jitoffs_t call = IA32_Call_Imm32(jit, 0);
	IA32_Write_Jump32_Abs(jit, call, addr);
}

inline void Write_Copy_RetVal(JitWriter *jit, SourceHook::PassInfo *pRetInfo)
{
	/* If the return value is a reference the size will probably be >sizeof(void *)
	* for objects, we need to fix that so we can actually return the reference.
	*/
	size_t size = pRetInfo->size;
	if (pRetInfo->flags & PASSFLAG_BYREF)
	{
		size = sizeof(void *);
	}

	if (pRetInfo->type == SourceHook::PassInfo::PassType_Float &&
		pRetInfo->flags & SourceHook::PassInfo::PassFlag_ByVal)
	{
		switch (size)
		{
		case 4:
			{
				//fld DWORD PTR [ebp-4]
				IA32_Fld_Mem32_Disp8(jit, kREG_EBP, -4);
				break;
			}
		case 8:
			{
				//fld QWORD PTR [ebp-8]
				IA32_Fld_Mem64_Disp8(jit, kREG_EBP, -8);
				break;
			}
		}
	}
	else if (pRetInfo->type == SourceHook::PassInfo::PassType_Object &&
		pRetInfo->flags & SourceHook::PassInfo::PassFlag_ByVal)
	{
		//cld
		//push edi
		//push esi
		//mov edi, [ebp+12]
		//lea esi, [ebp-<retsize>]
		//if dwords
		// mov ecx, <dwords>
		// rep movsd
		//if bytes
		// mov ecx, <bytes>
		// rep movsb
		//pop esi
		//pop edi

		jit_int32_t offs;
		jit_uint32_t dwords = size >> 2;
		jit_uint32_t bytes = size & 0x3;

		IA32_Cld(jit);
		IA32_Push_Reg(jit, kREG_EDI);
		IA32_Push_Reg(jit, kREG_ESI);

		IA32_Mov_Reg_Rm_Disp8(jit, kREG_EDI, kREG_EBP, 12);

		offs = -(jit_int32_t)pRetInfo->size;
		if (offs >= SCHAR_MIN)
		{
			IA32_Lea_DispRegImm8(jit, kREG_ESI, kREG_EBP, (jit_int8_t)offs);
		}
		else
		{
			IA32_Lea_DispRegImm32(jit, kREG_ESI, kREG_EBP, offs);
		}
		IA32_Mov_Reg_Rm(jit, kREG_EAX, kREG_EDI, MOD_REG);

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
	}
	else
	{
		switch (size)
		{
		case 1:
			{
				//mov al, BYTE PTR [ebp-4]
				IA32_Mov_Reg8_Rm8_Disp8(jit, kREG_EAX, kREG_EBP, -4);
				break;
			}
		case 2:
			{
				//mov ax, WORD PTR [ebp-4]
				jit->write_ubyte(IA32_16BIT_PREFIX);
				IA32_Mov_Reg_Rm_Disp8(jit, kREG_EAX, kREG_EBP, -4);
				break;
			}
		case 4:
			{
				//mov eax, DWORD PTR [ebp-4]
				IA32_Mov_Reg_Rm_Disp8(jit, kREG_EAX, kREG_EBP, -4);
				break;
			}
		case 8:
			{
				//mov eax, DWORD PTR [ebp-8]
				//mov edx, DWORD PTR [ebp-4]
				//:TODO: this is broken due to SH
				IA32_Mov_Reg_Rm_Disp8(jit, kREG_EAX, kREG_EBP, -8);
				IA32_Mov_Reg_Rm_Disp8(jit, kREG_EDX, kREG_EBP, -4);
				break;
			}
		}
	}
}

/******************************
 * Assembly Compiler Function *
 ******************************/

void *JIT_HookCompile(HookWrapper *pWrapper)
{
	JitWriter writer;
	JitWriter *jit = &writer;

	jit_uint32_t CodeSize = 0;
	jit_uint32_t ParamSize = pWrapper->GetParamSize();
	jit_uint32_t RetSize = pWrapper->GetRetSize();

	/* Local variable size allocated in the stack for the param and retval buffers */
	jit_uint32_t LocalVarSize = ParamSize + RetSize;

	/* Check if the return value is returned in memory */
	bool RetInMemory = false;
	SourceHook::PassInfo *pRetInfo = &pWrapper->GetProtoInfo()->retPassInfo;
	if ((pRetInfo->type == SourceHook::PassInfo::PassType_Object) && 
		(pRetInfo->flags & SourceHook::PassInfo::PassFlag_ByVal))
	{
		RetInMemory = true;
	}

	writer.outbase = NULL;
	writer.outptr = NULL;

jit_rewind:
	/* Write the function prologue */
	Write_Function_Prologue(jit, RetInMemory);

	/* Allocate the local variables into the stack */
	if (LocalVarSize)
	{
		Write_Stack_Alloc(jit, LocalVarSize);
	}

	/* Copy all the parameters into the buffer */
	if (ParamSize)
	{
		Write_Copy_Params(jit, RetInMemory, RetSize, ParamSize);
	}

	/* Push the parameters into the handler */
	Write_Push_Params(jit, !RetSize, !ParamSize, RetSize, ParamSize, pWrapper);

	/* Call the handler function */
	Write_Call_Handler(jit, pWrapper->GetHandlerAddr());

	/* Copy back the return value into eax or the hidden return buffer */
	if (RetSize)
	{
		Write_Copy_RetVal(jit, pRetInfo);
	}

	/* Write the function epilogue */
	Write_Function_Epilogue(jit, 
#if defined PLATFORM_WINDOWS
		ParamSize + ((RetInMemory) ? sizeof(void *) : 0)
#elif defined PLATFORM_LINUX || defined PLATFORM_APPLE
		(RetInMemory) ? sizeof(void *) : 0
#endif
	);

	if (writer.outbase == NULL)
	{
		CodeSize = writer.get_outputpos();
		writer.outbase = (jitcode_t)g_SPEngine->AllocatePageMemory(CodeSize);
		g_SPEngine->SetReadWrite(writer.outbase);
		writer.outptr = writer.outbase;
		g_RegDecoder = 0;
		goto jit_rewind;
	}
	g_SPEngine->SetReadExecute(writer.outbase);

	return writer.outbase;
}

void JIT_FreeHook(void *addr)
{
	g_SPEngine->FreePageMemory(addr);
}

#endif
