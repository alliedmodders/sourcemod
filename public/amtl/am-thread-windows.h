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

#ifndef _include_amtl_thread_windows_h_
#define _include_amtl_thread_windows_h_

#include <windows.h>

namespace ke {

class CriticalSection : public Lockable
{
 public:
  CriticalSection() {
    InitializeCriticalSection(&cs_);
  }
  ~CriticalSection() {
    DeleteCriticalSection(&cs_);
  }

  bool DoTryLock() KE_OVERRIDE {
    return !!TryEnterCriticalSection(&cs_);
  }
  void DoLock() KE_OVERRIDE {
    EnterCriticalSection(&cs_);
  }

  void DoUnlock() KE_OVERRIDE {
    LeaveCriticalSection(&cs_);
  }

 private:
  CRITICAL_SECTION cs_;
};

typedef CriticalSection Mutex;

// Currently, this class only supports single-listener CVs.
class ConditionVariable : public Lockable
{
 public:
  ConditionVariable() {
    event_ = CreateEvent(NULL, FALSE, FALSE, NULL);
  }
  ~ConditionVariable() {
    CloseHandle(event_);
  }

  bool DoTryLock() KE_OVERRIDE {
    return cs_.DoTryLock();
  }
  void DoLock() KE_OVERRIDE {
    cs_.DoLock();
  }
  void DoUnlock() KE_OVERRIDE {
    cs_.DoUnlock();
  }

  void Notify() {
    AssertCurrentThreadOwns();
    SetEvent(event_);
  }

  WaitResult Wait(size_t timeout_ms) {
    // This will assert if the lock has not been acquired. We don't need to be
    // atomic here, like pthread_cond_wait, because the event bit will stick
    // until reset by a wait function.
    Unlock();
    DWORD rv = WaitForSingleObject(event_, timeout_ms);
    Lock();

    if (rv == WAIT_TIMEOUT)
      return Wait_Timeout;
    if (rv == WAIT_FAILED)
      return Wait_Error;
    return Wait_Signaled;
  }

  WaitResult Wait() {
    return Wait(INFINITE);
  }

 private:
  CriticalSection cs_;
  HANDLE event_;
};

class Thread
{
 public:
  Thread(IRunnable *run, const char *name = NULL) {
    thread_ = CreateThread(NULL, 0, Main, run, 0, NULL);
  }
  ~Thread() {
    if (!thread_)
      return;
    CloseHandle(thread_);
  }

  bool Succeeded() const {
    return !!thread_;
  }

  void Join() {
    if (!Succeeded())
      return;
    WaitForSingleObject(thread_, INFINITE);
  }

  HANDLE handle() const {
	  return thread_;
  }

 private:
  static DWORD WINAPI Main(LPVOID arg) {
    ((IRunnable *)arg)->Run();
    return 0;
  }

#pragma pack(push, 8)
  struct ThreadNameInfo {
    DWORD dwType;
    LPCSTR szName;
    DWORD dwThreadID;
    DWORD dwFlags;
  };
#pragma pack(pop)

 private:
  HANDLE thread_;
};

} // namespace ke

#endif // _include_amtl_thread_windows_h_
