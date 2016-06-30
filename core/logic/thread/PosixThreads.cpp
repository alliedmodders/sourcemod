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

#include <unistd.h>
#include "PosixThreads.h"
#include "ThreadWorker.h"

IThreadWorker *PosixThreader::MakeWorker(IThreadWorkerCallbacks *hooks, bool threaded)
{
	if (threaded)
	{
		return new ThreadWorker(hooks, this, DEFAULT_THINK_TIME_MS);
	} else {
		return new BaseWorker(hooks);
	}
}

void PosixThreader::DestroyWorker(IThreadWorker *pWorker)
{
	delete pWorker;
}

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
	return new CompatMutex();
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

void PosixThreader::ThreadHandle::Run()
{
	// Wait for an unpause if necessary.
	{
		ke::AutoLock lock(&m_runlock);
		if (m_state == Thread_Paused)
			m_runlock.Wait();
	}

	m_run->RunThread(this);
	m_state = Thread_Done;
	m_run->OnTerminate(this, false);

	if (m_params.flags & Thread_AutoRelease)
		delete this;
}

ThreadParams g_defparams;
IThreadHandle *PosixThreader::MakeThread(IThread *pThread, const ThreadParams *params)
{
	if (params == NULL)
		params = &g_defparams;

	ThreadHandle* pHandle = new ThreadHandle(this, pThread, params);

	pHandle->m_thread = new ke::Thread([pHandle]() -> void {
		pHandle->Run();
	}, "SourceMod");
	if (!pHandle->m_thread->Succeeded()) {
		delete pHandle;
		return NULL;
	}

	if (!(params->flags & Thread_CreateSuspended))
		pHandle->Unpause();

	return pHandle;
}

IEventSignal *PosixThreader::MakeEventSignal()
{
	return new CompatCondVar();
}

/******************
* Thread Handles *
******************/

PosixThreader::ThreadHandle::ThreadHandle(IThreader *parent, IThread *run, const ThreadParams *params) : 
	m_parent(parent), m_params(*params), m_run(run), m_state(Thread_Paused)
{
}

PosixThreader::ThreadHandle::~ThreadHandle()
{
}

bool PosixThreader::ThreadHandle::WaitForThread()
{
	if (!m_thread)
		return false;

	m_thread->Join();
	return true;
}

ThreadState PosixThreader::ThreadHandle::GetState()
{
	return m_state;
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

	ke::AutoLock lock(&m_runlock);
	m_state = Thread_Running;
	m_runlock.Notify();
	return true;
}

