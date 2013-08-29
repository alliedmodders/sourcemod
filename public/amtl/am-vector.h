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

#ifndef _INCLUDE_KEIMA_TPL_CPP_VECTOR_H_
#define _INCLUDE_KEIMA_TPL_CPP_VECTOR_H_

#include <new>
#include <stdlib.h>
#include <am-allocator-policies.h>
#include <am-utility.h>
#include <am-moveable.h>

namespace ke {

template <typename T, typename AllocPolicy = SystemAllocatorPolicy>
class Vector : public AllocPolicy
{
 public:
  Vector(AllocPolicy = AllocPolicy())
   : data_(NULL),
     nitems_(0),
     maxsize_(0)
  {
  }

  Vector(Moveable<Vector<T, AllocPolicy> > other) {
    data_ = other->data_;
    nitems_ = other->nitems_;
    maxsize_ = other->maxsize_;
    other->reset();
  }

  ~Vector() {
    zap();
  }

  bool append(const T &item) {
    if (!growIfNeeded(1))
      return false;
    new (&data_[nitems_]) T(item);
    nitems_++;
    return true;
  }
  bool append(Moveable<T> item) {
    if (!growIfNeeded(1))
      return false;
    new (&data_[nitems_]) T(item);
    nitems_++;
    return true;
  }
  void infallibleAppend(const T &item) {
    assert(growIfNeeded(1));
    new (&data_[nitems_]) T(item);
    nitems_++;
  }
  void infallibleAppend(Moveable<T> item) {
    assert(growIfNeeded(1));
    new (&data_[nitems_]) T(item);
    nitems_++;
  }

  T popCopy() {
    T t = at(length() - 1);
    pop();
    return t;
  }
  void pop() {
    assert(nitems_);
    data_[nitems_ - 1].~T();
    nitems_--;
  }
  bool empty() const {
    return length() == 0;
  }
  size_t length() const {
    return nitems_;
  }
  T& at(size_t i) {
    assert(i < length());
    return data_[i];
  }
  const T& at(size_t i) const {
    assert(i < length());
    return data_[i];
  }
  T& operator [](size_t i) {
    return at(i);
  }
  const T& operator [](size_t i) const {
    return at(i);
  }
  void clear() {
    nitems_ = 0;
  }
  const T &back() const {
    return at(length() - 1);
  }
  T &back() {
    return at(length() - 1);
  }

  T *buffer() const {
    return data_;
  }

  bool ensure(size_t desired) {
    if (desired <= length())
      return true;

    return growIfNeeded(desired - length());
  }

 private:
  // These are disallowed because they basically violate the failure handling
  // model for AllocPolicies and are also likely to have abysmal performance.
  Vector(const Vector<T> &other) KE_DELETE;
  Vector &operator =(const Vector<T> &other) KE_DELETE;

 private:
  void zap() {
    for (size_t i = 0; i < nitems_; i++)
      data_[i].~T();
    this->free(data_);
  }
  void reset() {
    data_ = NULL;
    nitems_ = 0;
    maxsize_ = 0;
  }

  bool growIfNeeded(size_t needed)
  {
    if (!IsUintPtrAddSafe(nitems_, needed)) {
      this->reportAllocationOverflow();
      return false;
    }
    if (nitems_ + needed < maxsize_)
      return true;

    size_t new_maxsize = maxsize_ ? maxsize_ : 8;
    while (nitems_ + needed > new_maxsize) {
      if (!IsUintPtrMultiplySafe(new_maxsize, 2)) {
        this->reportAllocationOverflow();
        return false;
      }
      new_maxsize *= 2;
    }

    T* newdata = (T*)this->malloc(sizeof(T) * new_maxsize);
    if (newdata == NULL)
      return false;
    for (size_t i = 0; i < nitems_; i++) {
      new (&newdata[i]) T(Moveable<T>(data_[i]));
      data_[i].~T();
    }
    this->free(data_);

    data_ = newdata;
    maxsize_ = new_maxsize;
    return true;
  }

 private:
  T* data_;
  size_t nitems_;
  size_t maxsize_;
};

}

#endif /* _INCLUDE_KEIMA_TPL_CPP_VECTOR_H_ */

