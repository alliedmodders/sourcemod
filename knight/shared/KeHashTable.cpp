#include <string.h>
#include <KeHashTable.h>

using namespace Knight;

struct KeHashNode
{
	KeHashNode *next;
	uint32_t key_hash;
	const void *key;
	void *value;
};

namespace Knight
{
	class KeHashTable
	{
	public:
		KeHashNode **buckets;
		uint32_t num_buckets;
		uint32_t shift;
		uint32_t num_entries;
		KeHashGenerator key_gen;
		KeHashMarshal key_m;
		KeHashMarshal val_m;
		ke_allocator_t *node_alloc;
		size_t key_offs;
		size_t val_offs;
		size_t node_size;
		uint32_t grow_limit;
		KeHashNode *free_list;
		bool keep_free_list;
	};
}

void *ke_DefHashMalloc(ke_allocator_t *alloc, size_t amt)
{
	return malloc(amt);
}

void ke_DefHashFree(ke_allocator_t *alloc, void *addr)
{
	free(addr);
}

ke_allocator_t s_DefHashAllocator = 
{
	ke_DefHashMalloc,
	ke_DefHashFree,
	NULL
};

KeHashTable *Knight::KE_CreateHashTable(
	uint32_t bits,
	KeHashGenerator key_gen,
	const KeHashMarshal *key_marshal,
	const KeHashMarshal *val_marshal,
	ke_allocator_t *nodeAlloc,
	bool keep_free_list)
{
	KeHashTable *table;

	if (bits >= 27)
	{
		bits = 26;
	}
	else if (bits < 4)
	{
		bits = 4;
	}

	/* Validate marshals. */
	if ((key_marshal->bytes != 0
		 && key_marshal->ctor == NULL)
		|| (val_marshal->bytes != 0
			&& val_marshal->ctor == NULL))
	{
		return NULL;
	}

	table = new KeHashTable;
	table->key_gen = key_gen;
	table->key_m = *key_marshal;
	table->val_m = *val_marshal;
	table->num_entries = 0;
	table->shift = 32 - bits;
	table->node_alloc = nodeAlloc == NULL ? &s_DefHashAllocator : nodeAlloc;
	table->num_buckets = (1 << bits);
	table->grow_limit = (uint32_t)(0.9f * table->num_buckets);
	table->keep_free_list = keep_free_list;
	table->free_list = NULL;
	table->buckets = (KeHashNode **)malloc(sizeof(KeHashNode *) * table->num_buckets);
	memset(table->buckets, 0, sizeof(KeHashNode *) * table->num_buckets);

	table->key_offs = sizeof(KeHashNode);
	if (table->key_m.bytes != 0 && table->key_m.bytes % 8 != 0)
	{
		table->key_m.bytes += 8;
		table->key_m.bytes -= (table->key_m.bytes % 8);
	}
	
	table->val_offs = table->key_offs + table->key_m.bytes;
	table->node_size = table->val_offs + table->val_m.bytes;

	return table;
}

#define KE_GET_BUCKET(tbl, hsh) (&(tbl)->buckets[((hsh) * 0x9E3779B9) >> (tbl)->shift])

KeHashNode **ke_HashInternalFind(KeHashTable *table, uint32_t key_hash, const void *key)
{
	KeHashNode *node;
	KeHashNode **bucket;

	bucket = KE_GET_BUCKET(table, key_hash);

	/* :TODO: move to the front once found? */

	while ((node = *bucket) != NULL)
	{
		if (node->key_hash == key_hash
			&& ((table->key_m.cmp != NULL && table->key_m.cmp(node->key, key))
				 || node->key == key))
		{
			return bucket;
		}
		bucket = &node->next;
	}

	return bucket;
}

void ke_ResizeHashTable(KeHashTable *table, uint32_t new_shift)
{
	uint32_t entries;
	KeHashNode *next;
	KeHashNode *node;
	KeHashNode **rbucket;
	KeHashNode **old_buckets;
	uint32_t old_num_buckets;

	/* Save old data */
	old_num_buckets = table->num_buckets;
	old_buckets = table->buckets;
	entries = table->num_entries;

	/* Save new data */
	table->num_buckets = (1 << new_shift);
	table->shift = 32 - new_shift;
	table->grow_limit = (uint32_t)(0.9f * table->num_buckets);

	table->buckets = (KeHashNode **)malloc(sizeof(KeHashNode *) * table->num_buckets);
	memset(table->buckets, 0, sizeof(KeHashNode *) * table->num_buckets);

	/* For each old bucket... */
	for (uint32_t i = 0;
		 i < old_num_buckets && entries != 0;
		 i++)
	{
		node = old_buckets[i];
		/* Get each item in its list... */
		while (node != NULL)
		{
			next = node->next;
			
			/* Find the new replacement bucket it needs to go in. */
			rbucket = KE_GET_BUCKET(table, node->key_hash);

			/* Link this node to the next node in the new bucket. */
			if (*rbucket == NULL)
			{
				node->next = NULL;
			}
			else
			{
				node->next = *rbucket;
			}

			/* Add us to the front of that bucket's list. */
			*rbucket = node;

			node = next;
		}
	}

	free(old_buckets);
}

