// vim: set sts=2 ts=8 sw=2 tw=99 et:
// =============================================================================
// SourcePawn
// Copyright (C) 2004-2014 AlliedModders LLC.  All rights RESERVED.
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

#ifndef _INCLUDE_SPFILE_HEADERS_v2_H
#define _INCLUDE_SPFILE_HEADERS_v2_H

#include <smx-headers.h>

namespace sp {

// These structures are byte-packed.
#if defined __GNUC__
# pragma pack(1)
#else
# pragma pack(push)
# pragma pack(1)
#endif

// Tables present in smx v2:
//  .names            Blob of null-terminated strings.
//  .methods          Table of smx_method_t entries.
//  .pcode            Blob of smx_pcode_header_t entries.
//  .globals          Table of smx_global_var_t entries.
//  .types            Blob of type information.

// TypeSpec is a variable-length encoding referenced anywhere that "typespec"
// is specified.
//
// Whenever a TypeSpec is referenced from outside the .types table, specifically
// as a "typeid", it has an encoding to save space: If the id is < 0x20, then
// the id is a single-byte TypeSpec. Otherwise, the |id - 0x20| is an offset
// into the .types section.
enum class TypeSpec : uint8_t
{
  // The void type is special in that it is not a storable type.
  void_t            = 0x0,

  // Standard C++ datatypes.
  boolean           = 0x1,
  RESERVED_int8     = 0x2,
  RESERVED_uint8    = 0x3,
  RESERVED_int16    = 0x4,
  RESERVED_uint16   = 0x5,
  int32             = 0x6,
  RESERVED_uint32   = 0x7,
  RESERVED_int64    = 0x8,
  RESERVED_uint64   = 0x9,

  // platform-width int and uint.
  RESERVED_intptr   = 0xA,
  RESERVED_uintptr  = 0xB,

  float32           = 0xC,
  RESERVED_float64  = 0xD,

  // char8 is semantically an alias for int8.
  char8             = 0xE,

  RESERVED_char32   = 0xF,    // Full unicode point.

  RESERVED_string   = 0x16,   // String reference.
  RESERVED_object   = 0x17,   // Opaque object reference.

  // Followed by an int32 index into the .classes table.
  RESERVED_classdef  = 0x40,

  // Followed by an int32 index into the .structs table.
  RESERVED_structdef = 0x41,

  // Followed by:
  //      [int8 formalCount]  ; Number of formal parameters, counting varargs.
  //      [typespec *formals] ; Sequence of formal parameter specifications.
  //
  // When the type spec is for a method definition (for example, the .method
  // table), then each type in |formals| must begin with "named" (0x7e) or,
  // if it is the last formal, may be "variadic" (0x7a).
  method            = 0x50,

  // Fixed arrays have their size as part of their type, are copied by-value,
  // and passed by-reference.
  //
  // Followed by:
  //     typespec type     ; Type specification of innermost elements.
  //     uint8    rank     ; Number of dimensions
  //     int32*   dims     ; One dimension for each rank.
  fixedarray        = 0x60,

  // Arrays are true arrays. They are copied by-reference and passed by-
  // reference.
  //
  // Followed by:
  //     typesec  type     ; Type specification of innermost elements.
  //     uint8    rank     ; Number of dimensions.
  RESERVED_array    = 0x61,

  // Only legal as the initial byte or byte following "named" (0x7e) in a
  // parameter for a method signature.
  //
  // Followed by a typespec. Only typespecs < 0x60 ar elegal.
  byref             = 0x70,

  // Only legal as the initial byte or byte following "named" (0x7e) in a
  // parameter for a method signature.
  //
  // Followed by a typespec.
  variadic          = 0x7A,

  // Only legal as the initial byte in a parameter or local variable signature.
  //
  // Followed by:
  //     uint32    name    ; Index into the .names section.
  //     typespec  type    ; Type.
  named             = 0x7E,

  // For readability, this may be emitted at the end of typespec lists. It must
  // not be emitted at the end of nested typespecs.
  terminator        = 0x7F
};

// Flags for method definitions.
enum class MethodFlags : uint32_t
{
  STATIC            = 0x1,
  VARIADIC          = 0x2,
  PUBLIC            = 0x4,
  NATIVE            = 0x8,
  OPTIONAL          = 0x10
};

// Specifies an entry in the .methods table.
struct smx_method_t
{
  // Offset into the .names section.
  uint32_t name;

  MethodFlags flags;

  // Offset into .types, which must point at a TypeSpec::method.
  uint32_t typespec;

  // Offset into .pcode. If flags contains NATIVE, this must be 0.
  uint32_t address;
};

enum class GlobalVarFlags : uint32_t
{
  PUBLIC = 0x04,
  CONST  = 0x08
};

// Specifies an entry in the .globals table.
struct smx_global_var_t
{
  // Offset into the .names section.
  uint32_t name;

  GlobalVarFlags flags;

  // TypeId of the global.
  uint32_t type_id;
};

// Specifies the layout we expect at a valid pcode starting position.
struct smx_pcode_header_t
{
  // Must be 0.
  uint32_t reserved;

  // Number of local variables.
  uint16_t nlocals;

  // Maximum depth of the operand stack.
  uint16_t max_depth;

  // Pointer to .types section where local information begins.
  uint32_t local_specs;

  // Number of bytes of pcode in the method.
  uint32_t length;
};

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// DO NOT DEFINE NEW STRUCTURES BELOW.
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

#if defined __GNUC__
# pragma pack()
#else
# pragma pack(pop)
#endif

} // namespace sp

#endif // _INCLUDE_SPFILE_HEADERS_v2_H
