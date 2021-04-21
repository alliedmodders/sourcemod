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

#if defined _sdktools_stocks_included
 #endinput
#endif
#define _sdktools_stocks_included

/**
 * Given a partial team name, attempts to find a matching team.
 *
 * The search is performed case insensitively and only against the 
 * first N characters of the team names, where N is the number of 
 * characters in the search pattern.
 *
 * @param name          Partial or full team name.  
 * @return              A valid team index on success.
 *                      -1 if no team matched.
 *                      -2 if more than one team matched.
 */
stock int FindTeamByName(const char[] name)
{
	int name_len = strlen(name);
	int num_teams = GetTeamCount();
	char team_name[32];
	int found_team = -1;

	for (int i = 0; i < num_teams; i++)
	{
		GetTeamName(i, team_name, sizeof(team_name));

		if (strncmp(team_name, name, name_len, false) == 0)
		{
			if (found_team >= 0)
			{
				return -2;
			}
			else
			{
				found_team = i;
			}
		}
	}

	return found_team;
}
