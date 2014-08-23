// vim: set sts=8 ts=4 sw=4 tw=99 et:
//
// Copyright (C) 2013, David Anderson and AlliedModders LLC
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
//  * Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//  * Neither the name of AlliedModders LLC nor the names of its contributors
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#ifndef _include_amtl_refcounting_h_
#define _include_amtl_refcounting_h_

#include <am-utility.h>
#include <am-moveable.h>

namespace ke {

template <typename T> class Ref;

// Objects in AMTL inheriting from Refcounted will have an initial refcount
// of 0. However, in some systems (such as COM), the initial refcount is 1,
// or functions may return raw pointers that have been AddRef'd. In these
// cases it would be a mistake to use Ref<> or PassRef<>, since the object
// would leak an extra reference.
//
// This container holds a refcounted object without addrefing it. This is
// intended only for interacting with functions which return an object that
// has been manually AddRef'd. Note that this will perform a Release(), so
// so it is necessary to assign it to retain the object.
template <typename T>
class AlreadyRefed
{
  public:
    AlreadyRefed(T *t)
      : thing_(t)
    {
    }
    ~AlreadyRefed() {
        if (thing_)
            thing_->Release();
    }

    T *release() const {
        return ReturnAndVoid(thing_);
    }

  private:
    mutable T *thing_;
};

template <typename T>
static inline AlreadyRefed<T>
AdoptRef(T *t)
{
    return AlreadyRefed<T>(t);
}

// When returning a value, we'd rather not be needlessly changing the refcount,
// so we have a special type to use for returns.
template <typename T>
class PassRef
{
  public:
    PassRef(T *thing)
      : thing_(thing)
    {
        AddRef();
    }
    PassRef()
      : thing_(NULL)
    {
    }

    PassRef(const AlreadyRefed<T *> &other)
      : thing_(other.release())
    {
        // Don't addref, newborn means already addref'd.
    }

    template <typename S>
    PassRef(const AlreadyRefed<S *> &other)
      : thing_(other.release())
    {
        // Don't addref, newborn means already addref'd.
    }

    template <typename S>
    inline PassRef(const Ref<S> &other);

    PassRef(const PassRef &other)
      : thing_(other.release())
    {
    }
    template <typename S>
    PassRef(const PassRef<S> &other)
      : thing_(other.release())
    {
    }
    ~PassRef()
    {
        Release();
    }

    operator T &() {
        return *thing_;
    }
    operator T *() const {
        return thing_;
    }
    T *operator ->() const {
        return operator *();
    }
    T *operator *() const {
        return thing_;
    }
    bool operator !() const {
        return !thing_;
    }

    T *release() const {
        return ReturnAndVoid(thing_);
    }

    template <typename S>
    PassRef &operator =(const PassRef<S> &other) {
        Release();
        thing_ = other.release();
        return *this;
    }

  private:
    // Disallowed operators.
    PassRef &operator =(T *other);
    PassRef &operator =(AlreadyRefed<T> &other);

    void AddRef() {
        if (thing_)
            thing_->AddRef();
    }
    void Release() {
        if (thing_)
            thing_->Release();
    }

  private:
    mutable T *thing_;
};

// Classes which are refcounted should inherit from this. Note that reference
// counts start at 0 in AMTL, rather than 1. This avoids the complexity of
// having to adopt the initial ref upon allocation. However, this also means
// invoking Release() on a newly allocated object is illegal. Newborn objects
// must either be assigned to a Ref or PassRef (NOT an AdoptRef/AlreadyRefed),
// or must be deleted using |delete|.
template <typename T>
class Refcounted
{
  public:
    Refcounted()
      : refcount_(0)
    {
    }

    void AddRef() {
        refcount_++;
    }
    void Release() {
        assert(refcount_ > 0);
        if (--refcount_ == 0)
            delete static_cast<T *>(this);
    }

  protected:
    ~Refcounted() {
    }

  private:
    uintptr_t refcount_;
};

// Simple class for automatic refcounting.
template <typename T>
class Ref
{
  public:
    Ref(T *thing)
      : thing_(thing)
    {
        AddRef();
    }

    Ref()
      : thing_(NULL)
    {
    }

    Ref(const Ref &other)
      : thing_(other.thing_)
    {
        AddRef();
    }
    Ref(Moveable<Ref> other)
      : thing_(other->thing_)
    {
        other->thing_ = NULL;
    }
    template <typename S>
    Ref(const Ref<S> &other)
      : thing_(*other)
    {
        AddRef();
    }
    Ref(const PassRef<T> &other)
      : thing_(other.release())
    {
    }
    template <typename S>
    Ref(const PassRef<S> &other)
      : thing_(other.release())
    {
    }
    Ref(const AlreadyRefed<T> &other)
      : thing_(other.release())
    {
    }
    template <typename S>
    Ref(const AlreadyRefed<S> &other)
      : thing_(other.release())
    {
    }
    ~Ref()
    {
        Release();
    }

    T *operator ->() const {
        return operator *();
    }
    T *operator *() const {
        return thing_;
    }
    operator T *() {
        return thing_;
    }
    bool operator !() const {
        return !thing_;
    }

    template <typename S>
    Ref &operator =(S *thing) {
        Release();
        thing_ = thing;
        AddRef();
        return *this;
    }
    
    template <typename S>
    Ref &operator =(const PassRef<S> &other) {
        Release();
        thing_ = other.release();
        return *this;
    }

    template <typename S>
    Ref &operator =(const AlreadyRefed<S> &other) {
        Release();
        thing_ = other.release();
        return *this;
    }

    Ref &operator =(const Ref &other) {
        Release();
        thing_ = other.thing_;
        AddRef();
        return *this;
    }

    Ref &operator =(Moveable<Ref> other) {
        Release();
        thing_ = other->thing_;
        other->thing_ = NULL;
        return *this;
    }

  private:
    void AddRef() {
        if (thing_)
            thing_->AddRef();
    }
    void Release() {
        if (thing_)
            thing_->Release();
    }

  protected:
    T *thing_;
};

template <typename T> template <typename S>
PassRef<T>::PassRef(const Ref<S> &other)
  : thing_(*other)
{
    AddRef();
}

} // namespace ke

#endif // _include_amtl_refcounting_h_

