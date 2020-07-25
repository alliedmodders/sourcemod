/**
 * vim: set ts=4 :
 * =============================================================================
 * Source SDK Hooks Extension
 * Copyright (C) 2010-2012 Nicholas Hastings
 * Copyright (C) 2009-2010 Erik Minekus
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

#ifndef TAKEDAMAGEINFOHACK_H
#define TAKEDAMAGEINFOHACK_H
#ifdef _WIN32
#pragma once
#endif

#include "smsdk_ext.h"

#define GAME_DLL 1

#include <isaverestore.h>

#ifndef _DEBUG
#include <ehandle.h>
#else
#undef _DEBUG
#include <ehandle.h>
#define _DEBUG 1
#endif

#include <server_class.h>

#include <shareddefs.h>
#include <takedamageinfo.h>

class CTakeDamageInfoHack : public CTakeDamageInfo
{
public:
	CTakeDamageInfoHack( CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType, CBaseEntity *pWeapon, Vector vecDamageForce, Vector vecDamagePosition );
	int GetAttacker() const;
	void SetAttacker(CBaseEntity *pAttacker);
	inline int GetInflictor() const { return m_hInflictor.IsValid() ? m_hInflictor.GetEntryIndex() : -1; }
#if SOURCE_ENGINE >= SE_ORANGEBOX && SOURCE_ENGINE != SE_LEFT4DEAD
	inline int GetWeapon() const { return m_hWeapon.IsValid() ? m_hWeapon.GetEntryIndex() : -1; }
#else
	inline int GetWeapon() const { return -1; }
	inline void SetWeapon(CBaseEntity *) {}
#endif

	inline void SetDamageForce(vec_t x, vec_t y, vec_t z)
	{
		m_vecDamageForce.x = x;
		m_vecDamageForce.y = y;
		m_vecDamageForce.z = z;
	}

	inline void SetDamagePosition(vec_t x, vec_t y, vec_t z)
	{
		m_vecDamagePosition.x = x;
		m_vecDamagePosition.y = y;
		m_vecDamagePosition.z = z;
	}
#if SOURCE_ENGINE < SE_ORANGEBOX
	inline int GetDamageCustom() const { return GetCustomKill(); }
#endif
};

#endif //TAKEDAMAGEINFOHACK_H
