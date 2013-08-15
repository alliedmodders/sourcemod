/**
 * vim: set ts=8 sts=2 sw=2 tw=99 et:
 * =============================================================================
 * SourcePawn JIT SDK
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */
#ifndef _include_sourcepawn_assembler_x86_h__
#define _include_sourcepawn_assembler_x86_h__

#include <assembler.h>
#include <ke_vector.h>
#include <string.h>

struct Register
{
  const char *name() const {
    static const char *names[] = {
      "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi"
    };
    return names[code];
  }

  int code;

  bool operator == (const Register &other) const {
    return code == other.code;
  }
  bool operator != (const Register &other) const {
    return code != other.code;
  }
};

// X86 has an ancient FPU (called x87) which has a stack of registers
// numbered st0 through st7.
struct FpuRegister
{
  const char *name() const {
    static const char *names[] = {
      "st0", "st1", "st2", "st3", "st4", "st5", "st6", "st7"
    };
    return names[code];
  }

  int code;

  bool operator == (const FpuRegister &other) const {
    return code == other.code;
  }
  bool operator != (const FpuRegister &other) const {
    return code != other.code;
  }
};

struct FloatRegister
{
  const char *name() const {
    static const char *names[] = {
      "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7"
    };
    return names[code];
  }

  int code;

  bool operator == (const FloatRegister &other) const {
    return code == other.code;
  }
  bool operator != (const FloatRegister &other) const {
    return code != other.code;
  }
};

struct CPUFeatures
{
  CPUFeatures()
  {
    memset(this, 0, sizeof(*this));
  }

  bool fpu;
  bool mmx;
  bool sse;
  bool sse2;
  bool sse3;
  bool ssse3;
  bool sse4_1;
  bool sse4_2;
  bool avx;
  bool avx2;
};

const Register eax = { 0 };
const Register ecx = { 1 };
const Register edx = { 2 };
const Register ebx = { 3 };
const Register esp = { 4 };
const Register ebp = { 5 };
const Register esi = { 6 };
const Register edi = { 7 };

const Register r8_al = { 0 };
const Register r8_cl = { 1 };
const Register r8_dl = { 2 };
const Register r8_bl = { 3 };
const Register r8_ah = { 4 };
const Register r8_ch = { 5 };
const Register r8_dh = { 6 };
const Register r8_bh = { 7 };

const FpuRegister st0 = { 0 };
const FpuRegister st1 = { 1 };
const FpuRegister st2 = { 2 };
const FpuRegister st3 = { 3 };
const FpuRegister st4 = { 4 };
const FpuRegister st5 = { 5 };
const FpuRegister st6 = { 6 };
const FpuRegister st7 = { 7 };

const FloatRegister xmm0 = { 0 };
const FloatRegister xmm1 = { 1 };
const FloatRegister xmm2 = { 2 };
const FloatRegister xmm3 = { 3 };
const FloatRegister xmm4 = { 4 };
const FloatRegister xmm5 = { 5 };
const FloatRegister xmm6 = { 6 };
const FloatRegister xmm7 = { 7 };

static const uint8_t kModeDisp0 = 0;
static const uint8_t kModeDisp8 = 1;
static const uint8_t kModeDisp32 = 2;
static const uint8_t kModeReg = 3;
static const uint8_t kNoIndex = 4;
static const uint8_t kSIB = 4;
static const uint8_t kRIP = 5;

enum ConditionCode {
  overflow,
  no_overflow,
  below,
  not_below,
  equal,
  not_equal,
  not_above,
  above,
  negative,
  not_negative,
  even_parity,
  odd_parity,
  less,
  not_less,
  not_greater,
  greater,

  zero = equal,
  not_zero = not_equal,
  less_equal = not_greater,
  greater_equal = not_less
};

enum Scale {
  NoScale,
  ScaleTwo,
  ScaleFour,
  ScaleEight,
  ScalePointer = ScaleFour
};

struct Operand
{
  friend class AssemblerX86;

