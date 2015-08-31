// vim: set ts=4 sw=4 tw=99 noet :
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
#ifndef _INCLUDE_SOURCEMOD_CORE_PROVIDER_IMPL_H_
#define _INCLUDE_SOURCEMOD_CORE_PROVIDER_IMPL_H_

#include "logic/intercom.h"
#include <amtl/os/am-shared-library.h>

class CoreProviderImpl : public CoreProvider
{
public:
	CoreProviderImpl();

	// Local functions.
	void InitializeBridge();
	bool LoadBridge(char *error, size_t maxlength);
	void ShutdownBridge();

	// Provider implementation.
	ConVar *FindConVar(const char *name) override;
	const char *GetCvarString(ConVar *cvar) override;
	bool GetCvarBool(ConVar* cvar) override;
	bool GetGameName(char *buffer, size_t maxlength) override;
	const char *GetGameDescription() override;
	const char *GetSourceEngineName() override;
	bool SymbolsAreHidden() override;
	bool IsMapLoading() override;
	bool IsMapRunning() override;
	int MaxClients() override;
	bool DescribePlayer(int index, const char **namep, const char **authp, int *useridp) override;
	void LogToGame(const char *message) override;
	void ConPrint(const char *message) override;
	void ConsolePrintVa(const char *fmt, va_list ap) override;
	int LoadMMSPlugin(const char *file, bool *ok, char *error, size_t maxlength) override;
	void UnloadMMSPlugin(int id) override;

private:
	ke::Ref<ke::SharedLib> logic_;
	LogicInitFunction logic_init_;
};

extern CoreProviderImpl sCoreProviderImpl;

#endif // _INCLUDE_SOURCEMOD_CORE_PROVIDER_IMPL_H_
