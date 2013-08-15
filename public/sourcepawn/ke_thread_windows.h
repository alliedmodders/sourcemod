// vim: set ts=8 sts=2 sw=2 tw=99 et:
//
// This file is part of SourcePawn.
// 
// SourcePawn is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// SourcePawn is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with SourcePawn.  If not, see <http://www.gnu.org/licenses/>.
#ifndef _include_sourcepawn_thread_windows_h_
#define _include_sourcepawn_thread_windows_h_

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

  bool DoTryLock() {
    return !!TryEnterCriticalSection(&cs_);
  }
  void DoLock() {
    EnterCriticalSection(&cs_);
  }

  void DoUnlock() {
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

  bool DoTryLock() {
    return cs_.DoTryLock();
  }
  void DoLock() {
    cs_.DoLock();
  }
  void DoUnlock() {
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

#endif // _include_sourcepawn_thread_windows_h_
