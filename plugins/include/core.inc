#if defined _core_included
 #endinput
#endif
#define _core_included

#define SOURCEMOD_PLUGINAPI_VERSION		1
struct PlVers
{
	version,
};

public PlVers:__version = 
{
	version = SOURCEMOD_PLUGINAPI_VERSION,
};

struct Extension
{
	const String:name[],	/* Short name */
	const String:file[],	/* Default file name */
	bool:autoload,			/* Whether or not to auto-load */
	bool:required,			/* Whether or not to require */
};

#define AUTOLOAD_EXTENSIONS
#define REQUIRE_EXTENSIONS