 public:
  Operand(Register reg, int32_t disp) {
    if (reg == esp) {
      // If the reg is esp, we need a SIB encoding.
      if (disp == 0)
        sib_disp0(NoScale, kNoIndex, reg.code);
      else if (disp >= SCHAR_MIN && disp <= SCHAR_MAX)
        sib_disp8(NoScale, kNoIndex, reg.code, disp);
      else
        sib_disp32(NoScale, kNoIndex, reg.code, disp);
    } else if (disp == 0 && reg != ebp) {
      // note, [ebp+0] is disp32/rip
      modrm_disp0(reg.code);
    } else if (disp >= SCHAR_MIN && disp <= SCHAR_MAX) {
      modrm_disp8(reg.code, disp);
    } else {
      modrm_disp32(reg.code, disp);
    }
  }

  Operand(Register base, Scale scale, int32_t disp = 0) {
    if (disp == 0 && base != ebp)
      sib_disp0(scale, kNoIndex, base.code);
    else if (disp >= SCHAR_MIN && disp <= SCHAR_MAX)
      sib_disp8(scale, kNoIndex, base.code, disp);
    else
      sib_disp32(scale, kNoIndex, base.code, disp);
  }

  Operand(Register base, Register index, Scale scale, int32_t disp = 0) {
    assert(index.code != kNoIndex);
    if (disp == 0 && base != ebp)
      sib_disp0(scale, index.code, base.code);
    else if (disp >= SCHAR_MIN && disp <= SCHAR_MAX)
      sib_disp8(scale, index.code, base.code, disp);
    else
      sib_disp32(scale, index.code, base.code, disp);
  }

  explicit Operand(ExternalAddress address) {
    modrm(kModeDisp0, kRIP);
    *reinterpret_cast<const void **>(bytes_ + 1) = address.address();
  }

  bool isRegister() const {
    return mode() == kModeReg;
  }
  bool isRegister(Register r) const {
    return mode() == kModeReg && rm() == r.code;
  }
  int registerCode() const {
    return rm();
  }

  uint8_t getByte(size_t index) const {
    assert(index < length());
    return bytes_[index];
  }

  size_t length() const {
    if (mode() == kModeDisp0 && rm() == kRIP)
      return 5;
    size_t sib = (mode() != kModeReg && rm() == kSIB);
    if (mode() == kModeDisp32)
      return 5 + sib;
    if (mode() == kModeDisp8)
      return 2 + sib;
    return 1 + sib;
  }

 private:
  explicit Operand(Register reg) {
    modrm(kModeReg, reg.code);
  }

  void modrm(uint8_t mode, uint8_t rm) {
    assert(mode <= 3);
    assert(rm <= 7);
    bytes_[0] = (mode << 6) | rm;
  }
  void modrm_disp0(uint8_t rm) {
    modrm(kModeDisp0, rm);
  }
  void modrm_disp8(uint8_t rm, int8_t disp) {
    modrm(kModeDisp8, rm);
    bytes_[1] = disp;
  }
  void modrm_disp32(uint8_t rm, int32_t disp) {
    modrm(kModeDisp32, rm);
    *reinterpret_cast<int32_t *>(bytes_ + 1) = disp;
  }
  void sib(uint8_t mode, Scale scale, uint8_t index, uint8_t base) {
    modrm(mode, kSIB);

    assert(scale <= 3);
    assert(index <= 7);
    assert(base <= 7);
    bytes_[1] = (uint8_t(scale) << 6) | (index << 3) | base;
  }
  void sib_disp0(Scale scale, uint8_t index, uint8_t base) {
    sib(kModeDisp0, scale, index, base);
  }
  void sib_disp8(Scale scale, uint8_t index, uint8_t base, int8_t disp) {
    sib(kModeDisp8, scale, index, base);
    bytes_[2] = disp;
  }
  void sib_disp32(Scale scale, uint8_t index, uint8_t base, int32_t disp) {
    sib(kModeDisp32, scale, index, base);
    *reinterpret_cast<int32_t *>(bytes_ + 2) = disp;
  }

 private:
  uint8_t rm() const {
    return bytes_[0] & 7;
  }
  uint8_t mode() const {
    return bytes_[0] >> 6;
  }

 private:
  uint8_t bytes_[6];
};

class AssemblerX86 : public Assembler
{
 private:
  // List of processor features; to be used, this must be filled in at
  // startup.
  static CPUFeatures X86Features; 

