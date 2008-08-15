#include "KePageAllocator.h"

using namespace Knight;

struct PageInfo
{
	PageInfo *next;
	void *base;
};

class Knight::KePageAllocator
{
public:
	size_t page_size;
	size_t page_granularity;
	PageInfo *free_pages;
	PageInfo *page_blocks;
};

static void *ke_LumpPageAlloc(KePageAllocator *alloc)
{
	void *base;
	char *page;
	PageInfo *lump;
	size_t pagesInBlock;

#if defined KE_PLATFORM_WINDOWS
	base = VirtualAlloc(
		NULL,
		alloc->page_granularity,
		MEM_COMMIT|MEM_RESERVE,
		PAGE_READWRITE);
#elif defined KE_PLATFORM_POSIX
	base = valloc(alloc->page_granularity);
#else 
#error "Unsupported platform"
#endif

	if (base == NULL)
	{
		return NULL;
	}

	lump = new PageInfo;
	lump->base = base;
	lump->next = alloc->page_blocks;
	alloc->page_blocks = lump->next;

	page = (char *)base + alloc->page_size;
	pagesInBlock = alloc->page_granularity / alloc->page_size;

	for (size_t i = 1; i < pagesInBlock; i++)
	{
		lump = new PageInfo;
		lump->base = page;
		lump->next = alloc->free_pages;
		alloc->free_pages = lump;
		page += alloc->page_size;
	}

	return base;
}

KePageAllocator *Knight::KE_CreatePageAllocator()
{
	KePageAllocator *alloc;

	alloc = new KePageAllocator;
	
#if defined KE_PLATFORM_WINDOWS
	SYSTEM_INFO info;
	
	GetSystemInfo(&info);
	alloc->page_size = info.dwPageSize;
	alloc->page_granularity = info.dwAllocationGranularity;
#elif defined KE_PLATFORM_POSIX
	alloc->page_size = sysconf(_SC_PAGESIZE);
	alloc->page_granularity = alloc->page_size * 16;
#else 
#error "Unsupported platform"
#endif

	alloc->free_pages = NULL;
	alloc->page_blocks = NULL;

	return alloc;
}

void Knight::KE_DestroyPageAllocator(KePageAllocator *alloc)
{
	PageInfo *info, *next;

	info = alloc->page_blocks;
	while (info != NULL)
	{
		next = info->next;
#if defined KE_PLATFORM_WINDOWS
		VirtualFree(info->base, 0, MEM_RELEASE);
#elif defined KE_PLATFORM_WINDOWS
		free(info->base);
#else 
#error "Unsupported platform"
#endif
		delete info;
		next = info;
	}

	info = alloc->free_pages;
	while (info != NULL)
	{
		next = info->next;
		delete info;
		info = next;
	}
}

void *Knight::KE_PageAlloc(KePageAllocator *alloc)
{
	if (alloc->free_pages != NULL)
	{
		void *base;
		PageInfo *info;

		info = alloc->free_pages;
		alloc->free_pages = info->next;
		base = info->base;
		delete info;

		return base;
	}

	return ke_LumpPageAlloc(alloc);
}

void Knight::KE_PageFree(KePageAllocator *alloc, void *page)
{
	PageInfo *info;

	info = new PageInfo;
	info->base = page;
	info->next = alloc->free_pages;
	alloc->free_pages = info->next;
}

size_t Knight::KE_PageSize(KePageAllocator *alloc)
{
	return alloc->page_size;
}
