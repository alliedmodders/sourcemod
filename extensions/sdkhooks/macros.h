/**
 * vim: set ts=4 :
 * =============================================================================
 * Source SDK Hooks Extension
 * Copyright (C) 2010-2012 Nicholas Hastings
 * Copyright (C) 2009-2010 Erik Minekus
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

#define SET_PRE_true(gamedataname) g_HookTypes[SDKHook_##gamedataname].supported = true;
#define SET_PRE_false(gamedataname)
#define SET_POST_true(gamedataname) g_HookTypes[SDKHook_##gamedataname##Post].supported = true;
#define SET_POST_false(gamedataname)

#define CHECKOFFSET(gamedataname, supportsPre, supportsPost) \
	offset = 0; \
	g_pGameConf->GetOffset(#gamedataname, &offset); \
	if (offset > 0) \
	{ \
		SH_MANUALHOOK_RECONFIGURE(gamedataname, offset, 0, 0); \
		SET_PRE_##supportsPre(gamedataname) \
		SET_POST_##supportsPost(gamedataname) \
	}

#define CHECKOFFSET_W(gamedataname, supportsPre, supportsPost) \
	offset = 0; \
	g_pGameConf->GetOffset("Weapon_"#gamedataname, &offset); \
	if (offset > 0) \
	{ \
		SH_MANUALHOOK_RECONFIGURE(Weapon_##gamedataname, offset, 0, 0); \
		SET_PRE_##supportsPre(Weapon##gamedataname) \
		SET_POST_##supportsPost(Weapon##gamedataname) \
	}

#define HOOKLOOP \
	for(int i = g_HookList.Count() - 1; i >= 0; i--)
