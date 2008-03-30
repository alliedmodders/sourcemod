#if defined _tf2_included
 #endinput
#endif
#define _tf2_included


native TF2_Invuln(client, bool:something, bool:anothersomething);

native TF2_Burn(client);

/**
 * Do not edit below this line!
 */
public Extension:__ext_tf2 = 
{
	name = "TF2 Tools",
	file = "game.tf2.ext",
	autoload = 0,
#if defined REQUIRE_EXTENSIONS
	required = 1,
#else
	required = 0,
#endif
};
