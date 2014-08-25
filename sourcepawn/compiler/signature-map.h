// vim: set sts=2 ts=8 sw=2 tw=99 et:
//
// Copyright (C) 2012-2014 AlliedModders LLC, David Anderson
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
#ifndef _include_sc_signature_pool_h_
#define _include_sc_signature_pool_h_

#include <am-hashmap.h>

// A lookup key for the hashtable.
struct ByteSequence {
  // The sequence of bytes to look for.
  void *bytes;

  // The length of the byte sequence.
  size_t length;

  // The master buffer to compare against.
  uint8_t *master;
};

// A stored key in the hashtable.
struct SignatureKey {
  // An offset into the master buffer.
  uint32_t offset;

  // Length of the byte sequence.
  size_t length;
};

struct SigPolicy {
  static uint32_t hash(const ByteSequence &seq) {
    return ke::FastHashCharSequence((const char *)seq.bytes, seq.length);
  }
  static bool matches(const ByteSequence &seq, const SignatureKey &key) {
    if (seq.length != key.length)
      return false;
    return memcmp(seq.bytes, seq.master + key.offset, seq.length) == 0;
  }
};

#endif // _include_sc_signature_pool_h_
