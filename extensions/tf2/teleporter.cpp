/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Team Fortress 2 Extension
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

#include "teleporter.h"
#include "extension.h"

KHook::HookID_t g_HookCanPlayerBeTeleportedClass = KHook::INVALID_HOOK;
IForward *g_teleportForward = NULL;

class CTFPlayer;

class CanPlayerBeTeleportedClass
{	
#if defined(__linux__) && defined(__i386__)
public:
	__attribute__((regparm(2))) bool MakeReturn(CTFPlayer * pPlayer) {
		bool ret = *(bool*)::KHook::GetCurrentValuePtr(true);
		::KHook::DestroyReturnValue();
		return ret;
	}

	__attribute__((regparm(2))) bool MakeCallOriginal(CTFPlayer * pPlayer) {
		__attribute__((regparm(2))) bool (CanPlayerBeTeleportedClass::* CanPlayerBeTeleported_Actual)(CTFPlayer *) = nullptr;
		KHook::FillMFP(&CanPlayerBeTeleported_Actual, ::KHook::GetOriginalFunction());

		return ::KHook::ManualReturn<bool>({ KHook::Action::Ignore, (this->*CanPlayerBeTeleported_Actual)(pPlayer) }, true);
	}

	__attribute__((regparm(2))) bool CanPlayerBeTeleported(CTFPlayer * pPlayer); 
};
__attribute__((regparm(2))) bool CanPlayerBeTeleportedClass::CanPlayerBeTeleported(CTFPlayer* pPlayer)
#else
public:
	bool MakeReturn(CTFPlayer * pPlayer) {
		bool ret = *(bool*)::KHook::GetCurrentValuePtr(true);
		::KHook::DestroyReturnValue();
		return ret;
	}

	bool MakeCallOriginal(CTFPlayer * pPlayer) {
		auto CanPlayerBeTeleported_Actual = KHook::BuildMFP<CanPlayerBeTeleportedClass, bool, CTFPlayer*>(::KHook::GetOriginalFunction());

		return ::KHook::ManualReturn<bool>({ KHook::Action::Ignore, (this->*CanPlayerBeTeleported_Actual)(pPlayer) }, true);
	}
	bool CanPlayerBeTeleported(CTFPlayer * pPlayer); 
};
bool CanPlayerBeTeleportedClass::CanPlayerBeTeleported(CTFPlayer* pPlayer)
#endif
{
	bool origCanTeleport = *(bool*)KHook::GetCurrentValuePtr();

	cell_t teleporterCell = gamehelpers->EntityToBCompatRef((CBaseEntity *)this);
	cell_t playerCell = gamehelpers->EntityToBCompatRef((CBaseEntity *)pPlayer);

	if (!g_teleportForward)
	{
		g_pSM->LogMessage(myself, "Teleport forward is invalid");
		return KHook::ManualReturn<bool>({ KHook::Action::Ignore, origCanTeleport });
	}

	cell_t returnValue = origCanTeleport ? 1 : 0;

	g_teleportForward->PushCell(playerCell); // client
	g_teleportForward->PushCell(teleporterCell); // teleporter
	g_teleportForward->PushCellByRef(&returnValue); // return value

	cell_t result = 0;

	g_teleportForward->Execute(&result);

	if (result > Pl_Continue)
	{
		// plugin wants to override the game (returned something other than Plugin_Continue)
		return KHook::ManualReturn<bool>({ KHook::Action::Override, returnValue == 1 });
	}
	else
	{
		return KHook::ManualReturn<bool>({ KHook::Action::Ignore, origCanTeleport }); // let the game decide
	}
}

bool InitialiseTeleporterDetour()
{
	if (g_HookCanPlayerBeTeleportedClass != KHook::INVALID_HOOK) {
		return true;
	}

	void* addr = nullptr;
	if (!g_pGameConf->GetMemSig("CanPlayerBeTeleported", &addr) || addr == nullptr) {
		g_pSM->LogError(myself, "Failed to retrieve CanPlayerBeTeleported.");
		return false;
	}
	g_HookCanPlayerBeTeleportedClass = KHook::SetupHook(
		addr,
		nullptr,
		nullptr,
		nullptr,
		KHook::ExtractMFP(&CanPlayerBeTeleportedClass::CanPlayerBeTeleported),
		KHook::ExtractMFP(&CanPlayerBeTeleportedClass::MakeReturn),
		KHook::ExtractMFP(&CanPlayerBeTeleportedClass::MakeCallOriginal),
		true
	);

	if (g_HookCanPlayerBeTeleportedClass != KHook::INVALID_HOOK)
	{
		return true;
	}

	g_pSM->LogError(myself, "Teleport forward could not be initialized - Disabled hook");

	return false;
}

void RemoveTeleporterDetour()
{
	KHook::RemoveHook(g_HookCanPlayerBeTeleportedClass, true);
	g_HookCanPlayerBeTeleportedClass = KHook::INVALID_HOOK;
}
