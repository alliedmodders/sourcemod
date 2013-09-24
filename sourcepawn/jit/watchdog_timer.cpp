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
#include "watchdog_timer.h"
#include "x86/jit_x86.h"
#include <string.h>

WatchdogTimer g_WatchdogTimer;

WatchdogTimer::WatchdogTimer()
  : terminate_(false),
    mainthread_(ke::GetCurrentThreadId()),
    last_frame_id_(0),
    second_timeout_(false),
    timedout_(false)
{
}

WatchdogTimer::~WatchdogTimer()
{
  assert(!thread_);
}

bool
WatchdogTimer::Initialize(size_t timeout_ms)
{
  if (thread_)
    return false;

  timeout_ms_ = timeout_ms;

  thread_ = new ke::Thread(this, "SM Watchdog");
  if (!thread_->Succeeded())
    return false;

  return true;
}

void
WatchdogTimer::Shutdown()
{
  if (terminate_ || !thread_)
    return;

  {
    ke::AutoLock lock(&cv_);
    terminate_ = true;
    cv_.Notify();
  }
  thread_->Join();
  thread_ = NULL;
}

void
WatchdogTimer::Run()
{
  ke::AutoLock lock(&cv_);

  // Initialize the frame id, so we don't have to wait longer on startup.
  last_frame_id_ = g_Jit.FrameId();

  while (!terminate_) {
    ke::WaitResult rv = cv_.Wait(timeout_ms_ / 2);
    if (terminate_)
      return;

    if (rv == ke::Wait_Error)
      return;

    if (rv != ke::Wait_Timeout)
      continue;

    // We reached a timeout. If the current frame is not equal to the last
    // frame, then we assume the server is still moving enough to process
    // frames. We also make sure JIT code is actually on the stack, since
    // our concept of frames won't move if JIT code is not running.
    //
    // Note that it's okay if these two race: it's just a heuristic, and
    // worst case, we'll miss something that might have timed out but
    // ended up resuming.
    uintptr_t frame_id = g_Jit.FrameId();
    if (frame_id != last_frame_id_ || !g_Jit.RunningCode()) {
      last_frame_id_ = frame_id;
      second_timeout_ = false;
      continue;
    }

    // We woke up with the same frame. We might be timing out. Wait another
    // time to see if so.
    if (!second_timeout_) {
      second_timeout_ = true;
      continue;
    }

    {
      // Prevent the JIT from linking or destroying runtimes and functions.
      ke::AutoLock lock(g_Jit.Mutex());

      // Set the timeout notification bit. If this is detected before any patched
      // JIT backedges are reached, the main thread will attempt to acquire the
      // monitor lock, and block until we call Wait().
      timedout_ = true;
    
      // Patch all jumps. This can race with the main thread's execution since
      // all code writes are 32-bit integer instruction operands, which are
      // guaranteed to be atomic on x86.
      g_Jit.PatchAllJumpsForTimeout();
    }

    // The JIT will be free to compile new functions while we wait, but it will
    // see the timeout bit set above and immediately bail out.
    cv_.Wait();

    second_timeout_ = false;

    // Reset the last frame ID to something unlikely to be chosen again soon.
    last_frame_id_--;

    // It's possible that Shutdown() raced with the timeout, and if so, we
    // must leave now to prevent a deadlock.
    if (terminate_)
      return;
  }
}

bool
WatchdogTimer::NotifyTimeoutReceived()
{
  // We are guaranteed that the watchdog thread is waiting for our
  // notification, and is therefore blocked. We take the JIT lock
  // anyway for sanity.
  {
    ke::AutoLock lock(g_Jit.Mutex());
    g_Jit.UnpatchAllJumpsFromTimeout();
  }

  timedout_ = false;

  // Wake up the watchdog thread, it's okay to keep processing now.
  ke::AutoLock lock(&cv_);
  cv_.Notify();
  return false;
}

bool
WatchdogTimer::HandleInterrupt()
{
  if (timedout_)
    return NotifyTimeoutReceived();
  return true;
}
