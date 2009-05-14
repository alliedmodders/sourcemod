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
	m_QueueLock(NULL),
	m_StateLock(NULL),
	m_PauseSignal(NULL),
	m_AddSignal(NULL),
	me(NULL),
	m_think_time(DEFAULT_THINK_TIME_MS)
{
	m_state = Worker_Invalid;
}

ThreadWorker::ThreadWorker(IThreadWorkerCallbacks *hooks, IThreader *pThreader, unsigned int thinktime) : 
	BaseWorker(hooks),
	m_Threader(pThreader),
	m_QueueLock(NULL),
	m_StateLock(NULL),
	m_PauseSignal(NULL),
	m_AddSignal(NULL),
	me(NULL),
	m_think_time(thinktime)
{
	if (m_Threader)
	{
		m_state = Worker_Stopped;
	} else {
		m_state = Worker_Invalid;
	}
}

ThreadWorker::~ThreadWorker()
{
	if (m_state != Worker_Stopped || m_state != Worker_Invalid)
	{
		Stop(true);
	}

	if (m_ThreadQueue.size())
	{
		Flush(true);
	}
}

void ThreadWorker::OnTerminate(IThreadHandle *pHandle, bool cancel)
{
	//we don't particularly care
	return;
}

void ThreadWorker::RunThread(IThreadHandle *pHandle)
{
	WorkerState this_state = Worker_Running;
	size_t num;

	if (m_pHooks)
	{
		m_pHooks->OnWorkerStart(this);
	}
    
	while (true)
	{
		/**
		 * Check number of items in the queue
		 */
		m_StateLock->Lock();
		this_state = m_state;
		m_StateLock->Unlock();
		if (this_state != Worker_Stopped)
		{
			m_QueueLock->Lock();
			num = m_ThreadQueue.size();
			if (!num)
			{
				/** 
				 * if none, wait for an item
				 */
				m_Waiting = true;
				m_QueueLock->Unlock();
				/* first check if we should end again */
				if (this_state == Worker_Stopped)
				{
					break;
				}
				m_AddSignal->Wait();
				m_Waiting = false;
			} else {
				m_QueueLock->Unlock();
			}
		}
		m_StateLock->Lock();
		this_state = m_state;
		m_StateLock->Unlock();
		if (this_state != Worker_Running)
		{
			if (this_state == Worker_Paused || this_state == Worker_Stopped)
			{
				//wait until the lock is cleared.
				if (this_state == Worker_Paused)
				{
					m_PauseSignal->Wait();
				}
				if (this_state == Worker_Stopped)
				{
					//if we're supposed to flush cleanrly, 
					// run all of the remaining frames first.
					if (!m_FlushType)
					{
						while (m_ThreadQueue.size())
						{
							RunFrame();
						}
					}
					break;
				}
			}
		}
		/**
		 * Run the frame.
		 */
		RunFrame();

		/**
		 * wait in between threads if specified
		 */
		if (m_think_time)
		{
			m_Threader->ThreadSleep(m_think_time);
		}
	}

	if (m_pHooks)
	{
		m_pHooks->OnWorkerStop(this);
	}
}

SWThreadHandle *ThreadWorker::PopThreadFromQueue()
{
	if (m_state <= Worker_Stopped && !m_QueueLock)
	{
		return NULL;
	}

	SWThreadHandle *swt;
	m_QueueLock->Lock();
	swt = BaseWorker::PopThreadFromQueue();
	m_QueueLock->Unlock();

	return swt;
}

void ThreadWorker::AddThreadToQueue(SWThreadHandle *pHandle)
{
	if (m_state <= Worker_Stopped)
	{
		return;
	}

	m_QueueLock->Lock();
	BaseWorker::AddThreadToQueue(pHandle);
	if (m_Waiting)
	{
		m_AddSignal->Signal();
	}
	m_QueueLock->Unlock();
}

WorkerState ThreadWorker::GetStatus(unsigned int *threads)
{
	WorkerState state;

	m_StateLock->Lock();
	state = BaseWorker::GetStatus(threads);
	m_StateLock->Unlock();

	return state;
}

bool ThreadWorker::Start()
{
	if (m_state == Worker_Invalid)
	{
		if (m_Threader == NULL)
		{
			return false;
		}
	} else if (m_state != Worker_Stopped) {
		return false;
	}

	m_Waiting = false;
	m_QueueLock = m_Threader->MakeMutex();
	m_StateLock = m_Threader->MakeMutex();
	m_PauseSignal = m_Threader->MakeEventSignal();
	m_AddSignal = m_Threader->MakeEventSignal();
	m_state = Worker_Running;
	ThreadParams pt;
	pt.flags = Thread_Default;
	pt.prio = ThreadPrio_Normal;
	me = m_Threader->MakeThread(this, &pt);

	return true;
}

bool ThreadWorker::Stop(bool flush_cancel)
{
	if (m_state == Worker_Invalid || m_state == Worker_Stopped)
	{
		return false;
	}

	WorkerState oldstate;

	//set new state
	m_StateLock->Lock();
	oldstate = m_state;
	m_state = Worker_Stopped;
	m_FlushType = flush_cancel;
	m_StateLock->Unlock();

	if (oldstate == Worker_Paused)
	{
		Unpause();
	} else {
		m_QueueLock->Lock();
		if (m_Waiting)
		{
			m_AddSignal->Signal();
		}
		m_QueueLock->Unlock();
	}

	me->WaitForThread();
	//destroy it
	me->DestroyThis();
	//flush all remaining events
	Flush(true);

	//free mutex locks
	m_QueueLock->DestroyThis();
	m_StateLock->DestroyThis();
	m_PauseSignal->DestroyThis();
	m_AddSignal->DestroyThis();

	//invalidizzle
	m_QueueLock = NULL;
	m_StateLock = NULL;
	m_PauseSignal = NULL;
	m_AddSignal = NULL;
	me = NULL;

	return true;
}

bool ThreadWorker::Pause()
{
	if (m_state != Worker_Running)
	{
		return false;
	}

	m_StateLock->Lock();
	m_state = Worker_Paused;
	m_StateLock->Unlock();

	return true;
}


bool ThreadWorker::Unpause()
{
	if (m_state != Worker_Paused)
	{
		return false;
	}

	m_StateLock->Lock();
	m_state = Worker_Running;
	m_StateLock->Unlock();
	m_PauseSignal->Signal();
	if (m_Waiting)
	{
		m_AddSignal->Signal();
	}

	return true;
}
