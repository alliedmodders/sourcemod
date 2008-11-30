#include <sourcemod>

public Plugin:myinfo = 
{
	name = "File test",
	author = "AlliedModders LLC",
	description = "Tests file functions",
	version = "1.0.0.0",
	url = "http://www.sourcemod.net/"
};

public OnPluginStart()
{
	RegServerCmd("test_fread1", Test_ReadBinStr);
}

Handle:OpenFile2(const String:path[], const String:mode[])
{
	new Handle:file = OpenFile(path, mode);
	if (file == INVALID_HANDLE)
		PrintToServer("Failed to open file %s for %s", path, mode);
	else
		PrintToServer("Opened file handle %x: %s", file, path);
	return file;
}

public Action:Test_ReadBinStr(args)
{
	new items[] = {1, 3, 5, 7, 0, 92, 193, 26, 0, 84, 248, 2};
	new Handle:of = OpenFile2("smbintest", "wb");
	if (of == INVALID_HANDLE)
		return Plugin_Handled;
	WriteFile(of, items, sizeof(items), 1);
	CloseHandle(of);

	new Handle:inf = OpenFile2("smbintest", "rb");
	new String:buffer[sizeof(items)];
	ReadFileString(inf, buffer, sizeof(items), sizeof(items));
	FileSeek(inf, 0, SEEK_SET);
	new items2[sizeof(items)];
	ReadFile(inf, items2, sizeof(items), 1);
	CloseHandle(inf);

	for (new i = 0; i < sizeof(items); i++)
	{
		if (buffer[i] != items[i])
		{
			PrintToServer("FAILED ON INDEX %d: %d != %d", i, buffer[i], items[i]);
			return Plugin_Handled;
		}
		else if (items2[i] != items[i])
		{
			PrintToServer("FAILED ON INDEX %d: %d != %d", i, items2[i], items[i]);
			return Plugin_Handled;
		}
	}

	PrintToServer("Test passed!");

	return Plugin_Handled;
}

