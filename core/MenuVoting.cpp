/**
 * vim: set ts=4 sw=4 :
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

#include <string.h>
#include <stdlib.h>
#include "MenuVoting.h"
#include "PlayerManager.h"
#include "sourcemm_api.h"
#include <sourcemod.h>
#include <Logger.h>
#include <HalfLife2.h>
#include <mathlib.h>
#include <const.h>
#include <ITranslator.h>
#include "logic_bridge.h"

float g_next_vote = 0.0f;

#define VOTE_NOT_VOTING -2
#define VOTE_PENDING -1

ConVar sm_vote_hintbox("sm_vote_progress_hintbox", 
					   "0", 
					   0, 
					   "Show current vote progress in a hint box",
					   true,
					   0.0,
					   true,
					   1.0);

ConVar sm_vote_chat("sm_vote_progress_chat", 
					   "0", 
					   0, 
					   "Show current vote progress as chat messages",
					   true,
					   0.0,
					   true,
					   1.0);

ConVar sm_vote_console("sm_vote_progress_console", 
					   "0", 
					   0, 
					   "Show current vote progress as console messages",
					   true,
					   0.0,
					   true,
					   1.0);

ConVar sm_vote_client_console("sm_vote_progress_client_console", 
					   "0", 
					   0, 
					   "Show current vote progress as console messages to clients",
					   true,
					   0.0,
					   true,
					   1.0);


#if SOURCE_ENGINE >= SE_ORANGEBOX
void OnVoteDelayChange(IConVar *cvar, const char *value, float flOldValue);
#else
void OnVoteDelayChange(ConVar *cvar, const char *value);
#endif
ConVar sm_vote_delay("sm_vote_delay",
					 "30",
					 0,
					 "Sets the recommended time in between public votes",
					 false,
					 0.0,
					 false,
					 0.0,
					 OnVoteDelayChange);

#if SOURCE_ENGINE >= SE_ORANGEBOX
void OnVoteDelayChange(IConVar *cvar, const char *value, float flOldValue)
#else
void OnVoteDelayChange(ConVar *cvar, const char *value)
#endif
{
	/* See if the new vote delay isn't something we need to account for */
	if (sm_vote_delay.GetFloat() < 1.0f)
	{
		g_next_vote = 0.0f;
		return;
	}

	/* If there was never a last vote, ignore this change */
	if (g_next_vote < 0.1f)
	{
		return;
	}

	/* Subtract the original value, then add the new one. */
	g_next_vote -= (float)atof(value);
	g_next_vote += sm_vote_delay.GetFloat();
}

unsigned int VoteMenuHandler::GetRemainingVoteDelay()
{
	if (g_next_vote <= gpGlobals->curtime)
	{
		return 0;
	}

	return (unsigned int)(g_next_vote - gpGlobals->curtime);
}

void VoteMenuHandler::OnSourceModAllInitialized()
{
	g_Players.AddClientListener(this);
}

void VoteMenuHandler::OnSourceModShutdown()
{
	g_Players.RemoveClientListener(this);
}

void VoteMenuHandler::OnSourceModLevelChange(const char *mapName)
{
	g_next_vote = 0.0f;
}

unsigned int VoteMenuHandler::GetMenuAPIVersion2()
{
	return m_pHandler->GetMenuAPIVersion2();
}

void VoteMenuHandler::OnClientDisconnected(int client)
{
	if (!IsVoteInProgress())
	{
		return;
	}

	/* Wipe out their vote if they had one.  We have to make sure the the the
	 * newly connected client is not allowed to vote. 
	 */
	int item;
	if ((item = m_ClientVotes[client]) >= VOTE_PENDING)
	{
		if (item >= 0)
		{
			assert((unsigned)item < m_Items);
			assert(m_Votes[item] > 0);
			m_Votes[item]--;
		}
		m_ClientVotes[client] = VOTE_NOT_VOTING;
	}
}

