#include <sourcemod>
#include <clientprefs>

#pragma semicolon 1
#pragma newdecls required

Cookie g_Cookie1;
Cookie g_Cookie2;
Cookie g_Cookie3;
Cookie g_Cookie4;
Cookie g_Cookie5;

public void OnPluginStart()
{
	g_Cookie1 = RegClientCookie("test-cookie'", "A basic 'testing cookie", CookieAccess_Public);
	g_Cookie2 = RegClientCookie("test-cookie2\"", "A basic \"testing cookie", CookieAccess_Protected);
	g_Cookie3 = RegClientCookie("test-cookie3", "A basic testing cookie", CookieAccess_Public);
	g_Cookie4 = RegClientCookie("test-cookie4", "A basic testing cookie", CookieAccess_Private);
	g_Cookie5 = RegClientCookie("test-cookie5", "A basic testing cookie", CookieAccess_Public);
	
	g_Cookie1.SetPrefabMenu(CookieMenu_YesNo, "Cookie '1", CookieSelected, g_Cookie1);
	g_Cookie2.SetPrefabMenu(CookieMenu_YesNo_Int, "Cookie \"2");
	g_Cookie3.SetPrefabMenu(CookieMenu_OnOff, "Cookie 3");
	g_Cookie4.SetPrefabMenu(CookieMenu_OnOff_Int, "Cookie 4");
	
	SetCookieMenuItem(CookieSelected, g_Cookie5, "Get Cookie 5 value");
}

public void CookieSelected(int client, CookieMenuAction action, any info, char[] buffer, int maxlen)
{
	if (action == CookieMenuAction_DisplayOption)
	{
		PrintToChat(client, "About to draw item. Current text is : %s", buffer);
		FormatEx(buffer, maxlen, "HELLLLLLLLLLO");
	}
	else
	{
		LogMessage("SELECTED!");
		
		char value[100];
		GetClientCookie(client, info, value, sizeof(value));
		PrintToChat(client, "Value is : %s", value);
	}
}

public bool OnClientConnect(int client, char[] rejectmsg, int maxlen)
{
	LogMessage("Connect Cookie state: %s", AreClientCookiesCached(client) ? "YES" : "NO");
	
	return true;
}

public void OnClientCookiesCached(int client)
{
	LogMessage("Loaded Cookie state: %s", AreClientCookiesCached(client) ? "YES" : "NO");
	
	char value[100];
	g_Cookie1.Get(client, value, sizeof(value));
	LogMessage("Test before set: %s", value);
	g_Cookie1.Set(client, "somethingsomething'");
	g_Cookie1.Get(client, value, sizeof(value));
	LogMessage("Test after set: %s", value);
}
