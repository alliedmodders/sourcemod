#include <sourcemod>

public OnPluginStart()
{
	RegServerCmd("test_goto", Test_Goto);
}

public Action:Test_Goto(args)
{
	new bool:hello = false;
	new String:crab[] = "space crab";

sample_label:

	new String:yam[] = "yams";

	PrintToServer("%s %s", crab, yam);

	if (!hello)
	{
		new bool:what = true;
		hello = what
		goto sample_label;
	}

	return Plugin_Handled;
}

