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

#define _USE_MATH_DEFINES

#include <sourcemod_version.h>
#include <cmath>
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

	char m_GeoipDir[PLATFORM_MAX_PATH];
	g_pSM->BuildPath(Path_SM, m_GeoipDir, sizeof(m_GeoipDir), "configs/geoip");

	bool hasEntry = false;

	IDirectory *dir = libsys->OpenDirectory(m_GeoipDir);
	if (dir) 
	{
		while (dir->MoreFiles())
		{
			if (dir->IsEntryFile())
			{
				const char *name = dir->GetEntryName();
				size_t len = strlen(name);
				if (len >= 5 && strcmp(&name[len-5], ".mmdb") == 0)
				{
					char database[PLATFORM_MAX_PATH];
					libsys->PathFormat(database, sizeof(database), "%s/%s", m_GeoipDir, name);

					int status = MMDB_open(database, MMDB_MODE_MMAP, &mmdb);

					if (status != MMDB_SUCCESS)
					{
						ke::SafeStrcpy(error, maxlength, "Failed to open GeoIP2 database.");
						libsys->CloseDirectory(dir);
						return false;
					}

					hasEntry = true;
					break;
				}
			}
			dir->NextEntry();
		}
		libsys->CloseDirectory(dir);
	}

	if (!hasEntry)
	{
		ke::SafeStrcpy(error, maxlength, "Could not find GeoIP2 database.");
		return false;
	}

	g_pShareSys->AddNatives(myself, geoip_natives);
	g_pShareSys->RegisterLibrary(myself, "GeoIP");

	char date[40];
	const time_t epoch = (const time_t)mmdb.metadata.build_epoch;
	strftime(date, 40, "%F %T UTC", gmtime(&epoch));

	g_pSM->LogMessage(myself, "GeoIP2 database loaded: %s (%s)", mmdb.metadata.database_type, date);

	if (mmdb.metadata.languages.count > 0)
	{
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
	}

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

	pCtx->StringToLocalUTF8(params[2], 3, ccode, NULL);

	return (str.length() != 0) ? 1 : 0;
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

	pCtx->StringToLocalUTF8(params[2], 4, ccode, NULL);

	return (str.length() != 0) ? 1 : 0;
}

static cell_t sm_Geoip_RegionCode(IPluginContext *pCtx, const cell_t *params)
{
	char *ip;
	pCtx->LocalToString(params[1], &ip);
	StripPort(ip);

	size_t length = 0;
	char ccode[12] = { 0 };

	const char *pathCountry[] = {"country", "iso_code", NULL};
	std::string countryCode = lookupString(ip, pathCountry);

	if (countryCode.length() != 0)
	{
		const char *pathRegion[] = {"subdivisions", "0", "iso_code", NULL};
		std::string regionCode = lookupString(ip, pathRegion);

		length = regionCode.length();

		if (length != 0)
		{
			ke::SafeSprintf(ccode, sizeof(ccode), "%s-%s", countryCode.c_str(), regionCode.c_str());
		}
	}

	pCtx->StringToLocalUTF8(params[2], sizeof(ccode), ccode, NULL);

	return (length != 0) ? 1 : 0;
}

static cell_t sm_Geoip_ContinentCode(IPluginContext *pCtx, const cell_t *params)
{
	char *ip;
	pCtx->LocalToString(params[1], &ip);
	StripPort(ip);

	const char *path[] = {"continent", "code", NULL};
	std::string str = lookupString(ip, path);
	const char *ccode = str.c_str();

	pCtx->StringToLocalUTF8(params[2], 3, ccode, NULL);

	return getContinentId(ccode);
}

static cell_t sm_Geoip_Country(IPluginContext *pCtx, const cell_t *params)
{
	char *ip;
	pCtx->LocalToString(params[1], &ip);
	StripPort(ip);

	const char *path[] = {"country", "names", "en", NULL};
	std::string str = lookupString(ip, path);
	const char *ccode = str.c_str();

	pCtx->StringToLocalUTF8(params[2], params[3], ccode, NULL);

	return (str.length() != 0) ? 1 : 0;
}

static cell_t sm_Geoip_CountryEx(IPluginContext *pCtx, const cell_t *params)
{
	char *ip;
	pCtx->LocalToString(params[1], &ip);
	StripPort(ip);

	if (params[4] > 0)
	{
		IGamePlayer *player = playerhelpers->GetGamePlayer(params[4]);
		if (!player || !player->IsConnected())
		{
			return pCtx->ThrowNativeError("Invalid client index %d", params[4]);
		}
	}

	const char *path[] = {"country", "names", getLang(params[4]), NULL};
	std::string str = lookupString(ip, path);
	const char *ccode = str.c_str();

	pCtx->StringToLocalUTF8(params[2], params[3], ccode, NULL);

	return (str.length() != 0) ? 1 : 0;
}

