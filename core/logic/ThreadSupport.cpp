/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2009 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */
#include <sm_platform.h>
#include <amtl/am-deque.h>
#include <amtl/am-maybe.h>
#include <amtl/am-thread.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include "BaseWorker.h"
#include "ThreadSupport.h"
#include "common_logic.h"

static constexpr unsigned int DEFAULT_THINK_TIME_MS	= 20;

class CompatWorker final : public IThreadWorker
{
  public:
	explicit CompatWorker(IThreadWorkerCallbacks* callbacks);
	~CompatWorker();

	void MakeThread(IThread *pThread) override;
	IThreadHandle *MakeThread(IThread *pThread, ThreadFlags flags) override;
	IThreadHandle *MakeThread(IThread *pThread, const ThreadParams *params) override;
	void GetPriorityBounds(ThreadPriority &max, ThreadPriority &min) override;
	unsigned int RunFrame() override;
	bool Pause() override;
	bool Unpause() override;
	bool Start() override;
	bool Stop(bool flush) override;
	WorkerState GetStatus(unsigned int *numThreads) override;
	void SetMaxThreadsPerFrame(unsigned int threads) override;
	void SetThinkTimePerFrame(unsigned int thinktime) override;

  private:
	void Flush();
	void Worker();
	void RunWork(SWThreadHandle* handle);
	void RunWorkLocked(std::unique_lock<std::mutex>* lock, SWThreadHandle* handle);

  private:
	IThreadWorkerCallbacks* callbacks_;
	WorkerState state_;
	std::mutex mutex_;
	std::condition_variable work_cv_;
	std::deque<SWThreadHandle*> work_;
	std::unique_ptr<std::thread> thread_;
	std::atomic<unsigned int> jobs_per_wakeup_;
	std::atomic<unsigned int> wait_between_jobs_;
};

CompatWorker::CompatWorker(IThreadWorkerCallbacks* callbacks)
	: callbacks_(callbacks),
	  state_(Worker_Stopped),
	  jobs_per_wakeup_(SM_DEFAULT_THREADS_PER_FRAME),
	  wait_between_jobs_(DEFAULT_THINK_TIME_MS)
{
}

CompatWorker::~CompatWorker()
{
	Stop(false /* ignored */);
	Flush();
}

bool CompatWorker::Start()
{
	std::lock_guard<std::mutex> lock(mutex_);
	if (state_ != Worker_Stopped)
		return false;

	thread_ = ke::NewThread("SM CompatWorker Thread", [this]() -> void {
		Worker();
	});

	state_ = Worker_Running;
	return true;
}

bool CompatWorker::Stop(bool)
{
	{
		std::lock_guard<std::mutex> lock(mutex_);

		if (state_ <= Worker_Stopped)
			return false;

		state_ = Worker_Stopped;
		work_cv_.notify_all();
	}

	thread_->join();
	thread_ = nullptr;

	Flush();
	return true;
}

bool CompatWorker::Pause()
{
	std::lock_guard<std::mutex> lock(mutex_);
	if (state_ != Worker_Running)
		return false;

	state_ = Worker_Paused;
	work_cv_.notify_all();
	return true;
}

bool CompatWorker::Unpause()
{
	std::lock_guard<std::mutex> lock(mutex_);
	if (state_ != Worker_Paused)
		return false;

	state_ = Worker_Running;
	work_cv_.notify_all();
	return true;
}

void CompatWorker::Flush()
{
	while (!work_.empty()) {
		auto handle = ke::PopFront(&work_);
		handle->GetThread()->OnTerminate(handle, true);
		if (handle->m_params.flags & Thread_AutoRelease)
			delete handle;
	}
}

void CompatWorker::Worker()
{
	// Note: this must be first to ensure an ordering between Worker() and
	// Start(). It must also be outside of the loop to ensure the lock is
	// held across wakeup and retesting the predicates.
	std::unique_lock<std::mutex> lock(mutex_);

	if (callbacks_) {
		lock.unlock();
		callbacks_->OnWorkerStart(this);
		lock.lock();
	}

	typedef std::chrono::system_clock Clock;
	typedef std::chrono::time_point<Clock> TimePoint;

	auto can_work = [this]() -> bool {
		return state_ == Worker_Running && !work_.empty();
	};

	ke::Maybe<TimePoint> wait;
	unsigned int work_in_frame = 0;
	for (;;) {
		if (state_ == Worker_Stopped)
			break;

		if (!can_work()) {
			// Wait for work or a Stop.
			work_cv_.wait(lock);
			continue;
		}

		if (wait.isValid()) {
			// Wait until the specified time has passed. If we wake up with a
			// timeout, then the wait has elapsed, so reset the holder.
			if (work_cv_.wait_until(lock, wait.get()) == std::cv_status::timeout)
				wait = ke::Nothing();
			continue;
		}

		assert(state_ == Worker_Running);
		assert(!work_.empty());

		SWThreadHandle* handle = ke::PopFront(&work_);
		RunWorkLocked(&lock, handle);
		work_in_frame++;

		// If we've reached our max jobs per "frame", signal that the next
		// immediate job must be delayed. We retain the old ThreadWorker
		// behavior by checking if the queue has more work. Thus, a delay
		// only occurs if two jobs would be processed in the same wakeup.
		if (work_in_frame >= jobs_per_wakeup_ && wait_between_jobs_ && can_work())
			wait = ke::Some(Clock::now() + std::chrono::milliseconds(wait_between_jobs_));
	}

	assert(lock.owns_lock());

	while (!work_.empty()) {
		SWThreadHandle* handle = ke::PopFront(&work_);
		RunWorkLocked(&lock, handle);
	}
}

