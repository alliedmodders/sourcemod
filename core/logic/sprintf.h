// vim: set ts=4 sw=4 tw=99 noet :
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
#ifndef _include_sourcemod_core_logic_sprintf_h_
#define _include_sourcemod_core_logic_sprintf_h_

#include <sp_vm_api.h>

namespace SourceMod {
class IDatabase;
class IPhraseCollection;
}

// "AMX Templated Cell Printf", originally. SourceMod doesn't have cell-strings
// so this is a normal sprintf(), except that its variadic arguments are
// derived from scripted arguments.
size_t atcprintf(char *buffer,
                 size_t maxlen,
                 const char *format,
                 SourcePawn::IPluginContext *pCtx,
                 const cell_t *params,
                 int *param);

// "Generic Printf", originally. This is similar to atcprintf, except arguments
// are provided as an array of opaque pointers, rather than scripted arguments
// or C++ va_lists. This is essentially what Core uses to translate and format
// phrases internally.
bool gnprintf(char *buffer,
              size_t maxlen,
              const char *format,
              SourceMod::IPhraseCollection *pPhrases,
              void **params,
              unsigned int numparams,
              unsigned int &curparam,
              size_t *pOutLength,
              const char **pFailPhrase);

extern SourceMod::IDatabase *g_FormatEscapeDatabase;

#endif // _include_sourcemod_core_logic_sprintf_h_