 public:
  static void SetFeatures(const CPUFeatures &features) {
    X86Features = features;
  }
  static const CPUFeatures &Features() {
    return X86Features;
  }

  void movl(Register dest, Register src) {
    emit1(0x89, src.code, dest.code);
  }
  void movl(Register dest, const Operand &src) {
    emit1(0x8b, dest.code, src);
  }
  void movl(const Operand &dest, Register src) {
    emit1(0x89, src.code, dest);
  }
  void movl(Register dest, int32_t imm) {
    emit1(0xb8 + dest.code);
    writeInt32(imm);
  }
  void movl(const Operand &dest, int32_t imm) {
    if (dest.isRegister())
      emit1(0xb8 + dest.registerCode());
    else
      emit1(0xc7, 0, dest);
    writeInt32(imm);
  }
  void movw(const Operand &dest, Register src) {
    emit1(0x89, src.code, dest);
  }
  void movw(Register dest, const Operand &src) {
    emit1(0x8b, dest.code, src);
  }
  void movb(const Operand &dest, Register src) {
    emit1(0x88, src.code, dest);
  }
  void movb(Register dest, const Operand &src) {
    emit1(0x8a, dest.code, src);
  }
  void movzxb(Register dest, const Operand &src) {
    emit2(0x0f, 0xb6, dest.code, src);
  }
  void movzxb(Register dest, const Register src) {
    emit2(0x0f, 0xb6, dest.code, src.code);
  }
  void movzxw(Register dest, const Operand &src) {
    emit2(0x0f, 0xb7, dest.code, src);
  }
  void movzxw(Register dest, const Register src) {
    emit2(0x0f, 0xb7, dest.code, src.code);
  }

  void lea(Register dest, const Operand &src) {
    emit1(0x8d, dest.code, src);
  }

  void xchgl(Register dest, Register src) {
    if (src == eax)
      emit1(0x90 + dest.code);
    else if (dest == eax)
      emit1(0x90 + src.code);
    else
      emit1(0x87, src.code, dest.code);
  }

  void shll_cl(Register dest) {
    shift_cl(dest.code, 4);
  }
  void shll(Register dest, uint8_t imm) {
    shift_imm(dest.code, 4, imm);
  }
  void shll(const Operand &dest, uint8_t imm) {
    shift_imm(dest, 4, imm);
  }
  void shrl_cl(Register dest) {
    shift_cl(dest.code, 5);
  }
  void shrl(Register dest, uint8_t imm) {
    shift_imm(dest.code, 5, imm);
  }
  void shrl(const Operand &dest, uint8_t imm) {
    shift_imm(dest, 5, imm);
  }
  void sarl_cl(Register dest) {
    shift_cl(dest.code, 7);
  }
  void sarl(Register dest, uint8_t imm) {
    shift_imm(dest.code, 7, imm);
  }
  void sarl(const Operand &dest, uint8_t imm) {
    shift_imm(dest, 7, imm);
  }

  void cmpl(Register left, int32_t imm) {
    alu_imm(7, imm, Operand(left));
  }
  void cmpl(const Operand &left, int32_t imm) {
    alu_imm(7, imm, left);
  }
  void cmpl(Register left, Register right) {
    emit1(0x39, right.code, left.code);
  }
  void cmpl(const Operand &left, Register right) {
    emit1(0x39, right.code, left);
  }
  void cmpl(Register left, const Operand &right) {
    emit1(0x3b, left.code, right);
  }
  void andl(Register dest, int32_t imm) {
    alu_imm(4, imm, Operand(dest));
  }
  void andl(const Operand &dest, int32_t imm) {
    alu_imm(4, imm, dest);
  }
  void andl(Register dest, Register src) {
    emit1(0x21, src.code, dest.code);
  }
  void andl(const Operand &dest, Register src) {
    emit1(0x21, src.code, dest);
  }
  void andl(Register dest, const Operand &src) {
    emit1(0x23, dest.code, src);
  }
  void orl(Register dest, Register src) {
    emit1(0x09, src.code, dest.code);
  }
  void orl(const Operand &dest, Register src) {
    emit1(0x09, src.code, dest);
  }
  void orl(Register dest, const Operand &src) {
    emit1(0x0b, dest.code, src);
  }
  void xorl(Register dest, Register src) {
    emit1(0x31, src.code, dest.code);
  }
  void xorl(const Operand &dest, Register src) {
    emit1(0x31, src.code, dest);
  }
  void xorl(Register dest, const Operand &src) {
    emit1(0x33, dest.code, src);
  }

