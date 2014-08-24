// vim: set sts=2 ts=8 sw=2 tw=99 et:
// =============================================================================
// SourcePawn
// Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
// =============================================================================
// 
// This program is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License, version 3.0, as published by the
// Free Software Foundation.
// 
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
// details.
// 
// You should have received a copy of the GNU General Public License along with
// this program.  If not, see <http://www.gnu.org/licenses/>.
// 
// As a special exception, AlliedModders LLC gives you permission to link the
// code of this program (as well as its derivative works) to "Half-Life 2," the
// "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
// by the Valve Corporation.  You must obey the GNU General Public License in
// all respects for all other code used.  Additionally, AlliedModders LLC grants
// this exception to all derivative works.  AlliedModders LLC defines further
// exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
// or <http://www.sourcemod.net/license.php>.

#ifndef _INCLUDE_SPFILE_HEADERS_H
#define _INCLUDE_SPFILE_HEADERS_H

#include <stddef.h>
#include <stdint.h>

namespace sp {

struct SmxConsts
{
  // SourcePawn File Format (SPFF) magic number.
  static const uint32_t FILE_MAGIC = 0x53504646;

  // File format verison number.
  // 0x0101 - SourcePawn 1.0; initial version used by SourceMod 1.0.
  // 0x0102 - SourcePawn 1.1; used by SourceMod 1.1+.
  // 0x0200 - Used by spcomp2.
  //
  // The major version bits (8-15) indicate a product number. Consumers should
  // reject any version for a different product.
  //
  // The minor version bits (0-7) indicate a compatibility revision. Any minor
  // version higher than the current version should be rejected.
  static const uint16_t SP1_VERSION_1_0 = 0x0101;
  static const uint16_t SP1_VERSION_1_1 = 0x0102;
  static const uint16_t SP1_VERSION_1_7 = 0x0107;
  static const uint16_t SP1_VERSION_MIN = SP1_VERSION_1_0;
  static const uint16_t SP1_VERSION_MAX = SP1_VERSION_1_7;
  static const uint16_t SP2_VERSION_MIN = 0x0200;
  static const uint16_t SP2_VERSION_MAX = 0x0200;

  // Compression types.
  static const uint8_t FILE_COMPRESSION_NONE = 0;
  static const uint8_t FILE_COMPRESSION_GZ = 1;

  // SourcePawn 1.
  static const uint8_t CODE_VERSION_JIT_1_0 = 9;
  static const uint8_t CODE_VERSION_JIT_1_1 = 10;
  static const uint8_t CODE_VERSION_JIT_1_7 = 11;
  static const uint8_t CODE_VERSION_SP1_MIN = CODE_VERSION_JIT_1_0;
  static const uint8_t CODE_VERSION_SP1_MAX = CODE_VERSION_JIT_1_1;

  // For SP1 consumers, the container version may not be checked, but usually
  // the code version is. This constant allows newer containers to be rejected
  // in those applications.
  static const uint8_t CODE_VERSION_REJECT = 0x7F;
};

// These structures are byte-packed.
#if defined __GNUC__
# pragma pack(1)
#else
# pragma pack(push)
# pragma pack(1)
#endif

// The very first bytes in a .smx file. The .smx file format is a container for
// arbitrary sections, though to actually load a .smx file certain sections
// must be present. The on-disk format has four major regions, in order:
//   1. The header (sp_file_hdr_t).
//   2. The section list (sp_file_section_t).
//   3. The string table.
//   4. The section contents.
//
// Although any part of the file after the header can be compressed, by default
// only the section contents are, and the section list and string table are not.
typedef struct sp_file_hdr_s
{
  // Magic number and version number.
  uint32_t  magic;
  uint16_t  version;

  // disksize, imagesize, and dataoffs (at the end) describe a region of the
  // file that may be compressed. The region must occur after the initial
  // sp_file_hdr_t header. For SourceMod compatibility, the meanings are as
  // follows.
  //
  // Without compression:
  //   imagesize is the amount of bytes that must be read into memory to parse
  //             the SMX container, starting from the first byte of the file.
  //   disksize is undefined.
  //   dataoffs is undefined.
  //
  // With compression:
  //   dataoffs is an offset to the start of the compression region.
  //   disksize is the length of the compressed region, in bytes, plus dataoffs.
  //   imagesize is the size of the entire SMX container after decompression.
  //
  //   This means that at least |imagesize-dataoffs| must be allocated to
  //   decompress, and the compressed region's length is |datasize-dataoffs|.
  //
  // The compressed region should always be expanded "in-place". That is, to
  // parse the container, the compressed bytes should be replaced with
  // decompressed bytes.
  //
  // Note: This scheme may seem odd. It's a combination of historical debt and
  // previously unspecified behavior. The original .amx file format contained
  // an on-disk structure that supported an endian-agnostic variable-length 
  // encoding of its data section, and this structure was loaded directly into
  // memory and used as the VM context. AMX Mod X later developed a container
  // format called ".amxx" as a "universal binary" for 32-bit and 64-bit
  // plugins. This format dropped compact encoding, but supported gzip. The
  // disksize/imagesize oddness made its way to this file format. When .smx
  // was created for SourceMod, it persisted even though AMX was dropped
  // entirely. So it goes.
  uint8_t   compression;
  uint32_t  disksize;
  uint32_t  imagesize;

  // Number of named file sections.
  uint8_t   sections;

  // Offset to the string table. Each string is null-terminated. The string
  // table is only used for strings related to parsing the container itself.
  // For SourcePawn, a separate ".names" section exists for Pawn-specific data.
  uint32_t  stringtab;

  // Offset to where compression begins.
  uint32_t  dataoffs;
} sp_file_hdr_t;

// Each section is written immediately after the header.
typedef struct sp_file_section_s
{
  uint32_t  nameoffs;  /**< Offset into the string table. */
  uint32_t  dataoffs;  /**< Offset into the file for the section contents. */
  uint32_t  size;      /**< Size of this section's contents. */
} sp_file_section_t;

// Code section. This is used only in SP1, but is emitted by default for legacy
// systems which check |codeversion| but not the SMX file version.
typedef struct sp_file_code_s
{
  uint32_t  codesize;    /**< Size of the code section. */
  uint8_t   cellsize;    /**< Cellsize in bytes. */
  uint8_t   codeversion; /**< Version of opcodes supported. */
  uint16_t  flags;       /**< Flags. */
  uint32_t  main;        /**< Address to "main". */
  uint32_t  code;        /**< Offset to bytecode, relative to the start of this section. */
} sp_file_code_t;

#if defined __GNUC__
# pragma pack()
#else
# pragma pack(pop)
#endif

} // namespace sp

#endif //_INCLUDE_SPFILE_HEADERS_H
