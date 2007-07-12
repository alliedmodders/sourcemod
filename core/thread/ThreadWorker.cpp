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
