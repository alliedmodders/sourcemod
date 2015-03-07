// vim: set sts=2 ts=8 sw=2 tw=99 et:
// 
// Copyright (C) 2004-2015 AlliedModers LLC
//
// This file is part of SourcePawn. SourcePawn is licensed under the GNU
// General Public License, version 3.0 (GPL). If a copy of the GPL was not
// provided with this file, you can obtain it here:
//   http://www.gnu.org/licenses/gpl.html
//
#include "smx-v1-image.h"
#include "zlib/zlib.h"

using namespace ke;
using namespace sp;

SmxV1Image::SmxV1Image(FILE *fp)
 : FileReader(fp),
   hdr_(nullptr),
   header_strings_(nullptr),
   names_section_(nullptr),
   names_(nullptr),
   debug_names_section_(nullptr),
   debug_names_(nullptr),
   debug_syms_(nullptr),
   debug_syms_unpacked_(nullptr)
{
}

// Validating SMX v1 scripts is fairly expensive. We reserve real validation
// for v2.
bool
SmxV1Image::validate()
{
  if (length_ < sizeof(sp_file_hdr_t))
    return error("bad header");

  hdr_ = (sp_file_hdr_t *)buffer();
  if (hdr_->magic != SmxConsts::FILE_MAGIC)
    return error("bad header");

  switch (hdr_->version) {
    case SmxConsts::SP1_VERSION_1_0:
    case SmxConsts::SP1_VERSION_1_1:
    case SmxConsts::SP1_VERSION_1_7:
      break;
    default:
      return error("unsupported version");
  }

  switch (hdr_->compression) {
    case SmxConsts::FILE_COMPRESSION_GZ:
    {
      // The start of the compression cannot be larger than the file.
      if (hdr_->dataoffs > length_)
        return error("illegal compressed region");

      // The compressed region must start after the header.
      if (hdr_->dataoffs < sizeof(sp_file_hdr_t))
        return error("illegal compressed region");

      // The full size of the image must be at least as large as the start
      // of the compressed region.
      if (hdr_->imagesize < hdr_->dataoffs)
        return error("illegal image size");

      // Allocate the uncompressed image buffer.
      uint32_t compressedSize = hdr_->disksize - hdr_->dataoffs;
      AutoArray<uint8_t> uncompressed(new uint8_t[hdr_->imagesize]);
      if (!uncompressed)
        return error("out of memory");

      // Decompress.
      const uint8_t *src = buffer() + hdr_->dataoffs;
      uint8_t *dest = (uint8_t *)uncompressed + hdr_->dataoffs;
      uLongf destlen = hdr_->imagesize - hdr_->dataoffs;
      int rv = uncompress(
        (Bytef *)dest,
        &destlen,
        src,
        compressedSize);
      if (rv != Z_OK)
        return error("could not decode compressed region");

      // Copy the initial uncompressed region back in.
      memcpy((uint8_t *)uncompressed, buffer(), hdr_->dataoffs);

      // Replace the original buffer.
      length_ = hdr_->imagesize;
      buffer_ = uncompressed.take();
      hdr_ = (sp_file_hdr_t *)buffer();
      break;
    }

    case SmxConsts::FILE_COMPRESSION_NONE:
      break;

    default:
      return error("unknown compression type");
  }

  // Validate the string table.
  if (hdr_->stringtab >= length_)
    return error("invalid string table");
  header_strings_ = reinterpret_cast<const char *>(buffer() + hdr_->stringtab);

  // Validate sections header.
  if ((sizeof(sp_file_hdr_t) + hdr_->sections * sizeof(sp_file_section_t)) > length_)
    return error("invalid section table");

  size_t last_header_string = 0;
  const sp_file_section_t *sections =
    reinterpret_cast<const sp_file_section_t *>(buffer() + sizeof(sp_file_hdr_t));
  for (size_t i = 0; i < hdr_->sections; i++) {
    if (sections[i].nameoffs >= (hdr_->dataoffs - hdr_->stringtab))
      return error("invalid section name");

    if (sections[i].nameoffs > last_header_string)
      last_header_string = sections[i].nameoffs;

    sections_.append(Section());
    sections_.back().dataoffs = sections[i].dataoffs;
    sections_.back().size = sections[i].size;
    sections_.back().name = header_strings_ + sections[i].nameoffs;
  }

  // Validate sanity of section header strings.
  bool found_terminator = false;
  for (const uint8_t *iter = buffer() + last_header_string;
       iter < buffer() + hdr_->dataoffs;
       iter++)
  {
    if (*iter == '\0') {
      found_terminator = true;
      break;
    }
  }
  if (!found_terminator)
    return error("malformed section names header");

  names_section_ = findSection(".names");
  if (!names_section_)
    return error("could not find .names section");
  if (!validateSection(names_section_))
    return error("invalid names section");
  names_ = reinterpret_cast<const char *>(buffer() + names_section_->dataoffs);

  // The names section must be 0-length or be null-terminated.
  if (names_section_->size != 0 &&
      *(names_ + names_section_->size - 1) != '\0')
  {
    return error("malformed names section");
  }

  if (!validateCode())
    return false;
  if (!validateData())
    return false;
  if (!validatePublics())
    return false;
  if (!validatePubvars())
    return false;
  if (!validateNatives())
    return false;
  if (!validateDebugInfo())
    return false;

  return true;
}

