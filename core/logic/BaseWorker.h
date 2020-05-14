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

#ifndef _INCLUDE_SOURCEMOD_BASEWORKER_H
#define _INCLUDE_SOURCEMOD_BASEWORKER_H

#include "sh_list.h"
#include "ThreadSupport.h"

#define SM_DEFAULT_THREADS_PER_FRAME	1

class BaseWorker;

//SW = Simple Wrapper
class SWThreadHandle : public IThreadHandle
{
	friend class BaseWorker;
	friend class CompatWorker;
public:
	SWThreadHandle(IThreadCreator *parent, const ThreadParams *p, IThread *thread);
	IThread *GetThread();
public:
	//NOTE: We don't support this by default.
	//It's specific usage that'd require many mutexes
	virtual bool WaitForThread();
public:
	virtual void DestroyThis();
	virtual IThreadCreator *Parent();
	virtual void GetParams(ThreadParams *ptparams);
public:
	//Priorities not supported by default.
	virtual ThreadPriority GetPriority();
	virtual bool SetPriority(ThreadPriority prio);
public:
	virtual ThreadState GetState();
	virtual bool Unpause();
private:
	ThreadState m_state;
	ThreadParams m_params;
	IThreadCreator *m_parent;
	IThread *pThread;
};

class BaseWorker : public IThreadWorker
{
public:
	BaseWorker(IThreadWorkerCallbacks *hooks);
	virtual ~BaseWorker();
public:	//IWorker
	virtual unsigned int RunFrame();
	//Controls the worker
	virtual bool Pause();
	virtual bool Unpause();
	virtual bool Start();
	virtual bool Stop(bool flush_cancel);
	//Flushes out any remaining threads
	virtual unsigned int Flush(bool flush_cancel);
	//returns status and number of threads in queue
	virtual WorkerState GetStatus(unsigned int *numThreads);
	virtual void SetMaxThreadsPerFrame(unsigned int threads);
	virtual void SetThinkTimePerFrame(unsigned int thinktime) {}
public:	//IThreadCreator
	virtual void MakeThread(IThread *pThread);
	virtual IThreadHandle *MakeThread(IThread *pThread, ThreadFlags flags);
	virtual IThreadHandle *MakeThread(IThread *pThread, const ThreadParams *params);
	virtual void GetPriorityBounds(ThreadPriority &max, ThreadPriority &min);
public:	//BaseWorker
	virtual void AddThreadToQueue(SWThreadHandle *pHandle);
	virtual SWThreadHandle *PopThreadFromQueue();
protected:
	SourceHook::List<SWThreadHandle *> m_ThreadQueue;
	unsigned int m_perFrame;
	volatile WorkerState m_state;
	IThreadWorkerCallbacks *m_pHooks;
};

#endif //_INCLUDE_SOURCEMOD_BASEWORKER_H
