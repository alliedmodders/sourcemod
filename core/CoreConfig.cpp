/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * =============================================================================
 * SourceMod
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

#include <ITextParsers.h>
#include "CoreConfig.h"
#include "sourcemod.h"
#include "sourcemm_api.h"
#include "sm_stringutil.h"
#include "Logger.h"
#include "frame_hooks.h"
#include "logic_bridge.h"
#include "compat_wrappers.h"
#include <sourcemod_version.h>
#include <amtl/os/am-path.h>
#include <amtl/os/am-fsutil.h>
#include <sh_list.h>
#include <IForwardSys.h>
#include <bridge/include/IScriptManager.h>
#include <bridge/include/ILogger.h>

using namespace SourceHook;

#ifdef PLATFORM_WINDOWS
ConVar sm_corecfgfile("sm_corecfgfile", "addons\\sourcemod\\configs\\core.cfg", 0, "SourceMod core configuration file");
#elif defined PLATFORM_LINUX || defined PLATFORM_APPLE
ConVar sm_corecfgfile("sm_corecfgfile", "addons/sourcemod/configs/core.cfg", 0, "SourceMod core configuration file");
#endif

IForward *g_pOnServerCfg = NULL;
IForward *g_pOnConfigsExecuted = NULL;
IForward *g_pOnAutoConfigsBuffered = NULL;
CoreConfig g_CoreConfig;
bool g_bConfigsExecd = false;
bool g_bServerExecd = false;
bool g_bGotServerStart = false;
bool g_bGotTrigger = false;
ConCommand *g_pExecPtr = NULL;
ConVar *g_ServerCfgFile = NULL;

void CheckAndFinalizeConfigs();

#if SOURCE_ENGINE >= SE_ORANGEBOX
SH_DECL_EXTERN1_void(ConCommand, Dispatch, SH_NOATTRIB, false, const CCommand &);
void Hook_ExecDispatchPre(const CCommand &cmd)
#else
SH_DECL_EXTERN0_void(ConCommand, Dispatch, SH_NOATTRIB, false);
void Hook_ExecDispatchPre()
#endif
{
#if SOURCE_ENGINE <= SE_DARKMESSIAH
	CCommand cmd;
#endif

	const char *arg = cmd.Arg(1);

	if (!g_bServerExecd && arg != NULL && strcmp(arg, g_ServerCfgFile->GetString()) == 0)
	{
		g_bGotTrigger = true;
	}
}

#if SOURCE_ENGINE >= SE_ORANGEBOX
void Hook_ExecDispatchPost(const CCommand &cmd)
#else
void Hook_ExecDispatchPost()
#endif
{
	if (g_bGotTrigger)
	{
		g_bGotTrigger = false;
		g_bServerExecd = true;
		CheckAndFinalizeConfigs();
	}
}

void CheckAndFinalizeConfigs()
{
	if ((g_bServerExecd || g_ServerCfgFile == NULL) && g_bGotServerStart)
	{
        g_PendingInternalPush = true;
	}
}

void CoreConfig::OnSourceModAllInitialized()
{
	rootmenu->AddRootConsoleCommand3("config", "Set core configuration options", this);
	g_pOnServerCfg = forwardsys->CreateForward("OnServerCfg", ET_Ignore, 0, NULL);
	g_pOnConfigsExecuted = forwardsys->CreateForward("OnConfigsExecuted", ET_Ignore, 0, NULL);
	g_pOnAutoConfigsBuffered = forwardsys->CreateForward("OnAutoConfigsBuffered", ET_Ignore, 0, NULL);
}

CoreConfig::CoreConfig()
{
}

CoreConfig::~CoreConfig()
{
}

void CoreConfig::OnSourceModShutdown()
{
	rootmenu->RemoveRootConsoleCommand("config", this);
	forwardsys->ReleaseForward(g_pOnServerCfg);
	forwardsys->ReleaseForward(g_pOnConfigsExecuted);
	forwardsys->ReleaseForward(g_pOnAutoConfigsBuffered);

	if (g_pExecPtr != NULL)
	{
		SH_REMOVE_HOOK(ConCommand, Dispatch, g_pExecPtr, SH_STATIC(Hook_ExecDispatchPre), false);
		SH_REMOVE_HOOK(ConCommand, Dispatch, g_pExecPtr, SH_STATIC(Hook_ExecDispatchPost), true);
		g_pExecPtr = NULL;
	}
}

