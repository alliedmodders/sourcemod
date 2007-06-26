/**
 * vim: set ts=4 :
 * ===============================================================
 * SourceMod (C)2004-2007 AlliedModders LLC.  All rights reserved.
 * ===============================================================
 *
 * This file is not open source and may not be copied without explicit
 * written permission of AlliedModders LLC.  This file may not be redistributed 
 * in whole or significant part.
 * For information, see LICENSE.txt or http://www.sourcemod.net/license.php
 *
 * Version: $Id$
 */

#include "CoreConfig.h"
#include "sourcemod.h"
#include "sourcemm_api.h"
#include "sm_srvcmds.h"
#include "sm_version.h"
#include "sm_stringutil.h"
#include "LibrarySys.h"
#include "TextParsers.h"
#include "Logger.h"
#include "PluginSys.h"
#include "ForwardSys.h"

#ifdef PLATFORM_WINDOWS
ConVar sm_corecfgfile("sm_corecfgfile", "addons\\sourcemod\\configs\\core.cfg", 0, "SourceMod core configuration file");
#elif defined PLATFORM_LINUX
ConVar sm_corecfgfile("sm_corecfgfile", "addons/sourcemod/configs/core.cfg", 0, "SourceMod core configuration file");
#endif

IForward *g_pOnServerCfg = NULL;
IForward *g_pOnConfigsExecuted = NULL;
CoreConfig g_CoreConfig;
bool g_bConfigsExecd = false;

void CoreConfig::OnSourceModAllInitialized()
{
	g_RootMenu.AddRootConsoleCommand("config", "Set core configuration options", this);
	g_pOnServerCfg = g_Forwards.CreateForward("OnServerCfg", ET_Ignore, 0, NULL);
	g_pOnConfigsExecuted = g_Forwards.CreateForward("OnConfigsExecuted", ET_Ignore, 0, NULL);
}

void CoreConfig::OnSourceModShutdown()
{
	g_RootMenu.RemoveRootConsoleCommand("config", this);
	g_Forwards.ReleaseForward(g_pOnServerCfg);
	g_Forwards.ReleaseForward(g_pOnConfigsExecuted);
}

void CoreConfig::OnSourceModLevelChange(const char *mapName)
{
	g_bConfigsExecd = false;
}

void CoreConfig::OnRootConsoleCommand(const char *command, unsigned int argcount)
{
	if (argcount >= 4)
	{
		const char *option = engine->Cmd_Argv(2);
		const char *value = engine->Cmd_Argv(3);

		char error[255];

		ConfigResult res = SetConfigOption(option, value, ConfigSource_Console, error, sizeof(error));

		if (res == ConfigResult_Reject)
		{
			g_RootMenu.ConsolePrint("[SM] Could not set config option \"%s\" to \"%s\" (%s)", option, value, error);
		} else if (res == ConfigResult_Ignore) {
			g_RootMenu.ConsolePrint("[SM] No such config option \"%s\" exists.", option);
		} else {
			g_RootMenu.ConsolePrint("Config option \"%s\" successfully set to \"%s.\"", option, value);
		}

		return;
	}

	g_RootMenu.ConsolePrint("[SM] Usage: sm config <option> <value>");
}

void CoreConfig::Initialize()
{
	SMCParseError err;
	char filePath[PLATFORM_MAX_PATH];

	/* Try to get command line value of core config convar */
	const char *corecfg = icvar->GetCommandLineValue("sm_corecfgfile");

	/* If sm_corecfgfile not specified on command line, use default value */
	if (!corecfg)
	{
		corecfg = sm_corecfgfile.GetDefault();
	}

	/* Format path to config file */
	g_LibSys.PathFormat(filePath, sizeof(filePath), "%s/%s", g_SourceMod.GetGamePath(), corecfg);

	/* Parse config file */
	if ((err=g_TextParser.ParseFile_SMC(filePath, this, NULL, NULL)) != SMCParse_Okay)
	{
 		/* :TODO: This won't actually log or print anything :( - So fix that somehow */
		const char *error = g_TextParser.GetSMCErrorString(err);
		g_Logger.LogFatal("[SM] Error encountered parsing core config file: %s", error ? error : "");
	}
}

SMCParseResult CoreConfig::ReadSMC_KeyValue(const char *key, const char *value, bool key_quotes, bool value_quotes)
{
	char error[255];
	ConfigResult err = SetConfigOption(key, value, ConfigSource_File, error, sizeof(error));

	if (err == ConfigResult_Reject)
	{
		/* This is a fatal error */
		g_Logger.LogFatal("Config error (key: %s) (value: %s) %s", key, value, error);
	}

	return SMCParse_Continue;
}

ConfigResult CoreConfig::SetConfigOption(const char *option, const char *value, ConfigSource source, char *error, size_t maxlength)
{
	ConfigResult result;

	/* Notify! */
	SMGlobalClass *pBase = SMGlobalClass::head;
	while (pBase)
	{
		if ((result = pBase->OnSourceModConfigChanged(option, value, source, error, maxlength)) != ConfigResult_Ignore)
		{
			return result;
		}
		pBase = pBase->m_pGlobalClassNext;
	}

	return ConfigResult_Ignore;
}

