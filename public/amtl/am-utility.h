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

static const size_t kKB = 1024;
static const size_t kMB = 1024 * kKB;
static const size_t kGB = 1024 * kMB;

typedef uint8_t * Address;

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

// Wrapper that automatically deletes its contents. The pointer can be taken
// to avoid destruction.
template <typename T>
class AutoArray
{
    T *t_;

  public:
    AutoArray()
      : t_(NULL)
    {
    }
    explicit AutoArray(T *t)
      : t_(t)
    {
    }
    ~AutoArray() {
        delete [] t_;
    }
    T *take() {
        return ReturnAndVoid(t_);
    }
    T *operator *() const {
        return t_;
    }
    T &operator [](size_t index) {
      return t_[index];
    }
    const T &operator [](size_t index) const {
      return t_[index];
    }
    operator T *() const {
        return t_;
    }
    void operator =(T *t) {
        delete [] t_;
        t_ = t;
    }
    bool operator !() const {
        return !t_;
    }
};

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

template <typename T>
class StorageBuffer
{
 public:
  T *address() {
    return reinterpret_cast<T *>(buffer_);
  }
  const T *address() const {
    return reinterpret_cast<const T *>(buffer_);
  }

 private:
  union {
    char buffer_[sizeof(T)];
    uint64_t aligned_;
  };
};

#if __cplusplus >= 201103L
# define KE_CXX11
#endif

#if defined(KE_CXX11)
# define KE_DELETE delete
# define KE_OVERRIDE override
#else
# define KE_DELETE
# define KE_OVERRIDE
#endif

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
