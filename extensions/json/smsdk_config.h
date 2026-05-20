#ifndef _INCLUDE_SOURCEMOD_EXTENSION_CONFIG_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_CONFIG_H_

#define SMEXT_CONF_NAME			"SourceMod JSON Extension"
#define SMEXT_CONF_DESCRIPTION	"Provide JSON Native"
#define SMEXT_CONF_VERSION		"1.1.5e"
#define SMEXT_CONF_AUTHOR		"ProjectSky"
#define SMEXT_CONF_URL			"https://github.com/ProjectSky/sm-ext-yyjson"
#define SMEXT_CONF_LOGTAG		"json"
#define SMEXT_CONF_LICENSE		"GPL"
#define SMEXT_CONF_DATESTRING	__DATE__

#define SMEXT_LINK(name) SDKExtension *g_pExtensionIface = name;

#define SMEXT_ENABLE_HANDLESYS

#endif // _INCLUDE_SOURCEMOD_EXTENSION_CONFIG_H_