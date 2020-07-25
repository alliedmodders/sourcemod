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

#include "extension.h"
#include "takedamageinfohack.h"

CTakeDamageInfo::CTakeDamageInfo(){}

CTakeDamageInfoHack::CTakeDamageInfoHack( CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType, CBaseEntity *pWeapon, Vector vecDamageForce, Vector vecDamagePosition )
{
	m_hInflictor = pInflictor;
	if ( pAttacker )
	{
		SetAttacker(pAttacker);
	}
	else
	{
		SetAttacker(pInflictor);
	}

#if SOURCE_ENGINE >= SE_ORANGEBOX && SOURCE_ENGINE != SE_LEFT4DEAD
	m_hWeapon = pWeapon;
#endif

	m_flDamage = flDamage;

	m_flBaseDamage = BASEDAMAGE_NOT_SPECIFIED;

	m_bitsDamageType = bitsDamageType;

	m_flMaxDamage = flDamage;
	m_vecDamageForce = vecDamageForce;
	m_vecDamagePosition = vecDamagePosition;
	m_vecReportedPosition = vec3_origin;
	m_iAmmoType = -1;

#if SOURCE_ENGINE < SE_ORANGEBOX
	m_iCustomKillType = 0;
#else
	m_iDamageCustom = 0;
#endif

#if SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_HL2DM || SOURCE_ENGINE == SE_DODS || SOURCE_ENGINE == SE_SDK2013 \
	|| SOURCE_ENGINE == SE_BMS || SOURCE_ENGINE == SE_TF2
	m_iDamagedOtherPlayers = 0;
	m_iPlayerPenetrationCount = 0;
	m_flDamageBonus = 0.0f;
	m_bForceFriendlyFire = false;
#endif

#if SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_HL2DM || SOURCE_ENGINE == SE_DODS || SOURCE_ENGINE == SE_TF2
	m_flDamageForForce = 0.f;
#endif

#if SOURCE_ENGINE == SE_TF2
	m_eCritType = kCritType_None;
#endif

#if SOURCE_ENGINE >= SE_ALIENSWARM
	m_flRadius = 0.0f;
#endif

#if SOURCE_ENGINE == SE_INSURGENCY || SOURCE_ENGINE == SE_DOI || SOURCE_ENGINE == SE_CSGO || SOURCE_ENGINE == SE_BLADE
	m_iDamagedOtherPlayers = 0;
	m_iObjectsPenetrated = 0;
	m_uiBulletID = 0;
	m_uiRecoilIndex = 0;
#endif
}

#if SOURCE_ENGINE == SE_CSGO
int CTakeDamageInfoHack::GetAttacker() const
{
	return m_CSGOAttacker.m_hHndl.IsValid() ? m_CSGOAttacker.m_hHndl.GetEntryIndex() : -1;
}

void CTakeDamageInfoHack::SetAttacker(CBaseEntity *pAttacker)
{
	m_CSGOAttacker.m_bNeedInit = false;
	m_CSGOAttacker.m_hHndl = pAttacker;
	m_CSGOAttacker.m_bIsWorld = true;

	int entity = gamehelpers->EntityToBCompatRef(pAttacker);
	IGamePlayer *player = playerhelpers->GetGamePlayer(entity);
	if (player) {
		m_CSGOAttacker.m_bIsWorld = false;
		m_CSGOAttacker.m_bIsPlayer = true;
		m_CSGOAttacker.m_iClientIndex = player->GetIndex();
		m_CSGOAttacker.m_iUserId = player->GetUserId();

		IPlayerInfo *playerinfo = player->GetPlayerInfo();
		if (!playerinfo) {
			return;
		}
		m_CSGOAttacker.m_iTeamChecked = playerinfo->GetTeamIndex();
		m_CSGOAttacker.m_iTeamNum = playerinfo->GetTeamIndex();
	}
}
#else
int CTakeDamageInfoHack::GetAttacker() const
{
	return m_hAttacker.IsValid() ? m_hAttacker.GetEntryIndex() : -1;
}

void CTakeDamageInfoHack::SetAttacker(CBaseEntity *pAttacker)
{
	CTakeDamageInfo::SetAttacker(pAttacker);
}
#endif
