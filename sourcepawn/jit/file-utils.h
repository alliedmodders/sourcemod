// vim: set sts=2 ts=8 sw=2 tw=99 et:
// 
// Copyright (C) 2004-2015 AlliedModers LLC
//
// This file is part of SourcePawn. SourcePawn is licensed under the GNU
// General Public License, version 3.0 (GPL). If a copy of the GPL was not
// provided with this file, you can obtain it here:
//   http://www.gnu.org/licenses/gpl.html
//
#ifndef _include_sourcepawn_file_parser_h_
#define _include_sourcepawn_file_parser_h_

#include <stdio.h>
#include <am-utility.h>

namespace sp {

enum class FileType {
  UNKNOWN,
  AMX,
  AMXMODX,
  SPFF
};

FileType DetectFileType(FILE *fp);

class FileReader
{
 public:
  FileReader(FILE *fp);
  FileReader(ke::AutoArray<uint8_t> &buffer, size_t length);

  const uint8_t *buffer() const {
    return buffer_;
  }
  size_t length() const {
    return length_;
  }

 protected:
  ke::AutoArray<uint8_t> buffer_;
  size_t length_;
};

} // namespace sp

#endif // _include_sourcepawn_file_parser_h_
