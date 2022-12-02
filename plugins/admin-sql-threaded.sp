/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod SQL Admins Plugin (Threaded)
 * Fetches admins from an SQL database dynamically.
 *
 * SourceMod (C)2004-2008 AlliedModders LLC.  All rights reserved.
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

/* We like semicolons */
#pragma semicolon 1

#include <sourcemod>

#pragma newdecls required

public Plugin myinfo = 
{
	name = "SQL Admins (Threaded)",
	author = "AlliedModders LLC",
	description = "Reads admins from SQL dynamically",
	version = SOURCEMOD_VERSION,
	url = "http://www.sourcemod.net/"
};

/**
 * Notes:
 *
 * 1) All queries in here are high priority.  This is because the admin stuff 
 *    is very important.  Do not take this to mean that in your script, 
 *    everything should be high priority.  
 *
 * 2) All callbacks are locked with "sequence numbers."  This is to make sure 
 *    that multiple calls to sm_reloadadmins and the like do not make us 
 *    store the results from two or more callbacks accidentally.  Instead, we 
 *    check the sequence number in each callback with the current "allowed" 
 *    sequence number, and if it doesn't match, the callback is cancelled.
 *
 * 3) Sequence numbers for groups and overrides are not cleared unless there 
 *    was a 100% success in the fetch.  This is so we can potentially implement 
 *    connection retries in the future.
 *
 * 4) Sequence numbers for the user cache are ignored except for being 
 *    non-zero, which means players in-game should be re-checked for admin 
 *    powers.
 */

Database hDatabase = null;						/** Database connection */
int g_sequence = 0;								/** Global unique sequence number */
int ConnectLock = 0;							/** Connect sequence number */
int RebuildCachePart[3] = {0};					/** Cache part sequence numbers */

enum struct PlayerInfo {
	int sequencenum; /** Player-specific sequence numbers */
	bool authed; /** Whether a player has been "pre-authed" */
}

PlayerInfo playerinfo[MAXPLAYERS+1];

//#define _DEBUG

public void OnMapEnd()
{
	/**
	 * Clean up on map end just so we can start a fresh connection when we need it later.
	 */
	delete hDatabase;
}

public bool OnClientConnect(int client, char[] rejectmsg, int maxlen)
{
	playerinfo[client].sequencenum = 0;
	playerinfo[client].authed = false;
	return true;
}

public void OnClientDisconnect(int client)
{
	playerinfo[client].sequencenum = 0;
	playerinfo[client].authed = false;
}

public void OnDatabaseConnect(Database db, const char[] error, any data)
{
#if defined _DEBUG
	PrintToServer("OnDatabaseConnect(%x, %d) ConnectLock=%d", db, data, ConnectLock);
#endif

	/**
	 * If this happens to be an old connection request, ignore it.
	 */
	if (data != ConnectLock || hDatabase != null)
	{
		delete db;
		return;
	}
	
	ConnectLock = 0;
	hDatabase = db;
	
	/**
	 * See if the connection is valid.  If not, don't un-mark the caches
	 * as needing rebuilding, in case the next connection request works.
	 */
	if (hDatabase == null)
	{
		LogError("Failed to connect to database: %s", error);
		return;
	}
	
	/**
	 * See if we need to get any of the cache stuff now.
	 */
	int sequence;
	if ((sequence = RebuildCachePart[AdminCache_Overrides]) != 0)
	{
		FetchOverrides(hDatabase, sequence);
	}
	if ((sequence = RebuildCachePart[AdminCache_Groups]) != 0)
	{
		FetchGroups(hDatabase, sequence);
	}
	if ((sequence = RebuildCachePart[AdminCache_Admins]) != 0)
	{
		FetchUsersWeCan(hDatabase);
	}
}

void RequestDatabaseConnection()
{
	ConnectLock = ++g_sequence;
	if (SQL_CheckConfig("admins"))
	{
		Database.Connect(OnDatabaseConnect, "admins", ConnectLock);
	} else {
		Database.Connect(OnDatabaseConnect, "default", ConnectLock);
	}
}