void CoreConfig::OnSourceModLevelChange(const char *mapName)
{
	static bool already_checked = false;

	if (!already_checked)
	{
		if (engine->IsDedicatedServer())
		{
			g_ServerCfgFile = icvar->FindVar("servercfgfile");
		}
		else
		{
			g_ServerCfgFile = icvar->FindVar("lservercfgfile");
		}

		if (g_ServerCfgFile != NULL)
		{
			g_pExecPtr = FindCommand("exec");
			if (g_pExecPtr != NULL)
			{
				SH_ADD_HOOK(ConCommand, Dispatch, g_pExecPtr, SH_STATIC(Hook_ExecDispatchPre), false);
				SH_ADD_HOOK(ConCommand, Dispatch, g_pExecPtr, SH_STATIC(Hook_ExecDispatchPost), true);
			}
			else
			{
				g_ServerCfgFile = NULL;
			}
		}
		already_checked = true;
	}

	g_bConfigsExecd = false;
	g_bServerExecd = false;
	g_bGotServerStart = false;
	g_bGotTrigger = false;
}

void CoreConfig::OnRootConsoleCommand(const char *cmdname, const ICommandArgs *command)
{
	int argcount = command->ArgC();
	if (argcount >= 4)
	{
		const char *option = command->Arg(2);
		const char *value = command->Arg(3);

		char error[255];
		ConfigResult res = SetConfigOption(option, value, ConfigSource_Console, error, sizeof(error));

		if (res == ConfigResult_Reject)
		{
			UTIL_ConsolePrint("[SM] Could not set config option \"%s\" to \"%s\". (%s)", option, value, error);
		} else {
			if (res == ConfigResult_Ignore) {
				UTIL_ConsolePrint("[SM] WARNING: Config option \"%s\" is not registered.", option);
			}

			UTIL_ConsolePrint("[SM] Config option \"%s\" set to \"%s\".", option, value);
		}

		return;
	} else if (argcount >= 3) {
		const char *option = command->Arg(2);
		
		const char *value = GetCoreConfigValue(option);
		
		if (value == NULL)
		{
			UTIL_ConsolePrint("[SM] No such config option \"%s\" exists.", option);
		} else {
			UTIL_ConsolePrint("[SM] Config option \"%s\" is set to \"%s\".", option, value);
		}
		
		return;
	}

	UTIL_ConsolePrint("[SM] Usage: sm config <option> [value]");
}

void CoreConfig::Initialize()
{
	SMCError err;
	char filePath[PLATFORM_MAX_PATH];

	/* Try to get command line value of core config convar */
	const char *corecfg = icvar->GetCommandLineValue("sm_corecfgfile");

	/* If sm_corecfgfile is on the command line, use that
	 * If sm_corecfgfile isn't there, check sm_basepath on the command line and build the path off that
	 * If sm_basepath isn't there, just use the default path for the cfg
	 */
	if (corecfg)
	{
		ke::path::Format(filePath, sizeof(filePath), "%s/%s", g_SourceMod.GetGamePath(), corecfg);
	}
	else
	{
		const char *basepath = icvar->GetCommandLineValue("sm_basepath");

		/* Format path to config file */
		if (basepath)
		{
			ke::path::Format(filePath, sizeof(filePath), "%s/%s/%s", g_SourceMod.GetGamePath(), basepath, "configs/core.cfg");
		}
		else
		{
			ke::path::Format(filePath, sizeof(filePath), "%s/%s", g_SourceMod.GetGamePath(), sm_corecfgfile.GetDefault());
		}
	}

	/* Reset cached key values */
	m_KeyValues.clear();

	/* Parse config file */
	if ((err=textparsers->ParseFile_SMC(filePath, this, NULL)) != SMCError_Okay)
	{
 		/* :TODO: This won't actually log or print anything :( - So fix that somehow */
		const char *error = textparsers->GetSMCErrorString(err);
		logger->LogFatal("[SM] Error encountered parsing core config file: %s", error ? error : "");
	}
}

