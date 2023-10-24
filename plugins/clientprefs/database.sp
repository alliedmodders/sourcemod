static bool HasMigrated = false;
static bool IsConnecting = false;

bool DB_HasMigrated()
{
    return HasMigrated;
}

bool DB_IsConnected()
{
    return g_Database != null;
}

bool DB_IsConnecting()
{
    return IsConnecting;
}

void DB_SetHasMigrated(bool migrated)
{
    HasMigrated = migrated;
}

void DB_SetIsConnecting(bool connecting)
{
    IsConnecting = connecting;
}

bool DB_TryConnect()
{
    if (DB_IsConnected() || DB_IsConnecting())
    {
        return false;
    }

    DB_SetIsConnecting(true);

    if (SQL_CheckConfig("clientprefs"))
    {
        Database.Connect(OnDatabaseConnect, "clientprefs");
        return true;
    }

    if (SQL_CheckConfig("storage-local"))
    {
        Database.Connect(OnDatabaseConnect, "storage-local");
        return true;
    }

    SetFailState("Could not find any suitable database configs");
    return false;
}

void DB_CreateTables()
{
    if (!DB_IsConnected())
    {
        return;
    }

    char driverId[64];
    g_Database.Driver.GetIdentifier(driverId, sizeof(driverId));

    Transaction txn = new Transaction();

    if (StrEqual(driverId, "mysql"))
    {
        txn.AddQuery(
            "CREATE TABLE IF NOT EXISTS sm_cookies \
            ( \
                id INTEGER unsigned NOT NULL auto_increment, \
                name varchar(30) NOT NULL UNIQUE, \
                description varchar(255), \
                access INTEGER, \
                PRIMARY KEY (id) \
            )"
        );

        txn.AddQuery(
            "CREATE TABLE IF NOT EXISTS sm_cookie_cache \
            ( \
                player varchar(65) NOT NULL, \
                cookie_id int(10) NOT NULL, \
                value varchar(100), \
                timestamp int NOT NULL, \
                PRIMARY KEY (player, cookie_id) \
            )"
        );
    }
    else if (StrEqual(driverId, "sqlite"))
    {
        txn.AddQuery(
            "CREATE TABLE IF NOT EXISTS sm_cookies  \
            ( \
                id INTEGER PRIMARY KEY AUTOINCREMENT, \
                name varchar(30) NOT NULL UNIQUE, \
                description varchar(255), \
                access INTEGER \
            )"
        );

        txn.AddQuery(
            "CREATE TABLE IF NOT EXISTS sm_cookie_cache \
            ( \
                player varchar(65) NOT NULL, \
                cookie_id int(10) NOT NULL, \
                value varchar(100), \
                timestamp int, \
                PRIMARY KEY (player, cookie_id) \
            )"
        );
    }
    else if (StrEqual(driverId, "pgsql"))
    {
        txn.AddQuery(
            "CREATE TABLE IF NOT EXISTS sm_cookies \
            ( \
                id serial, \
                name varchar(30) NOT NULL UNIQUE, \
                description varchar(255), \
                access INTEGER, \
                PRIMARY KEY (id) \
            )"
        );

        txn.AddQuery(
            "CREATE TABLE IF NOT EXISTS sm_cookie_cache \
            ( \
                player varchar(65) NOT NULL, \
                cookie_id int NOT NULL, \
                value varchar(100), \
                timestamp int NOT NULL, \
                PRIMARY KEY (player, cookie_id) \
            )"
        );

        txn.AddQuery(
            "CREATE OR REPLACE FUNCTION add_or_update_cookie(in_player VARCHAR(65), in_cookie INT, in_value VARCHAR(100), in_time INT) RETURNS VOID AS \
            $$ \
            BEGIN \
                LOOP \
                UPDATE sm_cookie_cache SET value = in_value, timestamp = in_time WHERE player = in_player AND cookie_id = in_cookie; \
                IF found THEN \
                    RETURN; \
                END IF; \
                BEGIN \
                    INSERT INTO sm_cookie_cache (player, cookie_id, value, timestamp) VALUES (in_player, in_cookie, in_value, in_time); \
                    RETURN; \
                EXCEPTION WHEN unique_violation THEN \
                END; \
                END LOOP; \
            END; \
            $$ LANGUAGE plpgsql"
        );
    }
    else
    {
        ThrowError("Unsupported driver: %s", driverId);
        delete txn;
        return;
    }

    g_Database.Execute(txn, OnTablesSuccess, OnTablesFailure);
}

