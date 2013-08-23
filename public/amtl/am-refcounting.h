// vim: set sts=8 ts=2 sw=2 tw=99 et:
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

namespace ke {

template <typename T> class Ref;

// Holds a refcounted T without addrefing it. This is similar to PassRef<>
// below, but is intended only for freshly allocated objects which start
// with reference count 1, and we don't want to add an extra ref just by
// assigning to PassRef<> or Ref<>.
template <typename T>
class Newborn
{
  public:
    Newborn(T *t)
      : thing_(t)
    {
    }

    T *release() const {
        return ReturnAndVoid(thing_);
    }

  private:
    mutable T *thing_;
};

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

    PassRef(const Newborn<T *> &other)
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
    PassRef &operator =(Newborn<T> &other);

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

// Classes which are refcounted should inherit from this.
template <typename T>
class Refcounted
{
  public:
    Refcounted()
      : refcount_(1)
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
    Ref(const Newborn<T> &other)
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
    Ref &operator =(const Newborn<S> &other) {
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

