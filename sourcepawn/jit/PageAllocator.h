#ifndef _INCLUDE_SOURCEPAWN_PAGE_ALLOCATOR_H_
#define _INCLUDE_SOURCEPAWN_PAGE_ALLOCATOR_H_

#include <sp_vm_types.h>

namespace SourcePawn
{
	struct Page
	{
		Page *next;
		uint8_t *data;
	};

	class PageAllocator
	{
	public:
		PageAllocator();
		~PageAllocator();
		Page *AllocPage();
		void FreePage(Page *p);
		unsigned int GetPageSize();
	private:
		Page *m_pFreePages;
		unsigned int m_PageSize;
		unsigned int m_PageGranularity;
		unsigned int m_NumBlocks;
		void **m_Blocks;
	};
}

#endif //_INCLUDE_SOURCEPAWN_PAGE_ALLOCATOR_H_
