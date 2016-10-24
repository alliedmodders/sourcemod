/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod SDKTools Extension
 * Copyright (C) 2004-2010 AlliedModders LLC.  All rights reserved.
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
#include "util.h"
#include "vhelpers.h"
#include "vglobals.h"

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
		PassInfo info[1];
		info[0].flags = PASSFLAG_BYVAL;
		info[0].size = sizeof(void *);
		info[0].type = PassType_Basic;

		s_EyeAngles.call = g_pBinTools->CreateVCall(offset, 0, 0, info, NULL, 0);

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

	int ent_ref = gamehelpers->EntityToBCompatRef(tr.m_pEnt);
	int ent_index = gamehelpers->ReferenceToIndex(ent_ref);

	IGamePlayer *pTargetPlayer = playerhelpers->GetGamePlayer(ent_index);
	if (pTargetPlayer != NULL && !pTargetPlayer->IsInGame())
	{
		return -1;
	}
	else if (only_players && pTargetPlayer == NULL)
	{
		return -1;
	}

	return ent_ref;
}

bool IsEyeAnglesSupported()
{
	return SetupGetEyeAngles();
}

bool GetPlayerInfo(int client, player_info_t *info)
{
#if SOURCE_ENGINE >= SE_ORANGEBOX
	return engine->GetPlayerInfo(client, info);
#else
	return (iserver) ? iserver->GetPlayerInfo(client-1, info) : false;
#endif
}

void ShutdownHelpers()
{
	s_Teleport.Shutdown();
	s_GetVelocity.Shutdown();
	s_EyeAngles.Shutdown();
}

bool FindNestedDataTable(SendTable *pTable, const char *name)
{
	if (strcmp(pTable->GetName(), name) == 0)
	{
		return true;
	}

	int props = pTable->GetNumProps();
	SendProp *prop;

	for (int i=0; i<props; i++)
	{
		prop = pTable->GetProp(i);
		if (prop->GetDataTable())
		{
			if (FindNestedDataTable(prop->GetDataTable(), name))
			{
				return true;
			}
		}
	}

	return false;
}

