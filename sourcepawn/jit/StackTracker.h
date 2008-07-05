#ifndef _INCLUDE_STACK_TRACKER_H_
#define _INCLUDE_STACK_TRACKER_H_

#include <sp_vm_types.h>
#include <sh_list.h>
#include "JSI.h"

#define STACK_FRAME_REACH		(SP_MAX_EXEC_PARAMS + 3)

using namespace SourceHook;

namespace SourcePawn
{
	struct stack_region_t
	{
		cell_t position;
		JIns *value;
	};

	class StackTracker
	{
	public:
		void reset(JsiBufWriter *writer, JIns *frm);
		void add(cell_t amt);
		bool set(cell_t offs, JIns *value);
		bool drop(cell_t amt);
		JIns *get(cell_t offs);
		cell_t top();
	private:
		stack_region_t *find_region(cell_t offs);
	private:
		JIns *m_pFrm;
		cell_t m_StackPtr;
		JsiBufWriter *m_pBuf;
		List<stack_region_t> m_Regions;
		stack_region_t m_LocalParams[STACK_FRAME_REACH];
	};
}

#endif //_INCLUDE_STACK_TRACKER_H_
