/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod DHooks Extension
 * Copyright (C) 2019 AlliedModders LLC.  All rights reserved.
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

#include "listeners.h"
#include "vhook.h"

using namespace SourceHook;

ke::Vector<EntityListener> g_EntityListeners;
ke::Vector<DHooksManager *>g_pRemoveList;

void FrameCleanupHooks(void *data)
{
	for (int i = g_pRemoveList.length() - 1; i >= 0; i--)
	{
		DHooksManager *manager = g_pRemoveList.at(i);
		delete manager;
		g_pRemoveList.remove(i);
	}
}

void DHooks::OnCoreMapEnd()
{
	for(int i = g_pHooks.length() -1; i >= 0; i--)
	{
		DHooksManager *manager = g_pHooks.at(i);
		if(manager->callback->hookType == HookType_GameRules)
		{
			delete manager;
			g_pHooks.remove(i);
		}
	}
}

void DHooksEntityListener::CleanupListeners(IPluginContext *pContext)
{
	for(int i = g_EntityListeners.length() -1; i >= 0; i--)
	{
		if(pContext == NULL || pContext == g_EntityListeners.at(i).callback->GetParentRuntime()->GetDefaultContext())
		{
			g_EntityListeners.remove(i);
		}
	}
}

void DHooksEntityListener::OnEntityCreated(CBaseEntity *pEntity, const char *classname)
{
	int entity = gamehelpers->EntityToBCompatRef(pEntity);

	for(int i = g_EntityListeners.length() -1; i >= 0; i--)
	{
		EntityListener listerner = g_EntityListeners.at(i);
		if(listerner.type == ListenType_Created)
		{
			IPluginFunction *callback = listerner.callback;
			callback->PushCell(entity);
			callback->PushString(classname);
			callback->Execute(NULL);
		}
	}
}

void DHooksEntityListener::OnEntityDestroyed(CBaseEntity *pEntity)
{
	int entity = gamehelpers->EntityToBCompatRef(pEntity);

	for(int i = g_EntityListeners.length() -1; i >= 0; i--)
	{
		EntityListener listerner = g_EntityListeners.at(i);
		if(listerner.type == ListenType_Deleted)
		{
			IPluginFunction *callback = listerner.callback;
			callback->PushCell(gamehelpers->EntityToBCompatRef(pEntity));
			callback->Execute(NULL);
		}
	}

	for(int i = g_pHooks.length() -1; i >= 0; i--)
	{
		DHooksManager *manager = g_pHooks.at(i);
		if(manager->callback->hookType == HookType_Entity && manager->callback->entity == entity)
		{
			if(g_pRemoveList.length() == 0)
			{
				smutils->AddFrameAction(&FrameCleanupHooks, NULL);
			}

			g_pRemoveList.append(manager);
			g_pHooks.remove(i);
		}
	}
}
bool DHooksEntityListener::AddPluginEntityListener(ListenType type, IPluginFunction *callback)
{
	for(int i = g_EntityListeners.length() -1; i >= 0; i--)
	{
		EntityListener listerner = g_EntityListeners.at(i);
		if(listerner.callback == callback && listerner.type == type)
		{
			return true;
		}
	}
	EntityListener listener;
	listener.callback = callback;
	listener.type = type;
	g_EntityListeners.append(listener);
	return true;
}
bool DHooksEntityListener::RemovePluginEntityListener(ListenType type, IPluginFunction *callback)
{
	for(int i = g_EntityListeners.length() -1; i >= 0; i--)
	{
		EntityListener listerner = g_EntityListeners.at(i);
		if(listerner.callback == callback && listerner.type == type)
		{
			g_EntityListeners.remove(i);
			return true;
		}
	}
	return false;
}