bool VoteMenuHandler::IsVoteInProgress()
{
	return (m_pCurMenu != NULL);
}

bool VoteMenuHandler::StartVote(IBaseMenu *menu,
								unsigned int num_clients,
								int clients[],
								unsigned int max_time,
								unsigned int flags/* =0 */)
{
	if (!InitializeVoting(menu, menu->GetHandler(), max_time, flags))
	{
		return false;
	}

	/* Note: we can use game time and not universal time because 
	 * if we're voting then players are in-game.
	 */

	float fVoteDelay = sm_vote_delay.GetFloat();
	if (fVoteDelay < 1.0)
	{
		g_next_vote = 0.0;
	}
	else
	{
		/* This little trick breaks for infinite votes!
		 * However, we just ignore that since those 1) shouldn't exist and 
		 * 2) people must be checking IsVoteInProgress() beforehand anyway.
		 */
		g_next_vote = gpGlobals->curtime + fVoteDelay + (float)max_time;
	}

	m_fStartTime = gpGlobals->curtime;
	m_nMenuTime = max_time;

	for (unsigned int i=0; i<num_clients; i++)
	{
		if (clients[i] < 1 || clients[i] > 256)
		{
			continue;
		}
		menu->Display(clients[i], max_time, this);
	}

	StartVoting();

	return true;
}

bool VoteMenuHandler::IsClientInVotePool(int client)
{
	if (client < 1 
		|| client > g_Players.MaxClients() 
		|| m_pCurMenu == NULL)
	{
		return false;
	}

	return (m_ClientVotes[client] > VOTE_NOT_VOTING);
}

bool VoteMenuHandler::GetClientVoteChoice(int client, unsigned int *pItem)
{
	if (!IsClientInVotePool(client)
		|| m_ClientVotes[client] == VOTE_PENDING)
	{
		return false;
	}

	*pItem = m_ClientVotes[client];

	return true;
}

bool VoteMenuHandler::RedrawToClient(int client, bool revotes)
{
	unsigned int time_limit;

	if (!IsClientInVotePool(client))
	{
		return false;
	}

	if (m_ClientVotes[client] >= 0)
	{
		if ((m_VoteFlags & VOTEFLAG_NO_REVOTES) == VOTEFLAG_NO_REVOTES || !revotes)
		{
			return false;
		}
		assert((unsigned)m_ClientVotes[client] < m_Items);
		assert(m_Votes[m_ClientVotes[client]] > 0);
		m_Votes[m_ClientVotes[client]]--;
		m_ClientVotes[client] = VOTE_PENDING;
		m_Revoting[client] = true;
		m_NumVotes--;
	}

	if (m_nMenuTime == MENU_TIME_FOREVER)
	{
		time_limit = m_nMenuTime;
	}
	else
	{
		time_limit = (int)((float)m_nMenuTime - (gpGlobals->curtime - m_fStartTime));
		
		/* Make sure this doesn't round to zero */
		if (time_limit == MENU_TIME_FOREVER)
		{
			time_limit = 1;
		}
	}

	return m_pCurMenu->Display(client, time_limit, this);
}

bool VoteMenuHandler::InitializeVoting(IBaseMenu *menu, 
									   IMenuHandler *handler,
									   unsigned int time,
									   unsigned int flags)
{
	if (IsVoteInProgress())
	{
		return false;
	}

	InternalReset();

	/* Mark all clients as not voting */
	for (int i=1; i<=gpGlobals->maxClients; i++)
	{
		m_ClientVotes[i] = VOTE_NOT_VOTING;
		m_Revoting[i] = false;
	}

	m_Items = menu->GetItemCount();

	if (m_Votes.size() < (size_t)m_Items)
	{
		/* Only clear the items we need to... */
		size_t size = m_Votes.size();
		for (size_t i=0; i<size; i++)
		{
			m_Votes[i] = 0;
		}
		m_Votes.resize(m_Items, 0);
	}
	else
	{
		for (unsigned int i=0; i<m_Items; i++)
		{
			m_Votes[i] = 0;
		}
	}

	m_pCurMenu = menu;
	m_VoteTime = time;
	m_VoteFlags = flags;
	m_pHandler = handler;

	m_pHandler->OnMenuStart(m_pCurMenu);

	return true;
}

