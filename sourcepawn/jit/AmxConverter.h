#ifndef _INCLUDE_AMX_TO_SSA_H_
#define _INCLUDE_AMX_TO_SSA_H_

#include "sp_vm_basecontext.h"
#include "JSI.h"
#include "StackTracker.h"
#include "PageAllocator.h"
#include "AmxListener.h"
#include "AmxReader.h"

class BaseContext;

namespace SourcePawn
{
	class AmxConverter : public IAmxListener
	{
	public:
		AmxConverter(PageAllocator *alloc);
		~AmxConverter();
	public:
		JsiStream *Analyze(BaseContext *ctx, 
			uint32_t code_addr, 
			int *err, 
			char *errbuf, 
			size_t maxlength);
	public:
		virtual bool OP_BREAK();
		virtual bool OP_PROC();
		virtual bool OP_RETN();
		virtual bool OP_STACK(cell_t offs);
		virtual bool OP_CONST_S(cell_t offs, cell_t value);
		virtual bool OP_PUSH_C(cell_t value);
		virtual bool OP_STOR_S_REG(AmxReg reg, cell_t offs);
		virtual bool OP_LOAD_S_REG(AmxReg reg, cell_t offs);
		virtual bool OP_ADD();
		virtual bool OP_ADD_C(cell_t value);
		virtual bool OP_SUB_ALT();
	private:
		void SetError(int err, const char *msg, ...);
	private:
		AmxReader m_Reader;
		BaseContext *m_pContext;
		char *m_ErrorBuffer;
		size_t m_ErrorBufLen;
		int m_ErrorCode;
		JIns *m_pParamFrm;
		JIns *m_pPri;
		JIns *m_pAlt;
		StackTracker m_Stk;
		JsiBufWriter *m_pBuf;
		const sp_plugin_t *m_pPlugin;
		PageAllocator *m_pAlloc;
	};
};

#endif //_INCLUDE_AMX_TO_SSA_H_
