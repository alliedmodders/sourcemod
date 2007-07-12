/**
 * vim: set ts=4 :
 * ===============================================================
 * SourceMod (C)2004-2007 AlliedModders LLC.  All rights reserved.
 * ===============================================================
 *
 * This file is not open source and may not be copied without explicit
 * written permission of AlliedModders LLC.  This file may not be redistributed 
 * in whole or significant part.
 * For information, see LICENSE.txt or http://www.sourcemod.net/license.php
 *
 * Version: $Id$
 */

#define _WIN32_WINNT 0x0400
#include "WinThreads.h"
#include "ThreadWorker.h"

IThreadWorker *WinThreader::MakeWorker(IThreadWorkerCallbacks *hooks, bool threaded)
{
	if (threaded)
	{
		return new ThreadWorker(hooks, this, DEFAULT_THINK_TIME_MS);
	} else {
		return new BaseWorker(hooks);
	}
}

void WinThreader::DestroyWorker(IThreadWorker *pWorker)
{
	delete pWorker;
}

void WinThreader::ThreadSleep(unsigned int ms)
{
	Sleep((DWORD)ms);
}

IMutex *WinThreader::MakeMutex()
{
	WinMutex *pMutex = new WinMutex();

	return pMutex;
}

IThreadHandle *WinThreader::MakeThread(IThread *pThread, ThreadFlags flags)
{
	ThreadParams defparams;

	defparams.flags = flags;
	defparams.prio = ThreadPrio_Normal;

	return MakeThread(pThread, &defparams);
}

void WinThreader::MakeThread(IThread *pThread)
{
	ThreadParams defparams;

	defparams.flags = Thread_AutoRelease;
	defparams.prio = ThreadPrio_Normal;

	MakeThread(pThread, &defparams);
}

DWORD WINAPI Win32_ThreadGate(LPVOID param)
{
	WinThreader::ThreadHandle *pHandle = 
		reinterpret_cast<WinThreader::ThreadHandle *>(param);

	pHandle->m_run->RunThread(pHandle);

	ThreadParams params;
	EnterCriticalSection(&pHandle->m_crit);
	pHandle->m_state = Thread_Done;
	pHandle->GetParams(&params);
	LeaveCriticalSection(&pHandle->m_crit);

	pHandle->m_run->OnTerminate(pHandle, false);
	if (params.flags & Thread_AutoRelease)
		delete pHandle;

	return 0;
}

void WinThreader::GetPriorityBounds(ThreadPriority &max, ThreadPriority &min)
{
	max = ThreadPrio_Maximum;
	min = ThreadPrio_Minimum;
}

ThreadParams g_defparams;
IThreadHandle *WinThreader::MakeThread(IThread *pThread, const ThreadParams *params)
{
	if (params == NULL)
		params = &g_defparams;

	WinThreader::ThreadHandle *pHandle = 
		new WinThreader::ThreadHandle(this, NULL, pThread, params);

	DWORD tid;
	pHandle->m_thread = 
		CreateThread(NULL, 0, &Win32_ThreadGate, (LPVOID)pHandle, CREATE_SUSPENDED, &tid);

	if (!pHandle->m_thread)
	{
		delete pHandle;
		return NULL;
	}

	if (pHandle->m_params.prio != ThreadPrio_Normal)
	{
		pHandle->SetPriority(pHandle->m_params.prio);
	}

	if (!(pHandle->m_params.flags & Thread_CreateSuspended))
	{
		pHandle->Unpause();
	}

	return pHandle;
}

IEventSignal *WinThreader::MakeEventSignal()
{
	HANDLE event = CreateEventA(NULL, FALSE, FALSE, NULL);

	if (!event)
		return NULL;

	WinEvent *pEvent = new WinEvent(event);

	return pEvent;
}

/*****************
 **** Mutexes ****
 *****************/

WinThreader::WinMutex::WinMutex()
{
	InitializeCriticalSection(&m_crit);
}

WinThreader::WinMutex::~WinMutex()
{
	DeleteCriticalSection(&m_crit);
}

bool WinThreader::WinMutex::TryLock()
{
	return (TryEnterCriticalSection(&m_crit) != FALSE);
}

void WinThreader::WinMutex::Lock()
{
	EnterCriticalSection(&m_crit);
}

void WinThreader::WinMutex::Unlock()
{
	LeaveCriticalSection(&m_crit);
}

void WinThreader::WinMutex::DestroyThis()
{
	delete this;
}

/******************
 * Thread Handles *
 ******************/

WinThreader::ThreadHandle::ThreadHandle(IThreader *parent, HANDLE hthread, IThread *run, const ThreadParams *params) : 
	m_parent(parent), m_thread(hthread), m_run(run), m_params(*params),
	m_state(Thread_Paused)
{
	InitializeCriticalSection(&m_crit);
}

WinThreader::ThreadHandle::~ThreadHandle()
{
	if (m_thread)
	{
		CloseHandle(m_thread);
		m_thread = NULL;
	}
	DeleteCriticalSection(&m_crit);
}

bool WinThreader::ThreadHandle::WaitForThread()
{
	if (m_thread == NULL)
		return false;

	if (WaitForSingleObject(m_thread, INFINITE) != 0)
		return false;

	return true;
}

ThreadState WinThreader::ThreadHandle::GetState()
{
	ThreadState state;

	EnterCriticalSection(&m_crit);
	state = m_state;
	LeaveCriticalSection(&m_crit);

	return state;
}

IThreadCreator *WinThreader::ThreadHandle::Parent()
{
	return m_parent;
}

void WinThreader::ThreadHandle::DestroyThis()
{
	if (m_params.flags & Thread_AutoRelease)
		return;

	delete this;
}

void WinThreader::ThreadHandle::GetParams(ThreadParams *ptparams)
{
	if (!ptparams)
		return;

	*ptparams = m_params;
}

ThreadPriority WinThreader::ThreadHandle::GetPriority()
{
	return m_params.prio;
}

bool WinThreader::ThreadHandle::SetPriority(ThreadPriority prio)
{
	if (!m_thread)
		return false;

	BOOL res = FALSE;

	if (prio >= ThreadPrio_Maximum)
		res = SetThreadPriority(m_thread, THREAD_PRIORITY_HIGHEST);
	else if (prio <= ThreadPrio_Minimum)
		res = SetThreadPriority(m_thread, THREAD_PRIORITY_LOWEST);
	else if (prio == ThreadPrio_Normal)
		res = SetThreadPriority(m_thread, THREAD_PRIORITY_NORMAL);
	else if (prio == ThreadPrio_High)
		res = SetThreadPriority(m_thread, THREAD_PRIORITY_ABOVE_NORMAL);
	else if (prio == ThreadPrio_Low)
		res = SetThreadPriority(m_thread, THREAD_PRIORITY_BELOW_NORMAL);

	m_params.prio = prio;

	return (res != FALSE);
}

bool WinThreader::ThreadHandle::Unpause()
{
	if (!m_thread)
		return false;

	if (m_state != Thread_Paused)
		return false;

	m_state = Thread_Running;

	if (ResumeThread(m_thread) == -1)
	{
		m_state = Thread_Paused;
		return false;
	}

	return true;
}

/*****************
 * EVENT SIGNALS *
 *****************/

WinThreader::WinEvent::~WinEvent()
{
	CloseHandle(m_event);
}

void WinThreader::WinEvent::Wait()
{
	WaitForSingleObject(m_event, INFINITE);
}

void WinThreader::WinEvent::Signal()
{
	SetEvent(m_event);
}

void WinThreader::WinEvent::DestroyThis()
{
	delete this;
}
