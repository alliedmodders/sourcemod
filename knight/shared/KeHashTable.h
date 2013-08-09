#ifndef _INCLUDE_KNIGHT_KE_HASHTABLE_H_
#define _INCLUDE_KNIGHT_KE_HASHTABLE_H_

#include <stddef.h>
#include <stdint.h>
#include <KnightAllocator.h>

namespace Knight
{
	class KeHashTable;

	/**
	 * @brief Must generate a hash function given a key.
	 *
	 * @param key				Pointer to the key.
	 * @return					Hash value.
	 */
	typedef uint32_t (*KeHashGenerator)(const void *key);

	/**
	 * @brief Must compare two values.
	 *
	 * @param val1				First value.
	 * @param val2				Second value.
	 * @return					True if equal, false if not.
	 */
	typedef bool (*KeHashComparator)(const void *val1, const void *val2);

	/**
	 * @brief Must call the destructor of the given data, and free if necessary.
	 *
	 * @param val				Pointer to data.
	 */
	typedef void (*KeHashDestructor)(const void *val);

	/**
	 * @brief Must transfer the contents of an object from the source to the destination.
	 *
	 * @param dest				Destination address.
	 * @param source			Source address.
	 */
	typedef void (*KeHashCopyCtor)(void *dest, const void *source);

	/**
	 * @brief Contains information about how to process keys and values in a hash table.
	 */
	struct KeHashMarshal
	{
		size_t bytes;				/**< Bytes of storage needed (0 to use pointers). */
		KeHashComparator cmp;		/**< Comparator (if NULL, void * comparison used) */
		KeHashDestructor dtor;		/**< Optional function for performing dtor cleanup. */
		KeHashCopyCtor ctor;		/**< If bytes != 0, must be a valid function 
										 (ignored otherwise). */
	};

	/**
	 * @brief Creates a new hash table structure.
	 *
	 * @param bits				Dictates starting number of buckets as a power of two.
	 *							Pass 0 for the default (which is 4).
	 * @param key_gen			Key generation function.
	 * @param key_marshal		Structure detailing how to marshal keys.
	 * @param vak_marshal		Structure detailing how to marshal values.
	 * @param nodeAlloc			Node allocator (can be NULL for malloc/free).
	 * @param keep_free_list	True to keep a free list of nodes, false otherwise.
	 * @return					New hash table container.
	 */
	extern KeHashTable *KE_CreateHashTable(
		uint32_t bits,
		KeHashGenerator key_gen,
		const KeHashMarshal *key_marshal,
		const KeHashMarshal *val_marshal,
		ke_allocator_t *nodeAlloc,
		bool keep_free_list
		);

	/**
	 * @brief Destroys a hash table.
	 *
	 * @param table				Hash table.
	 */
	extern void KE_DestroyHashTable(KeHashTable *table);

	/**
	 * @brief Adds a key/value to the hash table.  If the pair already exists, the old value 
	 * is overwritten (calling any destructors as necessary).
	 *
	 * @param table				Hash table.
	 * @param key				Key pointer.
	 * @param val				Value pointer.
	 */
	extern void KE_AddToHashTable(KeHashTable *table, const void *key, void *val);

	/**
	 * @brief Removes a key entry from the hash table.
	 *
	 * @param table				Hash table.
	 * @param key				Key pointer.
	 */
	extern void KE_RemoveFromHashTable(KeHashTable *table, const void *key);

	/**
	 * @brief Finds an entry in the hash table.
	 *
	 * @param table				Hash table.
	 * @param key				Key pointer.
	 * @param value				Pointer to store the value (optional).
	 * @return					On success, true is returned and value is filled if given.
	 *							On failure, false is failed and outputs are undefined.
	 */
	extern bool KE_FindInHashTable(KeHashTable *table, const void *key, void **value);

	/**
	 * @brief Clears all entries in the hash table (caching free entries when possible).
	 *
	 * @param table				Hash table.
	 */
	extern void KE_ClearHashTable(KeHashTable *table);

	/**
	 * @brief Generic function for hashing strings.
	 *
	 * @param str				Key string.
	 * @return					Hash value.
	 */
	extern uint32_t KE_HashString(const void *str);

    /**
     * @brief Generic case-sensitive comparison of strings.
     *
     * @param str1              First string.
     * @param str2              Second string.
     * @return                  True if equal, false otherwise.
     */
    extern bool KE_AreStringsEqual(const void* str1, const void* str2);
}

#endif //_INCLUDE_KNIGHT_KE_HASHTABLE_H_

