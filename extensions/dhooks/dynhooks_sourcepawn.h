/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Dynamic Hooks Extension
 * Copyright (C) 2012-2021 AlliedModders LLC.  All rights reserved.
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

#ifndef _INCLUDE_DYNHOOKS_SP_H_
#define _INCLUDE_DYNHOOKS_SP_H_

#include "manager.h"
#include "vhook.h"
#include <am-hashmap.h>

class CDynamicHooksSourcePawn;
typedef ke::HashMap<IPluginFunction *, CDynamicHooksSourcePawn *, ke::PointerPolicy<IPluginFunction>> CallbackMap;
typedef std::vector<CDynamicHooksSourcePawn *> PluginCallbackList;
typedef ke::HashMap<CHook *, PluginCallbackList *, ke::PointerPolicy<CHook>> DetourMap;

//extern ke::Vector<CHook *> g_pDetours;
// Keep a list of plugin callback -> Hook wrapper for easily removing plugin hooks
//extern CallbackMap g_pPluginPreDetours;
//extern CallbackMap g_pPluginPostDetours;
// Keep a list of hook -> callbacks for calling in the detour handler
extern DetourMap g_pPreDetours;
extern DetourMap g_pPostDetours;

class CDynamicHooksSourcePawn : public DHooksInfo {
public:
	CDynamicHooksSourcePawn(HookSetup *setup, CHook *pDetour, IPluginFunction *pCallback, bool post);

	HookReturnStruct *GetReturnStruct();
	HookParamsStruct *GetParamStruct();
	void UpdateParamsFromStruct(HookParamsStruct *params);

public:
	CHook *m_pDetour;
	CallingConvention callConv;
};

ICallingConvention *ConstructCallingConvention(HookSetup *setup);
bool UpdateRegisterArgumentSizes(CHook* pDetour, HookSetup *setup);
ReturnAction_t HandleDetour(HookType_t hookType, CHook* pDetour);
bool AddDetourPluginHook(HookType_t hookType, CHook *pDetour, HookSetup *setup, IPluginFunction *pCallback);
bool RemoveDetourPluginHook(HookType_t hookType, CHook *pDetour, IPluginFunction *pCallback);
void RemoveAllCallbacksForContext(IPluginContext *pContext);
void CleanupDetours();

#endif