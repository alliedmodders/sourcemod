#include <limits.h>
#include "JSI.h"

using namespace SourcePawn;

JsiStream::JsiStream(const JsiStream & other) : m_pFirstPage(other.m_pFirstPage), m_pLast(other.m_pLast)
{
}

JsiStream::JsiStream(Page *pFirstPage, JIns *pLast) : m_pFirstPage(pFirstPage), m_pLast(pLast)
{
}

JIns *JsiStream::GetFirst() const
{
	return (JIns *)m_pFirstPage->data;
}

JIns *JsiStream::GetLast() const
{
	return m_pLast - 1;
}

JsiBufWriter::JsiBufWriter(PageAllocator *allocator) : JsiPipeline(NULL)
{
	m_pPos = NULL;
	m_pFirstPage = NULL;
	m_pCurPage = NULL;
	m_pAlloc = allocator;
}

JIns *JsiBufWriter::ins_imm(int32_t value)
{
	JIns *p = ensure_room();

	p->op = uint8_t(J_imm);
	p->param1.imm = value;

	return p;
}

JIns *JsiBufWriter::ins_imm_ptr(void *value)
{
	JIns *p = ensure_room();

	p->op = uint8_t(J_imm);
	p->param1.imm = int32_t(value);
	
	return p;
}

JIns *JsiBufWriter::ins_return(JIns *val)
{
	JIns *p = ensure_room();

	p->op = uint8_t(J_return);
	p->param1.instr = val;

	return p;
}

JIns *JsiBufWriter::ins_load(JitOp op, JIns *base, int32_t disp)
{
	JIns *p = ensure_room();

	p->op = uint8_t(op);
	p->param1.instr = base;
	p->param2.imm = disp;

	return p;
}

JIns *JsiBufWriter::ins_store(JIns *base, int32_t disp, JIns *val)
{
	JIns *p = ensure_room();

	p->op = uint8_t(J_st);
	p->param1.instr = base;
	p->param2.instr = val;
	p->value.imm = disp;

	return p;
}

JIns *JsiBufWriter::ins_add(JIns *op1, JIns *op2)
{
	JIns *p = ensure_room();

	p->op = uint8_t(J_add);
	p->param1.instr = op1;
	p->param2.instr = op2;

	return p;
}

JIns *JsiBufWriter::ins_sub(JIns *op1, JIns *op2)
{
	JIns *p = ensure_room();

	p->op = uint8_t(J_sub);
	p->param1.instr = op1;
	p->param2.instr = op2;

	return p;
}

JIns *JsiBufWriter::ins_stkadd(int32_t amt)
{
	JIns *p = ensure_room();

	p->op = J_stkadd;
	p->param1.imm = amt;

	return p;
}

void JsiBufWriter::ins_stkdrop(int32_t amt)
{
	JIns *p = ensure_room();

	p->op = J_stkdrop;
	p->param1.imm = amt;
}

JIns *JsiBufWriter::ins_frm()
{
	JIns *p = ensure_room();

	p->op = J_frm;

	return p;
}

JIns *JsiBufWriter::ensure_room()
{
	JIns *ret_pos;

	if (m_pPos == NULL)
	{
		m_pFirstPage = m_pAlloc->AllocPage();
		m_pCurPage = m_pFirstPage;
		m_pPos = (JIns *)m_pFirstPage->data;
	}
	else
	{
		/* Make sure we have enough room for two full instructions */
		uintptr_t begin, pos;
		
		pos = (intptr_t)m_pPos;
		begin = (pos & ~(m_pAlloc->GetPageSize() - 1)) + sizeof(Page);
		
		if (pos + sizeof(JIns) * 2 >= begin + m_pAlloc->GetPageSize())
		{
			/* allocate a new page */
			m_pCurPage->next = m_pAlloc->AllocPage();
			m_pCurPage = m_pCurPage->next;
			ret_pos = (JIns *)m_pPos;
			ret_pos->op = uint8_t(J_next);
			ret_pos->param1.instr = (JIns *)m_pCurPage->data;
			m_pPos = (JIns *)m_pCurPage->data;
			m_pPos->op = J_prev;
			m_pPos->param1.instr = ret_pos;
			m_pPos++;
		}
	}

	ret_pos = m_pPos++;

	return ret_pos;
}

void JsiBufWriter::kill_pages()
{
	Page *temp;

	while (m_pFirstPage != NULL)
	{
		temp = m_pFirstPage->next;
		m_pAlloc->FreePage(m_pFirstPage);
		m_pFirstPage = temp;
	}
}

