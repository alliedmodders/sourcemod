// vim: set sts=2 ts=8 sw=2 tw=99 et:
//
// Copyright (C) 2012-2014 AlliedModders LLC, David Anderson
//
// This file is part of SourcePawn.
//
// SourcePawn is free software: you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation, either version 3 of the License, or (at your option)
// any later version.
// 
// SourcePawn is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along with
// SourcePawn. If not, see http://www.gnu.org/licenses/.
#ifndef _include_sp_memory_buffer_h_
#define _include_sp_memory_buffer_h_

#include <stdio.h>
#include <am-utility.h>
#include "smx-builder.h"

// Interface for SmxBuilder to blit bytes.
class ISmxBuffer
{
 public:
  virtual bool write(const void *bytes, size_t len) = 0;
  virtual size_t pos() const = 0;
};

// An in-memory buffer for SmxBuilder.
class MemoryBuffer : public ISmxBuffer
{
  static const size_t kDefaultSize = 4096;

 public:
  MemoryBuffer() {
    buffer_ = (uint8_t *)calloc(kDefaultSize, 1);
    pos_ = buffer_;
    end_ = buffer_ + kDefaultSize;
  }
  ~MemoryBuffer() {
    free(buffer_);
  }

  bool write(const void *bytes, size_t len) KE_OVERRIDE {
    if (pos_ + len > end_)
      grow(len);
    memcpy(pos_, bytes, len);
    pos_ += len;
    return true;
  }

  size_t pos() const KE_OVERRIDE {
    return pos_ - buffer_;
  }

  uint8_t *bytes() const {
    return buffer_;
  }
  size_t size() const {
    return pos();
  }
  void rewind(size_t newpos) {
    assert(newpos < pos());
    pos_ = buffer_ + newpos;
  }

 private:
  void grow(size_t len) {
    if (!ke::IsUintPtrAddSafe(pos(), len)) {
      fprintf(stderr, "Allocation overflow!\n");
      abort();
    }

    size_t new_maxsize = end_ - buffer_;
    while (pos() + len > new_maxsize) {
      if (!ke::IsUintPtrMultiplySafe(new_maxsize, 2)) {
        fprintf(stderr, "Allocation overflow!\n");
        abort();
      }
      new_maxsize *= 2;
    }

    uint8_t *newbuffer = (uint8_t *)realloc(buffer_, new_maxsize);
    if (!newbuffer) {
      fprintf(stderr, "Out of memory!\n");
      abort();
    }
    pos_ = newbuffer + (pos_ - buffer_);
    end_ = newbuffer + new_maxsize;
    buffer_ = newbuffer;
  }

 private:
  uint8_t *buffer_;
  uint8_t *pos_;
  uint8_t *end_;
};

#endif // _include_sp_memory_buffer_h_
