/* vim: set ts=4 sw=4 tw=99 et: */
#ifndef _INCLUDE_SPCOMP_SYMHASH_H_
#define _INCLUDE_SPCOMP_SYMHASH_H_

uint32_t NameHash(const char *str);

struct HashTable;

HashTable *NewHashTable();
void DestroyHashTable(HashTable *ht);
void AddToHashTable(HashTable *ht, symbol *sym);
void RemoveFromHashTable(HashTable *ht, symbol *sym);
symbol *FindInHashTable(HashTable *ht, const char *name, int fnumber);
symbol *FindTaggedInHashTable(HashTable *ht, const char *name, int fnumber,
                                      int *cmptag);

#endif /* _INCLUDE_SPCOMP_SYMHASH_H_ */

