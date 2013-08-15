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
#ifndef _include_sourcepawn_thread_posix_h_
#define _include_sourcepawn_thread_posix_h_

#include <pthread.h>
#include <sys/time.h>
#include <errno.h>
#include <stdio.h>
#if defined(__linux__)
# include <sys/prctl.h>
#endif

namespace ke {

class Mutex : public Lockable
{
 public:
  Mutex() {
#if !defined(NDEBUG)
    int rv =
#endif
      pthread_mutex_init(&mutex_, NULL);
    assert(rv == 0);
  }
  ~Mutex() {
    pthread_mutex_destroy(&mutex_);
  }

  bool DoTryLock() {
    return pthread_mutex_trylock(&mutex_) == 0;
  }

  void DoLock() {
    pthread_mutex_lock(&mutex_);
  }

  void DoUnlock() {
    pthread_mutex_unlock(&mutex_);
  }
  
  pthread_mutex_t *raw() {
    return &mutex_;
  }

 private:
  pthread_mutex_t mutex_;
};

// Currently, this class only supports single-listener CVs.
class ConditionVariable : public Lockable
{
 public:
  ConditionVariable() {
#if !defined(NDEBUG)
    int rv =
#endif
      pthread_cond_init(&cv_, NULL);
    assert(rv == 0);
  }
  ~ConditionVariable() {
    pthread_cond_destroy(&cv_);
  }

  bool DoTryLock() {
    return mutex_.DoTryLock();
  }
  void DoLock() {
    mutex_.DoLock();
  }
  void DoUnlock() {
    mutex_.DoUnlock();
  }

  void Notify() {
    AssertCurrentThreadOwns();
    pthread_cond_signal(&cv_);
  }

  WaitResult Wait(size_t timeout_ms) {
    AssertCurrentThreadOwns();

    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
      return Wait_Error;

    ts.tv_sec += timeout_ms / 1000;
    ts.tv_nsec += (timeout_ms % 1000) * 1000000;
    if (ts.tv_nsec >= 1000000000) {
      ts.tv_sec++;
      ts.tv_nsec -= 1000000000;
    }

    DebugSetUnlocked();
    int rv = pthread_cond_timedwait(&cv_, mutex_.raw(), &ts);
    DebugSetLocked();

    if (rv == ETIMEDOUT)
      return Wait_Timeout;
    if (rv == 0)
      return Wait_Signaled;
    return Wait_Error;
  }

  WaitResult Wait() {
    AssertCurrentThreadOwns();

    DebugSetUnlocked();
    int rv = pthread_cond_wait(&cv_, mutex_.raw());
    DebugSetLocked();

    if (rv == 0)
      return Wait_Signaled;
    return Wait_Error;
  }

 private:
  Mutex mutex_;
  pthread_cond_t cv_;
};

class Thread
{
  struct ThreadData {
    IRunnable *run;
    char name[17];
  };
 public:
  Thread(IRunnable *run, const char *name = NULL) {
    ThreadData *data = new ThreadData;
    data->run = run;
    snprintf(data->name, sizeof(data->name), "%s", name ? name : "");

    initialized_ = (pthread_create(&thread_, NULL, Main, data) == 0);
    if (!initialized_)
      delete data;
  }

  bool Succeeded() const {
    return initialized_;
  }

  void Join() {
    if (!Succeeded())
      return;
    pthread_join(thread_, NULL);
  }

 private:
  static void *Main(void *arg) {
    AutoPtr<ThreadData> data((ThreadData *)arg);

    if (data->name[0]) {
#if defined(__linux__)
      prctl(PR_SET_NAME, (unsigned long)data->name);
#elif defined(__APPLE__)
      int (*fn)(const char *) = (int (*)(const char *))dlsym(RTLD_DEFAULT, "pthread_setname_np");
      if (fn)
        fn(data->name);
#endif
    }
    data->run->Run();
    return NULL;
  }

 private:
  bool initialized_;
  pthread_t thread_;
};

} // namespace ke

#endif // _include_sourcepawn_thread_posix_h_

