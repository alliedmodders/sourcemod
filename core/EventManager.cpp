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

#include "EventManager.h"
#include "ForwardSys.h"
#include "HandleSys.h"
#include "PluginSys.h"
#include "sm_stringutil.h"

EventManager g_EventManager;

SH_DECL_HOOK2(IGameEventManager2, FireEvent, SH_NOATTRIB, 0, bool, IGameEvent *, bool);

typedef List<EventHook *> EventHookList;

const ParamType GAMEEVENT_PARAMS[] = {Param_Cell, Param_String, Param_Cell};

EventManager::EventManager() : m_EventCopy(NULL), m_NotifyState(true)
{
	/* Create an event lookup trie */
	m_EventHooks = sm_trie_create();
}

EventManager::~EventManager()
{
	sm_trie_destroy(m_EventHooks);
}

void EventManager::OnSourceModAllInitialized()
{
	/* Add a hook for IGameEventManager2::FireEvent() */
	SH_ADD_HOOK_MEMFUNC(IGameEventManager2, FireEvent, gameevents, this, &EventManager::OnFireEvent, false);
	SH_ADD_HOOK_MEMFUNC(IGameEventManager2, FireEvent, gameevents, this, &EventManager::OnFireEvent_Post, true);

	HandleAccess sec;

	/* Handle access security for 'GameEvent' handle type */
	sec.access[HandleAccess_Read] = 0;
	sec.access[HandleAccess_Delete] = HANDLE_RESTRICT_IDENTITY | HANDLE_RESTRICT_OWNER;
	sec.access[HandleAccess_Clone] = HANDLE_RESTRICT_IDENTITY | HANDLE_RESTRICT_OWNER;

	/* Create the 'GameEvent' handle type */
	m_EventType = g_HandleSys.CreateType("GameEvent", this, 0, NULL, &sec, g_pCoreIdent, NULL);
}

void EventManager::OnSourceModShutdown()
{
	/* Remove hook for IGameEventManager2::FireEvent() */
	SH_REMOVE_HOOK_MEMFUNC(IGameEventManager2, FireEvent, gameevents, this, &EventManager::OnFireEvent, false);
	SH_REMOVE_HOOK_MEMFUNC(IGameEventManager2, FireEvent, gameevents, this, &EventManager::OnFireEvent_Post, true);

	/* Remove the 'GameEvent' handle type */
	g_HandleSys.RemoveType(m_EventType, g_pCoreIdent);

	/* Remove ourselves as listener for events */
	gameevents->RemoveListener(this);
}

void EventManager::OnHandleDestroy(HandleType_t type, void *object)
{
	if (type == m_EventType)
	{
		EventInfo *pInfo = static_cast<EventInfo *>(object);

		if (pInfo->canDelete)
		{
			gameevents->FreeEvent(pInfo->pEvent);
			delete pInfo;
		}
	}
}