bool SM_AreConfigsExecuted()
{
	return g_bConfigsExecd;
}

inline bool IsPathSepChar(char c)
{
#if defined PLATFORM_WINDOWS
	return (c == '\\' || c == '/');
#elif defined PLATFORM_LINUX
	return (c == '/');
#endif
}

bool SM_ExecuteConfig(CPlugin *pl, AutoConfig *cfg, bool can_create)
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

		if (!g_LibSys.IsPathDirectory(path))
		{
			char *cur_ptr = path;
			size_t len;
			
			g_LibSys.PathFormat(path, sizeof(path), "%s", folder);
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
				len += g_LibSys.PathFormat(&build[len], 
					sizeof(build)-len,
					"/%s",
					cur_ptr);
				if (!g_LibSys.CreateDirectory(build))
				{
					break;
				}
				cur_ptr = next_ptr;
			} while (cur_ptr);
		}
	}

	/* Check if the file exists. */
	char file[PLATFORM_MAX_PATH];
	char local[PLATFORM_MAX_PATH];

	if (cfg->folder.size())
	{
		g_LibSys.PathFormat(local, 
			sizeof(local), 
			"%s/%s.cfg",
			cfg->folder.c_str(),
			cfg->autocfg.c_str());
	} else {
		g_LibSys.PathFormat(local,
			sizeof(local),
			"%s.cfg",
			cfg->autocfg.c_str());
	}
	
	g_SourceMod.BuildPath(Path_Game, file, sizeof(file), "cfg/%s", local);

	bool file_exists = g_LibSys.IsPathFile(file);
	if (!file_exists && will_create)
	{
		List<const ConVar *> *convars = NULL;
		if (pl->GetProperty("ConVarList", (void **)&convars, false) && convars)
		{
			/* Attempt to create it */
			FILE *fp = fopen(file, "wt");
			if (fp)
			{
				fprintf(fp, "// This file was auto-generated by SourceMod (v%s)\n", SVN_FULL_VERSION);
				fprintf(fp, "// ConVars for plugin \"%s\"\n", pl->GetFilename());
				fprintf(fp, "\n\n");

				List<const ConVar *>::iterator iter;
				float x;
				for (iter = convars->begin(); iter != convars->end(); iter++)
				{
					//:TODO: GetFlags should probably be const so we don't have to do this!
					ConVar *cvar = (ConVar *)(*iter);

					if ((cvar->GetFlags() & FCVAR_DONTRECORD) == FCVAR_DONTRECORD)
					{
						continue;
					}

					char descr[255];
					char *dptr = descr;

					/* Print comments until there is no more */
					strncopy(descr, cvar->GetHelpText(), sizeof(descr));
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
					fprintf(fp, "%s \"%s\"\n", cvar->GetName(), cvar->GetDefault());
					fprintf(fp, "\n");
				}
				fprintf(fp, "\n");

				file_exists = true;
				can_create = false;
				fclose(fp);
			}
		}
	}

	if (file_exists)
	{
		char cmd[255];
		UTIL_Format(cmd, sizeof(cmd), "exec %s\n", local);
		engine->ServerCommand(cmd);
	}

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
	IPluginIterator *iter = g_PluginSys.GetPluginIterator();
	while (iter->MorePlugins())
	{
		CPlugin *plugin = (CPlugin *)(iter->GetPlugin());
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
	CPlugin *plugin = (CPlugin *)g_PluginSys.GetPluginByCtx(ctx->GetContext());

	unsigned int num = plugin->GetConfigCount();
	if (!num)
	{
		SM_DoSingleExecFwds(ctx);
	} else {
		bool can_create = true;
		for (unsigned int i=0; i<num; i++)
		{
			can_create = SM_ExecuteConfig(plugin, plugin->GetConfig(i), can_create);
		}
		char cmd[255];
		UTIL_Format(cmd, sizeof(cmd), "sm internal 2 %d\n", plugin->GetSerial());
		engine->ServerCommand(cmd);
	}
}

void SM_ExecuteAllConfigs()
{
	if (g_bConfigsExecd)
	{
		return;
	}

	engine->ServerCommand("exec cfg/sourcemod/sourcemod.cfg\n");

	IPluginIterator *iter = g_PluginSys.GetPluginIterator();
	while (iter->MorePlugins())
	{
		CPlugin *plugin = (CPlugin *)(iter->GetPlugin());
		unsigned int num = plugin->GetConfigCount();
		bool can_create = true;
		for (unsigned int i=0; i<num; i++)
		{
			can_create = SM_ExecuteConfig(plugin, plugin->GetConfig(i), can_create);
		}
		iter->NextPlugin();
	}
	iter->Release();

	engine->ServerCommand("sm internal 1\n");
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
