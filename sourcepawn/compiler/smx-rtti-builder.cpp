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
#include "smx-rtti-builder.h"
#include "sc.h"
#include "sctracker.h"

using namespace ke;
using namespace sp;

SmxRttiSection::SmxRttiSection(Ref<SmxNameTable> names)
 : SmxSection(".rtti"),
   names_(names)
{
  computed_tags_.init();
  computed_sigs_.init();
  typedefs_ = new SmxTypeDefSection(".types");

  // The first row in the typedefs table is 0, as a sentinel.
  smx_typedef_t &def = typedefs_->add();
  memset(&def, 0, sizeof(def));
}

static inline bool
TryShortEncoding(TypeInfoBuilder &builder, uint32_t *out)
{
  if (builder.length() > 4)
    return false;

  uint8_t *bytes = builder.bytes();
  if (builder.length() == 4 && (bytes[3] & 0xC0)) {
    // Since we encode the byte sequence as a little-endian uint32, and have
    // to apply the kTypeIdEmbeddedSpec mask, we can't use the encoding if
    // those bits are in use.
    return false;
  }

  *out = kTypeIdEmbeddedSpec;
  for (size_t i = 0; i < builder.length(); i++)
    *out |= uint32_t(bytes[i]) << (i * 8);
  return true;
}

uint32_t
SmxRttiSection::emit_type(TypeInfoBuilder &builder)
{
  assert(builder.length() > 0);
  uint32_t compact;
  if (TryShortEncoding(builder, &compact))
    return compact;

  ByteSequence seq;
  seq.bytes = builder.bytes();
  seq.length = builder.length();
  seq.master = buffer_.bytes();

  SigMap::Insert i = computed_sigs_.findForAdd(seq);
  if (i.found())
    return kTypeIdSpecOffset | i->value;

  SignatureKey key;
  key.offset = buffer_.pos();
  key.length = seq.length;
  computed_sigs_.add(i, key, key.offset);

  buffer_.write(seq.bytes, seq.length);
  return kTypeIdSpecOffset | key.offset;
}

static inline uint32_t
TypeSpecByteToTypeId(uint8_t tsb)
{
  return kTypeIdEmbeddedSpec | tsb;
}

static inline bool
SimpleTagToTypeSpec(int tag, uint8_t *out)
{
  if (tag == 0) {
    *out = TypeSpec::int32;
    return true;
  }
  if (tag == pc_tag_void) {
    *out = TypeSpec::void_t;
    return true;
  }
  if (tag == pc_tag_bool) {
    *out = TypeSpec::boolean;
    return true;
  }
  if (tag == pc_anytag) {
    *out = TypeSpec::any;
    return true;
  }
  if (tag == sc_rationaltag) {
    *out = TypeSpec::float32;
    return true;
  }
  if (tag == pc_tag_string) {
    *out = TypeSpec::char8;
    return true;
  }
  return false;
}

void
SmxRttiSection::embed_tag(TypeInfoBuilder &builder, int tag)
{
  uint8_t tsb;
  if (SimpleTagToTypeSpec(tag, &tsb)) {
    builder.append(tsb);
  } else {
    uint32_t typerefid = encode_complex_tag(tag);
    builder.write_typeref(typerefid);
  }
}

void
SmxRttiSection::embed_arg(TypeInfoBuilder &builder, ArgDesc *arg)
{
  switch (arg->ident()) {
   case iVARIABLE:
     embed_tag(builder, arg->tag());
     break;

   case iREFERENCE:
     if (arg->usage() & uCONST)
       builder.append(TypeSpec::constref);
     embed_tag(builder, arg->tag());
     break;

   case iREFARRAY:
     if (arg->usage() & uCONST)
       builder.append(TypeSpec::constref);
     if (arg->numdim() == 1 && arg->dim(0) == 0) {
       // We can emit the more compact flex array type.
       builder.append(TypeSpec::array);
       embed_tag(builder, arg->tag());
     } else {
       builder.append(TypeSpec::fixedarray);
       embed_tag(builder, arg->tag());
       builder.append(uint8_t(arg->numdim()));
       for (uint8_t i = 0; i < uint8_t(arg->numdim()); i++)
         builder.write_u32(arg->dim(i));
     }
     break;

   case iVARARGS:
     builder.append(MethodSignature::refva);
     embed_tag(builder, arg->tag());
     break;

   default:
    assert(false);
  }
}

uint32_t
SmxRttiSection::encode_complex_tag(int tag)
{
  // See if we've already emitted this tag.
  int tagnum = TAGID(tag);
  TagMap::Insert r = computed_tags_.findForAdd(tag);
  if (r.found())
    return kTypeIdTypeIndex | r->value;

  // Add an entry, but don't fill it in yet. We need to do this early for
  // circular references.
  uint32_t index = typedefs_->count();
  typedefs_->add();
  computed_tags_.add(r, tagnum, index);

  // Encode the definition into the empty typedef struct.
  smx_typedef_t def;
  memset(&def, 0, sizeof(def));
  if (tag & (ENUMTAG|METHODMAPTAG))
    encode_enum(def, tag);
  else if (tag & (FUNCTAG))
    encode_functag(def, tag);
  else
    assert(false);

  // Copy in the final definition data.
  typedefs_->at(index) = def;
  return kTypeIdTypeIndex | index;
}

void
SmxRttiSection::encode_enum(smx_typedef_t &def, int tag)
{
  constvalue *ptr = pc_tagptr_by_id(tag);
  def.flags = TypeDefKind::Enum;

  methodmap_t *map = methodmap_find_by_tag(ptr->value);
  if (map && map->nullable)
    def.flags |= TypeDefFlags::NULLABLE;

  def.name = names_->add(ptr->name);
  if (map && map->parent)
    def.base = encode_complex_tag(map->parent->tag);
  else
    def.base = TypeSpecByteToTypeId(TypeSpec::int32);
}

void
SmxRttiSection::encode_functag(smx_typedef_t &def, int tag)
{
  funcenum_t *fe = funcenums_find_by_tag(tag);
  assert(fe);

  TypeInfoBuilder builder;
  embed_functag(builder, fe);

  def.flags = TypeDefKind::Alias;
  def.name = names_->add(fe->name);
  def.base = emit_type(builder);
}

void
SmxRttiSection::embed_functag(TypeInfoBuilder &builder, funcenum_t *fe)
{
  if (!fe->first || fe->first != fe->last) {
    // funcenums are degenerate and need to be removed. In the meantime we
    // mark them as unencodable. This makes them a poison pill for run-time
    // type checking.
    builder.append(TypeSpec::unencodable);
    return;
  }

  functag_t *ft = fe->first;
  builder.start_method(ft->argcount);

  embed_tag(builder, ft->ret_tag);
  for (int i = 0; i < ft->argcount; i++) {
    FuncArgDesc desc(&ft->args[i]);
    embed_arg(builder, &desc);
  }
}