void EventManager::OnPluginUnloaded(IPlugin *plugin)
{
	EventHookList *pHookList;
	EventHook *pHook;

	// If plugin has an event hook list...
	if (plugin->GetProperty("EventHooks", reinterpret_cast<void **>(&pHookList), true))
	{
		for (EventHookList::iterator iter = pHookList->begin(); iter != pHookList->end(); iter++)
		{
			pHook = (*iter);

			if (--pHook->refCount == 0)
			{
				if (pHook->pPreHook)
				{
					g_Forwards.ReleaseForward(pHook->pPreHook);
				}

				if (pHook->pPostHook)
				{
					g_Forwards.ReleaseForward(pHook->pPostHook);
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
	if (!sm_trie_retrieve(m_EventHooks, name, (void **)&pHook))
	{
		EventHookList *pHookList;
		IPlugin *plugin = g_PluginSys.GetPluginByCtx(pFunction->GetParentContext()->GetContext());

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
			pHook->pPreHook = g_Forwards.CreateForwardEx(NULL, ET_Event, 3, GAMEEVENT_PARAMS);
			/* Add to forward list */
			pHook->pPreHook->AddFunction(pFunction);
		} else {
			/* Create forward for a post hook */
			pHook->pPostHook = g_Forwards.CreateForwardEx(NULL, ET_Ignore, 3, GAMEEVENT_PARAMS);
			/* Should we copy data from a pre hook to the post hook? */
			pHook->postCopy = (mode == EventHookMode_Post);
			/* Add to forward list */
			pHook->pPostHook->AddFunction(pFunction);
		}

		/* Increase reference count */
		pHook->refCount++;

		/* Add hook structure to hook lists */
		pHookList->push_back(pHook);
		sm_trie_insert(m_EventHooks, name, pHook);

		return EventHookErr_Okay;
	}

	/* Hook structure already exists at this point */

	if (mode == EventHookMode_Pre)
	{
		/* Create pre hook forward if necessary */
		if (!pHook->pPreHook)
		{
			pHook->pPreHook = g_Forwards.CreateForwardEx(NULL, ET_Event, 3, GAMEEVENT_PARAMS);
		}

		/* Add plugin function to forward list */
		pHook->pPreHook->AddFunction(pFunction);
	} else {
		/* Create post hook forward if necessary */
		if (!pHook->pPostHook)
		{
			pHook->pPostHook = g_Forwards.CreateForwardEx(NULL, ET_Ignore, 3, GAMEEVENT_PARAMS);
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
	if (!sm_trie_retrieve(m_EventHooks, name, (void **)&pHook))
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
		g_Forwards.ReleaseForward(*pEventForward);
		*pEventForward = NULL;
	}

	/* Decrement reference count */
	if (--pHook->refCount == 0)
	{
		/* If reference count is now 0, free hook structure */

		EventHookList *pHookList;
		IPlugin *plugin = g_PluginSys.GetPluginByCtx(pFunction->GetParentContext()->GetContext());

		/* Get plugin's event hook list */
		plugin->GetProperty("EventHooks", (void**)&pHookList);

		/* Remove current structure from plugin's list */
		pHookList->remove(pHook);

		/* Delete entry in trie */
		sm_trie_delete(m_EventHooks, name);

		/* And finally free structure memory */
		delete pHook;
	}

	return EventHookErr_Okay;
}

/* IGameEventManager2::FireEvent hook */
bool EventManager::OnFireEvent(IGameEvent *pEvent, bool bDontBroadcast)
{
	EventHook *pHook;
	IChangeableForward *pForward;
	cell_t res = Pl_Continue;

	if (!m_NotifyState)
	{
		RETURN_META_VALUE(MRES_IGNORED, true);
	}
	
	/* Get the event name, we're going to need this for passing to post hooks */
	m_EventName = pEvent->GetName();

	if (sm_trie_retrieve(m_EventHooks, m_EventName, reinterpret_cast<void **>(&pHook)))
	{
		pForward = pHook->pPreHook;

		if (pForward)
		{
			EventInfo info = {pEvent, false};

			Handle_t hndl = g_HandleSys.CreateHandle(m_EventType, &info, NULL, g_pCoreIdent, NULL);
			pForward->PushCell(hndl);
			pForward->PushString(m_EventName);
			pForward->PushCell(bDontBroadcast);
			pForward->Execute(&res, NULL);

			HandleSecurity sec = { NULL, g_pCoreIdent };
			g_HandleSys.FreeHandle(hndl, &sec);
		}

		if (pHook->postCopy)
		{
			m_EventCopy = gameevents->DuplicateEvent(pEvent);
		}

		if (res)
		{
			gameevents->FreeEvent(pEvent);
			RETURN_META_VALUE(MRES_SUPERCEDE, false);
		}
	}

	RETURN_META_VALUE(MRES_IGNORED, true);
}

/* IGameEventManager2::FireEvent post hook */
bool EventManager::OnFireEvent_Post(IGameEvent *pEvent, bool bDontBroadcast)
{
	EventHook *pHook;
	IChangeableForward *pForward;
	Handle_t hndl;

	if (!m_NotifyState)
	{
		m_NotifyState = true;
		RETURN_META_VALUE(MRES_IGNORED, true);
	}

	if (sm_trie_retrieve(m_EventHooks, m_EventName, reinterpret_cast<void **>(&pHook)))
	{
		pForward = pHook->pPostHook;

		if (pForward)
		{
			if (pHook->postCopy)
			{
				EventInfo info = {m_EventCopy, false};
				hndl = g_HandleSys.CreateHandle(m_EventType, &info, NULL, g_pCoreIdent, NULL);
				pForward->PushCell(hndl);
			} else {
				pForward->PushCell(BAD_HANDLE);
			}

			pForward->PushString(m_EventName);
			pForward->PushCell(bDontBroadcast);
			pForward->Execute(NULL);

			if (pHook->postCopy)
			{
				/* Free handle */
				HandleSecurity sec = { NULL, g_pCoreIdent };
				g_HandleSys.FreeHandle(hndl, &sec);

				/* Free event structure */
				gameevents->FreeEvent(m_EventCopy);
				m_EventCopy = NULL;
			}
		}
	}

	RETURN_META_VALUE(MRES_IGNORED, true);
}
