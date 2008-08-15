#ifndef _INCLUDE_KNIGHT_KE_LUMP_ALLOCATOR_H_
#define _INCLUDE_KNIGHT_KE_LUMP_ALLOCATOR_H_

#include <KeLinking.h>
#include <KnightAllocator.h>

namespace Knight
{
	/**
	 * @brief Creates a new lump allocator.
	 *
	 * The lump allocator is intended for cases where there are many allocations 
	 * and none need to be freed.  There is memory wastage, and the lump allocator 
	 * is typically thrown away after use.
	 *
	 * @return			New lump allocator.
	 */
	extern ke_allocator_t * KE_CreateLumpAllocator();

	/**
	 * @brief Destroys a lump allocator, freeing all of its resources.
	 *
	 * @param lump		Lump allocator.
	 */
	extern void KE_DestroyLumpAllocator(ke_allocator_t *alloc);

	/**
	 * @brief Clears a lump allocator, so its memory can be re-used from 
	 * the start.
	 *
	 * @param lump		Lump allocator.
	 */
	extern void KE_ResetLumpAllocator(ke_allocator_t *alloc);
}

#endif //_INCLUDE_KNIGHT_KE_LUMP_ALLOCATOR_H_
