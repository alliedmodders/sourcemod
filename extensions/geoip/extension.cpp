/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod GeoIP Extension
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
 * =============================================================================
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

#include <sourcemod_version.h>
#include "extension.h"
#include "geoip_util.h"

/**
 * @file extension.cpp
 * @brief Implement extension code here.
 */
GeoIP_Extension g_GeoIP;
MMDB_s mmdb;

SMEXT_LINK(&g_GeoIP);

bool GeoIP_Extension::SDK_OnLoad(char *error, size_t maxlength, bool late)
{
	if (mmdb.filename) // Already loaded.
	{
		return true;
	}

	const char *databases[] =
	{
		"GeoIP2-City",
		"GeoLite2-City",
		"GeoIP2-Country",
		"GeoLite2-Country"
	};

	char path[PLATFORM_MAX_PATH];
	int status;

	for (size_t i = 0; i < SM_ARRAYSIZE(databases); ++i)
	{
		g_pSM->BuildPath(Path_SM, path, sizeof(path), "configs/geoip/%s.mmdb", databases[i]);

		status = MMDB_open(path, MMDB_MODE_MMAP, &mmdb);

		if (status == MMDB_SUCCESS)
		{
			break;
		}
	
	}

	if (status != MMDB_SUCCESS)
	{
		ke::SafeStrcpy(error, maxlength, "Could not find GeoIP2 databases.");
		return false;
	}

	g_pShareSys->AddNatives(myself, geoip_natives);
	g_pShareSys->RegisterLibrary(myself, "GeoIP");

	char date[40];
	const time_t epoch = (const time_t)mmdb.metadata.build_epoch;
	strftime(date, 40, "%F %T UTC", gmtime(&epoch));

	g_pSM->LogMessage(myself, "GeoIP2 database loaded: %s (%s)", mmdb.metadata.database_type, date);

	char buf[64];
	for (size_t i = 0; i < mmdb.metadata.languages.count; i++)
	{
		if (i == 0)
		{
			strcpy(buf, mmdb.metadata.languages.names[i]);
		}
		else
		{
			strcat(buf, " ");
			strcat(buf, mmdb.metadata.languages.names[i]);
		}
	}

	g_pSM->LogMessage(myself, "GeoIP2 supported languages: %s", buf);

	return true;
}

void GeoIP_Extension::SDK_OnUnload()
{
	MMDB_close(&mmdb);
}

const char *GeoIP_Extension::GetExtensionVerString()
{
	return SOURCEMOD_VERSION;
}

const char *GeoIP_Extension::GetExtensionDateString()
{
	return SOURCEMOD_BUILD_TIME;
}

/*******************************
*                              *
* GEOIP NATIVE IMPLEMENTATIONS *
*                              *
*******************************/

inline void StripPort(char *ip)
{
	char *tmp = strchr(ip, ':');
	if (!tmp)
		return;
	*tmp = '\0';
}

static cell_t sm_Geoip_Code2(IPluginContext *pCtx, const cell_t *params)
{
	char *ip;
	pCtx->LocalToString(params[1], &ip);
	StripPort(ip);

	const char *path[] = {"country", "iso_code", NULL};
	std::string str = lookupString(ip, path);
	const char *ccode = str.c_str();

	pCtx->StringToLocal(params[2], 3, ccode);

	return (strlen(ccode) != 0) ? 1 : 0;
}

static cell_t sm_Geoip_Code3(IPluginContext *pCtx, const cell_t *params)
{
	char *ip;
	pCtx->LocalToString(params[1], &ip);
	StripPort(ip);

	const char *path[] = {"country", "iso_code", NULL};
	std::string str = lookupString(ip, path);
	const char *ccode = str.c_str();

	for (size_t i = 0; i < SM_ARRAYSIZE(GeoIPCountryCode); i++)
	{
		if (!strncmp(ccode, GeoIPCountryCode[i], 2))
		{
			ccode = GeoIPCountryCode3[i];
			break;
		}
	}

	pCtx->StringToLocal(params[2], 4, ccode);

	return (strlen(ccode) != 0) ? 1 : 0;
}

static cell_t sm_Geoip_RegionCode(IPluginContext *pCtx, const cell_t *params)
{
	char *ip;
	pCtx->LocalToString(params[1], &ip);
	StripPort(ip);

	const char *path[] = {"subdivisions", "0", "iso_code", NULL};
	std::string str = lookupString(ip, path);
	const char *ccode = str.c_str();

	pCtx->StringToLocal(params[2], 4, ccode);

	return (strlen(ccode) != 0) ? 1 : 0;
}

