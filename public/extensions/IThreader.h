/**
 * ===============================================================
 * SourceMod (C)2004-2007 AlliedModders LLC.  All rights reserved.
 * ===============================================================
 *
 *  This file is part of the SourceMod/SourcePawn SDK.  This file may only be used 
 * or modified under the Terms and Conditions of its License Agreement, which is found 
 * in LICENSE.txt.  The Terms and Conditions for making SourceMod extensions/plugins 
 * may change at any time.  To view the latest information, see:
 *   http://www.sourcemod.net/license.php
 *
 * Version: $Id$
 */

#ifndef _INCLUDE_SOURCEMOD_THREADER_H
#define _INCLUDE_SOURCEMOD_THREADER_H

/**
 * @file IThreader.h
 * @brief Contains platform independent routines for threading.
 */

#include <IShareSys.h>

#define SMINTERFACE_THREADER_NAME		"IThreader"
#define SMINTERFACE_THREADER_VERSION	1

namespace SourceMod
{
	/**
	 * @brief Thread creation flags
	 */
	enum ThreadFlags
	{
		Thread_Default = 0,
		/**
		 * @brief Auto-release handle on finish
		 *
		 * You are not guaranteed the handle for this is valid after
		 * calling MakeThread(), so never use it until OnTerminate is called.
		 */
		Thread_AutoRelease = 1,
		/**
		 * @brief Thread is created "suspended", meaning it is inactive until unpaused.
		 */
		Thread_CreateSuspended = 2,
	};

	/**
	 * @brief Specifies thread priority levels.
	 */
	enum ThreadPriority
	{
		ThreadPrio_Minimum = -8,
		ThreadPrio_Low = -3,
		ThreadPrio_Normal = 0,
		ThreadPrio_High = 3,
		ThreadPrio_Maximum = 8,
	};

	/**
	 * @brief The current state of a thread.
	 */
	enum ThreadState
	{
		Thread_Running = 0,
		Thread_Paused = 1,
		Thread_Done = 2,
	};

	/**
	 * @brief Thread-specific parameters.
	 */
	struct ThreadParams
	{
		/** Constructor */
		ThreadParams() : 
			flags(Thread_Default), 
			prio(ThreadPrio_Normal)
		{
		};
		ThreadFlags flags;		/**< Flags to set on the thread */
		ThreadPriority prio;	/**< Priority to set on the thread */
	};

	class IThreadCreator;

	/**
	 * @brief Describes a handle to a thread.
	 */
	class IThreadHandle
	{
	public:
		/** Virtual destructor */
		virtual ~IThreadHandle() { };
	public:
		/**
		 * @brief Pauses parent thread until this thread completes.
		 * 
		 * @return			True if successful, false otherwise.
		 */
		virtual bool WaitForThread() =0;

		/**
		 * @brief Destroys the thread handle.  This will not necessarily cancel the thread.
		 */
		virtual void DestroyThis() =0;

		/**
		 * @brief Returns the parent threader.
		 *
		 * @return			IThreadCreator that created this thread.
		 */
		virtual IThreadCreator *Parent() =0;

		/**
		 * @brief Returns the thread states.
		 *
		 * @param ptparams	Pointer to a ThreadParams buffer.
		 */
		virtual void GetParams(ThreadParams *ptparams) =0;

		/**
		 * @brief Returns the thread priority.
		 *
		 * @return			Thread priority.
		 */
		virtual ThreadPriority GetPriority() =0;

		/**
		 * @brief Sets thread priority.
		 * NOTE: On Linux, this always returns false.
		 *
		 * @param prio		Thread priority to set.
		 * @return			True if successful, false otherwise.
		 */
		virtual bool SetPriority(ThreadPriority prio) =0;

		/**
		 * @brief Returns the thread state.
		 * 
		 * @return			Current thread state.
		 */
		virtual ThreadState GetState() =0;

		/**
		 * @brief Attempts to unpause a paused thread.
		 *
		 * @return			True on success, false otherwise.
		 */
		virtual bool Unpause() =0;
	};

	/**
	 * @brief Handles a single thread's execution.
	 */
	class IThread
	{
	public:
		/** Virtual destructor */
		virtual ~IThread() { };
	public:
		/**
		 * @brief Called when the thread runs (in its own thread).
		 *
		 * @param pHandle		Pointer to the thread's handle.
		 */
		virtual void RunThread(IThreadHandle *pHandle) =0;

		/**
		 * @brief Called when the thread terminates.  This occurs inside the thread as well.
		 *
		 * @param pHandle		Pointer to the thread's handle.
		 * @param cancel		True if the thread did not finish, false otherwise.
		 */
		virtual void OnTerminate(IThreadHandle *pHandle, bool cancel) =0;
	};


	/**
	 * @brief Describes a thread creator
	 */
	class IThreadCreator
	{
	public:
		/** Virtual Destructor */
		virtual ~IThreadCreator() { };
	public:
		/**
		 * @brief Creates a basic thread.
		 *
		 * @param pThread		IThread pointer for callbacks.
		 */
		virtual void MakeThread(IThread *pThread) =0;

		/**
		 * @brief Creates a thread with specific options.
		 *
		 * @param pThread		IThread pointer for callbacks.
		 * @param flags			Flags for the thread.
		 * @return				IThreadHandle pointer (must be released).
		 */
		virtual IThreadHandle *MakeThread(IThread *pThread, ThreadFlags flags) =0;

