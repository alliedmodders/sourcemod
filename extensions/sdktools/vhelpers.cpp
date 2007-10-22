/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod SDKTools Extension
 * Copyright (C) 2004-2007 AlliedModders LLC.  All rights reserved.
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
#include "vhelpers.h"

CallHelper s_Teleport;
CallHelper s_GetVelocity;
CallHelper s_EyeAngles;

class CTraceFilterSimple : public CTraceFilterEntitiesOnly
{
public:
	CTraceFilterSimple(const IHandleEntity *passentity): m_pPassEnt(passentity)
	{
	}
	virtual bool ShouldHitEntity(IHandleEntity *pServerEntity, int contentsMask)
	{
		if (pServerEntity == m_pPassEnt)
		{
			return false;
		}
		return true;
	}
private:
	const IHandleEntity *m_pPassEnt;
};

bool SetupTeleport()
{
	if (s_Teleport.setup)
	{
		return s_Teleport.supported;
	}

	/* Setup Teleport */
	int offset;
	if (g_pGameConf->GetOffset("Teleport", &offset))
	{
		PassInfo info[3];
		info[0].flags = info[1].flags = info[2].flags = PASSFLAG_BYVAL;
		info[0].size = info[1].size = info[2].size = sizeof(void *);
		info[0].type = info[1].type = info[2].type = PassType_Basic;

		s_Teleport.call = g_pBinTools->CreateVCall(offset, 0, 0, NULL, info, 3);

		if (s_Teleport.call != NULL)
		{
			s_Teleport.supported = true;
		}
	}

	s_Teleport.setup = true;

	return s_Teleport.supported;
}

void Teleport(CBaseEntity *pEntity, Vector *origin, QAngle *ang, Vector *velocity)
{
	unsigned char params[sizeof(void *) * 4];
	unsigned char *vptr = params;
	*(CBaseEntity **)vptr = pEntity;
	vptr += sizeof(CBaseEntity *);
	*(Vector **)vptr = origin;
	vptr += sizeof(Vector *);
	*(QAngle **)vptr = ang;
	vptr += sizeof(QAngle *);
	*(Vector **)vptr = velocity;
	
	s_Teleport.call->Execute(params, NULL);
}

bool IsTeleportSupported()
{
	return SetupTeleport();
}

bool SetupGetVelocity()
{
	if (s_GetVelocity.setup)
	{
		return s_GetVelocity.supported;
	}

	int offset;
	if (g_pGameConf->GetOffset("GetVelocity", &offset))
	{
		PassInfo info[2];
		info[0].flags = info[1].flags = PASSFLAG_BYVAL;
		info[0].size = info[1].size = sizeof(void *);
		info[0].type = info[1].type = PassType_Basic;

		s_GetVelocity.call = g_pBinTools->CreateVCall(offset, 0, 0, NULL, info, 2);

		if (s_GetVelocity.call != NULL)
		{
			s_GetVelocity.supported = true;
		}
	}

	s_GetVelocity.setup = true;

	return s_GetVelocity.supported;
}

void GetVelocity(CBaseEntity *pEntity, Vector *velocity, AngularImpulse *angvelocity)
{
	unsigned char params[sizeof(void *) * 3];
	unsigned char *vptr = params;
	*(CBaseEntity **)vptr = pEntity;
	vptr += sizeof(CBaseEntity *);
	*(Vector **)vptr = velocity;
	vptr += sizeof(Vector *);
	*(AngularImpulse **)vptr = angvelocity;

	s_GetVelocity.call->Execute(params, NULL);
}

bool IsGetVelocitySupported()
{
	return SetupGetVelocity();
}

bool SetupGetEyeAngles()
{
	if (s_EyeAngles.setup)
	{
		return s_EyeAngles.supported;
	}

	int offset;
	if (g_pGameConf->GetOffset("EyeAngles", &offset))
	{
		PassInfo info[2];
		info[0].flags = info[1].flags = PASSFLAG_BYVAL;
		info[0].size = info[1].size = sizeof(void *);
		info[0].type = info[1].type = PassType_Basic;

		s_EyeAngles.call = g_pBinTools->CreateVCall(offset, 0, 0, &info[0], &info[1], 1);

		if (s_EyeAngles.call != NULL)
		{
			s_EyeAngles.supported = true;
		}
	}

	s_EyeAngles.setup = true;

	return s_EyeAngles.supported;
}

bool GetEyeAngles(CBaseEntity *pEntity, QAngle *pAngles)
{
	if (!IsEyeAnglesSupported())
	{
		return false;
	}

	QAngle *pRetAngle = NULL;
	unsigned char params[sizeof(void *)];
	unsigned char *vptr = params;

	*(CBaseEntity **)vptr = pEntity;
	vptr += sizeof(CBaseEntity *);

	s_EyeAngles.call->Execute(params, &pRetAngle);

	if (pRetAngle == NULL)
	{
		return false;
	}

	*pAngles = *pRetAngle;

	return true;
}

int GetClientAimTarget(edict_t *pEdict, bool only_players)
{
	CBaseEntity *pEntity = pEdict->GetUnknown() ? pEdict->GetUnknown()->GetBaseEntity() : NULL;

	if (pEntity == NULL)
	{
		return -1;
	}

	Vector eye_position;
	QAngle eye_angles;

	/* Get the private information we need */
	serverClients->ClientEarPosition(pEdict, &eye_position);
	if (!GetEyeAngles(pEntity, &eye_angles))
	{
		return -2;
	}

	Vector aim_dir;
	AngleVectors(eye_angles, &aim_dir);
	VectorNormalize(aim_dir);

	Vector vec_end = eye_position + aim_dir * 8000;

	Ray_t ray;
	ray.Init(eye_position, vec_end);

	trace_t tr;
	CTraceFilterSimple simple(pEdict->GetIServerEntity());

	enginetrace->TraceRay(ray, MASK_SOLID|CONTENTS_DEBRIS|CONTENTS_HITBOX, &simple, &tr);

	if (tr.fraction == 1.0f || tr.m_pEnt == NULL)
	{
		return -1;
	}

	edict_t *pTarget = gameents->BaseEntityToEdict(tr.m_pEnt);
	if (pTarget == NULL)
	{
		return -1;
	}

	int ent_index = engine->IndexOfEdict(pTarget);

	IGamePlayer *pTargetPlayer = playerhelpers->GetGamePlayer(ent_index);
	if (pTargetPlayer != NULL && !pTargetPlayer->IsInGame())
	{
		return -1;
	}
	else if (only_players && pTargetPlayer == NULL)
	{
		return -1;
	}

	return ent_index;
}

bool IsEyeAnglesSupported()
{
	return SetupGetEyeAngles();
}

void ShutdownHelpers()
{
	s_Teleport.Shutdown();
	s_GetVelocity.Shutdown();
}
