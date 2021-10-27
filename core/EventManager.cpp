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

#include "EventManager.h"
#include "sm_stringutil.h"
#include "PlayerManager.h"

#include "logic_bridge.h"
#include <bridge/include/IScriptManager.h>

EventManager g_EventManager;

SH_DECL_HOOK2(IGameEventManager2, FireEvent, SH_NOATTRIB, 0, bool, IGameEvent *, bool);

const ParamType GAMEEVENT_PARAMS[] = {Param_Cell, Param_String, Param_Cell};
typedef List<EventHook *> EventHookList;

class EventForwardFilter : public IForwardFilter
{
	EventInfo *pEventInfo;
public:
	EventForwardFilter(EventInfo *pEventInfo) : pEventInfo(pEventInfo)
	{
	}

	void Preprocess(IPluginFunction *fun, FwdParamInfo *params)
	{
		params[2].val = pEventInfo->bDontBroadcast ? 1 : 0;
	}
};

EventManager::EventManager() : m_EventType(0)
{
}

EventManager::~EventManager()
{
	/* Free memory used by EventInfo structs if any */
	CStack<EventInfo *>::iterator iter;
	for (iter = m_FreeEvents.begin(); iter != m_FreeEvents.end(); iter++)
	{
		delete (*iter);
	}

	m_FreeEvents.popall();
}

void EventManager::OnSourceModAllInitialized()
{
	/* Add a hook for IGameEventManager2::FireEvent() */
	SH_ADD_HOOK(IGameEventManager2, FireEvent, gameevents, SH_MEMBER(this, &EventManager::OnFireEvent), false);
	SH_ADD_HOOK(IGameEventManager2, FireEvent, gameevents, SH_MEMBER(this, &EventManager::OnFireEvent_Post), true);

	HandleAccess sec;

	/* Handle access security for 'GameEvent' handle type */
	sec.access[HandleAccess_Read] = 0;
	sec.access[HandleAccess_Delete] = HANDLE_RESTRICT_IDENTITY | HANDLE_RESTRICT_OWNER;
	sec.access[HandleAccess_Clone] = HANDLE_RESTRICT_IDENTITY | HANDLE_RESTRICT_OWNER;

	/* Create the 'GameEvent' handle type */
	m_EventType = handlesys->CreateType("GameEvent", this, 0, NULL, &sec, g_pCoreIdent, NULL);
}

void EventManager::OnSourceModShutdown()
{
	/* Remove hook for IGameEventManager2::FireEvent() */
	SH_REMOVE_HOOK(IGameEventManager2, FireEvent, gameevents, SH_MEMBER(this, &EventManager::OnFireEvent), false);
	SH_REMOVE_HOOK(IGameEventManager2, FireEvent, gameevents, SH_MEMBER(this, &EventManager::OnFireEvent_Post), true);

	/* Remove the 'GameEvent' handle type */
	handlesys->RemoveType(m_EventType, g_pCoreIdent);

	/* Remove ourselves as listener for events */
	gameevents->RemoveListener(this);
}

void EventManager::OnHandleDestroy(HandleType_t type, void *object)
{
	EventInfo *pInfo = static_cast<EventInfo *>(object);

	/* Should only free event when created by a plugin */
	if (pInfo->pOwner)
	{
		/* Free IGameEvent */
		gameevents->FreeEvent(pInfo->pEvent);

		/* Add EventInfo struct to free event stack */
		m_FreeEvents.push(pInfo);
	}
}

void EventManager::OnPluginUnloaded(IPlugin *plugin)
{
	EventHookList *pHookList;
	EventHookList::iterator iter;
	EventHook *pHook;

	// If plugin has an event hook list...
	if (plugin->GetProperty("EventHooks", reinterpret_cast<void **>(&pHookList), true))
	{
		for (iter = pHookList->begin(); iter != pHookList->end(); iter++)
		{
			pHook = (*iter);

			if (--pHook->refCount == 0)
			{
				if (pHook->pPreHook)
				{
					forwardsys->ReleaseForward(pHook->pPreHook);
				}

				if (pHook->pPostHook)
				{
					forwardsys->ReleaseForward(pHook->pPostHook);
				}

				delete pHook;
			}
		}

		delete pHookList;
	}
}

/* IGameEventListener2::FireGameEvent */
void EventManager::FireGameEvent(IGameEvent *pEvent)
{
	/* Not going to do anything here.
	   Just need to add ourselves as a listener to make our hook on IGameEventManager2::FireEvent work */
}

#if SOURCE_ENGINE >= SE_LEFT4DEAD
int EventManager::GetEventDebugID()
{
	return EVENT_DEBUG_ID_INIT;
}
#endif

