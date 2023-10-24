void CreateNatives()
{
    CreateNative("RegClientCookie", Native_RegCookie);
    CreateNative("FindClientCookie", Native_FindCookie);
    CreateNative("SetClientCookie", Native_SetCookieValue_Old);
    CreateNative("GetClientCookie", Native_GetCookieValue_Old);
    CreateNative("GetCookieAccess", Native_GetCookieAccess);
    CreateNative("GetClientCookieTime", Native_GetClientCookieTime_Old);
    CreateNative("SetAuthIdCookie", Native_SetCookieValueByAuthId_Old);
    CreateNative("AreClientCookiesCached", Native_AreClientCookiesCached);
    CreateNative("SetCookiePrefabMenu", Native_SetCookiePrefabMenu);
    //CreateNative("SetCookieMenuItem", Native_SetCookieMenuItem);
    CreateNative("ShowCookieMenu", Native_ShowCookieMenu);
    CreateNative("GetCookieIterator", Native_GetCookieIterator);
    CreateNative("ReadCookieIterator", Native_ReadCookieIterator);

    CreateNative("Cookie.Cookie", Native_RegCookie);
    CreateNative("Cookie.Find", Native_FindCookie);
    CreateNative("Cookie.Set", Native_SetCookieValue);
    CreateNative("Cookie.Get", Native_GetCookieValue);
    CreateNative("Cookie.AccessLevel.get", Native_GetCookieAccess);
    CreateNative("Cookie.GetClientTime", Native_GetClientCookieTime);
    CreateNative("Cookie.SetPrefabMenu", Native_SetCookiePrefabMenu);
    CreateNative("Cookie.SetByAuthId", Native_SetCookieValueByAuthId);
}

public any Native_RegCookie(Handle plugin, int numParams)
{
    char name[30];
    GetNativeString(1, name, sizeof(name));

    char description[255];
    GetNativeString(2, description, sizeof(description));

    CookieAccess accessLevel = GetNativeCell(3);

    CookieData cookieData;

    bool found = GetCookieDataByName(name, cookieData);
    if (!found)
    {
        cookieData.Name = name;
        cookieData.Description = description;
        cookieData.AccessLevel = accessLevel;

        SetCookieData(name, cookieData);
        DB_InsertCookie(name, description, accessLevel);
    }

    return CreateConsumerHandleForCookieData(cookieData, plugin);
}

public any Native_FindCookie(Handle plugin, int numParams)
{
    char name[30];
    GetNativeString(1, name, sizeof(name));

    CookieData cookieData;

    bool found = GetCookieDataByName(name, cookieData);
    if (!found)
    {
        return INVALID_HANDLE;
    }

    return CreateConsumerHandleForCookieData(cookieData, plugin);
}

public any Native_SetCookieValue(Handle plugin, int numParams)
{
    any handle = GetNativeCell(1);
    int client = GetNativeCell(2);

    char value[100];
    GetNativeString(3, value, sizeof(value));

    return SetCookieValue_Impl(client, value, handle);
}

public any Native_SetCookieValue_Old(Handle plugin, int numParams)
{
    int client = GetNativeCell(1);
    any handle = GetNativeCell(2);

    char value[100];
    GetNativeString(3, value, sizeof(value));

    return SetCookieValue_Impl(client, value, handle);
}

public any Native_GetCookieValue(Handle plugin, int numParams)
{
    any handle = GetNativeCell(1);
    int client = GetNativeCell(2);

    return GetCookieValue_Impl(client, handle);
}

public any Native_GetCookieValue_Old(Handle plugin, int numParams)
{
    int client = GetNativeCell(1);
    any handle = GetNativeCell(2);

    return GetCookieValue_Impl(client, handle);
}

public any Native_GetCookieAccess(Handle plugin, int numParams)
{
    any handle = GetNativeCell(1);

    CookieData data;

    bool hasData = GetCookieDataFromConsumerHandle(handle, data);
    if (!hasData)
    {
        // TODO: Throw?
        return -1;
    }

    return data.AccessLevel;
}