void Knight::KE_AddToHashTable(KeHashTable *table, const void *key, void *val)
{
	KeHashNode *node;
	uint32_t key_hash;
	KeHashNode **bucket;

	key_hash = table->key_gen(key);
	bucket = ke_HashInternalFind(table, key_hash, key);

	if ((node = *bucket) != NULL)
	{
		/* Already in the table */
		if ((table->val_m.cmp != NULL && table->val_m.cmp(node->value, val))
			 || node->value == val)
		{
			return;
		}

		/* Destroy old value if it's set. */
		if (node->value != NULL && table->val_m.dtor != NULL)
		{
			table->val_m.dtor(node->value);
		}
		
		/* Construct or set the new value. */
		if (table->val_m.bytes != 0)
		{
			table->val_m.ctor(node->value, val);
		}
		else
		{
			node->value = val;
		}

		return;
	}

	/* If we're overloaded, we may need to resize. 
	 * Right now, we do this if we hit a .9 entry:buckets ratio.
	 */
	if (table->num_entries >= table->grow_limit)
	{
		ke_ResizeHashTable(table, table->shift << 1);
		bucket = ke_HashInternalFind(table, key_hash, key);
	}

	if (table->free_list != NULL)
	{
		node = table->free_list;
		table->free_list = node->next;
	}
	else
	{
		node = (KeHashNode *)table->node_alloc->alloc(table->node_alloc, table->node_size);
	}
	
	if (table->key_m.bytes == 0)
	{
		node->key = key;
	}
	else
	{
		node->key = (char *)node + table->key_offs;
		table->key_m.ctor((void *)node->key, key);
	}

	if (table->val_m.bytes == 0)
	{
		node->value = val;
	}
	else
	{
		node->value = (char *)node + table->val_offs;
		table->val_m.ctor(node->value, val);
	}

	node->next = *bucket;
	node->key_hash = key_hash;
	*bucket = node;
}

inline void ke_CleanUpHashNode(KeHashTable *table, KeHashNode *node)
{
	/* Destroy old value if it's set. */
	if (node->value != NULL && table->val_m.dtor != NULL)
	{
		table->val_m.dtor(node->value);
	}

	/* Destroy the key. */
	if (table->key_m.dtor != NULL)
	{
		table->key_m.dtor(node->key);
	}

	/* Deallocate us as appropriate. */
	if (table->keep_free_list)
	{
		node->next = table->free_list;
		table->free_list = node;
	}
	else
	{
		table->node_alloc->dealloc(table->node_alloc, node);
	}
}

void Knight::KE_RemoveFromHashTable(KeHashTable *table, const void *key)
{
	KeHashNode *node;
	uint32_t key_hash;
	KeHashNode **bucket;

	key_hash = table->key_gen(key);
	bucket = ke_HashInternalFind(table, key_hash, key);

	if ((node = *bucket) == NULL)
	{
		return;
	}

	/* Link the bucket to its next (removing us). */
	*bucket = node->next;

	ke_CleanUpHashNode(table, node);
}

bool Knight::KE_FindInHashTable(KeHashTable *table, const void *key, void **value)
{
	KeHashNode *node;
	uint32_t key_hash;
	KeHashNode **bucket;

	key_hash = table->key_gen(key);
	bucket = ke_HashInternalFind(table, key_hash, key);

	if ((node = *bucket) == NULL)
	{
		return false;
	}

	if (value != NULL)
	{
		*value = node->value;
	}

	return true;
}

void Knight::KE_DestroyHashTable(KeHashTable *table)
{
	KeHashNode *node, *next;

	/* Turn off this caching! */
	table->keep_free_list = false;

	/* Find entries in buckets that need to be freed. */
	for (uint32_t i = 0; i < table->num_buckets; i++)
	{
		node = table->buckets[i];

		while (node != NULL)
		{
			next = node->next;
			ke_CleanUpHashNode(table, node);
			node = next;
		}
	}

	/* Free the free list */
	while (table->free_list != NULL)
	{
		next = table->free_list->next;
		ke_CleanUpHashNode(table, table->free_list);
		table->free_list = next;
	}

	/* Destroy everything now. */
	free(table->buckets);
	delete table;
}

void Knight::KE_ClearHashTable(KeHashTable *table)
{
	KeHashNode *node, *next;

	/* Free every entry in the table. */
	for (uint32_t i = 0; i < table->num_buckets; i++)
	{
		node = table->buckets[i];

		while (node != NULL)
		{
			next = node->next;
			ke_CleanUpHashNode(table, node);
			node = next;
		}
	}
}

#if defined _MSC_VER && (defined _M_IX86 || defined _M_AMD64 || defined _M_X64)
#pragma intrinsic(_rotl)
#endif

uint32_t Knight::KE_HashString(const void *str)
{
	uint32_t h;
	const unsigned char *us;

	h = 0;

	for (us = (const unsigned char *)str; *us != 0; us++)
	{
#if defined _MSC_VER && (defined _M_IX86 || defined _M_AMD64 || defined _M_X64)
		h = _rotl(h, 4) ^ *us;
#else
		h = ((h << 4) | (h >> 28)) ^ *us;
#endif
	}

	return h;
}

bool Knight::KE_AreStringsEqual(const void* str1, const void* str2)
{
    return (strcmp((const char*)str1, (const char*)str2) == 0) ? true : false;
}

