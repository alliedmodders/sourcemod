/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod
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

#ifndef _INCLUDE_SOURCEMOD_MENUVOTING_H_
#define _INCLUDE_SOURCEMOD_MENUVOTING_H_

#include <IMenuManager.h>
#include <IPlayerHelpers.h>
#include <sh_vector.h>
#include "sm_globals.h"
#include <TimerSys.h>

using namespace SourceHook;
using namespace SourceMod;

class VoteMenuHandler : 
	public IMenuHandler, 
	public SMGlobalClass,
	public IClientListener,
	public ITimedEvent
{
public: //SMGlobalClass
	void OnSourceModAllInitialized();
	void OnSourceModShutdown();
	void OnSourceModLevelChange(const char *mapName);
public: //IClientListener
	void OnClientDisconnected(int client);
public: //IMenuHandler
	unsigned int GetMenuAPIVersion2();
	void OnMenuStart(IBaseMenu *menu);
	void OnMenuDisplay(IBaseMenu *menu, int client, IMenuPanel *display);
	void OnMenuSelect(IBaseMenu *menu, int client, unsigned int item);
	void OnMenuSelect2(IBaseMenu *menu, int client, unsigned int item, unsigned int item_on_page);
	void OnMenuCancel(IBaseMenu *menu, int client, MenuCancelReason reason);
	void OnMenuEnd(IBaseMenu *menu, MenuEndReason reason);
	void OnMenuDrawItem(IBaseMenu *menu, int client, unsigned int item, unsigned int &style);
	unsigned int OnMenuDisplayItem(IBaseMenu *menu, int client, IMenuPanel *panel, unsigned int item, const ItemDrawInfo &dr);
public: //ITimedEvent
	ResultType OnTimer(ITimer *pTimer, void *pData);
	void OnTimerEnd(ITimer *pTimer, void *pData);
public:
	bool StartVote(IBaseMenu *menu,
		unsigned int num_clients,
		int clients[],
		unsigned int max_time,
		unsigned int flags=0);
	bool IsVoteInProgress();
	void CancelVoting();
	IBaseMenu *GetCurrentMenu();
	bool IsCancelling();
	unsigned int GetRemainingVoteDelay();
	bool IsClientInVotePool(int client);
	bool GetClientVoteChoice(int client, unsigned int *pItem);
	bool RedrawToClient(int client, bool revote);
private:
	void Reset(IMenuHandler *mh);
	void DecrementPlayerCount();
	void EndVoting();
	void InternalReset();
	bool InitializeVoting(IBaseMenu *menu, 
		IMenuHandler *handler,
		unsigned int time,
		unsigned int flags);
	void StartVoting();
	void DrawHintProgress();
	void BuildVoteLeaders();
private:
	IMenuHandler *m_pHandler;
	unsigned int m_Clients;
	unsigned int m_TotalClients;
	unsigned int m_Items;
	CVector<unsigned int> m_Votes;
	IBaseMenu *m_pCurMenu;
	bool m_bStarted;
	bool m_bCancelled;
	unsigned int m_NumVotes;
	unsigned int m_VoteTime;
	unsigned int m_VoteFlags;
	float m_fStartTime;
	unsigned int m_nMenuTime;
	int m_ClientVotes[256+1];
	bool m_Revoting[256+1];
	char m_leaderList[1024];
	ITimer *m_displayTimer;
};

#endif //_INCLUDE_SOURCEMOD_MENUVOTING_H_

