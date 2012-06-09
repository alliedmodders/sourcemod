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

#ifndef _INCLUDE_SOURCEMOD_THREADWORKER_H
#define _INCLUDE_SOURCEMOD_THREADWORKER_H

#include "BaseWorker.h"

#define DEFAULT_THINK_TIME_MS	20

class ThreadWorker : public BaseWorker, public IThread
{
public:
	ThreadWorker(IThreadWorkerCallbacks *hooks);
	ThreadWorker(IThreadWorkerCallbacks *hooks, IThreader *pThreader, unsigned int thinktime=DEFAULT_THINK_TIME_MS);
	virtual ~ThreadWorker();
public:	//IThread
	virtual void OnTerminate(IThreadHandle *pHandle, bool cancel);
	virtual void RunThread(IThreadHandle *pHandle);
public:	//IWorker
	//Controls the worker
	virtual bool Pause();
	virtual bool Unpause();
	virtual bool Start();
	virtual bool Stop(bool flush_cancel);
	//returns status and number of threads in queue
	virtual WorkerState GetStatus(unsigned int *numThreads);
public:	//BaseWorker
	virtual void AddThreadToQueue(SWThreadHandle *pHandle);
	virtual SWThreadHandle *PopThreadFromQueue();
protected:
	IThreader *m_Threader;
	IMutex *m_QueueLock;
	IMutex *m_StateLock;
	IEventSignal *m_PauseSignal;
	IEventSignal *m_AddSignal;
	IThreadHandle *me;
	unsigned int m_think_time;
	volatile bool m_Waiting;
	volatile bool m_FlushType;
};

#endif //_INCLUDE_SOURCEMOD_THREADWORKER_H