static cell_t sm_Geoip_Continent(IPluginContext *pCtx, const cell_t *params)
{
	char *ip;
	pCtx->LocalToString(params[1], &ip);
	StripPort(ip);

	if (params[4] > 0)
	{
		IGamePlayer *player = playerhelpers->GetGamePlayer(params[4]);
		if (!player || !player->IsConnected())
		{
			return pCtx->ThrowNativeError("Invalid client index %d", params[4]);
		}
	}

	const char *path[] = {"continent", "names", getLang(params[4]), NULL};
	std::string str = lookupString(ip, path);
	const char *ccode = str.c_str();

	pCtx->StringToLocalUTF8(params[2], params[3], ccode, NULL);

	return (str.length() != 0) ? 1 : 0;
}

static cell_t sm_Geoip_Region(IPluginContext *pCtx, const cell_t *params)
{
	char *ip;
	pCtx->LocalToString(params[1], &ip);
	StripPort(ip);

	if (params[4] > 0)
	{
		IGamePlayer *player = playerhelpers->GetGamePlayer(params[4]);
		if (!player || !player->IsConnected())
		{
			return pCtx->ThrowNativeError("Invalid client index %d", params[4]);
		}
	}

	const char *path[] = {"subdivisions", "0", "names", getLang(params[4]), NULL};
	std::string str = lookupString(ip, path);
	const char *ccode = str.c_str();

	pCtx->StringToLocalUTF8(params[2], params[3], ccode, NULL);

	return (str.length() != 0) ? 1 : 0;
}

static cell_t sm_Geoip_City(IPluginContext *pCtx, const cell_t *params)
{
	char *ip;
	pCtx->LocalToString(params[1], &ip);
	StripPort(ip);

	if (params[4] > 0)
	{
		IGamePlayer *player = playerhelpers->GetGamePlayer(params[4]);
		if (!player || !player->IsConnected())
		{
			return pCtx->ThrowNativeError("Invalid client index %d", params[4]);
		}
	}

	const char *path[] = {"city", "names", getLang(params[4]), NULL};
	std::string str = lookupString(ip, path);
	const char *ccode = str.c_str();

	pCtx->StringToLocalUTF8(params[2], params[3], ccode, NULL);

	return (str.length() != 0) ? 1 : 0;
}

static cell_t sm_Geoip_Timezone(IPluginContext *pCtx, const cell_t *params)
{
	char *ip;
	pCtx->LocalToString(params[1], &ip);
	StripPort(ip);

	const char *path[] = {"location", "time_zone", NULL};
	std::string str = lookupString(ip, path);
	const char *ccode = str.c_str();

	pCtx->StringToLocalUTF8(params[2], params[3], ccode, NULL);

	return (str.length() != 0) ? 1 : 0;
}

static cell_t sm_Geoip_Latitude(IPluginContext *pCtx, const cell_t *params)
{
	char *ip;
	pCtx->LocalToString(params[1], &ip);
	StripPort(ip);

	const char *path[] = {"location", "latitude", NULL};
	double latitude = lookupDouble(ip, path);

	return sp_ftoc(latitude);
}

static cell_t sm_Geoip_Longitude(IPluginContext *pCtx, const cell_t *params)
{
	char *ip;
	pCtx->LocalToString(params[1], &ip);
	StripPort(ip);

	const char *path[] = {"location", "longitude", NULL};
	double longitude = lookupDouble(ip, path);

	return sp_ftoc(longitude);
}

static cell_t sm_Geoip_Distance(IPluginContext *pCtx, const cell_t *params)
{
	float earthRadius = params[5] ? 3958.0 : 6370.997; // miles / km

	float lat1 = sp_ctof(params[1]) * (M_PI / 180);
	float lon1 = sp_ctof(params[2]) * (M_PI / 180);
	float lat2 = sp_ctof(params[3]) * (M_PI / 180);
	float lon2 = sp_ctof(params[4]) * (M_PI / 180);

	return sp_ftoc(earthRadius * acos(sin(lat1) * sin(lat2) + cos(lat1) * cos(lat2) * cos(lon2 - lon1)));
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
	{"GeoipLatitude",		sm_Geoip_Latitude},
	{"GeoipLongitude",		sm_Geoip_Longitude},
	{"GeoipDistance",		sm_Geoip_Distance},
	{NULL,					NULL},
};

