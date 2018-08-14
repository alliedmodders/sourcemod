// vim: set ts=4 sw=4 tw=99 noet:
// =============================================================================
// SourceMod
// Copyright (C) 2004-2015 AlliedModders LLC.  All rights reserved.
// =============================================================================
//
// This program is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License, version 3.0, as published by the
// Free Software Foundation.
// 
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along with
// this program.  If not, see <http://www.gnu.org/licenses/>.
//
// As a special exception, AlliedModders LLC gives you permission to link the
// code of this program (as well as its derivative works) to "Half-Life 2," the
// "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
// by the Valve Corporation.  You must obey the GNU General Public License in
// all respects for all other code used.  Additionally, AlliedModders LLC grants
// this exception to all derivative works.  AlliedModders LLC defines further
// exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
// or <http://www.sourcemod.net/license.php>.
#ifndef _INCLUDE_SOURCEMOD_LOGIC_PROVIDER_API_H_
#define _INCLUDE_SOURCEMOD_LOGIC_PROVIDER_API_H_

#include <sp_vm_api.h>

class SMGlobalClass;

namespace SourceMod {

using namespace SourcePawn;

class CoreProvider;
class IThreader;
class ITranslator;
class IGameConfig;
class IScriptManager;
class IShareSys;
class IHandleSys;
class ICommandArgs;
class IForwardManager;
class IAdminSystem;
class IRootConsole;
class IProviderCallbacks;
class IExtensionSys;
class ITextParsers;
class ILogger;
class ICellArray;

struct sm_logic_t
{
	SMGlobalClass	*head;
	IThreader		*threader;
	ITranslator		*translator;
	const char      *(*stristr)(const char *, const char *);
	size_t			(*atcprintf)(char *, size_t, const char *, IPluginContext *, const cell_t *, int *);
	bool			(*CoreTranslate)(char *,  size_t, const char *, unsigned int, size_t *, ...);
	void            (*AddCorePhraseFile)(const char *filename);
	unsigned int	(*ReplaceAll)(char*, size_t, const char *, const char *, bool);
	char            *(*ReplaceEx)(char *, size_t, const char *, size_t, const char *, size_t, bool);
	size_t          (*DecodeHexString)(unsigned char *, size_t, const char *);
	IGameConfig *   (*GetCoreGameConfig)();
	IDebugListener   *debugger;
	void			(*GenerateError)(IPluginContext *, cell_t, int, const char *, ...);
	void			(*AddNatives)(sp_nativeinfo_t *natives);
	void            (*RegisterProfiler)(IProfilingTool *tool);
	ICellArray *    (*CreateCellArray)(size_t blocksize);
	void            (*FreeCellArray)(ICellArray *arr);
	void *			(*FromPseudoAddress)(uint32_t pseudoAddr);
	uint32_t		(*ToPseudoAddress)(void *addr);
	IScriptManager	*scripts;
	IShareSys		*sharesys;
	IExtensionSys	*extsys;
	IHandleSys		*handlesys;
	IForwardManager	*forwardsys;
	IAdminSystem	*adminsys;
	IdentityToken_t *core_ident;
	ILogger			*logger;
	IRootConsole	*rootmenu;
	IProviderCallbacks *callbacks;
	float			sentinel;
};

} // namespace SourceMod

#endif // _INCLUDE_SOURCEMOD_LOGIC_PROVIDER_API_H_

