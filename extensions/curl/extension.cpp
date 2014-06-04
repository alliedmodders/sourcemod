/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Sample Extension
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
#include <stdarg.h>
#include <sm_platform.h>
#include <curl/curl.h>
#include "curlapi.h"
#include "FileDownloader.h"
#include "MemoryDownloader.h"


/**
 * @file extension.cpp
 * @brief Implement extension code here.
 */

CurlExt curl_ext;		/**< Global singleton for extension's main interface */

SMEXT_LINK(&curl_ext);

SessionHandler g_SessionHandler;
HandleType_t g_SessionHandle = 0;
FormHandler g_FormHandler;
HandleType_t g_FormHandle = 0;
DownloadHandler g_DownloadHandler;
HandleType_t g_DownloadHandle = 0;
HTTPSessionManager& g_SessionManager = HTTPSessionManager::instance();

bool CurlExt::SDK_OnLoad(char *error, size_t maxlength, bool late)
{
	long flags;
	CURLcode code;

	flags = CURL_GLOBAL_NOTHING;
#if defined PLATFORM_WINDOWS
	flags = CURL_GLOBAL_WIN32;
#endif

	code = curl_global_init(flags);
	if (code)
	{
		smutils->Format(error, maxlength, "%s", curl_easy_strerror(code));
		return false;
	}

	if (!sharesys->AddInterface(myself, &g_webternet))
	{
		smutils->Format(error, maxlength, "Could not add IWebternet interface");
		return false;
	}

	// Register natives
	g_pShareSys->AddNatives(myself, curlext_natives);
	// Register session handle handler
	g_SessionHandle = g_pHandleSys->CreateType("HTTPSession", 
		&g_SessionHandler, 0, 0, NULL, myself->GetIdentity(), NULL);
	// Register web form handle handler
	g_FormHandle = g_pHandleSys->CreateType("HTTPWebForm",
		&g_FormHandler, 0, 0, NULL, myself->GetIdentity(), NULL);
	// Register web form handle handler
	g_DownloadHandle = g_pHandleSys->CreateType("HTTPDownloader",
		&g_DownloadHandler, 0, 0, NULL, myself->GetIdentity(), NULL);
	plsys->AddPluginsListener(this);
	smutils->AddGameFrameHook(&OnGameFrame);
	g_SessionManager.Initialize();

	return true;
}

void CurlExt::SDK_OnUnload()
{
	g_pHandleSys->RemoveType(g_SessionHandle, myself->GetIdentity());
	g_pHandleSys->RemoveType(g_FormHandle, myself->GetIdentity());
	g_pHandleSys->RemoveType(g_DownloadHandle, myself->GetIdentity());
	plsys->RemovePluginsListener(this);
	smutils->RemoveGameFrameHook(&OnGameFrame);
	g_SessionManager.Cleanup();
	curl_global_cleanup();
}

const char *CurlExt::GetExtensionVerString()
{
	return SOURCEMOD_VERSION;
}

const char *CurlExt::GetExtensionDateString()
{
	return SOURCEMOD_BUILD_TIME;
}

void OnGameFrame(bool simulating)
{
	g_SessionManager.RunFrame();
}

void CurlExt::OnPluginUnloaded(IPlugin *plugin)
{
	g_SessionManager.PluginUnloaded(plugin);
}

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

	return g_pHandleSys->CreateHandle(g_DownloadHandle,
		(void*)downloader,
		pCtx->GetIdentity(),
		myself->GetIdentity(),
		NULL);
}

static cell_t HTTP_CreateMemoryDownloader(IPluginContext *pCtx, const cell_t *params)
{
	MemoryDownloader *downloader = new MemoryDownloader();

	if (!downloader)
	{
		return pCtx->ThrowNativeError("Could not create downloader");
	}

	return g_pHandleSys->CreateHandle(g_DownloadHandle,
		(void*)downloader,
		pCtx->GetIdentity(),
		myself->GetIdentity(),
		NULL);
}

