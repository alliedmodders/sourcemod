#include <unistd.h>
#include "PosixThreads.h"

void PosixThreader::ThreadSleep(unsigned int ms)
{
	usleep( ms * 1000 );
}

void PosixThreader::GetPriorityBounds(ThreadPriority &max, ThreadPriority &min)
{
	max = ThreadPrio_Normal;
	min = ThreadPrio_Normal;
}

IMutex *PosixThreader::MakeMutex()
{
	pthread_mutex_t mutex;

	if (pthread_mutex_init(&mutex, NULL) != 0)
		return NULL;

	PosixMutex *pMutex = new PosixMutex(mutex);

	return pMutex;
}


void PosixThreader::MakeThread(IThread *pThread)
{
	ThreadParams defparams;

	defparams.flags = Thread_AutoRelease;
	defparams.prio = ThreadPrio_Normal;

	MakeThread(pThread, &defparams);
}

IThreadHandle *PosixThreader::MakeThread(IThread *pThread, ThreadFlags flags)
{
	ThreadParams defparams;

	defparams.flags = flags;
	defparams.prio = ThreadPrio_Normal;

	return MakeThread(pThread, &defparams);
}

void *Posix_ThreadGate(void *param)
{
	PosixThreader::ThreadHandle *pHandle = 
		reinterpret_cast<PosixThreader::ThreadHandle *>(param);

	//Block this thread from being started initially.
	pthread_mutex_lock(&pHandle->m_runlock);
	//if we get here, we've obtained the lock and are allowed to run.
	//unlock and continue.
	pthread_mutex_unlock(&pHandle->m_runlock);

	pHandle->m_run->RunThread(pHandle);

	ThreadParams params;
	pthread_mutex_lock(&pHandle->m_statelock);
	pHandle->m_state = Thread_Done;
	pHandle->GetParams(&params);
	pthread_mutex_unlock(&pHandle->m_statelock);

	pHandle->m_run->OnTerminate(pHandle, false);
	if (params.flags & Thread_AutoRelease)
		delete pHandle;

	return 0;
}

ThreadParams g_defparams;
IThreadHandle *PosixThreader::MakeThread(IThread *pThread, const ThreadParams *params)
{
	if (params == NULL)
		params = &g_defparams;

	PosixThreader::ThreadHandle *pHandle = 
		new PosixThreader::ThreadHandle(this, pThread, params);

	pthread_mutex_lock(&pHandle->m_runlock);

	int err;
	err = pthread_create(&pHandle->m_thread, NULL, Posix_ThreadGate, (void *)pHandle);	

	if (err != 0)
	{
		pthread_mutex_unlock(&pHandle->m_runlock);
		delete pHandle;
		return NULL;
	}

	//Don't bother setting priority...

	if (!(pHandle->m_params.flags & Thread_CreateSuspended))
	{
		pHandle->m_state = Thread_Running;
		err = pthread_mutex_unlock(&pHandle->m_runlock);
		if (err != 0)
			pHandle->m_state = Thread_Paused;
	}

	return pHandle;
}

IEventSignal *PosixThreader::MakeEventSignal()
{
	return new PosixEventSignal();
}

/*****************
**** Mutexes ****
*****************/

PosixThreader::PosixMutex::~PosixMutex()
{
	pthread_mutex_destroy(&m_mutex);
}

bool PosixThreader::PosixMutex::TryLock()
{
	int err = pthread_mutex_trylock(&m_mutex);

	return (err == 0);
}

void PosixThreader::PosixMutex::Lock()
{
	pthread_mutex_lock(&m_mutex);
}

void PosixThreader::PosixMutex::Unlock()
{
	pthread_mutex_unlock(&m_mutex);
}

void PosixThreader::PosixMutex::DestroyThis()
{
	delete this;
}

/******************
* Thread Handles *
******************/

PosixThreader::ThreadHandle::ThreadHandle(IThreader *parent, IThread *run, const ThreadParams *params) : 
	m_parent(parent), m_params(*params), m_run(run), m_state(Thread_Paused)
{
	pthread_mutex_init(&m_runlock, NULL);
	pthread_mutex_init(&m_statelock, NULL);
}

PosixThreader::ThreadHandle::~ThreadHandle()
{
	pthread_mutex_destroy(&m_runlock);
	pthread_mutex_destroy(&m_statelock);
}

bool PosixThreader::ThreadHandle::WaitForThread()
{
	void *arg;
	
	if (pthread_join(m_thread, &arg) != 0)
		return false;

	return true;
}

ThreadState PosixThreader::ThreadHandle::GetState()
{
	ThreadState state;

	pthread_mutex_lock(&m_statelock);
	state = m_state;
	pthread_mutex_unlock(&m_statelock);

	return state;
}

IThreadCreator *PosixThreader::ThreadHandle::Parent()
{
	return m_parent;
}

void PosixThreader::ThreadHandle::DestroyThis()
{
	if (m_params.flags & Thread_AutoRelease)
		return;

	delete this;
}

void PosixThreader::ThreadHandle::GetParams(ThreadParams *ptparams)
{
	if (!ptparams)
		return;

	*ptparams = m_params;
}

ThreadPriority PosixThreader::ThreadHandle::GetPriority()
{
	return ThreadPrio_Normal;
}

bool PosixThreader::ThreadHandle::SetPriority(ThreadPriority prio)
{
	return (prio == ThreadPrio_Normal);
}

bool PosixThreader::ThreadHandle::Unpause()
{
	if (m_state != Thread_Paused)
		return false;

	m_state = Thread_Running;

	if (pthread_mutex_unlock(&m_runlock) != 0)
	{
		m_state = Thread_Paused;
		return false;
	}

	return true;
}

/*****************
 * EVENT SIGNALS *
 *****************/

PosixThreader::PosixEventSignal::PosixEventSignal()
{
	pthread_cond_init(&m_cond, NULL);
	pthread_mutex_init(&m_mutex, NULL);
}

PosixThreader::PosixEventSignal::~PosixEventSignal()
{
	pthread_cond_destroy(&m_cond);
	pthread_mutex_destroy(&m_mutex);
}

void PosixThreader::PosixEventSignal::Wait()
{
	pthread_mutex_lock(&m_mutex);
	pthread_cond_wait(&m_cond, &m_mutex);
	pthread_mutex_unlock(&m_mutex);
}

void PosixThreader::PosixEventSignal::Signal()
{
	pthread_mutex_lock(&m_mutex);
	pthread_cond_broadcast(&m_cond);
	pthread_mutex_unlock(&m_mutex);
}

void PosixThreader::PosixEventSignal::DestroyThis()
{
        delete this;
}

