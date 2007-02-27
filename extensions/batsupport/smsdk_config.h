// vim: set ts=4 :
#ifndef _INCLUDE_SOURCEMOD_EXTENSION_CONFIG_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_CONFIG_H_

/**
 * @file smsdk_config.h
 * @brief Contains macros for configuring basic extension information.
 */

/* Basic information exposed publically */
#define SMEXT_CONF_NAME			"BAT Support"
#define SMEXT_CONF_DESCRIPTION	"Adds support for BAT's AdminInterface"
#define SMEXT_CONF_VERSION		"1.0.0.0"
#define SMEXT_CONF_AUTHOR		"AlliedModders"
#define SMEXT_CONF_URL			"http://www.sourcemod.net/"
#define SMEXT_CONF_LOGTAG		"BATSUPPORT"
#define SMEXT_CONF_LICENSE		"GPL"
#define SMEXT_CONF_DATESTRING	__DATE__

/** 
 * @brief Exposes plugin's main interface.
 */
#define SMEXT_LINK(name) SDKExtension *g_pExtensionIface = name;

/**
 * @brief Sets whether or not this plugin required Metamod.
 * NOTE: Uncomment to enable, comment to disable.
 * NOTE: This is enabled automatically if a Metamod build is chosen in 
 *		 the Visual Studio project.
 */
#define SMEXT_CONF_METAMOD		

#endif //_INCLUDE_SOURCEMOD_EXTENSION_CONFIG_H_
