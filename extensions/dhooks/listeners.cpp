#include "listeners.h"
#include "vhook.h"

using namespace SourceHook;

std::vector<EntityListener> g_EntityListeners;
std::vector<DHooksManager *>g_pRemoveList;

void FrameCleanupHooks(void *data)
{
	for (int i = g_pRemoveList.size() - 1; i >= 0; i--)
	{
		DHooksManager *manager = g_pRemoveList.at(i);
		delete manager;
		g_pRemoveList.erase(g_pRemoveList.begin() + i);
	}
}

void DHooks::OnCoreMapEnd()
{
	for(int i = g_pHooks.size() -1; i >= 0; i--)
	{
		DHooksManager *manager = g_pHooks.at(i);
		if(manager->callback->hookType == HookType_GameRules)
		{
			delete manager;
			g_pHooks.erase(g_pHooks.begin() + i);
		}
	}
}

void DHooksEntityListener::CleanupListeners(IPluginContext *pContext)
{
	for(int i = g_EntityListeners.size() -1; i >= 0; i--)
	{
		if(pContext == NULL || pContext == g_EntityListeners.at(i).callback->GetParentRuntime()->GetDefaultContext())
		{
			g_EntityListeners.erase(g_EntityListeners.begin() + i);
		}
	}
	for (int i = g_pRemoveList.size() -1; i >= 0; i--)
	{
		DHooksManager *manager = g_pRemoveList.at(i);
		IPluginFunction *cb = manager->callback->plugin_callback;
		if (pContext == NULL || (cb && pContext == cb->GetParentRuntime()->GetDefaultContext()))
		{
			manager->callback->plugin_callback = nullptr;
			manager->remove_callback = nullptr;
		}
	}
}

void DHooksEntityListener::CleanupRemoveList()
{
	for (int i = g_pRemoveList.size() - 1; i >= 0; i--)
	{
		DHooksManager *manager = g_pRemoveList.at(i);
		delete manager;
	}
	g_pRemoveList.clear();
}

void DHooksEntityListener::OnEntityCreated(CBaseEntity *pEntity, const char *classname)
{
	int entity = gamehelpers->EntityToBCompatRef(pEntity);

	for(int i = g_EntityListeners.size() -1; i >= 0; i--)
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

	for(int i = g_EntityListeners.size() -1; i >= 0; i--)
	{
		EntityListener listerner = g_EntityListeners.at(i);
		if(listerner.type == ListenType_Deleted)
		{
			IPluginFunction *callback = listerner.callback;
			callback->PushCell(gamehelpers->EntityToBCompatRef(pEntity));
			callback->Execute(NULL);
		}
	}

	for(int i = g_pHooks.size() -1; i >= 0; i--)
	{
		DHooksManager *manager = g_pHooks.at(i);
		if(manager->callback->hookType == HookType_Entity && manager->callback->entity == entity)
		{
			if(g_pRemoveList.size() == 0)
			{
				smutils->AddFrameAction(&FrameCleanupHooks, NULL);
			}

			g_pRemoveList.push_back(manager);
			g_pHooks.erase(g_pHooks.begin() + i);
		}
	}
}
bool DHooksEntityListener::AddPluginEntityListener(ListenType type, IPluginFunction *callback)
{
	for(int i = g_EntityListeners.size() -1; i >= 0; i--)
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
	g_EntityListeners.push_back(listener);
	return true;
}
bool DHooksEntityListener::RemovePluginEntityListener(ListenType type, IPluginFunction *callback)
{
	for(int i = g_EntityListeners.size() -1; i >= 0; i--)
	{
		EntityListener listerner = g_EntityListeners.at(i);
		if(listerner.callback == callback && listerner.type == type)
		{
			g_EntityListeners.erase(g_EntityListeners.begin() + i);
			return true;
		}
	}
	return false;
}