public void OnRebuildAdminCache(AdminCachePart part)
{
	/**
	 * Mark this part of the cache as being rebuilt.  This is used by the 
	 * callback system to determine whether the results should still be 
	 * used.
	 */
	int sequence = ++g_sequence;
	RebuildCachePart[part] = sequence;
	
	/**
	 * If we don't have a database connection, we can't do any lookups just yet.
	 */
	if (!hDatabase)
	{
		/**
		 * Ask for a new connection if we need it.
		 */
		if (!ConnectLock)
		{
			RequestDatabaseConnection();
		}
		return;
	}
	
	if (part == AdminCache_Overrides)
	{
		FetchOverrides(hDatabase, sequence);
	} else if (part == AdminCache_Groups) {
		FetchGroups(hDatabase, sequence);
	} else if (part == AdminCache_Admins) {
		FetchUsersWeCan(hDatabase);
	}
}

public Action OnClientPreAdminCheck(int client)
{
	playerinfo[client].authed = true;
	
	/**
	 * Play nice with other plugins.  If there's no database, don't delay the 
	 * connection process.  Unfortunately, we can't attempt anything else and 
	 * we just have to hope either the database is waiting or someone will type 
	 * sm_reloadadmins.
	 */
	if (hDatabase == null)
	{
		return Plugin_Continue;
	}
	
	/**
	 * Similarly, if the cache is in the process of being rebuilt, don't delay 
	 * the user's normal connection flow.  The database will soon auth the user 
	 * normally.
	 */
	if (RebuildCachePart[AdminCache_Admins] != 0)
	{
		return Plugin_Continue;
	}
	
	/**
	 * If someone has already assigned an admin ID (bad bad bad), don't 
	 * bother waiting.
	 */
	if (GetUserAdmin(client) != INVALID_ADMIN_ID)
	{
		return Plugin_Continue;
	}
	
	FetchUser(hDatabase, client);
	
	return Plugin_Handled;
}

public void OnReceiveUserGroups(Database db, DBResultSet rs, const char[] error, any data)
{
	DataPack pk = view_as<DataPack>(data);
	pk.Reset();
	
	int client = pk.ReadCell();
	int sequence = pk.ReadCell();
	
	/**
	 * Make sure it's the same client.
	 */
	if (playerinfo[client].sequencenum != sequence)
	{
		delete pk;
		return;
	}
	
	AdminId adm = view_as<AdminId>(pk.ReadCell());
	
	/**
	 * Someone could have sneakily changed the admin id while we waited.
	 */
	if (GetUserAdmin(client) != adm)
	{
		NotifyPostAdminCheck(client);
		delete pk;
		return;
	}
	
	/**
	 * See if we got results.
	 */
	if (rs == null)
	{
		char query[255];
		pk.ReadString(query, sizeof(query));
		LogError("SQL error receiving user: %s", error);
		LogError("Query dump: %s", query);
		NotifyPostAdminCheck(client);
		delete pk;
		return;
	}
	
	char name[80];
	GroupId grp;
	
	while (rs.FetchRow())
	{
		rs.FetchString(0, name, sizeof(name));
		
		if ((grp = FindAdmGroup(name)) == INVALID_GROUP_ID)
		{
			continue;
		}
		
#if defined _DEBUG
		PrintToServer("Binding user group (%d, %d, %d, %s, %d)", client, sequence, adm, name, grp);
#endif
		
		adm.InheritGroup(grp);
	}
	
	/**
	 * We're DONE! Omg.
	 */
	NotifyPostAdminCheck(client);
	delete pk;
}

