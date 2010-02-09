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

public Plugin:myinfo = 
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

new Handle:hDatabase = INVALID_HANDLE;			/** Database connection */
new g_sequence = 0;								/** Global unique sequence number */
new ConnectLock = 0;							/** Connect sequence number */
new RebuildCachePart[3] = {0};					/** Cache part sequence numbers */
new PlayerSeq[MAXPLAYERS+1];					/** Player-specific sequence numbers */
new bool:PlayerAuth[MAXPLAYERS+1];				/** Whether a player has been "pre-authed" */

//#define _DEBUG

public OnMapEnd()
{
	/**
	 * Clean up on map end just so we can start a fresh connection when we need it later.
	 */
	if (hDatabase != INVALID_HANDLE)
	{
		CloseHandle(hDatabase);
		hDatabase = INVALID_HANDLE;
	}
}

public bool:OnClientConnect(client, String:rejectmsg[], maxlen)
{
	PlayerSeq[client] = 0;
	PlayerAuth[client] = false;
	return true;
}

public OnClientDisconnect(client)
{
	PlayerSeq[client] = 0;
	PlayerAuth[client] = false;
}

public OnDatabaseConnect(Handle:owner, Handle:hndl, const String:error[], any:data)
{
#if defined _DEBUG
	PrintToServer("OnDatabaseConnect(%x,%x,%d) ConnectLock=%d", owner, hndl, data, ConnectLock);
#endif

	/**
	 * If this happens to be an old connection request, ignore it.
	 */
	if (data != ConnectLock || hDatabase != INVALID_HANDLE)
	{
		if (hndl != INVALID_HANDLE)
		{
			CloseHandle(hndl);
		}
		return;
	}
	
	ConnectLock = 0;
	hDatabase = hndl;
	
	/**
	 * See if the connection is valid.  If not, don't un-mark the caches
	 * as needing rebuilding, in case the next connection request works.
	 */
	if (hDatabase == INVALID_HANDLE)
	{
		LogError("Failed to connect to database: %s", error);
		return;
	}
	
	/**
	 * See if we need to get any of the cache stuff now.
	 */
	new sequence;
	if ((sequence = RebuildCachePart[_:AdminCache_Overrides]) != 0)
	{
		FetchOverrides(hDatabase, sequence);
	}
	if ((sequence = RebuildCachePart[_:AdminCache_Groups]) != 0)
	{
		FetchGroups(hDatabase, sequence);
	}
	if ((sequence = RebuildCachePart[_:AdminCache_Admins]) != 0)
	{
		FetchUsersWeCan(hDatabase);
	}
}

RequestDatabaseConnection()
{
	ConnectLock = ++g_sequence;
	if (SQL_CheckConfig("admins"))
	{
		SQL_TConnect(OnDatabaseConnect, "admins", ConnectLock);
	} else {
		SQL_TConnect(OnDatabaseConnect, "default", ConnectLock);
	}
}

