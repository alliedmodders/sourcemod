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

// Run-time type information is stored in the ".rtti" section. It is a
// binary blob and does not a fixed table encoding.
//
// When the term "variable length uint32" is used, it is a 32-bit unsigned
// integer encoded in a variable number of bytes. The ranges and masks for
// the encoding is as follows:
//   0x00000000-0x0000007F: 0x7F
//   0x00000080-0x00003FFF: 0xFF 0x7F
//   0x00004000-0x001FFFFF: 0xFF 0xFF 0x7F
//   0x01000000-0x0FFFFFFF: 0xFF 0xFF 0xFF 0x7F
//   0x10000000-0xFFFFFFFF: 0xFF 0xFF 0xFF 0xFF 0x0F
// Variable length uint32 is also referred to as "vuint32".
//
// When the term "typeid" is used, it is a uint32 (either variable length or
// not), of which bits 6 and 7 are a control code.
//    0?: The first byte of the type fits in bits 0-6. If the encoding of the
//        typeid is in a fixed-width context, this also guarantees the type
//        sequence fits into 4 bytes (including the control byte).
//    10: Bits 0-4 are an index into the .types table. Bit 5 indicates
//        variable-length encoding. The maximum number of bytes is 4. The
//        ranges and sequences are:
//          0x00000000-0x0000001F: 0x9F
//          0x00000020-0x00000FFF: 0xBF 0x7F
//          0x00001000-0x0007FFFF: 0xBF 0xFF 0x7F
//          0x00800000-0x001FFFFF: 0xBF 0xFF 0xFF 0x7F
//        The maximum encodable index is 2^25-1.
//    11: reserved

// TypeSpec is a variable-length encoding referenced anywhere that "typespec"
// is specified. TypeSpec signatures can reference other typespecs via typeids.
//
// Anytime a uint32 is referenced in a typespec, it is variable-length encoded.
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

  RESERVED_char32   = 0xE,    // Full unicode point.

  RESERVED_string   = 0x11,   // String reference.
  RESERVED_object   = 0x12,   // Opaque object reference.

  // This is allowed as a prefix before the following types:
  //   fixedarray
  //   array
  //
  constref          = 0x13,

  // Fixed arrays have their size as part of their type, are copied by-value,
  // and passed by-reference.
  //
  // Followed by:
  //     typeid   type     ; Type of innermost elements.
  //     uint8    rank     ; Number of dimensions
  //     uint32*  dims     ; One dimension for each rank.
  fixedarray        = 0x14,

  // A normal array does not have a fixed size and is exactly one dimension.
  //
  // Followed by:
  //     typeid   type     ; Type of array elements.
  array             = 0x15,

  // Indicates a method signature.
  method            = 0x30,

  // These bytes are reserved for signature blobs.
  RESERVED_start    = 0x70,
  RESERVED_end      = 0x7e,

  // Placeholder. New types must be < 0x7f to comply with typeid encoding.
  RESERVED_terminator = 0x7f
};

// A method signature is comprised of the following bytes:
// 
// method ::= prefix argcount params
// argcount ::= vuint32
// params ::= typeid* (refva typeid)?
enum class MethodSignature : uint8_t
{
  // Prefix byte (TypeSpec::method).
  prefix    = 0x30,

  // Indicates an old-style variadic parameter (i.e. any:... or ...), where
  // each parameter must be passed by-reference.
  refva     = 0x70
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
