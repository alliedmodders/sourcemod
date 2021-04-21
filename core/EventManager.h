/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2010 AlliedModders LLC.  All rights reserved.
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

#ifndef _INCLUDE_SOURCEMOD_EVENTMANAGER_H_
#define _INCLUDE_SOURCEMOD_EVENTMANAGER_H_

#include "sm_globals.h"
#include "sourcemm_api.h"
#include <sm_namehashset.h>
#include <sh_list.h>
#include <sh_stack.h>
#include <IHandleSys.h>
#include <IForwardSys.h>
#include <IPluginSys.h>

class IClient;

using namespace SourceHook;

struct EventInfo
{
	EventInfo()
	{
	}
	EventInfo(IGameEvent *ev, IdentityToken_t *owner) : pEvent(ev), pOwner(owner)
	{
	}
	IGameEvent *pEvent;
	IdentityToken_t *pOwner;
	bool bDontBroadcast;
};

struct EventHook
{
	EventHook()
	{
		pPreHook = NULL;
		pPostHook = NULL;
		postCopy = false;
		refCount = 0;
	}
	IChangeableForward *pPreHook;
	IChangeableForward *pPostHook;
	bool postCopy;
	unsigned int refCount;
	std::string name;

	static inline bool matches(const char *name, const EventHook *hook)
	{
		return strcmp(name, hook->name.c_str()) == 0;
	}
	static inline uint32_t hash(const detail::CharsAndLength &key)
	{
		return key.hash();
	}
};

enum EventHookMode
{
	EventHookMode_Pre,
	EventHookMode_Post,
	EventHookMode_PostNoCopy
};

enum EventHookError
{
	EventHookErr_Okay = 0,			/**< No error */
	EventHookErr_InvalidEvent,		/**< Specified event does not exist */
	EventHookErr_NotActive,			/**< Specified event has no active hook */
	EventHookErr_InvalidCallback,	/**< Specified event does not fire specified callback */ 
};

class EventManager :
	public SMGlobalClass,
	public IHandleTypeDispatch,
	public IPluginsListener,
	public IGameEventListener2
{
public:
	EventManager();
	~EventManager();
public: // SMGlobalClass
	void OnSourceModAllInitialized();
	void OnSourceModShutdown();
public: // IHandleTypeDispatch
	void OnHandleDestroy(HandleType_t type, void *object);
public: // IPluginsListener
	void OnPluginUnloaded(IPlugin *plugin);
public: // IGameEventListener2
	void FireGameEvent(IGameEvent *pEvent);
#if SOURCE_ENGINE >= SE_LEFT4DEAD
	int GetEventDebugID();
#endif
public:
	/**
	 * Get the 'GameEvent' handle type ID.
	 */
	inline HandleType_t GetHandleType()
	{
		return m_EventType;
	}
public:
	EventHookError HookEvent(const char *name, IPluginFunction *pFunction, EventHookMode mode=EventHookMode_Post);
	EventHookError UnhookEvent(const char *name, IPluginFunction *pFunction, EventHookMode mode=EventHookMode_Post);
	EventInfo *CreateEvent(IPluginContext *pContext, const char *name, bool force=false);
	void FireEvent(EventInfo *pInfo, bool bDontBroadcast=false);
	void FireEventToClient(EventInfo *pInfo, IClient *pClient);
	void CancelCreatedEvent(EventInfo *pInfo);
private: // IGameEventManager2 hooks
	bool OnFireEvent(IGameEvent *pEvent, bool bDontBroadcast);
	bool OnFireEvent_Post(IGameEvent *pEvent, bool bDontBroadcast);
private:
	HandleType_t m_EventType;
	NameHashSet<EventHook *> m_EventHooks;
	CStack<EventInfo *> m_FreeEvents;
	CStack<EventHook *> m_EventStack;
	CStack<IGameEvent *> m_EventCopies;
};

extern EventManager g_EventManager;

#endif // _INCLUDE_SOURCEMOD_EVENTMANAGER_H_