  void subl(Register dest, Register src) {
    emit1(0x29, src.code, dest.code);
  }
  void subl(const Operand &dest, Register src) {
    emit1(0x29, src.code, dest);
  }
  void subl(Register dest, const Operand &src) {
    emit1(0x2b, dest.code, src);
  }
  void subl(Register dest, int32_t imm) {
    alu_imm(5, imm, Operand(dest));
  }
  void subl(const Operand &dest, int32_t imm) {
    alu_imm(5, imm, dest);
  }
  void addl(Register dest, Register src) {
    emit1(0x01, src.code, dest.code);
  }
  void addl(const Operand &dest, Register src) {
    emit1(0x01, src.code, dest);
  }
  void addl(Register dest, const Operand &src) {
    emit1(0x03, dest.code, src);
  }
  void addl(Register dest, int32_t imm) {
    alu_imm(0, imm, Operand(dest));
  }
  void addl(const Operand &dest, int32_t imm) {
    alu_imm(0, imm, dest);
  }

  void imull(Register dest, const Operand &src) {
    emit2(0x0f, 0xaf, dest.code, src);
  }
  void imull(Register dest, Register src) {
    emit2(0x0f, 0xaf, dest.code, src.code);
  }
  void imull(Register dest, const Operand &src, int32_t imm) {
    if (imm >= SCHAR_MIN && imm <= SCHAR_MAX) {
      emit1(0x6b, dest.code, src);
      *pos_++ = imm;
    } else {
      emit1(0x69, dest.code, src);
      writeInt32(imm);
    }
  }
  void imull(Register dest, Register src, int32_t imm) {
    imull(dest, Operand(src), imm);
  }

  void testl(const Operand &op1, Register op2) {
    emit1(0x85, op2.code, op1);
  }
  void testl(Register op1, Register op2) {
    emit1(0x85, op2.code, op1.code);
  }
  void set(ConditionCode cc, const Operand &dest) {
    emit2(0x0f, 0x90 + uint8_t(cc), 0, dest);
  }
  void set(ConditionCode cc, Register dest) {
    emit2(0x0f, 0x90 + uint8_t(cc), 0, dest.code);
  }
  void negl(Register srcdest) {
    emit1(0xf7, 3, srcdest.code);
  }
  void negl(const Operand &srcdest) {
    emit1(0xf7, 3, srcdest);
  }
  void notl(Register srcdest) {
    emit1(0xf7, 2, srcdest.code);
  }
  void notl(const Operand &srcdest) {
    emit1(0xf7, 2, srcdest);
  }
  void idivl(Register dividend) {
    emit1(0xf7, 7, dividend.code);
  }
  void idivl(const Operand &dividend) {
    emit1(0xf7, 7, dividend);
  }

  void ret() {
    emit1(0xc3);
  }
  void cld() {
    emit1(0xfc);
  }
  void push(Register reg) {
    emit1(0x50 + reg.code);
  }
  void push(const Operand &src) {
    if (src.isRegister())
      emit1(0x50 + src.registerCode());
    else
      emit1(0xff, 6, src);
  }
  void push(int32_t imm) {
    emit1(0x68);
    writeInt32(imm);
  }
  void pop(Register reg) {
    emit1(0x58 + reg.code);
  }
  void pop(const Operand &src) {
    if (src.isRegister())
      emit1(0x58 + src.registerCode());
    else
      emit1(0x8f, 0, src);
  }

  void rep_movsb() {
    emit2(0xf3, 0xa4);
  }
  void rep_movsd() {
    emit2(0xf3, 0xa5);
  }
  void rep_stosd() {
    emit2(0xf3, 0xab);
  }
  void breakpoint() {
    emit1(0xcc);
  }

