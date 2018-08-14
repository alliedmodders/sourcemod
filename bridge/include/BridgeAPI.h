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
#ifndef _INCLUDE_SOURCEMOD_BRIDGE_API_H_
#define _INCLUDE_SOURCEMOD_BRIDGE_API_H_

#include <bridge/include/CoreProvider.h>
#include <bridge/include/LogicProvider.h>
#include <stdint.h>

namespace SourceMod {

// Add 1 to the RHS of this expression to bump the intercom file
// This is to prevent mismatching core/logic binaries
static const uint32_t SM_LOGIC_MAGIC = 0x0F47C0DE - 57;

} // namespace SourceMod

typedef void (*LogicInitFunction)(SourceMod::CoreProvider *core, SourceMod::sm_logic_t *logic);
typedef LogicInitFunction (*LogicLoadFunction)(uint32_t magic);
typedef SourceMod::ITextParsers *(*GetITextParsers)();

#endif // _INCLUDE_SOURCEMOD_BRIDGE_API_H_
