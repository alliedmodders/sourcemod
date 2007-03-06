/**
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

#ifndef _INCLUDE_SOURCEMOD_CGAMEEVENTMANAGER_H_
#define _INCLUDE_SOURCEMOD_CGAMEEVENTMANAGER_H_

#include "sm_globals.h"
#include "sourcemm_api.h"
#include "sm_trie.h"
#include <sh_list.h>
#include <sh_stack.h>
#include <IHandleSys.h>
#include <IForwardSys.h>
#include <IPluginSys.h>

using namespace SourceHook;

//#define EVENT_PASSTHRU	(1<<0)
#define EVENT_PASSTHRU_ALL	(1<<1)

struct EventInfo
{
	IGameEvent *pEvent;
	IdentityToken_t *pOwner;
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
	EventInfo *CreateEvent(IPluginContext *pContext, const char *name);
	void FireEvent(EventInfo *pInfo, int flags=0, bool bDontBroadcast=false);
private: // IGameEventManager2 hooks
	bool OnFireEvent(IGameEvent *pEvent, bool bDontBroadcast);
	bool OnFireEvent_Post(IGameEvent *pEvent, bool bDontBroadcast);
private:
	HandleType_t m_EventType;
	bool m_NotifyPlugins;
	const char *m_EventName;
	IGameEvent *m_EventCopy;
	Trie *m_EventHooks;
	CStack<EventInfo *> m_FreeEvents;
};

extern EventManager g_EventManager;

#endif // _INCLUDE_SOURCEMOD_CGAMEEVENTMANAGER_H_