  void fld32(const Operand &src) {
    emit1(0xd9, 0, src);
  }
  void fild32(const Operand &src) {
    emit1(0xdb, 0, src);
  }
  void fistp32(const Operand &dest) {
    emit1(0xdb, 3, dest);
  }
  void fadd32(const Operand &src) {
    emit1(0xd8, 0, src);
  }
  void fsub32(const Operand &src) {
    emit1(0xd8, 4, src);
  }
  void fmul32(const Operand &src) {
    emit1(0xd8, 1, src);
  }
  void fdiv32(const Operand &src) {
    emit1(0xd8, 6, src);
  }
  void fstp32(const Operand &dest) {
    emit1(0xd9, 3, dest);
  }
  void fstp(FpuRegister src) {
    emit2(0xdd, 0xd8 + src.code);
  }
  void fldcw(const Operand &src) {
    emit1(0xd9, 5, src);
  }
  void fstcw(const Operand &dest) {
    emit2(0x9b, 0xd9, 7, dest);
  }
  void fsubr32(const Operand &src) {
    emit1(0xd8, 5, src);
  }

  // Compare st0 with stN.
  void fucomip(FpuRegister other) {
    emit2(0xdf, 0xe8 + other.code);
  }

  // At least one argument of these forms must be st0.
  void fadd32(FpuRegister dest, FpuRegister src) {
    assert(dest == st0 || src == st0);
    if (dest == st0)
      emit2(0xd8, 0xc0 + dest.code);
    else
      emit2(0xdc, 0xc0 + src.code);
  }

  void jmp32(Label *dest) {
    emit1(0xe9);
    emitJumpTarget(dest);
  }
  void jmp(Label *dest) {
    int8_t d8;
    if (canEmitSmallJump(dest, &d8)) {
      emit2(0xeb, d8);
    } else {
      emit1(0xe9);
      emitJumpTarget(dest);
    }
  }
  void jmp(Register target) {
    emit1(0xff, 4, target.code);
  }
  void jmp(const Operand &target) {
    emit1(0xff, 4, target);
  }
  void j32(ConditionCode cc, Label *dest) {
    emit2(0x0f, 0x80 + uint8_t(cc));
    emitJumpTarget(dest);
  }
  void j(ConditionCode cc, Label *dest) {
    int8_t d8;
    if (canEmitSmallJump(dest, &d8)) {
      emit2(0x70 + uint8_t(cc), d8);
    } else {
      emit2(0x0f, 0x80 + uint8_t(cc));
      emitJumpTarget(dest);
    }
  }
  void call(Label *dest) {
    emit1(0xe8);
    emitJumpTarget(dest);
  }
  void bind(Label *target) {
    if (outOfMemory()) {
      // If we ran out of memory, the code stream is potentially invalid and
      // we cannot use the embedded linked list.
      target->bind(pc());
      return;
    }

    assert(!target->bound());
    uint32_t status = target->status();
    while (Label::More(status)) {
      // Grab the offset. It should be at least a 1byte op + rel32.
      uint32_t offset = Label::ToOffset(status);
      assert(offset >= 5);

      // Grab the delta from target to pc.
      ptrdiff_t delta = pos_ - (buffer() + offset);
      assert(delta >= INT_MIN && delta <= INT_MAX);

      int32_t *p = reinterpret_cast<int32_t *>(buffer() + offset - 4);
      status = *p;
      *p = delta;
    }
    target->bind(pc());
  }

  void bind(DataLabel *address) {
    if (outOfMemory())
      return;
    if (address->used()) {
      uint32_t offset = DataLabel::ToOffset(address->status());
      *reinterpret_cast<int32_t *>(buffer() + offset - 4) = position() - int32_t(offset);
    }
    address->bind(pc());
  }
  void movl(Register dest, DataLabel *src) {
    emit1(0xb8 + dest.code);
    if (src->bound()) {
      writeInt32(int32_t(src->offset()) - (position() + 4));
    } else {
      writeInt32(0xabcdef0);
      src->use(pc());
    }
    if (!local_refs_.append(pc()))
      outOfMemory_ = true;
  }
  void emit_absolute_address(Label *address) {
    if (address->bound())
      writeUint32(int32_t(address->offset()) - (position() + 4));
    else
      writeUint32(address->addPending(position() + 4));
    if (!local_refs_.append(pc()))
      outOfMemory_ = true;
  }

