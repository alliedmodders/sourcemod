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
#include "LibrarySys.h"
#include "TextParsers.h"
#include "Logger.h"

#ifdef PLATFORM_WINDOWS
ConVar sm_corecfgfile("sm_corecfgfile", "addons\\sourcemod\\configs\\core.cfg", 0, "SourceMod core configuration file");
#elif defined PLATFORM_LINUX
ConVar sm_corecfgfile("sm_corecfgfile", "addons/sourcemod/configs/core.cfg", 0, "SourceMod core configuration file");
#endif

CoreConfig g_CoreConfig;

void CoreConfig::OnSourceModAllInitialized()
{
	g_RootMenu.AddRootConsoleCommand("config", "Set core configuration options", this);
}

void CoreConfig::OnSourceModShutdown()
{
	g_RootMenu.RemoveRootConsoleCommand("config", this);
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
