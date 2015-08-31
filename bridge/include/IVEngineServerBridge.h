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
#ifndef _INCLUDE_SOURCEMOD_BRIDGE_IVENGINESERVER_H_
#define _INCLUDE_SOURCEMOD_BRIDGE_IVENGINESERVER_H_

struct edict_t;

namespace SourceMod {

class IVEngineServerBridge
{
public:
	virtual bool IsDedicatedServer() = 0;
	virtual void InsertServerCommand(const char *cmd) = 0;
	virtual void ServerCommand(const char *cmd) = 0;
	virtual void ServerExecute() = 0;
	virtual const char *GetClientConVarValue(int clientIndex, const char *name) = 0;
	virtual void ClientCommand(edict_t *pEdict, const char *szCommand) = 0;
	virtual void FakeClientCommand(edict_t *pEdict, const char *szCommand) = 0;
};

} // namespace SourceMod

#endif // _INCLUDE_SOURCEMOD_BRIDGE_IVENGINESERVER_H_
