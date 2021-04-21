/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2018 AlliedModders LLC.  All rights reserved.
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
#include "DatabaseConfBuilder.h"
#include <bridge/include/ILogger.h>

#define DBPARSE_LEVEL_NONE		0
#define DBPARSE_LEVEL_MAIN		1
#define DBPARSE_LEVEL_DATABASE	2

DatabaseConfBuilder::DatabaseConfBuilder()
	: m_ParseList(nullptr),
	  m_InfoList(new ConfDbInfoList())
{
}

void DatabaseConfBuilder::SetPath(char *str)
{
	m_Filename = str;
}

DatabaseConfBuilder::~DatabaseConfBuilder()
{
}

ConfDbInfoList *DatabaseConfBuilder::GetConfigList()
{
	return m_InfoList;
}

void DatabaseConfBuilder::StartParse()
{
	SMCError err;
	SMCStates states = {0, 0};
	if ((err = textparsers->ParseFile_SMC(m_Filename.c_str(), this, &states)) != SMCError_Okay)
	{
		logger->LogError("[SM] Detected parse error(s) in file \"%s\"", m_Filename.c_str());
		if (err != SMCError_Custom)
		{
			const char *txt = textparsers->GetSMCErrorString(err);
			logger->LogError("[SM] Line %d: %s", states.line, txt);
		}
	}
}

void DatabaseConfBuilder::ReadSMC_ParseStart()
{
	m_ParseLevel = 0;
	m_ParseState = DBPARSE_LEVEL_NONE;
	
	m_ParseList = new ConfDbInfoList();
}
 
SMCResult DatabaseConfBuilder::ReadSMC_NewSection(const SMCStates *states, const char *name)
{
	if (m_ParseLevel)
	{
		m_ParseLevel++;
		return SMCResult_Continue;
	}

	if (m_ParseState == DBPARSE_LEVEL_NONE)
	{
		if (strcmp(name, "Databases") == 0)
		{
			m_ParseState = DBPARSE_LEVEL_MAIN;
		} else {
			m_ParseLevel++;
		}
	} else if (m_ParseState == DBPARSE_LEVEL_MAIN) {
		m_ParseCurrent = new ConfDbInfo();
		m_ParseCurrent->name = name;
		m_ParseState = DBPARSE_LEVEL_DATABASE;
	} else if (m_ParseState == DBPARSE_LEVEL_DATABASE) {
		m_ParseLevel++;
	}

	return SMCResult_Continue;
}

SMCResult DatabaseConfBuilder::ReadSMC_KeyValue(const SMCStates *states, const char *key, const char *value)
{
	if (m_ParseLevel)
	{
		return SMCResult_Continue;
	}

	if (m_ParseState == DBPARSE_LEVEL_MAIN)
	{
		if (strcmp(key, "driver_default") == 0)
		{
			m_ParseList->SetDefaultDriver(value);
		}
	} else if (m_ParseState == DBPARSE_LEVEL_DATABASE) {
		if (strcmp(key, "driver") == 0)
		{
			if (strcmp(value, "default") != 0)
			{
				m_ParseCurrent->driver = value;
			}
		} else if (strcmp(key, "database") == 0) {
			m_ParseCurrent->database = value;
		} else if (strcmp(key, "host") == 0) {
			m_ParseCurrent->host = value;
		} else if (strcmp(key, "user") == 0) {
			m_ParseCurrent->user = value;
		} else if (strcmp(key, "pass") == 0) {
			m_ParseCurrent->pass = value;
		} else if (strcmp(key, "timeout") == 0) {
			m_ParseCurrent->info.maxTimeout = atoi(value);
		} else if (strcmp(key, "port") == 0) {
			m_ParseCurrent->info.port = atoi(value);
		}
	}

	return SMCResult_Continue;
}

SMCResult DatabaseConfBuilder::ReadSMC_LeavingSection(const SMCStates *states)
{
	if (m_ParseLevel)
	{
		m_ParseLevel--;
		return SMCResult_Continue;
	}

	if (m_ParseState == DBPARSE_LEVEL_DATABASE)
	{
		m_ParseCurrent->info.driver = m_ParseCurrent->driver.c_str();
		m_ParseCurrent->info.database = m_ParseCurrent->database.c_str();
		m_ParseCurrent->info.host = m_ParseCurrent->host.c_str();
		m_ParseCurrent->info.user = m_ParseCurrent->user.c_str();
		m_ParseCurrent->info.pass = m_ParseCurrent->pass.c_str();
		
		/* Save it.. */
		m_ParseCurrent->AddRef();
		m_ParseList->push_back(m_ParseCurrent);
		m_ParseCurrent = nullptr;
		
		/* Go up one level */
		m_ParseState = DBPARSE_LEVEL_MAIN;
	} else if (m_ParseState == DBPARSE_LEVEL_MAIN) {
		m_ParseState = DBPARSE_LEVEL_NONE;
		return SMCResult_Halt;
	}

	return SMCResult_Continue;
}

void DatabaseConfBuilder::ReadSMC_ParseEnd(bool halted, bool failed)
{
	m_InfoList->ReleaseMembers();
	delete m_InfoList;
	m_InfoList = m_ParseList;
	
	m_ParseList = nullptr;
}
