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

#include <string.h>
#include <stdlib.h>
#include "MenuVoting.h"
#include "PlayerManager.h"
#include "sourcemm_api.h"

float g_next_vote = 0.0f;

#if defined ORANGEBOX_BUILD
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

#if defined ORANGEBOX_BUILD
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

	/* Wipe out their vote if they had one */
	int item;
	if ((item = m_ClientVotes[client]) >= 0)
	{
		assert((unsigned)item < m_Items);
		assert(m_Votes[item] > 0);
		m_Votes[item]--;
		m_ClientVotes[client] = -1;
	}
}

bool VoteMenuHandler::IsVoteInProgress()
{
	return (m_pCurMenu != NULL);
}

bool VoteMenuHandler::StartVote(IBaseMenu *menu, unsigned int num_clients, int clients[], unsigned int max_time, unsigned int flags/* =0 */)
{
	if (!InitializeVoting(menu, menu->GetHandler(), max_time, flags))
	{
		return false;
	}

	float fVoteDelay = sm_vote_delay.GetFloat();
	if (fVoteDelay < 1.0)
	{
		g_next_vote = 0.0;
	} else {
		/* This little trick breaks for infinite votes!
		 * However, we just ignore that since those 1) shouldn't exist and 
		 * 2) people must be checking IsVoteInProgress() beforehand anyway.
		 */
		g_next_vote = gpGlobals->curtime + fVoteDelay + (float)max_time;
	}

	for (unsigned int i=0; i<num_clients; i++)
	{
		menu->Display(clients[i], max_time, this);
	}

	StartVoting();

	return true;
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
		m_ClientVotes[i] = -2;
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
	} else {
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

	/* By now we know how many clients were set.  
	 * If there are none, we should end IMMEDIATELY.
	 */
	if (m_Clients == 0)
	{
		EndVoting();
	}
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
	} else {
		g_next_vote = gpGlobals->curtime + fVoteDelay;
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
		if (m_ClientVotes[i] >= -1)
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
	m_ClientVotes[client] = -1;
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
		m_ClientVotes[client] = item;
		m_Votes[item]++;
		m_NumVotes++;
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
