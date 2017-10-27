/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Team Fortress 2 Extension
 * Copyright (C) 2004-2015 AlliedModders LLC.  All rights reserved.
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

#ifndef _INCLUDE_SOURCEMOD_CONDITIONS_H_
#define _INCLUDE_SOURCEMOD_CONDITIONS_H_

#include "extension.h"

struct CondChangeData_t;

class PlayerConditionsMgr : public IClientListener
{
public:
	PlayerConditionsMgr();
	bool Init();
	void Shutdown();
public: // IClientListener
	void OnClientPutInServer(int client);
public:
	enum CondVar : size_t
	{
		m_nPlayerCond,
		_condition_bits,
		m_nPlayerCondEx,
		m_nPlayerCondEx2,
		m_nPlayerCondEx3,
		m_nPlayerCondEx4,

		CondVar_Count
	};

	void OnConVarChange(CondVar var, const SendProp *pProp, const void *pStructBase, const void *pData, DVariant *pOut, int iElement, int objectID);
	void ProcessCondChange(CondChangeData_t *pCondData);
private:
	inline unsigned int GetPropOffs(CondVar var)
	{
		return m_CondVarProps[var].actual_offset;
	}
	inline SendProp *GetProp(CondVar var)
	{
		return m_CondVarProps[var].prop;
	}
	template<CondVar var>
	bool SetupProp(const char *varname);
private:
	int m_OldConds[SM_MAXPLAYERS + 1][CondVar_Count];
	sm_sendprop_info_t m_CondVarProps[CondVar_Count];
	int m_CondOffset[CondVar_Count];
	SendVarProxyFn m_BackupProxyFns[CondVar_Count];
};

extern PlayerConditionsMgr g_CondMgr;

extern IForward *g_addCondForward;
extern IForward *g_removeCondForward;

#endif //_INCLUDE_SOURCEMOD_CONDITIONS_H_
