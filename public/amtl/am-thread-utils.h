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

#ifndef _include_amtl_threads_
#define _include_amtl_threads_

#include <assert.h>
#if defined(_MSC_VER)
# include <windows.h>
# include <WinBase.h>
#else
# include <pthread.h>
#endif
#include <am-utility.h>

// Thread primitives for SourcePawn.
//
// Linking Requirements:
// 
// OS X: None (-lpthread and -ldl are optional)
// Windows: None
// Linux: -lpthread -lrt required
//
// -- Mutexes --
//
// A Lockable is a mutual exclusion primitive. It can be owned by at most one
// thread at a time, and ownership blocks any other thread from taking taking
// ownership. Ownership must be acquired and released on the same thread.
// Lockables are not re-entrant.
//
// While a few classes support the Lockable interface, the simplest Lockable
// object that can be instantiated is a Mutex.
//
// -- Condition Variables --
//
// A ConditionVariable provides mutually exclusive access based on a
// condition ocurring. CVs provide two capabilities: Wait(), which will block
// until the condition is triggered, and Notify(), which signals any blocking
// thread that the condition has occurred.
//
// Condition variables have an underlying mutex lock. This lock must be
// acquired before calling Wait() or Notify(). It is automatically released
// once Wait begins blocking. This operation is atomic with respect to other
// threads and the mutex. For example, it is not possible for the lock to be
// acquired by another thread in between unlocking and blocking. Since Notify
// also requires the lock to be acquired, there is no risk of an event
// accidentally dissipating into thin air because it was sent before the other
// thread began blocking.
//
// When Wait() returns, the lock is automatically re-acquired. This operation
// is NOT atomic. In between waking up and re-acquiring the lock, another
// thread may steal the lock and issue another event. Applications must
// account for this. For example, a message pump should check that there are
// no messages left to process before blocking again.
//
// Likewise, it is also not defined whether a Signal() will have any effect
// while a thread is not waiting on the monitor. This is yet another reason
// the above paragraph is so important - applications should, under a lock of
// the condition variable - check for state changes before waiting.
//
// -- Threads --
//
// A Thread object, when created, spawns a new thread with the given callback
// (the callbacks must implement IRunnable). Threads have one method of
// interest, Join(), which will block until the thread's execution finishes.
// Deleting a thread object will free any operating system resources associated
// with that thread, if the thread has finished executing.
//
// Threads can fail to spawn; make sure to check Succeeded().
//

namespace ke {

// Abstraction for getting a unique thread identifier. Debug-only.
#if defined(_MSC_VER)
typedef DWORD ThreadId;

static inline ThreadId GetCurrentThreadId()
{
  return ::GetCurrentThreadId();
}
#else
typedef pthread_t ThreadId;

static inline ThreadId GetCurrentThreadId()
{
  return pthread_self();
}
#endif

// Classes which use non-reentrant, same-thread lock/unlock semantics should
// inherit from this and implement DoLock/DoUnlock.
class Lockable
{
 public:
  Lockable()
  {
#if !defined(NDEBUG)
    owner_ = 0;
#endif
  }
  virtual ~Lockable() {
  }

  bool TryLock() {
    if (DoTryLock()) {
      DebugSetLocked();
      return true;
    }
    return false;
  }

  void Lock() {
    assert(Owner() != GetCurrentThreadId());
    DoLock();
    DebugSetLocked();
  }

  void Unlock() {
    assert(Owner() == GetCurrentThreadId());
    DebugSetUnlocked();
    DoUnlock();
  }

  void AssertCurrentThreadOwns() const {
    assert(Owner() == GetCurrentThreadId());
  }
#if !defined(NDEBUG)
  bool Locked() const {
    return owner_ != 0;
  }
  ThreadId Owner() const {
    return owner_;
  }
#endif

  virtual bool DoTryLock() = 0;
  virtual void DoLock() = 0;
  virtual void DoUnlock() = 0;

 protected:
  void DebugSetUnlocked() {
#if !defined(NDEBUG)
    owner_ = 0;
#endif
  }
  void DebugSetLocked() {
#if !defined(NDEBUG)
    owner_ = GetCurrentThreadId();
#endif
  }

 protected:
#if !defined(NDEBUG)
  ThreadId owner_;
#endif
};

// RAII for automatically locking and unlocking an object.
class AutoLock
{
 public:
  AutoLock(Lockable *lock)
   : lock_(lock)
  {
    lock_->Lock();
  }
  ~AutoLock() {
    lock_->Unlock();
  }

 private:
  Lockable *lock_;
};

class AutoTryLock
{
 public:
  AutoTryLock(Lockable *lock)
  {
    lock_ = lock->TryLock() ? lock : NULL;
  }
  ~AutoTryLock() {
    if (lock_)
      lock_->Unlock();
  }

 private:
  Lockable *lock_;
};

// RAII for automatically unlocking and relocking an object.
class AutoUnlock
{
 public:
  AutoUnlock(Lockable *lock)
   : lock_(lock)
  {
    lock_->Unlock();
  }
  ~AutoUnlock() {
    lock_->Lock();
  }

 private:
  Lockable *lock_;
};

enum WaitResult {
  // Woke up because something happened.
  Wait_Signaled,

  // Woke up because nothing happened and a timeout was specified.
  Wait_Timeout,

  // Woke up, but because of an error.
  Wait_Error
};

// This must be implemented in order to spawn a new thread.
class IRunnable
{
 public:
  virtual ~IRunnable() {
  }

  virtual void Run() = 0;
};

} // namespace ke

// Include the actual thread implementations.
#if defined(_MSC_VER)
# include "am-thread-windows.h"
#else
# include "am-thread-posix.h"
#endif

#endif // _include_amtl_threads_
