/**
 * vim: set ts=4 :
 * ===============================================================
 * SourceMod (C)2004-2007 AlliedModders LLC.  All rights reserved.
 * ===============================================================
 *
 * This file is not open source and may not be copied without explicit
 * written permission of AlliedModders LLC.  This file may not be redistributed 
 * in whole or significant part.
 * For information, see LICENSE.txt or http://www.sourcemod.net/license.php
 *
 * Version: $Id$
 */

#include <time.h>
#include "sm_globals.h"
#include "sourcemod.h"
#include "PluginSys.h"
#include "HandleSys.h"

#if defined PLATFORM_WINDOWS
#include <windows.h>
#elif defined PLATFORM_LINUX
#include <limits.h>
#include <unistd.h>
#include <sys/times.h>
#endif

HandleType_t g_PlIter;

class CoreNativeHelpers : 
	public SMGlobalClass,
	public IHandleTypeDispatch
{
public:
	void OnSourceModAllInitialized()
	{
		HandleAccess hacc;
		g_HandleSys.InitAccessDefaults(NULL,  &hacc);
		hacc.access[HandleAccess_Clone] = HANDLE_RESTRICT_IDENTITY|HANDLE_RESTRICT_OWNER;

		g_PlIter = g_HandleSys.CreateType("PluginIterator", this, 0, NULL, NULL, g_pCoreIdent, NULL);
	}
	void OnHandleDestroy(HandleType_t type, void *object)
	{
		IPluginIterator *iter = (IPluginIterator *)object;
		iter->Release();
	}
	void OnSourceModShutdown()
	{
		g_HandleSys.RemoveType(g_PlIter, g_pCoreIdent);
	}
} g_CoreNativeHelpers;


static cell_t ThrowError(IPluginContext *pContext, const cell_t *params)
{
	char buffer[512];

	g_SourceMod.FormatString(buffer, sizeof(buffer), pContext, params, 1);

	if (pContext->GetContext()->n_err == SP_ERROR_NONE)
	{
		pContext->ThrowNativeErrorEx(SP_ERROR_ABORTED, "%s", buffer);
	}

	return 0;
}

static cell_t GetTime(IPluginContext *pContext, const cell_t *params)
{
	time_t t = time(NULL);
	cell_t *addr;
	pContext->LocalToPhysAddr(params[1], &addr);

	*(time_t *)addr = t;

	return static_cast<cell_t>(t);
}

static cell_t GetPluginIterator(IPluginContext *pContext, const cell_t *params)
{
	IPluginIterator *iter = g_PluginSys.GetPluginIterator();

	Handle_t hndl = g_HandleSys.CreateHandle(g_PlIter, iter, pContext->GetIdentity(), g_pCoreIdent, NULL);

	if (hndl == BAD_HANDLE)
	{
		iter->Release();
	}

	return hndl;
}

