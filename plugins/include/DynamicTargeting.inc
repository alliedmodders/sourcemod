#if defined _DynamicTargeting_Included
	#endinput
#endif
#define _DynamicTargeting_Included

native int AmbiguousMenu(int client, char[] sCommand, char[] sArgString, char[] sPattern, int FilterFlags);

public SharedPlugin __pl_DynamicTargeting =
{
	name = "DynamicTargeting",
	file = "DynamicTargeting.smx",
#if defined REQUIRE_PLUGIN
	required = 1,
#else
	required = 0,
#endif
};

#if !defined REQUIRE_PLUGIN
public __pl_DynamicTargeting_SetNTVOptional()
{
	MarkNativeAsOptional("AmbiguousMenu");
}
#endif
