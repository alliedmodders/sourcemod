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

#include <sp_vm_types.h>

using namespace SourcePawn
{
	enum JitOp
	{
		J_sysreq,			/* [imm:idx] -> value */
		J_return,			/* [instr:retval] */
		J_imm,				/* [imm:value] -> value */
	};

	#define JSIL_VAL8			(1<<0)
	#define JSIL_VAL16			(1<<1)
	#define JSIL_VAL32			(1<<2)
	#define JSIL_VAL64			(1<<3)
	#define JSIL_IMMVAL			(1<<4)

	typedef uint8_t * JIns;

	class JsiWriter
	{
	public:
		virtual JIns *ins_sysreq(uint32_t index) = 0;
		virtual JIns *ins_imm(int32_t value) = 0;
		virtual JIns *ins_return(JIns *val) = 0;
	};
}

#endif //_INCLUDE_SOURCEPAWN_JSI_H_
