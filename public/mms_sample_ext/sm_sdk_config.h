/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Extension Code for Metamod:Source
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
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

#ifndef _INCLUDE_SOURCEMOD_CONFIG_H_
#define _INCLUDE_SOURCEMOD_CONFIG_H_

#include <stdio.h>

/**
 * @brief Acquires the interfaces enabled at the bottom of this header.
 *
 * @param error			Buffer to store error message.
 * @param maxlength		Maximum size of the error buffer.
 * @return				True on success, false on failure.
 * 						On failure, a null-terminated string will be stored 
 * 						in the error buffer, if the buffer is non-NULL and 
 * 						greater than 0 bytes in size.
 */
bool SM_AcquireInterfaces(char *error, size_t maxlength);

/**
 * @brief Sets each acquired interface to NULL.
 */
void SM_UnsetInterfaces();

/** 
 * Enable interfaces you want to use here by uncommenting lines.
 * These interfaces are all part of SourceMod's core.
 */
//#define SMEXT_ENABLE_FORWARDSYS
//#define SMEXT_ENABLE_HANDLESYS
//#define SMEXT_ENABLE_PLAYERHELPERS
//#define SMEXT_ENABLE_DBMANAGER
//#define SMEXT_ENABLE_GAMECONF
//#define SMEXT_ENABLE_MEMUTILS
//#define SMEXT_ENABLE_GAMEHELPERS
//#define SMEXT_ENABLE_TIMERSYS
//#define SMEXT_ENABLE_THREADER
//#define SMEXT_ENABLE_LIBSYS
//#define SMEXT_ENABLE_MENUS
//#define SMEXT_ENABLE_ADTFACTORY
//#define SMEXT_ENABLE_PLUGINSYS
//#define SMEXT_ENABLE_ADMINSYS
//#define SMEXT_ENABLE_TEXTPARSERS
//#define SMEXT_ENABLE_TRANSLATOR


/**
 * There is no need to edit below.
 */

#include <IShareSys.h>
#include <IExtensionSys.h>
extern SourceMod::IExtension *myself;
extern SourceMod::IExtensionManager *smexts;
extern SourceMod::IShareSys *sharesys;

#include <ISourceMod.h>
extern SourceMod::ISourceMod *sm_main;

#if defined SMEXT_ENABLE_FORWARDSYS
#include <IForwardSys.h>
extern SourceMod::IForwardManager *sm_forwards;
#endif

#if defined SMEXT_ENABLE_HANDLESYS
#include <IHandleSys.h>
extern SourceMod::IHandleSys *sm_handlesys;
#endif

#if defined SMEXT_ENABLE_PLAYERHELPERS
#include <IPlayerHelpers.h>
extern SourceMod::IPlayerManager *sm_players;
#endif

#if defined SMEXT_ENABLE_DBMANAGER
#include <IDBDriver.h>
extern SourceMod::IDBManager *sm_dbi;
#endif 

#if defined SMEXT_ENABLE_GAMECONF
#include <IGameConfigs.h>
extern SourceMod::IGameConfigManager *sm_gameconfs;
#endif

#if defined SMEXT_ENABLE_MEMUTILS
#include <IMemoryUtils.h>
extern SourceMod::IMemoryUtils *sm_memutils;
#endif

#if defined SMEXT_ENABLE_GAMEHELPERS
#include <IGameHelpers.h>
extern SourceMod::IGameHelpers *sm_gamehelpers;
#endif

#if defined SMEXT_ENABLE_TIMERSYS
#include <ITimerSystem.h>
extern SourceMod::ITimerSystem *sm_timersys;
#endif

#if defined SMEXT_ENABLE_THREADER
#include <IThreader.h>
extern SourceMod::IThreader *sm_threader;
#endif

#if defined SMEXT_ENABLE_LIBSYS
#include <ILibrarySys.h>
extern SourceMod::ILibrarySys *sm_libsys;
#endif

#if defined SMEXT_ENABLE_PLUGINSYS
#include <IPluginSys.h>
extern SourceMod::IPluginManager *sm_plsys;
#endif 

#if defined SMEXT_ENABLE_MENUS
#include <IMenuManager.h>
extern SourceMod::IMenuManager *sm_menus;
#endif

#if defined SMEXT_ENABLE_ADMINSYS
#include <IAdminSystem.h>
extern SourceMod::IAdminSystem *sm_adminsys;
#endif

#if defined SMEXT_ENABLE_TEXTPARSERS
#include <ITextParsers.h>
extern SourceMod::ITextParsers *sm_text;
#endif

#if defined SMEXT_ENABLE_TRANSLATOR
#include <ITranslator.h>
extern SourceMod::ITranslator *sm_translator;
#endif

#endif //_INCLUDE_SOURCEMOD_CONFIG_H_

