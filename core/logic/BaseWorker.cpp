/**
 * vim: set ts=4 :
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

#include "BaseWorker.h"

BaseWorker::BaseWorker(IThreadWorkerCallbacks *hooks) : 
	m_perFrame(SM_DEFAULT_THREADS_PER_FRAME),
	m_state(Worker_Stopped),
	m_pHooks(hooks)
{
}

BaseWorker::~BaseWorker()
{
	if (m_state != Worker_Stopped && m_state != Worker_Invalid)
		Stop(true);

	if (m_ThreadQueue.size())
		Flush(true);
}

void BaseWorker::MakeThread(IThread *pThread)
{
	ThreadParams pt;

	pt.flags = Thread_AutoRelease;
	pt.prio = ThreadPrio_Normal;

	MakeThread(pThread, &pt);
}

IThreadHandle *BaseWorker::MakeThread(IThread *pThread, ThreadFlags flags)
{
	ThreadParams pt;

	pt.flags = flags;
	pt.prio = ThreadPrio_Normal;

	return MakeThread(pThread, &pt);
}

IThreadHandle *BaseWorker::MakeThread(IThread *pThread, const ThreadParams *params)
{
	if (m_state != Worker_Running)
		return NULL;

	SWThreadHandle *swt = new SWThreadHandle(this, params, pThread);

	AddThreadToQueue(swt);

	return swt;
}

void BaseWorker::GetPriorityBounds(ThreadPriority &max, ThreadPriority &min)
{
	max = ThreadPrio_Normal;
	min = ThreadPrio_Normal;
}

unsigned int BaseWorker::Flush(bool flush_cancel)
{
	SWThreadHandle *swt;
	unsigned int num = 0;

	while ((swt=PopThreadFromQueue()) != NULL)
	{
		swt->m_state = Thread_Done;
		if (!flush_cancel)
			swt->pThread->RunThread(swt);
		swt->pThread->OnTerminate(swt, flush_cancel);
		if (swt->m_params.flags & Thread_AutoRelease)
			delete swt;
		num++;
	}

	return num;
}

SWThreadHandle *BaseWorker::PopThreadFromQueue()
{
	if (!m_ThreadQueue.size())
		return NULL;

	SourceHook::List<SWThreadHandle *>::iterator begin;
	SWThreadHandle *swt;

	begin = m_ThreadQueue.begin();
	swt = (*begin);
	m_ThreadQueue.erase(begin);

	return swt;
}

void BaseWorker::AddThreadToQueue(SWThreadHandle *pHandle)
{
	m_ThreadQueue.push_back(pHandle);
}

WorkerState BaseWorker::GetStatus(unsigned int *threads)
{
	if (threads)
		*threads = m_perFrame;

	return m_state;
}

unsigned int BaseWorker::RunFrame()
{
	unsigned int done = 0;
	unsigned int max = m_perFrame;
	SWThreadHandle *swt = NULL;
	IThread *pThread = NULL;

	while (done < max)
	{
		if ((swt=PopThreadFromQueue()) == NULL)
		{
			break;
		}
		pThread = swt->pThread;
		swt->m_state = Thread_Running;
		pThread->RunThread(swt);
		swt->m_state = Thread_Done;
		pThread->OnTerminate(swt, false);
		if (swt->m_params.flags & Thread_AutoRelease)
		{
			delete swt;
		}
		done++;
	}

	return done;
}

void BaseWorker::SetMaxThreadsPerFrame(unsigned int threads)
{
	m_perFrame = threads;
}

bool BaseWorker::Start()
{
	if (m_state != Worker_Invalid && m_state != Worker_Stopped)
	{
		return false;
	}

	m_state = Worker_Running;

	if (m_pHooks)
	{
		m_pHooks->OnWorkerStart(this);
	}

    return true;
}

bool BaseWorker::Stop(bool flush_cancel)
{
	if (m_state == Worker_Invalid || m_state == Worker_Stopped)
	{
		return false;
	}

	if (m_state == Worker_Paused)
	{
		if (!Unpause())
		{
			return false;
		}
	}

	m_state = Worker_Stopped;
	Flush(flush_cancel);

	if (m_pHooks)
	{
		m_pHooks->OnWorkerStop(this);
	}

	return true;
}

bool BaseWorker::Pause()
{
	if (m_state != Worker_Running)
	{
		return false;
	}

	m_state = Worker_Paused;

	return true;
}


bool BaseWorker::Unpause()
{
	if (m_state != Worker_Paused)
	{
		return false;
	}

	m_state = Worker_Running;

	return true;
}

/***********************
 * THREAD HANDLE STUFF *
 ***********************/

void SWThreadHandle::DestroyThis()
{
	delete this;
}

void SWThreadHandle::GetParams(ThreadParams *p)
{
	*p = m_params;
}

ThreadPriority SWThreadHandle::GetPriority()
{
	return m_params.prio;
}

ThreadState SWThreadHandle::GetState()
{
	return m_state;
}

IThreadCreator *SWThreadHandle::Parent()
{
	return m_parent;
}

bool SWThreadHandle::SetPriority(ThreadPriority prio)
{
	if (m_params.prio != ThreadPrio_Normal)
		return false;

	m_params.prio = prio;

	return true;
}

bool SWThreadHandle::Unpause()
{
	if (m_state != Thread_Paused)
		return false;

	m_state = Thread_Running;

	return true;
}

bool SWThreadHandle::WaitForThread()
{
	return false;
}

SWThreadHandle::SWThreadHandle(IThreadCreator *parent, const ThreadParams *p, IThread *thread) : 
	m_state(Thread_Paused), m_params(*p), m_parent(parent), pThread(thread)
{
}

IThread *SWThreadHandle::GetThread()
{
	return pThread;
}