const SmxV1Image::Section *
SmxV1Image::findSection(const char *name)
{
  for (size_t i = 0; i < sections_.length(); i++) {
    if (strcmp(sections_[i].name, name) == 0)
      return &sections_[i];
  }
  return nullptr;
}

bool
SmxV1Image::validateSection(const Section *section)
{
  if (section->dataoffs >= length_)
    return false;
  if (section->size > length_ - section->dataoffs)
    return false;
  return true;
}

bool
SmxV1Image::validateData()
{
  // .data is required.
  const Section *section = findSection(".data");
  if (!section)
    return error("could not find data");
  if (!validateSection(section))
    return error("invalid data section");

  const sp_file_data_t *data =
    reinterpret_cast<const sp_file_data_t *>(buffer() + section->dataoffs);
  if (data->data > section->size)
    return error("invalid data blob");
  if (data->datasize > (section->size - data->data))
    return error("invalid data blob");

  const uint8_t *blob =
    reinterpret_cast<const uint8_t *>(data) + data->data;
  data_ = Blob<sp_file_data_t>(section, data, blob, data->datasize);
  return true;
}

bool
SmxV1Image::validateCode()
{
  // .code is required.
  const Section *section = findSection(".code");
  if (!section)
    return error("could not find code");
  if (!validateSection(section))
    return error("invalid code section");

  const sp_file_code_t *code =
    reinterpret_cast<const sp_file_code_t *>(buffer() + section->dataoffs);
  if (code->codeversion < SmxConsts::CODE_VERSION_SP1_MIN)
    return error("code version is too old, no longer supported");
  if (code->codeversion > SmxConsts::CODE_VERSION_SP1_MAX)
    return error("code version is too new, not supported");
  if (code->cellsize != 4)
    return error("unsupported cellsize");
  if (code->flags & ~CODEFLAG_DEBUG)
    return error("unsupported code settings");
  if (code->code > section->size)
    return error("invalid code blob");
  if (code->codesize > (section->size - code->code))
    return error("invalid code blob");

  const uint8_t *blob =
    reinterpret_cast<const uint8_t *>(code) + code->code;
  code_ = Blob<sp_file_code_t>(section, code, blob, code->codesize);
  return true;
}

bool
SmxV1Image::validatePublics()
{
  const Section *section = findSection(".publics");
  if (!section)
    return true;
  if (!validateSection(section))
    return error("invalid .publics section");
  if ((section->size % sizeof(sp_file_publics_t)) != 0)
    return error("invalid .publics section");

  const sp_file_publics_t *publics =
    reinterpret_cast<const sp_file_publics_t *>(buffer() + section->dataoffs);
  size_t length = section->size / sizeof(sp_file_publics_t);

  for (size_t i = 0; i < length; i++) {
    if (!validateName(publics[i].name))
      return error("invalid public name");
  }

  publics_ = List<sp_file_publics_t>(publics, length);
  return true;
}

bool
SmxV1Image::validatePubvars()
{
  const Section *section = findSection(".pubvars");
  if (!section)
    return true;
  if (!validateSection(section))
    return error("invalid .pubvars section");
  if ((section->size % sizeof(sp_file_pubvars_t)) != 0)
    return error("invalid .pubvars section");

  const sp_file_pubvars_t *pubvars =
    reinterpret_cast<const sp_file_pubvars_t *>(buffer() + section->dataoffs);
  size_t length = section->size / sizeof(sp_file_pubvars_t);

  for (size_t i = 0; i < length; i++) {
    if (!validateName(pubvars[i].name))
      return error("invalid pubvar name");
  }

  pubvars_ = List<sp_file_pubvars_t>(pubvars, length);
  return true;
}

