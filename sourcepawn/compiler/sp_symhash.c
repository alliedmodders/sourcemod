/* vim: set ts=4 sw=4 tw=99 et: */
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "sc.h"
#include "sp_symhash.h"
#include "sp_file_headers.h"

SC_FUNC uint32_t
NameHash(const char *str)
{
  size_t len = strlen(str);
  const uint8_t *data = (uint8_t *)str;
  #undef get16bits
  #if (defined(__GNUC__) && defined(__i386__)) || defined(__WATCOMC__) \
      || defined(_MSC_VER) || defined (__BORLANDC__) || defined (__TURBOC__)
    #define get16bits(d) (*((const uint16_t *) (d)))
  #endif
  #if !defined (get16bits)
    #define get16bits(d) ((((uint32_t)(((const uint8_t *)(d))[1])) << 8)\
                     +(uint32_t)(((const uint8_t *)(d))[0]) )
  #endif
  uint32_t hash = len, tmp;
  int rem;

  if (len <= 0 || data == NULL) return 0;

  rem = len & 3;
  len >>= 2;

  /* Main loop */
  for (;len > 0; len--) {
      hash  += get16bits (data);
      tmp    = (get16bits (data+2) << 11) ^ hash;
      hash   = (hash << 16) ^ tmp;
      data  += 2*sizeof (uint16_t);
      hash  += hash >> 11;
  }

  /* Handle end cases */
  switch (rem) {
    case 3: hash += get16bits (data);
      hash ^= hash << 16;
      hash ^= data[sizeof (uint16_t)] << 18;
      hash += hash >> 11;
      break;
    case 2: hash += get16bits (data);
      hash ^= hash << 11;
      hash += hash >> 17;
      break;
    case 1: hash += *data;
      hash ^= hash << 10;
      hash += hash >> 1;
  }

  /* Force "avalanching" of final 127 bits */
  hash ^= hash << 3;
  hash += hash >> 5;
  hash ^= hash << 4;
  hash += hash >> 17;
  hash ^= hash << 25;
  hash += hash >> 6;

  return hash;

  #undef get16bits
}

SC_FUNC HashTable *NewHashTable()
{
    HashTable *ht = (HashTable*)malloc(sizeof(HashTable));
    if (!ht)
        return ht;
    ht->buckets = (HashEntry **)calloc(32, sizeof(HashEntry *));
    if (!ht->buckets) {
        free(ht);
        return NULL;
    }
    ht->nbuckets = 32;
    ht->nused = 0;
    ht->bucketmask = 32 - 1;
    return ht;
}

SC_FUNC void
DestroyHashTable(HashTable *ht)
{
    uint32_t i;
    if (!ht)
        return;
    for (i = 0; i < ht->nbuckets; i++) {
        HashEntry *he = ht->buckets[i];
        while (he != NULL) {
            HashEntry *next = he->next;
            free(he);
            he = next;
        }
    }
    free(ht->buckets);
    free(ht);
}

SC_FUNC symbol *
FindTaggedInHashTable(HashTable *ht, const char *name, int fnumber,
                      int *cmptag)
{
    int count=0;
    symbol *firstmatch=NULL;
    uint32_t hash = NameHash(name);
    uint32_t bucket = hash & ht->bucketmask;
    HashEntry *he = ht->buckets[bucket];

    assert(cmptag!=NULL);

    while (he != NULL) {
        symbol *sym = he->sym;
        if ((sym->parent==NULL || sym->ident==iCONSTEXPR) &&
            (sym->fnumber<0 || sym->fnumber==fnumber) &&
            (strcmp(sym->name, name) == 0))
        {
            /* return closest match or first match; count number of matches */
            if (firstmatch==NULL)
                firstmatch=sym;
            if (*cmptag==0)
                count++;
            if (*cmptag==sym->tag) {
                *cmptag=1;    /* good match found, set number of matches to 1 */
                return sym;
            }
        }
        he = he->next;
    }

    if (firstmatch!=NULL)
        *cmptag=count;
    return firstmatch;
}

SC_FUNC symbol *
FindInHashTable(HashTable *ht, const char *name, int fnumber)
{
    uint32_t hash = NameHash(name);
    uint32_t bucket = hash & ht->bucketmask;
    HashEntry *he = ht->buckets[bucket];

    while (he != NULL) {
        symbol *sym = he->sym;
        if ((sym->parent==NULL || sym->ident==iCONSTEXPR) &&
            (sym->fnumber<0 || sym->fnumber==fnumber) &&
            (strcmp(sym->name, name) == 0))
        {
            return sym;
        }
        he = he->next;
    }

    return NULL;
}

static void
ResizeHashTable(HashTable *ht)
{
    uint32_t i;
    uint32_t xnbuckets = ht->nbuckets * 2;
    uint32_t xbucketmask = xnbuckets - 1;
    HashEntry **xbuckets = (HashEntry **)calloc(xnbuckets, sizeof(HashEntry*));
    if (!xbuckets)
        return;

    for (i = 0; i < ht->nbuckets; i++) {
        HashEntry *he = ht->buckets[i];
        while (he != NULL) {
            HashEntry *next = he->next;
            uint32_t bucket = he->sym->hash & xbucketmask;
            he->next = xbuckets[bucket];
            xbuckets[bucket] = he;
            he = next;
        }
    }
    free(ht->buckets);
    ht->buckets = xbuckets;
    ht->nbuckets = xnbuckets;
    ht->bucketmask = xbucketmask;
}

SC_FUNC void
AddToHashTable(HashTable *ht, symbol *sym)
{
    uint32_t bucket = sym->hash & ht->bucketmask;
    HashEntry **hep;

    hep = &ht->buckets[bucket];
    while (*hep) {
        assert((*hep)->sym != sym);
        hep = &(*hep)->next;
    }

    HashEntry *he = (HashEntry *)malloc(sizeof(HashEntry));
    if (!he)
      error(123);
    he->sym = sym;
    he->next = NULL;
    *hep = he;
    ht->nused++;

    if (ht->nused > ht->nbuckets && ht->nbuckets <= INT_MAX / 2)
        ResizeHashTable(ht);
}

SC_FUNC void
RemoveFromHashTable(HashTable *ht, symbol *sym)
{
    uint32_t bucket = sym->hash & ht->bucketmask;
    HashEntry **hep = &ht->buckets[bucket];
    HashEntry *he = *hep;

    while (he != NULL) {
        if (he->sym == sym) {
            *hep = he->next;
            free(he);
            ht->nused--;
            return;
        }
        hep = &he->next;
        he = *hep;
    }

    assert(0);
}