		/**
		 * @brief Creates a thread with specific options.
		 *
		 * @param pThread		IThread pointer for callbacks.
		 * @param params		Extended options for the thread.
		 * @return				IThreadHandle pointer (must be released).
		 */
		virtual IThreadHandle *MakeThread(IThread *pThread, const ThreadParams *params) =0;

		/**
		 * @brief Returns the priority bounds.
		 * Note: On Linux, the min and max are both Thread_Normal.
		 *
		 * @param max			Stores the maximum priority level.
		 * @param min			Stores the minimum priority level.
		 */
		virtual void GetPriorityBounds(ThreadPriority &max, ThreadPriority &min) =0;
	};

	/**
	 * @brief Describes a simple locking mutex.
	 */
	class IMutex
	{
	public:
		/** Virtual Destructor */
		virtual ~IMutex() { };
	public:
		/**
		 * @brief Attempts to lock, but returns instantly.
		 *
		 * @return		True if lock was obtained, false otherwise.
		 */
		virtual bool TryLock() =0;

		/**
 		 * @brief Attempts to lock by waiting for release.
		 */
		virtual void Lock() =0;

		/**
		 * @brief Unlocks the mutex.
		 */
		virtual void Unlock() =0;

		/**
		 * @brief Destroys the mutex handle.
		 */
		virtual void DestroyThis() =0;
	};

	/**
	 * @brief Describes a simple "condition variable"/signal lock.
	 */
	class IEventSignal
	{
	public:
		/** Virtual Destructor */
		virtual ~IEventSignal() { };
	public:
		/**
		 * @brief Waits for a signal.
		 */
		virtual void Wait() =0;

		/** 
		 * @brief Triggers the signal and resets the signal after triggering.
		 */
		virtual void Signal() =0;

		/**
		 * @brief Frees the signal handle.
		 */
		virtual void DestroyThis() =0;
	};

	/**
	 * @brief Describes possible worker states
	 */
	enum WorkerState
	{
		Worker_Invalid = -3,
		Worker_Stopped = -2,
		Worker_Paused = -1,
		Worker_Running,
	};

	/**
	 * @brief This is a "worker pool."  A single thread places tasks in a queue.
	 * Each IThread is then a task, rather than its own separate thread.
	 */
	class IThreadWorker : public IThreadCreator
	{
	public:
		/** Virtual Destructor */
		virtual ~IThreadWorker()
		{
		};
	public:
		/**
		 * @brief Runs one "frame" of the worker.
		 *
		 * @return			Number of tasks processed.
		 */
		virtual unsigned int RunFrame() =0;
	public:
		/**
		 * @brief Pauses the worker.
		 *
		 * @return			True on success, false otherwise.
		 */
		virtual bool Pause() =0;

		/**
		 * @brief Unpauses the worker.
		 *
		 * @return			True on success, false otherwise.
		 */
		virtual bool Unpause() =0;

		/** 
		 * @brief Starts the worker thread.
		 *
		 * @return			True on success, false otherwise.
		 */
		virtual bool Start() =0;

		/**
		 * @brief Stops the worker thread.
		 *
		 * @param flush		If true, all remaining tasks will be cancelled.
		 *					Otherwise, the threader will wait until the queue is empty.
		 * @return			True on success, false otherwise.
		 */
		virtual bool Stop(bool flush) =0;

		/**
		 * @brief Returns the status of the worker.
		 *
		 * @param numThreads	Pointer to store number of threads in the queue.
		 * @return				State of the worker.
		 */
		virtual WorkerState GetStatus(unsigned int *numThreads) =0;
	};

	/**
	 * @brief Describes a threading system
	 */
	class IThreader : public SMInterface, public IThreadCreator
	{
	public:
		virtual const char *GetInterfaceName()
		{
			return SMINTERFACE_THREADER_NAME;
		}
		virtual unsigned int GetInterfaceVersion()
		{
			return SMINTERFACE_THREADER_VERSION;
		}
	public:
		/**
		 * @brief Creates a mutex (mutual exclusion lock).
		 *
		 * @return			A new IMutex pointer (must be destroyed).
		 */
		virtual IMutex *MakeMutex() =0;

		/**
		 * @brief Sleeps the calling thread for a number of milliseconds.
		 *
		 * @param ms		Millisecond count to sleep.
		 */
		virtual void ThreadSleep(unsigned int ms) =0;

		/**
		 * @brief Creates a non-signalled event.
		 *
		 * @return			A new IEventSignal pointer (must be destroyed).
		 */
		virtual IEventSignal *MakeEventSignal() =0;

		/**
		 * @brief Creates a thread worker.
		 *
		 * @param threaded	If true, the worker will be threaded.
		 *					If false, the worker will require manual frame execution.
		 * @return			A new IThreadWorker pointer (must be destroyed).
		 */
		virtual IThreadWorker *MakeWorker(bool threaded) =0;

		/**
		 * @brief Destroys an IThreadWorker pointer.
		 *
		 * @param pWorker	IThreadWorker pointer to destroy.
		 */
		virtual void DestroyWorker(IThreadWorker *pWorker) =0;
	};
};

#endif //_INCLUDE_SOURCEMOD_THREADER_H
