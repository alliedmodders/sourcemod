/**
 * vim: set ts=4 :
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

#include <sp_vm_api.h>
#include <IHandleSys.h>
#include <IShareSys.h>

#ifndef _INCLUDE_SOURCEMOD_INTERCOM_H_
#define _INCLUDE_SOURCEMOD_INTERCOM_H_

using namespace SourceMod;

/**
 * Add 1 to the RHS of this expression to bump the intercom file
 * This is to prevent mismatching core/logic binaries
 */
#define SM_LOGIC_MAGIC		(0x0F47C0DE - 3)

#if defined SM_LOGIC
class IVEngineServer
#else
class IVEngineServer_Logic
#endif
{
public:
	virtual bool IsMapValid(const char *map) = 0;
};

namespace SourceMod
{
	class ISourceMod;
	class ILibrarySys;
	class ITextParsers;
	class IThreader;
	class IRootConsole;
	class IPluginManager;
	class IForwardManager;
}

class IVEngineServer;
class ConVar;

struct sm_core_t
{
	/* Objects */
	IHandleSys 		*handlesys;
	IdentityToken_t *core_ident;
	ISourceMod		*sm;
	ILibrarySys		*libsys;
	IVEngineServer	*engine;
	IShareSys		*sharesys;
	IRootConsole	*rootmenu;
	IPluginManager	*pluginsys;
	IForwardManager	*forwardsys;
	/* Functions */
	void			(*AddNatives)(sp_nativeinfo_t* nlist);
	ConVar *		(*FindConVar)(const char*);
	unsigned int	(*strncopy)(char*, const char*, size_t);
	char *			(*TrimWhitespace)(char *, size_t &);
	void			(*LogError)(const char*, ...);
	const char *	(*GetCvarString)(ConVar*);
	size_t			(*Format)(char*, size_t, const char*, ...);
	unsigned int	(*ReplaceAll)(char*, size_t, const char *, const char *, bool);
};

struct sm_logic_t
{
	SMGlobalClass	*head;
	IThreader		*threader;
	IProfiler		*profiler;
	unsigned int    (*CRC32)(const void *, size_t);
};

typedef void (*LogicInitFunction)(const sm_core_t *core, sm_logic_t *logic);
typedef LogicInitFunction (*LogicLoadFunction)(uint32_t magic);
typedef ITextParsers *(*GetITextParsers)();

#endif /* _INCLUDE_SOURCEMOD_INTERCOM_H_ */

