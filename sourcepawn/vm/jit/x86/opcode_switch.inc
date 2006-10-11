		case OP_MOVE_PRI:
			{
				WriteOp_Move_Pri(jit);
				break;
			}
		case OP_MOVE_ALT:
			{
				WriteOp_Move_Alt(jit);
				break;
			}
		case OP_XCHG:
			{
				WriteOp_Xchg(jit);
				break;
			}
		case OP_PUSH:
			{
				WriteOp_Push(jit);
				break;
			}
		case OP_PUSH_S:
			{
				WriteOp_Push_S(jit);
				break;
			}
		case OP_PUSH2_C:
			{
				WriteOp_Push2_C(jit);
				break;
			}
		case OP_PUSH3_C:
			{
				WriteOp_Push3_C(jit);
				break;
			}
		case OP_PUSH4_C:
			{
				WriteOp_Push4_C(jit);
				break;
			}
		case OP_PUSH5_C:
			{
				WriteOp_Push5_C(jit);
				break;
			}
		case OP_PUSH2_ADR:
			{
				WriteOp_Push2_Adr(jit);
				break;
			}
		case OP_PUSH3_ADR:
			{
				WriteOp_Push3_Adr(jit);
				break;
			}
		case OP_PUSH4_ADR:
			{
				WriteOp_Push4_Adr(jit);
				break;
			}
		case OP_PUSH5_ADR:
			{
				WriteOp_Push5_Adr(jit);
				break;
			}
		case OP_PUSH2_S:
			{
				WriteOp_Push2_S(jit);
				break;
			}
		case OP_PUSH3_S:
			{
				WriteOp_Push3_S(jit);
				break;
			}
		case OP_PUSH4_S:
			{
				WriteOp_Push4_S(jit);
				break;
			}
		case OP_PUSH5_S:
			{
				WriteOp_Push5_S(jit);
				break;
			}
		case OP_PUSH5:
			{
				WriteOp_Push5(jit);
				break;
			}
		case OP_PUSH4:
			{
				WriteOp_Push4(jit);
				break;
			}
		case OP_PUSH3:
			{
				WriteOp_Push3(jit);
				break;
			}
		case OP_PUSH2:
			{
				WriteOp_Push2(jit);
				break;
			}
		case OP_ZERO_PRI:
			{
				WriteOp_Zero_Pri(jit);
				break;
			}
		case OP_ZERO_ALT:
			{
				WriteOp_Zero_Alt(jit);
				break;
			}
		case OP_PROC:
			{
				WriteOp_Proc(jit);
				break;
			}
		case OP_SHL:
			{
				WriteOp_Shl(jit);
				break;
			}
		case OP_SHR:
			{
				WriteOp_Shr(jit);
				break;
			}
		case OP_SSHR:
			{
				WriteOp_Sshr(jit);
				break;
			}
		case OP_SHL_C_PRI:
			{
				WriteOp_Shl_C_Pri(jit);
				break;
			}
		case OP_SHL_C_ALT:
			{
				WriteOp_Shl_C_Alt(jit);
				break;
			}
		case OP_SHR_C_PRI:
			{
				WriteOp_Shr_C_Pri(jit);
				break;
			}
		case OP_SHR_C_ALT:
			{
				WriteOp_Shr_C_Alt(jit);
				break;
			}
		case OP_SMUL:
			{
				WriteOp_SMul(jit);
				break;
			}
		case OP_ADD:
			{
				WriteOp_Add(jit);
				break;
			}
		case OP_SUB:
			{
				WriteOp_Sub(jit);
				break;
			}
		case OP_SUB_ALT:
			{
				WriteOp_Sub_Alt(jit);
				break;
			}
		case OP_NOP:
			{
				/* do nothing */
				break;
			}
		case OP_NOT:
			{
				WriteOp_Not(jit);
				break;
			}
		case OP_NEG:
			{
				WriteOp_Neg(jit);
				break;
			}
		case OP_XOR:
			{
				WriteOp_Xor(jit);
				break;
			}
		case OP_OR:
			{
				WriteOp_Or(jit);
				break;
			}
		case OP_AND:
			{
				WriteOp_And(jit);
				break;
			}
		case OP_INVERT:
			{
				WriteOp_Invert(jit);
				break;
			}
		case OP_ADD_C:
			{
				WriteOp_Add_C(jit);
				break;
			}
		case OP_SMUL_C:
			{
				WriteOp_SMul_C(jit);
				break;
			}
		case OP_SIGN_PRI:
			{
				WriteOp_Sign_Pri(jit);
				break;
			}
		case OP_SIGN_ALT:
			{
				WriteOp_Sign_Alt(jit);
				break;
			}
		case OP_EQ:
			{
				WriteOp_Eq(jit);
				break;
			}
		case OP_NEQ:
			{
				WriteOp_Neq(jit);
				break;
			}
		case OP_SLESS:
			{
				WriteOp_Sless(jit);
				break;
			}
		case OP_SLEQ:
			{
				WriteOp_Sleq(jit);
				break;
			}
		case OP_SGRTR:
			{
				WriteOp_Sgrtr(jit);
				break;
			}
		case OP_SGEQ:
			{
				WriteOp_Sgeq(jit);
				break;
			}
		case OP_EQ_C_PRI:
			{
				WriteOp_Eq_C_Pri(jit);
				break;
			}
		case OP_EQ_C_ALT:
			{
				WriteOp_Eq_C_Alt(jit);
				break;
			}
		case OP_INC_PRI:
			{
				WriteOp_Inc_Pri(jit);
				break;
			}
		case OP_INC_ALT:
			{
				WriteOp_Inc_Alt(jit);
				break;
			}
		case OP_INC:
			{
				WriteOp_Inc(jit);
				break;
			}
		case OP_INC_S:
			{
				WriteOp_Inc_S(jit);
				break;
			}
		case OP_INC_I:
			{
				WriteOp_Inc_I(jit);
				break;
			}
		case OP_DEC_PRI:
			{
				WriteOp_Dec_Pri(jit);
				break;
			}
		case OP_DEC_ALT:
			{
				WriteOp_Dec_Alt(jit);
				break;
			}
		case OP_DEC:
			{
				WriteOp_Dec(jit);
				break;
			}
		case OP_DEC_S:
			{
				WriteOp_Dec_S(jit);
				break;
			}
		case OP_DEC_I:
			{
				WriteOp_Dec_I(jit);
				break;
			}
		case OP_LOAD_PRI:
			{
				WriteOp_Load_Pri(jit);
				break;
			}
		case OP_LOAD_ALT:
			{
				WriteOp_Load_Alt(jit);
				break;
			}
		case OP_LOAD_S_PRI:
			{
				WriteOp_Load_S_Pri(jit);
				break;
			}
		case OP_LOAD_S_ALT:
			{
				WriteOp_Load_S_Alt(jit);
				break;
			}
		case OP_LREF_PRI:
			{
				WriteOp_Lref_Pri(jit);
				break;
			}
		case OP_LREF_ALT:
			{
				WriteOp_Lref_Alt(jit);
				break;
			}
		case OP_LREF_S_PRI:
			{
				WriteOp_Lref_S_Pri(jit);
				break;
			}
		case OP_LREF_S_ALT:
			{
				WriteOp_Lref_S_Alt(jit);
				break;
			}
		case OP_CONST_PRI:
			{
				WriteOp_Const_Pri(jit);
				break;
			}
		case OP_CONST_ALT:
			{
				WriteOp_Const_Alt(jit);
				break;
			}
		case OP_ADDR_PRI:
			{
				WriteOp_Addr_Pri(jit);
				break;
			}
		case OP_ADDR_ALT:
			{
				WriteOp_Addr_Alt(jit);
				break;
			}
		case OP_STOR_PRI:
			{
				WriteOp_Stor_Pri(jit);
				break;
			}
		case OP_STOR_ALT:
			{
				WriteOp_Stor_Alt(jit);
				break;
			}
		case OP_STOR_S_PRI:
			{
				WriteOp_Stor_S_Pri(jit);
				break;
			}
		case OP_STOR_S_ALT:
			{
				WriteOp_Stor_S_Alt(jit);
				break;
			}
		case OP_IDXADDR:
			{
				WriteOp_Idxaddr(jit);
				break;
			}
		case OP_SREF_PRI:
			{
				WriteOp_Sref_Pri(jit);
				break;
			}
		case OP_SREF_ALT:
			{
				WriteOp_Sref_Alt(jit);
				break;
			}
		case OP_SREF_S_PRI:
			{
				WriteOp_Sref_S_Pri(jit);
				break;
			}
		case OP_SREF_S_ALT:
			{
				WriteOp_Sref_S_Alt(jit);
				break;
			}
		case OP_POP_PRI:
			{
				WriteOp_Pop_Pri(jit);
				break;
			}
		case OP_POP_ALT:
			{
				WriteOp_Pop_Alt(jit);
				break;
			}
		case OP_SWAP_PRI:
			{
				WriteOp_Swap_Pri(jit);
				break;
			}
		case OP_SWAP_ALT:
			{
				WriteOp_Swap_Alt(jit);
				break;
			}
		case OP_PUSH_ADR:
			{
				WriteOp_PushAddr(jit);
				break;
			}
		case OP_MOVS:
			{
				WriteOp_Movs(jit);
				break;
			}
		case OP_FILL:
			{
				WriteOp_Fill(jit);
				break;
			}
		case OP_HEAP_PRI:
			{
				WriteOp_Heap_Pri(jit);
				break;
			}
		case OP_PUSH_HEAP_C:
			{
				WriteOp_Push_Heap_C(jit);
				break;
			}
		case OP_POP_HEAP_PRI:
			{
				WriteOp_Pop_Heap_Pri(jit);
				break;
			}
		case OP_PUSH_C:
			{
				WriteOp_Push_C(jit);
				break;
			}
		case OP_ZERO:
			{
				WriteOp_Zero(jit);
				break;
			}
		case OP_ZERO_S:
			{
				WriteOp_Zero_S(jit);
				break;
			}
		case OP_PUSH_PRI:
			{
				WriteOp_Push_Pri(jit);
				break;
			}
		case OP_PUSH_ALT:
			{
				WriteOp_Push_Alt(jit);
				break;
			}
		case OP_LOAD_BOTH:
			{
				WriteOp_Load_Both(jit);
				break;
			}
		case OP_LOAD_S_BOTH:
			{
				WriteOp_Load_S_Both(jit);
				break;
			}
		case OP_CONST:
			{
				WriteOp_Const(jit);
				break;
			}
		case OP_CONST_S:
			{
				WriteOp_Const_S(jit);
				break;
			}
		case OP_LOAD_I:
			{
				WriteOp_Load_I(jit);
				break;
			}
		case OP_STOR_I:
			{
				WriteOp_Stor_I(jit);
				break;
			}
		case OP_LIDX:
			{
				WriteOp_Lidx(jit);
				break;
			}
		case OP_LIDX_B:
			{
				WriteOp_Lidx_B(jit);
				break;
			}
		case OP_IDXADDR_B:
			{
				WriteOp_Idxaddr_B(jit);
				break;
			}
		case OP_STACK:
			{
				WriteOp_Stack(jit);
				break;
			}
		case OP_HEAP:
			{
				WriteOp_Heap(jit);
				break;
			}
		case OP_SDIV:
			{
				WriteOp_SDiv(jit);
				break;
			}
		case OP_SDIV_ALT:
			{
				WriteOp_SDiv_Alt(jit);
				break;
			}
		case OP_RETN:
			{
				WriteOp_Retn(jit);
				break;
			}
		case OP_BOUNDS:
			{
				WriteOp_Bounds(jit);
				break;
			}
		case OP_HALT:
			{
				WriteOp_Halt(jit);
				break;
			}
		case OP_BREAK:
			{
				WriteOp_Break(jit);
				break;
			}
		case OP_JUMP:
			{
				WriteOp_Jump(jit);
				break;
			}
		case OP_JZER:
			{
				WriteOp_Jzer(jit);
				break;
			}
		case OP_JNZ:
			{
				WriteOp_Jnz(jit);
				break;
			}
		case OP_JEQ:
			{
				WriteOp_Jeq(jit);
				break;
			}
		case OP_JNEQ:
			{
				WriteOp_Jneq(jit);
				break;
			}
		case OP_JSLESS:
			{
				WriteOp_Jsless(jit);
				break;
			}
		case OP_JSGRTR:
			{
				WriteOp_JsGrtr(jit);
				break;
			}
		case OP_JSGEQ:
			{
				WriteOp_JsGeq(jit);
				break;
			}
		case OP_JSLEQ:
			{
				WriteOp_Jsleq(jit);
				break;
			}
		case OP_SWITCH:
			{
				WriteOp_Switch(jit);
				break;
			}
		case OP_CASETBL:
			{
				WriteOp_Casetbl(jit);
				break;
			}
		case OP_CALL:
			{
				WriteOp_Call(jit);
				break;
			}
		case OP_SYSREQ_N:
			{
				if (data->inline_level & JIT_INLINE_NATIVES)
				{
					WriteOp_Sysreq_N(jit);
				} else {
					WriteOp_Sysreq_N_NoInline(jit);
				}
				break;
			}
#if defined USE_UNGEN_OPCODES
		#include "ungen_opcode_switch.inc"
#endif
		default:
			{
				data->error_set = SP_ERR_INVALID_INSTRUCTION;
				break;
			}
			