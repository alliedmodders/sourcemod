// vim: set sts=2 ts=8 sw=2 tw=99 et:
// 
// Copyright (C) 2004-2015 AlliedModers LLC
//
// This file is part of SourcePawn. SourcePawn is licensed under the GNU
// General Public License, version 3.0 (GPL). If a copy of the GPL was not
// provided with this file, you can obtain it here:
//   http://www.gnu.org/licenses/gpl.html
//
#ifndef _include_sourcepawn_smx_parser_h_
#define _include_sourcepawn_smx_parser_h_

#include <stdio.h>
#include <smx/smx-headers.h>
#include <smx/smx-v1.h>
#include <am-utility.h>
#include <am-string.h>
#include <am-vector.h>
#include "file-utils.h"
#include "legacy-image.h"

namespace sp {

class SmxV1Image
  : public FileReader,
    public LegacyImage
{
 public:
  SmxV1Image(FILE *fp);

  // This must be called to initialize the reader.
  bool validate();

  const sp_file_hdr_t *hdr() const {
    return hdr_;
  }

  const char *errorMessage() const {
    return error_.chars();
  }

 public:
  Code DescribeCode() const KE_OVERRIDE;
  Data DescribeData() const KE_OVERRIDE;
  size_t NumNatives() const KE_OVERRIDE;
  const char *GetNative(size_t index) const KE_OVERRIDE;
  bool FindNative(const char *name, size_t *indexp) const KE_OVERRIDE;
  size_t NumPublics() const KE_OVERRIDE;
  void GetPublic(size_t index, uint32_t *offsetp, const char **namep) const KE_OVERRIDE;
  bool FindPublic(const char *name, size_t *indexp) const KE_OVERRIDE;
  size_t NumPubvars() const KE_OVERRIDE;
  void GetPubvar(size_t index, uint32_t *offsetp, const char **namep) const KE_OVERRIDE;
  bool FindPubvar(const char *name, size_t *indexp) const KE_OVERRIDE;
  size_t HeapSize() const KE_OVERRIDE;
  size_t ImageSize() const KE_OVERRIDE;
  const char *LookupFile(uint32_t code_offset) KE_OVERRIDE;
  const char *LookupFunction(uint32_t code_offset) KE_OVERRIDE;
  bool LookupLine(uint32_t code_offset, uint32_t *line) KE_OVERRIDE;

 private:
   struct Section
   {
     const char *name;
     uint32_t dataoffs;
     uint32_t size;
   };
  const Section *findSection(const char *name);

 public:
  template <typename T>
  class Blob
  {
   public:
    Blob()
     : header_(nullptr),
       section_(nullptr),
       blob_(nullptr),
       length_(0)
    {}
    Blob(const Section *header,
         const T *section,
         const uint8_t *blob,
         size_t length)
     : header_(header),
       section_(section),
       blob_(blob),
       length_(length)
    {}

    size_t size() const {
      return section_->size;
    }
    const T * operator ->() const {
      return section_;
    }
    const uint8_t *blob() const {
      return blob_;
    }
    size_t length() const {
      return length_;
    }
    bool exists() const {
      return !!header_;
    }

   private:
    const Section *header_;
    const T *section_;
    const uint8_t *blob_;
    size_t length_;
  };

  template <typename T>
  class List
  {
   public:
    List()
     : section_(nullptr),
       length_(0)
    {}
    List(const T *section, size_t length)
     : section_(section),
       length_(length)
    {}

    size_t length() const {
      return length_;
    }
    const T &operator[](size_t index) const {
      assert(index < length());
      return section_[index];
    }
    bool exists() const {
      return !!section_;
    }

   private:
    const T *section_;
    size_t length_;
  };

 public:
  const Blob<sp_file_code_t> &code() const {
    return code_;
  }
  const Blob<sp_file_data_t> &data() const {
    return data_;
  }
  const List<sp_file_publics_t> &publics() const {
    return publics_;
  }
  const List<sp_file_natives_t> &natives() const {
    return natives_;
  }
  const List<sp_file_pubvars_t> &pubvars() const {
    return pubvars_;
  }

 protected:
  bool error(const char *msg) {
    error_ = msg;
    return false;
  }
  bool validateName(size_t offset);
  bool validateSection(const Section *section);
  bool validateCode();
  bool validateData();
  bool validatePublics();
  bool validatePubvars();
  bool validateNatives();
  bool validateDebugInfo();

 private:
  template <typename SymbolType, typename DimType>
  const char *lookupFunction(const SymbolType *syms, uint32_t addr);

 private:
  sp_file_hdr_t *hdr_;
  ke::AString error_;
  const char *header_strings_;
  ke::Vector<Section> sections_;

  const Section *names_section_;
  const char *names_;

  Blob<sp_file_code_t> code_;
  Blob<sp_file_data_t> data_;
  List<sp_file_publics_t> publics_;
  List<sp_file_natives_t> natives_;
  List<sp_file_pubvars_t> pubvars_;

  const Section *debug_names_section_;
  const char *debug_names_;
  const sp_fdbg_info_t *debug_info_;
  List<sp_fdbg_file_t> debug_files_;
  List<sp_fdbg_line_t> debug_lines_;
  const Section *debug_symbols_section_;
  const sp_fdbg_symbol_t *debug_syms_;
  const sp_u_fdbg_symbol_t *debug_syms_unpacked_;
};

} // namespace sp

#endif // _include_sourcepawn_smx_parser_h_
