/**
 * vim: set ts=4 :
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
#include "sm_srvcmds.h"
#include "sm_version.h"
#include "sm_stringutil.h"
#include "LibrarySys.h"
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
IForward *g_pOnAutoConfigsBuffered = NULL;
CoreConfig g_CoreConfig;
bool g_bConfigsExecd = false;
bool g_bServerExecd = false;
bool g_bGotServerStart = false;
bool g_bGotTrigger = false;
ConCommand *g_pExecPtr = NULL;
ConVar *g_ServerCfgFile = NULL;

void CheckAndFinalizeConfigs();

#if defined ORANGEBOX_BUILD
SH_DECL_EXTERN1_void(ConCommand, Dispatch, SH_NOATTRIB, false, const CCommand &);
void Hook_ExecDispatchPre(const CCommand &cmd)
#else
extern bool __SourceHook_FHAddConCommandDispatch(void *,bool,class fastdelegate::FastDelegate0<void>);
extern bool __SourceHook_FHRemoveConCommandDispatch(void *,bool,class fastdelegate::FastDelegate0<void>);
void Hook_ExecDispatchPre()
#endif
{
#if !defined ORANGEBOX_BUILD
	CCommand cmd;
#endif

	const char *arg = cmd.Arg(1);

	if (!g_bServerExecd 
		&& arg != NULL 
		&& strcmp(arg, g_ServerCfgFile->GetString()) == 0)
	{
		g_bGotTrigger = true;
	}
}

#if defined ORANGEBOX_BUILD
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
	if ((g_bServerExecd || g_ServerCfgFile == NULL) 
		&& g_bGotServerStart)
	{
		/* Order is important here.  We need to buffer things before we send the command out. */
		g_pOnAutoConfigsBuffered->Execute(NULL);
		engine->ServerCommand("sm internal 1\n");
	}
}

void CoreConfig::OnSourceModAllInitialized()
{
	g_RootMenu.AddRootConsoleCommand("config", "Set core configuration options", this);
	g_pOnServerCfg = g_Forwards.CreateForward("OnServerCfg", ET_Ignore, 0, NULL);
	g_pOnConfigsExecuted = g_Forwards.CreateForward("OnConfigsExecuted", ET_Ignore, 0, NULL);
	g_pOnAutoConfigsBuffered = g_Forwards.CreateForward("OnAutoConfigsBuffered", ET_Ignore, 0, NULL);
}

void CoreConfig::OnSourceModShutdown()
{
	g_RootMenu.RemoveRootConsoleCommand("config", this);
	g_Forwards.ReleaseForward(g_pOnServerCfg);
	g_Forwards.ReleaseForward(g_pOnConfigsExecuted);
	g_Forwards.ReleaseForward(g_pOnAutoConfigsBuffered);

	if (g_pExecPtr != NULL)
	{
		SH_REMOVE_HOOK_STATICFUNC(ConCommand, Dispatch, g_pExecPtr, Hook_ExecDispatchPre, false);
		SH_REMOVE_HOOK_STATICFUNC(ConCommand, Dispatch, g_pExecPtr, Hook_ExecDispatchPost, true);
		g_pExecPtr = NULL;
	}
}

void CoreConfig::OnSourceModLevelChange(const char *mapName)
{
	static bool already_checked = false;

	if (!already_checked)
	{
		g_ServerCfgFile = icvar->FindVar("servercfgfile");
		if (g_ServerCfgFile != NULL)
		{
			ConCommandBase *pBase = icvar->GetCommands();
			while (pBase != NULL)
			{
				if (pBase->IsCommand() && strcmp(pBase->GetName(), "exec") == 0)
				{
					break;
				}
				pBase = const_cast<ConCommandBase *>(pBase->GetNext());
			}

			g_pExecPtr = (ConCommand *)pBase;
			if (g_pExecPtr != NULL)
			{
				SH_ADD_HOOK_STATICFUNC(ConCommand, Dispatch, g_pExecPtr, Hook_ExecDispatchPre, false);
				SH_ADD_HOOK_STATICFUNC(ConCommand, Dispatch, g_pExecPtr, Hook_ExecDispatchPost, true);
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

void CoreConfig::OnRootConsoleCommand(const char *cmdname, const CCommand &command)
{
	int argcount = command.ArgC();
	if (argcount >= 4)
	{
		const char *option = command.Arg(2);
		const char *value = command.Arg(3);

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
	SMCError err;
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
	if ((err=textparsers->ParseFile_SMC(filePath, this, NULL)) != SMCError_Okay)
	{
 		/* :TODO: This won't actually log or print anything :( - So fix that somehow */
		const char *error = textparsers->GetSMCErrorString(err);
		g_Logger.LogFatal("[SM] Error encountered parsing core config file: %s", error ? error : "");
	}
}

SMCResult CoreConfig::ReadSMC_KeyValue(const SMCStates *states, const char *key, const char *value)
{
	char error[255];
	ConfigResult err = SetConfigOption(key, value, ConfigSource_File, error, sizeof(error));

	if (err == ConfigResult_Reject)
	{
		/* This is a fatal error */
		g_Logger.LogFatal("Config error (key: %s) (value: %s) %s", key, value, error);
	}

	return SMCResult_Continue;
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
				if (!g_LibSys.CreateFolder(build))
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
					const ConVar *cvar = (*iter);

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
	}
	else
	{
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
	if (g_bGotServerStart)
	{
		return;
	}

	engine->ServerCommand("exec sourcemod/sourcemod.cfg\n");

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

#if defined ORANGEBOX_BUILD
	engine->ServerExecute();
#endif

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
