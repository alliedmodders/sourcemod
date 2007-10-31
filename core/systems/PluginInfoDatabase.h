/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod
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

#ifndef _INCLUDE_SOURCEMOD_CORE_SYSTEM_PLUGININFODATABASE_H_
#define _INCLUDE_SOURCEMOD_CORE_SYSTEM_PLUGININFODATABASE_H_

/**
 * This file parses plugin_settings.cfg and stores the information in cached memory.
 * It provides simplistic abstraction to retrieving the info.
 * :TODO: currently untested
 */

#include "sm_memtable.h"
#include "ITextParsers.h"
#include "IPluginSys.h"
#include "sm_globals.h"

struct PluginOpts
{
	int key;
	int val;
};

struct PluginSettings
{
	void Init();
	int name;
	bool pause_val;
	PluginType type_val;
	int optarray;
	size_t opts_num;
	size_t opts_size;
	bool blockload_val;
};

class CPluginInfoDatabase : public ITextListener_SMC
{
public:
	CPluginInfoDatabase();
	~CPluginInfoDatabase();
public: //ITextListener_SMC
	void ReadSMC_ParseStart();
	SMCResult ReadSMC_NewSection(const SMCStates *states, const char *name);
	SMCResult ReadSMC_KeyValue(const SMCStates *states, const char *key, const char *value);
	SMCResult ReadSMC_LeavingSection(const SMCStates *states);
public:
	/**
	 * Returns the number of plugin settings available.
	 */
	unsigned int GetSettingsNum();

	/** 
	 * Given an index, returns the plugin settings block if the filename matches.
	 * Otherwise, returns NULL.
	 */
	PluginSettings *GetSettingsIfMatch(unsigned int index, const char *filename);

	/** 
	 *  Given a plugin settings struct and an index,
	 * returns the given JIT key/value option pair at that index.
	 * If the input is invalid, key and val will be set to NULL.
	 */
	void GetOptionsForPlugin(PluginSettings *settings, unsigned int opt_num, const char **key, const char **val);
private:
	SMCResult MakeError(const char *fmt, ...);
private:
	BaseStringTable *m_strtab;
	int m_errmsg;
	bool in_plugins;
	bool in_options;
	int m_infodb;
	size_t m_infodb_count;
	size_t m_infodb_size;
	int cur_plugin;
};


#endif //_INCLUDE_SOURCEMOD_CORE_SYSTEM_PLUGININFODATABASE_H_
