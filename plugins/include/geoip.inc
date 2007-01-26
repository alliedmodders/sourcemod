/**
 * ===============================================================
 * SourceMod (C)2004-2007 AlliedModders LLC.  All rights reserved.
 * ===============================================================
 *
 *  This file is part of the SourceMod/SourcePawn SDK.  This file may only be used 
 * or modified under the Terms and Conditions of its License Agreement, which is found 
 * in LICENSE.txt.  The Terms and Conditions for making SourceMod extensions/plugins 
 * may change at any time.  To view the latest information, see:
 *   http://www.sourcemod.net/license.php
 *
 * Version: $Id$
 */

#if defined _geoip_included
  #endinput
#endif
#define _geoip_included

#include <core>

/**
 * @GLOBAL@
 *   IP address can contain ports, the ports will be stripped out.
 */

/**
 * Gets the two character country code from an IP address. (US, CA, etc)
 *
 * @param ip			Ip to determine the country code.
 * @param ccode			Destination string buffer to store the code.
 * @noreturn
 */
native GeoipCode2(const String:ip[], String:ccode[3]);

/**
 * Gets the three character country code from an IP address. (USA, CAN, etc)
 *
 * @param ip			Ip to determine the country code.
 * @param ccode			Destination string buffer to store the code.
 * @noreturn
 */
native GeoipCode3(const String:ip[], String:ccode[4]);

/**
 * Gets the full country name. (max length of output string is 45)
 *
 * @param ip			Ip to determine the country code.
 * @param ccode			Destination string buffer to store the country name.
 * @param len			Maximum length of output string buffer.
 * @noreturn
 */
native GeoipCountry(const String:ip[], String:name[], len=45);

/**
 * Do not edit below this line!
 */
public Extension:__ext_geoip = 
{
	name = "GeoIP",
	file = "geoip.ext",
#if defined AUTOLOAD_EXTENSIONS
	autoload = 1,
#else
	autoload = 0,
#endif
#if defined REQUIRE_EXTENSIONS
	required = 1,
#else
	required = 0,
#endif
};

