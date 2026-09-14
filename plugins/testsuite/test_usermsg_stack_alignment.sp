#pragma semicolon 1
#pragma newdecls required

#include <sourcemod>

bool g_SeenMessages[255];
int g_HookedMessages;
int g_InterceptedMessages;

public Plugin myinfo =
{
    name = "User Message Stack Alignment Test",
    author = "AlliedModders LLC",
    description = "Exercises pre user-message interception callbacks",
    version = "1.0.0.0",
    url = "https://www.sourcemod.net/"
};

public void OnPluginStart()
{
    if (GetEngineVersion() != Engine_CSS)
    {
        SetFailState("This test is intended for Counter-Strike: Source.");
        return;
    }

    for (int id = 0; id < sizeof(g_SeenMessages); id++)
    {
        UserMsg message = view_as<UserMsg>(id);
        char name[64];
        if (!GetUserMessageName(message, name, sizeof(name)))
            continue;

        HookUserMessage(message, OnUserMessage, true);
        g_HookedMessages++;
    }

    RegServerCmd("sm_test_usermsg_stack_alignment_status", Command_Status);
    LogMessage("Hooked %d user messages for pre-interception.", g_HookedMessages);
}

public Action Command_Status(int args)
{
    LogMessage("Intercepted %d user messages.", g_InterceptedMessages);
    return Plugin_Handled;
}

public Action OnUserMessage(UserMsg message, BfRead buffer, const int[] players,
                            int playersNum, bool reliable, bool init)
{
    int id = view_as<int>(message);
    g_InterceptedMessages++;

    // Log each message type once so normal game traffic does not flood the log.
    if (!g_SeenMessages[id])
    {
        char name[64];
        GetUserMessageName(message, name, sizeof(name));
        LogMessage("Intercepted %s (%d) for the first time.", name, id);
        g_SeenMessages[id] = true;
    }

    return Plugin_Continue;
}
