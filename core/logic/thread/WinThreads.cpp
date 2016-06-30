/**
 * vim: set ts=4 sw=4 tw=99 noet:
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
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
	return new CompatMutex();
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

void WinThreader::ThreadHandle::Run()
{
	// Wait for an unpause if necessary.
	{
		ke::AutoLock lock(&suspend_);
		if (m_state == Thread_Paused)
			suspend_.Wait();
	}
	
	m_run->RunThread(this);
	m_state = Thread_Done;
	m_run->OnTerminate(this, false);
	if (m_params.flags & Thread_AutoRelease)
		delete this;
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

	ThreadHandle* pHandle = new ThreadHandle(this, pThread, params);

	pHandle->m_thread = new ke::Thread([pHandle]() -> void {
		pHandle->Run();
	}, "SourceMod");
	if (!pHandle->m_thread->Succeeded()) {
		delete pHandle;
		return nullptr;
	}

	if (pHandle->m_params.prio != ThreadPrio_Normal)
		pHandle->SetPriority(pHandle->m_params.prio);

	if (!(params->flags & Thread_CreateSuspended))
		pHandle->Unpause();

	return pHandle;
}

IEventSignal *WinThreader::MakeEventSignal()
{
	return new CompatCondVar();
}

/******************
 * Thread Handles *
 ******************/

WinThreader::ThreadHandle::ThreadHandle(IThreader *parent, IThread *run, const ThreadParams *params) : 
	m_parent(parent), m_run(run), m_params(*params),
	m_state(Thread_Paused)
{
}

WinThreader::ThreadHandle::~ThreadHandle()
{
}

bool WinThreader::ThreadHandle::WaitForThread()
{
	if (!m_thread)
		return false;

	m_thread->Join();
	return true;
}

ThreadState WinThreader::ThreadHandle::GetState()
{
	return m_state;
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
	BOOL res = FALSE;

	if (prio >= ThreadPrio_Maximum)
		res = SetThreadPriority(m_thread->handle(), THREAD_PRIORITY_HIGHEST);
	else if (prio <= ThreadPrio_Minimum)
		res = SetThreadPriority(m_thread->handle(), THREAD_PRIORITY_LOWEST);
	else if (prio == ThreadPrio_Normal)
		res = SetThreadPriority(m_thread->handle(), THREAD_PRIORITY_NORMAL);
	else if (prio == ThreadPrio_High)
		res = SetThreadPriority(m_thread->handle(), THREAD_PRIORITY_ABOVE_NORMAL);
	else if (prio == ThreadPrio_Low)
		res = SetThreadPriority(m_thread->handle(), THREAD_PRIORITY_BELOW_NORMAL);

	m_params.prio = prio;

	return (res != FALSE);
}

bool WinThreader::ThreadHandle::Unpause()
{
	if (m_state != Thread_Paused)
		return false;

	ke::AutoLock lock(&suspend_);
	m_state = Thread_Running;
	suspend_.Notify();
	return true;
}
