#ifndef _INCLUDE_KNIGHT_KE_PAGE_ALLOCATOR_H_
#define _INCLUDE_KNIGHT_KE_PAGE_ALLOCATOR_H_

#include <stddef.h>
#include <stdint.h>

namespace Knight
{
	class KePageAllocator;

	/**
	 * @brief Creates a page allocator.
	 *
	 * @return				New page allocator.
	 */
	extern KePageAllocator *KE_CreatePageAllocator();

	/**
	 * @brief Destroys a page allocator, freeing all live pages it owns.
	 *
	 * @param				Page allocator.
	 */
	extern void KE_DestroyPageAllocator(KePageAllocator *alloc);

	/**
	 * @brief Allocates a page of memory.
	 *
	 * @param alloc			Page allocator.
	 * @return				Page of memory.
	 */
	extern void *KE_PageAlloc(KePageAllocator *alloc);

	/**
	* @brief Frees a page of memory.
	*
	* @param alloc			Page allocator.
	* @param page			Page of memory.
	*/
	extern void KE_PageFree(KePageAllocator *alloc, void *page);

	/**
	* @brief Returns the size of a page.
	*
	* @param alloc			Page allocator.
	* @return				Page size.
	*/
	extern size_t KE_PageSize(KePageAllocator *alloc);
}

#endif //_INCLUDE_KNIGHT_KE_PAGE_ALLOCATOR_H_
