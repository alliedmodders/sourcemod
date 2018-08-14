/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod SQL Admins Plugin (Prefetch)
 * Prefetches admins from an SQL database without threading.
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
	name = "SQL Admins (Prefetch)",
	author = "AlliedModders LLC",
	description = "Reads all admins from SQL",
	version = SOURCEMOD_VERSION,
	url = "http://www.sourcemod.net/"
};

public void OnRebuildAdminCache(AdminCachePart part)
{
	/* First try to get a database connection */
	char error[255];
	Database db;
	
	if (SQL_CheckConfig("admins"))
	{
		db = SQL_Connect("admins", true, error, sizeof(error));
	} else {
		db = SQL_Connect("default", true, error, sizeof(error));
	}
	
	if (db == null)
	{
		LogError("Could not connect to database \"default\": %s", error);
		return;
	}
	
	if (part == AdminCache_Overrides)
	{
		FetchOverrides(db);
	} else if (part == AdminCache_Groups) {
		FetchGroups(db);
	} else if (part == AdminCache_Admins) {
		FetchUsers(db);
	}
	
	delete db;
}

void FetchUsers(Database db)
{
	char query[255], error[255];
	DBResultSet rs;

	Format(query, sizeof(query), "SELECT id, authtype, identity, password, flags, name, immunity FROM sm_admins");
	if ((rs = SQL_Query(db, query)) == null)
	{
		SQL_GetError(db, error, sizeof(error));
		LogError("FetchUsers() query failed: %s", query);
		LogError("Query error: %s", error);
		return;
	}

	char authtype[16];
	char identity[80];
	char password[80];
	char flags[32];
	char name[80];
	int immunity;
	AdminId adm;
	GroupId grp;
	int id;

	/* Keep track of a mapping from admin DB IDs to internal AdminIds to
	 * enable group lookups en masse */
	StringMap htAdmins = new StringMap();
	char key[16];
	
	while (rs.FetchRow())
	{
		id = rs.FetchInt(0);
		IntToString(id, key, sizeof(key));
		rs.FetchString(1, authtype, sizeof(authtype));
		rs.FetchString(2, identity, sizeof(identity));
		rs.FetchString(3, password, sizeof(password));
		rs.FetchString(4, flags, sizeof(flags));
		rs.FetchString(5, name, sizeof(name));
		immunity = rs.FetchInt(6);
		
		/* Use a pre-existing admin if we can */
		if ((adm = FindAdminByIdentity(authtype, identity)) == INVALID_ADMIN_ID)
		{
			adm = CreateAdmin(name);
			if (!adm.BindIdentity(authtype, identity))
			{
				LogError("Could not bind prefetched SQL admin (authtype \"%s\") (identity \"%s\")", authtype, identity);
				continue;
			}
		}

		htAdmins.SetValue(key, adm);
		
#if defined _DEBUG
		PrintToServer("Found SQL admin (%d,%s,%s,%s,%s,%s,%d):%d", id, authtype, identity, password, flags, name, immunity, adm);
#endif
		
		/* See if this admin wants a password */
		if (password[0] != '\0')
		{
			adm.SetPassword(password);
		}
		
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

		adm.ImmunityLevel = immunity;
	}

	delete rs;

	Format(query, sizeof(query), "SELECT ag.admin_id AS id, g.name FROM sm_admins_groups ag JOIN sm_groups g ON ag.group_id = g.id  ORDER BY id, inherit_order ASC");
	if ((rs = SQL_Query(db, query)) == null)
	{
		SQL_GetError(db, error, sizeof(error));
		LogError("FetchUsers() query failed: %s", query);
		LogError("Query error: %s", error);
		return;
	}

	char group[80];
	while (rs.FetchRow())
	{
		IntToString(rs.FetchInt(0), key, sizeof(key));
		rs.FetchString(1, group, sizeof(group));

		if (htAdmins.GetValue(key, adm))
		{
			if ((grp = FindAdmGroup(group)) == INVALID_GROUP_ID)
			{
				/* Group wasn't found, don't bother with it.  */
				continue;
			}

			adm.InheritGroup(grp);
		}
	}
	
	delete rs;
	delete htAdmins;
}

void FetchGroups(Database db)
{
	char query[255];
	DBResultSet rs;
	
	Format(query, sizeof(query), "SELECT flags, name, immunity_level FROM sm_groups");

	if ((rs = SQL_Query(db, query)) == null)
	{
		char error[255];
		SQL_GetError(db, error, sizeof(error));
		LogError("FetchGroups() query failed: %s", query);
		LogError("Query error: %s", error);
		return;
	}
	
	/* Now start fetching groups */
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
		
		/* Set the immunity level this group has */
		grp.ImmunityLevel = immunity;
	}
	
	delete rs;
	
	/** 
	 * Get immunity in a big lump.  This is a nasty query but it gets the job done.
	 */
	int len = 0;
	len += Format(query[len], sizeof(query)-len, "SELECT g1.name, g2.name FROM sm_group_immunity gi");
	len += Format(query[len], sizeof(query)-len, " LEFT JOIN sm_groups g1 ON g1.id = gi.group_id ");
	len += Format(query[len], sizeof(query)-len, " LEFT JOIN sm_groups g2 ON g2.id = gi.other_id");
	
	if ((rs = SQL_Query(db, query)) == null)
	{
		char error[255];
		SQL_GetError(db, error, sizeof(error));
		LogError("FetchGroups() query failed: %s", query);
		LogError("Query error: %s", error);
		return;
	}
	
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
	
	delete rs;
	
	/**
	 * Fetch overrides in a lump query.
	 */
	Format(query, sizeof(query), "SELECT g.name, go.type, go.name, go.access FROM sm_group_overrides go LEFT JOIN sm_groups g ON go.group_id = g.id");
	
	if ((rs = SQL_Query(db, query)) == null)
	{
		char error[255];
		SQL_GetError(db, error, sizeof(error));
		LogError("FetchGroups() query failed: %s", query);
		LogError("Query error: %s", error);
		return;
	}
	
	char type[16];
	char cmd[64];
	char access[16];
	while (rs.FetchRow())
	{
		rs.FetchString(0, name, sizeof(name));
		rs.FetchString(1, type, sizeof(type));
		rs.FetchString(2, cmd, sizeof(cmd));
		rs.FetchString(3, access, sizeof(access));
		
		GroupId grp;
		if ((grp = FindAdmGroup(name)) == INVALID_GROUP_ID)
		{
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
		PrintToServer("AddAdmGroupCmdOverride(%d, %s, %d, %d)", grp, cmd, o_type, o_rule);
#endif
		
		grp.AddCommandOverride(cmd, o_type, o_rule);
	}
	
	delete rs;
}

void FetchOverrides(Database db)
{
	char query[255];
	DBResultSet rs;
	
	Format(query, sizeof(query), "SELECT type, name, flags FROM sm_overrides");

	if ((rs = SQL_Query(db, query)) == null)
	{
		char error[255];
		SQL_GetError(db, error, sizeof(error));
		LogError("FetchOverrides() query failed: %s", query);
		LogError("Query error: %s", error);
		return;
	}
	
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
	
	delete rs;
}
