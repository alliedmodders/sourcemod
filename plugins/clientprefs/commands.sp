void CreateCommands()
{
    RegConsoleCmd("sm_cookies", Command_Cookies, "sm_cookies <name> [value]");
	//RegConsoleCmd("sm_settings", Command_Settings);
}

public Action Command_Cookies(int client, int args)
{
    if (args == 0)
    {
        ReplyToCommand(client, "[SM] Usage: sm_cookies <name> [value]");
        ReplyToCommand(client, "[SM] %t", "Printing Cookie List");

        PrintToConsole(client, "%t:", "Cookie List");

        StringMapSnapshot snap = GetCookieDataSnapshot();

        int count = 1;

        for (int i = 0; i < snap.Length; i++)
        {
            char name[30];
            snap.GetKey(i, name, sizeof(name));

            CookieData cookieData;
            GetCookieDataByName(name, cookieData);

            if (cookieData.AccessLevel < CookieAccess_Private)
            {
                PrintToConsole(client, "[%03d] %s - %s", count++, name, cookieData.Description);
            }
        }

        delete snap;
        return Plugin_Handled;
    }

    if (client == 0)
    {
        PrintToServer("%T", "No Console", LANG_SERVER);
        return Plugin_Handled;
    }

    char name[30];
    GetCmdArg(1, name, sizeof(name));

    CookieData cookieData;

    bool hasCookieData = GetCookieDataByName(name, cookieData);
    if (!hasCookieData)
    {
        ReplyToCommand(client, "[SM] %t", "Cookie not Found", name);
        return Plugin_Handled;
    }

    if (cookieData.AccessLevel == CookieAccess_Private)
    {
        ReplyToCommand(client, "[SM] %t", "Cookie not Found", name);
        return Plugin_Handled;
    }

    if (args == 1)
    {
        PlayerData playerData;

        bool hasPlayerData = GetCookiePlayerData(client, name, playerData);
        if (!hasPlayerData)
        {
            // Enum struct strings would default to "" so this is technically an useless branch
            ReplyToCommand(client, "[SM] %t", "Cookie Value", name, "");
            return Plugin_Handled;
        }

        ReplyToCommand(client, "[SM] %t", "Cookie Value", name, playerData.Value);

        // Eww mutating...
        TrimString(cookieData.Description);

        if (cookieData.Description[0])
        {
            ReplyToCommand(client, "- %s", cookieData.Description);
        }

        return Plugin_Handled;
    }

    if (cookieData.AccessLevel == CookieAccess_Protected)
    {
        ReplyToCommand(client, "[SM] %t", "Protected Cookie", name);
        return Plugin_Handled;
    }

    char value[100];
    GetCmdArg(2, value, sizeof(value));

    PlayerData playerData;
    GetCookiePlayerData(client, name, playerData);

    playerData.Value = value;
    SetPlayerData(client, name, playerData);

    // TODO: Decide on using internal state/funcs vs Plugin API

    ReplyToCommand(client, "[SM] %t", "Cookie Changed Value", name, value);
    return Plugin_Handled;
}