unsigned int CompatWorker::RunFrame()
{
	unsigned int nprocessed = 0;
	for (unsigned int i = 1; i <= jobs_per_wakeup_; i++) {
		SWThreadHandle* handle;
		{
			std::lock_guard<std::mutex> lock(mutex_);
			if (work_.empty())
				break;
			handle = ke::PopFront(&work_);
		}

		RunWork(handle);
		nprocessed++;
	}
	return nprocessed;
}

void CompatWorker::RunWorkLocked(std::unique_lock<std::mutex>* lock, SWThreadHandle* handle)
{
	lock->unlock();
	RunWork(handle);
	lock->lock();
}

void CompatWorker::RunWork(SWThreadHandle* handle)
{
	bool autorelease = !!(handle->m_params.flags & Thread_AutoRelease);
	handle->m_state = Thread_Running;
	handle->GetThread()->RunThread(handle);
	handle->m_state = Thread_Done;
	handle->GetThread()->OnTerminate(handle, false);
	if (autorelease)
		delete handle;
}

void CompatWorker::MakeThread(IThread *pThread)
{
	ThreadParams params;
	params.flags = Thread_AutoRelease;
	MakeThread(pThread, &params);
}

IThreadHandle *CompatWorker::MakeThread(IThread *pThread, ThreadFlags flags)
{
	ThreadParams params;
	params.flags = flags;
	return MakeThread(pThread, &params);
}

IThreadHandle *CompatWorker::MakeThread(IThread *pThread, const ThreadParams *params)
{
	std::lock_guard<std::mutex> lock(mutex_);

	ThreadParams def_params;
	if (!params)
		params = &def_params;

	if (state_ <= Worker_Stopped)
		return nullptr;

	SWThreadHandle* handle = new SWThreadHandle(this, params, pThread);
	work_.push_back(handle);
	work_cv_.notify_one();
	return handle;
}

void CompatWorker::GetPriorityBounds(ThreadPriority &max, ThreadPriority &min)
{
	min = ThreadPrio_Normal;
	max = ThreadPrio_Normal;
}

void CompatWorker::SetMaxThreadsPerFrame(unsigned int threads)
{
	jobs_per_wakeup_ = threads;
}

void CompatWorker::SetThinkTimePerFrame(unsigned int thinktime)
{
	wait_between_jobs_ = thinktime;
}

WorkerState CompatWorker::GetStatus(unsigned int *numThreads)
{ 
	std::lock_guard<std::mutex> lock(mutex_);

	// This number is meaningless and the status is racy.
	if (numThreads)
		*numThreads = jobs_per_wakeup_;
	return state_;
}

class CompatThread final : public IThreadHandle
{
public:
	CompatThread(IThread* callbacks, const ThreadParams* params);

	bool WaitForThread() override;
	void DestroyThis() override;
	IThreadCreator *Parent() override;
	void GetParams(ThreadParams *ptparams) override;
	ThreadPriority GetPriority() override;
	bool SetPriority(ThreadPriority prio) override;
	ThreadState GetState() override;
	bool Unpause() override;

private:
	void Run();

private:
	IThread* callbacks_;
	ThreadParams params_;
	std::unique_ptr<std::thread> thread_;
	std::mutex mutex_;
	std::condition_variable check_cv_;
	std::atomic<bool> finished_;
};

CompatThread::CompatThread(IThread* callbacks, const ThreadParams* params)
	: callbacks_(callbacks),
	  params_(*params)
{
	if (!(params_.flags & Thread_CreateSuspended))
		Unpause();
}

bool CompatThread::Unpause()
{
	std::unique_lock<std::mutex> lock(mutex_);
	if (thread_)
		return false;

	thread_ = ke::NewThread("SM CompatThread", [this]() -> void {
		Run();
	});

	return true;
}

