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

StringMap g_htAdmins = null;
Database g_hSQLConnection = null;
bool g_bConnecting = false;

enum QueryType
{
	eAdminCache_Overrides = view_as<int>(AdminCache_Overrides),
	eAdminCache_Groups = view_as<int>(AdminCache_Groups),
	eAdminCache_Admins = view_as<int>(AdminCache_Admins),
	eAdminUserGroups = eAdminCache_Overrides+eAdminCache_Groups+eAdminCache_Admins,
	eAdminUserImmunity,
	eAdminUserOverrides
}

public Plugin:myinfo = 
{
	name = "SQL Admins (Prefetch)",
	author = "AlliedModders LLC",
	description = "Reads all admins from SQL",
	version = SOURCEMOD_VERSION,
	url = "http://www.sourcemod.net/"
};

public OnPluginStart()
{
	g_htAdmins = new StringMap();
}

public OnRebuildAdminCache(AdminCachePart part)
{
	PerformSQLQuery(view_as<QueryType>(part));
}

static void PerformSQLQuery(QueryType queryval)
{
	if (!g_hSQLConnection && !g_bConnecting)
	{
		char dbname[24];
		if (SQL_CheckConfig("admins"))
			strcopy(dbname, sizeof(dbname), "admins");
		else
			strcopy(dbname, sizeof(dbname), "default");

		g_bConnecting = true;
		Database.Connect(SQL_OnDatabaseConnected, dbname, queryval);
		return;
	}

	char query[256];
	switch (queryval)
	{
		case eAdminCache_Overrides:
		{
			strcopy(query, sizeof(query), "SELECT type, name, flags FROM sm_overrides");
		}

		case eAdminCache_Groups:
		{
			strcopy(query, sizeof(query), "SELECT flags, name, immunity_level FROM sm_groups");
		}

		case eAdminCache_Admins:
		{
			strcopy(query, sizeof(query), "SELECT id, authtype, identity, password, flags, name, immunity FROM sm_admins");
		}

		case eAdminUserGroups:
		{
			strcopy(query, sizeof(query), "SELECT ag.admin_id AS id, g.name FROM sm_admins_groups ag JOIN sm_groups g ON ag.group_id = g.id  ORDER BY id, inherit_order ASC");
		}

		case eAdminUserImmunity:
		{
			strcopy(query, sizeof(query), "SELECT g1.name, g2.name FROM sm_group_immunity gi\
												  LEFT JOIN sm_groups g1 ON g1.id = gi.group_id\
												  LEFT JOIN sm_groups g2 ON g2.id = gi.other_id");
		}

		case eAdminUserOverrides:
		{
			strcopy(query, sizeof(query), "SELECT g.name, go.type, go.name, go.access FROM sm_group_overrides go LEFT JOIN sm_groups g ON go.group_id = g.id");
		}

		default:
		{
			ThrowError("Undefined enum type: %u", queryval);
		}
	}

	g_hSQLConnection.Query(SQL_OnQueryCompleted, query, queryval);
}

public SQL_OnDatabaseConnected(Database db, const char[] error, any data)
{
	g_bConnecting = false;
	if (!db)
	{
		LogError("Could not connect to database \"default\": %s", error);
		return;
	}

	delete g_hSQLConnection;
	g_hSQLConnection = db;
	PerformSQLQuery(eAdminCache_Overrides);
	PerformSQLQuery(eAdminCache_Groups);
	PerformSQLQuery(eAdminCache_Admins);
}

public SQL_OnQueryCompleted(Database db, DBResultSet results, const char[] error, any data)
{
	if (!results || error[0] != '\0')
	{
		LogError("Query Error: %s", error);
		return;
	}

	switch (view_as<QueryType>(data))
	{
		case eAdminCache_Overrides:
		{
			FetchOverrides(results);
		}

		case eAdminCache_Groups:
		{
			PerformSQLQuery(eAdminUserImmunity);
			PerformSQLQuery(eAdminUserOverrides);
			FetchGroups(results);
		}

		case eAdminCache_Admins:
		{
			PerformSQLQuery(eAdminUserGroups);
			FetchUsers(results);
		}

		case eAdminUserGroups:
		{
			FetchUserGroups(results);
		}

		case eAdminUserImmunity:
		{
			FetchUserImmunity(results);
		}

		case eAdminUserOverrides:
		{
			FetchUserOverrides(results);
		}
	}
}

void FetchUsers(DBResultSet rs)
{
	char authtype[16];
	char identity[80];
	char password[80];
	char flags[32];
	char name[80];
	int immunity;
	AdminId adm;
	int id;

	/* Keep track of a mapping from admin DB IDs to internal AdminIds to
	 * enable group lookups en masse */
	g_htAdmins.Clear();
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

		g_htAdmins.SetValue(key, adm);
		
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

void FetchUserGroups(DBResultSet rs)
{
	char group[80], key[16];
	GroupId grp;
	AdminId adm;

	while (rs.FetchRow())
	{
		IntToString(rs.FetchInt(0), key, sizeof(key));
		rs.FetchString(1, group, sizeof(group));

		if (g_htAdmins.GetValue(key, adm))
		{
			if ((grp = FindAdmGroup(group)) == INVALID_GROUP_ID)
			{
				/* Group wasn't found, don't bother with it.  */
				continue;
			}

			adm.InheritGroup(grp);
		}
	}
}

void FetchGroups(DBResultSet rs)
{
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
}

void FetchUserImmunity(DBResultSet rs)
{	
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
}

FetchUserOverrides(DBResultSet rs)
{	
	char type[16];
	char cmd[64];
	char access[16];
	char name[64];
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
}

void FetchOverrides(DBResultSet rs)
{
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
}
