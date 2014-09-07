// vim: set sts=8 ts=2 sw=2 tw=99 et:
//
// Copyright (C) 2013-2014, David Anderson and AlliedModders LLC
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
#ifndef _include_amtl_fixedarray_h_
#define _include_amtl_fixedarray_h_

#include <am-utility.h>
#include <am-allocator-policies.h>
#include <am-moveable.h>

namespace ke {

template <typename T, typename AllocPolicy = SystemAllocatorPolicy>
class FixedArray : public AllocPolicy
{
 public:
  FixedArray(size_t length, AllocPolicy = AllocPolicy()) {
    length_ = length;
    data_ = (T *)this->malloc(sizeof(T) * length_);
    if (!data_)
      return;

    for (size_t i = 0; i < length_; i++)
      new (&data_[i]) T();
  }
  ~FixedArray() {
    for (size_t i = 0; i < length_; i++)
      data_[i].~T();
    this->free(data_);
  }

  // This call may be skipped if the allocator policy is infallible.
  bool initialize() {
    return length_ == 0 || !!data_;
  }

  size_t length() const {
    return length_;
  }
  T &operator [](size_t index) {
    return at(index);
  }
  const T &operator [](size_t index) const {
    return at(index);
  }
  T &at(size_t index) {
    assert(index < length());
    return data_[index];
  }
  const T &at(size_t index) const {
    assert(index < length());
    return data_[index];
  }
  void set(size_t index, const T &t) {
    assert(index < length());
    data_[index] = t;
  }
  void set(size_t index, ke::Moveable<T> t) {
    assert(index < length());
    data_[index] = t;
  }

 private:
  FixedArray(const FixedArray &other) KE_DELETE;
  FixedArray &operator =(const FixedArray &other) KE_DELETE;

 private:
  size_t length_;
  T *data_;
};

} // namespace ke

#endif // _include_amtl_fixedarray_h_
