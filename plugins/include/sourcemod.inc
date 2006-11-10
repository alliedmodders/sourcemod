/**
 * :TODO: license info
 */

#if defined _sourcemod_included
 #endinput
#endif
#define _sourcemod_included

struct Plugin
{
   const String:name[],			/* Plugin Name */
   const String:description[],		/* Plugin Description */
   const String:author[],		/* Plugin Author */
   const String:version[],		/* Plugin Version */
   const String:url[],			/* Plugin URL */
}

/**
 * Declare this as a struct in your plugin to expose its information.
 * Example:
 *
 * public Plugin:myinfo =
 * {
 *    name = "My Plugin",
 *    //etc
 * };
 */
public Plugin:myinfo;
 
/**
 * Called when the plugin is fully initialized and all known external references are resolved,
 * such as dynamic natives.
 *
 * @noreturn
 */
forward OnPluginInit();
 
/**
 * Called before OnPluginInit, in case the plugin wants to check for load failure.
 *
 * @param myself	Handle to the plugin.
 * @param late		Whether or not the plugin was loaded "late" (after map load).
 * @param error		Error message buffer in case load failed.
 * @param err_max	Maximum number of characters for error message buffer.
 * @return		True if load success, false otherwise.
 */
forward bool:OnPluginLoad(Handle:myself, bool:late, String:error[], err_max);

/**
 * Called when the plugin is first mapped into memory.  Use this to set dynamic natives ONLY.
 *
 * @noreturn
 */
forward OnCreatePlugin();

/**
 * Called when the plugin is about to be unloaded.
 *
 * @noreturn
 */
forward OnPluginUnload();

/**
 * Called when the plugin's pause status is changing.
 *
 * @noreturn
 */
forward OnPluginPauseChange(bool:pause);
