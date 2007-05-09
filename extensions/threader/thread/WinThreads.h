/**
 * vim: set ts=4 :
 * ===============================================================
 * SourceMod Threader API
 * Copyright (C) 2004-2007 AlliedModders LLC. All rights reserved.
 * ===============================================================
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Version: $Id$
 */

#ifndef _INCLUDE_WINTHREADS_H_
#define _INCLUDE_WINTHREADS_H_

#include <windows.h>
#include "IThreader.h"

using namespace SourceMod;

DWORD WINAPI Win32_ThreadGate(LPVOID param);

class WinThreader : public IThreader
{
public:
	class ThreadHandle : public IThreadHandle
	{
		friend class WinThreader;
		friend DWORD WINAPI Win32_ThreadGate(LPVOID param);
	public:
		ThreadHandle(IThreader *parent, HANDLE hthread, IThread *run, const ThreadParams *params);
		virtual ~ThreadHandle();
	public:
		virtual bool WaitForThread();
		virtual void DestroyThis();
		virtual IThreadCreator *Parent();
		virtual void GetParams(ThreadParams *ptparams);
		virtual ThreadPriority GetPriority();
		virtual bool SetPriority(ThreadPriority prio);
		virtual ThreadState GetState();
		virtual bool Unpause();
	protected:
		IThreader *m_parent;		//Parent handle
		HANDLE m_thread;			//Windows HANDLE	
		ThreadParams m_params;		//Current Parameters
		IThread *m_run;	//Runnable context
		ThreadState m_state;		//internal state
		CRITICAL_SECTION m_crit;
	};
	class WinMutex : public IMutex
	{
	public:
		WinMutex(HANDLE mutex) : m_mutex(mutex)
		{
		};
		virtual ~WinMutex();
	public:
		virtual bool TryLock();
		virtual void Lock();
		virtual void Unlock();
		virtual void DestroyThis();
	protected:
		HANDLE m_mutex;
	};
	class WinEvent : public IEventSignal
	{
	public:
		WinEvent(HANDLE event) : m_event(event)
		{
		};
		virtual ~WinEvent();
	public:
		virtual void Wait();
		virtual void Signal();
		virtual void DestroyThis();
	public:
		HANDLE m_event;
	};
public:
	IMutex *MakeMutex();
	void MakeThread(IThread *pThread);
	IThreadHandle *MakeThread(IThread *pThread, ThreadFlags flags);
	IThreadHandle *MakeThread(IThread *pThread, const ThreadParams *params);
	void GetPriorityBounds(ThreadPriority &max, ThreadPriority &min);
	void ThreadSleep(unsigned int ms);
	IEventSignal *MakeEventSignal();
	IThreadWorker *MakeWorker(bool threaded);
	void DestroyWorker(IThreadWorker *pWorker);
};

#if defined SM_DEFAULT_THREADER && !defined SM_MAIN_THREADER
#define SM_MAIN_THREADER WinThreader;
typedef class WinThreader MainThreader;
#endif

#endif //_INCLUDE_WINTHREADS_H_
