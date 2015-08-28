/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2009 AlliedModders LLC.  All rights reserved.
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
#include "sourcemod.h"
#include "sourcemm_api.h"
#include "logic_bridge.h"
#include "sm_globals.h"
#include "RootConsoleMenu.h"
#include "CoreConfig.h"
#include <compat_wrappers.h>
#include <ITranslator.h>
#include <amtl/am-string.h>

#if SOURCE_ENGINE==SE_EPISODEONE || SOURCE_ENGINE==SE_DARKMESSIAH
class CCommandArgs : public ICommandArgs
{
public:
	CCommandArgs(const CCommand& _cmd)
	{
	}
	const char *Arg(int n) const
	{
		return engine->Cmd_Argv(n);
	}
	int ArgC() const
	{
		return engine->Cmd_Argc();
	}
	const char *ArgS() const
	{
		return engine->Cmd_Args();
	}
};
#else
class CCommandArgs : public ICommandArgs
{
	const CCommand *cmd;
public:
	CCommandArgs(const CCommand& _cmd) : cmd(&_cmd)
	{
	}
	const char *Arg(int n) const
	{
		return cmd->Arg(n);
	}
	int ArgC() const
	{
		return cmd->ArgC();
	}
	const char *ArgS() const
	{
		return cmd->ArgS();
	}
};
#endif

CON_COMMAND(sm, "SourceMod Menu")
{
#if SOURCE_ENGINE <= SE_DARKMESSIAH
	CCommand args;
#endif
	CCommandArgs cargs(args);

	if (cargs.ArgC() >= 2) {
		const char *cmdname = cargs.Arg(1);
		if (strcmp(cmdname, "internal") == 0)
		{
			if (cargs.ArgC() >= 3)
			{
				const char *arg = cargs.Arg(2);
				if (strcmp(arg, "1") == 0)
				{
					SM_ConfigsExecuted_Global();
				}
				else if (strcmp(arg, "2") == 0)
				{
					if (cargs.ArgC() >= 4)
					{
						SM_ConfigsExecuted_Plugin(atoi(cargs.Arg(3)));
					}
				}
			}
			return;
		}

	}

	g_RootMenu.GotRootCmd(&cargs);
}

FILE *g_pHndlLog = NULL;

void write_handles_to_log(const char *fmt, ...)
{
	va_list ap;
	
	va_start(ap, fmt);
	vfprintf(g_pHndlLog, fmt, ap);
	fprintf(g_pHndlLog, "\n");
	va_end(ap);
}

void write_handles_to_game(const char *fmt, ...)
{
	size_t len;
	va_list ap;
	char buffer[1024];
	
	va_start(ap, fmt);
	len = ke::SafeSprintf(buffer, sizeof(buffer)-2, fmt, ap);
	va_end(ap);

	buffer[len] = '\n';
	buffer[len+1] = '\0';

	engine->LogPrint(buffer);
}

CON_COMMAND(sm_dump_handles, "Dumps Handle usage to a file for finding Handle leaks")
{
#if SOURCE_ENGINE <= SE_DARKMESSIAH
	CCommand args;
#endif
	if (args.ArgC() < 2)
	{
		rootmenu->ConsolePrint("Usage: sm_dump_handles <file> or <log> for game logs");
		return;
	}

	if (strcmp(args.Arg(1), "log") != 0)
	{
		char filename[PLATFORM_MAX_PATH];
		const char *arg = args.Arg(1);
		g_SourceMod.BuildPath(Path_Game, filename, sizeof(filename), "%s", arg);

		FILE *fp = fopen(filename, "wt");
		if (!fp)
		{
			rootmenu->ConsolePrint("Failed to open \"%s\" for writing", filename);
			return;
		}

		g_pHndlLog = fp;
		logicore.DumpHandles(write_handles_to_log);
		g_pHndlLog = NULL;

		fclose(fp);
	}
	else
	{
		logicore.DumpHandles(write_handles_to_game);
	}
}

CON_COMMAND(sm_reload_translations, "Reparses all loaded translation files")
{
	translator->RebuildLanguageDatabase();
}

CON_COMMAND(sm_dump_admcache, "Dumps the admin cache for debugging")
{
	char buffer[PLATFORM_MAX_PATH];
	g_SourceMod.BuildPath(Path_SM, buffer, sizeof(buffer), "data/admin_cache_dump.txt");

	if (!logicore.DumpAdminCache(buffer))
	{
		rootmenu->ConsolePrint("Could not open file for writing: %s", buffer);
		return;
	}

	rootmenu->ConsolePrint("Admin cache dumped to: %s", buffer);
}

