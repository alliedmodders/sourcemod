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
#else
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

		CoreConfigErr err = SetConfigOption(option, value);

		switch (err)
		{
		case CoreConfig_NoRuntime:
			g_RootMenu.ConsolePrint("[SM] Cannot set \"%s\" while SourceMod is running.", option);
			break;
		case CoreConfig_InvalidValue:
			g_RootMenu.ConsolePrint("[SM] Invalid value \"%s\" specified for configuration option \"%s\"", value, option);
			break;
		case CoreConfig_InvalidOption:
			g_RootMenu.ConsolePrint("[SM] Invalid configuration option specified: %s", option);
			break;
		default:
			break;
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
	g_LibSys.PathFormat(filePath, sizeof(filePath), "%s/%s", g_SourceMod.GetModPath(), corecfg);

	/* Parse config file */
	if ((err=g_TextParser.ParseFile_SMC(filePath, this, NULL, NULL))
		!= SMCParse_Okay)
	{
		/* :TODO: This won't actually log or print anything :( - So fix that somehow */
		g_Logger.LogError("[SM] Error encountered parsing core config file: %s", g_TextParser.GetSMCErrorString(err));
	}
}

SMCParseResult CoreConfig::ReadSMC_KeyValue(const char *key, const char *value, bool key_quotes, bool value_quotes)
{
	CoreConfigErr err = SetConfigOption(key, value);

	if (err == CoreConfig_InvalidOption)
	{
		g_Logger.LogError("[SM] Warning: Ignoring invalid option \"%s\" in configuration file.", key);
	} else if (err == CoreConfig_InvalidValue) {
		g_Logger.LogError("[SM] Warning encountered parsing core configuration file.");
		g_Logger.LogError("[SM] Invalid value \"%s\" specified for option \"%s\"", value, key);
	}

	return SMCParse_Continue;
}

CoreConfigErr CoreConfig::SetConfigOption(const char *option, const char *value)
{
	CoreConfigErr err = CoreConfig_TOTAL;
	CoreConfigErr currentErr;

	/* Notify! */
	SMGlobalClass *pBase = SMGlobalClass::head;
	while (pBase)
	{
		currentErr = pBase->OnSourceModConfigChanged(option, value);

		/* Lowest error code has priority */
		if (currentErr < err)
		{
			err = currentErr;
		}

		pBase = pBase->m_pGlobalClassNext;
	}

	return err;
}
