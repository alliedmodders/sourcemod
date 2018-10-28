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
#include "libmaxminddb/include/maxminddb.h"
#include "am-string.h"

#define DATA_REL_PATH "configs/geoip/GeoLite2-Country.mmdb"

/**
 * @file extension.cpp
 * @brief Implement extension code here.
 */
GeoIP_Extension g_GeoIP;
MMDB_s gi2;

SMEXT_LINK(&g_GeoIP);

bool GeoIP_Extension::SDK_OnLoad(char *error, size_t maxlength, bool late)
{
	char path[PLATFORM_MAX_PATH];

	g_pSM->BuildPath(Path_SM, path, sizeof(path), DATA_REL_PATH);
	int res = MMDB_open(path, 0, &gi2);
	if (res != MMDB_SUCCESS)
	{
		ke::SafeSprintf(error, maxlength, "Could not load GeoIP data from %s. (%s)", DATA_REL_PATH, MMDB_strerror(res));
		return false;
	}

	g_pShareSys->AddNatives(myself, geoip_natives);
	g_pShareSys->RegisterLibrary(myself, "GeoIP");

	if (gi2.metadata.description.count)
	{
		g_pSM->LogMessage(myself, "GeoIP database info: %s", gi2.metadata.description.descriptions[0]->description);
	}
	else
	{
		g_pSM->LogMessage(myself, "GeoIP database info: Unknown version");
	}

	return true;
}

void GeoIP_Extension::SDK_OnUnload()
{
	MMDB_close(&gi2);
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

	int gai_error;
	int mmdb_error;
	MMDB_lookup_result_s res = MMDB_lookup_string(&gi2, ip, &gai_error, &mmdb_error);
	if (gai_error != 0 || mmdb_error != MMDB_SUCCESS || !res.found_entry)
	{
		return 0;
	}

	MMDB_entry_data_s entry_data;
	int status = MMDB_get_value(&res.entry, &entry_data, "country", "iso_code", NULL);
	if (status != MMDB_SUCCESS || !entry_data.has_data || entry_data.type != MMDB_DATA_TYPE_UTF8_STRING)
	{
		return 0;
	}

	pCtx->StringToLocal(params[2], 3, entry_data.utf8_string);

	return 1;
}

static cell_t sm_Geoip_Code3(IPluginContext *pCtx, const cell_t *params)
{
	return pCtx->ThrowNativeError("Unsupported.");
}

static cell_t sm_Geoip_Country(IPluginContext *pCtx, const cell_t *params)
{
	char *ip;
	pCtx->LocalToString(params[1], &ip);
	StripPort(ip);

	int gai_error;
	int mmdb_error;
	MMDB_lookup_result_s res = MMDB_lookup_string(&gi2, ip, &gai_error, &mmdb_error);
	if (gai_error != 0 || mmdb_error != MMDB_SUCCESS || !res.found_entry)
	{
		return 0;
	}

	MMDB_entry_data_s entry_data;
	int status = MMDB_get_value(&res.entry, &entry_data, "country", "names", "en", NULL);
	if (status != MMDB_SUCCESS || !entry_data.has_data || entry_data.type != MMDB_DATA_TYPE_UTF8_STRING)
	{
		return 0;
	}

	pCtx->StringToLocal(params[2], params[3], entry_data.utf8_string);

	return 1;
}

const sp_nativeinfo_t geoip_natives[] = 
{
	{"GeoipCode2",			sm_Geoip_Code2},
	{"GeoipCode3",			sm_Geoip_Code3},
	{"GeoipCountry",		sm_Geoip_Country},
	{NULL,					NULL},
};