public void OnReceiveUser(Database db, DBResultSet rs, const char[] error, any data)
{
	DataPack pk = view_as<DataPack>(data);
	pk.Reset();

	int client = pk.ReadCell();
	
	/**
	 * Check if this is the latest result request.
	 */
	int sequence = pk.ReadCell();
	if (playerinfo[client].sequencenum != sequence)
	{
		/* Discard everything, since we're out of sequence. */
		delete pk;
		return;
	}
	
	/**
	 * If we need to use the results, make sure they succeeded.
	 */
	if (rs == null)
	{
		char query[255];
		pk.ReadString(query, sizeof(query));
		LogError("SQL error receiving user: %s", error);
		LogError("Query dump: %s", query);
		RunAdminCacheChecks(client);
		NotifyPostAdminCheck(client);
		delete pk;
		return;
	}
	
	int num_accounts = rs.RowCount;
	if (num_accounts == 0)
	{
		RunAdminCacheChecks(client);
		NotifyPostAdminCheck(client);
		delete pk;
		return;
	}
	
	char authtype[16];
	char identity[80];
	char password[80];
	char flags[32];
	char name[80];
	int immunity, id;
	AdminId adm;
	
	/**
	 * Cache user info -- [0] = db id, [1] = cache id, [2] = groups
	 */
	int[][] user_lookup = new int[num_accounts][3];
	int total_users = 0;
	
	while (rs.FetchRow())
	{
		id = rs.FetchInt(0);
		rs.FetchString(1, authtype, sizeof(authtype));
		rs.FetchString(2, identity, sizeof(identity));
		rs.FetchString(3, password, sizeof(password));
		rs.FetchString(4, flags, sizeof(flags));
		rs.FetchString(5, name, sizeof(name));
		immunity = rs.FetchInt(7);
		
		/* For dynamic admins we clear anything already in the cache. */
		if ((adm = FindAdminByIdentity(authtype, identity)) != INVALID_ADMIN_ID)
		{
			RemoveAdmin(adm);
		}
		
		adm = CreateAdmin(name);
		if (!adm.BindIdentity(authtype, identity))
		{
			LogError("Could not bind prefetched SQL admin (authtype \"%s\") (identity \"%s\")", authtype, identity);
			continue;
		}
		
		user_lookup[total_users][0] = id;
		user_lookup[total_users][1] = view_as<int>(adm);
		user_lookup[total_users][2] = rs.FetchInt(6);
		total_users++;
		
#if defined _DEBUG
		PrintToServer("Found SQL admin (%d,%s,%s,%s,%s,%s,%d):%d:%d", id, authtype, identity, password, flags, name, immunity, adm, user_lookup[total_users-1][2]);
#endif
		
		/* See if this admin wants a password */
		if (password[0] != '\0')
		{
			adm.SetPassword(password);
		}

		adm.ImmunityLevel = immunity;
		
		/* Apply each flag */
		int len = strlen(flags);
		AdminFlag flag;
		for (int i=0; i<len; i++)
		{
			if (!FindFlagByChar(flags[i], flag))
			{
				continue;
			}
			adm.SetFlag(flag, true);
		}
	}
	
	/**
	 * Try binding the user.
	 */	
	int group_count = 0;
	RunAdminCacheChecks(client);
	adm = GetUserAdmin(client);
	id = 0;
	
	
	for (int i=0; i<total_users; i++)
	{
		if (user_lookup[i][1] == view_as<int>(adm))
		{
			id = user_lookup[i][0];
			group_count = user_lookup[i][2];
			break;
		}
	}
	
#if defined _DEBUG
	PrintToServer("Binding client (%d, %d) resulted in: (%d, %d, %d)", client, sequence, id, adm, group_count);
#endif
	
	/**
	 * If we can't verify that we assigned a database admin, or the user has no 
	 * groups, don't bother doing anything.
	 */
	if (!id || !group_count)
	{
		NotifyPostAdminCheck(client);
		delete pk;
		return;
	}
	
	/**
	 * The user has groups -- we need to fetch them!
	 */
	char query[255];
	Format(query, sizeof(query), "SELECT g.name FROM sm_admins_groups ag JOIN sm_groups g ON ag.group_id = g.id WHERE ag.admin_id = %d", id);
	 
	pk.Reset();
	pk.WriteCell(client);
	pk.WriteCell(sequence);
	pk.WriteCell(adm);
	pk.WriteString(query);
	
	db.Query(OnReceiveUserGroups, query, pk, DBPrio_High);
}

