/**
 * vim: set ts=4 :
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

#ifndef _INCLUDE_SOURCEPAWN_JSI_H_
#define _INCLUDE_SOURCEPAWN_JSI_H_

#include <stdio.h>
#include <sp_vm_types.h>
#include "PageAllocator.h"

namespace SourcePawn
{
	enum JitOp
	{
		J_nop,				/* no operation, used for DCE */
		J_next,				/* [ptr] -- special opcode for next page */
		J_prev,				/* [ptr] -- special opcode for previous page */
		J_return,			/* [instr:retval] */
		J_imm,				/* [imm:value] -> value */
		J_load,				/* [instr:base, instr:disp] -> value */
		J_loadi,			/* [instr:base, imm:disp] -> value */
		J_store,			/* [instr:base, instr:disp, instr:value] */
		J_storei,			/* [instr:base, imm:disp, instr:value] */
		J_add,				/* [instr:op1, instr:op2] -> value (op1 + op2) */
		J_stkadd,			/* [imm] -> address */
		J_stkdrop,			/* [instr:val] */
	};

	struct JIns;

	union JInsParam
	{
		uint8_t *ptr;
		JIns *instr;
		int32_t imm;
	};

	struct JIns
	{
		uint8_t op;
		JInsParam param1;
		JInsParam param2;
		JInsParam value;
	};

	class JsiStream
	{
	public:
		JsiStream(const JsiStream & other);
		JsiStream(Page *pFirstPage, JIns *pLast);
	public:
		JIns *GetFirst() const;
		JIns *GetLast() const;
	private:
		Page *m_pFirstPage;
		JIns *m_pLast;
	};

	class JsiBufWriter
	{
	public:
		JsiBufWriter(PageAllocator *allocator);
	public:
		virtual JIns *ins_imm(int32_t value);
		virtual JIns *ins_imm_ptr(void *value);
		virtual JIns *ins_return(JIns *val);
		virtual JIns *ins_loadi(JIns *base, int32_t disp);
		virtual JIns *ins_load(JIns *base, JIns *disp);
		virtual JIns *ins_storei(JIns *base, int32_t disp, JIns *val);
		virtual JIns *ins_store(JIns *base, JIns *disp, JIns *val);
		virtual JIns *ins_add(JIns *op1, JIns *op2);
		virtual JIns *ins_stkadd(int32_t amt);
		virtual void ins_stkdrop(int32_t amt);
	public:
		void kill_pages();
		JsiStream getstream();
	private:
		JIns *ensure_room();
	private:
		JIns *m_pPos;
		Page *m_pCurPage;
		Page *m_pFirstPage;
		PageAllocator *m_pAlloc;
	};

	class JsiReader
	{
	public:
		virtual JIns *next() = 0;
	};

	class JsiForwardReader : public JsiReader
	{
	public:
		JsiForwardReader(const JsiStream & stream);
	public:
		virtual JIns *next();
	private:
		JIns *m_pCur;
		JIns *m_pLast;
	};

	class JsiReverseReader : public JsiReader
	{
	public:
		JsiReverseReader(const JsiStream & stream);
	public:
		virtual JIns *next();
	private:
		JIns *m_pCur;
		JIns *m_pFirst;
	};

	class JsiPrinter
	{
	public:
		JsiPrinter(const JsiStream & stream);
	public:
		void emit_to_file(FILE *fp);
	private:
		JsiForwardReader m_Reader;
	};
}

extern SourcePawn::PageAllocator g_PageAlloc;

#endif //_INCLUDE_SOURCEPAWN_JSI_H_