char *UTIL_SendFlagsToString(int flags, int type)
{
	static char str[1024];
	str[0] = 0;

	if (flags & SPROP_UNSIGNED)
	{
		strcat(str, "Unsigned|");
	}
	if (flags & SPROP_COORD)
	{
		strcat(str, "Coord|");
	}
	if (flags & SPROP_NOSCALE)
	{
		strcat(str, "NoScale|");
	}
	if (flags & SPROP_ROUNDDOWN)
	{
		strcat(str, "RoundDown|");
	}
	if (flags & SPROP_ROUNDUP)
	{
		strcat(str, "RoundUp|");
	}
	if (flags & SPROP_NORMAL)
	{
		if (type == DPT_Int)
		{
			strcat(str, "VarInt|");
		}
		else
		{
			strcat(str, "Normal|");
		}
	}
	if (flags & SPROP_EXCLUDE)
	{
		strcat(str, "Exclude|");
	}
	if (flags & SPROP_XYZE)
	{
		strcat(str, "XYZE|");
	}
	if (flags & SPROP_INSIDEARRAY)
	{
		strcat(str, "InsideArray|");
	}
	if (flags & SPROP_PROXY_ALWAYS_YES)
	{
		strcat(str, "AlwaysProxy|");
	}
	if (flags & SPROP_CHANGES_OFTEN)
	{
		strcat(str, "ChangesOften|");
	}
	if (flags & SPROP_IS_A_VECTOR_ELEM)
	{
		strcat(str, "VectorElem|");
	}
	if (flags & SPROP_COLLAPSIBLE)
	{
		strcat(str, "Collapsible|");
	}
#if SOURCE_ENGINE >= SE_ORANGEBOX
	if (flags & SPROP_COORD_MP)
	{
		strcat(str, "CoordMP|");
	}
	if (flags & SPROP_COORD_MP_LOWPRECISION)
	{
		strcat(str, "CoordMPLowPrec|");
	}
	if (flags & SPROP_COORD_MP_INTEGRAL)
	{
		strcat(str, "CoordMpIntegral|");
	}
#endif

	int len = strlen(str) - 1;
	if (len > 0)
	{
		str[len] = 0; // Strip the final '|'
	}

	return str;
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
		fprintf(fp, "   %s<flags>%s</flags>\n", spaces, UTIL_SendFlagsToString(pProp->GetFlags(), pProp->GetType()));

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

void UTIL_DrawSendTable(FILE *fp, SendTable *pTable, int level = 1)
{
	SendProp *pProp;
	const char *type;

	for (int i = 0; i < pTable->GetNumProps(); i++)
	{
		pProp = pTable->GetProp(i);
		if (pProp->GetDataTable())
		{
			fprintf(fp, "%*sTable: %s (offset %d) (type %s)\n", 
				level, "", 
				pProp->GetName(), 
				pProp->GetOffset(), 
				pProp->GetDataTable()->GetName());
			
			UTIL_DrawSendTable(fp, pProp->GetDataTable(), level + 1);
		}
		else
		{
			type = GetDTTypeName(pProp->GetType());

			if (type != NULL)
			{
				fprintf(fp,
					"%*sMember: %s (offset %d) (type %s) (bits %d) (%s)\n", 
					level, "", 
					pProp->GetName(),
					pProp->GetOffset(),
					type,
					pProp->m_nBits,
					UTIL_SendFlagsToString(pProp->GetFlags(), pProp->GetType()));
			}
			else
			{
				fprintf(fp,
					"%*sMember: %s (offset %d) (type %d) (bits %d) (%s)\n", 
					level, "", 
					pProp->GetName(),
					pProp->GetOffset(),
					pProp->GetType(),
					pProp->m_nBits,
					UTIL_SendFlagsToString(pProp->GetFlags(), pProp->GetType()));
			}
		}
	}
}

#if defined SUBPLATFORM_SECURECRT
void _ignore_invalid_parameter(
	const wchar_t * expression,
	const wchar_t * function, 
	const wchar_t * file,
	unsigned int line,
	uintptr_t pReserved
	)
{
	/* Wow we don't care, thanks Microsoft. */
}
#endif

CON_COMMAND(sm_dump_netprops_xml, "Dumps the networkable property table as an XML file")
{
#if SOURCE_ENGINE <= SE_DARKMESSIAH
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

	char buffer[80];
	buffer[0] = 0;

#if defined SUBPLATFORM_SECURECRT
	_invalid_parameter_handler handler = _set_invalid_parameter_handler(_ignore_invalid_parameter);
#endif

	time_t t = g_pSM->GetAdjustedTime();
	size_t written = strftime(buffer, sizeof(buffer), "%Y/%m/%d", localtime(&t));

#if defined SUBPLATFORM_SECURECRT
	_set_invalid_parameter_handler(handler);
#endif

	fprintf(fp, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\n");
	fprintf(fp, "<!-- Dump of all network properties for \"%s\" as at %s -->\n\n", g_pSM->GetGameFolderName(), buffer);

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
#if SOURCE_ENGINE <= SE_DARKMESSIAH
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
	
	char buffer[80];
	buffer[0] = 0;

#if defined SUBPLATFORM_SECURECRT
	_invalid_parameter_handler handler = _set_invalid_parameter_handler(_ignore_invalid_parameter);
#endif

	time_t t = g_pSM->GetAdjustedTime();
	size_t written = strftime(buffer, sizeof(buffer), "%Y/%m/%d", localtime(&t));

#if defined SUBPLATFORM_SECURECRT
	_set_invalid_parameter_handler(handler);
#endif

	fprintf(fp, "// Dump of all network properties for \"%s\" as at %s\n//\n\n", g_pSM->GetGameFolderName(), buffer);

	ServerClass *pBase = gamedll->GetAllServerClasses();
	while (pBase != NULL)
	{
		fprintf(fp, "%s (type %s)\n", pBase->GetName(), pBase->m_pTable->GetName());
		UTIL_DrawSendTable(fp, pBase->m_pTable);
		pBase = pBase->m_pNext;
	}

	fclose(fp);
}

CEntityFactoryDictionary *GetEntityFactoryDictionary()
{
	static CEntityFactoryDictionary *dict = NULL;

#if SOURCE_ENGINE == SE_TF2        \
	|| SOURCE_ENGINE == SE_CSS     \
	|| SOURCE_ENGINE == SE_DODS    \
	|| SOURCE_ENGINE == SE_HL2DM   \
	|| SOURCE_ENGINE == SE_SDK2013 \
	|| SOURCE_ENGINE == SE_BMS     \
	|| SOURCE_ENGINE == SE_NUCLEARDAWN
	dict = (CEntityFactoryDictionary *) servertools->GetEntityFactoryDictionary();
#else
	if (dict == NULL)
	{
		void *addr;
		if (g_pGameConf->GetMemSig("EntityFactoryFinder", (void **) &addr) && addr)
		{
			int offset;
			if (!g_pGameConf->GetOffset("EntityFactoryOffset", &offset) || !offset)
			{
				return NULL;
			}
			dict = *reinterpret_cast<CEntityFactoryDictionary **>((intptr_t) addr + offset);
		}
	}

	if (dict == NULL)
	{
		ICallWrapper *pWrapper = NULL;

		PassInfo retData;
		retData.flags = PASSFLAG_BYVAL;
		retData.size = sizeof(void *);
		retData.type = PassType_Basic;

		void *addr;
		if (!g_pGameConf->GetMemSig("EntityFactory", &addr) || !addr)
		{
			int offset;

			if (!g_pGameConf->GetMemSig("EntityFactoryCaller", &addr) || !addr)
				return NULL;

			if (!g_pGameConf->GetOffset("EntityFactoryCallOffset", &offset))
				return NULL;

			// Get relative offset to function
			int32_t funcOffset = *(int32_t *)((intptr_t)addr + offset);

			// Get real address of function
			// Address of signature + offset of relative offset + sizeof(int32_t) offset + relative offset
			addr = (void *)((intptr_t)addr + offset + 4 + funcOffset);
		}

		pWrapper = g_pBinTools->CreateCall(addr, CallConv_Cdecl, &retData, NULL, 0);

		if (pWrapper)
		{
			void *returnData = NULL;

			pWrapper->Execute(NULL, &returnData);

			pWrapper->Destroy();

			if (returnData == NULL)
			{
				return NULL;
			}

			dict = (CEntityFactoryDictionary *) returnData;
		}
	}
#endif

	return dict;
}

CON_COMMAND(sm_dump_classes, "Dumps the class list as a text file")
{
#if SOURCE_ENGINE <= SE_DARKMESSIAH
	CCommand args;
#endif

	if (args.ArgC() < 2)
	{
		META_CONPRINT("Usage: sm_dump_classes <file>\n");
		return;
	}

	const char *file = args.Arg(1);
	if (!file || file[0] == '\0')
	{
		META_CONPRINT("Usage: sm_dump_classes <file>\n");
		return;
	}

	CEntityFactoryDictionary *dict = GetEntityFactoryDictionary();
	if (dict == NULL)
	{
		META_CONPRINT("Failed to locate function\n");
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

	char buffer[80];
	buffer[0] = 0;

#if defined SUBPLATFORM_SECURECRT
	_invalid_parameter_handler handler = _set_invalid_parameter_handler(_ignore_invalid_parameter);
#endif

	time_t t = g_pSM->GetAdjustedTime();
	size_t written = strftime(buffer, sizeof(buffer), "%Y/%m/%d", localtime(&t));

#if defined SUBPLATFORM_SECURECRT
	_set_invalid_parameter_handler(handler);
#endif

	fprintf(fp, "// Dump of all classes for \"%s\" as at %s\n//\n\n", g_pSM->GetGameFolderName(), buffer);

	sm_datatable_info_t info;
	for ( int i = dict->m_Factories.First(); i != dict->m_Factories.InvalidIndex(); i = dict->m_Factories.Next( i ) )
	{
		IServerNetworkable *entity = dict->Create(dict->m_Factories.GetElementName(i));
		ServerClass *sclass = entity->GetServerClass();
		fprintf(fp,"%s - %s\n",sclass->GetName(), dict->m_Factories.GetElementName(i));
		
		if (!gamehelpers->FindDataMapInfo(gamehelpers->GetDataMap(entity->GetBaseEntity()), "m_iEFlags", &info))
			continue;
		
		int *eflags = (int *)((char *)entity->GetBaseEntity() + info.actual_offset);
		*eflags |= (1<<0); // EFL_KILLME
	}

	fclose(fp);

}

char *UTIL_DataFlagsToString(int flags)
{
	static char str[1024];
	str[0] = 0;

	if (flags & FTYPEDESC_GLOBAL)
	{
		strcat(str,	"Global|");
	}
	if (flags & FTYPEDESC_SAVE)
	{
		strcat(str,	"Save|");
	}
	if (flags & FTYPEDESC_KEY)
	{
		strcat(str,	"Key|");
	}
	if (flags & FTYPEDESC_INPUT)
	{
		strcat(str,	"Input|");
	}
	if (flags & FTYPEDESC_OUTPUT)
	{
		strcat(str,	"Output|");
	}
	if (flags & FTYPEDESC_FUNCTIONTABLE)
	{
		strcat(str,	"FunctionTable|");
	}
	if (flags & FTYPEDESC_PTR)
	{
		strcat(str,	"Ptr|");
	}
	if (flags & FTYPEDESC_OVERRIDE)
	{
		strcat(str,	"Override|");
	}

	int len = strlen(str) - 1;
	if (len > 0)
	{
		str[len] = 0; // Strip the final '|'
	}

	return str;
}

void UTIL_DrawDataTable(FILE *fp, datamap_t *pMap, int level)
{
	char spaces[255];

	for (int i=0; i<level; i++)
	{
		spaces[i] = ' ';
	}

	spaces[level] = '\0';

	const char *externalname;
	char *flags;

	while (pMap)
	{
		for (int i=0; i<pMap->dataNumFields; i++)
		{
			if (pMap->dataDesc[i].fieldName == NULL)
			{
				continue;
			}

			if (pMap->dataDesc[i].td)
			{
				fprintf(fp, " %sSub-Class Table (%d Deep): %s - %s\n", spaces, level+1, pMap->dataDesc[i].fieldName, pMap->dataDesc[i].td->dataClassName);
				UTIL_DrawDataTable(fp, pMap->dataDesc[i].td, level+1);
			}
			else
			{
				externalname  = pMap->dataDesc[i].externalName;
				flags = UTIL_DataFlagsToString(pMap->dataDesc[i].flags);

				if (externalname == NULL)
				{
					fprintf(fp, "%s- %s (Offset %d) (%s)(%i Bytes)\n", spaces, pMap->dataDesc[i].fieldName, GetTypeDescOffs(&pMap->dataDesc[i]), flags, pMap->dataDesc[i].fieldSizeInBytes);
				}
				else
				{
					fprintf(fp, "%s- %s (Offset %d) (%s)(%i Bytes) - %s\n", spaces, pMap->dataDesc[i].fieldName, GetTypeDescOffs(&pMap->dataDesc[i]), flags, pMap->dataDesc[i].fieldSizeInBytes, externalname);
				}
			}
		}
		pMap = pMap->baseMap;
	}
}

CON_COMMAND(sm_dump_datamaps, "Dumps the data map list as a text file")
{
#if SOURCE_ENGINE <= SE_DARKMESSIAH
	CCommand args;
#endif

	if (args.ArgC() < 2)
	{
		META_CONPRINT("Usage: sm_dump_datamaps <file>\n");
		return;
	}

	const char *file = args.Arg(1);
	if (!file || file[0] == '\0')
	{
		META_CONPRINT("Usage: sm_dump_datamaps <file>\n");
		return;
	}

	CEntityFactoryDictionary *dict = GetEntityFactoryDictionary();
	if ( dict == NULL )
	{
		META_CONPRINT("Failed to locate function\n");
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

	char buffer[80];
	buffer[0] = 0;

#if defined SUBPLATFORM_SECURECRT
	_invalid_parameter_handler handler = _set_invalid_parameter_handler(_ignore_invalid_parameter);
#endif

	time_t t = g_pSM->GetAdjustedTime();
	size_t written = strftime(buffer, sizeof(buffer), "%Y/%m/%d", localtime(&t));

#if defined SUBPLATFORM_SECURECRT
	_set_invalid_parameter_handler(handler);
#endif

	fprintf(fp, "// Dump of all datamaps for \"%s\" as at %s\n//\n//\n", g_pSM->GetGameFolderName(), buffer);


	fprintf(fp, "// Flag Details:\n//\n");

	fprintf(fp, "// Global: This field is masked for global entity save/restore\n");
	fprintf(fp, "// Save: This field is saved to disk\n");
	fprintf(fp, "// Key: This field can be requested and written to by string name at load time\n");
	fprintf(fp, "// Input: This field can be written to by string name at run time, and a function called\n");
	fprintf(fp, "// Output: This field propogates it's value to all targets whenever it changes\n");
	fprintf(fp, "// FunctionTable: This is a table entry for a member function pointer\n");
	fprintf(fp, "// Ptr: This field is a pointer, not an embedded object\n");
	fprintf(fp, "// Override: The field is an override for one in a base class (only used by prediction system for now)\n");

	fprintf(fp, "//\n\n");

	static int offsEFlags = -1;
	for ( int i = dict->m_Factories.First(); i != dict->m_Factories.InvalidIndex(); i = dict->m_Factories.Next( i ) )
	{
		IServerNetworkable *entity = dict->Create(dict->m_Factories.GetElementName(i));
		ServerClass *sclass = entity->GetServerClass();
		datamap_t *pMap = gamehelpers->GetDataMap(entity->GetBaseEntity());

		fprintf(fp,"%s - %s\n", sclass->GetName(), dict->m_Factories.GetElementName(i));

		UTIL_DrawDataTable(fp, pMap, 0);

		if (offsEFlags == -1)
		{
			sm_datatable_info_t info;
			if (!gamehelpers->FindDataMapInfo(pMap, "m_iEFlags", &info))
			{
				continue;
			}

			offsEFlags = info.actual_offset;
		}
		
		int *eflags = (int *)((char *)entity->GetBaseEntity() + offsEFlags);
		*eflags |= (1<<0); // EFL_KILLME
	}

	fclose(fp);

}
