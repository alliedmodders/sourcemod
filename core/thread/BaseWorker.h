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
	BaseWorker();
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
public:	//IThreadCreator
	virtual void MakeThread(IThread *pThread);
	virtual IThreadHandle *MakeThread(IThread *pThread, ThreadFlags flags);
	virtual IThreadHandle *MakeThread(IThread *pThread, const ThreadParams *params);
	virtual void GetPriorityBounds(ThreadPriority &max, ThreadPriority &min);
public:	//BaseWorker
	virtual void AddThreadToQueue(SWThreadHandle *pHandle);
	virtual SWThreadHandle *PopThreadFromQueue();
	virtual void SetMaxThreadsPerFrame(unsigned int threads);
	virtual unsigned int GetMaxThreadsPerFrame();
protected:
	SourceHook::List<SWThreadHandle *> m_ThreadQueue;
	unsigned int m_perFrame;
	volatile WorkerState m_state;
};

#endif //_INCLUDE_SOURCEMOD_BASEWORKER_H