void DB_SelectCookieId(const char[] name)
{
    if (!DB_IsConnected())
    {
        return;
    }

    char query[2048];
    g_Database.Format(query, sizeof(query),
        "SELECT id, name FROM sm_cookies WHERE name = '%s'",
        name
    );

    g_Database.Query(OnSelectCookieId, query);
}

void DB_SelectPlayerData(int client, const char[] authId)
{
    if (!DB_IsConnected())
    {
        return;
    }

    char query[2048];
    g_Database.Format(query, sizeof(query),
        "SELECT sm_cookies.id, sm_cookies.name, sm_cookie_cache.value, sm_cookies.description, \
            sm_cookies.access, sm_cookie_cache.timestamp \
            FROM sm_cookies \
            JOIN sm_cookie_cache \
            ON sm_cookies.id = sm_cookie_cache.cookie_id \
            WHERE player = '%s'",
        authId
    );

    g_Database.Query(OnSelectPlayerData, query, GetClientUserId(client));
}

void DB_InsertCookie(const char[] name, const char[] desc, CookieAccess accessLevel)
{
    if (!DB_IsConnected())
    {
        return;
    }

    char driverId[64];
    g_Database.Driver.GetIdentifier(driverId, sizeof(driverId));

    char query[2048];

    if (StrEqual(driverId, "mysql"))
    {
        g_Database.Format(query, sizeof(query),
            "INSERT IGNORE INTO sm_cookies (name, description, access) \
                VALUES (\"%s\", \"%s\", %d)",
            name,
            desc,
            accessLevel
        );
    }
    else if (StrEqual(driverId, "sqlite"))
    {
        g_Database.Format(query, sizeof(query),
            "INSERT OR IGNORE INTO sm_cookies (name, description, access) \
                VALUES ('%s', '%s', %d)",
            name,
            desc,
            accessLevel
        );
    }
    else if (StrEqual(driverId, "pgsql"))
    {
        g_Database.Format(query, sizeof(query),
            "INSERT INTO sm_cookies (name, description, access) \
                VALUES ('%s', '%s', %d)",
            name,
            desc,
            accessLevel
        );
    }
    else
    {
        ThrowError("Unsupported driver: %s", driverId);
        return;
    }

    DataPack dp = new DataPack();
    dp.WriteString(name);

    g_Database.Query(OnInsertCookie, query, dp);
}

void DB_InsertPlayerData(const char[] authId, int cookieId, const char[] value)
{
    if (!DB_IsConnected())
    {
        return;
    }

    char driverId[64];
    g_Database.Driver.GetIdentifier(driverId, sizeof(driverId));

    char query[2048];

    int timestamp = GetTime();

    if (StrEqual(driverId, "mysql"))
    {
        g_Database.Format(query, sizeof(query),
            "INSERT INTO sm_cookie_cache (player, cookie_id, value, timestamp) \
                VALUES (\"%s\", %d, \"%s\", %d) \
                ON DUPLICATE KEY UPDATE \
                value = \"%s\", timestamp = %d",
            authId,
            cookieId,
            value,
            timestamp,
            value,
            timestamp
        );
    }
    else if (StrEqual(driverId, "sqlite"))
    {
        g_Database.Format(query, sizeof(query),
            "INSERT OR REPLACE INTO sm_cookie_cache \
                (player, cookie_id, value, timestamp) \
                VALUES ('%s', %d, '%s', %d)",
            authId,
            cookieId,
            value,
            timestamp
        );
    }
    else if (StrEqual(driverId, "pgsql"))
    {
        /*
            SourceMod comes with pgsql v9.6.9, UPSERT is in 9.5
            INSERT INTO sm_cookie_cache \
            (player, cookie_id, value, timestamp) \
            VALUES ('%s', %d, '%s', %d)
            ON CONFLICT (player, cookie_id) DO UPDATE
            SET value = '%s', timestamp = %d
        */

        g_Database.Format(query, sizeof(query),
            "SELECT add_or_update_cookie ('%s', %d, '%s', %d)",
            authId,
            cookieId,
            value,
            timestamp
        );
    }
    else
    {
        ThrowError("Unsupported driver: %s", driverId);
        return;
    }

    g_Database.Query(OnInsertPlayerData, query);
}

