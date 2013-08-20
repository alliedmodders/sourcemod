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

#include "ThreadWorker.h"

ThreadWorker::ThreadWorker(IThreadWorkerCallbacks *hooks) : BaseWorker(hooks),
	m_Threader(NULL),
	me(NULL),
	m_think_time(DEFAULT_THINK_TIME_MS)
{
	m_state = Worker_Invalid;
}

ThreadWorker::ThreadWorker(IThreadWorkerCallbacks *hooks, IThreader *pThreader, unsigned int thinktime) : 
	BaseWorker(hooks),
	m_Threader(pThreader),
	me(NULL),
	m_think_time(thinktime)
{
	m_state = m_Threader ? Worker_Stopped : Worker_Invalid;
}

ThreadWorker::~ThreadWorker()
{
	if (m_state != Worker_Stopped || m_state != Worker_Invalid)
		Stop(true);
	
	if (m_ThreadQueue.size())
		Flush(true);
}

void ThreadWorker::OnTerminate(IThreadHandle *pHandle, bool cancel)
{
	//we don't particularly care
	return;
}

void ThreadWorker::RunThread(IThreadHandle *pHandle)
{
	if (m_pHooks)
		m_pHooks->OnWorkerStart(this);

	ke::AutoLock lock(&monitor_);

	while (true)
	{
		if (m_state == Worker_Paused)
		{
			// Wait until we're told to wake up.
			monitor_.Wait();
			continue;
		}

		if (m_state == Worker_Stopped)
		{
			// We've been told to stop entirely. If we've also been told to
			// flush the queue, do that now.
			while (!m_ThreadQueue.empty())
			{
				// Release the lock since PopThreadFromQueue() will re-acquire it. The
				// main thread is blocking anyway.
				ke::AutoUnlock unlock(&monitor_);
				RunFrame();
			}
			assert(m_state == Worker_Stopped);
			return;
		}

		assert(m_state == Worker_Running);

		// Process one frame.
		WorkerState oldstate = m_state;
		{
			ke::AutoUnlock unlock(&monitor_);
			RunFrame();
		}

		// If the state changed, loop back and process the new state.
		if (m_state != oldstate)
			continue;
		
		// If the thread queue is now empty, wait for a signal. Otherwise, if
		// we're on a delay, wait for either a notification or a timeout to
		// process the next item. If the queue has items and we don't have a
		// delay, then we just loop around and keep processing.
		if (m_ThreadQueue.empty())
			monitor_.Wait();
		else if (m_think_time)
			monitor_.Wait(m_think_time);
	}

	{
		ke::AutoUnlock unlock(&monitor_);
		if (m_pHooks)
			m_pHooks->OnWorkerStop(this);
	}
}

SWThreadHandle *ThreadWorker::PopThreadFromQueue()
{
	ke::AutoLock lock(&monitor_);
	if (m_state <= Worker_Stopped)
		return NULL;

	return BaseWorker::PopThreadFromQueue();
}

void ThreadWorker::AddThreadToQueue(SWThreadHandle *pHandle)
{
	ke::AutoLock lock(&monitor_);
	if (m_state <= Worker_Stopped)
		return;

	BaseWorker::AddThreadToQueue(pHandle);
	monitor_.Notify();
}

WorkerState ThreadWorker::GetStatus(unsigned int *threads)
{
	ke::AutoLock lock(&monitor_);
	return BaseWorker::GetStatus(threads);
}

void ThreadWorker::SetThinkTimePerFrame(unsigned int thinktime)
{
	m_think_time = thinktime;
}

bool ThreadWorker::Start()
{
	if (m_state == Worker_Invalid && m_Threader == NULL)
		return false;

	if (m_state != Worker_Stopped)
		return false;

	m_state = Worker_Running;

	ThreadParams pt;
	pt.flags = Thread_Default;
	pt.prio = ThreadPrio_Normal;
	me = m_Threader->MakeThread(this, &pt);
	return true;
}

bool ThreadWorker::Stop(bool flush_cancel)
{
	// Change the state to signal a stop, and then trigger a notify.
	{
		ke::AutoLock lock(&monitor_);
		if (m_state == Worker_Invalid || m_state == Worker_Stopped)
			return false;

		m_state = Worker_Stopped;
		m_FlushType = flush_cancel;
		monitor_.Notify();
	}

	me->WaitForThread();
	//destroy it
	me->DestroyThis();
	//flush all remaining events
	Flush(true);

	me = NULL;
	return true;
}

bool ThreadWorker::Pause()
{
	if (m_state != Worker_Running)
		return false;

	ke::AutoLock lock(&monitor_);
	m_state = Worker_Paused;
	monitor_.Notify();
	return true;
}


bool ThreadWorker::Unpause()
{
	if (m_state != Worker_Paused)
		return false;

	ke::AutoLock lock(&monitor_);
	m_state = Worker_Running;
	monitor_.Notify();
	return true;
}