SMCResult CoreConfig::ReadSMC_KeyValue(const SMCStates *states, const char *key, const char *value)
{
	char error[255];
	ConfigResult err = SetConfigOption(key, value, ConfigSource_File, error, sizeof(error));

	if (err == ConfigResult_Reject)
	{
		/* This is a fatal error */
		logger->LogFatal("Config error (key: %s) (value: %s) %s", key, value, error);
	}

	return SMCResult_Continue;
}

ConfigResult CoreConfig::SetConfigOption(const char *option, const char *value, ConfigSource source, char *error, size_t maxlength)
{
	ConfigResult result = ConfigResult_Ignore;

	/* Notify! */
	SMGlobalClass *pBase = SMGlobalClass::head;
	while (pBase)
	{
		if ((result = pBase->OnSourceModConfigChanged(option, value, source, error, maxlength)) != ConfigResult_Ignore)
		{
			break;
		}
		pBase = pBase->m_pGlobalClassNext;
	}

	std::string vstr(value);
	m_KeyValues.replace(option, std::move(vstr));

	return result;
}

const char *CoreConfig::GetCoreConfigValue(const char *key)
{
	StringHashMap<std::string>::Result r = m_KeyValues.find(key);
	if (!r.found())
		return NULL;
	return r->value.c_str();
}

bool SM_AreConfigsExecuted()
{
	return g_bConfigsExecd;
}

inline bool IsPathSepChar(char c)
{
#if defined PLATFORM_WINDOWS
	return (c == '\\' || c == '/');
#elif defined PLATFORM_LINUX || defined PLATFORM_POSIX
	return (c == '/');
#endif
}

void SM_ParseConfig(FILE *fp, ke::HashMap<std::string, std::string, StringPolicy> &cvars_list)
{
	char line[4096];
	char cvar_name[256];

	while (fgets(line, sizeof(line), fp))
	{
		char *ptr = line;

		// Ignore whitespaces and tabs
		while (*ptr == ' ' || *ptr == '\t')
		{
			ptr++;
		}

		// Ignore empty lines if found
		if (*ptr == '\0' || *ptr == '\n' || *ptr == '\r')
		{
			continue;
		}

		// Ignore comments
		if (*ptr == '/' && *(ptr + 1) == '/')
		{
			continue;
		}

		size_t i = 0;
		while (*ptr != '\0' && *ptr != '\n' && *ptr != '\r' && *ptr != ' ' && *ptr != '\t' && i < (sizeof(cvar_name) - 1))
		{
			cvar_name[i++] = *ptr++;
		}

		cvar_name[i] = '\0';

		if (!i)
		{
			continue;
		}

		auto p = cvars_list.findForAdd(cvar_name);
		if (p.found())
		{
			continue;
		}

		// Remove spaces and tabs before the value
		while (*ptr == ' ' || *ptr == '\t')
		{
        	ptr++;
		}

		char *value_start = ptr;
		char *value_end = value_start + strlen(value_start);

		while (value_end > value_start &&
			(*(value_end - 1) == '\n' ||
			*(value_end - 1) == '\r' ||
			*(value_end - 1) == ' '  ||
			*(value_end - 1) == '\t' ||
			*(value_end - 1) == '"'))
		{
			value_end--;
		}

		*value_end = '\0';

		if (*value_start == '"')
		{
			value_start++;
		}

		cvars_list.add(p, cvar_name, value_start);
	}
}

inline bool SM_CvarExistsInConfig(ke::HashMap<std::string, std::string, StringPolicy> &cvars, const char *cvar_name)
{
	return cvars.find(cvar_name).found();
}

bool SM_CvarExistsInPlugin(List<const ConVar *> &convars, const char *cvar_name)
{
	for (List<const ConVar *>::iterator iter = convars.begin(); iter != convars.end(); iter++)
	{
		const ConVar *cvar = (*iter);
#if SOURCE_ENGINE >= SE_ORANGEBOX
		if (cvar->IsFlagSet(FCVAR_DONTRECORD))
#else
		if (cvar->IsBitSet(FCVAR_DONTRECORD))
#endif
		{
			continue;
		}

		if (strcmp(cvar_name, cvar->GetName()) == 0)
		{
			return true;
		}
	}

	return false;
}