void VoteMenuHandler::StartVoting()
{
	if (!m_pCurMenu)
	{
		return;
	}

	m_bStarted = true;

	m_pHandler->OnMenuVoteStart(m_pCurMenu);

	m_displayTimer = g_Timers.CreateTimer(this, 1.0, NULL, TIMER_FLAG_REPEAT|TIMER_FLAG_NO_MAPCHANGE);

	/* By now we know how many clients were set.  
	 * If there are none, we should end IMMEDIATELY.
	 */
	if (m_Clients == 0)
	{
		EndVoting();
	}

	m_TotalClients = m_Clients;
}

void VoteMenuHandler::DecrementPlayerCount()
{
	assert(m_Clients > 0);

	m_Clients--;

	if (m_bStarted && m_Clients == 0)
	{
		EndVoting();
	}
}

int SortVoteItems(const void *item1, const void *item2)
{
	return ((menu_vote_result_t::menu_item_vote_t *)item2)->count 
		- ((menu_vote_result_t::menu_item_vote_t *)item1)->count;
}

void VoteMenuHandler::EndVoting()
{
	/* Set when the next delay ends.  We ignore cancellation because a menu
	 * was, at one point, displayed, which is all that counts.  However, we
	 * do re-calculate the time just in case the menu had no time limit.
	 */
	float fVoteDelay = sm_vote_delay.GetFloat();
	if (fVoteDelay < 1.0)
	{
		g_next_vote = 0.0;
	}
	else
	{
		g_next_vote = gpGlobals->curtime + fVoteDelay;
	}

	if (m_displayTimer)
	{
		g_Timers.KillTimer(m_displayTimer);
	}

	if (m_bCancelled)
	{
		/* If we were cancelled, don't bother tabulating anything.
		 * Reset just in case someone tries to redraw, which means
		 * we need to save our states.
		 */
		IBaseMenu *menu = m_pCurMenu;
		IMenuHandler *handler = m_pHandler;
		InternalReset();
		handler->OnMenuVoteCancel(menu, VoteCancel_Generic);
		handler->OnMenuEnd(menu, MenuEnd_VotingCancelled);
		return;
	}

	menu_vote_result_t vote;
	menu_vote_result_t::menu_client_vote_t client_vote[256];
	menu_vote_result_t::menu_item_vote_t item_vote[256];

	memset(&vote, 0, sizeof(vote));

	/* Build the item list */
	for (unsigned int i=0; i<m_Items; i++)
	{
		if (m_Votes[i] > 0)
		{
			item_vote[vote.num_items].count = m_Votes[i];
			item_vote[vote.num_items].item = i;
			vote.num_votes += m_Votes[i];
			vote.num_items++;
		}
	}
	vote.item_list = item_vote;

	if (!vote.num_votes)
	{
		IBaseMenu *menu = m_pCurMenu;
		IMenuHandler *handler = m_pHandler;
		InternalReset();
		handler->OnMenuVoteCancel(menu, VoteCancel_NoVotes);
		handler->OnMenuEnd(menu, MenuEnd_VotingCancelled);
		return;
	}

	/* Build the client list */
	for (int i=1; i<=gpGlobals->maxClients; i++)
	{
		if (m_ClientVotes[i] >= VOTE_PENDING)
		{
			client_vote[vote.num_clients].client = i;
			client_vote[vote.num_clients].item = m_ClientVotes[i];
			vote.num_clients++;
		}
	}
	vote.client_list = client_vote;

	/* Sort the item list descending like we promised */
	qsort(item_vote,
		vote.num_items,
		sizeof(menu_vote_result_t::menu_item_vote_t),
		SortVoteItems);

	/* Save states, then clear what we've saved.
	 * This makes us re-entrant, which is always the safe way to go.
	 */
	IBaseMenu *menu = m_pCurMenu;
	IMenuHandler *handler = m_pHandler;
	InternalReset();

	/* Send vote info */
	handler->OnMenuVoteResults(menu, &vote);
	handler->OnMenuEnd(menu, MenuEnd_VotingDone);
}

