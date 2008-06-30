#ifndef _INCLUDE_STACK_TRACKER_H_
#define _INCLUDE_STACK_TRACKER_H_

#include "JSI.h"

namespace SourcePawn
{
	struct stack_entry_t
	{
		int pos;
		JIns *ins;
	};

	class StackTracker
	{
	public:
		StackTracker();
		~StackTracker();
	public:
		void set(int position, JIns *ins);
		JIns *get(int position);
		void pop(int new_stk);
	private:
		stack_entry_t *internal_get(int pos);
	private:
		stack_entry_t *m_Entries;
		size_t m_NumEntries;
		size_t m_MaxEntries;
	};
}

#endif //_INCLUDE_STACK_TRACKER_H_
