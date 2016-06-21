/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Webternet Extension
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
 
#include "extension.h"
#include "FileDownloader.h"
#include "MemoryDownloader.h"
#include "curlapi.h"
 
 static cell_t HTTP_CreateFileDownloader(IPluginContext *pCtx, const cell_t *params)
{
	char *file;
	// 1st param: file name
	pCtx->LocalToString(params[1], &file);

	char localPath[PLATFORM_MAX_PATH];
	// Build absolute path for fopen()
	smutils->BuildPath(Path_Game, localPath, PLATFORM_MAX_PATH, file);

	// Use file-based download handler
	FileDownloader *downloader = new FileDownloader(localPath);

	// This should never happen but, oh well...
	if (!downloader)
	{
		return pCtx->ThrowNativeError("Could not create downloader");
	}

	auto hndl = g_pHandleSys->CreateHandle(g_DownloadHandle,
		(void*)downloader,
		pCtx->GetIdentity(),
		myself->GetIdentity(),
		NULL);

	if (hndl == BAD_HANDLE)
	{
		delete downloader;
	}

	return hndl;
}

static cell_t HTTP_CreateMemoryDownloader(IPluginContext *pCtx, const cell_t *params)
{
	MemoryDownloader *downloader = new MemoryDownloader();

	if (!downloader)
	{
		return pCtx->ThrowNativeError("Could not create downloader");
	}

	auto hndl = g_pHandleSys->CreateHandle(g_DownloadHandle,
		(void*)downloader,
		pCtx->GetIdentity(),
		myself->GetIdentity(),
		NULL);

	if (hndl == BAD_HANDLE)
	{
		delete downloader;
	}

	return hndl;
}

static cell_t HTTP_CreateWebForm(IPluginContext *pCtx, const cell_t *params)
{
	IWebForm *form = g_webternet.CreateForm();

	if (!form)
	{
		return pCtx->ThrowNativeError("Could not create web form");
	}

	auto hndl = g_pHandleSys->CreateHandle(g_FormHandle,
		(void*)form,
		pCtx->GetIdentity(),
		myself->GetIdentity(),
		NULL);

	if (hndl == BAD_HANDLE)
	{
		delete form;
	}

	return hndl;
}

static cell_t HTTP_AddStringToWebForm(IPluginContext *pCtx, const cell_t *params)
{
	// 1st param: web form handle
	Handle_t hndl = static_cast<Handle_t>(params[1]);

	HandleError err;
	HandleSecurity sec;
	sec.pOwner = pCtx->GetIdentity();
	sec.pIdentity = myself->GetIdentity();

	IWebForm *form = NULL;

	// Validate handle data
	if ((err = g_pHandleSys->ReadHandle(hndl, g_FormHandle, &sec, 
		(void **)&form)) != HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid web form handle %x (error %d)", 
			hndl, err);
	}

	if (!form)
	{
		return pCtx->ThrowNativeError("HTTP web form data not found\n");
	}

	char *name, *data;
	// 2nd param: name of the POST variable
	pCtx->LocalToString(params[2], &name);
	// 3rd param: value of the POST variable
	pCtx->LocalToString(params[3], &data);

	return form->AddString(name, data);
}

static cell_t HTTP_AddFileToWebForm(IPluginContext *pCtx, const cell_t *params)
{
	// 1st param: web form handle
	Handle_t hndl = static_cast<Handle_t>(params[1]);

	HandleError err;
	HandleSecurity sec;
	sec.pOwner = pCtx->GetIdentity();
	sec.pIdentity = myself->GetIdentity();

	IWebForm *form = NULL;

	// Validate handle data
	if ((err = g_pHandleSys->ReadHandle(hndl, g_FormHandle, &sec, 
		(void **)&form)) != HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid web form handle %x (error %d)", 
			hndl, err);
	}

	if (!form)
	{
		return pCtx->ThrowNativeError("HTTP web form data not found\n");
	}

	char *name, *path;
	// 2nd param: name of the POST variable
	pCtx->LocalToString(params[2], &name);
	// 3rd param: value of the POST variable
	pCtx->LocalToString(params[3], &path);

	return form->AddFile(name, path);
}

static cell_t HTTP_CreateSession(IPluginContext *pCtx, const cell_t *params)
{
	IWebTransfer *x = g_webternet.CreateSession();

	if (!x)
	{
		return pCtx->ThrowNativeError("Could not create session");
	}

	auto hndl = g_pHandleSys->CreateHandle(g_SessionHandle, 
		(void*)x, 
		pCtx->GetIdentity(), 
		myself->GetIdentity(),
		NULL);

	if (hndl == BAD_HANDLE)
	{
		delete x;
	}

	return hndl;
}

