#ifndef MAXMINDDB_CONFIG_H
#define MAXMINDDB_CONFIG_H

#include "osdefs.h" // BYTE_ORDER, LITTLE_ENDIAN

/* This fixes a behavior change in after https://github.com/maxmind/libmaxminddb/pull/123. */
#if defined(BYTE_ORDER) && BYTE_ORDER == LITTLE_ENDIAN
	#define MMDB_LITTLE_ENDIAN 1
#endif

#define MMDB_UINT128_USING_MODE 	0
#define MMDB_UINT128_IS_BYTE_ARRAY 	1

#define PACKAGE_VERSION 		"1.5.2"

#endif                          /* MAXMINDDB_CONFIG_H */
