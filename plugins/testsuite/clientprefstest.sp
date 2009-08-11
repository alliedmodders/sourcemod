#include <sourcemod>
#include <clientprefs.inc>

new Handle:g_Cookie;
new Handle:g_Cookie2;
new Handle:g_Cookie3;
new Handle:g_Cookie4;
new Handle:g_Cookie5;

public OnPluginStart()
{
	g_Cookie = RegClientCookie("test-cookie'", "A basic 'testing cookie", CookieAccess_Public);
	g_Cookie2 = RegClientCookie("test-cookie2\"", "\"A basic testing cookie", CookieAccess_Protected);
	g_Cookie3 = RegClientCookie("test-cookie3", "A basic testing cookie", CookieAccess_Public);
	g_Cookie4 = RegClientCookie("test-cookie4", "A basic testing cookie", CookieAccess_Private);
	
	g_Cookie5 = RegClientCookie("test-cookie5", "A basic testing cookie", CookieAccess_Public);
	
	SetCookiePrefabMenu(g_Cookie, CookieMenu_YesNo, "Cookie '1", CookieSelected, any:g_Cookie);
	SetCookiePrefabMenu(g_Cookie2, CookieMenu_YesNo_Int, "Cookie 2");
	SetCookiePrefabMenu(g_Cookie3, CookieMenu_OnOff, "Cookie 3");
	SetCookiePrefabMenu(g_Cookie4, CookieMenu_OnOff_Int, "Cookie 4");
	
	SetCookieMenuItem(CookieSelected, g_Cookie5, "Get Cookie 5 value");	
}

public CookieSelected(client, CookieMenuAction:action, any:info, String:buffer[], maxlen)
{
	if (action == CookieMenuAction_DisplayOption)
	{
		PrintToChat(client, "About to draw item. Current text is : %s", buffer);
		Format(buffer, maxlen, "HELLLLLLLLLLO");
	}
	else
	{
		LogMessage("SELECTED!");
	
		new String:value[100];
		GetClientCookie(client, info, value, sizeof(value));
		PrintToChat(client, "Value is : %s", value);
	}
}

public bool:OnClientConnect(client, String:rejectmsg[], maxlen)
{
	LogMessage("Connect Cookie state: %s", AreClientCookiesCached(client) ? "YES" : "NO");
	
	return true;
}

public OnClientCookiesCached(client)
{
	LogMessage("Loaded Cookie state: %s", AreClientCookiesCached(client) ? "YES" : "NO");
	
	new String:hi[100];
	GetClientCookie(client, g_Cookie, hi, sizeof(hi));
	LogMessage("Test: %s",hi);
	SetClientCookie(client, g_Cookie, "somethingsomething'");
	GetClientCookie(client, g_Cookie, hi, sizeof(hi));
	LogMessage("Test: %s",hi);	
}
