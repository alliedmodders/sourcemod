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
#ifndef _include_spcomp2_smx_builder_h_
#define _include_spcomp2_smx_builder_h_

#include <am-string.h>
#include <am-vector.h>
#include <am-hashmap.h>
#include <am-refcounting.h>
#include <smx/smx-headers.h>
#include "string-pool.h"

namespace ke {

class ISmxBuffer
{
 public:
  virtual bool write(const void *bytes, size_t len) = 0;
  virtual size_t pos() const = 0;
};

class SmxSection : public Refcounted<SmxSection>
{
 public:
  SmxSection(const char *name)
   : name_(name)
  {
  }

  virtual bool write(ISmxBuffer *buf) = 0;
  virtual size_t length() const = 0;

  const AString &name() const {
    return name_;
  }

 private:
  AString name_;
};

template <typename T>
class SmxBlobSection : public SmxSection
{
 public:
  SmxBlobSection(const char *name)
   : SmxSection(name)
  {
  }

  T &header() {
    return t_;
  }
  void setBlob(uint8_t *blob, size_t len) {
    extra_ = blob;
    extra_len_ = len;
  }
  bool write(ISmxBuffer *buf) KE_OVERRIDE {
    if (!buf->write(&t_, sizeof(t_)))
      return false;
    return buf->write(extra_, extra_len_);
  }
  size_t length() const KE_OVERRIDE {
    return sizeof(t_) + extra_len_;
  }

 private:
  T t_;
  uint8_t *extra_;
  size_t extra_len_;
};

template <typename T>
class SmxListSection : public SmxSection
{
 public:
  SmxListSection(const char *name)
   : SmxSection(name)
  {
  }

  void append(const T &t) {
    list_.append(t);
  }
  T &add() {
    list_.append(T());
    return list_.back();
  }
  bool write(ISmxBuffer *buf) KE_OVERRIDE {
    return buf->write(list_.buffer(), list_.length() * sizeof(T));
  }
  size_t length() const KE_OVERRIDE {
    return list_.length() * sizeof(T);
  }

 private:
  Vector<T> list_;
};

class SmxNameTable : public SmxSection
{
 public:
  SmxNameTable(const char *name)
   : SmxSection(".names"),
     buffer_size_(0)
  {
    name_table_.init(64);
  }

  uint32_t add(StringPool &pool, const char *str) {
    return add(pool.add(str));
  }

  uint32_t add(Atom *str) {
    NameTable::Insert i = name_table_.findForAdd(str);
    if (i.found())
      return i->value;

    assert(IsUint32AddSafe(buffer_size_, str->length() + 1));

    uint32_t index = buffer_size_;
    name_table_.add(i, str, index);
    names_.append(str);
    buffer_size_ += str->length() + 1;
    return index;
  }

  bool write(ISmxBuffer *buf) KE_OVERRIDE;
  size_t length() const KE_OVERRIDE {
    return buffer_size_;
  }

 private:
  struct HashPolicy {
    static uint32_t hash(Atom *str) {
      return HashPointer(str);
    }
    static bool matches(Atom *a, Atom *b) {
      return a == b;
    }
  };
  typedef HashMap<Atom *, size_t, HashPolicy> NameTable;

  NameTable name_table_;
  Vector<Atom *> names_;
  uint32_t buffer_size_;
};

class SmxBuilder
{
 public:
  SmxBuilder();

  bool write(ISmxBuffer *buf);

  void add(const Ref<SmxSection> &section) {
    sections_.append(section);
  }

 private:
  Vector<Ref<SmxSection>> sections_;
};

} // namespace ke

#endif // _include_spcomp2_smx_builder_h_
