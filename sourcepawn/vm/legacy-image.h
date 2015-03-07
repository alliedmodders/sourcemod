// vim: set sts=2 ts=8 sw=2 tw=99 et:
// 
// Copyright (C) 2006-2015 AlliedModders LLC
// 
// This file is part of SourcePawn. SourcePawn is free software: you can
// redistribute it and/or modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation, either version 3 of
// the License, or (at your option) any later version.
//
// You should have received a copy of the GNU General Public License along with
// SourcePawn. If not, see http://www.gnu.org/licenses/.
//
#ifndef _include_sourcepawn_vm_legacy_image_h_
#define _include_sourcepawn_vm_legacy_image_h_

#include <string.h>

namespace sp {

// A LegacyImage is an abstraction layer for reading the various types of
// binaries that expose Pawn or SourcePawn v1 pcode.
class LegacyImage
{
 public:
  virtual ~LegacyImage()
  {}

  enum class CodeVersion {
    Unknown,
    SP_1_0,
    SP_1_1
  };

  struct Code {
    const uint8_t *bytes;
    size_t length;
    CodeVersion version;
  };
  struct Data {
    const uint8_t *bytes;
    size_t length;
  };

  // (Almost) everything needed to implement the AMX and SPVM API.
  virtual Code DescribeCode() const = 0;
  virtual Data DescribeData() const = 0;
  virtual size_t NumNatives() const = 0;
  virtual const char *GetNative(size_t index) const = 0;
  virtual bool FindNative(const char *name, size_t *indexp) const = 0;
  virtual size_t NumPublics() const = 0;
  virtual void GetPublic(size_t index, uint32_t *offsetp, const char **namep) const = 0;
  virtual bool FindPublic(const char *name, size_t *indexp) const = 0;
  virtual size_t NumPubvars() const = 0;
  virtual void GetPubvar(size_t index, uint32_t *offsetp, const char **namep) const = 0;
  virtual bool FindPubvar(const char *name, size_t *indexp) const = 0;
  virtual size_t HeapSize() const = 0;
  virtual size_t ImageSize() const = 0;
  virtual const char *LookupFile(uint32_t code_offset) = 0;
  virtual const char *LookupFunction(uint32_t code_offset) = 0;
  virtual bool LookupLine(uint32_t code_offset, uint32_t *line) = 0;
};

class EmptyImage : public LegacyImage
{
 public:
  EmptyImage(size_t heapSize)
   : heap_size_(heapSize)
  {
    heap_size_ += sizeof(uint32_t);
    heap_size_ -= heap_size_ % sizeof(uint32_t);
    memset(data_, 0, sizeof(data_));
    memset(code_, 0, sizeof(code_));
  }

 public:
  Code DescribeCode() const KE_OVERRIDE {
    Code out;
    out.bytes = code_;
    out.length = sizeof(code_);
    out.version = CodeVersion::SP_1_1;
    return out;
  }
  Data DescribeData() const KE_OVERRIDE {
    Data out;
    out.bytes = data_;
    out.length = sizeof(data_);
    return out;
  }
  size_t NumNatives() const KE_OVERRIDE {
    return 0;
  }
  const char *GetNative(size_t index) const KE_OVERRIDE {
    return nullptr;
  }
  bool FindNative(const char *name, size_t *indexp) const KE_OVERRIDE {
    return false;
  }
  size_t NumPublics() const KE_OVERRIDE {
    return 0;
  }
  void GetPublic(size_t index, uint32_t *offsetp, const char **namep) const KE_OVERRIDE {
  }
  bool FindPublic(const char *name, size_t *indexp) const KE_OVERRIDE {
    return false;
  }
  size_t NumPubvars() const KE_OVERRIDE {
    return 0;
  }
  void GetPubvar(size_t index, uint32_t *offsetp, const char **namep) const KE_OVERRIDE {
  }
  bool FindPubvar(const char *name, size_t *indexp) const KE_OVERRIDE {
    return false;
  }
  size_t HeapSize() const KE_OVERRIDE {
    return heap_size_;
  }
  size_t ImageSize() const KE_OVERRIDE {
    return 0;
  }
  const char *LookupFile(uint32_t code_offset) KE_OVERRIDE {
    return nullptr;
  }
  const char *LookupFunction(uint32_t code_offset) {
    return nullptr;
  }
  bool LookupLine(uint32_t code_offset, uint32_t *line) KE_OVERRIDE {
    return false;
  }

 private:
  size_t heap_size_;
  uint8_t data_[4];
  uint8_t code_[4];
};

}

#endif // _include_sourcepawn_vm_legacy_image_h_
