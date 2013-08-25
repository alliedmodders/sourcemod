// vim: set sts=8 ts=2 sw=2 tw=99 et:
//
// Copyright (C) 2013, David Anderson and AlliedModders LLC
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
//  * Redistributions of source code must retain the above copyright notice, this
//  list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright notice,
//  this list of conditions and the following disclaimer in the documentation
//  and/or other materials provided with the distribution.
//  * Neither the name of AlliedModders LLC nor the names of its contributors
//  may be used to endorse or promote products derived from this software
//  without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#ifndef _INCLUDE_KEIMA_HASHTABLE_H_
#define _INCLUDE_KEIMA_HASHTABLE_H_

#include <new>
#include <limits.h>
#include <stdlib.h>
#include "am-utility.h"

namespace ke {

namespace detail {
  template <typename T>
  class HashTableEntry
  {
    uint32_t hash_;
    T t_;
  
   public:
    static const uint32_t kFreeHash = 0;
    static const uint32_t kRemovedHash = 1;
  
   public:
    void setHash(uint32_t hash, const T &t) {
      hash_ = hash;
      new (&t_) T(t);
    }
    uint32_t hash() const {
      return hash_;
    }
    void setRemoved() {
      destruct();
      hash_ = kRemovedHash;
    }
    void setFree() {
      destruct();
      hash_ = kFreeHash;
    }
    void initialize() {
      hash_ = kFreeHash;
    }
    void destruct() {
      if (isLive())
        t_.~T();
    }
    bool removed() const {
      return hash_ == kRemovedHash;
    }
    bool free() const {
      return hash_ == kFreeHash;
    }
    bool isLive() const {
      return hash_ > kRemovedHash;
    }
    T &payload() {
      assert(isLive());
      return t_;
    }
    bool sameHash(uint32_t hash) const {
      return hash_ == hash;
    }
  
   private:
    HashTableEntry(const HashTableEntry &other) KE_DELETE;
    HashTableEntry &operator =(const HashTableEntry &other) KE_DELETE;
  };
}

// The HashPolicy for the table must have the following members:
//
//     Payload
//     static uint32_t hash(const LookupType &key);
//     static bool matches(const LookupType &key, const Payload &other);
//
// Payload must be a type, and LookupType is any type that lookups will be
// performed with (these functions can be overloaded). Example:
//
//     struct Policy {
//       typedef KeyValuePair Payload;
//       static uint32 hash(const Key &key) {
//         ...
//       }
//       static bool matches(const Key &key, const KeyValuePair &pair) {
//         ...
//       }
//     };
//
// Note that the table is not usable until init() has been called.
//
template <typename HashPolicy, typename AllocPolicy = SystemAllocatorPolicy>
class HashTable : public AllocPolicy
{
  friend class iterator;

  typedef typename HashPolicy::Payload Payload;
  typedef detail::HashTableEntry<Payload> Entry;

 private:
  static const uint32_t kMinCapacity = 16;
  static const uint32_t kMaxCapacity = INT_MAX / sizeof(Entry);

  template <typename Key>
  uint32_t computeHash(const Key &key) {
    // Multiply by golden ratio.
    uint32_t hash = HashPolicy::hash(key) * 0x9E3779B9;
    if (hash == Entry::kFreeHash || hash == Entry::kRemovedHash)
      hash += 2;
    return hash;
  }

  Entry *createTable(uint32_t capacity) {
    assert(capacity <= kMaxCapacity);

    Entry *table = (Entry *)this->malloc(capacity * sizeof(Entry));
    if (!table)
      return NULL;

    for (size_t i = 0; i < capacity; i++)
      table[i].initialize();

    return table;
  }

 public:
  class Result
  {
    friend class HashTable;

    Entry *entry_;

    Entry &entry() {
      return *entry_;
    }

   public:
    Result(Entry *entry)
      : entry_(entry)
    { }

    Payload * operator ->() {
      return &entry_->payload();
    }
    Payload & operator *() {
      return entry_->payload();
    }

    bool found() const {
      return entry_->isLive();
    }
  };

  class Insert : public Result
  {
    uint32_t hash_;

   public:
    Insert(Entry *entry, uint32_t hash)
      : Result(entry),
      hash_(hash)
    {
    }

    uint32_t hash() const {
      return hash_;
    }
  };

 private:
  class Probulator {
    uint32_t hash_;
    uint32_t capacity_;

   public:
    Probulator(uint32_t hash, uint32_t capacity)
      : hash_(hash),
      capacity_(capacity)
    {
      assert(IsPowerOfTwo(capacity_)); 
    }

    uint32_t entry() const {
      return hash_ & (capacity_ - 1);
    }
    uint32_t next() {
      hash_++;
      return entry();
    }
  };