bool SM_ConfigCheckRegeneration(
	const char *file,
	ke::HashMap<std::string, std::string, StringPolicy> &existing_cvars,
	List<const ConVar *> &convars)
{
	FILE *fp = fopen(file, "rt");
	if (!fp)
	{
		logger->LogError("Failed to read config for checking regeneration, make sure the directory has read permission.");
		return true;
	}

	SM_ParseConfig(fp, existing_cvars);
	fclose(fp);

	/* #1. Check if the plugin's cvar list has a cvar that doesn't exist in the config file
	 * If not, then let sourcemod remove the existing config file and generate a new one
	 */
	for (List<const ConVar *>::iterator iter = convars.begin(); iter != convars.end(); iter++)
	{
		const ConVar *cvar = (*iter);
#if SOURCE_ENGINE >= SE_ORANGEBOX
		if (cvar->IsFlagSet(FCVAR_DONTRECORD))
#else
		if (cvar->IsBitSet(FCVAR_DONTRECORD))
#endif
		{
			continue;
		}

		if (!SM_CvarExistsInConfig(existing_cvars, cvar->GetName()))
		{
			return true;
		}
	}

	/* #2. Check if the config file has a cvar that doesn't exist in the plugin's cvar list
	 * If not, then let sourcemod remove the existing config file and generate a new one
	 */
	for (auto it = existing_cvars.iter(); !it.empty(); it.next())
	{
		if (!SM_CvarExistsInPlugin(convars, it->key.c_str()))
		{
			return true;
		}
	}

	return false;
}

inline void SM_ExecuteConfigFile(const char *file)
{
	char cmd[255];
	ke::SafeSprintf(cmd, sizeof(cmd), "exec %s\n", file);
	engine->ServerCommand(cmd);
}

bool SM_GenerateConfigFile(
	IPlugin *pl,
	const char *file,
	List<const ConVar *> &convars,
	bool needs_regeneration = false,
	ke::HashMap<std::string, std::string, StringPolicy> *existing_cvars = nullptr)
{
	FILE *fp = NULL;
	char tmp_file[PLATFORM_MAX_PATH];
	if (!needs_regeneration)
	{
		fp = fopen(file, "wt");
		if (!fp)
		{
			logger->LogError("Failed to auto generate config for %s, make sure the directory has write permission.", pl->GetFilename());
			return false;
		}
	}
	else
	{
		/* Create a .tmp file with the new contents of the new config file and then override it to the existing file */
		ke::path::Format(tmp_file, sizeof(tmp_file), "%s.tmp", file);
		fp = fopen(tmp_file, "wt");
		if (!fp)
		{
			logger->LogError("Failed to create temp config file for %s, make sure the directory has write permission.", pl->GetFilename());
			return false;
		}
	}

	fprintf(fp, "// This file was auto-generated by SourceMod (v%s)\n", SOURCEMOD_VERSION);
	fprintf(fp, "// ConVars for plugin \"%s\"\n", pl->GetFilename());
	fprintf(fp, "\n\n");

	float x;
	for (List<const ConVar *>::iterator iter = convars.begin(); iter != convars.end(); iter++)
	{
		const ConVar *cvar = (*iter);
#if SOURCE_ENGINE >= SE_ORANGEBOX
		if (cvar->IsFlagSet(FCVAR_DONTRECORD))
#else
		if (cvar->IsBitSet(FCVAR_DONTRECORD))
#endif
		{
			continue;
		}

		char descr[255];
		char *dptr = descr;

		/* Print comments until there is no more */
		ke::SafeStrcpy(descr, sizeof(descr), cvar->GetHelpText());
		while (*dptr != '\0')
		{
			/* Find the next line */
			char *next_ptr = dptr;
			while (*next_ptr != '\0')
			{
				if (*next_ptr == '\n')
				{
					*next_ptr = '\0';
					next_ptr++;
					break;
				}
				next_ptr++;
			}
			fprintf(fp, "// %s\n", dptr);
			dptr = next_ptr;
		}

		fprintf(fp, "// -\n");
		fprintf(fp, "// Default: \"%s\"\n", cvar->GetDefault());
		if (cvar->GetMin(x))
		{
			fprintf(fp, "// Minimum: \"%02f\"\n", x);
		}
		if (cvar->GetMax(x))
		{
			fprintf(fp, "// Maximum: \"%02f\"\n", x);
		}

		const char *value = cvar->GetDefault();

		if (needs_regeneration && existing_cvars)
		{
			auto p = existing_cvars->find(cvar->GetName());
			if (p.found())
			{
				value = p->value.c_str();
			}
		}

		fprintf(fp, "%s \"%s\"\n", cvar->GetName(), value);
		fprintf(fp, "\n");
	}
	fprintf(fp, "\n");

	fclose(fp);

	if (needs_regeneration)
	{
#ifdef PLATFORM_WINDOWS
		if (!MoveFileEx(tmp_file, file, MOVEFILE_REPLACE_EXISTING))
#else
		if (rename(tmp_file, file) != 0)
#endif
		{
			/* Replacing the temp config file with the existing one failed, remove it and notify the user about it */
			logger->LogError("Failed to replace temp config file for %s, make sure the directory has write permission.", pl->GetFilename());
			if (remove(tmp_file) != 0)
			{
				logger->LogError("Failed to remove temp config file for %s.", pl->GetFilename());
			}

			return false;
		}
	}

	return true;
}

