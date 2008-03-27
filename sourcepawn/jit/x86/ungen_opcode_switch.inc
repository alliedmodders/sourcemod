/**
 * vim: set ts=4 :
 * =============================================================================
 * SourcePawn JIT
 * Copyright (C)2004-2008 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This file is not open source and may not be copied without explicit wriiten
 * permission of AlliedModders LLC.  This file may not be redistributed in whole
 * or significant part.
 * For information, see LICENSE.txt or http://www.sourcemod.net/license.php
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