void FetchUser(Database db, int client)
{
	char name[MAX_NAME_LENGTH];
	char safe_name[(MAX_NAME_LENGTH * 2) - 1];
	char steamid2[32];
	char steamid2alt[32];
	char steamid3[32];
	char steamid64[32];
	char ipaddr[24];
	
	/**
	 * Get authentication information from the client.
	 */
	GetClientName(client, name, sizeof(name));
	GetClientIP(client, ipaddr, sizeof(ipaddr));
	
	steamid2[0] = '\0';
	if (GetClientAuthId(client, AuthId_Steam2, steamid2, sizeof(steamid2)))
	{
		if (StrEqual(steamid2, "STEAM_ID_LAN"))
		{
			steamid2[0] = '\0';
		}
	}

	steamid3[0] = '\0';
	if (GetClientAuthId(client, AuthId_Steam3, steamid3, sizeof(steamid3)))
	{
		if (StrEqual(steamid3, "STEAM_ID_LAN"))
		{
			steamid3[0] = '\0';
		}
	}

	steamid64[0] = '\0';
	if (GetClientAuthId(client, AuthId_SteamID64, steamid64, sizeof(steamid64)))
	{
		if (StrEqual(steamid64, "0"))
		{
			steamid64[0] = '\0';
		}
	}
	
	db.Escape(name, safe_name, sizeof(safe_name));
	
	/**
	 * Construct the query using the information the user gave us.
	 */
	char query[512];
	int len = 0;
	
	len += Format(query[len], sizeof(query)-len, "SELECT a.id, a.authtype, a.identity, a.password, a.flags, a.name, COUNT(ag.group_id), immunity");
	len += Format(query[len], sizeof(query)-len, " FROM sm_admins a LEFT JOIN sm_admins_groups ag ON a.id = ag.admin_id WHERE ");
	len += Format(query[len], sizeof(query)-len, " (a.authtype = 'ip' AND a.identity = '%s')", ipaddr);
	len += Format(query[len], sizeof(query)-len, " OR (a.authtype = 'name' AND a.identity = '%s')", safe_name);
	if (steamid2[0] != '\0' && steamid3[0] != '\0' && steamid64[0] != '\0')
	{
		strcopy(steamid2alt, sizeof(steamid2alt), steamid2);
		steamid2alt[6] = (steamid2[6] == '0') ? '1' : '0';

		len += Format(query[len], sizeof(query)-len,
			" OR (a.authtype = 'steam' AND (a.identity = '%s' OR a.identity = '%s' OR a.identity = '%s' OR a.identity = '%s'))",
			steamid2, steamid2alt, steamid3, steamid64);
	}
	len += Format(query[len], sizeof(query)-len, " GROUP BY a.id");
	
	/**
	 * Send the actual query.
	 */	
	playerinfo[client].sequencenum = ++g_sequence;
	
	DataPack pk = new DataPack();
	pk.WriteCell(client);
	pk.WriteCell(playerinfo[client].sequencenum);
	pk.WriteString(query);
	
#if defined _DEBUG
	PrintToServer("Sending user query: %s", query);
#endif
	
	db.Query(OnReceiveUser, query, pk, DBPrio_High);
}

void FetchUsersWeCan(Database db)
{
	for (int i=1; i<=MaxClients; i++)
	{
		if (playerinfo[i].authed && GetUserAdmin(i) == INVALID_ADMIN_ID)
		{
			FetchUser(db, i);
		}
	}
	
	/**
	 * This round of updates is done.  Go in peace.
	 */
	RebuildCachePart[AdminCache_Admins] = 0;
}

