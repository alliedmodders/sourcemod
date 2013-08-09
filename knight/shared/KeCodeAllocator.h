#ifndef _INCLUDE_KNIGHT_KE_CODE_ALLOCATOR_H_
#define _INCLUDE_KNIGHT_KE_CODE_ALLOCATOR_H_

#include <KeLinking.h>
#include <stddef.h>
#include <stdint.h>

namespace Knight
{
	class KeCodeCache;

	/**
	 * @brief Creates a new code cache/allocator.
	 *
	 * @return				New code cache allocator.
	 */
	extern KeCodeCache *KE_CreateCodeCache();

	/**
	 * @brief Destroys a code cache allocator.
	 *
	 * @param cache			Code cache object.
	 */
	extern void KE_DestroyCodeCache(KeCodeCache *cache);

	/**
	 * @brief Allocates code memory that is readable, writable, 
	 * and executable.
	 *
	 * The address returned wlil be aligned, minimally, on a 16-byte 
	 * boundary.
	 *
	 * @param cache			Code cache object.
	 * @param size			Amount of memory needed.
	 * @return				Address pointing to the memory.
	 */
	extern void *KE_AllocCode(KeCodeCache *cache, size_t size);

	/**
	 * @brief Frees code memory.
	 *
	 * @param cache			Code cache object.
	 * @param code			Address of code memory.
	 */
	extern void KE_FreeCode(KeCodeCache *cache, void *code);
}

#endif //_INCLUDE_KNIGHT_KE_CODE_ALLOCATOR_H_
