#include "extension.h"
#include "GeoIP.h"

GeoIP_Extension g_GeoIP;
GeoIP *gi = NULL;

SMEXT_LINK(&g_GeoIP);

bool GeoIP_Extension::SDK_OnLoad(char *error, size_t err_max, bool late)
{
	char *path = "GeoIP.dat"; //:TODO: build a real path here
	//:TODO: log any errors on load.
	gi = GeoIP_open(path, GEOIP_MEMORY_CACHE);

	if (!gi)
	{
		//:TODO: log
		return false;
	}
	return true;
}

void GeoIP_Extension::SDK_OnUnload()
{
	GeoIP_delete(gi);
	gi = NULL;
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
	pCtx->StringToLocal(params[2], 3, (ccode) ? ccode : "er");

	return 1;
}

static cell_t sm_Geoip_Code3(IPluginContext *pCtx, const cell_t *params)
{
	char *ip;
	const char *ccode;

	pCtx->LocalToString(params[1], &ip);
	StripPort(ip);

	ccode = GeoIP_country_code3_by_addr(gi, ip);
	pCtx->StringToLocal(params[2], 4, (ccode) ? ccode : "err");

	return 1;
}

static cell_t sm_Geoip_Country(IPluginContext *pCtx, const cell_t *params)
{
	char *ip;
	const char *ccode;

	pCtx->LocalToString(params[1], &ip);
	StripPort(ip);

	ccode = GeoIP_country_name_by_addr(gi, ip);
	pCtx->StringToLocal(params[2], params[3], (ccode) ? ccode : "error");

	return 1;
}
