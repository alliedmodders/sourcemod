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

#ifndef _include_amtl_string_h_
#define _include_amtl_string_h_

#include <stdlib.h>
#include <string.h>
#include <am-utility.h>
#include <am-moveable.h>

namespace ke {

// ASCII string.
class AString
{
 public:
  AString()
   : length_(0)
  {
  }

  explicit AString(const char *str) {
    set(str, strlen(str));
  }
  AString(const char *str, size_t length) {
    set(str, length);
  }
  AString(const AString &other) {
    if (other.length_)
      set(other.chars_, other.length_);
    else
      length_ = 0;
  }
  AString(Moveable<AString> other)
    : chars_(other->chars_.take()),
      length_(other->length_)
  {
    other->length_ = 0;
  }

  AString &operator =(const char *str) {
    if (str && str[0]) {
      set(str, strlen(str));
    } else {
      chars_ = NULL;
      length_ = 0;
    }
    return *this;
  }
  AString &operator =(const AString &other) {
    if (other.length_) {
      set(other.chars_, other.length_);
    } else {
      chars_ = NULL;
      length_ = 0;
    }
    return *this;
  }
  AString &operator =(Moveable<AString> other) {
    chars_ = other->chars_.take();
    length_ = other->length_;
    other->length_ = 0;
    return *this;
  }

  int compare(const char *str) const {
    return strcmp(chars(), str);
  }
  int compare(const AString &other) const {
    return strcmp(chars(), other.chars());
  }
  bool operator ==(const AString &other) const {
    return other.length() == length() &&
           memcmp(other.chars(), chars(), length()) == 0;
  }

  char operator [](size_t index) const {
    assert(index < length());
    return chars()[index];
  }

  size_t length() const {
    return length_;
  }

  const char *chars() const {
    if (!chars_)
      return "";
    return chars_;
  }

 private:
  static const size_t kInvalidLength = (size_t)-1;

  void set(const char *str, size_t length) {
    chars_ = new char[length + 1];
    length_ = length;
    memcpy(chars_, str, length);
    chars_[length] = '\0';
  }

 private:
  AutoArray<char> chars_;
  size_t length_;
};

}

#endif // _include_amtl_string_h_