JsiStream JsiBufWriter::getstream()
{
	return JsiStream(m_pFirstPage, m_pPos);
}

JsiPipeline::JsiPipeline(JsiPipeline *other) : m_pOut(other)
{
}

JIns *JsiPipeline::ins_imm(int32_t value)
{
	return m_pOut->ins_imm(value);
}

JIns *JsiPipeline::ins_imm_ptr(void *value)
{
	return m_pOut->ins_imm_ptr(value);
}

JIns *JsiPipeline::ins_return(JIns *val)
{
	return m_pOut->ins_return(val);
}

JIns *JsiPipeline::ins_load(JitOp op, JIns *base, int32_t disp)
{
	return m_pOut->ins_load(op, base, disp);
}

JIns *JsiPipeline::ins_store(JIns *base, int32_t disp, JIns *val)
{
	return m_pOut->ins_store(base, disp, val);
}

JIns *JsiPipeline::ins_add(JIns *op1, JIns *op2)
{
	return m_pOut->ins_add(op1, op2);
}

JIns *JsiPipeline::ins_sub(JIns *op1, JIns *op2)
{
	return m_pOut->ins_sub(op1, op2);
}

JIns *JsiPipeline::ins_stkadd(int32_t amt)
{
	return m_pOut->ins_stkadd(amt);
}

void JsiPipeline::ins_stkdrop(int32_t amt)
{
	return m_pOut->ins_stkdrop(amt);
}

JIns *JsiPipeline::ins_frm()
{
	return m_pOut->ins_frm();
}

JsiForwardReader::JsiForwardReader(const JsiStream & stream)
{
	m_pCur = stream.GetFirst();
	m_pLast = stream.GetLast();
}

JIns *JsiForwardReader::next()
{
	JIns *ret;

	if (m_pCur == NULL)
	{
		return NULL;
	}
	else if (m_pCur->op == J_next)
	{
		m_pCur = m_pCur->param1.instr;
	}

	ret = m_pCur;
	
	if (ret == m_pLast)
	{
		m_pCur = NULL;
	}
	else
	{
		m_pCur++;
	}

	if (ret->op == J_prev || ret->op == J_nop)
	{
		return next();
	}

	return ret;
}

JsiReverseReader::JsiReverseReader(const JsiStream & stream)
{
	m_pFirst = stream.GetFirst();
	m_pCur = stream.GetLast();
}

JIns *JsiReverseReader::next()
{
	JIns *ret;

	if (m_pCur == NULL)
	{
		return NULL;
	}
	else if (m_pCur->op == J_prev)
	{
		m_pCur = m_pCur->param1.instr;
	}

	ret = m_pCur;

	if (ret == m_pFirst)
	{
		m_pCur = NULL;
	}
	else
	{
		m_pCur--;
	}

	if (ret->op == J_next || ret->op == J_nop)
	{
		return next();
	}

	return ret;
}

JsiPrinter::JsiPrinter(const JsiStream & stream) : m_Reader(stream)
{
}

const char *op_table[] = 
{
	"nop",
	"next",
	"prev",
	"return",
	"imm",
	"ld",
	"ldb",
	"st",
	"add",
	"sub",
	"stkadd",
	"stkdrop",
	"frm"
};

void JsiPrinter::emit_to_file(FILE *fp)
{
	JIns *ins;

	while ((ins = m_Reader.next()) != NULL)
	{
		fprintf(fp, " %p: %s ", ins, op_table[ins->op]);

		switch (ins->op)
		{
		case J_return:
			{
				fprintf(fp, " %p\n", ins->param1.instr);
				break;
			}
		case J_imm:
			{
				fprintf(fp, " #%d\n", ins->param1.imm);
				break;
			}
		case J_ld:
		case J_ldb:
			{
				fprintf(fp, " %p(%p)\n", ins->param2.instr, ins->param1.instr);
				break;
			}
		case J_st:
			{
				fprintf(fp, " #%d(%p), %p\n", ins->value.imm, ins->param1.instr, ins->param2.instr);
				break;
			}
		case J_add:
		case J_sub:
			{
				fprintf(fp, " %p, %p\n", ins->param1.instr, ins->param2.instr);
				break;
			}
		case J_stkadd:
		case J_stkdrop:
			{
				fprintf(fp, " %d\n", ins->param1.imm);
				break;
			}
		default:
			{
				fprintf(fp, "\n");
				break;
			}
		}
	}
}

