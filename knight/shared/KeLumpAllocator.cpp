#include "KePlatform.h"
#include <KeLumpAllocator.h>
#include <KeVector.h>

using namespace Knight;

/**
 * :TODO: don't make this part of the page, because touching it means 
 * dirtying a page.  Instead, we should have a separate linked list.
 * Maybe that linked list itself could be marshalled from one page.
 */
struct KeLumpRegion
{
	char *base;
	char *cur;
	size_t size;
	size_t avail;
	KeLumpRegion *next;
	KeLumpRegion *prev;
};

class KeLumpAllocator
{
public:
	KeLumpAllocator() : m_pUsableRegions(NULL), m_pUnusableRegions(NULL)
	{
		m_DefLumpSize = 65536;

#if defined KE_PLATFORM_WINDOWS
		SYSTEM_INFO info;

		GetSystemInfo(&info);
		if (info.dwAllocationGranularity > m_DefLumpSize)
		{
			m_DefLumpSize = info.dwAllocationGranularity;
		}
#endif
	}

	~KeLumpAllocator()
	{
		FreeRegionChain(m_pUsableRegions);
		FreeRegionChain(m_pUnusableRegions);
	}

	void Reset()
	{
		KeLumpRegion *region;

		/* Find the tail of the usable regions. */
		region = m_pUsableRegions;
		while (region != NULL)
		{
			if (region->next == NULL)
			{
				break;
			}
		}

		/* Link the unusable chain into the usable chain. */
		if (region == NULL)
		{
			m_pUsableRegions = m_pUnusableRegions;
		}
		else
		{
			region->next = m_pUnusableRegions;
			m_pUnusableRegions->prev = region;
		}
		m_pUnusableRegions = NULL;

		region = m_pUsableRegions;
		while (region != NULL)
		{
			region->avail = region->size;
			region->cur = region->base;
			region = region->next;
		}
	}

	void FreeRegionChain(KeLumpRegion *region)
	{
		KeLumpRegion *next;

		while (region != NULL)
		{
			next = region->next;
			
#if defined KE_PLATFORM_WINDOWS
			VirtualFree(region, 0, MEM_RELEASE);
#else
            free(region);
#endif
			region = next;
		}
	}

	void *Alloc(size_t size)
	{
		char *blob;
		KeLumpRegion *region;

		if (size % 8 != 0)
		{
			size += 8;
			size -= size % 8;
		}

		region = FindRegionForSize(size);

		blob = region->cur;
		region->avail -= size;
		region->cur += size;

		/** 
		 * Technically we could make one last small allocation, but 
		 * this edge case is not worth the extra work.
		 */
		if (region->avail < 8)
		{
			/* Unlink us from the usable list */
			if (region == m_pUsableRegions)
			{
				m_pUsableRegions = m_pUsableRegions->next;
				m_pUsableRegions->prev = NULL;
			}
			else
			{
				region->prev->next = region->next;
				if (region->next != NULL)
				{
					region->next->prev = region->prev;
				}
			}

			/* Link us into the unusable list */
			region->prev = NULL;
			region->next = m_pUnusableRegions;

			if (m_pUnusableRegions != NULL)
			{
				m_pUnusableRegions->prev = region;
			}

			m_pUnusableRegions = region;
		}

		return blob;
	}

private:
	KeLumpRegion *FindRegionForSize(size_t size)
	{
		char *base;
		KeLumpRegion *region;
		size_t size_of_region;

		/**
		 * :TODO: replace this with a priority queue or something 
		 * that's actually fast.  Even worse is we dirty pages by 
		 * doing this.  Ouch!
		 */
		region = m_pUsableRegions;
		while (region != NULL)
		{
			if (region->avail >= size)
			{
				return region;
			}
			region = region->next;
		}

		/* Make sure regions end at 8-byte alignment. */
		size_of_region = sizeof(KeLumpRegion);
		if (size_of_region % 8 != 0)
		{
			size_of_region += 8;
			size_of_region -= size_of_region % 8;
		}

		/* If the size is too big, fix that. */
		if (size > m_DefLumpSize - size_of_region)
		{
			size += m_DefLumpSize;
			size -= size % m_DefLumpSize;
		}
		else
		{
			size = m_DefLumpSize;
		}

#if defined KE_PLATFORM_WINDOWS
		base = (char *)VirtualAlloc(
			NULL,
			m_DefLumpSize,
			MEM_COMMIT|MEM_RESERVE,
			PAGE_READWRITE);
#else
        base = (char*)valloc(m_DefLumpSize);
#endif

		/* Initialize the region */
		region = (KeLumpRegion *)base;
		region->base = &base[size_of_region];
		region->size = size - size_of_region;
		region->cur = region->base;
		region->avail = region->size;
		region->prev = NULL;
		region->next = m_pUsableRegions;

		if (m_pUsableRegions != NULL)
		{
			m_pUsableRegions->prev = region;
		}

		m_pUsableRegions = region;

		return region;
	}

private:
	KeLumpRegion *m_pUsableRegions;
	KeLumpRegion *m_pUnusableRegions;
	size_t m_DefLumpSize;
};

inline KeLumpAllocator *ke_LumpFromAllocator(ke_allocator_t *arena)
{
	return (KeLumpAllocator *)arena->user;
}

void *ke_LumpAlloc(ke_allocator_t *arena, size_t size)
{
	return ke_LumpFromAllocator(arena)->Alloc(size);
}

void ke_LumpFree(ke_allocator_t *arena, void *ptr)
{
}

ke_allocator_t * KE_LINK Knight::KE_CreateLumpAllocator()
{
	ke_allocator_t *alloc;

	alloc = new ke_allocator_t;
	alloc->alloc = ke_LumpAlloc;
	alloc->dealloc = ke_LumpFree;
	alloc->user = new KeLumpAllocator();

	return alloc;
}

void KE_LINK Knight::KE_DestroyLumpAllocator(ke_allocator_t *alloc)
{
	delete ke_LumpFromAllocator(alloc);
	delete alloc;
}

void KE_LINK Knight::KE_ResetLumpAllocator(ke_allocator_t *alloc)
{
	ke_LumpFromAllocator(alloc)->Reset();
}