void VoteMenuHandler::OnMenuStart(IBaseMenu *menu)
{
	m_Clients++;
}

void VoteMenuHandler::OnMenuEnd(IBaseMenu *menu, MenuEndReason reason)
{
	DecrementPlayerCount();
}

void VoteMenuHandler::OnMenuCancel(IBaseMenu *menu, int client, MenuCancelReason reason)
{
	m_pHandler->OnMenuCancel(menu, client, reason);
}

void VoteMenuHandler::OnMenuDisplay(IBaseMenu *menu, int client, IMenuPanel *display)
{
	m_ClientVotes[client] = VOTE_PENDING;
	m_pHandler->OnMenuDisplay(menu, client, display);
}

unsigned int VoteMenuHandler::OnMenuDisplayItem(IBaseMenu *menu, int client, IMenuPanel *panel, unsigned int item, const ItemDrawInfo &dr)
{
	return m_pHandler->OnMenuDisplayItem(menu, client, panel, item, dr);
}

void VoteMenuHandler::OnMenuDrawItem(IBaseMenu *menu, int client, unsigned int item, unsigned int &style)
{
	m_pHandler->OnMenuDrawItem(menu, client, item, style);
}

void VoteMenuHandler::OnMenuSelect(IBaseMenu *menu, int client, unsigned int item)
{
	/* Check by our item count, NOT the vote array size */
	if (item < m_Items)
	{
		unsigned int index = menu->GetRealItemIndex(client, item);
		m_ClientVotes[client] = index;
		m_Votes[index]++;
		m_NumVotes++;

		if (sm_vote_chat.GetBool() || sm_vote_console.GetBool() || sm_vote_client_console.GetBool())
		{
			static char buffer[1024];
			ItemDrawInfo dr;
			menu->GetItemInfo(item, &dr, client);

			if (sm_vote_console.GetBool())
			{
				int target = SOURCEMOD_SERVER_LANGUAGE;
				logicore.CoreTranslate(buffer, sizeof(buffer), "[SM] %T", 4, NULL, "Voted For",
				                       &target, g_Players.GetPlayerByIndex(client)->GetName(), dr.display);
				Engine_LogPrintWrapper(buffer);
			}
			
			if (sm_vote_chat.GetBool() || sm_vote_client_console.GetBool())
			{
				int maxclients = g_Players.GetMaxClients();
				for (int i=1; i<=maxclients; i++)
				{	
					CPlayer *pPlayer = g_Players.GetPlayerByIndex(i);
					assert(pPlayer);

					if (pPlayer->IsInGame() && !pPlayer->IsFakeClient())
					{
						if (m_Revoting[client])
						{
							logicore.CoreTranslate(buffer, sizeof(buffer), "[SM] %T", 4, NULL, "Changed Vote",
							                       &i, g_Players.GetPlayerByIndex(client)->GetName(), dr.display);
						}
						else
						{
							logicore.CoreTranslate(buffer, sizeof(buffer), "[SM] %T", 4, NULL, "Voted For",
							                       &i, g_Players.GetPlayerByIndex(client)->GetName(), dr.display);
						}

						if (sm_vote_chat.GetBool())
						{
							g_HL2.TextMsg(i, HUD_PRINTTALK, buffer);
						}

						if (sm_vote_client_console.GetBool())
						{
							ClientConsolePrint(pPlayer->GetEdict(), buffer);
						}		
					}
				}
			}
		}

		BuildVoteLeaders();
		DrawHintProgress();
	}

	m_pHandler->OnMenuSelect(menu, client, item);
}

