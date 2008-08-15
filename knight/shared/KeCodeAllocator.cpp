#include <KePlatform.h>
#include <assert.h>
#include <string.h>

#if defined KE_PLATFORM_WINDOWS
#include <windows.h>
#elif defined KE_PLATFORM_POSIX
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#else
#error "TODO"
#endif

#include "KeCodeAllocator.h"

#define ALIGNMENT	16

using namespace Knight;

struct KeFreedCode;

/**
 * Defines a region of memory that is made of pages. 
 */
struct KeCodeRegion
{
	KeCodeRegion *next;
	unsigned char *block_start;
	unsigned char *block_pos;
	KeFreedCode *free_list;
	size_t total_size;
	size_t end_free;
	size_t total_free;
};

/**
 * Defines freed code.  We keep the size here because 
 * when we touch the linked list we don't want to dirty pages.
 */
struct KeFreedCode
{
	KeCodeRegion *region;
	unsigned char *block_start;
	size_t size;
	KeFreedCode *next;
};

struct KeSecret
{
	KeCodeRegion *region;
	size_t size;
};

class Knight::KeCodeCache
{
public:
	/**
	 * First region that is live for use.
	 */
	KeCodeRegion *first_live;

	/**
	 * First region that is full but has free entries.
	 */
	KeCodeRegion *first_partial;

	/**
	 * First region that is full.
	 */
	KeCodeRegion *first_full;

	/**
	 * Page granularity and size.
	 */
	unsigned int page_size;
	unsigned int page_granularity;

	/**
	 * This isn't actually for code, this is the node cache.
	 */
	KeCodeRegion *node_cache;
	KeFreedCode *free_node_list;
};

KeCodeCache *Knight::KE_CreateCodeCache()
{
	KeCodeCache *cache;

	cache = new KeCodeCache;

	memset(cache, 0, sizeof(KeCodeCache));

#if defined KE_PLATFORM_WINDOWS
	SYSTEM_INFO info;

	GetSystemInfo(&info);
	cache->page_size = info.dwPageSize;
	cache->page_granularity = info.dwAllocationGranularity;
#else
	cache->page_size = cache->page_granularity = sysconf(_SC_PAGESIZE);
#endif

	return cache;
}

inline size_t MinAllocSize()
{
	size_t size;

	size = sizeof(KeSecret);
	size += ALIGNMENT;
	size -= size % ALIGNMENT;

	return size;
}

inline size_t ke_GetAllocSize(size_t size)
{
	size += sizeof(KeSecret);
	size += ALIGNMENT;
	size -= size % ALIGNMENT;

	return size;
}

void *ke_AllocInRegion(KeCodeCache *cache,
					   KeCodeRegion **prev,
					   KeCodeRegion *region,
					   unsigned char *ptr,
					   size_t alloc_size,
					   bool is_live)
{
	KeSecret *secret;

	/* Squirrel some info in the alloc. */
	secret = (KeSecret *)ptr;
	secret->region = region;
	secret->size = alloc_size;
	ptr += sizeof(KeSecret);

	region->total_free -= alloc_size;

	/* Check if we can't use the fast-path anymore. */
	if ((is_live && region->end_free < MinAllocSize())
		|| (!is_live && region->total_free < MinAllocSize()))
	{
		KeCodeRegion **start;

		*prev = region->next;

		/* Select the appropriate arena. */
		if (is_live)
		{
			if (region->total_free < MinAllocSize())
			{
				start = &cache->first_full;
			}
			else
			{
				start = &cache->first_partial;
			}
		}
		else
		{
			start = &cache->first_full;
		}
		
		region->next = *start;
		*start = region;
	}

	return ptr;
}

void *ke_AllocFromLive(KeCodeCache *cache, size_t size)
{
	void *ptr;
	size_t alloc_size;
	KeCodeRegion *region, **prev;

	region = cache->first_live;
	prev = &cache->first_live;
	alloc_size = ke_GetAllocSize(size);

	while (region != NULL)
	{
		if (region->end_free >= alloc_size)
		{
			/* Yay! We can do a simple alloc here. */
			ptr = ke_AllocInRegion(cache, prev, region, region->block_pos, alloc_size, true);

			/* Update our counters. */
			region->block_pos += alloc_size;
			region->end_free -= alloc_size;

			return ptr;
		}
		prev = &region->next;
		region = region->next;
	}

	return NULL;
}

void *ke_AllocFromPartial(KeCodeCache *cache, size_t size)
{
	void *ptr;
	size_t alloc_size;
	KeCodeRegion *region, **prev;

	region = cache->first_partial;
	prev = &cache->first_partial;
	alloc_size = ke_GetAllocSize(size);

	while (region != NULL)
	{
		if (region->total_free >= alloc_size)
		{
			KeFreedCode *node, **last;

			assert(region->free_list != NULL);

			last = &region->free_list;
			node = region->free_list;
			while (node != NULL)
			{
				if (node->size >= alloc_size)
				{
					/* Use this node */
					ptr = ke_AllocInRegion(cache, prev, region, node->block_start, alloc_size, false);

					region->total_free -= node->size;
					*last = node->next;

					/* Make sure bookkeepping is correct. */
					assert((region->free_list == NULL && region->total_free == 0)
						   || (region->free_list != NULL && region->total_free != 0));

					/* Link us back into the free node list. */
					node->next = cache->free_node_list;
					cache->free_node_list = node->next;

					return ptr;
				}
				last = &node->next;
				node = node->next;
			}
		}
		prev = &region->next;
		region = region->next;
	}

	return NULL;
}