  void call(Register target) {
    emit1(0xff, 2, target.code);
  }
  void call(const Operand &target) {
    emit1(0xff, 2, target);
  }
  void call(ExternalAddress address) {
    emit1(0xe8);
    writeInt32(address.value());
    if (!external_refs_.append(pc()))
      outOfMemory_ = true;
  }
  void jmp(ExternalAddress address) {
    assert(sizeof(address) == sizeof(int32_t));
    emit1(0xe9);
    writeInt32(address.value());
    if (!external_refs_.append(pc()))
      outOfMemory_ = true;
  }

  void cpuid() {
    emit2(0x0f, 0xa2);
  }


  // SSE operations can only be used if the feature detection function has
  // been run *and* detected the appropriate level of functionality.
  void movss(FloatRegister dest, const Operand &src) {
    assert(Features().sse);
    emit3(0xf3, 0x0f, 0x10, dest.code, src);
  }
  void cvttss2si(Register dest, Register src) {
    assert(Features().sse);
    emit3(0xf3, 0x0f, 0x2c, dest.code, src.code);
  }
  void cvttss2si(Register dest, const Operand &src) {
    assert(Features().sse);
    emit3(0xf3, 0x0f, 0x2c, dest.code, src);
  }
  void cvtss2si(Register dest, Register src) {
    assert(Features().sse);
    emit3(0xf3, 0x0f, 0x2d, dest.code, src.code);
  }
  void cvtss2si(Register dest, const Operand &src) {
    assert(Features().sse);
    emit3(0xf3, 0x0f, 0x2d, dest.code, src);
  }
  void cvtsi2ss(FloatRegister dest, Register src) {
    assert(Features().sse);
    emit3(0xf3, 0x0f, 0x2a, dest.code, src.code);
  }
  void cvtsi2ss(FloatRegister dest, const Operand &src) {
    assert(Features().sse);
    emit3(0xf3, 0x0f, 0x2a, dest.code, src);
  }
  void addss(FloatRegister dest, const Operand &src) {
    assert(Features().sse);
    emit3(0xf3, 0x0f, 0x58, dest.code, src);
  }
  void subss(FloatRegister dest, const Operand &src) {
    assert(Features().sse);
    emit3(0xf3, 0x0f, 0x5c, dest.code, src);
  }
  void mulss(FloatRegister dest, const Operand &src) {
    assert(Features().sse);
    emit3(0xf3, 0x0f, 0x59, dest.code, src);
  }
  void divss(FloatRegister dest, const Operand &src) {
    assert(Features().sse);
    emit3(0xf3, 0x0f, 0x5e, dest.code, src);
  }
  void ucomiss(FloatRegister left, Register right) {
    emit2(0x0f, 0x2e, left.code, right.code);
  }
  void ucomiss(FloatRegister left, const Operand &right) {
    emit2(0x0f, 0x2e, left.code, right);
  }

  // SSE2-only instructions.
  void movd(Register dest, FloatRegister src) {
    assert(Features().sse2);
    emit3(0x66, 0x0f, 0x7e, dest.code, src.code);
  }
  void movd(Register dest, const Operand &src) {
    assert(Features().sse2);
    emit3(0x66, 0x0f, 0x7e, dest.code, src);
  }

  static void PatchRel32Absolute(uint8_t *ip, void *ptr) {
    int32_t delta = uint32_t(ptr) - uint32_t(ip);
    *reinterpret_cast<int32_t *>(ip - 4) = delta;
  }

  void emitToExecutableMemory(void *code) {
    assert(!outOfMemory());

    // Relocate anything we emitted as rel32 with an external pointer.
    uint8_t *base = reinterpret_cast<uint8_t *>(code);
    memcpy(base, buffer(), length());
    for (size_t i = 0; i < external_refs_.length(); i++) {
      size_t offset = external_refs_[i];
      PatchRel32Absolute(base + offset, *reinterpret_cast<void **>(base + offset - 4));
    }

    // Relocate everything we emitted as an abs32 with an internal offset. Note
    // that in the code stream, we use relative offsets so we can use both Label
    // and DataLabel.
    for (size_t i = 0; i < local_refs_.length(); i++) {
      size_t offset = local_refs_[i];
      int32_t delta = *reinterpret_cast<int32_t *>(base + offset - 4);
      *reinterpret_cast<void **>(base + offset - 4) = base + offset + delta;
    }
  }