public void OnReceiveGroupImmunity(Database db, DBResultSet rs, const char[] error, any data)
{
	DataPack pk = view_as<DataPack>(data);
	pk.Reset();
	
	/**
	 * Check if this is the latest result request.
	 */
	int sequence = pk.ReadCell();
	if (RebuildCachePart[AdminCache_Groups] != sequence)
	{
		/* Discard everything, since we're out of sequence. */
		delete pk;
		return;
	}
	
	/**
	 * If we need to use the results, make sure they succeeded.
	 */
	if (rs == null)
	{
		char query[255];
		pk.ReadString(query, sizeof(query));		
		LogError("SQL error receiving group immunity: %s", error);
		LogError("Query dump: %s", query);
		delete pk;	
		return;
	}
	
	/* We're done with the pack forever. */
	delete pk;
	
	while (rs.FetchRow())
	{
		char group1[80];
		char group2[80];
		GroupId grp, other;
		
		rs.FetchString(0, group1, sizeof(group1));
		rs.FetchString(1, group2, sizeof(group2));
		
		if (((grp = FindAdmGroup(group1)) == INVALID_GROUP_ID)
			|| (other = FindAdmGroup(group2)) == INVALID_GROUP_ID)
		{
			continue;
		}
		
		grp.AddGroupImmunity(other);
#if defined _DEBUG
		PrintToServer("SetAdmGroupImmuneFrom(%d, %d)", grp, other);
#endif
	}
	
	/* Clear the sequence so another connect doesn't refetch */
	RebuildCachePart[AdminCache_Groups] = 0;
}

public void OnReceiveGroupOverrides(Database db, DBResultSet rs, const char[] error, any data)
{
	DataPack pk = view_as<DataPack>(data);
	pk.Reset();
	
	/**
	 * Check if this is the latest result request.
	 */
	int sequence = pk.ReadCell();
	if (RebuildCachePart[AdminCache_Groups] != sequence)
	{
		/* Discard everything, since we're out of sequence. */
		delete pk;
		return;
	}
	
	/**
	 * If we need to use the results, make sure they succeeded.
	 */
	if (rs == null)
	{
		char query[255];
		pk.ReadString(query, sizeof(query));		
		LogError("SQL error receiving group overrides: %s", error);
		LogError("Query dump: %s", query);
		delete pk;	
		return;
	}
	
	/**
	 * Fetch the overrides.
	 */
	char name[80];
	char type[16];
	char command[64];
	char access[16];
	GroupId grp;
	while (rs.FetchRow())
	{
		rs.FetchString(0, name, sizeof(name));
		rs.FetchString(1, type, sizeof(type));
		rs.FetchString(2, command, sizeof(command));
		rs.FetchString(3, access, sizeof(access));
		
		/* Find the group.  This is actually faster than doing the ID lookup. */
		if ((grp = FindAdmGroup(name)) == INVALID_GROUP_ID)
		{
			/* Oh well, just ignore it. */
			continue;
		}
		
		OverrideType o_type = Override_Command;
		if (StrEqual(type, "group"))
		{
			o_type = Override_CommandGroup;
		}
				
		OverrideRule o_rule = Command_Deny;
		if (StrEqual(access, "allow"))
		{
			o_rule = Command_Allow;
		}
				
#if defined _DEBUG
		PrintToServer("AddAdmGroupCmdOverride(%d, %s, %d, %d)", grp, command, o_type, o_rule);
#endif
				
		grp.AddCommandOverride(command, o_type, o_rule);
	}
	
	/**
	 * It's time to get the group immunity list.
	 */
	int len = 0;
	char query[256];
	len += Format(query[len], sizeof(query)-len, "SELECT g1.name, g2.name FROM sm_group_immunity gi");
	len += Format(query[len], sizeof(query)-len, " LEFT JOIN sm_groups g1 ON g1.id = gi.group_id ");
	len += Format(query[len], sizeof(query)-len, " LEFT JOIN sm_groups g2 ON g2.id = gi.other_id");

	pk.Reset();
	pk.WriteCell(sequence);
	pk.WriteString(query);
	
	db.Query(OnReceiveGroupImmunity, query, pk, DBPrio_High);
}

