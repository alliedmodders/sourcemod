#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <assert.h>
#include "AmxConverter.h"
#include "ConstantFolding.h"
#include "ArithmeticOptimizer.h"

using namespace SourcePawn;

#define AMX_OP_PROC		46

size_t UTIL_FormatArgs(char *buffer, size_t maxlength, const char *fmt, va_list ap)
{
	size_t len = vsnprintf(buffer, maxlength, fmt, ap);

	if (len >= maxlength)
	{
		buffer[maxlength - 1] = '\0';
		return (maxlength - 1);
	}
	else
	{
		return len;
	}
}

size_t UTIL_Format(char *buffer, size_t maxlength, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	size_t len = UTIL_FormatArgs(buffer, maxlength, fmt, ap);
	va_end(ap);

	return len;
}

AmxConverter::AmxConverter(PageAllocator *alloc) : 
	m_pAlloc(alloc)
{
	m_pBuf = NULL;
}

AmxConverter::~AmxConverter()
{
}

void AmxConverter::SetError(int err, const char *msg, ...)
{
	m_ErrorCode = err;

	if (m_ErrorBuffer != NULL && m_ErrorBufLen != 0)
	{
		va_list ap;
		
		va_start(ap, msg);
		UTIL_FormatArgs(m_ErrorBuffer, m_ErrorBufLen, msg, ap);
		va_end(ap);
	}
}

JsiStream *AmxConverter::Analyze(BaseContext *ctx, uint32_t code_addr, int *err, char *errbuf, size_t maxlength)
{
	JsiBufWriter *buf;

	m_pContext = ctx;
	m_pPlugin = ctx->GetPlugin();
	m_ErrorBuffer = errbuf;
	m_ErrorBufLen = maxlength;
	m_ErrorCode = SP_ERROR_NONE;

	m_pPri = NULL;
	m_pAlt = NULL;
	buf = new JsiBufWriter(m_pAlloc);

	ArithmeticOptimizer ao(buf);
	ConstantFolding cf(&ao);

	m_pBuf = &cf;
	
	if (*(cell_t *)(m_pPlugin->pcode + code_addr) != AMX_OP_PROC)
	{
		SetError(SP_ERROR_INVALID_INSTRUCTION, "A function can only start with PROC");
	}
	else
	{
		m_pParamMem = m_pBuf->ins_imm_ptr(m_pPlugin->memory);
		m_pParamFrm = m_pBuf->ins_frm();
		m_Stk.reset(m_pBuf, m_pParamFrm);

		if (!m_Reader.Read(this, m_pPlugin, code_addr + sizeof(cell_t)))
		{
			if (m_ErrorCode == SP_ERROR_NONE)
			{
				SetError(SP_ERROR_INVALID_INSTRUCTION, "Invalid instruction");
			}
		}
	}

	if (m_ErrorCode != SP_ERROR_NONE)
	{
		if (err != NULL)
		{
			*err = m_ErrorCode;
		}

		buf->kill_pages();
		delete buf;

		return NULL;
	}

	JsiStream *stream = new JsiStream(buf->getstream());
	delete buf;

	return stream;
}

void AmxConverter::SetReg(AmxReg reg, JIns *val)
{
	if (reg == Amx_Pri)
	{
		m_pPri = val;
	}
	else if (reg == Amx_Alt)
	{
		m_pAlt = val;
	}
}

JIns *AmxConverter::GetReg(AmxReg reg)
{
	if (reg == Amx_Pri)
	{
		return m_pPri;
	}
	else if (reg == Amx_Alt)
	{
		return m_pAlt;
	}
	
	return NULL;
}

bool AmxConverter::OP_BREAK()
{
	return true;
}

bool AmxConverter::OP_PROC()
{
	SetError(SP_ERROR_INVALID_INSTRUCTION, "New function detected without a return");
	return false;
}