EventHookError EventManager::HookEvent(const char *name, IPluginFunction *pFunction, EventHookMode mode)
{
	EventHook *pHook;

	/* If we aren't listening to this event... */
	if (!gameevents->FindListener(this, name))
	{
		/* Then add ourselves */
		if (!gameevents->AddListener(this, name, true))
		{
			/* If event doesn't exist... */
			return EventHookErr_InvalidEvent;
		}
	}

	/* If a hook structure does not exist... */
	if (!m_EventHooks.retrieve(name, &pHook))
	{
		EventHookList *pHookList;
		IPlugin *plugin = scripts->FindPluginByContext(pFunction->GetParentContext()->GetContext());

		/* Check plugin for an existing EventHook list */
		if (!plugin->GetProperty("EventHooks", (void **)&pHookList))
		{
			pHookList = new EventHookList();
			plugin->SetProperty("EventHooks", pHookList);
		}

		/* Create new GameEventHook structure */
		pHook = new EventHook();

		if (mode == EventHookMode_Pre)
		{
			/* Create forward for a pre hook */
			pHook->pPreHook = forwardsys->CreateForwardEx(NULL, ET_Hook, 3, GAMEEVENT_PARAMS);
			/* Add to forward list */
			pHook->pPreHook->AddFunction(pFunction);
		} else {
			/* Create forward for a post hook */
			pHook->pPostHook = forwardsys->CreateForwardEx(NULL, ET_Ignore, 3, GAMEEVENT_PARAMS);
			/* Should we copy data from a pre hook to the post hook? */
			pHook->postCopy = (mode == EventHookMode_Post);
			/* Add to forward list */
			pHook->pPostHook->AddFunction(pFunction);
		}

		/* Cache the name for post hooks */
		pHook->name = name;

		/* Increase reference count */
		pHook->refCount++;

		/* Add hook structure to hook lists */
		pHookList->push_back(pHook);
		m_EventHooks.insert(name, pHook);

		return EventHookErr_Okay;
	}

	/* Hook structure already exists at this point */

	if (mode == EventHookMode_Pre)
	{
		/* Create pre hook forward if necessary */
		if (!pHook->pPreHook)
		{
			pHook->pPreHook = forwardsys->CreateForwardEx(NULL, ET_Event, 3, GAMEEVENT_PARAMS);
		}

		/* Add plugin function to forward list */
		pHook->pPreHook->AddFunction(pFunction);
	} else {
		/* Create post hook forward if necessary */
		if (!pHook->pPostHook)
		{
			pHook->pPostHook = forwardsys->CreateForwardEx(NULL, ET_Ignore, 3, GAMEEVENT_PARAMS);
		}

		/* If postCopy is false, then we may want to set it to true */
		if (!pHook->postCopy)
		{
			pHook->postCopy = (mode == EventHookMode_Post);
		}

		/* Add plugin function to forward list */
		pHook->pPostHook->AddFunction(pFunction);
	}

	/* Increase reference count */
	pHook->refCount++;

	return EventHookErr_Okay;
}

EventHookError EventManager::UnhookEvent(const char *name, IPluginFunction *pFunction, EventHookMode mode)
{
	EventHook *pHook;
	IChangeableForward **pEventForward;

	/* If hook does not exist at all */
	if (!m_EventHooks.retrieve(name, &pHook))
	{
		return EventHookErr_NotActive;
	}

	/* One forward to rule them all */
	if (mode == EventHookMode_Pre)
	{
		pEventForward = &pHook->pPreHook;
	} else {
		pEventForward = &pHook->pPostHook;
	}

	/* Remove function from forward's list */
	if (*pEventForward == NULL || !(*pEventForward)->RemoveFunction(pFunction))
	{
		return EventHookErr_InvalidCallback;
	}

	/* If forward's list contains 0 functions now, free it */
	if ((*pEventForward)->GetFunctionCount() == 0)
	{
		forwardsys->ReleaseForward(*pEventForward);
		*pEventForward = NULL;
	}

	/* Decrement reference count */
	if (--pHook->refCount == 0)
	{
		/* If reference count is now 0, free hook structure */

		EventHookList *pHookList;
		IPlugin *plugin = scripts->FindPluginByContext(pFunction->GetParentContext()->GetContext());

		/* Get plugin's event hook list */
		if (!plugin->GetProperty("EventHooks", (void**)&pHookList))
		{
			return EventHookErr_NotActive;
		}

		/* Make sure the event was actually being hooked */
		if (pHookList->find(pHook) == pHookList->end())
		{
			return EventHookErr_NotActive;
		}

		/* Remove current structure from plugin's list */
		pHookList->remove(pHook);

		/* Delete entry in trie */
		m_EventHooks.remove(name);

		/* And finally free structure memory */
		delete pHook;
	}

	return EventHookErr_Okay;
}

EventInfo *EventManager::CreateEvent(IPluginContext *pContext, const char *name, bool force)
{
	EventInfo *pInfo;
	IGameEvent *pEvent = gameevents->CreateEvent(name, force);

	if (pEvent)
	{
		if (m_FreeEvents.empty())
		{
			pInfo = new EventInfo();
		} else {
			pInfo = m_FreeEvents.front();
			m_FreeEvents.pop();
		}


		pInfo->pEvent = pEvent;
		pInfo->pOwner = pContext->GetIdentity();
		pInfo->bDontBroadcast = false;

		return pInfo;
	}

	return NULL;
}

