/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod (C)2004-2008 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This file is part of the SourceMod/SourcePawn SDK.
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

#if defined _nd_stocks_included_
  #endinput
#endif
#define _nd_stocks_included_

/* The team name constants */
#define TEAM_UNASSIGNED		0
#define TEAM_SPEC		1
#define TEAM_CONSORT		2
#define TEAM_EMPIRE		3

/**
 * Gets the name of a given team index
 *
 * @param team			Team index.
 * @return			The name of the team or "Unknown" on invalid team index
 * @error			Invalid team index.
 */

stock String:ND_GetTeamName(team)
{
	new String:TeamName[12];
	
	switch(team)
	{
		case TEAM_EMPIRE: 	TeamName = "Empire";
		case TEAM_CONSORT: 	TeamName = "Consortium";
		case TEAM_SPEC:		TeamName = "Spectate";
		case TEAM_UNASSIGNED:	TeamName = "Unassigned";
		default: 		TeamName = "Unknown";
	}
	
	return TeamName;
}

/** 
 * Retreives the level of a given client index
 *
 * @param client		Client index.
 * @return			The level of the client or some value less than 2
 * @error			Unloaded level or level less than 2.
 */
 
stock ND_RetreiveLevel(client)
{
	return GetEntProp(GetPlayerResourceEntity(), Prop_Send, "m_iPlayerRank", 1, client);
}

/** 
 * Retreives the current score of a given client index
 *
 * @param client		Client index.
 * @return			The current score of the client
 */

stock ND_RetrieveScore(client)
{
	return GetEntProp(GetPlayerResourceEntity(), Prop_Send, "m_iScore", 1, client);
}
