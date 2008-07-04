#ifndef _INCLUDE_SOURCEPAWN_JIT2_AMX_READER_H_
#define _INCLUDE_SOURCEPAWN_JIT2_AMX_READER_H_

#include "sp_vm_basecontext.h"
#include "AmxListener.h"

namespace SourcePawn
{
	class AmxReader
	{
	public:
		bool Read(IAmxListener *pListener, const sp_plugin_t *pl, cell_t code_offs);
		void SetDone();
	private:
		cell_t *m_pCode;
		bool m_Done;
	};
}

#endif //_INCLUDE_SOURCEPAWN_JIT2_AMX_READER_H_
