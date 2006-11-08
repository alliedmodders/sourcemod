/**
 * :TODO: license info
 */

#if defined _sourcemod_included
 #endinput
#endif
#define _sourcemod_included

/**
 * Note: A plugin can define the following public strings for external information:
 * public String:PLUGIN_AUTHOR[]	- Author
 * public String:PLUGIN_NAME[]		- Name/Title
 * public String:PLUGIN_DESCRIPTION[]	- Short Description
 * public String:PLUGIN_VERSION[]	- Version information
 * public String:PLUGIN_URL[]		- Website URL
 */

 
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
 * Called when the plugin is first mapped into memory.  Use this to set dynamic natives.
 *
 * @noreturn
 */
forward OnPluginCreated();

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
