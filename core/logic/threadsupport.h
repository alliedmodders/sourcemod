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

#ifndef _INCLUDE_SOURCEMOD_THREAD_SUPPORT_H
#define _INCLUDE_SOURCEMOD_THREAD_SUPPORT_H

#include <IThreader.h>
#include <am-thread-utils.h>
#include <am-utility.h>

using namespace SourceMod;

class CompatMutex : public IMutex
{
public:
	bool TryLock() {
		return mutex_.TryLock();
	}
	void Lock() {
		mutex_.Lock();
	}
	void Unlock() {
		mutex_.Unlock();
	}
	void DestroyThis() {
		delete this;
	}
private:
	ke::Mutex mutex_;
};

class CompatCondVar : public IEventSignal
{
public:
	void Wait() {
		ke::AutoLock lock(&cv_);
		cv_.Wait();
	}
	void Signal() {
		ke::AutoLock lock(&cv_);
		cv_.Notify();
	}
	void DestroyThis() {
		delete this;
	}
private:
	ke::ConditionVariable cv_;
};

extern IThreader *g_pThreader;

#endif //_INCLUDE_SOURCEMOD_THREAD_SUPPORT_H
