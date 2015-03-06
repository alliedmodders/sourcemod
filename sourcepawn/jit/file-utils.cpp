// vim: set sts=2 ts=8 sw=2 tw=99 et:
// 
// Copyright (C) 2004-2015 AlliedModers LLC
//
// This file is part of SourcePawn. SourcePawn is licensed under the GNU
// General Public License, version 3.0 (GPL). If a copy of the GPL was not
// provided with this file, you can obtain it here:
//   http://www.gnu.org/licenses/gpl.html
//
#include <stdint.h>
#include <smx/smx-headers.h>
#include "file-utils.h"

using namespace sp;

FileType
sp::DetectFileType(FILE *fp)
{
  uint32_t magic = 0;
  if (fread(&magic, sizeof(uint32_t), 1, fp) != 1)
    return FileType::UNKNOWN;

  if (magic == SmxConsts::FILE_MAGIC)
    return FileType::SPFF;

  return FileType::UNKNOWN;
}

FileReader::FileReader(FILE *fp)
 : length_(0)
{
  if (fseek(fp, 0, SEEK_END) != 0)
    return;
  long size = ftell(fp);
  if (size < 0)
    return;
  if (fseek(fp, 0, SEEK_SET) != 0)
    return;

  ke::AutoArray<uint8_t> bytes(new uint8_t[size]);
  if (!bytes || fread(bytes, sizeof(uint8_t), size, fp) != (size_t)size)
    return;

  buffer_ = bytes.take();
  length_ = size;
}

FileReader::FileReader(ke::AutoArray<uint8_t> &buffer, size_t length)
 : buffer_(buffer.take()),
   length_(length)
{
}
