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

#ifndef _include_amtl_thread_posix_h_
#define _include_amtl_thread_posix_h_

#include <pthread.h>
#include <sys/time.h>
#include <errno.h>
#include <stdio.h>
#if defined(__linux__)
# include <sys/prctl.h>
#endif
#if defined(__APPLE__)
# include <dlfcn.h>
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

  bool DoTryLock() KE_OVERRIDE {
    return pthread_mutex_trylock(&mutex_) == 0;
  }

  void DoLock() KE_OVERRIDE {
    pthread_mutex_lock(&mutex_);
  }

  void DoUnlock() KE_OVERRIDE {
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

  bool DoTryLock() KE_OVERRIDE {
    return mutex_.DoTryLock();
  }
  void DoLock() KE_OVERRIDE {
    mutex_.DoLock();
  }
  void DoUnlock() KE_OVERRIDE {
    mutex_.DoUnlock();
  }

  void Notify() {
    AssertCurrentThreadOwns();
    pthread_cond_signal(&cv_);
  }

  WaitResult Wait(size_t timeout_ms) {
    AssertCurrentThreadOwns();

#if defined(__linux__)
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
      return Wait_Error;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);

    struct timespec ts;
    ts.tv_sec = tv.tv_sec;
    ts.tv_nsec = tv.tv_usec * 1000;
#endif

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

#endif // _include_amtl_thread_posix_h_