static cell_t MorePlugins(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	IPluginIterator *pIter;

	HandleSecurity sec;
	sec.pIdentity = g_pCoreIdent;
	sec.pOwner = pContext->GetIdentity();

	if ((err=g_HandleSys.ReadHandle(hndl, g_PlIter, &sec, (void **)&pIter)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Could not read Handle %x (error %d)", hndl, err);
	}

	return pIter->MorePlugins() ? 1 : 0;
}

static cell_t ReadPlugin(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	IPluginIterator *pIter;

	HandleSecurity sec;
	sec.pIdentity = g_pCoreIdent;
	sec.pOwner = pContext->GetIdentity();

	if ((err=g_HandleSys.ReadHandle(hndl, g_PlIter, &sec, (void **)&pIter)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Could not read Handle %x (error %d)", hndl, err);
	}

	CPlugin *pPlugin = (CPlugin *)pIter->GetPlugin();
	if (!pPlugin)
	{
		return BAD_HANDLE;
	}

	pIter->NextPlugin();

	return pPlugin->GetMyHandle();
}

CPlugin *GetPluginFromHandle(IPluginContext *pContext, Handle_t hndl)
{
	if (hndl == BAD_HANDLE)
	{
		return g_PluginSys.GetPluginByCtx(pContext->GetContext());
	} else {
		HandleError err;
		CPlugin *pPlugin = (CPlugin *)g_PluginSys.PluginFromHandle(hndl, &err);
		if (!pPlugin)
		{
			pContext->ThrowNativeError("Could not read Handle %x (error %d)", hndl, err);
		}
		return pPlugin;
	}
}

static cell_t GetPluginStatus(IPluginContext *pContext, const cell_t *params)
{
	CPlugin *pPlugin = GetPluginFromHandle(pContext, params[1]);
	if (!pPlugin)
	{
		return 0;
	}

	return pPlugin->GetStatus();
}

static cell_t GetPluginFilename(IPluginContext *pContext, const cell_t *params)
{
	CPlugin *pPlugin = GetPluginFromHandle(pContext, params[1]);
	if (!pPlugin)
	{
		return 0;
	}

	pContext->StringToLocalUTF8(params[2], params[3], pPlugin->GetFilename(), NULL);

	return 1;
}

static cell_t IsPluginDebugging(IPluginContext *pContext, const cell_t *params)
{
	CPlugin *pPlugin = GetPluginFromHandle(pContext, params[1]);
	if (!pPlugin)
	{
		return 0;
	}

	return pPlugin->IsDebugging() ? 1 : 0;
}

/* Local to plugins only */
enum PluginInfo
{
	PlInfo_Name,			/**< Plugin name */
	PlInfo_Author,			/**< Plugin author */
	PlInfo_Description,		/**< Plugin description */
	PlInfo_Version,			/**< Plugin verison */
	PlInfo_URL,				/**< Plugin URL */
};

static cell_t GetPluginInfo(IPluginContext *pContext, const cell_t *params)
{
	CPlugin *pPlugin = GetPluginFromHandle(pContext, params[1]);
	if (!pPlugin)
	{
		return 0;
	}

	const sm_plugininfo_t *info = pPlugin->GetPublicInfo();

	if (!info)
	{
		return 0;
	}

	const char *str = NULL;

	switch ((PluginInfo)params[2])
	{
	case PlInfo_Name:
		{
			str = info->name;
			break;
		}
	case PlInfo_Author:
		{
			str = info->author;
			break;
		}
	case PlInfo_Description:
		{
			str = info->description;
			break;
		}
	case PlInfo_Version:
		{
			str = info->version;
			break;
		}
	case PlInfo_URL:
		{
			str = info->url;
			break;
		}
	}

	if (!str || str[0] == '\0')
	{
		return 0;
	}

	pContext->StringToLocalUTF8(params[3], params[4], str, NULL);

	return 1;
}

static cell_t SetFailState(IPluginContext *pContext, const cell_t *params)
{
	char *str;
	pContext->LocalToString(params[1], &str);

	CPlugin *pPlugin = g_PluginSys.GetPluginByCtx(pContext->GetContext());
	pPlugin->SetErrorState(Plugin_Error, "%s", str);

	return pContext->ThrowNativeErrorEx(SP_ERROR_ABORTED, "%s", str);
}

static cell_t GetSysTickCount(IPluginContext *pContext, const cell_t *params)
{
#if defined PLATFORM_WINDOWS
	return (cell_t)GetTickCount();
#elif defined PLATFORM_LINUX
	tms tm;
	clock_t ticks = times(&tm);
	long ticks_per_sec = sysconf(_SC_CLK_TCK);
	double fticks = (double)ticks / (double)ticks_per_sec;
	fticks *= 1000.0f;
	if (fticks > INT_MAX)
	{
		double r = (int)(fticks / INT_MAX) * (double)INT_MAX;
		fticks -= r;
	}
	return (cell_t)fticks;
#endif
}

REGISTER_NATIVES(coreNatives)
{
	{"GetPluginFilename",	GetPluginFilename},
	{"GetPluginInfo",		GetPluginInfo},
	{"GetPluginIterator",	GetPluginIterator},
	{"GetPluginStatus",		GetPluginStatus},
	{"GetSysTickCount",		GetSysTickCount},
	{"GetTime",				GetTime},
	{"IsPluginDebugging",	IsPluginDebugging},
	{"MorePlugins",			MorePlugins},
	{"ReadPlugin",			ReadPlugin},
	{"ThrowError",			ThrowError},
	{"SetFailState",		SetFailState},
	{NULL,					NULL},
};
