#include <sourcemod>

public Plugin:myinfo = 
{
	name = "File test",
	author = "AlliedModders LLC",
	description = "Tests file functions",
	version = "1.0.0.0",
	url = "http://www.sourcemod.net/"
};

public void OnPluginStart()
{
	RegServerCmd("test_fread1", Test_ReadBinStr);
}

File OpenFile2(const String:path[], const String:mode[])
{
	File file = OpenFile(path, mode);
	if (!file)
		PrintToServer("Failed to open file %s for %s", path, mode);
	else
		PrintToServer("Opened file handle %x: %s", file, path);
	return file;
}

public Action Test_ReadBinStr(args)
{
	int items[] = {1, 3, 5, 7, 0, 92, 193, 26, 0, 84, 248, 2};
	File of = OpenFile2("smbintest", "wb");
	if (!of)
		return Plugin_Handled;
	of.Write(items, sizeof(items), 1);
	of.Close();

	File inf = OpenFile2("smbintest", "rb");
	char buffer[sizeof(items)];
	inf.ReadString(buffer, sizeof(items), sizeof(items));
	inf.Seek(0, SEEK_SET);
	int items2[sizeof(items)];
	inf.Read(items2, sizeof(items), 1);
	inf.Close();

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

	inf = OpenFile2("smbintest", "rb");
	for (new i = 0; i < sizeof(items); i++)
	{
		new item;
		if (!inf.ReadInt8(item) || item != items[i])
		{
			PrintToServer("FAILED ON INDEX %d: %d != %d", i, item, items[i]);
			return Plugin_Handled;
		}
	}

	PrintToServer("Test passed!");

	return Plugin_Handled;
}

