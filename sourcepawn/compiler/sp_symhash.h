/* vim: set ts=4 sw=4 tw=99 et: */
#ifndef _INCLUDE_SPCOMP_SYMHASH_H_
#define _INCLUDE_SPCOMP_SYMHASH_H_

SC_FUNC uint32_t NameHash(const char *str);

typedef struct HashEntry {
    symbol *sym;
    struct HashEntry *next;
} HashEntry;

struct HashTable {
    uint32_t nbuckets;
    uint32_t nused;
    uint32_t bucketmask;
    HashEntry **buckets;
};

SC_FUNC HashTable *NewHashTable();
SC_FUNC void DestroyHashTable(HashTable *ht);
SC_FUNC void AddToHashTable(HashTable *ht, symbol *sym);
SC_FUNC void RemoveFromHashTable(HashTable *ht, symbol *sym);
SC_FUNC symbol *FindInHashTable(HashTable *ht, const char *name, int fnumber);
SC_FUNC symbol *FindTaggedInHashTable(HashTable *ht, const char *name, int fnumber,
                                      int *cmptag);

#endif /* _INCLUDE_SPCOMP_SYMHASH_H_ */