static cell_t HTTP_CreateWebForm(IPluginContext *pCtx, const cell_t *params)
{
	IWebForm *form = g_webternet.CreateForm();

	if (!form)
	{
		return pCtx->ThrowNativeError("Could not create web form");
	}

	return g_pHandleSys->CreateHandle(g_FormHandle,
		(void*)form,
		pCtx->GetIdentity(),
		myself->GetIdentity(),
		NULL);
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

	return g_pHandleSys->CreateHandle(g_SessionHandle, 
		(void*)x, 
		pCtx->GetIdentity(), 
		myself->GetIdentity(),
		NULL);
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

	HTTPRequestCompletedContextPack contextPack;
	contextPack.pCallbackFunction = new HTTPRequestCompletedContextFunction;

	// TODO: this might be obsolete
	contextPack.pCallbackFunction->pContext = pCtx;
	// 5th param: callback function
	contextPack.pCallbackFunction->uPluginFunction = params[5];

	// 6th param: custom user data
	if (params[0] >= 6)
	{
		contextPack.pCallbackFunction->bHasContext = true;
		contextPack.iPluginContextValue = params[6];
	}

	// Queue request for asynchronous execution
	g_SessionManager.PostAndDownload(pCtx, handles, url, contextPack);
	
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

	HTTPRequestCompletedContextPack contextPack;
	contextPack.pCallbackFunction = new HTTPRequestCompletedContextFunction;

	// TODO: this might be obsolete
	contextPack.pCallbackFunction->pContext = pCtx;
	// 4th param: callback function
	contextPack.pCallbackFunction->uPluginFunction = params[4];

	// 5th param: custom user data
	if (params[0] >= 5)
	{
		contextPack.pCallbackFunction->bHasContext = true;
		contextPack.iPluginContextValue = params[5];
	}

	// Queue request for asynchronous execution
	g_SessionManager.Download(pCtx, handles, url, contextPack);

	return true;
}

static cell_t HTTP_GetBodySize(IPluginContext *pCtx, const cell_t *params)
{
	// 1st param: session handle
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
	// 1st param: session handle
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
		return pCtx->ThrowNativeError("HTTP Downloader data not found\n");
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

	memcpy(body, dldr->GetBuffer(), params[3]);
	body[dldr->GetSize()] = '\0';

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
		return pCtx->ThrowNativeError("HTTP Session data not found\n");
	}

	return xfer->SetFailOnHTTPError(static_cast<bool>(params[2]));
}

const sp_nativeinfo_t curlext_natives[] = 
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
	{NULL,							NULL},
};

void HTTPSessionManager::PluginUnloaded(IPlugin *plugin)
{
	// Check for pending requests and cancel them
	{
		pRequestsLock->Lock();

		if (!requests.empty())
		{
			// Run through requests queue
			for (std::deque<HTTPRequest>::iterator i(requests.begin()), end(requests.end()); i != end; ++i)
			{
				// Identify requests associated to (nearly) unmapped plugin context
				if (i->pCtx == plugin->GetBaseContext())
				{
					// All context related data and callbacks are marked invalid
					i->pCtx = NULL;
					i->contextPack.pCallbackFunction = NULL;
				}
			}
		}

		pRequestsLock->Unlock();
	}

	// Wait for running requests to finish
	if (!threads.empty())
	{
		for (std::list<IThreadHandle*>::iterator i(threads.begin()), end(threads.end()); i != end; ++i)
		{
			if ((*i) != NULL)
			{
				(*i)->WaitForThread();
				// NOTE: do not remove handles here as it would break 
				// the iteration. Dead thread handles are destroyed
				// on the next available game frame in the main thread
				
				// Check for pending callbacks and cancel them
				{
					pCallbacksLock->Lock();

					if (!callbacks.empty())
					{
						// Run through callback queue
						for (std::deque<HTTPRequest>::iterator i(callbacks.begin()), end(callbacks.end()); i != end; ++i)
						{
							// Identify callbacks associated to (nearly) unmapped plugin context
							if (i->pCtx == plugin->GetBaseContext())
							{
								// All context related data and callbacks are marked invalid
								i->pCtx = NULL;
								i->contextPack.pCallbackFunction = NULL;
							}
						}
					}

					pCallbacksLock->Unlock();
				}
			}
		}
	}
}

void HTTPSessionManager::PostAndDownload(IPluginContext *pCtx, 
	HTTPRequestHandleSet handles, const char *url, 
	HTTPRequestCompletedContextPack contextPack)
{
	HTTPRequest request;
	BurnSessionHandle(pCtx, handles);

	request.pCtx = pCtx;
	request.handles = handles;
	request.method = HTTP_POST;
	request.url = url;
	request.contextPack = contextPack;

	pRequestsLock->Lock();
	this->requests.push_front(request);
	pRequestsLock->Unlock();
}

void HTTPSessionManager::Download(IPluginContext *pCtx, 
	HTTPRequestHandleSet handles, const char *url, 
	HTTPRequestCompletedContextPack contextPack)
{
	HTTPRequest request;
	BurnSessionHandle(pCtx, handles);

	request.pCtx = pCtx;
	request.handles = handles;
	request.method = HTTP_GET;
	request.url = url;
	request.contextPack = contextPack;

	pRequestsLock->Lock();
	this->requests.push_front(request);
	pRequestsLock->Unlock();
}

void HTTPSessionManager::BurnSessionHandle(IPluginContext *pCtx, 
	HTTPRequestHandleSet &handles)
{
	HandleSecurity sec;
	sec.pOwner = pCtx->GetIdentity();
	sec.pIdentity = myself->GetIdentity();

	// TODO: maybe better way to do this?
	// Make session handle inaccessible to the user
	Handle_t hndlNew = g_pHandleSys->FastCloneHandle(handles.hndlSession);
	if (g_pHandleSys->FreeHandle(handles.hndlSession, &sec) != 
		HandleError_None)
	{
		pCtx->ThrowNativeError("Couldn't free HTTP session handle");
		return;
	}

	handles.hndlSession = hndlNew;
}