bool SM_ExecuteConfig(IPlugin *pl, AutoConfig *cfg, bool can_create)
{
	bool will_create = false;
	
	/* See if we should be creating */
	if (can_create && cfg->create)
	{
		will_create = true;
		
		/* If the folder does not exist, attempt to create it.  
		 * We're awfully nice.
		 */
		const char *folder = cfg->folder.c_str();
		char path[PLATFORM_MAX_PATH];
		char build[PLATFORM_MAX_PATH];

		g_SourceMod.BuildPath(Path_Game, path, sizeof(path), "cfg/%s", folder);

		if (!ke::file::IsDirectory(path))
		{
			char *cur_ptr = path;
			size_t len;
			
			ke::path::Format(path, sizeof(path), "%s", folder);
			len = g_SourceMod.BuildPath(Path_Game, build, sizeof(build), "cfg");

			do
			{
				/* Find next suitable path */
				char *next_ptr = cur_ptr;
				while (*next_ptr != '\0')
				{
					if (IsPathSepChar(*next_ptr))
					{
						*next_ptr = '\0';
						next_ptr++;
						break;
					}
					next_ptr++;
				}
				if (*next_ptr == '\0')
				{
					next_ptr = NULL;
				}
				len += ke::path::Format(&build[len],
					sizeof(build)-len,
					"/%s",
					cur_ptr);
				if (!ke::file::CreateDirectory(build))
					break;
				cur_ptr = next_ptr;
			} while (cur_ptr);
		}
	}

	/* Check if the file exists. */
	char file[PLATFORM_MAX_PATH];
	char local[PLATFORM_MAX_PATH];

	if (cfg->folder.size())
	{
		ke::path::Format(local,
			sizeof(local),
			"%s/%s.cfg",
			cfg->folder.c_str(),
			cfg->autocfg.c_str());
	} else {
		ke::path::Format(local,
			sizeof(local),
			"%s.cfg",
			cfg->autocfg.c_str());
	}
	
	g_SourceMod.BuildPath(Path_Game, file, sizeof(file), "cfg/%s", local);

	bool file_exists = ke::file::IsFile(file);

	if (!will_create)
	{
		if (file_exists)
		{
			SM_ExecuteConfigFile(local);
		}

		return can_create;
	}

	List<const ConVar *> *convars = NULL;
	if (!pl->GetProperty("ConVarList", (void **)&convars, false) || !convars)
	{
		if (file_exists)
		{
			SM_ExecuteConfigFile(local);
		}

		return can_create;
	}

	if (!file_exists)
	{
		/* If file doesn't exist, then automatically generate the config file and execute it. */
		/* If sourcemod somehow failed to generate the config file, then stop. */
		if (!SM_GenerateConfigFile(pl, file, *convars))
		{
			return can_create;
		}

		SM_ExecuteConfigFile(local);
		can_create = false;
		return can_create;
	}

	/* 
	 * Now if the file exists, read the content of it to see if we need to regenerate it or not
	 * We need to remove the file before regenerating it.
	 */
	ke::HashMap<std::string, std::string, StringPolicy> existing_cvars;
	existing_cvars.init();
	if (!SM_ConfigCheckRegeneration(file, existing_cvars, *convars))
	{
		SM_ExecuteConfigFile(local);
		return can_create;
	}

	/* File exists and misses cvars between the config file and the plugin's convars list
	 * Create a temporary config file with the new contents and then override it to the existing file.
	 * If sourcemod somehow failed to override the new contents to the existing config file, then execute the existing config file.
	 */
	can_create = false;
	if (!SM_GenerateConfigFile(pl, file, *convars, true, &existing_cvars))
	{
		logger->LogError("An error occurred while regenerating config file for %s, executing the existing file instead.", pl->GetFilename());
	}

	SM_ExecuteConfigFile(local);
	return can_create;
}