bool AmxConverter::OP_RETN()
{
	if (m_pPri == NULL)
	{
		SetError(SP_ERROR_INVALID_INSTRUCTION, "Function returns with no value");
		return false;
	}

	if (m_Stk.top() != 0)
	{
		SetError(SP_ERROR_STACKLEAK, "Return edge has a stack leak");
		return false;
	}

	m_pBuf->ins_return(m_pPri);

	m_Reader.SetDone();

	return true;
}

bool AmxConverter::OP_STACK(cell_t offs)
{
	if (offs > 0)
	{
		return m_Stk.drop(offs);
	}
	else
	{
		m_Stk.add(abs(offs));
	}

	return true;
}

bool AmxConverter::OP_CONST_S(cell_t offs, cell_t value)
{
	return m_Stk.set(offs, m_pBuf->ins_imm(value));
}

bool AmxConverter::OP_PUSH_C(cell_t value)
{
	m_Stk.add(4);
	return m_Stk.set(m_Stk.top(), m_pBuf->ins_imm(value));
}

bool AmxConverter::OP_PUSH_REG(AmxReg reg)
{
	JIns *pIns;

	pIns = GetReg(reg);

	if (pIns == NULL)
	{
		SetError(SP_ERROR_INSTRUCTION_PARAM, "Invalid or unset register used");
		return false;
	}

	m_Stk.add(4);

	return m_Stk.set(m_Stk.top(), pIns);
}

bool AmxConverter::OP_POP_REG(AmxReg reg)
{
	JIns *pIns;

	if ((pIns = m_Stk.get(m_Stk.top())) == NULL)
	{
		SetError(SP_ERROR_INVALID_INSTRUCTION, "POP used without a valid stack");
		return false;
	}

	SetReg(reg, pIns);

	m_Stk.drop(4);

	return true;
}

bool AmxConverter::OP_STOR_S_REG(AmxReg reg, cell_t offs)
{
	JIns *pIns = NULL;

	pIns = GetReg(reg);

	if (pIns == NULL)
	{
		SetError(SP_ERROR_INSTRUCTION_PARAM, "Invalid or unset register used");
		return false;
	}

	return m_Stk.set(offs, pIns);
}

bool AmxConverter::OP_LOAD_S_REG(AmxReg reg, cell_t offs)
{
	JIns *ins;

	if ((ins = m_Stk.get(offs)) == NULL)
	{
		return false;
	}

	SetReg(reg, ins);

	return true;
}

bool AmxConverter::OP_LOAD_I()
{
	if (m_pPri == NULL)
	{
		SetError(SP_ERROR_INVALID_INSTRUCTION, "LOAD.I used with an uninitialized operand");
		return false;
	}

	m_pPri = m_pBuf->ins_load(m_pParamMem, m_pPri);

	return true;
}

bool AmxConverter::OP_ADD()
{
	if (m_pPri == NULL || m_pAlt == NULL)
	{
		SetError(SP_ERROR_INVALID_INSTRUCTION, "ADD used with uninitialized operands");
		return false;
	}

	m_pPri = m_pBuf->ins_add(m_pPri, m_pAlt);

	return true;
}

bool AmxConverter::OP_ADD_C(cell_t value)
{
	if (m_pPri == NULL)
	{
		SetError(SP_ERROR_INVALID_INSTRUCTION, "ADD.C used with uninitialized operand");
		return false;
	}

	m_pPri = m_pBuf->ins_add(m_pPri, m_pBuf->ins_imm(value));

	return true;
}

bool AmxConverter::OP_SUB_ALT()
{
	if (m_pPri == NULL || m_pAlt == NULL)
	{
		SetError(SP_ERROR_INVALID_INSTRUCTION, "SUB.ALT used with uninitialized operands");
		return false;
	}

	m_pPri = m_pBuf->ins_sub(m_pAlt, m_pPri);

	return true;
}

bool AmxConverter::OP_ZERO_REG(AmxReg reg)
{
	SetReg(reg, m_pBuf->ins_imm(0));

	return true;
}
