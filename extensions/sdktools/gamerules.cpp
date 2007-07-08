/**
 * vim: set ts=4 :
 * ===============================================================
 * SourceMod SDK Tools Extension
 * Copyright (C) 2004-2007 AlliedModders LLC. All rights reserved.
 * ===============================================================
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Version: $Id$
 */

#include "extension.h"

void **g_pGameRules = NULL;

void InitializeGameRules()
{
	char *addr = NULL;

#ifdef PLATFORM_WINDOWS
	int offset = 0;

	if (!g_pGameConf->GetMemSig("CreateGameRulesObject", (void **)&addr) || !addr)
	{
		return;
	}
	if (!g_pGameConf->GetOffset("g_pGameRules", &offset) || !offset)
	{
		return;
	}

	g_pGameRules = *reinterpret_cast<void ***>(addr + offset);
#elif defined PLATFORM_LINUX
	if (!g_pGameConf->GetMemSig("g_pGameRules", (void **)&addr) || !addr)
	{
		return;
	}

	g_pGameRules = reinterpret_cast<void **>(addr);
#endif
}
