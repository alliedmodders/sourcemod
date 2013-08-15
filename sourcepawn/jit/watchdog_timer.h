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
#ifndef _include_sourcepawn_watchdog_timer_posix_h_
#define _include_sourcepawn_watchdog_timer_posix_h_

#include <stddef.h>
#include <stdint.h>
#include <ke_thread_utils.h>

typedef bool (*WatchdogCallback)();

class WatchdogTimer : public ke::IRunnable
{
 public:
  WatchdogTimer();
  ~WatchdogTimer();

  bool Initialize(size_t timeout_ms);
  void Shutdown();

  // Called from main thread.
  bool NotifyTimeoutReceived();
  bool HandleInterrupt();

 private:
  // Watchdog thread.
  void Run();

 private:
  bool terminate_;
  size_t timeout_ms_;
  ke::ThreadId mainthread_;

  ke::AutoPtr<ke::Thread> thread_;
  ke::ConditionVariable cv_;

  // Accessed only on the watchdog thread.
  uintptr_t last_frame_id_;
  bool second_timeout_;

  // Accessed only on the main thread.
  bool timedout_;
};

extern WatchdogTimer g_WatchdogTimer;

#endif // _include_sourcepawn_watchdog_timer_posix_h_
