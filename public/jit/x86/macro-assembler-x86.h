/**
 * vim: set ts=8 sts=2 sw=2 tw=99 et:
 * =============================================================================
 * SourcePawn JIT SDK
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
#ifndef _include_sourcepawn_macroassembler_x86h__
#define _include_sourcepawn_macroassembler_x86h__

#include <assembler.h>
#include <am-vector.h>
#include <string.h>
#include <assembler-x86.h>

class MacroAssemblerX86 : public AssemblerX86
{
  public:
    static void GenerateFeatureDetection(MacroAssemblerX86 &masm) {
      masm.push(ebp);
      masm.movl(ebp, esp);
      masm.push(ebx);
      {
        // Get ECX, EDX feature bits at the first CPUID level.
        masm.movl(eax, 1);
        masm.cpuid();
        masm.movl(eax, Operand(ebp, 8));
        masm.movl(Operand(eax, 0), ecx);
        masm.movl(eax, Operand(ebp, 12));
        masm.movl(Operand(eax, 0), edx);
      }

      // Zero out bits we're not guaranteed to get.
      masm.movl(eax, Operand(ebp, 16));
      masm.movl(Operand(eax, 0), 0);

      Label skip_level_7;
      {
        // Get EBX feature bits at 7th CPUID level.
        masm.movl(eax, 0);
        masm.cpuid();
        masm.cmpl(eax, 7);
        masm.j(below, &skip_level_7);
        masm.movl(eax, 7);
        masm.movl(ecx, 0);
        masm.cpuid();
        masm.movl(eax, Operand(ebp, 16));
        masm.movl(Operand(eax, 0), ebx);
      }
      masm.bind(&skip_level_7);

      masm.pop(ebx);
      masm.pop(ebp);
      masm.ret();
    }

    static void RunFeatureDetection(void *code) {
      typedef void (*fn_t)(int *reg_ecx, int *reg_edx, int *reg_ebx);

      int reg_ecx, reg_edx, reg_ebx;
      ((fn_t)code)(&reg_ecx, &reg_edx, &reg_ebx);
      
      CPUFeatures features;
      features.fpu = !!(reg_edx & (1 << 0));
      features.mmx = !!(reg_edx & (1 << 23));
      features.sse = !!(reg_edx & (1 << 25));
      features.sse2 = !!(reg_edx & (1 << 26));
      features.sse3 = !!(reg_ecx & (1 << 0));
      features.ssse3 = !!(reg_ecx & (1 << 9));
      features.sse4_1 = !!(reg_ecx & (1 << 19));
      features.sse4_2 = !!(reg_ecx & (1 << 20));
      features.avx = !!(reg_ecx & (1 << 28));
      features.avx2 = !!(reg_ebx & (1 << 5));
      SetFeatures(features);
    }

  private:
};

#endif // _include_sourcepawn_macroassembler_x86h__