static cell_t sm_Geoip_ContinentCode(IPluginContext *pCtx, const cell_t *params)
{
	char *ip;
	pCtx->LocalToString(params[1], &ip);
	StripPort(ip);

	const char *path[] = {"continent", "code", NULL};
	std::string str = lookupString(ip, path);
	const char *ccode = str.c_str();

	pCtx->StringToLocal(params[2], 3, ccode);

	return (strlen(ccode) != 0) ? 1 : 0;
}

static cell_t sm_Geoip_Country(IPluginContext *pCtx, const cell_t *params)
{
	char *ip;
	pCtx->LocalToString(params[1], &ip);
	StripPort(ip);

	const char *path[] = {"country", "names", "en", NULL};
	std::string str = lookupString(ip, path);
	const char *ccode = str.c_str();

	pCtx->StringToLocal(params[2], params[3], ccode);

	return (strlen(ccode) != 0) ? 1 : 0;
}

static cell_t sm_Geoip_CountryEx(IPluginContext *pCtx, const cell_t *params)
{
	char *ip;
	char *lang;
	pCtx->LocalToString(params[1], &ip);
	pCtx->LocalToString(params[4], &lang);
	StripPort(ip);

	if (strlen(lang) == 0) strcpy(lang, "en");

	const char *path[] = {"country", "names", lang, NULL};
	std::string str = lookupString(ip, path);
	const char *ccode = str.c_str();

	pCtx->StringToLocal(params[2], params[3], ccode);

	return (strlen(ccode) != 0) ? 1 : 0;
}

static cell_t sm_Geoip_Continent(IPluginContext *pCtx, const cell_t *params)
{
	char *ip;
	char *lang;
	pCtx->LocalToString(params[1], &ip);
	pCtx->LocalToString(params[4], &lang);
	StripPort(ip);

	if (strlen(lang) == 0) strcpy(lang, "en");

	const char *path[] = {"continent", "names", lang, NULL};
	std::string str = lookupString(ip, path);
	const char *ccode = str.c_str();

	pCtx->StringToLocal(params[2], params[3], ccode);

	return (strlen(ccode) != 0) ? 1 : 0;
}

static cell_t sm_Geoip_Region(IPluginContext *pCtx, const cell_t *params)
{
	char *ip;
	char *lang;
	pCtx->LocalToString(params[1], &ip);
	pCtx->LocalToString(params[4], &lang);
	StripPort(ip);

	if (strlen(lang) == 0) strcpy(lang, "en");

	const char *path[] = {"subdivisions", "0", "names", lang, NULL};
	std::string str = lookupString(ip, path);
	const char *ccode = str.c_str();

	pCtx->StringToLocal(params[2], params[3], ccode);

	return (strlen(ccode) != 0) ? 1 : 0;
}

static cell_t sm_Geoip_City(IPluginContext *pCtx, const cell_t *params)
{
	char *ip;
	char *lang;
	pCtx->LocalToString(params[1], &ip);
	pCtx->LocalToString(params[4], &lang);
	StripPort(ip);

	if (strlen(lang) == 0) strcpy(lang, "en");

	const char *path[] = {"city", "names", lang, NULL};
	std::string str = lookupString(ip, path);
	const char *ccode = str.c_str();

	pCtx->StringToLocal(params[2], params[3], ccode);

	return (strlen(ccode) != 0) ? 1 : 0;
}

static cell_t sm_Geoip_Timezone(IPluginContext *pCtx, const cell_t *params)
{
	char *ip;
	pCtx->LocalToString(params[1], &ip);
	StripPort(ip);

	const char *path[] = {"location", "time_zone", NULL};
	std::string str = lookupString(ip, path);
	const char *ccode = str.c_str();

	pCtx->StringToLocal(params[2], params[3], ccode);

	return (strlen(ccode) != 0) ? 1 : 0;
}

const sp_nativeinfo_t geoip_natives[] = 
{
	{"GeoipCode2",			sm_Geoip_Code2},
	{"GeoipCode3",			sm_Geoip_Code3},
	{"GeoipRegionCode",		sm_Geoip_RegionCode},
	{"GeoipContinentCode",	sm_Geoip_ContinentCode},
	{"GeoipCountry",		sm_Geoip_Country},
	{"GeoipCountryEx",		sm_Geoip_CountryEx},
	{"GeoipContinent",		sm_Geoip_Continent},
	{"GeoipRegion",			sm_Geoip_Region},
	{"GeoipCity",			sm_Geoip_City},
	{"GeoipTimezone",		sm_Geoip_Timezone},
	{NULL,					NULL},
};

