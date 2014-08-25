// vim: set sts=2 ts=8 sw=2 tw=99 et:
//
// Copyright (C) 2012-2014 David Anderson
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
#ifndef _include_sc_utility_h_
#define _include_sc_utility_h_

#include "sc.h"
#include "sctracker.h"

// Wrapper around arguments, since SP has no consistent concept of a type.
class ArgDesc
{
 public:
  virtual int ident() const = 0;
  virtual int tag() const = 0;
  virtual int numdim() const = 0;
  virtual int dim(int index) const = 0;
  virtual int usage() const = 0;
};

class ArgInfoDesc : public ArgDesc
{
 public:
  ArgInfoDesc(arginfo *arg)
   : arg_(arg)
  { }
  int ident() const KE_OVERRIDE {
    return arg_->ident;
  }
  int tag() const KE_OVERRIDE {
    assert(arg_->numtags == 1);
    return arg_->tags[0];
  }
  int numdim() const KE_OVERRIDE {
    return arg_->numdim;
  }
  int dim(int index) const KE_OVERRIDE {
    return arg_->dim[index];
  }
  int usage() const KE_OVERRIDE {
    return arg_->usage;
  }

 private:
  arginfo *arg_;
};

class FuncArgDesc : public ArgDesc
{
 public:
  FuncArgDesc(funcarg_t *arg)
   : arg_(arg)
  { }
  int ident() const KE_OVERRIDE {
    return arg_->ident;
  }
  int tag() const KE_OVERRIDE {
    assert(arg_->tagcount == 1);
    return arg_->tags[0];
  }
  int numdim() const KE_OVERRIDE {
    return arg_->dimcount;
  }
  int dim(int index) const KE_OVERRIDE {
    return arg_->dims[index];
  }
  int usage() const KE_OVERRIDE {
    return arg_->fconst ? uCONST : 0;
  }
 private:
  funcarg_t *arg_;
};

#endif // _include_sc_utility_h_
