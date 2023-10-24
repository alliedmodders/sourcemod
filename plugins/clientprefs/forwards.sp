static GlobalForward OnCookiesCached;

void CreateForwards()
{
    OnCookiesCached = new GlobalForward("OnClientCookiesCached", ET_Ignore, Param_Cell);
}

void Call_OnCookiesCached(int client)
{
    Call_StartForward(OnCookiesCached);
    Call_PushCell(client);
    Call_Finish();
}
