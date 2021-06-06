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

enum Continent
{
	CONTINENT_UNKNOWN = 0,
	CONTINENT_AFRICA,
	CONTINENT_ANTARCTICA,
	CONTINENT_ASIA,
	CONTINENT_EUROPE,
	CONTINENT_NORTH_AMERICA,
	CONTINENT_OCEANIA,
	CONTINENT_SOUTH_AMERICA,
};

// The system of measurement for calculate the distance between geographical coordinates
#define SYSTEM_METRIC   0 // kilometers
#define SYSTEM_IMPERIAL 1 // statute miles

#include <core>

/**
 * @section IP addresses can contain ports, the ports will be stripped out.
 */

/**
 * Gets the two character country code from an IP address. (US, CA, etc)
 *
 * @param ip            Ip to determine the country code.
 * @param ccode         Destination string buffer to store the code.
 * @return              True on success, false otherwise.
 */
native bool GeoipCode2(const char[] ip, char ccode[3]);

/**
 * Gets the three character country code from an IP address. (USA, CAN, etc)
 *
 * @param ip            Ip to determine the country code.
 * @param ccode         Destination string buffer to store the code.
 * @return              True on success, false otherwise.
 */
native bool GeoipCode3(const char[] ip, char ccode[4]);

/**
 * Gets the region code with country code from an IP address. (US-IL, CH-CHE, etc)
 *
 * @param ip            Ip to determine the region code.
 * @param ccode         Destination string buffer to store the code.
 * @return              True on success, false otherwise.
 */
native bool GeoipRegionCode(const char[] ip, char ccode[12]);

/**
 * Gets the two character continent code from an IP address. (EU, AS, etc)
 *
 * @param ip            Ip to determine the continent code.
 * @param ccode         Destination string buffer to store the code.
 * @return              The continent id on success, 0 otherwise.
 */
native Continent GeoipContinentCode(const char[] ip, char ccode[3]);

/**
 * Gets the full country name.
 *
 * @param ip            Ip to determine the country code.
 * @param name          Destination string buffer to store the country name.
 * @param maxlength     Maximum length of output string buffer.
 * @return              True on success, false otherwise.
 */
native bool GeoipCountry(const char[] ip, char[] name, int maxlength);

/**
 * Gets the full country name.
 *
 * @param ip            Ip to determine the country code.
 * @param name          Destination string buffer to store the country name.
 * @param maxlength     Maximum length of output string buffer.
 * @param client        Client index in order to return the result in the player's language
 *                      -1: the default language, which is english.
 *                      0: the server language. You can use LANG_SERVER define.
 *                      >=1: the player's language.
 * @return              True on success, false otherwise.
 */
native bool GeoipCountryEx(const char[] ip, char[] name, int maxlength, int client = -1);

/**
 * Gets the full continent name.
 *
 * @param ip            Ip to determine the continent code.
 * @param name          Destination string buffer to store the continent name.
 * @param maxlength     Maximum length of output string buffer.
 * @param client        Client index in order to return the result in the player's language
 *                      -1: the default language, which is english.
 *                      0: the server language. You can use LANG_SERVER define.
 *                      >=1: the player's language.
 * @return              True on success, false otherwise.
 */
native bool GeoipContinent(const char[] ip, char[] name, int maxlength, int client = -1);

/**
 * Gets the full region name.
 *
 * @param ip            Ip to determine the region code.
 * @param name          Destination string buffer to store the region name.
 * @param maxlength     Maximum length of output string buffer.
 * @param client        Client index in order to return the result in the player's language
 *                      -1: the default language, which is english.
 *                      0: the server language. You can use LANG_SERVER define.
 *                      >=1: the player's language.
 * @return              True on success, false otherwise.
 */
native bool GeoipRegion(const char[] ip, char[] name, int maxlength, int client = -1);

/**
 * Gets the city name.
 *
 * @param ip            Ip to determine the city code.
 * @param name          Destination string buffer to store the city name.
 * @param maxlength     Maximum length of output string buffer.
 * @param client        Client index in order to return the result in the player's language
 *                      -1: the default language, which is english.
 *                      0: the server language. You can use LANG_SERVER define.
 *                      >=1: the player's language.
 * @return              True on success, false otherwise.
 */
native bool GeoipCity(const char[] ip, char[] name, int maxlength, int client = -1);

/**
 * Gets the timezone.
 *
 * @param ip            Ip to determine the timezone.
 * @param name          Destination string buffer to store the timezone.
 * @param maxlength     Maximum length of output string buffer.
 * @return              True on success, false otherwise.
 */
native bool GeoipTimezone(const char[] ip, char[] name, int maxlength);

/**
 * Gets the city's latitude
 *
 * @param ip            Ip to determine the city latitude.
 * @return              The result of the latitude, 0 if latitude is not found
 */
native float GeoipLatitude(const char[] ip);

/**
 * Gets the city's longitude
 *
 * @param ip            Ip to determine the city longitude.
 * @return              The result of the longitude, 0 if longitude is not found
 */
native float GeoipLongitude(const char[] ip);

/*
 * Calculate the distance between geographical coordinates, latitude and longitude.
 *
 * @param lat1          The first IP latitude.
 * @param lon1          The first IP longitude.
 * @param lat2          The second IP latitude.
 * @param lon2          The second IP longitude.
 * @param system        The system of measurement, 0 = Metric(kilometers) or 1 = English(miles).
 *
 * @return              The distance as result in specified system of measurement.
 */
native float GeoipDistance(float lat1, float lon1, float lat2, float lon2, int system = SYSTEM_METRIC);

/**
 * @endsection
 */

/**
 * Do not edit below this line!
 */
public Extension __ext_geoip = 
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
public void __ext_geoip_SetNTVOptional()
{
	MarkNativeAsOptional("GeoipCode2");
	MarkNativeAsOptional("GeoipCode3");
	MarkNativeAsOptional("GeoipRegionCode");
	MarkNativeAsOptional("GeoipContinentCode");
	MarkNativeAsOptional("GeoipCountry");
	MarkNativeAsOptional("GeoipCountryEx");
	MarkNativeAsOptional("GeoipContinent");
	MarkNativeAsOptional("GeoipRegion");
	MarkNativeAsOptional("GeoipCity");
	MarkNativeAsOptional("GeoipTimezone");
	MarkNativeAsOptional("GeoipLatitude");
	MarkNativeAsOptional("GeoipLongitude");
	MarkNativeAsOptional("GeoipDistance");
}
#endif
