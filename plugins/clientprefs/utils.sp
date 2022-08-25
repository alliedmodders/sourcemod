int FindClientByAuthId(const char[] authId)
{
    for (int i = 1; i <= MaxClients; i++)
    {
        if (!IsFakeClient(i) && IsClientConnected(i))
        {
            char targetAuthId[32];

            bool gotAuth = GetClientAuthId(i, AuthId_Steam2, targetAuthId, sizeof(targetAuthId));
            if (!gotAuth)
            {
                continue;
            }

            if (StrEqual(targetAuthId, authId))
            {
                return i;
            }
        }
    }

    return -1;
}
