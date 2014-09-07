// vim: set sts=2 ts=8 sw=2 tw=99 et:
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

#ifndef _include_amtl_thread_local_h_
#define _include_amtl_thread_local_h_

#include <am-thread-utils.h>

namespace ke {

// Stores a per-thread value. In single-threaded mode (KE_SINGLE_THREADED),
// this is a no-op container wrapper.
//
// T must be castable to uintptr_t.
//
// When assigning to a ThreadLocal<T>, the assigment will automatically attempt
// to allocate thread-local storage from the operating system. If it fails, it
// will abort the program. If this is undesirable, you may call allocate()
// up-front and handle the error case manually.
//
// The number of thread local slots available to processes is limited (on
// Linux, it is generally 1024). It is best to use ThreadLocal sparingly to
// play nicely with other libraries.
//
// ThreadLocal will free the underlying thread-local storage slot in its
// destructor, but it is not an AutoPtr. It does not delete pointers. Since
// one thread's value is only observable from that thread, make sure to free
// the contained resource (if necessary) before the thread exits.
template <typename T>
class ThreadLocal
{
 public:
  void operator =(const T &other) {
    set(other);
  }

  T operator *() const {
    return get();
  }
  T operator ->() const {
    return get();
  }
  bool operator !() const {
    return !get();
  }
  bool operator ==(const T &other) const {
    return get() == other;
  }
  bool operator !=(const T &other) const {
    return get() != other;
  }

 private:
  ThreadLocal(const ThreadLocal &other) KE_DELETE;
  ThreadLocal &operator =(const ThreadLocal &other) KE_DELETE;

#if !defined(KE_SINGLE_THREADED)
 private:
  int allocated_;

 public:
  ThreadLocal() {
    allocated_ = 0;
  }

  T get() const {
    if (!allocated_)
      return T();
    return internalGet();
  }
  void set(const T &t) {
    if (!allocated_ && !allocate()) {
      fprintf(stderr, "could not allocate thread-local storage\n");
      abort();
    }
    internalSet(t);
  }

# if defined(_MSC_VER)
  ~ThreadLocal() {
    if (allocated_)
      TlsFree(key_);
  }

 private:
  T internalGet() const {
    return reinterpret_cast<T>(TlsGetValue(key_));
  }
  void internalSet(const T &t) {
    TlsSetValue(key_, reinterpret_cast<LPVOID>(t));
  }
  bool allocate() {
    if (InterlockedCompareExchange(&allocated_, 1, 0) == 1)
      return true;
    key_ = TlsAlloc();
    return key_ != TLS_OUT_OF_INDEXES;
  }

  DWORD key_;

# else
 public:
  ~ThreadLocal() {
    if (allocated_)
      pthread_key_delete(key_);
  }

  bool allocate() {
    if (!__sync_bool_compare_and_swap(&allocated_, 0, 1))
      return true;
    return pthread_key_create(&key_, NULL) == 0;
  }

 private:
  T internalGet() const {
    return (T)reinterpret_cast<uintptr_t>(pthread_getspecific(key_));
  }
  void internalSet(const T &t) {
    pthread_setspecific(key_, reinterpret_cast<void *>(t));
  }

  pthread_key_t key_;
# endif // !_MSC_VER

#else // KE_SINGLE_THREADED
 public:
  ThreadLocal() {
    t_ = T();
  }

  bool allocate() {
    return true;
  }

  T get() const {
    return t_;
  }
  void set(const T &t) {
    t_ = t;
  }

 private:
  T t_;
#endif
};

} // namespace ke

#endif // _include_amtl_thread_local_h_
