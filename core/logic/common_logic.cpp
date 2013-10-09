/**
 * vim: set ts=4 sw=4 tw=99 noet:
 * =============================================================================
 * SourceMod
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
#include <new>
#include <stdlib.h>
#include <sm_platform.h>
#include <string.h>
#include <stdarg.h>
#include "common_logic.h"
#include "TextParsers.h"
#include "Profiler.h"
#include "sm_crc32.h"
#include "MemoryUtils.h"
#include "stringutil.h"
#include "ThreadSupport.h"
#include "Translator.h"
#include "GameConfigs.h"
#include "DebugReporter.h"
#include "PluginSys.h"
#include "ShareSys.h"
#include "NativeOwner.h"
#include "HandleSys.h"
#include "ExtensionSys.h"
#include "ForwardSys.h"

sm_core_t smcore;
IHandleSys *handlesys = &g_HandleSys;
IdentityToken_t *g_pCoreIdent;
SMGlobalClass *SMGlobalClass::head = NULL;
ISourceMod *g_pSM;
ILibrarySys *libsys;
ITextParsers *textparser = &g_TextParser;
IVEngineServer *engine;
IShareSys *sharesys = &g_ShareSys;
IRootConsole *rootmenu;
IPluginManager *pluginsys = g_PluginSys.GetOldAPI();
IForwardManager *forwardsys = &g_Forwards;
ITimerSystem *timersys;
ServerGlobals serverGlobals;
IPlayerManager *playerhelpers;
IAdminSystem *adminsys;
IGameHelpers *gamehelpers;
ISourcePawnEngine *g_pSourcePawn;
ISourcePawnEngine2 *g_pSourcePawn2;
CNativeOwner g_CoreNatives;
IScriptManager *scripts = &g_PluginSys;

static void AddCorePhraseFile(const char *filename)
{
	g_pCorePhrases->AddPhraseFile(filename);
}

static IGameConfig *GetCoreGameConfig()
{
	return g_pGameConf;
}

// Defined in smn_filesystem.cpp.
extern bool OnLogPrint(const char *msg);

static void GenerateError(IPluginContext *ctx, cell_t idx, int err, const char *msg, ...)
{
	va_list ap;
	va_start(ap, msg);
	g_DbgReporter.GenerateErrorVA(ctx, idx, err, msg, ap);
	va_end(ap);
}

static void AddNatives(sp_nativeinfo_t *natives)
{
	g_CoreNatives.AddNatives(natives);
}

static void DumpHandles(void (*dumpfn)(const char *fmt, ...))
{
	g_HandleSys.Dump(dumpfn);
}

static sm_logic_t logic =
{
	NULL,
	g_pThreader,
	sm_profiler,
	&g_Translator,
	stristr,
	CoreTranslate,
	AddCorePhraseFile,
	UTIL_ReplaceAll,
	UTIL_ReplaceEx,
	UTIL_DecodeHexString,
	GetCoreGameConfig,
	OnLogPrint,
	&g_DbgReporter,
	GenerateError,
	AddNatives,
	DumpHandles,
	&g_PluginSys,
	&g_ShareSys,
	&g_Extensions,
	&g_HandleSys,
	&g_Forwards,
	NULL,
	-1.0f
};

static void logic_init(const sm_core_t* core, sm_logic_t* _logic)
{
	logic.head = SMGlobalClass::head;

	memcpy(&smcore, core, sizeof(sm_core_t));
	memcpy(_logic, &logic, sizeof(sm_logic_t));
	memcpy(&serverGlobals, core->serverGlobals, sizeof(ServerGlobals));

	libsys = core->libsys;
	engine = core->engine;
	g_pSM = core->sm;
	rootmenu = core->rootmenu;
	timersys = core->timersys;
	playerhelpers = core->playerhelpers;
	adminsys = core->adminsys;
	gamehelpers = core->gamehelpers;
	g_pSourcePawn = *core->spe1;
	g_pSourcePawn2 = *core->spe2;
	SMGlobalClass::head = core->listeners;

	g_ShareSys.Initialize();
	g_pCoreIdent = g_ShareSys.CreateCoreIdentity();
	
	_logic->core_ident = g_pCoreIdent;
}

PLATFORM_EXTERN_C ITextParsers *get_textparsers()
{
	return &g_TextParser;
}

PLATFORM_EXTERN_C LogicInitFunction logic_load(uint32_t magic)
{
	if (magic != SM_LOGIC_MAGIC)
	{
		return NULL;
	}

	return logic_init;
}

void CoreNativesToAdd::OnSourceModAllInitialized()
{
	g_CoreNatives.AddNatives(m_NativeList);
}

/* Overload a few things to prevent libstdc++ linking */
#if defined __linux__ || defined __APPLE__
extern "C" void __cxa_pure_virtual(void)
{
}

void *operator new(size_t size)
{
	return malloc(size);
}

void *operator new[](size_t size) 
{
	return malloc(size);
}

void operator delete(void *ptr) 
{
	free(ptr);
}

void operator delete[](void * ptr)
{
	free(ptr);
}
#endif