void EventManager::FireEvent(EventInfo *pInfo, bool bDontBroadcast)
{
	/* Actually fire event now */
	gameevents->FireEvent(pInfo->pEvent, bDontBroadcast);

	/* IGameEvent is free at this point, so no one owns this */
	pInfo->pOwner = NULL;

	/* Add EventInfo struct to free event stack */
	m_FreeEvents.push(pInfo);
}

void EventManager::FireEventToClient(EventInfo *pInfo, IClient *pClient)
{
	// The IClient vtable is +sizeof(void *) from the IGameEventListener2 (CBaseClient) vtable due to multiple inheritance.
	IGameEventListener2 *pGameClient = (IGameEventListener2 *)((intptr_t)pClient - sizeof(void *));
	pGameClient->FireGameEvent(pInfo->pEvent);
}

void EventManager::CancelCreatedEvent(EventInfo *pInfo)
{
	/* Free event from IGameEventManager2 */
	gameevents->FreeEvent(pInfo->pEvent);

	/* IGameEvent is free at this point, so no one owns this */
	pInfo->pOwner = NULL;

	/* Add EventInfo struct to free event stack */
	m_FreeEvents.push(pInfo);
}

/* IGameEventManager2::FireEvent hook */
bool EventManager::OnFireEvent(IGameEvent *pEvent, bool bDontBroadcast)
{
	EventHook *pHook;
	IChangeableForward *pForward;
	const char *name;
	cell_t res = Pl_Continue;
	bool broadcast = bDontBroadcast;

	/* The engine accepts NULL without crashing, so to prevent a crash in SM we ignore these */
	if (!pEvent)
	{
		RETURN_META_VALUE(MRES_IGNORED, false);
	}

	name = pEvent->GetName();
	
	if (m_EventHooks.retrieve(name, &pHook))
	{
		/* Push the event onto the event stack.  The reference count is increased to make sure 
		 * the structure is not garbage collected in between now and the post hook.
		 */
		pHook->refCount++;
		m_EventStack.push(pHook);

		pForward = pHook->pPreHook;

		if (pForward)
		{
			EventInfo info(pEvent, NULL);
			HandleSecurity sec(NULL, g_pCoreIdent);
			Handle_t hndl = handlesys->CreateHandle(m_EventType, &info, NULL, g_pCoreIdent, NULL);

			info.bDontBroadcast = bDontBroadcast;

			EventForwardFilter filter(&info);

			pForward->PushCell(hndl);
			pForward->PushString(name);
			pForward->PushCell(bDontBroadcast);
			pForward->Execute(&res, &filter);

			broadcast = info.bDontBroadcast;

			handlesys->FreeHandle(hndl, &sec);
		}

		if (pHook->postCopy)
		{
			m_EventCopies.push(gameevents->DuplicateEvent(pEvent));
		}

		if (res >= Pl_Handled)
		{
			gameevents->FreeEvent(pEvent);
			RETURN_META_VALUE(MRES_SUPERCEDE, false);
		}
	}
	else
	{
		m_EventStack.push(NULL);
	}

	if (broadcast != bDontBroadcast)
		RETURN_META_VALUE_NEWPARAMS(MRES_IGNORED, true, &IGameEventManager2::FireEvent, (pEvent, broadcast));

	RETURN_META_VALUE(MRES_IGNORED, true);
}

/* IGameEventManager2::FireEvent post hook */
bool EventManager::OnFireEvent_Post(IGameEvent *pEvent, bool bDontBroadcast)
{
	EventHook *pHook;
	EventInfo info;
	IChangeableForward *pForward;
	Handle_t hndl = 0;

	/* The engine accepts NULL without crashing, so to prevent a crash in SM we ignore these */
	if (!pEvent)
	{
		RETURN_META_VALUE(MRES_IGNORED, false);
	}

	pHook = m_EventStack.front();

	if (pHook != NULL)
	{
		pForward = pHook->pPostHook;

		if (pForward)
		{
			if (pHook->postCopy)
			{
				info.bDontBroadcast = bDontBroadcast;
				info.pEvent = m_EventCopies.front();
				info.pOwner = NULL;
				hndl = handlesys->CreateHandle(m_EventType, &info, NULL, g_pCoreIdent, NULL);

				pForward->PushCell(hndl);
			} else {
				pForward->PushCell(BAD_HANDLE);
			}

			pForward->PushString(pHook->name.c_str());
			pForward->PushCell(bDontBroadcast);
			pForward->Execute(NULL);

			if (pHook->postCopy)
			{
				/* Free handle */
				HandleSecurity sec(NULL, g_pCoreIdent);
				handlesys->FreeHandle(hndl, &sec);

				/* Free event structure */
				gameevents->FreeEvent(info.pEvent);
				m_EventCopies.pop();
			}
		}

		/* Decrement reference count, check if a delayed delete is needed */
		if (--pHook->refCount == 0)
		{
			assert(pHook->pPostHook == NULL);
			assert(pHook->pPreHook == NULL);
			m_EventHooks.remove(pHook->name.c_str());
			delete pHook;
		}
	}

	m_EventStack.pop();

	RETURN_META_VALUE(MRES_IGNORED, true);
}
