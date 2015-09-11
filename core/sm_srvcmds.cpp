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
#include "CoreConfig.h"
#include "command_args.h"
#include <ITranslator.h>
#include <amtl/am-string.h>

CON_COMMAND(sm, "SourceMod Menu")
{
#if SOURCE_ENGINE <= SE_DARKMESSIAH
	CCommand args;
#endif
	EngineArgs cargs(args);

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

	logicore.OnRootCommand(&cargs);
}
