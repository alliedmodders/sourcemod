enum struct CookieData
{
    int dbId;
    char Name[COOKIE_MAX_NAME_LENGTH];
    char Description[COOKIE_MAX_DESCRIPTION_LENGTH];
    CookieAccess AccessLevel;
}

enum struct PlayerData
{
    int Timestamp;
    char Value[COOKIE_MAX_VALUE_LENGTH];
}

static bool gB_DataLoaded[MAXPLAYERS + 1];
static bool gB_DataPending[MAXPLAYERS + 1];

static StringMap g_Cookies;
static StringMap g_PlayerData[MAXPLAYERS + 1];

void InitCookies()
{
    g_Cookies = new StringMap();

    // MaxClients is not available here, creates potentially unnecessary handles
    for (int i = 1; i < sizeof(g_PlayerData); i++)
    {
        g_PlayerData[i] = new StringMap();
    }
}

void ClearPlayerData(int client)
{
    g_PlayerData[client].Clear();
}

bool IsPlayerDataCached(int client)
{
    return gB_DataLoaded[client];
}

bool IsPlayerDataPending(int client)
{
    return gB_DataPending[client];
}

void SetCookieData(const char[] name, CookieData data)
{
    g_Cookies.SetArray(name, data, sizeof(data));
}

void SetCookieKnownDatabaseId(const char[] name, int id)
{
    CookieData data;

    bool exists = GetCookieDataByName(name, data);
    if (!exists)
    {
        return;
    }

    data.dbId = id;
    SetCookieData(name, data);
}

void SetPlayerData(int client, const char[] name, PlayerData playerData)
{
    g_PlayerData[client].SetArray(name, playerData, sizeof(playerData));
}

void SetPlayerDataLoaded(int client, bool loaded)
{
    gB_DataLoaded[client] = loaded;
}

void SetPlayerDataPending(int client, bool pending)
{
    gB_DataPending[client] = pending;
}

int GetCookieCount()
{
    StringMapSnapshot snap = GetCookieDataSnapshot();
    int count = snap.Length;

    delete snap;
    return count;
}

StringMapSnapshot GetCookieDataSnapshot()
{
    return g_Cookies.Snapshot();
}

bool GetCookieDataByName(const char[] name, CookieData foundData)
{
    return g_Cookies.GetArray(name, foundData, sizeof(foundData));
}

bool GetCookieDataByIndex(int index, CookieData foundData)
{
    StringMapSnapshot snap = GetCookieDataSnapshot();

    char name[30];
    snap.GetKey(index, name, sizeof(name));

    delete snap;
    return GetCookieDataByName(name, foundData);
}

bool GetCookiePlayerData(int client, const char[] name, PlayerData foundData)
{
    return g_PlayerData[client].GetArray(name, foundData, sizeof(foundData));
}

int GetIteratorIndexFromConsumerHandle(ArrayList consumerHandle)
{
    return consumerHandle.Get(0);
}

bool IncrementIteratorIndexForConsumerHandle(ArrayList consumerHandle)
{
    int index = GetIteratorIndexFromConsumerHandle(consumerHandle);
    if (index < 0)
    {
        return false;
    }

    consumerHandle.Set(0, index + 1);
    return true;
}

bool GetCookieDataFromConsumerHandle(ArrayList consumerHandle, CookieData foundData)
{
    char name[30];
    consumerHandle.GetString(0, name, sizeof(name));

    return GetCookieDataByName(name, foundData);
}

bool GetCookiePlayerDataFromConsumerHandle(ArrayList consumerHandle, int client, PlayerData foundData)
{
    CookieData cookieData;

    bool hasCookieData = GetCookieDataFromConsumerHandle(consumerHandle, cookieData);
    if (!hasCookieData)
    {
        return false;
    }

    return GetCookiePlayerData(client, cookieData.Name, foundData);
}

ArrayList CreateConsumerHandleForIterator(Handle plugin)
{
    ArrayList originalHandle = new ArrayList(1, 1);
    originalHandle.Set(0, 0);

    Handle clonedHandle = CloneHandle(originalHandle, plugin);
    delete originalHandle;

    return view_as<ArrayList>(clonedHandle);
}

ArrayList CreateConsumerHandleForCookieData(CookieData data, Handle plugin)
{
    ArrayList originalHandle = new ArrayList(ByteCountToCells(30), 1);
    originalHandle.SetString(0, data.Name);

    Handle clonedHandle = CloneHandle(originalHandle, plugin);
    delete originalHandle;

    return view_as<ArrayList>(clonedHandle);
}
