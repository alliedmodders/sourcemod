/* Copyright Abandoned 1996, 1999, 2001 MySQL AB
   This file is public domain and comes with NO WARRANTY of any kind */

/* Version numbers for protocol & mysqld */

#ifndef _mariadb_version_h_
#define _mariadb_version_h_

#ifdef _CUSTOMCONFIG_
#include <custom_conf.h>
#else
#define PROTOCOL_VERSION		10
#define MARIADB_CLIENT_VERSION_STR	"10.8.8"
#define MARIADB_BASE_VERSION		"mariadb-10.8"
#define MARIADB_VERSION_ID		100808
#define MARIADB_PORT	        	3306
#define MARIADB_UNIX_ADDR               "/tmp/mysql.sock"
#ifndef MYSQL_UNIX_ADDR
#define MYSQL_UNIX_ADDR MARIADB_UNIX_ADDR
#endif
#ifndef MYSQL_PORT
#define MYSQL_PORT MARIADB_PORT
#endif

#define MYSQL_CONFIG_NAME               "my"
#define MYSQL_VERSION_ID                100808
#define MYSQL_SERVER_VERSION            "10.8.8-MariaDB"

#define MARIADB_PACKAGE_VERSION "3.3.8"
#define MARIADB_PACKAGE_VERSION_ID 30308
#if defined(__linux__)
#define MARIADB_SYSTEM_TYPE "Linux"
#define MARIADB_PLUGINDIR "/usr/local/lib/mariadb/plugin"
#elif defined(_WIN32)
#define MARIADB_SYSTEM_TYPE "Windows"
#define MARIADB_PLUGINDIR "C:/Program Files (x86)/mariadb-connector-c/lib/mariadb/plugin"
#endif

#if defined(__x86_64__) || defined(_M_X64)
#define MARIADB_MACHINE_TYPE "x86_64"
#elif defined(i386) || defined(__i386__) || defined(__i386) || defined(_M_IX86)
#define MARIADB_MACHINE_TYPE "x86"
#endif

/* mysqld compile time options */
#ifndef MYSQL_CHARSET
#define MYSQL_CHARSET			""
#endif
#endif

/* Source information */
#define CC_SOURCE_REVISION "458a4396b443dcefedcf464067560078aa09d8b4"

#endif /* _mariadb_version_h_ */