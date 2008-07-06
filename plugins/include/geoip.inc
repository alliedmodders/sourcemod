/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod (C)2004-2008 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This file is part of the SourceMod/SourcePawn SDK.
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */

#if defined _geoip_included
  #endinput
#endif
#define _geoip_included

#include <core>

/**
 * @section IP addresses can contain ports, the ports will be stripped out.
 */

/**
 * Gets the two character country code from an IP address. (US, CA, etc)
 *
 * @param ip			Ip to determine the country code.
 * @param ccode			Destination string buffer to store the code.
 * @return				True on success, false if no country found.
 */
native bool:GeoipCode2(const String:ip[], String:ccode[3]);

/**
 * Gets the three character country code from an IP address. (USA, CAN, etc)
 *
 * @param ip			Ip to determine the country code.
 * @param ccode			Destination string buffer to store the code.
 * @return				True on success, false if no country found.
 */
native bool:GeoipCode3(const String:ip[], String:ccode[4]);

/**
 * Gets the full country name. (max length of output string is 45)
 *
 * @param ip			Ip to determine the country code.
 * @param ccode			Destination string buffer to store the country name.
 * @param len			Maximum length of output string buffer.
 * @return				True on success, false if no country found.
 */
native bool:GeoipCountry(const String:ip[], String:name[], maxlength);

/**
 * @endsection
 */

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

#if !defined REQUIRE_EXTENSIONS
public __ext_geoip_SetNTVOptional()
{
	MarkNativeAsOptional("GeoipCode2");
	MarkNativeAsOptional("GeoipCode3");
	MarkNativeAsOptional("GeoipCountry");
}
#endif