void HTTPSessionManager::RunFrame()
{
	// Try to execute pending callbacks
	if (this->pCallbacksLock->TryLock())
	{
		if (!this->callbacks.empty())
		{
			HTTPRequest request = this->callbacks.front();
			IPluginContext *pCtx = request.pCtx;

			// Is the requesting plugin still alive?
			if (pCtx != NULL)
			{
				// TODO: this may be unnecessary
				if (plsys->FindPluginByContext(request.pCtx->GetContext()))
				{
					funcid_t id = request.contextPack.pCallbackFunction->uPluginFunction;
					IPluginFunction *pFunction = NULL;
					pFunction = pCtx->GetFunctionById(id);

					if (pFunction != NULL)
					{
						// Push data and execute callback
						pFunction->PushCell(request.handles.hndlSession);
						pFunction->PushCell(request.result);
						pFunction->PushCell(request.handles.hndlDownloader);
						if (request.contextPack.pCallbackFunction->bHasContext)
						{
							pFunction->PushCell(request.contextPack.iPluginContextValue);
						}
						pFunction->Execute(NULL);
					}
				}
			}

			this->callbacks.pop_front();
		}

		this->pCallbacksLock->Unlock();
	}

	// Try to fire up some new asynchronous requests
	if (pRequestsLock->TryLock())
	{
		if (!this->requests.empty())
		{
			// NOTE: this is my "burst thread creation" solution
			// Using a thread pool is slow as it executes the threads
			// sequentially and not parallel.
			// Not using a thread pool might cause SRCDS to crash, so
			// we are spawning just a few threads every frame to not
			// affect performance too much and still having the advantage
			// of parallel execution.
			for (unsigned int i = 0; i < iMaxRequestsPerFrame; i++)
			{
				// Create new thread object
				HTTPAsyncRequestHandler *async = 
					new HTTPAsyncRequestHandler(this->requests.front());
				// Skip requests with unloaded parent plugin
				if (this->requests.front().pCtx != NULL)
				{
					// Create new thread
					IThreadHandle *pThread = 
						threader->MakeThread(async, Thread_Default);
					// Save thread handle
					this->threads.push_front(pThread);
				}
				// Remove request as it's being handled now
				this->requests.pop_front();
			}
		}

		pRequestsLock->Unlock();
	}

	// Do some quick "garbage collection" on finished threads
	if (!this->threads.empty())
	{
		for (std::list<IThreadHandle*>::iterator i(threads.begin()), end(threads.end()); i != end; ++i)
		{
			if ((*i) != NULL)
			{
				if ((*i)->GetState() == Thread_Done)
				{
					(*i)->DestroyThis();
					this->threads.remove((*i));
					// NOTE: this action breaks the iteration so we have
					// to leave the loop and start over on next frame
					break;
				}
			}
		}
	}
}

void HTTPSessionManager::Initialize()
{
	pRequestsLock = threader->MakeMutex();
	pCallbacksLock = threader->MakeMutex();
}

void HTTPSessionManager::Cleanup()
{
	if (pRequestsLock != NULL)
	{
		pRequestsLock->DestroyThis();
	}

	if (pCallbacksLock != NULL)
	{
		pCallbacksLock->DestroyThis();
	}
}

void HTTPSessionManager::AddCallback(HTTPRequest request)
{
	this->pCallbacksLock->Lock();
	this->callbacks.push_front(request);
	this->pCallbacksLock->Unlock();
}

void HTTPSessionManager::HTTPAsyncRequestHandler::RunThread(IThreadHandle *pHandle)
{
	HandleError err;
	HandleSecurity sec;
	sec.pOwner = this->request.pCtx->GetIdentity();
	sec.pIdentity = myself->GetIdentity();

	IWebTransfer *xfer = NULL;
	g_pHandleSys->ReadHandle(this->request.handles.hndlSession,
		g_SessionHandle, &sec, (void **)&xfer);

	IBaseDownloader *downldr = NULL;
	g_pHandleSys->ReadHandle(this->request.handles.hndlDownloader,
		g_DownloadHandle, &sec, (void **)&downldr);

	switch (this->request.method)
	{
	case HTTP_GET:
		this->request.result = 
			xfer->Download(this->request.url, downldr, NULL);
		break;
	case HTTP_POST:
		IWebForm *form = NULL;
		g_pHandleSys->ReadHandle(this->request.handles.hndlForm,
			g_FormHandle, &sec, (void **)&form);

		this->request.result =
			xfer->PostAndDownload(this->request.url, form, downldr, NULL);
		break;
	}

	g_SessionManager.AddCallback(request);
}
