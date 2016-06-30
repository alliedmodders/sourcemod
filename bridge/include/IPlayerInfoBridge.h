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
#ifndef _INCLUDE_SOURCEMOD_BRIDGE_INCLUDE_IPLAYERINFO_H_
#define _INCLUDE_SOURCEMOD_BRIDGE_INCLUDE_IPLAYERINFO_H_

class IPlayerInfo;

namespace SourceMod {

class IPlayerInfoBridge
{
public:
	virtual bool IsObserver(IPlayerInfo *pInfo) = 0;
	virtual int GetTeamIndex(IPlayerInfo *pInfo) = 0;
	virtual int GetFragCount(IPlayerInfo *pInfo) = 0;
	virtual int GetDeathCount(IPlayerInfo *pInfo) = 0;
	virtual int GetArmorValue(IPlayerInfo *pInfo) = 0;
	virtual void GetAbsOrigin(IPlayerInfo *pInfo, float *x, float *y, float *z) = 0;
	virtual void GetAbsAngles(IPlayerInfo *pInfo, float *x, float *y, float *z) = 0;
	virtual void GetPlayerMins(IPlayerInfo *pInfo, float *x, float *y, float *z) = 0;
	virtual void GetPlayerMaxs(IPlayerInfo *pInfo, float *x, float *y, float *z) = 0;
	virtual const char *GetWeaponName(IPlayerInfo *pInfo) = 0;
	virtual const char *GetModelName(IPlayerInfo *pInfo) = 0;
	virtual int GetHealth(IPlayerInfo *pInfo) = 0;
	virtual void ChangeTeam(IPlayerInfo *pInfo, int iTeamNum) = 0;
};

} // namespace SourceMod

#endif // _INCLUDE_SOURCEMOD_BRIDGE_INCLUDE_IPLAYERINFO_H_

