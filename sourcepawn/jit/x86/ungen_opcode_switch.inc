/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod
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

		case OP_UMUL:
			{
				WriteOp_UMul(jit);
				break;
			}
		case OP_LESS:
			{
				WriteOp_Less(jit);
				break;
			}
		case OP_LEQ:
			{
				WriteOp_Leq(jit);
				break;
			}
		case OP_GRTR:
			{
				WriteOp_Grtr(jit);
				break;
			}
		case OP_GEQ:
			{
				WriteOp_Geq(jit);
				break;
			}
		case OP_ALIGN_PRI:
			{
				WriteOp_Align_Pri(jit);
				break;
			}
		case OP_ALIGN_ALT:
			{
				WriteOp_Align_Alt(jit);
				break;
			}
		case OP_LCTRL:
			{
				WriteOp_Lctrl(jit);
				break;
			}
		case OP_SCTRL:
			{
				WriteOp_Sctrl(jit);
				break;
			}
		case OP_UDIV:
			{
				WriteOp_UDiv(jit);
				break;
			}
		case OP_UDIV_ALT:
			{
				WriteOp_UDiv_Alt(jit);
				break;
			}
		case OP_RET:
			{
				WriteOp_Ret(jit);
				break;
			}
		case OP_CMPS:
			{
				WriteOp_Cmps(jit);
				break;
			}
		case OP_JREL:
			{
				WriteOp_JRel(jit);
				break;
			}
		case OP_JLESS:
			{
				WriteOp_Jless(jit);
				break;
			}
		case OP_JLEQ:
			{
				WriteOp_Jeq(jit);
				break;
			}
		case OP_JGRTR:
			{
				WriteOp_Jgrtr(jit);
				break;
			}
		case OP_JGEQ:
			{
				WriteOp_Jgeq(jit);
				break;
			}