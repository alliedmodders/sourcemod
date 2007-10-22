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

const char *GetDTTypeName(int type)
{
	switch (type)
	{
	case DPT_Int:
		{
			return "integer";
		}
	case DPT_Float:
		{
			return "float";
		}
	case DPT_Vector:
		{
			return "vector";
		}
	case DPT_String:
		{
			return "string";
		}
	case DPT_Array:
		{
			return "array";
		}
	case DPT_DataTable:
		{
			return "datatable";
		}
	default:
		{
			return NULL;
		}
	}

	return NULL;
}

void UTIL_DrawSendTable_XML(FILE *fp, SendTable *pTable, int space_count)
{
	char spaces[255];

	for (int i = 0; i < space_count; i++)
	{
		spaces[i] = ' ';
	}
	spaces[space_count] = '\0';

	const char *type_name;
	SendTable *pOtherTable;
	SendProp *pProp;

	fprintf(fp, " %s<sendtable name=\"%s\">\n", spaces, pTable->GetName());
	for (int i = 0; i < pTable->GetNumProps(); i++)
	{
		pProp = pTable->GetProp(i);

		fprintf(fp, "  %s<property name=\"%s\">\n", spaces, pProp->GetName());

		if ((type_name = GetDTTypeName(pProp->GetType())) != NULL)
		{
			fprintf(fp, "   %s<type>%s</type>\n", spaces, type_name);
		}
		else
		{
			fprintf(fp, "   %s<type>%d</type>\n", spaces, pProp->GetType());
		}

		fprintf(fp, "   %s<offset>%d</offset>\n", spaces, pProp->GetOffset());
		fprintf(fp, "   %s<bits>%d</bits>\n", spaces, pProp->m_nBits);

		if ((pOtherTable = pTable->GetProp(i)->GetDataTable()) != NULL)
		{
			UTIL_DrawSendTable_XML(fp, pOtherTable, space_count + 3);
		}

		fprintf(fp, "  %s</property>\n", spaces);
	}
	fprintf(fp, " %s</sendtable>\n", spaces);
}

void UTIL_DrawServerClass_XML(FILE *fp, ServerClass *sc)
{
	fprintf(fp, "<serverclass name=\"%s\">\n", sc->GetName());
	UTIL_DrawSendTable_XML(fp, sc->m_pTable, 0);
	fprintf(fp, "</serverclass>\n");
}

void UTIL_DrawSendTable(FILE *fp, SendTable *pTable, int level)
{
	char spaces[255];
	for (int i=0; i<level; i++)
		spaces[i] = ' ';
	spaces[level] = '\0';

	const char *name, *type;
	SendProp *pProp;

	fprintf(fp, "%sSub-Class Table (%d Deep): %s\n", spaces, level, pTable->GetName());

	for (int i=0; i<pTable->GetNumProps(); i++)
	{
		pProp = pTable->GetProp(i);
		name = pProp->GetName();
		if (pProp->GetDataTable())
		{
			UTIL_DrawSendTable(fp, pProp->GetDataTable(), level + 1);
		}
		else
		{
			type = GetDTTypeName(pProp->GetType());

			if (type != NULL)
			{
				fprintf(fp,
					"%s-Member: %s (offset %d) (type %s) (bits %d)\n", 
					spaces, 
					pProp->GetName(),
					pProp->GetOffset(),
					type,
					pProp->m_nBits);
			}
			else
			{
				fprintf(fp,
					"%s-Member: %s (offset %d) (type %d) (bits %d)\n", 
					spaces, 
					pProp->GetName(),
					pProp->GetOffset(),
					pProp->GetType(),
					pProp->m_nBits);
			}
		}
	}
}

CON_COMMAND(sm_dump_netprops_xml, "Dumps the networkable property table as an XML file")
{
#if !defined ORANGEBOX_BUILD
	CCommand args;
#endif

	if (args.ArgC() < 2)
	{
		META_CONPRINT("Usage: sm_dump_netprops_xml <file>\n");
		return;
	}

	const char *file = args.Arg(1);
	if (!file || file[0] == '\0')
	{
		META_CONPRINT("Usage: sm_dump_netprops_xml <file>\n");
		return;
	}

	char path[PLATFORM_MAX_PATH];
	g_pSM->BuildPath(Path_Game, path, sizeof(path), "%s", file);

	FILE *fp = NULL;
	if ((fp = fopen(path, "wt")) == NULL)
	{
		META_CONPRINTF("Could not open file \"%s\"\n", path);
		return;
	}

	fprintf(fp, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\n");
	fprintf(fp, "<!-- Dump of all network properties for \"%s\" follows -->\n\n", g_pSM->GetGameFolderName());

	ServerClass *pBase = gamedll->GetAllServerClasses();
	while (pBase != NULL)
	{
		UTIL_DrawServerClass_XML(fp, pBase);
		pBase = pBase->m_pNext;
	}

	fclose(fp);
}

CON_COMMAND(sm_dump_netprops, "Dumps the networkable property table as a text file")
{
#if !defined ORANGEBOX_BUILD
	CCommand args;
#endif

	if (args.ArgC() < 2)
	{
		META_CONPRINT("Usage: sm_dump_netprops <file>\n");
		return;
	}

	const char *file = args.Arg(1);
	if (!file || file[0] == '\0')
	{
		META_CONPRINT("Usage: sm_dump_netprops <file>\n");
		return;
	}

	char path[PLATFORM_MAX_PATH];
	g_pSM->BuildPath(Path_Game, path, sizeof(path), "%s", file);

	FILE *fp = NULL;
	if ((fp = fopen(path, "wt")) == NULL)
	{
		META_CONPRINTF("Could not open file \"%s\"\n", path);
		return;
	}

	fprintf(fp, "// Dump of all network properties for \"%s\" follows\n//\n\n", g_pSM->GetGameFolderName());

	ServerClass *pBase = gamedll->GetAllServerClasses();
	while (pBase != NULL)
	{
		fprintf(fp, "%s:\n", pBase->GetName());
		UTIL_DrawSendTable(fp, pBase->m_pTable, 1);
		pBase = pBase->m_pNext;
	}

	fclose(fp);
}

