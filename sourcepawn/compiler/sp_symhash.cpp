// vim: set ts=8 sts=2 sw=2 tw=99 et:
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "sc.h"
#include "sp_symhash.h"
#include <am-hashtable.h>

struct NameAndScope
{
  const char *name;
  int fnumber;
  int *cmptag;
  mutable symbol *matched;
  mutable int count;

  NameAndScope(const char *name, int fnumber, int *cmptag)
   : name(name),
     fnumber(fnumber),
     cmptag(cmptag),
     matched(nullptr),
     count(0)
  {
  }
};

struct SymbolHashPolicy
{
  typedef symbol *Payload;

  // Everything with the same name has the same hash, because the compiler
  // wants to know two names that have the same tag for some reason. Even
  // so, we can't be that accurate, since we might match the right symbol
  // very early.
  static uint32_t hash(const NameAndScope &key) {
    return ke::HashCharSequence(key.name, strlen(key.name));
  }
  static uint32_t hash(const symbol *s) {
    return ke::HashCharSequence(s->name, strlen(s->name));
  }

  static bool matches(const NameAndScope &key, symbol *sym) {
    if (sym->parent && sym->ident != iCONSTEXPR)
      return false;
    if (sym->fnumber >= 0 && sym->fnumber != key.fnumber)
      return false;
    if (strcmp(key.name, sym->name) != 0)
      return false;
    if (key.cmptag) {
      key.count++;
      key.matched = sym;
      if (*key.cmptag != sym->tag)
        return false;
    }
    return true;
  }
  static bool matches(const symbol *key, symbol *sym) {
    return key == sym;
  }
};

struct HashTable : public ke::HashTable<SymbolHashPolicy>
{
};

uint32_t
NameHash(const char *str)
{
  return ke::HashCharSequence(str, strlen(str));
}

HashTable *NewHashTable()
{
  HashTable *ht = new HashTable();
  if (!ht->init()) {
    delete ht;
    return nullptr;
  }
  return ht;
}

void
DestroyHashTable(HashTable *ht)
{
  delete ht;
}

symbol *
FindTaggedInHashTable(HashTable *ht, const char *name, int fnumber, int *cmptag)
{
  NameAndScope nas(name, fnumber, cmptag);
  HashTable::Result r = ht->find(nas);
  if (!r.found()) {
    if (nas.matched) {
      *cmptag = nas.count;
      return nas.matched;
    }
    return nullptr;
  }

  *cmptag = 1;
  return *r;
}

symbol *
FindInHashTable(HashTable *ht, const char *name, int fnumber)
{
  NameAndScope nas(name, fnumber, nullptr);
  HashTable::Result r = ht->find(nas);
  if (!r.found())
    return nullptr;
  return *r;
}

void
AddToHashTable(HashTable *ht, symbol *sym)
{
  HashTable::Insert i = ht->findForAdd(sym);
  assert(!i.found());
  ht->add(i, sym);
}

void
RemoveFromHashTable(HashTable *ht, symbol *sym)
{
  HashTable::Result r = ht->find(sym);
  assert(r.found());
  ht->remove(r);
}
