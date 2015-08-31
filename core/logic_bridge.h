/**
 * vim: set ts=4 sw=4 :
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
#ifndef _INCLUDE_SOURCEMOD_LOGIC_BRIDGE_H_
#define _INCLUDE_SOURCEMOD_LOGIC_BRIDGE_H_

#include <bridge/include/LogicProvider.h>

extern SourceMod::sm_logic_t logicore;
extern SourceMod::ITranslator *translator;
extern SourceMod::IGameConfig *g_pGameConf;
extern SourceMod::IScriptManager *scripts;
extern SourceMod::IShareSys *sharesys;
extern SourceMod::IExtensionSys *extsys;
extern SourceMod::IHandleSys *handlesys;
extern SourceMod::IForwardManager *forwardsys;
extern SourceMod::IAdminSystem *adminsys;
extern SourceMod::ILogger *logger;
extern SourceMod::IRootConsole *rootmenu;

#endif /* _INCLUDE_SOURCEMOD_LOGIC_BRIDGE_H_ */