public any Native_GetClientCookieTime(Handle plugin, int numParams)
{
    any handle = GetNativeCell(1);
    int client = GetNativeCell(2);

    return GetClientCookieTime_Impl(client, handle);
}

public any Native_GetClientCookieTime_Old(Handle plugin, int numParams)
{
    int client = GetNativeCell(1);
    any handle = GetNativeCell(2);

    return GetClientCookieTime_Impl(client, handle);
}

public any Native_AreClientCookiesCached(Handle plugin, int numParams)
{
    int client = GetNativeCell(1);
    if (client < 1 || client > MaxClients)
    {
        ThrowNativeError(SP_ERROR_NATIVE, "Client index %d is invalid", client);
        return false;
    }

    return IsPlayerDataCached(client);
}

public any Native_SetCookiePrefabMenu(Handle plugin, int numParams)
{
    any handle = GetNativeCell(1);

    CookieData data;

    bool hasData = GetCookieDataFromConsumerHandle(handle, data);
    if (!hasData)
    {
        // TODO: Throw?
        return false;
    }

    CookieMenu type = GetNativeCell(2);

    char display[128];
    GetNativeString(3, display, sizeof(display));

    Function handler = GetNativeFunction(4);

    any userData = GetNativeCell(5);

    AddSettingsMenuItem(plugin, data, display, type, handler, userData);
    return true;
}

public any Native_SetCookieValueByAuthId(Handle plugin, int numParams)
{
    any handle = GetNativeCell(1);

    char authId[32];
    GetNativeString(2, authId, sizeof(authId));

    char value[100];
    GetNativeString(3, value, sizeof(value));

    return SetCookieValueByAuthId_Impl(authId, value, handle);
}

public any Native_SetCookieValueByAuthId_Old(Handle plugin, int numParams)
{
    char authId[32];
    GetNativeString(1, authId, sizeof(authId));

    any handle = GetNativeCell(2);

    char value[100];
    GetNativeString(3, value, sizeof(value));

    return SetCookieValueByAuthId_Impl(authId, value, handle);
}

public any Native_ShowCookieMenu(Handle plugin, int numParams)
{
    int client = GetNativeCell(1);
    return DisplaySettingsMenu(client);
}

public any Native_GetCookieIterator(Handle plugin, int numParams)
{
    return CreateConsumerHandleForIterator(plugin);
}

public any Native_ReadCookieIterator(Handle plugin, int numParams)
{
    any handle = GetNativeCell(1);

    int index = GetIteratorIndexFromConsumerHandle(handle);
    if (index < 0 || index >= GetCookieCount())
    {
        return false;
    }

    CookieData data;
    GetCookieDataByIndex(index, data);

    SetNativeString(2, data.Name, GetNativeCell(3));

    SetNativeCellRef(4, data.AccessLevel);

    SetNativeString(5, data.Description, GetNativeCell(6));

    IncrementIteratorIndexForConsumerHandle(handle);
    return true;
}

any SetCookieValue_Impl(int client, const char value[100], any handle)
{
    CookieData cookieData;
    GetCookieDataFromConsumerHandle(handle, cookieData);

    return UpdatePlayerCookieValue(client, cookieData.Name, value);
}

any GetCookieValue_Impl(int client, any handle)
{
    PlayerData data;

    bool hasData = GetCookiePlayerDataFromConsumerHandle(handle, client, data);
    if (!hasData)
    {
        return false;
    }

    int maxlength = GetNativeCell(4);

    SetNativeString(3, data.Value, maxlength);
    return true;
}

any GetClientCookieTime_Impl(int client, any handle)
{
    PlayerData data;

    bool hasData = GetCookiePlayerDataFromConsumerHandle(handle, client, data);
    if (!hasData)
    {
        return -1;
    }

    return data.Timestamp;
}

any SetCookieValueByAuthId_Impl(const char[] authId, const char value[100], any handle)
{
    int foundClient = FindClientByAuthId(authId);
    if (foundClient == -1)
    {
        return false;
    }

    return SetCookieValue_Impl(foundClient, value, handle);
}
