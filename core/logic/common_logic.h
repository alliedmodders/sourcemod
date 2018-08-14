/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2009 AlliedModders LLC.  All rights reserved.
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

#ifndef _INCLUDE_SOURCEMOD_COMMON_LOGIC_H_
#define _INCLUDE_SOURCEMOD_COMMON_LOGIC_H_

#include "../sm_globals.h"
#ifdef PLATFORM_X64
#include "PseudoAddrManager.h"
#endif

namespace SourceMod {
class CoreProvider;
class IHandleSys;
class ISourceMod;
class ILibrarySys;
class ITextParsers;
class IShareSys;
class IRootConsole ;
class IPluginManager;
class IForwardManager;
class ITimerSystem;
class IPlayerManager;
class IAdminSystem;
class IGameHelpers;
class IScriptManager;
class IExtensionSys;
class ILogger;
class IMenuManager;
#if defined SM_LOGIC
class IVEngineServerBridge;
#endif
} // namespace SourceMod
struct ServerGlobals;
class PseudoAddressManager;

extern SourceMod::CoreProvider *bridge;
extern SourceMod::IHandleSys *handlesys;
extern SourceMod::ISourceMod *g_pSM;
extern SourceMod::ILibrarySys *libsys;
extern SourceMod::ITextParsers *textparser;
extern SourceMod::IShareSys *sharesys;
extern SourceMod::IRootConsole *rootmenu;
extern SourceMod::IPluginManager *pluginsys;
extern SourceMod::IForwardManager *forwardsys;
extern SourceMod::ITimerSystem *timersys;
extern ServerGlobals serverGlobals;
extern SourceMod::IPlayerManager *playerhelpers;
extern SourceMod::IAdminSystem *adminsys;
extern SourceMod::IGameHelpers *gamehelpers;
extern SourceMod::IScriptManager *scripts;
extern SourceMod::IExtensionSys *extsys;
extern SourceMod::ILogger *logger;
extern SourceMod::IMenuManager *menus;
extern PseudoAddressManager pseudoAddr;

#if defined SM_LOGIC
extern SourceMod::IVEngineServerBridge *engine;
#else
extern IVEngineServer *engine;
#endif

#endif /* _INCLUDE_SOURCEMOD_COMMON_LOGIC_H_ */