void SM_DoSingleExecFwds(IPluginContext *ctx)
{
	IPluginFunction *pf;

	if ((pf = ctx->GetFunctionByName("OnServerCfg")) != NULL)
	{
		pf->Execute(NULL);
	}

	if ((pf = ctx->GetFunctionByName("OnConfigsExecuted")) != NULL)
	{
		pf->Execute(NULL);
	}
}

void SM_ConfigsExecuted_Plugin(unsigned int serial)
{
	IPluginIterator *iter = scripts->GetPluginIterator();
	while (iter->MorePlugins())
	{
		IPlugin *plugin = iter->GetPlugin();
		if (plugin->GetSerial() == serial)
		{
			SM_DoSingleExecFwds(plugin->GetBaseContext());
			break;
		}
		iter->NextPlugin();
	}
	iter->Release();
}

void SM_ExecuteForPlugin(IPluginContext *ctx)
{
	SMPlugin *plugin = scripts->FindPluginByContext(ctx->GetContext());

	unsigned int num = plugin->GetConfigCount();
	if (!num)
	{
		SM_DoSingleExecFwds(ctx);
	}
	else
	{
		bool can_create = true;
		for (unsigned int i=0; i<num; i++)
		{
			can_create = SM_ExecuteConfig(plugin, plugin->GetConfig(i), can_create);
		}
		char cmd[255];
		ke::SafeSprintf(cmd, sizeof(cmd), "sm_internal 2 %d\n", plugin->GetSerial());
		engine->ServerCommand(cmd);
	}
}

void SM_ExecuteAllConfigs()
{
	if (g_bGotServerStart)
	{
		return;
	}

	engine->ServerCommand("exec sourcemod/sourcemod.cfg\n");

	AutoPluginList plugins(scripts);
	for (size_t i = 0; i < plugins->size(); i++)
	{
		SMPlugin *plugin = plugins->at(i);
		unsigned int num = plugin->GetConfigCount();
		bool can_create = true;
		for (unsigned int i=0; i<num; i++)
		{
			can_create = SM_ExecuteConfig(plugin, plugin->GetConfig(i), can_create);
		}
	}

	g_bGotServerStart = true;
	CheckAndFinalizeConfigs();
}

void SM_ConfigsExecuted_Global()
{
	if (g_bConfigsExecd)
	{
		return;
	}

	g_bConfigsExecd = true;

	g_pOnServerCfg->Execute(NULL);
	g_pOnConfigsExecuted->Execute(NULL);
}

void SM_InternalCmdTrigger()
{
	/* Order is important here.  We need to buffer things before we send the command out. */
	g_pOnAutoConfigsBuffered->Execute(NULL);
	engine->ServerCommand("sm_internal 1\n");
    g_PendingInternalPush = false;
}

CON_COMMAND(sm_internal, "")
{
#if SOURCE_ENGINE <= SE_DARKMESSIAH
	CCommand args;
#endif

	if (args.ArgC() < 1)
		return;

	const char *arg = args.Arg(1);
	if (strcmp(arg, "1") == 0) {
		SM_ConfigsExecuted_Global();
	} else if (strcmp(arg, "2") == 0) {
		if (args.ArgC() >= 3)
			SM_ConfigsExecuted_Plugin(atoi(args.Arg(2)));
	}
}
