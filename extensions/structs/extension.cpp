/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Sample Extension
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

/**
 * @file extension.cpp
 * @brief Implement extension code here.
 */

Structs g_Structs;		/**< Global singleton for extension's main interface */

SMEXT_LINK(&g_Structs);

HandleType_t g_StructHandle = 0;
StructHandler g_StructHandler;

IGameConfig *conf;

size_t UTIL_Format(char *buffer, size_t maxlength, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	size_t len = vsnprintf(buffer, maxlength, fmt, ap);
	va_end(ap);

	if (len >= maxlength)
	{
		buffer[maxlength - 1] = '\0';
		return (maxlength - 1);
	}
	else
	{
		return len;
	}
}

void Structs::SDK_OnAllLoaded()
{
	g_StructHandle = handlesys->CreateType("Struct", 
		&g_StructHandler, 
		0, 
		NULL, 
		NULL, 
		myself->GetIdentity(), 
		NULL);

	gameconfs->AddUserConfigHook("Structs", this);

	sharesys->AddNatives(myself, MyNatives);


	char error[100];
	if (!gameconfs->LoadGameConfigFile("structs.gamedata", &conf, error, sizeof(error)))
	{
		g_pSM->LogError(myself, "Parsing Failed!");
	}
}

void Structs::SDK_OnUnload()
{
	handlesys->RemoveType(g_StructHandle, myself->GetIdentity());
	gameconfs->CloseGameConfigFile(conf);
	gameconfs->RemoveUserConfigHook("Structs", this);
}

bool Structs::SDK_OnLoad( char *error, size_t maxlength, bool late )
{
	sharesys->AddInterface(myself, &g_StructManager);

	m_typeLookup.insert("int", Member_Int32);
	m_typeLookup.insert("int*", Member_Int32Pointer);
	m_typeLookup.insert("float", Member_Float);
	m_typeLookup.insert("float*", Member_FloatPointer);
	m_typeLookup.insert("char", Member_Char);
	m_typeLookup.insert("char*", Member_CharPointer);
	m_typeLookup.insert("Vector", Member_Vector);
	m_typeLookup.insert("Vector*", Member_VectorPointer);
	m_typeLookup.insert("ent", Member_EHandle);
	m_typeLookup.insert("ent*", Member_EHandlePointer);

	return true;
}

void Structs::ReadSMC_ParseStart()
{
	m_bInStruct = false;
	m_currentStruct = NULL;
	m_bInMember = false;
	m_currentMember = NULL;
}

SourceMod::SMCResult Structs::ReadSMC_NewSection( const SMCStates *states, const char *name )
{
	if (!m_bInStruct)
	{
		m_currentStruct = new StructInfo();
		UTIL_Format(m_currentStruct->name, sizeof(m_currentStruct->name), "%s", name);
		m_bInStruct = true;

		return SMCResult_Continue;
	}

	if (!m_bInMember)
	{
		m_bInMember = true;
		m_currentMember = new MemberInfo();
		UTIL_Format(m_currentMember->name, sizeof(m_currentMember->name), "%s", name);

		return SMCResult_Continue;
	}

	g_pSM->LogMessage(myself, "Cannot nest within a member: line: %i col: %i", states->line, states->col);

	return SMCResult_HaltFail;
}

SourceMod::SMCResult Structs::ReadSMC_KeyValue( const SMCStates *states, const char *key, const char *value )
{
	if (!m_bInStruct)
	{
		//hrm..
		g_pSM->LogMessage(myself, "Unknown value %s: line: %i col: %i", key, states->line, states->col);
		return SMCResult_HaltFail;
	}

	if (m_bInMember)
	{
		if (strcmp(key, "type") == 0)
		{
			MemberType *pType = m_typeLookup.retrieve(value);

			if (pType == NULL)
			{
				//invalid type
				g_pSM->LogMessage(myself, "Invalid Type: line: %i col: %i", states->line, states->col);
				return SMCResult_HaltFail;
			}

			/* Note all 'int' types are assumed to be 32bit for the moment */
			m_currentMember->type = *pType;
		}
		else if (strcmp(key, "size") == 0)
		{
			m_currentMember->size = atoi(value);
		}
#if defined WIN32
		else if (strcmp(key, "windows") == 0)
#else
		else if (strcmp(key, "linux") == 0)
#endif
		{
			m_currentMember->offset = atoi(value);
		}
	}
	else
	{
		if (strcmp(key, "size") == 0)
		{
			m_currentStruct->size = atoi(value);
		}
	}

	return SMCResult_Continue;
}

SourceMod::SMCResult Structs::ReadSMC_LeavingSection( const SMCStates *states )
{
	if (m_bInMember)
	{
		/** Check for missing or invalid parameters */
		if (m_currentMember->type == Member_Unknown || m_currentMember->offset == -1)
		{
			g_pSM->LogMessage(myself, "Missing offset or type: line: %i col: %i", states->line, states->col);
			return SMCResult_HaltFail;
		}

		if (m_currentMember->size == -1)
		{
			switch (m_currentMember->type)
			{
				case Member_Float:
				case Member_FloatPointer:
				case Member_Vector:
				case Member_VectorPointer:
				case Member_EHandle:
				case Member_EHandlePointer:
				{
					break;
				}

				default:
				{
					g_pSM->LogMessage(myself, "Missing size: line: %i col: %i", states->line, states->col);
					return SMCResult_HaltFail;
				}
			}
		}

		/* Work out the int size */
		if (m_currentMember->type == Member_Int32)
		{
			if (m_currentMember->size == 1)
			{
				m_currentMember->type = Member_Int8;
			} 
			else if (m_currentMember->size == 2)
			{
				m_currentMember->type = Member_Int16;
			} 
			else if (m_currentMember->size == 4)
			{
				m_currentMember->type = Member_Int32;
			} 
			else
			{
				g_pSM->LogMessage(myself, "Invalid int size %i: line: %i col: %i", m_currentMember->size, states->line, states->col);
				return SMCResult_HaltFail;
			}

		} 
		else if (m_currentMember->type == Member_Int32)
		{
			if (m_currentMember->size == 1)
			{
				m_currentMember->type = Member_Int8Pointer;
			} 
			else if (m_currentMember->size == 2)
			{
				m_currentMember->type = Member_Int16Pointer;
			} 
			else if (m_currentMember->size == 4)
			{
				m_currentMember->type = Member_Int32Pointer;
			} 
			else
			{
				g_pSM->LogMessage(myself, "Invalid int size %i: line: %i col: %i", m_currentMember->size, states->line, states->col);
				return SMCResult_HaltFail;
			}
		}

		m_currentStruct->AddMember(m_currentMember->name, m_currentMember);

		m_bInMember = false;
		m_currentMember = NULL;

		return SMCResult_Continue;
	}

	g_StructManager.AddStruct(m_currentStruct->name, m_currentStruct);

	m_bInStruct = false;
	m_currentStruct = NULL;

	return SMCResult_Continue;
}
