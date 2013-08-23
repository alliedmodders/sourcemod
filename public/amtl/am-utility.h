// vim: set sts=8 ts=2 sw=2 tw=99 et:
//
// Copyright (C) 2013, David Anderson and AlliedModders LLC
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
//  * Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//  * Neither the name of AlliedModders LLC nor the names of its contributors
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission.
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

#ifndef _include_amtl_utility_h_
#define _include_amtl_utility_h_

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#if defined(_MSC_VER)
# include <intrin.h>
#endif

#define KE_32BIT

#if defined(_MSC_VER)
# pragma warning(disable:4355)
#endif

namespace ke {

static const size_t kMallocAlignment = sizeof(void *) * 2;

typedef uint8_t uint8;
typedef int32_t int32;
typedef uint32_t uint32;
typedef int64_t int64;
typedef uint64_t uint64;

typedef uint8 * Address;

static const size_t kKB = 1024;
static const size_t kMB = 1024 * kKB;
static const size_t kGB = 1024 * kMB;

template <typename T> T
ReturnAndVoid(T &t)
{
    T saved = t;
    t = T();
    return saved;
}

// Wrapper that automatically deletes its contents. The pointer can be taken
// to avoid destruction.
template <typename T>
class AutoPtr
{
    T *t_;

  public:
    AutoPtr()
      : t_(NULL)
    {
    }
    explicit AutoPtr(T *t)
      : t_(t)
    {
    }
    ~AutoPtr() {
        delete t_;
    }
    T *take() {
        return ReturnAndVoid(t_);
    }
    T *operator *() const {
        return t_;
    }
    T *operator ->() const {
        return t_;
    }
    operator T *() const {
        return t_;
    }
    void operator =(T *t) {
        delete t_;
        t_ = t;
    }
    bool operator !() const {
        return !t_;
    }
};

// Bob Jenkin's one-at-a-time hash function[1].
//
// [1] http://burtleburtle.net/bob/hash/doobs.html
class CharacterStreamHasher
{
    uint32 hash;

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

    uint32 result() {
        hash += (hash << 3);
        hash ^= (hash >> 11);
        hash += (hash << 15);
        return hash;
    }
};

static inline uint32
HashCharSequence(const char *s, size_t length)
{
    CharacterStreamHasher hasher;
    hasher.add(s, length);
    return hasher.result();
}

// From http://burtleburtle.net/bob/hash/integer.html
static inline uint32
HashInt32(int32 a)
{
    a = (a ^ 61) ^ (a >> 16);
    a = a + (a << 3);
    a = a ^ (a >> 4);
    a = a * 0x27d4eb2d;
    a = a ^ (a >> 15);
    return a;
}

// From http://www.cris.com/~Ttwang/tech/inthash.htm
static inline uint32
HashInt64(int64 key)
{
  key = (~key) + (key << 18); // key = (key << 18) - key - 1;
  key = key ^ (uint64(key) >> 31);
  key = key * 21; // key = (key + (key << 2)) + (key << 4);
  key = key ^ (uint64(key) >> 11);
  key = key + (key << 6);
  key = key ^ (uint64(key) >> 22);
  return uint32(key);
}

static inline uint32
HashPointer(void *p)
{
#if defined(KE_32BIT)
    return HashInt32(reinterpret_cast<int32>(p));
#elif defined(KE_64BIT)
    return HashInt64(reinterpret_cast<int64>(p));
#endif
}

static inline size_t
Log2(size_t number)
{
    assert(number != 0);

#ifdef _MSC_VER
    unsigned long rval;
# ifdef _M_IX86
    _BitScanReverse(&rval, number);
# elif _M_X64
    _BitScanReverse64(&rval, number);
# endif
    return rval;
#else
    size_t bit;
    asm("bsr %1, %0\n"
        : "=r" (bit)
        : "rm" (number));
    return bit;
#endif
}

static inline size_t
FindRightmostBit(size_t number)
{
    assert(number != 0);

#ifdef _MSC_VER
    unsigned long rval;
# ifdef _M_IX86
    _BitScanForward(&rval, number);
# elif _M_X64
    _BitScanForward64(&rval, number);
# endif
    return rval;
#else
    size_t bit;
    asm("bsf %1, %0\n"
        : "=r" (bit)
        : "rm" (number));
    return bit;
#endif
}

static inline bool
IsPowerOfTwo(size_t value)
{
    if (value == 0)
        return false;
    return !(value & (value - 1));
}

static inline size_t
Align(size_t count, size_t alignment)
{
    assert(IsPowerOfTwo(alignment));
    return count + (alignment - (count % alignment)) % alignment;
}

static inline bool
IsUint32AddSafe(unsigned a, unsigned b)
{
    if (!a || !b)
        return true;
    size_t log2_a = Log2(a);
    size_t log2_b = Log2(b);
    return (log2_a < sizeof(unsigned) * 8) &&
           (log2_b < sizeof(unsigned) * 8);
}

static inline bool
IsUintPtrAddSafe(size_t a, size_t b)
{
    if (!a || !b)
        return true;
    size_t log2_a = Log2(a);
    size_t log2_b = Log2(b);
    return (log2_a < sizeof(size_t) * 8) &&
           (log2_b < sizeof(size_t) * 8);
}

static inline bool
IsUint32MultiplySafe(unsigned a, unsigned b)
{
    if (a <= 1 || b <= 1)
        return true;

    size_t log2_a = Log2(a);
    size_t log2_b = Log2(b);
    return log2_a + log2_b <= sizeof(unsigned) * 8;
}

static inline bool
IsUintPtrMultiplySafe(size_t a, size_t b)
{
    if (a <= 1 || b <= 1)
        return true;

    size_t log2_a = Log2(a);
    size_t log2_b = Log2(b);
    return log2_a + log2_b <= sizeof(size_t) * 8;
}

#define ARRAY_LENGTH(array) (sizeof(array) / sizeof(array[0]))
#define STATIC_ASSERT(cond) extern int static_assert_f(int a[(cond) ? 1 : -1])

#define IS_ALIGNED(addr, alignment)    (!(uintptr_t(addr) & ((alignment) - 1)))

template <typename T>
static inline bool
IsAligned(T addr, size_t alignment)
{
    assert(IsPowerOfTwo(alignment));
    return !(uintptr_t(addr) & (alignment - 1));
}

static inline Address
AlignedBase(Address addr, size_t alignment)
{
    assert(IsPowerOfTwo(alignment));
    return Address(uintptr_t(addr) & ~(alignment - 1));
}

template <typename T> static inline T
Min(const T &t1, const T &t2)
{
    return t1 < t2 ? t1 : t2;
}

template <typename T> static inline T
Max(const T &t1, const T &t2)
{
    return t1 > t2 ? t1 : t2;
}

#if defined(_MSC_VER)
# define KE_SIZET_FMT           "%Iu"
#elif defined(__GNUC__)
# define KE_SIZET_FMT           "%zu"
#else
# error "Implement format specifier string"
#endif

#if defined(__GNUC__)
# define KE_CRITICAL_LIKELY(x)  __builtin_expect(!!(x), 1)
#else
# define KE_CRITICAL_LIKELY(x)  x
#endif

}

#endif // _include_amtl_utility_h_
