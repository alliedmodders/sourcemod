#include <sourcemod>
#include "../include/clientprefs.inc"

new Handle:g_Cookie;

public OnPluginStart()
{
		g_Cookie = RegClientPrefCookie("test-cookie", "A basic testing cookie");
}

public  bool:OnClientConnect(client, String:rejectmsg[], maxlen)
{
	LogMessage("Connect Cookie state: %s", AreClientCookiesCached(client) ? "YES" : "NO");
}

public OnClientCookiesLoaded(client)
{
	LogMessage("Loaded Cookie state: %s", AreClientCookiesCached(client) ? "YES" : "NO");
	
	new String:hi[100];
	GetClientPrefCookie(client, g_Cookie, hi, sizeof(hi));
	LogMessage("Test: %s",hi);
	SetClientPrefCookie(client, g_Cookie, "somethingsomething");
	GetClientPrefCookie(client, g_Cookie, hi, sizeof(hi));
	LogMessage("Test: %s",hi);	
}