bool
SmxV1Image::validateNatives()
{
  const Section *section = findSection(".natives");
  if (!section)
    return true;
  if (!validateSection(section))
    return error("invalid .natives section");
  if ((section->size % sizeof(sp_file_natives_t)) != 0)
    return error("invalid .natives section");

  const sp_file_natives_t *natives =
    reinterpret_cast<const sp_file_natives_t *>(buffer() + section->dataoffs);
  size_t length = section->size / sizeof(sp_file_natives_t);

  for (size_t i = 0; i < length; i++) {
    if (!validateName(natives[i].name))
      return error("invalid pubvar name");
  }

  natives_ = List<sp_file_natives_t>(natives, length);
  return true;
}

bool
SmxV1Image::validateName(size_t offset)
{
  return offset < names_section_->size;
}

bool
SmxV1Image::validateDebugInfo()
{
  const Section *dbginfo = findSection(".dbg.info");
  if (!dbginfo)
    return true;
  if (!validateSection(dbginfo))
    return error("invalid .dbg.info section");

  debug_info_ =
    reinterpret_cast<const sp_fdbg_info_t *>(buffer() + dbginfo->dataoffs);

  debug_names_section_ = findSection(".dbg.strings");
  if (!debug_names_section_)
    return error("no debug string table");
  if (!validateSection(debug_names_section_))
    return error("invalid .dbg.strings section");
  debug_names_ = reinterpret_cast<const char *>(buffer() + debug_names_section_->dataoffs);

  // Name tables must be null-terminated.
  if (debug_names_section_->size != 0 &&
      *(debug_names_ + debug_names_section_->size - 1) != '\0')
  {
    return error("invalid .dbg.strings section");
  }

  const Section *files = findSection(".dbg.files");
  if (!files)
    return error("no debug file table");
  if (!validateSection(files))
    return error("invalid debug file table");
  if (files->size < sizeof(sp_fdbg_file_t) * debug_info_->num_files)
    return error("invalid debug file table");
  debug_files_ = List<sp_fdbg_file_t>(
    reinterpret_cast<const sp_fdbg_file_t *>(buffer() + files->dataoffs),
    debug_info_->num_files);

  const Section *lines = findSection(".dbg.lines");
  if (!lines)
    return error("no debug lines table");
  if (!validateSection(lines))
    return error("invalid debug lines table");
  if (lines->size < sizeof(sp_fdbg_line_t) * debug_info_->num_lines)
    return error("invalid debug lines table");
  debug_lines_ = List<sp_fdbg_line_t>(
    reinterpret_cast<const sp_fdbg_line_t *>(buffer() + lines->dataoffs),
    debug_info_->num_lines);

  debug_symbols_section_ = findSection(".dbg.symbols");
  if (!debug_symbols_section_)
    return error("no debug symbol table");
  if (!validateSection(debug_symbols_section_))
    return error("invalid debug symbol table");

  // See the note about unpacked debug sections in smx-headers.h.
  if (hdr_->version == SmxConsts::SP1_VERSION_1_0 &&
      !findSection(".dbg.natives"))
  {
    debug_syms_unpacked_ =
      reinterpret_cast<const sp_u_fdbg_symbol_t *>(buffer() + debug_symbols_section_->dataoffs);
  } else {
    debug_syms_ =
      reinterpret_cast<const sp_fdbg_symbol_t *>(buffer() + debug_symbols_section_->dataoffs);
  }

  return true;
}

auto
SmxV1Image::DescribeCode() const -> Code
{
  Code code;
  code.bytes = code_.blob();
  code.length = code_.length();
  switch (code_->codeversion) {
    case SmxConsts::CODE_VERSION_JIT_1_0:
      code.version = CodeVersion::SP_1_0;
      break;
    case SmxConsts::CODE_VERSION_JIT_1_1:
      code.version = CodeVersion::SP_1_1;
      break;
    default:
      assert(false);
      code.version = CodeVersion::Unknown;
      break;
  }
  return code;
}

auto
SmxV1Image::DescribeData() const -> Data
{
  Data data;
  data.bytes = data_.blob();
  data.length = data_.length();
  return data;
}

size_t
SmxV1Image::NumNatives() const
{
  return natives_.length();
}

const char *
SmxV1Image::GetNative(size_t index) const
{
  assert(index < natives_.length());
  return names_ + natives_[index].name;
}

