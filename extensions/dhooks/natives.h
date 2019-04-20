#ifndef _INCLUDE_NATIVES_H_
#define _INCLUDE_NATIVES_H_

#include "extension.h"
#include "vhook.h"
#include "listeners.h"

extern DHooksEntityListener *g_pEntityListener;
extern ISDKTools *g_pSDKTools;
extern HandleType_t g_HookSetupHandle;
extern HandleType_t g_HookParamsHandle;
extern HandleType_t g_HookReturnHandle;
extern ke::Vector<DHooksManager *> g_pHooks;
#endif