KeCodeRegion *ke_AddRegionForSize(KeCodeCache *cache, size_t size)
{
	KeCodeRegion *region;

	region = new KeCodeRegion;

	size = ke_GetAllocSize(size);
	size += cache->page_granularity * 2;
	size -= size % cache->page_granularity;

#if defined KE_PLATFORM_WINDOWS
	region->block_start = (unsigned char *)VirtualAlloc(NULL, size, MEM_COMMIT|MEM_RESERVE, PAGE_EXECUTE_READWRITE);
#elif defined KE_PLATFORM_POSIX
	region->block_start = (unsigned char *)valloc(size);
	if (mprotect(region->block_start, size, PROT_READ|PROT_WRITE|PROT_EXEC) == -1)
	{
		free(region->block_start);
		delete region;
		return NULL;
	}
#else
#error "TODO"
#endif

	if (region->block_start == NULL)
	{
		delete region;
		return NULL;
	}

	region->block_pos = region->block_start;
	region->end_free = region->total_free = region->total_size = size;
	region->next = cache->first_live;
	cache->first_live = region;
	region->free_list = NULL;

	return region;
}

void *Knight::KE_AllocCode(KeCodeCache *cache, size_t size)
{
	void *ptr;

	/* Check live easy-adds */
	if (cache->first_live != NULL)
	{
		if ((ptr = ke_AllocFromLive(cache, size)) != NULL)
		{
			return ptr;
		}
	}
	
	/* Try looking in the free lists */
	if (cache->first_partial != NULL)
	{
		if ((ptr = ke_AllocFromPartial(cache, size)) != NULL)
		{
			return ptr;
		}
	}

	/* Create a new region */
	if (ke_AddRegionForSize(cache, size) == NULL)
	{
		return NULL;
	}

	return ke_AllocFromLive(cache, size);
}

KeFreedCode *ke_GetFreeNode(KeCodeCache *cache)
{
	KeFreedCode *ret;

	if (cache->free_node_list != NULL)
	{
		ret = cache->free_node_list;
		cache->free_node_list = ret->next;

		return ret;
	}

	/* See if the current free node region has space. */
	if (cache->node_cache != NULL
		&& cache->node_cache->end_free >= sizeof(KeFreedCode))
	{
		ret = (KeFreedCode *)cache->node_cache->block_pos;
		cache->node_cache->block_pos += sizeof(KeFreedCode);
		cache->node_cache->total_free -= sizeof(KeFreedCode);
		cache->node_cache->end_free -= sizeof(KeFreedCode);

		return ret;
	}

	/* Otherwise, we need to alloc a new region. */
	KeCodeRegion *region = new KeCodeRegion;
	
	region->block_start = new unsigned char[cache->page_size / sizeof(KeFreedCode)];
	region->block_pos = region->block_start + sizeof(KeFreedCode);
	region->total_size = cache->page_size / sizeof(KeFreedCode);
	region->total_free = region->end_free = (region->total_size - sizeof(KeFreedCode));
	region->free_list = NULL;
	region->next = cache->node_cache;
	cache->node_cache = region;

	return (KeFreedCode *)region->block_start;
}

void Knight::KE_FreeCode(KeCodeCache *cache, void *code)
{
	KeSecret *secret;
	KeFreedCode *node;
	unsigned char *ptr;
	KeCodeRegion *region;

	ptr = (unsigned char *)code;
	secret = (KeSecret *)(ptr - sizeof(KeSecret));
	region = secret->region;
	node = ke_GetFreeNode(cache);
	node->block_start = (unsigned char *)code;
	node->next = region->free_list;
	region->free_list = node;
	node->region = region;
	node->size = secret->size;
}

KeCodeRegion *ke_DestroyRegion(KeCodeRegion *region)
{
	KeCodeRegion *next;

	next = region->next;

#if defined KE_PLATFORM_WINDOWS
	VirtualFree(region->block_start, 0, MEM_RELEASE);
#else
	free(region->block_start);
#endif

	delete region;

	return next;
}

void ke_DestroyRegionChain(KeCodeRegion *first)
{
	while (first != NULL)
	{
		first = ke_DestroyRegion(first);
	}
}

void Knight::KE_DestroyCodeCache(KeCodeCache *cache)
{
	/* Destroy every region and call it a day. */
	ke_DestroyRegionChain(cache->first_full);
	ke_DestroyRegionChain(cache->first_live);
	ke_DestroyRegionChain(cache->first_partial);

	/* We use normal malloc for node cache regions */
	KeCodeRegion *region, *next;
	
	region = cache->node_cache;
	while (region != NULL)
	{
		next = region->next;
		delete [] region->block_start;
		delete region;
		region = next;
	}
	
	delete cache;
}