public void OnDatabaseConnect(Database db, const char[] error, any data)
{
    g_Database = db;

    DB_SetIsConnecting(false);

    if (!db || strlen(error) > 0)
    {
        LogError("Failed connecting to the database, error: %s", error);
        return;
    }

    if (!DB_HasMigrated())
    {
        DB_CreateTables();
    }
    else
    {
        InsertPendingCookies();
    }
}

public void OnTablesSuccess(Database db, any data, int numQueries, Handle[] results, any[] queryData)
{
    DB_SetHasMigrated(true);

    LateLoadClients();
    InsertPendingCookies();
}

public void OnTablesFailure(Database db, any data, int numQueries, const char[] error, int failIndex, any[] queryData)
{
    PrintToServer("Failed creating tables, error: %s", error);
}

public void OnSelectCookieId(Database db, DBResultSet results, const char[] error, any data)
{
    if (!db || strlen(error) > 0)
    {
        LogError("Failed selecting cookie id, error: %s", error);
        return;
    }

    if (!results.FetchRow())
    {
        return;
    }

    int id = results.FetchInt(0);

    char name[30];
    results.FetchString(1, name, sizeof(name));

    SetCookieKnownDatabaseId(name, id);
}

public void OnSelectPlayerData(Database db, DBResultSet results, const char[] error, int userId)
{
    if (!db || strlen(error) > 0)
    {
        LogError("Failed selecting player data, error: %s", error);
        return;
    }

    int client = GetClientOfUserId(userId);
    if (client == 0)
    {
        return;
    }

    SetPlayerDataPending(client, false);

    if (results == null)
    {
        return;
    }

    while (results.FetchRow())
    {
        char name[30];
        results.FetchString(1, name, sizeof(name));

        CookieData cookieData;
        cookieData.Name = name;

        bool hasData = GetCookieDataByName(name, cookieData);
        if (!hasData)
        {
            cookieData.dbId = results.FetchInt(0);
            cookieData.AccessLevel = view_as<CookieAccess>(results.FetchInt(4));

            results.FetchString(3, cookieData.Description, sizeof(cookieData.Description));

            SetCookieData(name, cookieData);
        }

        PlayerData playerData;
        playerData.Timestamp = results.FetchInt(5);

        results.FetchString(2, playerData.Value, sizeof(playerData.Value));

        SetPlayerData(client, cookieData.Name, playerData);
    }

    SetPlayerDataLoaded(client, true);

    Call_OnCookiesCached(client);
    PrintToServer("Loaded data for %N", client);
}

public void OnInsertCookie(Database db, DBResultSet results, const char[] error, DataPack dp)
{
    if (!db || strlen(error) > 0)
    {
        LogError("Failed inserting cookie, error: %s", error);
        delete dp;
        return;
    }

    dp.Reset();

    char name[30];
    dp.ReadString(name, sizeof(name));

    delete dp;

    int dbId = results.InsertId;
    if (dbId > 0)
    {
        SetCookieKnownDatabaseId(name, dbId);
        return;
    }

    // Manually lookup id if this was not an insert
    DB_SelectCookieId(name);
}

public void OnInsertPlayerData(Database db, DBResultSet results, const char[] error, any data)
{
    if (!db || strlen(error) > 0)
    {
        LogError("Failed inserting player data, error: %s", error);
        return;
    }
}
