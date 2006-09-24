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
		case OP_LODB_I:
			{
				WriteOp_Lodb_I(jit);
				break;
			}
		case OP_STRB_I:
			{
				WriteOp_Strb_I(jit);
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