public OnRebuildAdminCache(AdminCachePart:part)
{
	/**
	 * Mark this part of the cache as being rebuilt.  This is used by the 
	 * callback system to determine whether the results should still be 
	 * used.
	 */
	new sequence = ++g_sequence;
	RebuildCachePart[_:part] = sequence;
	
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

public Action:OnClientPreAdminCheck(client)
{
	PlayerAuth[client] = true;
	
	/**
	 * Play nice with other plugins.  If there's no database, don't delay the 
	 * connection process.  Unfortunately, we can't attempt anything else and 
	 * we just have to hope either the database is waiting or someone will type 
	 * sm_reloadadmins.
	 */
	if (hDatabase == INVALID_HANDLE)
	{
		return Plugin_Continue;
	}
	
	/**
	 * Similarly, if the cache is in the process of being rebuilt, don't delay 
	 * the user's normal connection flow.  The database will soon auth the user 
	 * normally.
	 */
	if (RebuildCachePart[_:AdminCache_Admins] != 0)
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

public OnReceiveUserGroups(Handle:owner, Handle:hndl, const String:error[], any:data)
{
	new Handle:pk = Handle:data;
	ResetPack(pk);
	
	new client = ReadPackCell(pk);
	new sequence = ReadPackCell(pk);
	
	/**
	 * Make sure it's the same client.
	 */
	if (PlayerSeq[client] != sequence)
	{
		CloseHandle(pk);
		return;
	}
	
	new AdminId:adm = AdminId:ReadPackCell(pk);
	
	/**
	 * Someone could have sneakily changed the admin id while we waited.
	 */
	if (GetUserAdmin(client) != adm)
	{
		NotifyPostAdminCheck(client);
		CloseHandle(pk);
		return;
	}
	
	/**
	 * See if we got results.
	 */
	if (hndl == INVALID_HANDLE)
	{
		decl String:query[255];
		ReadPackString(pk, query, sizeof(query));
		LogError("SQL error receiving user: %s", error);
		LogError("Query dump: %s", query);
		NotifyPostAdminCheck(client);
		CloseHandle(pk);
		return;
	}
	
	decl String:name[80];
	new GroupId:gid;
	
	while (SQL_FetchRow(hndl))
	{
		SQL_FetchString(hndl, 0, name, sizeof(name));
		
		if ((gid = FindAdmGroup(name)) == INVALID_GROUP_ID)
		{
			continue;
		}
		
#if defined _DEBUG
		PrintToServer("Binding user group (%d, %d, %d, %s, %d)", client, sequence, adm, name, gid);
#endif
		
		AdminInheritGroup(adm, gid);
	}
	
	/**
	 * We're DONE! Omg.
	 */
	NotifyPostAdminCheck(client);
	CloseHandle(pk);
}

public OnReceiveUser(Handle:owner, Handle:hndl, const String:error[], any:data)
{
	new Handle:pk = Handle:data;
	ResetPack(pk);
	
	new client = ReadPackCell(pk);
	
	/**
	 * Check if this is the latest result request.
	 */
	new sequence = ReadPackCell(pk);
	if (PlayerSeq[client] != sequence)
	{
		/* Discard everything, since we're out of sequence. */
		CloseHandle(pk);
		return;
	}
	
	/**
	 * If we need to use the results, make sure they succeeded.
	 */
	if (hndl == INVALID_HANDLE)
	{
		decl String:query[255];
		ReadPackString(pk, query, sizeof(query));
		LogError("SQL error receiving user: %s", error);
		LogError("Query dump: %s", query);
		RunAdminCacheChecks(client);
		NotifyPostAdminCheck(client);
		CloseHandle(pk);
		return;
	}
	
	new num_accounts = SQL_GetRowCount(hndl);
	if (num_accounts == 0)
	{
		RunAdminCacheChecks(client);
		NotifyPostAdminCheck(client);
		CloseHandle(pk);
		return;
	}
	
	decl String:authtype[16];
	decl String:identity[80];
	decl String:password[80];
	decl String:flags[32];
	decl String:name[80];
	new AdminId:adm, id;
	new immunity;
	
	/**
	 * Cache user info -- [0] = db id, [1] = cache id, [2] = groups
	 */
	decl user_lookup[num_accounts][3];
	new total_users = 0;
	
	while (SQL_FetchRow(hndl))
	{
		id = SQL_FetchInt(hndl, 0);
		SQL_FetchString(hndl, 1, authtype, sizeof(authtype));
		SQL_FetchString(hndl, 2, identity, sizeof(identity));
		SQL_FetchString(hndl, 3, password, sizeof(password));
		SQL_FetchString(hndl, 4, flags, sizeof(flags));
		SQL_FetchString(hndl, 5, name, sizeof(name));
		immunity = SQL_FetchInt(hndl, 7);
		
		/* For dynamic admins we clear anything already in the cache. */
		if ((adm = FindAdminByIdentity(authtype, identity)) != INVALID_ADMIN_ID)
		{
			RemoveAdmin(adm);
		}
		
		adm = CreateAdmin(name);
		if (!BindAdminIdentity(adm, authtype, identity))
		{
			LogError("Could not bind prefetched SQL admin (authtype \"%s\") (identity \"%s\")", authtype, identity);
			continue;
		}
		
		user_lookup[total_users][0] = id;
		user_lookup[total_users][1] = _:adm;
		user_lookup[total_users][2] = SQL_FetchInt(hndl, 6);
		total_users++;
		
#if defined _DEBUG
		PrintToServer("Found SQL admin (%d,%s,%s,%s,%s,%s,%d):%d:%d", id, authtype, identity, password, flags, name, immunity, adm, user_lookup[total_users-1][2]);
#endif
		
		/* See if this admin wants a password */
		if (password[0] != '\0')
		{
			SetAdminPassword(adm, password);
		}

		SetAdminImmunityLevel(adm, immunity);
		
		/* Apply each flag */
		new len = strlen(flags);
		new AdminFlag:flag;
		for (new i=0; i<len; i++)
		{
			if (!FindFlagByChar(flags[i], flag))
			{
				continue;
			}
			SetAdminFlag(adm, flag, true);
		}
	}
	
	/**
	 * Try binding the user.
	 */	
	new group_count = 0;
	RunAdminCacheChecks(client);
	adm = GetUserAdmin(client);
	id = 0;
	
	
	for (new i=0; i<total_users; i++)
	{
		if (user_lookup[i][1] == _:adm)
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
		CloseHandle(pk);
		return;
	}
	
	/**
	 * The user has groups -- we need to fetch them!
	 */
	decl String:query[255];
	Format(query, sizeof(query), "SELECT g.name FROM sm_admins_groups ag JOIN sm_groups g ON ag.group_id = g.id WHERE ag.admin_id = %d", id);
	 
	ResetPack(pk);
	WritePackCell(pk, client);
	WritePackCell(pk, sequence);
	WritePackCell(pk, _:adm);
	WritePackString(pk, query);
	
	SQL_TQuery(owner, OnReceiveUserGroups, query, pk, DBPrio_High);
}

FetchUser(Handle:db, client)
{
	decl String:name[65];
	decl String:safe_name[140];
	decl String:steamid[32];
	decl String:steamidalt[32];
	decl String:ipaddr[24];
	
	/**
	 * Get authentication information from the client.
	 */
	GetClientName(client, name, sizeof(name));
	GetClientIP(client, ipaddr, sizeof(ipaddr));
	
	steamid[0] = '\0';
	if (GetClientAuthString(client, steamid, sizeof(steamid)))
	{
		if (StrEqual(steamid, "STEAM_ID_LAN"))
		{
			steamid[0] = '\0';
		}
	}
	
	SQL_EscapeString(db, name, safe_name, sizeof(safe_name));
	
	/**
	 * Construct the query using the information the user gave us.
	 */
	decl String:query[512];
	new len = 0;
	
	len += Format(query[len], sizeof(query)-len, "SELECT a.id, a.authtype, a.identity, a.password, a.flags, a.name, COUNT(ag.group_id), immunity");
	len += Format(query[len], sizeof(query)-len, " FROM sm_admins a LEFT JOIN sm_admins_groups ag ON a.id = ag.admin_id WHERE ");
	len += Format(query[len], sizeof(query)-len, " (a.authtype = 'ip' AND a.identity = '%s')", ipaddr);
	len += Format(query[len], sizeof(query)-len, " OR (a.authtype = 'name' AND a.identity = '%s')", safe_name);
	if (steamid[0] != '\0')
	{
		strcopy(steamidalt, sizeof(steamidalt), steamid);
		steamidalt[6] = (steamid[6] == '0') ? '1' : '0';

		len += Format(query[len], sizeof(query)-len, " OR (a.authtype = 'steam' AND (a.identity = '%s' OR a.identity = '%s'))", steamid, steamidalt);
	}
	len += Format(query[len], sizeof(query)-len, " GROUP BY a.id");
	
	/**
	 * Send the actual query.
	 */	
	PlayerSeq[client] = ++g_sequence;
	
	new Handle:pk;
	pk = CreateDataPack();
	WritePackCell(pk, client);
	WritePackCell(pk, PlayerSeq[client]);
	WritePackString(pk, query);
	
#if defined _DEBUG
	PrintToServer("Sending user query: %s", query);
#endif
	
	SQL_TQuery(db, OnReceiveUser, query, pk, DBPrio_High);
}

FetchUsersWeCan(Handle:db)
{
	for (new i=1; i<=MaxClients; i++)
	{
		if (PlayerAuth[i] && GetUserAdmin(i) == INVALID_ADMIN_ID)
		{
			FetchUser(db, i);
		}
	}
	
	/**
	 * This round of updates is done.  Go in peace.
	 */
	RebuildCachePart[_:AdminCache_Admins] = 0;
}


public OnReceiveGroupImmunity(Handle:owner, Handle:hndl, const String:error[], any:data)
{
	new Handle:pk = Handle:data;
	ResetPack(pk);
	
	/**
	 * Check if this is the latest result request.
	 */
	new sequence = ReadPackCell(pk);
	if (RebuildCachePart[_:AdminCache_Groups] != sequence)
	{
		/* Discard everything, since we're out of sequence. */
		CloseHandle(pk);
		return;
	}
	
	/**
	 * If we need to use the results, make sure they succeeded.
	 */
	if (hndl == INVALID_HANDLE)
	{
		decl String:query[255];
		ReadPackString(pk, query, sizeof(query));		
		LogError("SQL error receiving group immunity: %s", error);
		LogError("Query dump: %s", query);
		CloseHandle(pk);	
		return;
	}
	
	/* We're done with the pack forever. */
	CloseHandle(pk);
	
	while (SQL_FetchRow(hndl))
	{
		decl String:group1[80];
		decl String:group2[80];
		new GroupId:gid1, GroupId:gid2;
		
		SQL_FetchString(hndl, 0, group1, sizeof(group1));
		SQL_FetchString(hndl, 1, group2, sizeof(group2));
		
		if (((gid1 = FindAdmGroup(group1)) == INVALID_GROUP_ID)
			|| (gid2 = FindAdmGroup(group2)) == INVALID_GROUP_ID)
		{
			continue;
		}
		
		SetAdmGroupImmuneFrom(gid1, gid2);
#if defined _DEBUG
		PrintToServer("SetAdmGroupImmuneFrom(%d, %d)", gid1, gid2);
#endif
	}
	
	/* Clear the sequence so another connect doesn't refetch */
	RebuildCachePart[_:AdminCache_Groups] = 0;
}

public OnReceiveGroupOverrides(Handle:owner, Handle:hndl, const String:error[], any:data)
{
	new Handle:pk = Handle:data;
	ResetPack(pk);
	
	/**
	 * Check if this is the latest result request.
	 */
	new sequence = ReadPackCell(pk);
	if (RebuildCachePart[_:AdminCache_Groups] != sequence)
	{
		/* Discard everything, since we're out of sequence. */
		CloseHandle(pk);
		return;
	}
	
	/**
	 * If we need to use the results, make sure they succeeded.
	 */
	if (hndl == INVALID_HANDLE)
	{
		decl String:query[255];
		ReadPackString(pk, query, sizeof(query));		
		LogError("SQL error receiving group overrides: %s", error);
		LogError("Query dump: %s", query);
		CloseHandle(pk);	
		return;
	}
	
	/**
	 * Fetch the overrides.
	 */
	decl String:name[80];
	decl String:type[16];
	decl String:command[64];
	decl String:access[16];
	new GroupId:gid;
	while (SQL_FetchRow(hndl))
	{
		SQL_FetchString(hndl, 0, name, sizeof(name));
		SQL_FetchString(hndl, 1, type, sizeof(type));
		SQL_FetchString(hndl, 2, command, sizeof(command));
		SQL_FetchString(hndl, 3, access, sizeof(access));
		
		/* Find the group.  This is actually faster than doing the ID lookup. */
		if ((gid = FindAdmGroup(name)) == INVALID_GROUP_ID)
		{
			/* Oh well, just ignore it. */
			continue;
		}
		
		new OverrideType:o_type = Override_Command;
		if (StrEqual(type, "group"))
		{
			o_type = Override_CommandGroup;
		}
				
		new OverrideRule:o_rule = Command_Deny;
		if (StrEqual(access, "allow"))
		{
			o_rule = Command_Allow;
		}
				
#if defined _DEBUG
		PrintToServer("AddAdmGroupCmdOverride(%d, %s, %d, %d)", gid, command, o_type, o_rule);
#endif
				
		AddAdmGroupCmdOverride(gid, command, o_type, o_rule);
	}
	
	/**
	 * It's time to get the group immunity list.
	 */
	new len = 0;
	decl String:query[256];
	len += Format(query[len], sizeof(query)-len, "SELECT g1.name, g2.name FROM sm_group_immunity gi");
	len += Format(query[len], sizeof(query)-len, " LEFT JOIN sm_groups g1 ON g1.id = gi.group_id ");
	len += Format(query[len], sizeof(query)-len, " LEFT JOIN sm_groups g2 ON g2.id = gi.other_id");

	ResetPack(pk);
	WritePackCell(pk, sequence);
	WritePackString(pk, query);
	
	SQL_TQuery(owner, OnReceiveGroupImmunity, query, pk, DBPrio_High);
}

public OnReceiveGroups(Handle:owner, Handle:hndl, const String:error[], any:data)
{
	new Handle:pk = Handle:data;
	ResetPack(pk);
	
	/**
	 * Check if this is the latest result request.
	 */
	new sequence = ReadPackCell(pk);
	if (RebuildCachePart[_:AdminCache_Groups] != sequence)
	{
		/* Discard everything, since we're out of sequence. */
		CloseHandle(pk);
		return;
	}
	
	/**
	 * If we need to use the results, make sure they succeeded.
	 */
	if (hndl == INVALID_HANDLE)
	{
		decl String:query[255];
		ReadPackString(pk, query, sizeof(query));
		LogError("SQL error receiving groups: %s", error);
		LogError("Query dump: %s", query);
		CloseHandle(pk);
		return;
	}
	
	/**
	 * Now start fetching groups.
	 */
	decl String:flags[32];
	decl String:name[128];
	new immunity;
	while (SQL_FetchRow(hndl))
	{
		SQL_FetchString(hndl, 0, flags, sizeof(flags));
		SQL_FetchString(hndl, 1, name, sizeof(name));
		immunity = SQL_FetchInt(hndl, 2);
		
#if defined _DEBUG
		PrintToServer("Adding group (%d, %s, %s)", immunity, flags, name);
#endif
		
		/* Find or create the group */
		new GroupId:gid;
		if ((gid = FindAdmGroup(name)) == INVALID_GROUP_ID)
		{
			gid = CreateAdmGroup(name);
		}

		/* Add flags from the database to the group */
		new num_flag_chars = strlen(flags);
		for (new i=0; i<num_flag_chars; i++)
		{
			decl AdminFlag:flag;
			if (!FindFlagByChar(flags[i], flag))
			{
				continue;
			}
			SetAdmGroupAddFlag(gid, flag, true);
		}
		
		SetAdmGroupImmunityLevel(gid, immunity);
	}
	
	/**
	 * It's time to get the group override list.
	 */
	decl String:query[255];
	Format(query, 
		sizeof(query), 
		"SELECT g.name, og.type, og.name, og.access FROM sm_group_overrides og JOIN sm_groups g ON og.group_id = g.id ORDER BY g.id DESC");

	ResetPack(pk);
	WritePackCell(pk, sequence);
	WritePackString(pk, query);
	
	SQL_TQuery(owner, OnReceiveGroupOverrides, query, pk, DBPrio_High);
}

FetchGroups(Handle:db, sequence)
{
	decl String:query[255];
	new Handle:pk;
	
	Format(query, sizeof(query), "SELECT flags, name, immunity_level FROM sm_groups");

	pk = CreateDataPack();
	WritePackCell(pk, sequence);
	WritePackString(pk, query);
	
	SQL_TQuery(db, OnReceiveGroups, query, pk, DBPrio_High);
}

public OnReceiveOverrides(Handle:owner, Handle:hndl, const String:error[], any:data)
{
	new Handle:pk = Handle:data;
	ResetPack(pk);
	
	/**
	 * Check if this is the latest result request.
	 */
	new sequence = ReadPackCell(pk);
	if (RebuildCachePart[_:AdminCache_Overrides] != sequence)
	{
		/* Discard everything, since we're out of sequence. */
		CloseHandle(pk);
		return;
	}
	
	/**
	 * If we need to use the results, make sure they succeeded.
	 */
	if (hndl == INVALID_HANDLE)
	{
		decl String:query[255];
		ReadPackString(pk, query, sizeof(query));
		LogError("SQL error receiving overrides: %s", error);
		LogError("Query dump: %s", query);
		CloseHandle(pk);
		return;
	}
	
	/**
	 * We're done with you, now.
	 */
	CloseHandle(pk);
	
	decl String:type[64];
	decl String:name[64];
	decl String:flags[32];
	new flag_bits;
	while (SQL_FetchRow(hndl))
	{
		SQL_FetchString(hndl, 0, type, sizeof(type));
		SQL_FetchString(hndl, 1, name, sizeof(name));
		SQL_FetchString(hndl, 2, flags, sizeof(flags));
		
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
	RebuildCachePart[_:AdminCache_Overrides] = 0;
}

FetchOverrides(Handle:db, sequence)
{
	decl String:query[255];
	new Handle:pk;
	
	Format(query, sizeof(query), "SELECT type, name, flags FROM sm_overrides");

	pk = CreateDataPack();
	WritePackCell(pk, sequence);
	WritePackString(pk, query);
	
	SQL_TQuery(db, OnReceiveOverrides, query, pk, DBPrio_High);
}