  bool underloaded() const {
    // Check if the table is underloaded: < 25% entries used.
    return (capacity_ > kMinCapacity) && (nelements_ + ndeleted_ < capacity_ / 4);
  }
  bool overloaded() const {
    // Grow if the table is overloaded: > 75% entries used.
    return (nelements_ + ndeleted_) > ((capacity_ / 2) + (capacity_ / 4));
  }

  bool shrink() {
    if ((capacity_ >> 1) < minCapacity_)
      return true;
    return changeCapacity(capacity_ >> 1);
  }
  
  bool grow() {
    if (capacity_ >= kMaxCapacity) {
      this->reportAllocationOverflow();
      return false;
    }
    return changeCapacity(capacity_ << 1);
  }

  bool changeCapacity(uint32_t newCapacity) {
    assert(newCapacity <= kMaxCapacity);

    Entry *newTable = createTable(newCapacity);
    if (!newTable)
      return false;

    Entry *oldTable = table_;
    uint32_t oldCapacity = capacity_;

    table_ = newTable;
    capacity_ = newCapacity;
    ndeleted_ = 0;

    for (uint32_t i = 0; i < oldCapacity; i++) {
      Entry &oldEntry = oldTable[i];
      if (oldEntry.isLive()) {
        Insert p = insertUnique(oldEntry.hash());
        p.entry().setHash(p.hash(), oldEntry.payload());
      }
      oldEntry.destruct();
    }
    this->free(oldTable);

    return true;
  }

  // For use when the key is known to be unique.
  Insert insertUnique(uint32_t hash) {
    Probulator probulator(hash, capacity_);

    Entry *e = &table_[probulator.entry()];
    for (;;) {
      if (e->free() || e->removed())
        break;
      e = &table_[probulator.next()];
    }

    return Insert(e, hash);
  }

  template <typename Key>
  Result lookup(const Key &key) {
    uint32_t hash = computeHash(key);
    Probulator probulator(hash, capacity_);

    Entry *e = &table_[probulator.entry()];
    for (;;) {
      if (e->free())
        break;
      if (e->isLive() &&
        e->sameHash(hash) &&
        HashPolicy::matches(key, e->payload()))
      {
        return Result(e);
      }
      e = &table_[probulator.next()];
    }

    return Result(e);
  }

  template <typename Key>
  Insert lookupForAdd(const Key &key) {
    uint32_t hash = computeHash(key);
    Probulator probulator(hash, capacity_);

    Entry *e = &table_[probulator.entry()];
    for (;;) {
      if (!e->isLive())
        break;
      if (e->sameHash(hash) && HashPolicy::matches(key, e->payload()))
        break;
      e = &table_[probulator.next()];
    }

    return Insert(e, hash);
  }

  bool internalAdd(Insert &i, const Payload &payload) {
    assert(!i.found());

    // If the entry is deleted, just re-use the slot.
    if (i.entry().removed()) {
      ndeleted_--;
    } else {
      // Otherwise, see if we're at max capacity.
      if (nelements_ == kMaxCapacity) {
        this->reportAllocationOverflow();
        return false;
      }

      // Check if the table is over or underloaded. The table is always at
      // least 25% free, so this check is enough to guarantee one free slot.
      // (Without one free slot, insertion search could infinite loop.)
      uint32_t oldCapacity = capacity_;
      if (!checkDensity())
        return false;

      // If the table changed size, we need to find a new insertion point.
      // Note that a removed entry is impossible: either we caught it above,
      // or we just resized and no entries are removed.
      if (capacity_ != oldCapacity)
        i = insertUnique(i.hash());
    }

    i.entry().setHash(i.hash(), payload);
    nelements_++;
    return true;
  }

  void removeEntry(Entry &e) {
    assert(e.isLive());
    e.setRemoved();
    ndeleted_++;
    nelements_--;
  }

 public:
  HashTable(AllocPolicy ap = AllocPolicy())
  : AllocPolicy(ap),
    capacity_(0),
    nelements_(0),
    ndeleted_(0),
    table_(NULL),
    minCapacity_(kMinCapacity)
  {
  }

  ~HashTable()
  {
    for (uint32_t i = 0; i < capacity_; i++)
      table_[i].destruct();
    this->free(table_);
  }

  bool init(uint32_t capacity = 0) {
    if (capacity < kMinCapacity) {
      capacity = kMinCapacity;
    } else if (capacity > kMaxCapacity) {
      this->reportAllocationOverflow();
      return false;
    }

    minCapacity_ = capacity;

    assert(IsPowerOfTwo(capacity));
    capacity_ = capacity;

    table_ = createTable(capacity_);
    if (!table_)
      return false;

    return true;
  }

