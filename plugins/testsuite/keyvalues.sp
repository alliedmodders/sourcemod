
#include <sourcemod>

public Plugin:myinfo =
{
	name		= "KeyValues test",
	author		= "AlliedModders LLC",
	description	= "KeyValues test",
	version		= SOURCEMOD_VERSION,
	url			= "http://www.sourcemod.net/"
};


public OnPluginStart()
{
	RegServerCmd("test_keyvalues", RunTests);
}

public Action:RunTests(argc)
{
	new String:validKv[] =
		"\"root\" \
		{ \
			\"child\" \"value\" \
			\"subkey\" { \
				subchild subvalue \
				subfloat 1.0 \
			} \
		}";

	new Handle:kv = CreateKeyValues("");

	if (!StringToKeyValues(kv, validKv))
		ThrowError("Valid kv not read correctly!");

	decl String:value[128];
	KvGetString(kv, "child", value, sizeof(value));

	if (!StrEqual(value, "value"))
		ThrowError("Child kv should have 'value' but has: '%s'", value);

	if (!KvJumpToKey(kv, "subkey"))
		ThrowError("No sub kv subkey exists!");

	KvGetString(kv, "subchild", value, sizeof(value));

	if (!StrEqual(value, "subvalue"))
		ThrowError("Subkv subvalue should have 'subvalue' but has: '%s'", value);

	new Float:subfloat = KvGetFloat(kv, "subfloat");

	if (subfloat != 1.0)
		ThrowError( "Subkv subfloat should have 1.0 but has: %f", subfloat)

	CloseHandle(kv);

	PrintToServer("KeyValue tests passed!");
}
