// vim: set sts=2 ts=8 sw=2 tw=99 et:
// =============================================================================
// SourcePawn
// Copyright (C) 2004-2014 AlliedModders LLC.  All rights reserved.
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

#ifndef _INCLUDE_SPFILE_HEADERS_v1_H
#define _INCLUDE_SPFILE_HEADERS_v1_H

#include <stddef.h>
#include <stdint.h>
#include <smx/smx-headers.h>

namespace sp {

// These structures are byte-packed.
#if defined __GNUC__
# pragma pack(1)
#else
# pragma pack(push)
# pragma pack(1)
#endif

// (Obsolete) debug-information is present.
static const uint16_t CODEFLAG_DEBUG = 0x1;

// The ".data" section.
typedef struct sp_file_data_s
{
  uint32_t  datasize; /**< Size of data section in memory */
  uint32_t  memsize;  /**< Total mem required (includes data) */
  uint32_t  data;     /**< File offset to data (helper) */
} sp_file_data_t;

// The ".publics" section.
typedef struct sp_file_publics_s
{
  uint32_t  address;  /**< Address relative to code section */
  uint32_t  name;     /**< Index into nametable */
} sp_file_publics_t;

// The ".natives" section.
typedef struct sp_file_natives_s
{
  uint32_t  name;     /**< Index into nametable */
} sp_file_natives_t;

// The ".pubvars" section.
typedef struct sp_file_pubvars_s
{
  uint32_t  address;  /**< Address relative to the DAT section */
  uint32_t  name;     /**< Index into nametable */
} sp_file_pubvars_t;

// The ".tags" section.
typedef struct sp_file_tag_s
{
  uint32_t  tag_id;   /**< Tag ID from compiler */
  uint32_t  name;     /**< Index into nametable */
} sp_file_tag_t;

// The ".dbg.info" section.
typedef struct sp_fdbg_info_s
{
  uint32_t  num_files;  /**< number of files */
  uint32_t  num_lines;  /**< number of lines */
  uint32_t  num_syms;   /**< number of symbols */
  uint32_t  num_arrays; /**< number of symbols which are arrays */
} sp_fdbg_info_t;

// The ".dbg.files" section.
typedef struct sp_fdbg_file_s
{
  uint32_t  addr;   /**< Address into code */
  uint32_t  name;   /**< Offset into debug nametable */
} sp_fdbg_file_t;

// The ".dbg.lines" section.
typedef struct sp_fdbg_line_s
{
  uint32_t  addr;   /**< Address into code */
  uint32_t  line;   /**< Line number */
} sp_fdbg_line_t;

static const uint8_t IDENT_VARIABLE = 1;    // Scalar local variable.
static const uint8_t IDENT_REFERENCE = 2;   // Reference to a scalar argument.
static const uint8_t IDENT_ARRAY = 3;       // Array with known dimensions.
static const uint8_t IDENT_REFARRAY = 4;    // Array with unknown dimensions.
static const uint8_t IDENT_FUNCTION = 9;    // Symbolic function.
static const uint8_t IDENT_VARARGS = 11;    // Variadic argument (...).

// The ".dbg.symbols" table.
typedef struct sp_fdbg_symbol_s
{
  int32_t   addr;       /**< Address rel to DAT or stack frame */
  int16_t   tagid;      /**< Tag id */
  uint32_t  codestart;  /**< Start scope validity in code */
  uint32_t  codeend;    /**< End scope validity in code */
  uint8_t   ident;      /**< Variable type */
  uint8_t   vclass;     /**< Scope class (local vs global) */
  uint16_t  dimcount;   /**< Dimension count (for arrays) */
  uint32_t  name;       /**< Offset into debug nametable */
} sp_fdbg_symbol_t;

// Occurs after an fdbg_symbol entry, for each dimension.
typedef struct sp_fdbg_arraydim_s
{
  int16_t   tagid;      /**< Tag id */
  uint32_t  size;       /**< Size of dimension */
} sp_fdbg_arraydim_t;

// Typedef for the ".names" section.
typedef char * sp_file_nametab_t;

// Header for the ".dbg.natives" section. It is followed by a number of
// sp_fdbg_native_t entries.
typedef struct sp_fdbg_ntvtab_s
{
  uint32_t num_entries;  /**< Number of entries. */
} sp_fdbg_ntvtab_t;

// An entry in the .dbg.natives section. Each is followed by an
// sp_fdbg_ntvarg_t for each argument.
typedef struct sp_fdbg_native_s
{
  uint32_t index;      /**< Native index in the plugin. */
  uint32_t name;      /**< Offset into debug nametable. */
  int16_t tagid;      /**< Return tag. */
  uint16_t nargs;      /**< Number of formal arguments. */
} sp_fdbg_native_t;

static const uint8_t IDENT_NATIVE_VARARGS = 1;

// Each entry is followed by an sp_fdbg_arraydim_t for each dimcount.
typedef struct sp_fdbg_ntvarg_s
{
  uint8_t   ident;    /**< Variable type */
  int16_t   tagid;    /**< Tag id */
  uint16_t  dimcount; /**< Dimension count (for arrays) */
  uint32_t  name;     /**< Offset into debug nametable */
} sp_fdbg_ntvarg_t;

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// DO NOT DEFINE NEW STRUCTURES BELOW.
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

#if defined __GNUC__
# pragma pack()    /* reset default packing */
#else
# pragma pack(pop) /* reset previous packing */
#endif

// The packing for files changed by accident for a small window of time, and
// some files may have unparsable debug information using sp_fdbg_arraydim_t or
// sp_fdbg_symbol_t.
// 
// If the file version is >= 0x0102, all structures will be packed. If the
// file version is < 0x0101, and the ".dbg.natives" table is present,
// all structures will be packed.
//
// If the version is 0x0101 and ".dbg.natives" is not present, then you must
// use the unpacked versions of those structures below. There is an extremely
// small chance, if the plugin used no natives, that the packing is
// indeterminate. This case is unlikely to be interesting, but if such a file
// exists, the only solution is to re-parse if the data looks corrupt.

typedef struct sp_u_fdbg_arraydim_s
{
  int16_t   tagid;    /**< Tag id */
  uint32_t  size;     /**< Size of dimension */
} sp_u_fdbg_arraydim_t;

typedef struct sp_u_fdbg_symbol_s
{
  int32_t   addr;       /**< Address rel to DAT or stack frame */
  int16_t   tagid;      /**< Tag id */
  uint32_t  codestart;  /**< Start scope validity in code */
  uint32_t  codeend;    /**< End scope validity in code */
  uint8_t   ident;      /**< Variable type */
  uint8_t   vclass;     /**< Scope class (local vs global) */
  uint16_t  dimcount;   /**< Dimension count (for arrays) */
  uint32_t  name;       /**< Offset into debug nametable */
} sp_u_fdbg_symbol_t;

} // namespace sp

#endif //_INCLUDE_SPFILE_HEADERS_v1_H