static cell_t HTTP_GetLastError(IPluginContext *pCtx, const cell_t *params)
{
	// 1st param: session handle
	Handle_t hndl = static_cast<Handle_t>(params[1]);

	HandleError err;
	HandleSecurity sec;
	sec.pOwner = pCtx->GetIdentity();
	sec.pIdentity = myself->GetIdentity();

	IWebTransfer *x = NULL;

	// Validate handle data
	if ((err = g_pHandleSys->ReadHandle(hndl, g_SessionHandle, &sec, 
		(void **)&x)) != HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid session handle %x (error %d)", 
			hndl, err);
	}

	if (!x)
	{
		return pCtx->ThrowNativeError("HTTP session data not found\n");
	}

	// Copy error message in output string
	pCtx->StringToLocalUTF8(params[2], params[3], x->LastErrorMessage(), NULL);

	return true;
}

static cell_t HTTP_PostAndDownload(IPluginContext *pCtx, const cell_t *params)
{
	HTTPRequestHandleSet handles;
	// 1st param: session handle
	handles.hndlSession = static_cast<Handle_t>(params[1]);
	
	HandleError err;
	HandleSecurity sec;
	sec.pOwner = pCtx->GetIdentity();
	sec.pIdentity = myself->GetIdentity();

	IWebTransfer *x = NULL;

	if ((err = g_pHandleSys->ReadHandle(handles.hndlSession, 
		g_SessionHandle, &sec, (void **)&x)) != HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid session handle %x (error %d)", 
			handles.hndlSession, err);
	}

	if (!x)
	{
		return pCtx->ThrowNativeError("HTTP session data not found\n");
	}
	
	// 2nd param: downloader handle
	handles.hndlDownloader = static_cast<Handle_t>(params[2]);
	IBaseDownloader *downloader = NULL;

	if ((err = g_pHandleSys->ReadHandle(handles.hndlDownloader,
		g_DownloadHandle, &sec, (void **)&downloader)) != HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid downloader handle %x (error %d)", 
			handles.hndlDownloader, err);
	}

	if (!downloader)
	{
		return pCtx->ThrowNativeError("HTTP downloader data not found\n");
	}

	// 3rd param: web form handle
	handles.hndlForm = static_cast<Handle_t>(params[3]);;
	IWebForm *form = NULL;
	
	if ((err = g_pHandleSys->ReadHandle(handles.hndlForm,
		g_FormHandle, &sec, (void **)&form)) != HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid web form handle %x (error %d)",
			handles.hndlForm, err);
	}

	if (!form)
	{
		return pCtx->ThrowNativeError("HTTP web form data not found\n");
	}

	char *url;
	// 4th param: target URL
	pCtx->LocalToString(params[4], &url);

	// 5th param: callback function
	HTTPRequestCompletedContext ctx(params[5]);

	// 6th param: custom user data
	if (params[0] >= 6)
	{
		ctx.SetContextValue(params[6]);
	}

	// Queue request for asynchronous execution
	g_SessionManager.PostAndDownload(pCtx, handles, url, ctx);
	
	return true;
}

static cell_t HTTP_Download(IPluginContext *pCtx, const cell_t *params)
{
	HTTPRequestHandleSet handles;
	// 1st param: session handle
	handles.hndlSession = static_cast<Handle_t>(params[1]);

	HandleError err;
	HandleSecurity sec;
	sec.pOwner = pCtx->GetIdentity();
	sec.pIdentity = myself->GetIdentity();

	IWebTransfer *x = NULL;

	// Validate handle data
	if ((err = g_pHandleSys->ReadHandle(handles.hndlSession, 
		g_SessionHandle, &sec, (void **)&x)) != HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid session handle %x (error %d)", 
			handles.hndlSession, err);
	}

	if (!x)
	{
		return pCtx->ThrowNativeError("HTTP session data not found\n");
	}

	// 2nd param: downloader handle
	handles.hndlDownloader = static_cast<Handle_t>(params[2]);
	IBaseDownloader *downloader = NULL;

	if ((err = g_pHandleSys->ReadHandle(handles.hndlDownloader,
		g_DownloadHandle, &sec, (void **)&downloader)) != HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid downloader handle %x (error %d)", 
			handles.hndlDownloader, err);
	}

	if (!downloader)
	{
		return pCtx->ThrowNativeError("HTTP downloader data not found\n");
	}

	char *url;
	// 3rd param: target URL
	pCtx->LocalToString(params[3], &url);

	// 4th param: callback function
	HTTPRequestCompletedContext ctx(params[4]);

	// 5th param: custom user data
	if (params[0] >= 5)
	{
		ctx.SetContextValue(params[5]);
	}

	// Queue request for asynchronous execution
	g_SessionManager.Download(pCtx, handles, url, ctx);

	return true;
}

