// vim: set ts=4 sw=4 tw=99 noet:
// =============================================================================
// SourceMod
// Copyright (C) 2004-2015 AlliedModders LLC.  All rights reserved.
// =============================================================================
//
// This program is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License, version 3.0, as published by the
// Free Software Foundation.
// 
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along with
// this program.  If not, see <http://www.gnu.org/licenses/>.
//
// As a special exception, AlliedModders LLC gives you permission to link the
// code of this program (as well as its derivative works) to "Half-Life 2," the
// "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
// by the Valve Corporation.  You must obey the GNU General Public License in
// all respects for all other code used.  Additionally, AlliedModders LLC grants
// this exception to all derivative works.  AlliedModders LLC defines further
// exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
// or <http://www.sourcemod.net/license.php>.
#ifndef _INCLUDE_SOURCEMOD_CORE_PROVIDER_API_H_
#define _INCLUDE_SOURCEMOD_CORE_PROVIDER_API_H_

#include <stddef.h>
#include <stdint.h>
#include <IAdminSystem.h>
#include <amtl/am-function.h>

namespace SourcePawn {
class ISourcePawnEngine;
class ISourcePawnEngine2;
} // namespace SourcePawn

// SDK types.
#if defined(SM_LOGIC)
class ConCommandBase {};
class ConVar : public ConCommandBase {};
#else
class ConCommandBase;
class ConVar;
#endif
class KeyValues;

struct ServerGlobals
{
	const double *universalTime;
	float *interval_per_tick;
	float *frametime;
};

namespace SourceMod {

class ISourceMod;
class IVEngineServerBridge;
class IFileSystemBridge;
class ITimerSystem;
class IPlayerManager;
class IGameHelpers;
class IMenuManager;
struct DatabaseInfo;
class IPlayerInfoBridge;
class ICommandArgs;

typedef ke::Function<bool(int client, const ICommandArgs*)> CommandFunc;

class CoreProvider
{
public:
	/* Objects */
	ISourceMod		*sm;
	IVEngineServerBridge *engine;
	IFileSystemBridge *filesystem;
	IPlayerInfoBridge *playerInfo;
	ITimerSystem    *timersys;
	IPlayerManager  *playerhelpers;
	IGameHelpers    *gamehelpers;
	IMenuManager	*menus;
	SourcePawn::ISourcePawnEngine **spe1;
	SourcePawn::ISourcePawnEngine2 **spe2;
	const char		*gamesuffix;
	/* Data */
	ServerGlobals   *serverGlobals;
	void *          serverFactory;
	void *          engineFactory;
	void *          matchmakingDSFactory;
	SMGlobalClass *	listeners;

	// ConVar functions.
	virtual ConVar *FindConVar(const char *name) = 0;
	virtual const char *GetCvarString(ConVar *cvar) = 0;
	virtual bool GetCvarBool(ConVar* cvar) = 0;

	// Command functions.
	virtual void DefineCommand(const char *cmd, const char *help, const CommandFunc &callback) = 0;

	// Game description functions.
	virtual bool GetGameName(char *buffer, size_t maxlength) = 0;
	virtual const char *GetGameDescription() = 0;
	virtual const char *GetSourceEngineName() = 0;
	virtual bool SymbolsAreHidden() = 0;

	// Game state and helper functions.
	virtual bool IsMapLoading() = 0;
	virtual bool IsMapRunning() = 0;
	virtual int MaxClients() = 0;
	virtual bool DescribePlayer(int index, const char **namep, const char **authp, int *useridp) = 0;
	virtual void LogToGame(const char *message) = 0;
	virtual void ConPrint(const char *message) = 0;
	virtual void ConsolePrint(const char *fmt, ...) = 0;
	virtual void ConsolePrintVa(const char *fmt, va_list ap) = 0;

	// Game engine helper functions.
	virtual bool IsClientConVarQueryingSupported() = 0;
	virtual int QueryClientConVar(int client, const char *cvar) = 0;

	// Metamod:Source functions.
	virtual int LoadMMSPlugin(const char *file, bool *ok, char *error, size_t maxlength) = 0;
	virtual void UnloadMMSPlugin(int id) = 0;

	const char *	(*GetCoreConfigValue)(const char*);
	void			(*DoGlobalPluginLoads)();
	bool			(*AreConfigsExecuted)();
	void			(*ExecuteConfigs)(IPluginContext *ctx);
	void			(*GetDBInfoFromKeyValues)(KeyValues *, DatabaseInfo *);
	int				(*GetActivityFlags)();
	int				(*GetImmunityMode)();
	void			(*UpdateAdminCmdFlags)(const char *cmd, OverrideType type, FlagBits bits, bool remove);
	bool			(*LookForCommandAdminFlags)(const char *cmd, FlagBits *pFlags);
	int             (*GetGlobalTarget)();
};

} // namespace SourceMod

#endif // _INCLUDE_SOURCEMOD_CORE_PROVIDER_API_H_