bool
SmxV1Image::FindNative(const char *name, size_t *indexp) const
{
  for (size_t i = 0; i < natives_.length(); i++) {
    const char *candidate = names_ + natives_[i].name;
    if (strcmp(candidate, name) == 0) {
      if (indexp)
        *indexp = i;
      return true;
    }
  }
  return false;
}

size_t
SmxV1Image::NumPublics() const
{
  return publics_.length();
}

void
SmxV1Image::GetPublic(size_t index, uint32_t *offsetp, const char **namep) const
{
  assert(index < publics_.length());
  if (offsetp)
    *offsetp = publics_[index].address;
  if (namep)
    *namep = names_ + publics_[index].name;
}

bool
SmxV1Image::FindPublic(const char *name, size_t *indexp) const
{
  int high = publics_.length() - 1;
  int low = 0;
  while (low <= high) {
    int mid = (low + high) / 2;
    const char *candidate = names_ + publics_[mid].name;
    int diff = strcmp(candidate, name);
    if (diff == 0) {
      if (indexp) {
        *indexp = mid;
        return true;
      }
    }
    if (diff < 0)
      low = mid + 1;
    else
      high = mid - 1;
  }
  return false;
}

size_t
SmxV1Image::NumPubvars() const
{
  return pubvars_.length();
}

void
SmxV1Image::GetPubvar(size_t index, uint32_t *offsetp, const char **namep) const
{
  assert(index < pubvars_.length());
  if (offsetp)
    *offsetp = pubvars_[index].address;
  if (namep)
    *namep = names_ + pubvars_[index].name;
}

bool
SmxV1Image::FindPubvar(const char *name, size_t *indexp) const
{
  int high = pubvars_.length() - 1;
  int low = 0;
  while (low <= high) {
    int mid = (low + high) / 2;
    const char *candidate = names_ + pubvars_[mid].name;
    int diff = strcmp(candidate, name);
    if (diff == 0) {
      if (indexp) {
        *indexp = mid;
        return true;
      }
    }
    if (diff < 0)
      low = mid + 1;
    else
      high = mid - 1;
  }
  return false;
}

size_t
SmxV1Image::HeapSize() const
{
  return data_->memsize;
}

size_t
SmxV1Image::ImageSize() const
{
  return length_;
}

const char *
SmxV1Image::LookupFile(uint32_t addr)
{
  int high = debug_files_.length();
  int low = -1;

  while (high - low > 1) {
    int mid = (low + high) / 2;
    if (debug_files_[mid].addr <= addr)
      low = mid;
    else
      high = mid;
  }

  if (low == -1)
    return nullptr;
  if (debug_files_[low].name >= debug_names_section_->size)
    return nullptr;

  return debug_names_ + debug_files_[low].name;
}

template <typename SymbolType, typename DimType>
const char *
SmxV1Image::lookupFunction(const SymbolType *syms, uint32_t addr)
{
  const uint8_t *cursor = reinterpret_cast<const uint8_t *>(syms);
  const uint8_t *cursor_end = cursor + debug_symbols_section_->size;
  for (uint32_t i = 0; i < debug_info_->num_syms; i++) {
    if (cursor + sizeof(SymbolType) > cursor_end)
      break;

    const SymbolType *sym = reinterpret_cast<const SymbolType *>(cursor);
    if (sym->ident == sp::IDENT_FUNCTION &&
        sym->codestart <= addr &&
        sym->codeend > addr)
    {
      if (sym->name >= debug_names_section_->size)
        return nullptr;
      return debug_names_ + sym->name;
    }

    if (sym->dimcount > 0)
      cursor += sizeof(DimType) * sym->dimcount;
    cursor += sizeof(SymbolType);
  }
  return nullptr;
}

const char *
SmxV1Image::LookupFunction(uint32_t code_offset)
{
  if (debug_syms_) {
    return lookupFunction<sp_fdbg_symbol_t, sp_fdbg_arraydim_t>(
      debug_syms_, code_offset);
  }
  return lookupFunction<sp_u_fdbg_symbol_t, sp_u_fdbg_arraydim_t>(
      debug_syms_unpacked_, code_offset);
}

bool
SmxV1Image::LookupLine(uint32_t addr, uint32_t *line)
{
  int high = debug_lines_.length();
  int low = -1;

  while (high - low > 1) {
    int mid = (low + high) / 2;
    if (debug_lines_[mid].addr <= addr)
      low = mid;
    else
      high = mid;
  }

  if (low == -1)
    return false;

  // Since the CIP occurs BEFORE the line, we have to add one.
  *line = debug_lines_[low].line + 1;
  return true;
}
