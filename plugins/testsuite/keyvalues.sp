
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

	KeyValues kv = CreateKeyValues("");

	if (!kv.ImportFromString(validKv))
		ThrowError("Valid kv not read correctly!");

	char value[128];
	kv.GetString("child", value, sizeof(value));

	if (!StrEqual(value, "value"))
		ThrowError("Child kv should have 'value' but has: '%s'", value);

	if (!kv.JumpToKey("subkey"))
		ThrowError("No sub kv subkey exists!");

	kv.GetString("subchild", value, sizeof(value));

	if (!StrEqual(value, "subvalue"))
		ThrowError("Subkv subvalue should have 'subvalue' but has: '%s'", value);

	float subfloat = kv.GetFloat("subfloat");

	if (subfloat != 1.0)
		ThrowError( "Subkv subfloat should have 1.0 but has: %f", subfloat)

	delete kv;

	PrintToServer("KeyValue tests passed!");
}
