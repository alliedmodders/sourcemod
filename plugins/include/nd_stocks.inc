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
enum NDTeam 
{
	NDTeam_Unassigned = 0,
    	NDTeam_Spec,
    	NDTeam_Consort,
    	NDTeam_Empire
};

/**
 * Gets the team of a given player
 *
 * @param client		Client index.
 * @return			The client's team as a NDTeam enum
 * @error			Invalid client index or client not ingame.
 */

stock NDTeam GetClientNDTeam(int client) { return view_as<NDTeam>(GetClientTeam(client)); }

/**
 * Gets the name of a given team index
 *
 * @param team			Team index.
 * @return			The name of the team or "Unknown" on invalid team index
 * @error			Invalid team index.
 */

stock String:ND_GetTeamName(int team)
{
	new String:TeamName[12];
	
	switch(team)
	{
		case NDTeam_Empire: 	TeamName = "Empire";
		case NDTeam_Consort: 	TeamName = "Consortium";
		case NDTeam_Spec:	TeamName = "Spectate";
		case NDTeam_Unassigned:	TeamName = "Unassigned";
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
 
stock int ND_GetPlayerRank(int client)
{
	return GetEntProp(GetPlayerResourceEntity(), Prop_Send, "m_iPlayerRank", 1, client);
}

/** 
 * Retreives the current score of a given client index
 *
 * @param client		Client index.
 * @return			The current score of the client
 */

stock int ND_GetPlayerScore(int client)
{
	return GetEntProp(GetPlayerResourceEntity(), Prop_Send, "m_iScore", 1, client);
}

/* Player Class Functions */

//main player class enum
enum ND_MainClasses
{
	ND_MainClass_Assault = 0,
	ND_MainClass_Exo,
	ND_MainClass_Stealth,
	ND_MainClass_Support
};

/* Sub Classes */
#define ND_ASSAULT_CLASS_INFANTRY 	0
#define ND_ASSAULT_CLASS_GRENDIER 	1
#define ND_ASSAULT_CLASS_SNIPER 	2

#define ND_EXO_CLASS_SUPRESSION		0
#define ND_EXO_CLASS_SEIGEKIT 		1

#define ND_STEALTH_CLASS_ASSASSIN	0
#define ND_STEALTH_CLASS_SNIPER 	1
#define ND_STEALTH_CLASS_SABATUER 	2

#define ND_SUPPORT_CLASS_MEDIC 		0
#define ND_SUPPORT_CLASS_ENGINEER 	1
#define ND_SUPPORT_CLASS_BBQ	 	2

/* Gizmo(s) */
#define ND_GIZMO_NONE 			0

/** 
 * Sets the class of a client index
 *
 * @param client		Client index.
 * @parm2 MainClass		The main class to set, see mainclass enum
 * @parm3 SubClass		The sub class to set, see subclass consants
 * @parm4 Gizmo			The gizmo to change to
 */

stock void ND_SetPlayerClass(int client, ND_MainClasses MainClass, int SubClass, int gizmo = ND_GIZMO_NONE)
{
	SetEntProp(client, Prop_Send, "m_iPlayerClass", view_as<int>MainClass);
    	SetEntProp(client, Prop_Send, "m_iPlayerSubclass", SubClass);
	SetEntProp(client, Prop_Send, "m_iDesiredPlayerClass", view_as<int>MainClass);
	SetEntProp(client, Prop_Send, "m_iDesiredPlayerSubclass", SubClass);
	SetEntProp(client, Prop_Send, "m_iDesiredGizmo", gizmo);
}

/** 
 * Get the main class of a client index
 *
 * @param client		Client index.
 * @return			The main class of the client, see mainclass enum
 */

stock ND_MainClasses ND_GetPlayerMainClass(int client)
{
	return view_as<ND_MainClasses>(GetEntProp(client, Prop_Send, "m_iPlayerClass"));
}

/** 
 * Get the sub class of a client index
 *
 * @param client		Client index.
 * @return			The sub class of the client, see subclass consants
 */

stock int ND_GetPlayerSubClass(int client)
{
	return GetEntProp(client, Prop_Send, "m_iPlayerSubclass");
}


/* Commander Functions */

/** 
 * Get wetheir or not a client is commander
 *
 * @param client		Client index.
 * @return			True if commander otherwise false
 * @error			Invalid client index or client not ingame
 */
stock bool ND_IsCommander(client)
{
	return ND_GetTeamCommander(GetClientNDTeam(client)) == client;
}

/** 
 * Get the commander belonging to a particular team
 *
 * @param team			NDTeam enum.
 * @return			The client index
 * @error			Invalid team or -1 if no commander
 */
stock int ND_GetTeamCommander(NDTeam team)
{
	return GameRules_GetPropEnt("m_hCommanders", view_as<int>team - 2);
}

/** 
 * Demote a commander on a specified team
 *
 * @param team			NDTeam enum.
 * @error			Invalid team or no commander
 */

stock void ND_DemoteTeamCommander(NDTeam team)
{
	int commander = ND_GetTeamCommander(team);
	
	FakeClientCommand(commander, "startmutiny");
	FakeClientCommand(commander, "rtsview");
}


/* Structure Functions */

enum NDStructFlags
{
	NDFLAG_EXPLODE_ON_REMOVE= (1 << 1),
	NDFLAG_CANT_SELL= (1 << 2),
	NDFLAG_ALWAYS_OPERATIONAL= (1 << 3),
	NDFLAG_POWER_CONNECTED= (1 << 4),
	NDFLAG_HAS_FULL_POWER= (1 << 5),
	NDFLAG_REMOVED= (1 << 6)
};

/** 
 * Get the properties which belong to a structure
 *
 * @param entity		Entity index of the structure.
 * @return			The flags that structure has
 */

stock ND_GetStructureFlags(int entity)
{
	return GetEntProp(entity, Prop_Send, "m_iStructFlags");
}

/** 
 * Give a structure a specific property
 *
 * @param entity		Entity index of the structure.
 * @parm2 flag			Flag to give the structure, see enum above
 */

stock void ND_AddStructureFlag(int entity, NDStructFlags flag)
{
	SetEntProp(entity, Prop_Send, "m_iStructFlags", ND_GetStructureFlags(entity) | view_as<int>flag);
}

/** 
 * Remove specific property from a structure
 *
 * @param entity		Entity index of the structure.
 * @parm2 flag			Flag to remove from structure, see enum above
 */

stock void ND_RemoveStructureFlag(int entity, NDStructFlags flag)
{
	SetEntProp(entity, Prop_Send, "m_iStructFlags", ND_GetStructureFlags(entity) &~ view_as<int>flag);
}
