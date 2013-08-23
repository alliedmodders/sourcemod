/* vim: set ts=2 sw=2 tw=99 et:
 *
 * Copyright (C) 2012 David Anderson
 *
 * This file is part of SourcePawn.
 *
 * SourcePawn is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 * 
 * SourcePawn is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * SourcePawn. If not, see http://www.gnu.org/licenses/.
 */
#ifndef _INCLUDE_KEIMA_TPL_CPP_VECTOR_H_
#define _INCLUDE_KEIMA_TPL_CPP_VECTOR_H_

#include <new>
#include <stdlib.h>
#include <am-allocator-policies.h>
#include <am-utility.h>

namespace ke {

template <typename T, typename AllocPolicy = SystemAllocatorPolicy>
class Vector : public AllocPolicy
{
 public:
  Vector(AllocPolicy = AllocPolicy())
  : data(NULL),
    nitems(0),
    maxsize(0)
  {
  }

  ~Vector()
  {
    zap();
  }

  void steal(Vector &other) {
    zap();
    data = other.data;
    nitems = other.nitems;
    maxsize = other.maxsize;
    other.reset();
  }

  bool append(const T& item) {
    if (!growIfNeeded(1))
      return false;
    new (&data[nitems]) T(item);
    nitems++;
    return true;
  }
  void infallibleAppend(const T &item) {
    assert(growIfNeeded(1));
    new (&data[nitems]) T(item);
    nitems++;
  }
  T popCopy() {
    T t = at(length() - 1);
    pop();
    return t;
  }
  void pop() {
    assert(nitems);
    data[nitems - 1].~T();
    nitems--;
  }
  bool empty() const {
    return length() == 0;
  }
  size_t length() const {
    return nitems;
  }
  T& at(size_t i) {
    assert(i < length());
    return data[i];
  }
  const T& at(size_t i) const {
    assert(i < length());
    return data[i];
  }
  T& operator [](size_t i) {
    return at(i);
  }
  const T& operator [](size_t i) const {
    return at(i);
  }
  void clear() {
    nitems = 0;
  }
  const T &back() const {
    return at(length() - 1);
  }
  T &back() {
    return at(length() - 1);
  }

  T *buffer() const {
    return data;
  }

  bool ensure(size_t desired) {
    if (desired <= length())
      return true;

    return growIfNeeded(desired - length());
  }

 private:
  void zap() {
    for (size_t i = 0; i < nitems; i++)
      data[i].~T();
    this->free(data);
  }
  void reset() {
    data = NULL;
    nitems = 0;
    maxsize = 0;
  }

  bool growIfNeeded(size_t needed)
  {
    if (!IsUintPtrAddSafe(nitems, needed)) {
      this->reportAllocationOverflow();
      return false;
    }
    if (nitems + needed < maxsize)
      return true;
    if (maxsize == 0)
      maxsize = 8;
    while (nitems + needed > maxsize) {
      if (!IsUintPtrMultiplySafe(maxsize, 2)) {
        this->reportAllocationOverflow();
        return false;
      }
      maxsize *= 2;
    }
    T* newdata = (T*)this->malloc(sizeof(T) * maxsize);
    if (newdata == NULL)
      return false;
    for (size_t i = 0; i < nitems; i++) {
      new (&newdata[i]) T(data[i]);
      data[i].~T();
    }
    this->free(data);
    data = newdata;
    return true;
  }

 private:
  T* data;
  size_t nitems;
  size_t maxsize;
};

}

#endif /* _INCLUDE_KEIMA_TPL_CPP_VECTOR_H_ */

