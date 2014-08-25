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

#include <smx/smx-headers.h>

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
namespace TypeSpec
{
  // The void type is special in that it is not a storable type.
  static const uint8_t void_t            = 0x0;

  // Standard C++ datatypes.
  static const uint8_t boolean           = 0x1;
  static const uint8_t RESERVED_int8     = 0x2;
  static const uint8_t RESERVED_uint8    = 0x3;
  static const uint8_t RESERVED_int16    = 0x4;
  static const uint8_t RESERVED_uint16   = 0x5;
  static const uint8_t int32             = 0x6;
  static const uint8_t RESERVED_uint32   = 0x7;
  static const uint8_t RESERVED_int64    = 0x8;
  static const uint8_t RESERVED_uint64   = 0x9;

  // platform-width int and uint.
  static const uint8_t RESERVED_intptr   = 0xA;
  static const uint8_t RESERVED_uintptr  = 0xB;

  static const uint8_t float32           = 0xC;
  static const uint8_t RESERVED_float64  = 0xD;

  // The magic "char8" type in SP1.
  static const uint8_t char8             = 0xE;

  // Full unicode code point.
  static const uint8_t RESERVED_char32   = 0xF;

  // The magic-ish "any" type present in SP1.
  static const uint8_t any               = 0x10;

  static const uint8_t RESERVED_string   = 0x11;   // String reference.
  static const uint8_t RESERVED_object   = 0x12;   // Opaque object reference.

  // This is allowed as a prefix before the following types:
  //   fixedarray
  //   array
  //
  static const uint8_t constref          = 0x13;

  // Fixed arrays have their size as part of their type, are copied by-value,
  // and passed by-reference.
  //
  // Followed by:
  //     typeid   type     ; Type of innermost elements.
  //     uint8    rank     ; Number of dimensions
  //     uint32*  dims     ; One dimension for each rank.
  static const uint8_t fixedarray        = 0x14;

  // A normal array does not have a fixed size and is exactly one dimension.
  //
  // Followed by:
  //     typeid   type     ; Type of array elements.
  static const uint8_t array             = 0x15;

  // Indicates a method signature.
  static const uint8_t method            = 0x30;

  // These bytes are reserved for signature blobs.
  static const uint8_t RESERVED_start    = 0x70;
  static const uint8_t RESERVED_end      = 0x7e;

  // Placeholder. New types must be < 0x7f to comply with typeid encoding.
  static const uint8_t RESERVED_terminator = 0x7;
};

static const uint32_t kMaxMethodArgs = 64;

// A method signature is comprised of the following bytes:
// 
// method ::= prefix argcount return params
// argcount ::= vuint32
// return ::= typeid
// params ::= param-type* (refva param-type)?
// param-type ::= byref? typeid
//
// The maximum number of arguments for a method is currently 64.
namespace MethodSignature
{
  // Prefix byte (TypeSpec::method).
  static const uint8_t prefix = 0x30;

  // Indicates an old-style variadic parameter (i.e. any:... or ...), where
  // each parameter must be passed by-reference.
  static const uint8_t refva  = 0x70;

  // Indicates that the parameter is passed by-reference. Only allowed for
  // primitive types.
  static const uint8_t byref  = 0x71;
};

// Constants for smx_typedef_t::flags & KIND_MASK.
namespace TypeDefKind
{
  static const uint32_t Enum  = 0x1;
  static const uint32_t Alias = 0x2;
};

// Flags for smx_typedef_t.
namespace TypeDefFlags
{
  static const uint32_t KIND_MASK  = 0x0000000f;

  // Anonymous types are created by the compiler and are not visible. They may
  // be named, but those names are not importable.
  static const uint32_t ANONYMOUS  = 0x00000010;

  // For enums, this indicates that the special null type can be assigned.
  static const uint32_t NULLABLE   = 0x00000020;
};

// List of entries found in the .types table.
struct smx_typedef_t
{
  // TypeDefFlags attributes.
  uint32_t flags;

  // If anonymous, this may be 0. Otherwise, it must be an offset into the
  // .names section.
  uint32_t name;

  // Base type. For enums, this must be TypeSpec::int32 or another enum. For
  // aliases, it must must be a typeid describing the type being aliased.
  uint32_t base;

  // Reserved words.
  uint32_t reserved0;
  uint32_t reserved1;
  uint32_t reserved2;
  uint32_t reserved3;
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
