// vim: set sts=2 ts=8 sw=2 tw=99 et:
//
// Copyright (C) 2012-2014 David Anderson
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
#include "smx-builder.h"

using namespace ke;
using namespace sp;

SmxBuilder::SmxBuilder()
{
}

bool
SmxBuilder::write(ISmxBuffer *buf)
{
  sp_file_hdr_t header;
  header.magic = SmxConsts::FILE_MAGIC;
  header.version = SmxConsts::SP1_VERSION_1_1;
  header.compression = SmxConsts::FILE_COMPRESSION_NONE;

  header.disksize = sizeof(header) +
                    sizeof(sp_file_section_t) * sections_.length();

  // Note that |dataoffs| here is just to mimic what it would be in earlier
  // versions of Pawn. Its value does not actually matter per the SMX spec,
  // aside from having to be >= sizeof(sp_file_hdr_t). Here, we hint that
  // only the region after the section list and names should be compressed.
  header.dataoffs = header.disksize;

  size_t current_string_offset = 0;
  for (size_t i = 0; i < sections_.length(); i++) {
    Ref<SmxSection> section = sections_[i];
    header.disksize += section->length();
    current_string_offset += section->name().length() + 1;
  }
  header.disksize += current_string_offset;
  header.dataoffs += current_string_offset;

  header.imagesize = header.disksize;
  header.sections = sections_.length();

  // We put the string table after the sections table.
  header.stringtab = sizeof(header) + sizeof(sp_file_section_t) * sections_.length();

  if (!buf->write(&header, sizeof(header)))
    return false;

  size_t current_offset = sizeof(header);
  size_t current_data_offset = header.stringtab + current_string_offset;
  current_string_offset = 0;
  for (size_t i = 0; i < sections_.length(); i++) {
    sp_file_section_t s;
    s.nameoffs = current_string_offset;
    s.dataoffs = current_data_offset;
    s.size = sections_[i]->length();
    if (!buf->write(&s, sizeof(s)))
      return false;

    current_offset += sizeof(s);
    current_data_offset += s.size;
    current_string_offset += sections_[i]->name().length() + 1;
  }
  assert(buf->pos() == current_offset);
  assert(current_offset == header.stringtab);
  
  for (size_t i = 0; i < sections_.length(); i++) {
    const AString &name = sections_[i]->name();
    if (!buf->write(name.chars(), name.length() + 1))
      return false;
  }
  current_offset += current_string_offset;

  assert(buf->pos() == current_offset);

  for (size_t i = 0; i < sections_.length(); i++) {
    if (!sections_[i]->write(buf))
      return false;
    current_offset += sections_[i]->length();
  }

  assert(buf->pos() == current_offset);
  assert(current_offset == header.disksize);

  return true;
}

bool
SmxNameTable::write(ISmxBuffer *buf)
{
  for (size_t i = 0; i < names_.length(); i++) {
    Atom *str = names_[i];
    if (!buf->write(str->chars(), str->length() + 1))
      return false;
  }
  return true;
}

