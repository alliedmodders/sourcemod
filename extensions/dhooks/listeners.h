#ifndef _INCLUDE_LISTENERS_H_
#define _INCLUDE_LISTENERS_H_

#include "extension.h"
#include "vhook.h"

enum ListenType
{
	ListenType_Created,
	ListenType_Deleted
};

class DHooksEntityListener : public ISMEntityListener
{
public:
	virtual void OnEntityCreated(CBaseEntity *pEntity, const char *classname);
	virtual void OnEntityDestroyed(CBaseEntity *pEntity);
	void CleanupListeners(IPluginContext *func = NULL);
	bool AddPluginEntityListener(ListenType type, IPluginFunction *callback);
	bool RemovePluginEntityListener(ListenType type, IPluginFunction *callback);
};


struct EntityListener
{
	ListenType type;
	IPluginFunction *callback;
};

extern ke::Vector<DHooksManager *> g_pHooks;
#endif
