/**
 * SourceMod Threading Extension, (C)2007 AlliedModders LLC.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Version: $Id$
 */
#ifndef _INCLUDE_SOURCEMOD_EXTENSION_CONFIG_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_CONFIG_H_

#include "svn_version.h"

/* Basic information exposed publically */
#define SMEXT_CONF_NAME			"Threader"
#define SMEXT_CONF_DESCRIPTION	"Provides threading to other modules"
#define SMEXT_CONF_VERSION		SVN_FULL_VERSION
#define SMEXT_CONF_AUTHOR		"AlliedModders"
#define SMEXT_CONF_URL			"http://www.sourcemod.net/"
#define SMEXT_CONF_LOGTAG		"THREADER"
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
//#define SMEXT_CONF_METAMOD		

#endif //_INCLUDE_SOURCEMOD_EXTENSION_CONFIG_H_
