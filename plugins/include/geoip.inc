/**
 * :TODO: license info
 */

#if defined _geoip_included
  #endinput
#endif
#define _geoip_included

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
native Geoip_Code2(const String:ip[], String:ccode[3]);

/**
 * Gets the three character country code from an IP address. (USA, CAN, etc)
 *
 * @param ip			Ip to determine the country code.
 * @param ccode			Destination string buffer to store the code.
 * @noreturn
 */
native Geoip_Code3(const String:ip[], String:ccode[4]);

/**
 * Gets the full country name. (max length of output string is 45)
 *
 * @param ip			Ip to determine the country code.
 * @param ccode			Destination string buffer to store the country name.
 * @param len			Maximum length of output string buffer.
 * @noreturn
 */
native Geoip_Country(const String:ip[], String:name[], len=45);