void VoteMenuHandler::OnMenuSelect2(IBaseMenu *menu, int client, unsigned int item, unsigned int item_on_page)
{
	if (m_pHandler->GetMenuAPIVersion2() >= 13)
	{
		m_pHandler->OnMenuSelect2(menu, client, item, item_on_page);
	}
}

void VoteMenuHandler::InternalReset()
{
	m_Clients = 0;
	m_Items = 0;
	m_bStarted = false;
	m_pCurMenu = NULL;
	m_NumVotes = 0;
	m_bCancelled = false;
	m_pHandler = NULL;
	m_leaderList[0] = '\0';
	m_displayTimer = NULL;
	m_TotalClients = 0;
}

void VoteMenuHandler::CancelVoting()
{
	if (m_bCancelled || !m_pCurMenu)
	{
		return;
	}
	m_bCancelled = true;
	m_pCurMenu->Cancel();
}

IBaseMenu *VoteMenuHandler::GetCurrentMenu()
{
	return m_pCurMenu;
}

bool VoteMenuHandler::IsCancelling()
{
	return m_bCancelled;
}

void VoteMenuHandler::DrawHintProgress()
{
	if (!sm_vote_hintbox.GetBool())
	{
		return;
	}

	static char buffer[1024];

	float timeRemaining = (m_fStartTime + m_nMenuTime) - gpGlobals->curtime;
	if (timeRemaining < 0)
	{
		timeRemaining = 0.0;
	}

	int iTimeRemaining = RoundFloatToInt(timeRemaining);
	
	int maxclients = g_Players.GetMaxClients();
	for (int i=1; i<=maxclients; i++)
	{
		if (g_Players.GetPlayerByIndex(i)->IsInGame())
		{
			logicore.CoreTranslate(buffer, sizeof(buffer), "%T%s", 6, NULL, "Vote Count",
			                       &i, &m_NumVotes, &m_TotalClients, &iTimeRemaining, &m_leaderList);
			g_HL2.HintTextMsg(i, buffer);
		}
	}
}

void VoteMenuHandler::BuildVoteLeaders()
{
	if (m_NumVotes == 0 || !sm_vote_hintbox.GetBool())
	{
		return;
	}

	menu_vote_result_t vote;
	menu_vote_result_t::menu_item_vote_t item_vote[256];

	memset(&vote, 0, sizeof(vote));

	/* Build the item list */
	for (unsigned int i=0; i<m_Items; i++)
	{
		if (m_Votes[i] > 0)
		{
			item_vote[vote.num_items].count = m_Votes[i];
			item_vote[vote.num_items].item = i;
			vote.num_votes += m_Votes[i];
			vote.num_items++;
		}
	}
	vote.item_list = item_vote;
	assert(vote.num_votes);

	/* Sort the item list descending */
	qsort(item_vote,
		vote.num_items,
		sizeof(menu_vote_result_t::menu_item_vote_t),
		SortVoteItems);

	/* Take the top 3 (if applicable) and draw them */
	int len = 0;
	for (unsigned int i=0; i<vote.num_items && i<3; i++)
	{
		int curItem = vote.item_list[i].item;
		ItemDrawInfo dr;
		m_pCurMenu->GetItemInfo(curItem, &dr);
		len += g_SourceMod.Format(m_leaderList + len, sizeof(m_leaderList) - len, "\n%i. %s: (%i)", i+1, dr.display, vote.item_list[i].count);
	}
}

SourceMod::ResultType VoteMenuHandler::OnTimer(ITimer *pTimer, void *pData)
{
	DrawHintProgress();

	return Pl_Continue;
}

void VoteMenuHandler::OnTimerEnd(ITimer *pTimer, void *pData)
{
	m_displayTimer = NULL;
}
