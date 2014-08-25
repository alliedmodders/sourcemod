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
#ifndef _include_spcomp2_smx_rtti_builder_h_
#define _include_spcomp2_smx_rtti_builder_h_

#include <am-vector.h>
#include <am-hashmap.h>
#include <smx/smx-rtti.h>
#include "smx-builder.h"
#include "sc.h"
#include "sctracker.h"
#include "sc-utility.h"
#include "signature-map.h"

namespace ke {

using namespace sp;

class TypeInfoBuilder
{
 public:
  uint8_t *bytes() {
    return bytes_.buffer();
  }
  size_t length() const {
    return bytes_.length();
  }

  void start_method(uint32_t argc) {
    assert(argc < kMaxMethodArgs);
    bytes_.append(TypeSpec::method);
    bytes_.append(uint8_t(argc));
  }
  void append(uint8_t byte) {
    bytes_.append(byte);
  }
  void write_typeref(uint32_t refid) {
    assert(refid > 0);

    append(TypeSpec::typeref);
    write_u32(refid);
  }
  void write_u32(uint32_t u32) {
    do {
      uint8_t b = uint8_t(u32 & 0x7f);
      if (u32 > 0x7f)
        b |= 0x80;
      bytes_.append(b);
      u32 >>= 7;
    } while (u32);
  }

 private:
  Vector<uint8_t> bytes_;
};

typedef SmxListSection<smx_typedef_t> SmxTypeDefSection;

// Helper for building .rtti and .types sections.
class SmxRttiSection : public SmxSection
{
 public:
  SmxRttiSection(Ref<SmxNameTable> names);

  bool write(ISmxBuffer *buf) KE_OVERRIDE {
    return buf->write(buffer_.bytes(), buffer_.size());
  }
  size_t length() const {
    return buffer_.size();
  }

  // This must be added to the builder as well as the parent .rtti section.
  Ref<SmxTypeDefSection> typedefs() {
    return typedefs_;
  }

  // Encode a tag into a typespec.
  void embed_tag(TypeInfoBuilder &builder, int tagid);

  // Encode an argument into a typespec.
  void embed_arg(TypeInfoBuilder &builder, ArgDesc *arg);

  // Emit a type signature into the .rtti section. The value returns is a
  // typeid.
  uint32_t emit_type(TypeInfoBuilder &builder);

 private:
  // Encode a complex tag into .types and return an index.
  uint32_t encode_complex_tag(int tagid);

  // Helpers for encode_complex_tag().
  void encode_enum(smx_typedef_t &def, int tagid);
  void encode_functag(smx_typedef_t &def, int tagid);
  void embed_functag(TypeInfoBuilder &builder, funcenum_t *e);

 private:
  // The .rtti buffer.
  MemoryBuffer buffer_;

  // Embedded .types table, since it's so closely intertwined with .rtti.
  Ref<SmxTypeDefSection> typedefs_;

  // Reference to the .names table.
  Ref<SmxNameTable> names_;

  // Cache for computed tags.
  struct TagPolicy {
    static uint32_t hash(const int &tagid) {
      return HashInt32(tagid);
    }
    static bool matches(const int &tag1, const int &tag2) {
      return tag1 == tag2;
    }
  };
  typedef HashMap<int, uint32_t, TagPolicy> TagMap;
  TagMap computed_tags_;

  // Coalescing of signatures.
  typedef HashMap<SignatureKey, uint32_t, SigPolicy> SigMap;
  SigMap computed_sigs_;
};

}

#endif // _include_spcomp2_smx_rtti_builder_h_
