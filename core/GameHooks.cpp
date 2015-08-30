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
#include "GameHooks.h"
#include "sourcemod.h"
#include "ConVarManager.h"

#if SOURCE_ENGINE >= SE_ORANGEBOX
SH_DECL_HOOK3_void(ICvar, CallGlobalChangeCallbacks, SH_NOATTRIB, false, ConVar *, const char *, float);
#else
SH_DECL_HOOK2_void(ICvar, CallGlobalChangeCallback, SH_NOATTRIB, false, ConVar *, const char *);
#endif

GameHooks::GameHooks()
{
}

void GameHooks::Start()
{
#if SOURCE_ENGINE >= SE_ORANGEBOX
	SH_ADD_HOOK(ICvar, CallGlobalChangeCallbacks, icvar, SH_STATIC(OnConVarChanged), false);
#else
	SH_ADD_HOOK(ICvar, CallGlobalChangeCallback, icvar, SH_STATIC(OnConVarChanged), false);
#endif
}

void GameHooks::Shutdown()
{
#if SOURCE_ENGINE >= SE_ORANGEBOX
	SH_REMOVE_HOOK(ICvar, CallGlobalChangeCallbacks, icvar, SH_STATIC(OnConVarChanged), false);
#else
	SH_REMOVE_HOOK(ICvar, CallGlobalChangeCallback, icvar, SH_STATIC(OnConVarChanged), false);
#endif
}

#if SOURCE_ENGINE >= SE_ORANGEBOX
void GameHooks::OnConVarChanged(ConVar *pConVar, const char *oldValue, float flOldValue)
{
#else
void GameHooks::OnConVarChanged(ConVar *pConVar, const char *oldValue)
{
  float flOldValue = atof(oldValue);
#endif
  g_ConVarManager.OnConVarChanged(pConVar, oldValue, flOldValue);
}
