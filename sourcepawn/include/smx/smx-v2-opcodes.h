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

#ifndef _include_spfile_headers_v2_opcodes_h_
#define _include_spfile_headers_v2_opcodes_h_

namespace sp {

// Opcode definitions:
//    number  symbol        name            pops  pushes    length    flags
OPDEF(0x00,   NOP,          "nop",          0,    0,        1,        0)

// Basic stack ops.
OPDEF(0x01,   POP,          "pop",          1,    0,        1,        0)
OPDEF(0x02,   PUSH_TRUE,    "push.true",    0,    1,        1,        0)
OPDEF(0x03,   PUSH_FALSE,   "push.false",   0,    1,        1,        0)
OPDEF(0x04,   PUSH_NULL,    "push.null",    0,    1,        1,        0)
OPDEF(0x05,   PUSH_I8,      "push.i8",      0,    1,        2,        0)
OPDEF(0x06,   PUSH_I32,     "push.i32",     0,    1,        5,        0)
OPDEF(0x07,   PUSH_I64,     "push.i64",     0,    1,        9,        0)
ODPEF(0x08,   PUSH_F32,     "push.f32",     0,    1,        5,        0)
OPDEF(0x09,   PUSH_F64,     "push.f64",     0,    1,        9,        0)
OPDEF(0x0A,   PUSH_STR,     "push.s",       0,    1,        5,        0)

// Get/set local variables.
OPDEF(0x10,   GETLOCAL,     "getlocal",     0,    1,        2,        0)
OPDEF(0x11,   GETLOCAL_W,   "getlocal.w",   0,    1,        3,        0)
OPDEF(0x12,   GETLOCAL_0,   "getlocal.0",   0,    1,        1,        0)
OPDEF(0x13,   GETLOCAL_1,   "getlocal.1",   0,    1,        1,        0)
OPDEF(0x14,   GETLOCAL_2,   "getlocal.2",   0,    1,        1,        0)
OPDEF(0x15,   GETLOCAL_3,   "getlocal.3",   0,    1,        1,        0)
OPDEF(0x18,   SETLOCAL,     "setlocal",     1,    0,        2,        0)
OPDEF(0x19,   SETLOCAL_W,   "setlocal.w",   1,    0,        3,        0)
OPDEF(0x1A,   SETLOCAL_0,   "setlocal.0",   1,    0,        1,        0)
OPDEF(0x1B,   SETLOCAL_1,   "setlocal.1",   1,    0,        1,        0)
OPDEF(0x1C,   SETLOCAL_2,   "setlocal.2",   1,    0,        1,        0)
OPDEF(0x1D,   SETLOCAL_3,   "setlocal.3",   1,    0,        1,        0)

// Local variable references.
OPDEF(0x1E,   REFLOCAL,     "reflocal",     0,    1,        3,        0)
OPDEF(0x1F,   LDREF,        "ldref",        1,    1,        1,        0)
OPDEF(0x20,   STREF,        "stref",        1,    0,        1,        0)

// Math operators.
OPDEF(0x40,   ADD,          "add",          2,    1,        1,        0)
OPDEF(0x41,   SUB,          "sub",          2,    1,        1,        0)
OPDEF(0x42,   MUL,          "mul",          2,    1,        1,        0)
OPDEF(0x43,   DIV,          "div",          2,    1,        1,        0)
OPDEF(0x44,   MOD,          "mod",          2,    1,        1,        0)
OPDEF(0x45,   NEG,          "neg",          1,    1,        1,        0)
OPDEF(0x46,   SHR,          "shr",          2,    1,        1,        0)
OPDEF(0x47,   USHR,         "ushr",         2,    1,        1,        0)
OPDEF(0x48,   SHL,          "shl",          2,    1,        1,        0)
OPDEF(0x49,   BITOR,        "bitor",        2,    1,        1,        0)
OPDEF(0x4A,   BITAND,       "bitand",       2,    1,        1,        0)
OPDEF(0x4B,   BITXOR,       "bitxor",       2,    1,        1,        0)
OPDEF(0x4C,   BITNOT,       "bitnot",       1,    1,        1,        0)

// Logical operators.
OPDEF(0x60,   AND,          "and",          2,    1,        1,        0)
OPDEF(0x61,   OR,           "or",           2,    1,        1,        0)
OPDEF(0x62,   NOT,          "not",          1,    1,        1,        0)
OPDEF(0x63,   LT,           "lt",           2,    1,        1,        0)
OPDEF(0x64,   LE,           "le",           2,    1,        1,        0)
OPDEF(0x65,   GT,           "gt",           2,    1,        1,        0)
OPDEF(0x66,   GE,           "ge",           2,    1,        1,        0)
OPDEF(0x67,   EQ,           "eq",           2,    1,        1,        0)
OPDEF(0x68,   NE,           "ne",           2,    1,        1,        0)

// Branching operators.
OPDEF(0x70,   JUMP,         "jump",         0,    0,        1,        0)
OPDEF(0x71,   JT,           "jt",           1,    0,        1,        0)
OPDEF(0x72,   JF,           "jf",           1,    0,        1,        0)
OPDEF(0x73,   JLT,          "jlt",          2,    0,        1,        0)
OPDEF(0x74,   JLE,          "jle",          2,    0,        1,        0)
OPDEF(0x75,   JGT,          "jgt",          2,    0,        1,        0)
OPDEF(0x76,   JGE,          "jge",          2,    0,        1,        0)
OPDEF(0x77,   JEQ,          "jeq",          2,    0,        1,        0)
OPDEF(0x78,   JNE,          "jne",          2,    0,        1,        0)

// Function calling.
//
// The low 4 bits of the index for OP_CALL and OP_CALL_A indicate the type
// of call:
//    0000 - 0x0 : Invoke a method constant such as a native or global function.
//                 Bits 4-31 are an index into the .methods table.
//
// Call is followed by a method identifier.
OPDEF(0x90,   CALL,         "call",         -1,   -1,       5,        0)
// Same as call.a but followed with a variadic argument count.
OPDEF(0x91,   CALL_A,       "call.a",       -1,   -1,       9,        0)

// Array has a typeid and pops a length off the stack.
OPDEF(0x96,   ARRAY,        "newarray",     1,    1,        5,        0)
// Fixedarray has an offset into the .types table.
OPDEF(0x97,   FIXEDARRAY,   "fixedarray",   0,    1,        5,        0)

// Advanced stack operations.
OPDEF(0xB0,   DUP,          "dup",          1,    2,        1,        0)


} // namespace sp

#endif // _include_spfile_headers_v2_opcodes_h_