  // The Result object must not be used past mutating table operations.
  template <typename Key>
  Result find(const Key &key) {
    return lookup(key);
  }

  // The Insert object must not be used past mutating table operations.
  template <typename Key>
  Insert findForAdd(const Key &key) {
    return lookupForAdd(key);
  }

  template <typename Key>
  void removeIfExists(const Key &key) {
    Result r = find(key);
    if (!r.found())
      return;
    remove(r);
  }

  void remove(Result &r) {
    assert(r.found());
    removeEntry(r.entry());
  }

  // The table must not have been mutated in between findForAdd() and add().
  // The Insert object is still valid after add() returns, however.
  bool add(Insert &i, const Payload &payload) {
    return internalAdd(i, payload);
  }
  bool add(Insert &i) {
    return internalAdd(i, Payload());
  }

  bool checkDensity() {
    if (underloaded())
      return shrink(); 
    if (overloaded())
      return grow();
    return true;
  }

  void clear() {
    for (size_t i = 0; i < capacity_; i++) {
      table_[i].setFree();
    }
    ndeleted_ = 0;
    nelements_ = 0;
  }

  size_t estimateMemoryUse() const {
    return sizeof(Entry) * capacity_;
  }

 public:
  // It is illegal to mutate a HashTable during iteration.
  class iterator
  {
   public:
    iterator(HashTable *table)
      : table_(table),
      i_(table->table_),
      end_(table->table_ + table->capacity_)
    {
      while (i_ < end_ && !i_->isLive())
        i_++;
    }

    bool empty() const {
      return i_ == end_;
    }

    void erase() {
      assert(!empty());
      table_->removeEntry(*i_);
    }

    const Payload &operator *() const {
      return i_->payload();
    }

    void next() {
      do {
        i_++;
      } while (i_ < end_ && !i_->isLive());
    }

   private:
    HashTable *table_;
    Entry *i_;
    Entry *end_;
  };

 private:
  HashTable(const HashTable &other) KE_DELETE;
  HashTable &operator =(const HashTable &other) KE_DELETE;

 private:
  uint32_t capacity_;
  uint32_t nelements_;
  uint32_t ndeleted_;
  Entry *table_;
  uint32_t minCapacity_;
};

// Bob Jenkin's one-at-a-time hash function[1].
//
// [1] http://burtleburtle.net/bob/hash/doobs.html
class CharacterStreamHasher
{
  uint32_t hash;

 public:
  CharacterStreamHasher()
    : hash(0)
  { }

  void add(char c) {
    hash += c;
    hash += (hash << 10);
    hash ^= (hash >> 6);
  }

  void add(const char *s, size_t length) {
    for (size_t i = 0; i < length; i++)
      add(s[i]);
  }

  uint32_t result() {
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash;
  }
};

static inline uint32_t
HashCharSequence(const char *s, size_t length)
{
  CharacterStreamHasher hasher;
  hasher.add(s, length);
  return hasher.result();
}

static inline uint32_t
FastHashCharSequence(const char *s, size_t length)
{
  uint32_t hash = 0;
  for (size_t i = 0; i < length; i++)
    hash = s[i] + (hash << 6) + (hash << 16) - hash;
  return hash;
}

// From http://burtleburtle.net/bob/hash/integer.html
static inline uint32_t
HashInt32(int32_t a)
{
  a = (a ^ 61) ^ (a >> 16);
  a = a + (a << 3);
  a = a ^ (a >> 4);
  a = a * 0x27d4eb2d;
  a = a ^ (a >> 15);
  return a;
}

// From http://www.cris.com/~Ttwang/tech/inthash.htm
static inline uint32_t
HashInt64(int64_t key)
{
  key = (~key) + (key << 18); // key = (key << 18) - key - 1;
  key = key ^ (uint64(key) >> 31);
  key = key * 21; // key = (key + (key << 2)) + (key << 4);
  key = key ^ (uint64(key) >> 11);
  key = key + (key << 6);
  key = key ^ (uint64(key) >> 22);
  return uint32(key);
}

template <size_t Size>
static inline uint32_t
HashInteger(uintptr_t value);

template <>
inline uint32_t
HashInteger<4>(uintptr_t value)
{
  return HashInt32(value);
}

template <>
inline uint32_t
HashInteger<8>(uintptr_t value)
{
  return HashInt64(value);
}

static inline uint32_t
HashPointer(void *ptr)
{
  return HashInteger<sizeof(ptr)>(reinterpret_cast<uintptr_t>(ptr));
}

} // namespace ke

#endif // _INCLUDE_KEIMA_HASHTABLE_H_