  void align(uint32_t bytes) {
    int32_t delta = (pc() & ~(bytes - 1)) + bytes - pc();
    for (int32_t i = 0; i < delta; i++)
      emit1(0xcc);
  }

 private:
  bool canEmitSmallJump(Label *dest, int8_t *deltap) {
    if (!dest->bound())
      return false;

    // All small jumps are assumed to be 2 bytes.
    ptrdiff_t delta = ptrdiff_t(dest->offset()) - (position() + 2);
    if (delta < SCHAR_MIN || delta > SCHAR_MAX)
      return false;
    *deltap = delta;
    return true;
  }
  void emitJumpTarget(Label *dest) {
    if (dest->bound()) {
      ptrdiff_t delta = ptrdiff_t(dest->offset()) - (position() + 4);
      assert(delta >= INT_MIN && delta <= INT_MAX);
      writeInt32(delta);
    } else {
      writeUint32(dest->addPending(position() + 4));
    }
  }

  void emit(uint8_t reg, const Operand &operand) {
    *pos_++ = operand.getByte(0) | (reg << 3);
    size_t length = operand.length();
    for (size_t i = 1; i < length; i++)
      *pos_++ = operand.getByte(i);
  }

  void emit1(uint8_t opcode) {
    ensureSpace();
    *pos_++ = opcode;
  }
  void emit1(uint8_t opcode, uint8_t reg, uint8_t opreg) {
    ensureSpace();
    assert(reg <= 7);
    assert(opreg <= 7);
    *pos_++ = opcode;
    *pos_++ = (kModeReg << 6) | (reg << 3) | opreg;
  }
  void emit1(uint8_t opcode, uint8_t reg, const Operand &operand) {
    ensureSpace();
    assert(reg <= 7);
    *pos_++ = opcode;
    emit(reg, operand);
  }

  void emit2(uint8_t prefix, uint8_t opcode) {
    ensureSpace();
    *pos_++ = prefix;
    *pos_++ = opcode;
  }
  void emit2(uint8_t prefix, uint8_t opcode, uint8_t reg, uint8_t opreg) {
    emit2(prefix, opcode);
    assert(reg <= 7);
    *pos_++ = (kModeReg << 6) | (reg << 3) | opreg;
  }
  void emit2(uint8_t prefix, uint8_t opcode, uint8_t reg, const Operand &operand) {
    emit2(prefix, opcode);
    emit(reg, operand);
  }

  void emit3(uint8_t prefix1, uint8_t prefix2, uint8_t opcode) {
    ensureSpace();
    *pos_++ = prefix1;
    *pos_++ = prefix2;
    *pos_++ = opcode;
  }
  void emit3(uint8_t prefix1, uint8_t prefix2, uint8_t opcode, uint8_t reg, uint8_t opreg) {
    emit3(prefix1, prefix2, opcode);
    assert(reg <= 7);
    *pos_++ = (kModeReg << 6) | (reg << 3) | opreg;
  }
  void emit3(uint8_t prefix1, uint8_t prefix2, uint8_t opcode, uint8_t reg, const Operand &operand) {
    emit3(prefix1, prefix2, opcode);
    emit(reg, operand);
  }

  template <typename T>
  void shift_cl(const T &t, uint8_t r) {
    emit1(0xd3, r, t);
  }

  template <typename T>
  void shift_imm(const T &t, uint8_t r, int32_t imm) {
    if (imm == 1) {
      emit1(0xd1, r, t);
    } else {
      emit1(0xc1, r, t);
      *pos_++ = imm & 0x1F;
    }
  }
  void alu_imm(uint8_t r, int32_t imm, const Operand &operand) {
    if (imm >= SCHAR_MIN && imm <= SCHAR_MAX) {
      emit1(0x83, r, operand);
      *pos_++ = uint8_t(imm & 0xff);
    } else if (operand.isRegister(eax)) {
      emit1(0x05 | (r << 3));
      writeInt32(imm);
    } else {
      emit1(0x81, r, operand);
      writeInt32(imm);
    }
  }

 private:
  ke::Vector<uint32_t> external_refs_;
  ke::Vector<uint32_t> local_refs_;
};

#endif // _include_sourcepawn_assembler_x86_h__