static cell_t HTTP_GetBodySize(IPluginContext *pCtx, const cell_t *params)
{
	// 1st param: downloader handle
	Handle_t hndl = static_cast<Handle_t>(params[1]);

	HandleError err;
	HandleSecurity sec;
	sec.pOwner = pCtx->GetIdentity();
	sec.pIdentity = myself->GetIdentity();

	IBaseDownloader *dldr = NULL;

	// Validate handle data
	if ((err = g_pHandleSys->ReadHandle(hndl, 
		g_DownloadHandle, &sec, (void **)&dldr)) != HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid downloader handle %x (error %d)", 
			hndl, err);
	}

	if (!dldr)
	{
		return pCtx->ThrowNativeError("HTTP downloader data not found\n");
	}

	return dldr->GetSize();
}

static cell_t HTTP_GetBodyContent(IPluginContext *pCtx, const cell_t *params)
{
	// 1st param: downloader handle
	Handle_t hndl = static_cast<Handle_t>(params[1]);

	HandleError err;
	HandleSecurity sec;
	sec.pOwner = pCtx->GetIdentity();
	sec.pIdentity = myself->GetIdentity();

	IBaseDownloader *dldr = NULL;

	// Validate handle data
	if ((err = g_pHandleSys->ReadHandle(hndl, 
		g_DownloadHandle, &sec, (void **)&dldr)) != HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid downloader handle %x (error %d)", hndl, err);
	}

	if (!dldr)
	{
		return pCtx->ThrowNativeError("HTTP downloader data not found\n");
	}

	char *body;
	// 2nd param: receiving buffer
	pCtx->LocalToString(params[2], &body);
	// 3rd param: max buffer length
	uint32_t bodySize = params[3];

	// NOTE: we need one additional byte for the null terminator
	if (bodySize < (dldr->GetSize() + 1))
	{
		return pCtx->ThrowNativeError("Buffer too small\n");
	}

	if (dldr->GetBuffer() != NULL)
	{
		memcpy(body, dldr->GetBuffer(), params[3]);
		body[dldr->GetSize()] = '\0';
	}

	return pCtx->StringToLocal(params[2], params[3], body);
}

static cell_t HTTP_SetFailOnHTTPError(IPluginContext *pCtx, const cell_t *params)
{
	// 1st param: session handle
	Handle_t hndl = static_cast<Handle_t>(params[1]);

	HandleError err;
	HandleSecurity sec;
	sec.pOwner = pCtx->GetIdentity();
	sec.pIdentity = myself->GetIdentity();

	IWebTransfer *xfer = NULL;

	// Validate handle data
	if ((err = g_pHandleSys->ReadHandle(hndl, 
		g_SessionHandle, &sec, (void **)&xfer)) != HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid session handle %x (error %d)", hndl, err);
	}

	if (!xfer)
	{
		return pCtx->ThrowNativeError("HTTP session data not found\n");
	}

	return xfer->SetFailOnHTTPError(params[2] != 0);
}

sp_nativeinfo_t curlext_natives[] = 
{
	{"HTTP_CreateFileDownloader",	HTTP_CreateFileDownloader},
	{"HTTP_CreateMemoryDownloader",	HTTP_CreateMemoryDownloader},
	{"HTTP_CreateSession",			HTTP_CreateSession},
	{"HTTP_SetFailOnHTTPError",		HTTP_SetFailOnHTTPError},
	{"HTTP_GetLastError",			HTTP_GetLastError},
	{"HTTP_Download",				HTTP_Download},
	{"HTTP_PostAndDownload",		HTTP_PostAndDownload},
	{"HTTP_CreateWebForm",			HTTP_CreateWebForm},
	{"HTTP_AddStringToWebForm",		HTTP_AddStringToWebForm},
	{"HTTP_AddFileToWebForm",		HTTP_AddFileToWebForm},
	{"HTTP_GetBodySize",			HTTP_GetBodySize},
	{"HTTP_GetBodyContent",			HTTP_GetBodyContent},

	// Methodmap versions
	{"HTTPDownloader.BodySize.get",	HTTP_GetBodySize},
	{"HTTPDownloader.GetBodyContent",	HTTP_GetBodyContent},
	{"HTTPSession.SetFailOnHTTPError",		HTTP_SetFailOnHTTPError},
	{"HTTPSession.GetLastError",			HTTP_GetLastError},
	{"HTTPSession.Download",				HTTP_Download},
	{"HTTPSession.PostAndDownload",		HTTP_PostAndDownload},
	{"HTTPWebForm.AddString",		HTTP_AddStringToWebForm},
	{"HTTPWebForm.AddFile",		HTTP_AddFileToWebForm},
	{"HTTPFileDownloader.HTTPFileDownloader",				HTTP_CreateFileDownloader},
	{"HTTPMemoryDownloader.HTTPMemoryDownloader",		HTTP_CreateMemoryDownloader},
	{"HTTPSession.HTTPSession",		HTTP_CreateSession},
	{"HTTPWebForm.HTTPWebForm",		HTTP_CreateWebForm},
	{NULL,							NULL},
};
