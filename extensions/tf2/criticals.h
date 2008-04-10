/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod TF2 Extension
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

#include "extension.h"
#include <jit/jit_helpers.h>
#include <jit/x86/x86_macros.h>
#include "detours.h"

class CriticalHitManager
{
public:
	CriticalHitManager()
	{
		enabled = false;
		detoured = false;
		critical_address = NULL;
		critical_callback = NULL;

		melee_address = NULL;
		melee_callback = NULL;

		knife_address = NULL;
		knife_callback = NULL;

		forward = NULL;
	}

	~CriticalHitManager()
	{
		forwards->ReleaseForward(forward);
		DeleteCriticalDetour();
	}

	void Init()
	{
		if (!CreateCriticalDetour() || !CreateCriticalMeleeDetour() || !CreateCriticalKnifeDetour())
		{
			enabled = false;
			return;
		}

		forward = forwards->CreateForward("TF2_CalcIsAttackCritical", ET_Hook, 4, NULL, Param_Cell, Param_Cell, Param_String, Param_CellByRef);

		if (!forward)
		{
			g_pSM->LogError(myself, "Failed to create forward - Disabling critical hit hook");
			enabled = false;
			return;
		}

		/* TODO: Only enable this once forwards exist. Requires IForwardListener functionality */
		EnableCriticalDetour();

		enabled = true;
	}

	bool IsEnabled()
	{
		return enabled;
	}

	bool CriticalDetour(void *pWeapon);

private:
	IForward *forward;

	/* These create/delete the allocated memory */
	bool CreateCriticalDetour();
	bool CreateCriticalMeleeDetour();
	bool CreateCriticalKnifeDetour();
	void DeleteCriticalDetour();

	/* These patch/unpatch the server.dll */
	void EnableCriticalDetour();
	void DisableCriticalDetour();

	bool enabled;
	bool detoured;

	patch_t critical_restore;
	void *critical_address;
	void *critical_callback;

	patch_t melee_restore;
	void *melee_address;
	void *melee_callback;

	patch_t knife_restore;
	void *knife_address;
	void *knife_callback;
};

bool TempDetour(void *pWeapon);

extern ISourcePawnEngine *spengine;
extern IServerGameEnts *gameents;
extern CriticalHitManager g_CriticalHitManager;
