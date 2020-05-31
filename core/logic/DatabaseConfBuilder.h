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
 
#ifndef _INCLUDE_DATABASE_CONF_BUILDER_H_
#define _INCLUDE_DATABASE_CONF_BUILDER_H_

#include <IDBDriver.h>
#include <ITextParsers.h>
#include "common_logic.h"

#include <am-vector.h>
#include <am-string.h>
#include <am-refcounting-threadsafe.h>

class ConfDbInfo : public ke::RefcountedThreadsafe<ConfDbInfo>
{
public:
	ConfDbInfo() : realDriver(NULL)
	{
	}
	std::string name;
	std::string driver;
	std::string host;
	std::string user;
	std::string pass;
	std::string database;
	IDBDriver *realDriver;
	DatabaseInfo info;
};

class ConfDbInfoList : public std::vector<ConfDbInfo *>
{
	/* Allow internal usage of ConfDbInfoList */
	friend class DBManager;
	friend class DatabaseConfBuilder;
private:
	std::string& GetDefaultDriver() {
		return m_DefDriver;
	}
	
	ConfDbInfo *GetDatabaseConf(const char *name) {
		for (size_t i = 0; i < this->size(); i++)
		{
			ConfDbInfo *current = this->at(i);
			/* If we run into the default configuration, then we'll save it
			 * for the next call to GetDefaultConfiguration */ 
			if (strcmp(current->name.c_str(), "default") == 0)
			{
				m_DefaultConfig = current;
			}
			if (strcmp(current->name.c_str(), name) == 0)
			{
				return current;
			}
		}
		return nullptr;
	}
	ConfDbInfo *GetDefaultConfiguration() {
		return m_DefaultConfig;
	}
	void SetDefaultDriver(const char *input) {
		m_DefDriver = std::string(input);
	}
	void ReleaseMembers() {
		for (size_t i = 0; i < this->size(); i++) {
			ConfDbInfo *current = this->at(i);
			current->Release();
		}
	}
private:
	ConfDbInfo *m_DefaultConfig;
	std::string m_DefDriver;
};


class DatabaseConfBuilder : public ITextListener_SMC
{
public:
	DatabaseConfBuilder();
	~DatabaseConfBuilder();
	void StartParse();
	void SetPath(char* path);
	ConfDbInfoList *GetConfigList();
public: //ITextListener_SMC
	void ReadSMC_ParseStart();
	SMCResult ReadSMC_NewSection(const SMCStates *states, const char *name);
	SMCResult ReadSMC_KeyValue(const SMCStates *states, const char *key, const char *value);
	SMCResult ReadSMC_LeavingSection(const SMCStates *states);
	void ReadSMC_ParseEnd(bool halted, bool failed);

private:
	unsigned int m_ParseLevel;
	unsigned int m_ParseState;
	ConfDbInfo *m_ParseCurrent;
	ConfDbInfoList *m_ParseList;
private:
	std::string m_Filename;
	ConfDbInfoList *m_InfoList;
};

#endif //_INCLUDE_DATABASE_CONF_BUILDER_H_