void CompatThread::Run()
{
	// Create an ordering between when the thread runs and when thread_ is assigned.
	std::unique_lock<std::mutex> lock(mutex_);

	lock.unlock();
	callbacks_->RunThread(this);
	finished_ = true;
	callbacks_->OnTerminate(this, false);

	if (params_.flags & Thread_AutoRelease) {
		// There should be no handles outstanding, so it's safe to self-destruct.
		thread_->detach();
		delete this;
		return;
	}

	lock.lock();
	callbacks_ = nullptr;
	check_cv_.notify_all();
}

bool CompatThread::WaitForThread()
{
	std::unique_lock<std::mutex> lock(mutex_);
	for (;;) {
		// When done, callbacks are unset. If paused, this will deadlock.
		if (!callbacks_)
			break;
		check_cv_.wait(lock);
	}

	thread_->join();
	return true;
}

ThreadState CompatThread::GetState()
{
	std::unique_lock<std::mutex> lock(mutex_);
	if (!thread_)
		return Thread_Paused;
	return finished_ ? Thread_Done : Thread_Running;
}

void CompatThread::DestroyThis()
{
	delete this;
}

ThreadPriority CompatThread::GetPriority()
{
	return ThreadPrio_Normal;
}

bool CompatThread::SetPriority(ThreadPriority prio)
{
	return prio == ThreadPrio_Normal;
}

IThreadCreator *CompatThread::Parent()
{
	return g_pThreader;
}

void CompatThread::GetParams(ThreadParams *ptparams)
{
	*ptparams = params_;
}

class CompatMutex : public IMutex
{
public:
	bool TryLock() {
		return mutex_.try_lock();
	}
	void Lock() {
		mutex_.lock();
	}
	void Unlock() {
		mutex_.unlock();
	}
	void DestroyThis() {
		delete this;
	}
private:
	std::mutex mutex_;
};

class CompatThreader final : public IThreader
{
public:
	void MakeThread(IThread *pThread) override;
	IThreadHandle *MakeThread(IThread *pThread, ThreadFlags flags) override;
	IThreadHandle *MakeThread(IThread *pThread, const ThreadParams *params) override;
	void GetPriorityBounds(ThreadPriority &max, ThreadPriority &min) override;
	IMutex *MakeMutex() override;
	void ThreadSleep(unsigned int ms) override;
	IEventSignal *MakeEventSignal() override;
	IThreadWorker *MakeWorker(IThreadWorkerCallbacks *hooks, bool threaded) override;
	void DestroyWorker(IThreadWorker *pWorker) override;
} sCompatThreader;

void CompatThreader::MakeThread(IThread *pThread)
{
	ThreadParams params;
	params.flags = Thread_AutoRelease;
	MakeThread(pThread, &params);
}

IThreadHandle *CompatThreader::MakeThread(IThread *pThread, ThreadFlags flags)
{
	ThreadParams params;
	params.flags = flags;
	return MakeThread(pThread, &params);
}

IThreadHandle *CompatThreader::MakeThread(IThread *pThread, const ThreadParams *params)
{
	ThreadParams def_params;
	if (!params)
		params = &def_params;

	return new CompatThread(pThread, params);
}

void CompatThreader::GetPriorityBounds(ThreadPriority &max, ThreadPriority &min)
{
	min = ThreadPrio_Normal;
	max = ThreadPrio_Normal;
}

IMutex *CompatThreader::MakeMutex()
{
	return new CompatMutex();
}

void CompatThreader::ThreadSleep(unsigned int ms)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

class CompatEventSignal final : public IEventSignal
{
public:
	void Wait() override {
		std::unique_lock<std::mutex> lock(mutex_);
		cv_.wait(lock);
	}
	void Signal() override {
		std::lock_guard<std::mutex> lock(mutex_);
		cv_.notify_all();
	}
	void DestroyThis() override {
		delete this;
	}

private:
	std::mutex mutex_;
	std::condition_variable cv_;
};

IEventSignal *CompatThreader::MakeEventSignal()
{
	return new CompatEventSignal();
}

IThreadWorker *CompatThreader::MakeWorker(IThreadWorkerCallbacks *hooks, bool threaded)
{
	if (!threaded)
		return new BaseWorker(hooks);
    return new CompatWorker(hooks);
}

void CompatThreader::DestroyWorker(IThreadWorker *pWorker)
{
	delete pWorker;
}

IThreader *g_pThreader = &sCompatThreader;

class RegThreadStuff : public SMGlobalClass
{
public:
	void OnSourceModAllInitialized()
	{
		sharesys->AddInterface(NULL, g_pThreader);
	}
} s_RegThreadStuff;

