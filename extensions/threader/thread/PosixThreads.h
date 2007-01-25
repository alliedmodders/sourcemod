#ifndef _INCLUDE_POSIXTHREADS_H_
#define _INCLUDE_POSIXTHREADS_H_

#include <pthread.h>
#include "IThreader.h"

using namespace SourceMod;

void *Posix_ThreadGate(void *param);

class PosixThreader : public IThreader
{
public:
	class ThreadHandle : public IThreadHandle
	{
		friend class PosixThreader;
		friend void *Posix_ThreadGate(void *param);
	public:
		ThreadHandle(IThreader *parent, IThread *run, const ThreadParams *params);
		virtual ~ThreadHandle();
	public:
		virtual bool WaitForThread();
		virtual void DestroyThis();
		virtual IThreadCreator *Parent();
		virtual void GetParams(ThreadParams *ptparams);
		virtual ThreadPriority GetPriority();
		virtual bool SetPriority(ThreadPriority prio);
		virtual ThreadState GetState();
		virtual bool Unpause();
	protected:
		IThreader *m_parent;		//Parent handle
		pthread_t m_thread;			//Windows HANDLE	
		ThreadParams m_params;		//Current Parameters
		IThread *m_run;	//Runnable context
		pthread_mutex_t m_statelock;
		pthread_mutex_t m_runlock;
		ThreadState m_state;		//internal state
	};
	class PosixMutex : public IMutex
	{
	public:
		PosixMutex(pthread_mutex_t m) : m_mutex(m)
		{
		};
		virtual ~PosixMutex();
	public:
		virtual bool TryLock();
		virtual void Lock();
		virtual void Unlock();
		virtual void DestroyThis();
	protected:
		pthread_mutex_t m_mutex;
	};
	class PosixEventSignal : public IEventSignal
	{
	public:
		PosixEventSignal();
		virtual ~PosixEventSignal();
	public:
		virtual void Wait();
		virtual void Signal();
		virtual void DestroyThis();
	protected:
		pthread_cond_t m_cond;
		pthread_mutex_t m_mutex;
	};
public:
	IMutex *MakeMutex();
	void MakeThread(IThread *pThread);
	IThreadHandle *MakeThread(IThread *pThread, ThreadFlags flags);
	IThreadHandle *MakeThread(IThread *pThread, const ThreadParams *params);
	void GetPriorityBounds(ThreadPriority &max, ThreadPriority &min);
	void ThreadSleep(unsigned int ms);
	IEventSignal *MakeEventSignal();
	IThreadWorker *MakeWorker(bool threaded);
	void DestroyWorker(IThreadWorker *pWorker);
};

#if defined SM_DEFAULT_THREADER && !defined SM_MAIN_THREADER
#define SM_MAIN_THREADER PosixThreader;
typedef class PosixThreader MainThreader;
#endif

#endif //_INCLUDE_POSIXTHREADS_H_
