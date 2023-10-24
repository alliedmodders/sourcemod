#include <sourcemod>
#include <clientprefs>

#pragma semicolon 1
#pragma newdecls required

Database g_Database = null;

#include "clientprefs/utils.sp"
#include "clientprefs/menus.sp"
#include "clientprefs/cookies.sp"
#include "clientprefs/commands.sp"
#include "clientprefs/database.sp"
#include "clientprefs/forwards.sp"
#include "clientprefs/natives.sp"

public Plugin myinfo =
{
	name = "Client Preferences",
	author = "AlliedModders LLC",
	description = "Saves client preference settings",
	version = SOURCEMOD_VERSION,
	url = "http://www.sourcemod.net/"
};

public APLRes AskPluginLoad2(Handle myself, bool late, char[] error, int err_max)
{
    InitMenus();
    InitCookies();
    CreateNatives();
    CreateForwards();

    RegPluginLibrary("clientprefs");
    return APLRes_Success;
}

public void OnPluginStart()
{
    LoadTranslations("common.phrases");
    LoadTranslations("clientprefs.phrases");

    CreateCommands();

    DB_TryConnect();
}

public void OnPluginEnd()
{
    // Try to save player data
    for (int i = 1; i <= MaxClients; i++)
    {
        OnClientDisconnect(i);
    }
}

public void OnMapStart()
{
    DB_TryConnect();
}

// Second param is unused
public void OnClientAuthorized(int client)
{
    if (!DB_HasMigrated() || IsFakeClient(client))
    {
        return;
    }

    SetPlayerDataPending(client, true);

    char steamId2[32];
    GetClientAuthId(client, AuthId_Steam2, steamId2, sizeof(steamId2));

    DB_SelectPlayerData(client, steamId2);
}

public void OnClientDisconnect(int client)
{
    if (!IsClientConnected(client) || IsFakeClient(client))
    {
        return;
    }

    char authId[32];

    bool hasAuth = GetClientAuthId(client, AuthId_Steam2, authId, sizeof(authId));
    if (!hasAuth)
    {
        // Unlucky
        return;
    }

    // TODO: Could this use transactions per player?
    StringMapSnapshot snap = GetCookieDataSnapshot();

    for (int i = 0; i < snap.Length; i++)
    {
        char name[30];
        snap.GetKey(i, name, sizeof(name));

        CookieData cookieData;
        GetCookieDataByName(name, cookieData);

        if (cookieData.dbId > 0)
        {
            PlayerData playerData;
            GetCookiePlayerData(client, name, playerData);

            DB_InsertPlayerData(authId, cookieData.dbId, playerData.Value);
        }
    }

    delete snap;

    ClearPlayerData(client);
    SetPlayerDataLoaded(client, false);
    SetPlayerDataPending(client, false);
}

void LateLoadClients()
{
    for (int i = 1; i <= MaxClients; i++)
    {
        if (IsPlayerDataCached(i) || IsPlayerDataPending(i))
        {
            continue;
        }

        if (!IsClientAuthorized(i))
        {
            continue;
        }

        OnClientAuthorized(i);
    }
}

void InsertPendingCookies()
{
    StringMapSnapshot snap = GetCookieDataSnapshot();

    for (int i = 0; i < snap.Length; i++)
    {
        char name[30];
        snap.GetKey(i, name, sizeof(name));

        CookieData cookieData;
        GetCookieDataByName(name, cookieData);

        // If the database id is not set, this has not yet made it into the db
        if (cookieData.dbId == 0)
        {
            DB_InsertCookie(cookieData.Name, cookieData.Description, cookieData.AccessLevel);
        }
    }

    delete snap;
}
