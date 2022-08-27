enum struct CookieMenuData
{
    any UserData;
    CookieMenu Type;
    PrivateForward Handler;
}

static Menu g_settingsMenu;
static StringMap g_menuData;

void InitMenus()
{
    g_menuData = new StringMap();
    g_settingsMenu = new Menu(MenuHandler_Settings);
}

bool GetMenuDataByName(const char[] name, CookieMenuData foundData)
{
    return g_menuData.GetArray(name, foundData, sizeof(foundData));
}

bool SetMenuDataByName(const char[] name, CookieMenuData data)
{
    return g_menuData.SetArray(name, data, sizeof(data));
}

void AddSettingsMenuItem(
    Handle callingPlugin,
    CookieData cookieData,
    const char[] display,
    CookieMenu type,
    Function handler,
    any userData
) {
    CookieMenuData data;

    bool hasData = GetMenuDataByName(cookieData.Name, data);
    if (!hasData)
    {
        data.Handler = new PrivateForward(ET_Ignore, Param_Cell, Param_Cell, Param_Cell, Param_String, Param_Cell);
        g_settingsMenu.AddItem(cookieData.Name, display);
    }

    if (handler != INVALID_FUNCTION)
    {
        data.Handler.AddFunction(callingPlugin, handler);
    }

    data.Type = type;
    data.UserData = userData;

    SetMenuDataByName(cookieData.Name, data);
}

void DisplaySettingsMenu(int client)
{
    g_settingsMenu.SetTitle("%T:", "Client Settings", client);
    g_settingsMenu.Display(client, MENU_TIME_FOREVER);
}

void DisplayOptionSelectionMenu(int client, CookieData cookieData)
{
    CookieMenuData menuData;

    bool hasMenuData = GetMenuDataByName(cookieData.Name, menuData);
    if (!hasMenuData)
    {
        return;
    }

    Menu menu = new Menu(MenuHandler_Option, MENU_ACTIONS_DEFAULT | MenuAction_DisplayItem);
    menu.SetTitle("%T:", "Choose Option", client);

    // Store cookie name in menu item 0 for context
    menu.AddItem(cookieData.Name, "", ITEMDRAW_IGNORE);

    switch (menuData.Type)
    {
        case CookieMenu_YesNo:
        {
            menu.AddItem("yes", "Yes");
            menu.AddItem("no", "No");
        }
        case CookieMenu_YesNo_Int:
        {
            menu.AddItem("1", "Yes");
            menu.AddItem("0", "No");
        }
        case CookieMenu_OnOff:
        {
            menu.AddItem("on", "On");
            menu.AddItem("off", "Off");
        }
        case CookieMenu_OnOff_Int:
        {
            menu.AddItem("1", "On");
            menu.AddItem("0", "Off");
        }
    }

    menu.Display(client, MENU_TIME_FOREVER);
}

public int MenuHandler_Settings(Menu menu, MenuAction action, int param1, int param2)
{
    if (action == MenuAction_Select)
    {
        char name[30];
        menu.GetItem(param2, name, sizeof(name));

        CookieData cookieData;

        bool hasCookieData = GetCookieDataByName(name, cookieData);
        if (!hasCookieData)
        {
            return 0;
        }

        CookieMenuData menuData;

        bool hasMenuData = GetMenuDataByName(name, menuData);
        if (!hasMenuData)
        {
            return 0;
        }

        if (menuData.Handler != null)
        {
            Call_StartForward(menuData.Handler);
            Call_PushCell(param1);
            Call_PushCell(CookieMenuAction_SelectOption);
            Call_PushCell(menuData.UserData);
            Call_PushString("");
            Call_PushCell(0);
            Call_Finish();
        }

        DisplayOptionSelectionMenu(param1, cookieData);
    }

    return 0;
}

public int MenuHandler_Option(Menu menu, MenuAction action, int param1, int param2)
{
    if (action == MenuAction_Select)
    {
        char name[30];
        menu.GetItem(0, name, sizeof(name));

        char value[100];
        menu.GetItem(param2, value, sizeof(value));

        CookieData cookieData;

        bool hasCookieData = GetCookieDataByName(name, cookieData);
        if (!hasCookieData)
        {
            return 0;
        }

        bool updated = UpdatePlayerCookieValue(param1, name, value);
        if (!updated)
        {
            return 0;
        }

        PrintToChat(param1, "[SM] %t", "Cookie Changed Value", name, value);
    }
    else if (action == MenuAction_DisplayItem)
    {
        char name[30];
        menu.GetItem(0, name, sizeof(name));

        CookieMenuData data;

        bool hasData = GetMenuDataByName(name, data);
        if (!hasData || data.Handler == null)
        {
            return 0;
        }

        char buffer[100];
        menu.GetItem(param2, "", 0, _, buffer, sizeof(buffer));

        Call_StartForward(data.Handler);
        Call_PushCell(param1);
        Call_PushCell(CookieMenuAction_DisplayOption);
        Call_PushCell(data.UserData);
        Call_PushStringEx(buffer, sizeof(buffer), SM_PARAM_STRING_COPY, SM_PARAM_COPYBACK);
        Call_PushCell(sizeof(buffer));
        Call_Finish();

        return RedrawMenuItem(buffer);
    }
    else if (action == MenuAction_End)
    {
        delete menu;
    }

    return 0;
}