public void OnReceiveGroups(Database db, DBResultSet rs, const char[] error, any data)
{
	DataPack pk = view_as<DataPack>(data);
	pk.Reset();
	
	/**
	 * Check if this is the latest result request.
	 */
	int sequence = pk.ReadCell();
	if (RebuildCachePart[AdminCache_Groups] != sequence)
	{
		/* Discard everything, since we're out of sequence. */
		delete pk;
		return;
	}
	
	/**
	 * If we need to use the results, make sure they succeeded.
	 */
	if (rs == null)
	{
		char query[255];
		pk.ReadString(query, sizeof(query));
		LogError("SQL error receiving groups: %s", error);
		LogError("Query dump: %s", query);
		delete pk;
		return;
	}
	
	/**
	 * Now start fetching groups.
	 */
	char flags[32];
	char name[128];
	int immunity;
	while (rs.FetchRow())
	{
		rs.FetchString(0, flags, sizeof(flags));
		rs.FetchString(1, name, sizeof(name));
		immunity = rs.FetchInt(2);
		
#if defined _DEBUG
		PrintToServer("Adding group (%d, %s, %s)", immunity, flags, name);
#endif
		
		/* Find or create the group */
		GroupId grp;
		if ((grp = FindAdmGroup(name)) == INVALID_GROUP_ID)
		{
			grp = CreateAdmGroup(name);
		}

		/* Add flags from the database to the group */
		int num_flag_chars = strlen(flags);
		for (int i=0; i<num_flag_chars; i++)
		{
			AdminFlag flag;
			if (!FindFlagByChar(flags[i], flag))
			{
				continue;
			}
			grp.SetFlag(flag, true);
		}
		
		grp.ImmunityLevel = immunity;
	}
	
	/**
	 * It's time to get the group override list.
	 */
	char query[255];
	Format(query, 
		sizeof(query), 
		"SELECT g.name, og.type, og.name, og.access FROM sm_group_overrides og JOIN sm_groups g ON og.group_id = g.id ORDER BY g.id DESC");

	pk.Reset();
	pk.WriteCell(sequence);
	pk.WriteString(query);
	
	db.Query(OnReceiveGroupOverrides, query, pk, DBPrio_High);
}

void FetchGroups(Database db, int sequence)
{
	char query[255];
	
	Format(query, sizeof(query), "SELECT flags, name, immunity_level FROM sm_groups");

	DataPack pk = new DataPack();
	pk.WriteCell(sequence);
	pk.WriteString(query);
	
	db.Query(OnReceiveGroups, query, pk, DBPrio_High);
}

public void OnReceiveOverrides(Database db, DBResultSet rs, const char[] error, any data)
{
	DataPack pk = view_as<DataPack>(data);
	pk.Reset();
	
	/**
	 * Check if this is the latest result request.
	 */
	int sequence = pk.ReadCell();
	if (RebuildCachePart[AdminCache_Overrides] != sequence)
	{
		/* Discard everything, since we're out of sequence. */
		delete pk;
		return;
	}
	
	/**
	 * If we need to use the results, make sure they succeeded.
	 */
	if (rs == null)
	{
		char query[255];
		pk.ReadString(query, sizeof(query));
		LogError("SQL error receiving overrides: %s", error);
		LogError("Query dump: %s", query);
		delete pk;
		return;
	}
	
	/**
	 * We're done with you, now.
	 */
	delete pk;
	
	char type[64];
	char name[64];
	char flags[32];
	int flag_bits;
	while (rs.FetchRow())
	{
		rs.FetchString(0, type, sizeof(type));
		rs.FetchString(1, name, sizeof(name));
		rs.FetchString(2, flags, sizeof(flags));
		
#if defined _DEBUG
		PrintToServer("Adding override (%s, %s, %s)", type, name, flags);
#endif
		
		flag_bits = ReadFlagString(flags);
		if (StrEqual(type, "command"))
		{
			AddCommandOverride(name, Override_Command, flag_bits);
		} else if (StrEqual(type, "group")) {
			AddCommandOverride(name, Override_CommandGroup, flag_bits);
		}
	}
	
	/* Clear the sequence so another connect doesn't refetch */
	RebuildCachePart[AdminCache_Overrides] = 0;
}

void FetchOverrides(Database db, int sequence)
{
	char query[255];
	
	Format(query, sizeof(query), "SELECT type, name, flags FROM sm_overrides");

	DataPack pk = new DataPack();
	pk.WriteCell(sequence);
	pk.WriteString(query);
	
	db.Query(OnReceiveOverrides, query, pk, DBPrio_High);
}
