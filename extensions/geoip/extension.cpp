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
#include "GeoIP.h"
#include "am-string.h"

/**
 * @file extension.cpp
 * @brief Implement extension code here.
 */
GeoIP_Extension g_GeoIP;
GeoIP *gi = NULL;

SMEXT_LINK(&g_GeoIP);

bool GeoIP_Extension::SDK_OnLoad(char *error, size_t maxlength, bool late)
{
	char path[PLATFORM_MAX_PATH];

	g_pSM->BuildPath(Path_SM, path, sizeof(path), "configs/geoip/GeoIP.dat");
	gi = GeoIP_open(path, GEOIP_MEMORY_CACHE);

	if (!gi)
	{
		ke::SafeStrcpy(error, maxlength, "Could not load configs/geoip/GeoIP.dat");
		return false;
	}

	g_pShareSys->AddNatives(myself, geoip_natives);
	g_pShareSys->RegisterLibrary(myself, "GeoIP");
	g_pSM->LogMessage(myself, "GeoIP database info: %s", GeoIP_database_info(gi));

	return true;
}

void GeoIP_Extension::SDK_OnUnload()
{
	GeoIP_delete(gi);
	gi = NULL;
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
	const char *ccode;

	pCtx->LocalToString(params[1], &ip);
	StripPort(ip);

	ccode = GeoIP_country_code_by_addr(gi, ip);

	pCtx->StringToLocal(params[2], 3, ccode ? ccode : "");

	return ccode ? 1 : 0;
}

static cell_t sm_Geoip_Code3(IPluginContext *pCtx, const cell_t *params)
{
	char *ip;
	const char *ccode;

	pCtx->LocalToString(params[1], &ip);
	StripPort(ip);

	ccode = GeoIP_country_code3_by_addr(gi, ip);
	pCtx->StringToLocal(params[2], 4, ccode ? ccode : "");

	return ccode ? 1 : 0;
}

static cell_t sm_Geoip_Country(IPluginContext *pCtx, const cell_t *params)
{
	char *ip;
	const char *ccode;

	pCtx->LocalToString(params[1], &ip);
	StripPort(ip);

	ccode = GeoIP_country_name_by_addr(gi, ip);
	pCtx->StringToLocal(params[2], params[3], (ccode) ? ccode : "");

	return ccode ? 1 : 0;
}

const sp_nativeinfo_t geoip_natives[] = 
{
	{"GeoipCode2",			sm_Geoip_Code2},
	{"GeoipCode3",			sm_Geoip_Code3},
	{"GeoipCountry",		sm_Geoip_Country},
	{NULL,					NULL},
};

