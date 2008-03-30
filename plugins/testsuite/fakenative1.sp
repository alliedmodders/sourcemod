#include <sourcemod>

public Plugin:myinfo = 
{
	name = "FakeNative Testing Lab #1",
	author = "AlliedModders LLC",
	description = "Test suite #1 for dynamic natives",
	version = "1.0.0.0",
	url = "http://www.sourcemod.net/"
};

public bool:AskPluginLoad(Handle:myself, bool:late, String:error[], err_max)
{
	CreateNative("TestNative1", __TestNative1);
	CreateNative("TestNative2", __TestNative2);
	CreateNative("TestNative3", __TestNative3);
	CreateNative("TestNative4", __TestNative4);
	CreateNative("TestNative5", __TestNative5);
	return true;
}

public __TestNative1(Handle:plugin, numParams)
{
	PrintToServer("TestNative1: Plugin: %x params: %d", plugin, numParams);
	if (numParams == 4)
	{
		ThrowNativeError(SP_ERROR_NATIVE, "Four parameters ARE NOT ALLOWED lol");
	}
	return 5;
}

public __TestNative2(Handle:plugin, numParams)
{
	new String:buffer[512];
	new bytes;
	
	GetNativeString(1, buffer, sizeof(buffer), bytes);
	
	for (new i=0; i<bytes; i++)
	{
		buffer[i] = buffer[i] + 1;
	}
	
	SetNativeString(1, buffer, bytes+1);
}

public __TestNative3(Handle:plugin, numParams)
{
	new val1 = GetNativeCell(1);
	new val2 = GetNativeCellRef(2);
	
	SetNativeCellRef(2, val1+val2);
}

public __TestNative4(Handle:plugin, numParams)
{
	new size = GetNativeCell(3);
	new local[size];
	
	GetNativeArray(1, local, size);
	SetNativeArray(2, local, size);
}

public __TestNative5(Handle:plugin, numParams)
{
	new local = GetNativeCell(1);
	
	if (local)
	{
		FormatNativeString(2, 4, 5, GetNativeCell(3));
	} else {
		new len = GetNativeCell(3);
		new fmt_len;
		new String:buffer[len];
			
		GetNativeStringLength(4, fmt_len);
		
		new String:format[fmt_len];
		
		GetNativeString(2, buffer, len);
		GetNativeString(4, format, fmt_len);
		
		FormatNativeString(0, 0, 5, len, _, buffer, format);
	}
}
