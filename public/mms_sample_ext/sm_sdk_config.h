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

#endif //_INCLUDE_SOURCEMOD_CONFIG_H_

