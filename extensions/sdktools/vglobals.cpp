/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod SDKTools Extension
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

#include "extension.h"

void **g_pGameRules = NULL;
void *g_EntList = NULL;

#ifdef PLATFORM_WINDOWS
void InitializeValveGlobals()
{
	char *addr = NULL;
	int offset;

	/* gEntList and/or g_pEntityList */
	if (!g_pGameConf->GetMemSig("LevelShutdown", (void **)&addr) || !addr)
	{
		return;
	}
	if (!g_pGameConf->GetOffset("gEntList", &offset) || !offset)
	{
		return;
	}
	g_EntList = *reinterpret_cast<void **>(addr + offset);

	/* g_pGameRules */
	if (!g_pGameConf->GetMemSig("CreateGameRulesObject", (void **)&addr) || !addr)
	{
		return;
	}
	if (!g_pGameConf->GetOffset("g_pGameRules", &offset) || !offset)
	{
		return;
	}
	g_pGameRules = *reinterpret_cast<void ***>(addr + offset);
}
#elif defined PLATFORM_LINUX
void InitializeValveGlobals()
{
	char *addr = NULL;

	/* gEntList and/or g_pEntityList */
	if (!g_pGameConf->GetMemSig("gEntList", (void **)&addr) || !addr)
	{
		return;
	}
	g_EntList = reinterpret_cast<void *>(addr);

	/* g_pGameRules */
	if (!g_pGameConf->GetMemSig("g_pGameRules", (void **)&addr) || !addr)
	{
		return;
	}
	g_pGameRules = reinterpret_cast<void **>(addr);
}
#endif

bool vcmp(const void *_addr1, const void *_addr2, size_t len)
{
	unsigned char *addr1 = (unsigned char *)_addr1;
	unsigned char *addr2 = (unsigned char *)_addr2;

	for (size_t i=0; i<len; i++)
	{
		if (addr2[i] == '*')
			continue;
		if (addr1[i] != addr2[i])
			return false;
	}

	return true;
}

#if defined PLATFORM_WINDOWS
	/* Thanks to DS for the sigs */
	#define ISERVER_WIN_SIG				"\x8B\x44\x24\x2A\x50\xB9\x2A\x2A\x2A\x2A\xE8"
	#define ISERVER_WIN_SIG_LEN			11
void GetIServer()
{
	int offset;
	void *vfunc = NULL;

	/* Get the offset into CreateFakeClient */
	if (!g_pGameConf->GetOffset("sv", &offset))
	{
		return;
	}
#if defined METAMOD_PLAPI_VERSION
	/* Get the CreateFakeClient function pointer */
	if (!(vfunc=SH_GET_ORIG_VFNPTR_ENTRY(engine, &IVEngineServer::CreateFakeClient)))
	{
		return;
	}

	/* Check if we're on the expected function */
	if (!vcmp(vfunc, ISERVER_WIN_SIG, ISERVER_WIN_SIG_LEN))
	{
		return;
	}

	/* Finally we have the interface we were looking for */
	iserver = *reinterpret_cast<IServer **>(reinterpret_cast<unsigned char *>(vfunc) + offset);
#else
	/* Get the interface manually */
	SourceHook::MemFuncInfo info = {true, -1, 0, 0};
	SourceHook::GetFuncInfo(&IVEngineServer::CreateFakeClient, info);

	vfunc = enginePatch->GetOrigFunc(info.vtbloffs, info.vtblindex);
	if (!vfunc)
	{
		void **vtable = *reinterpret_cast<void ***>(enginePatch->GetThisPtr() + info.thisptroffs + info.vtbloffs);
		vfunc = vtable[info.vtblindex];
	}
	/* Check if we're on the expected function */
	if (!vcmp(vfunc, ISERVER_WIN_SIG, ISERVER_WIN_SIG_LEN))
	{
		return;
	}

	iserver = *reinterpret_cast<IServer **>(reinterpret_cast<unsigned char *>(vfunc) + offset);
#endif
}
#elif defined PLATFORM_POSIX
void GetIServer()
{
	void *addr;
	if (!g_pGameConf->GetMemSig("sv", &addr) || !addr)
	{
		return;
	}

	iserver = reinterpret_cast<IServer *>(addr);
}
#endif
