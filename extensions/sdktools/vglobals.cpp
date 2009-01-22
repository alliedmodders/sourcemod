/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod SDKTools Extension
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

size_t UTIL_StringToSignature(const char *str, char buffer[], size_t maxlength)
{
	size_t real_bytes = 0;
	size_t length = strlen(str);

	for (size_t i=0; i<length; i++)
	{
		if (real_bytes >= maxlength)
		{
			break;
		}
		buffer[real_bytes++] = (unsigned char)str[i];
		if (str[i] == '\\'
			&& str[i+1] == 'x')
		{
			if (i + 3 >= length)
			{
				continue;
			}
			/* Get the hex part */
			char s_byte[3];
			int r_byte;
			s_byte[0] = str[i+2];
			s_byte[1] = str[i+3];
			s_byte[2] = '\n';
			/* Read it as an integer */
			sscanf(s_byte, "%x", &r_byte);
			/* Save the value */
			buffer[real_bytes-1] = (unsigned char)r_byte;
			/* Adjust index */
			i += 3;
		}
	}

	return real_bytes;
}

bool UTIL_VerifySignature(const void *addr, const char *sig, size_t len)
{
	unsigned char *addr1 = (unsigned char *) addr;
	unsigned char *addr2 = (unsigned char *) sig;

	for (size_t i = 0; i < len; i++)
	{
		if (addr2[i] == '*')
			continue;
		if (addr1[i] != addr2[i])
			return false;
	}

	return true;
}

#if defined PLATFORM_WINDOWS
void GetIServer()
{
	const char *sigstr;
	char sig[32];
	size_t siglen;
	int offset;
	void *vfunc = NULL;

#if defined METAMOD_PLAPI_VERSION
	/* Get the CreateFakeClient function pointer */
	if (!(vfunc=SH_GET_ORIG_VFNPTR_ENTRY(engine, &IVEngineServer::CreateFakeClient)))
	{
		return;
	}
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
#endif

	/* Get signature string for IVEngineServer::CreateFakeClient() */
	sigstr = g_pGameConf->GetKeyValue("CreateFakeClient_Windows");

	if (!sigstr)
	{
		return;
	}

	/* Convert signature string to signature bytes */
	siglen = UTIL_StringToSignature(sigstr, sig, sizeof(sig));

	/* Check if we're on the expected function */
	if (!UTIL_VerifySignature(vfunc, sig, siglen))
	{
		return;
	}

	/* Get the offset into CreateFakeClient */
	if (!g_pGameConf->GetOffset("sv", &offset))
	{
		return;
	}

	/* Finally we have the interface we were looking for */
	iserver = *reinterpret_cast<IServer **>(reinterpret_cast<unsigned char *>(vfunc) + offset);
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
