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

HandleType_t g_SessionHandle = 0;
HandleType_t g_FormHandle = 0;
HandleType_t g_DownloadHandle = 0;
HTTPHandleDispatcher g_HTTPHandler;
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
	HandleAccess hacc;
	g_pHandleSys->InitAccessDefaults(NULL, &hacc);
	hacc.access[HandleAccess_Clone] = HANDLE_RESTRICT_IDENTITY;
	g_SessionHandle = g_pHandleSys->CreateType("HTTPSession", 
		&g_HTTPHandler, 0, 0, &hacc, myself->GetIdentity(), NULL);

	// Register web form handle handler
	g_pHandleSys->InitAccessDefaults(NULL, &hacc);
	hacc.access[HandleAccess_Delete] = HANDLE_RESTRICT_IDENTITY;
	g_FormHandle = g_pHandleSys->CreateType("HTTPWebForm",
		&g_HTTPHandler, 0, 0, &hacc, myself->GetIdentity(), NULL);

	// Register download handle handler
	g_pHandleSys->InitAccessDefaults(NULL, &hacc);
	hacc.access[HandleAccess_Clone] = HANDLE_RESTRICT_IDENTITY;
	g_DownloadHandle = g_pHandleSys->CreateType("HTTPDownloader",
		&g_HTTPHandler, 0, 0, &hacc, myself->GetIdentity(), NULL);

	// Register game listeners
	plsys->AddPluginsListener(this);
	g_SessionManager.Initialize();
	smutils->AddGameFrameHook(&OnGameFrame);
	
	return true;
}

void CurlExt::SDK_OnUnload()
{
	g_pHandleSys->RemoveType(g_SessionHandle, myself->GetIdentity());
	g_pHandleSys->RemoveType(g_FormHandle, myself->GetIdentity());
	g_pHandleSys->RemoveType(g_DownloadHandle, myself->GetIdentity());
	plsys->RemovePluginsListener(this);
	smutils->RemoveGameFrameHook(&OnGameFrame);
	g_SessionManager.Shutdown();
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
	// Wait for running requests to finish
	/* NOTE: when a plugin gets unloaded all associated handles get
	 * automatically destroyed, so running threads may access invalid
	 * memory and crash the host process. This blocking "solution" may
	 * have to be re factored to a better solution.
	 */
	{
		ke::AutoLock lock(&threads_);

		if (!threads.empty())
		{
			for (ke::LinkedList<IThreadHandle*>::iterator i(threads.begin()), end(threads.end()); i != end; ++i)
			{
				if ((*i) != NULL)
				{
					(*i)->WaitForThread();
					(*i)->DestroyThis();
					i = this->threads.erase(i);
				}
			}
		}
	}
}

void HTTPSessionManager::PostAndDownload(IPluginContext *pCtx, 
	HTTPRequestHandleSet handles, const char *url, 
	HTTPRequestCompletedContextPack contextPack)
{
	HTTPRequest request = {};
	BurnSessionHandle(pCtx, handles);

	request.plugin = FindPluginByContext(pCtx)->GetMyHandle();
	request.handles = handles;
	request.method = HTTP_POST;
	request.url = url;
	request.contextPack = contextPack;

	m_pWorker->MakeThread(new HTTPAsyncRequestHandler(request), Thread_AutoRelease);
}

void HTTPSessionManager::Download(IPluginContext *pCtx, 
	HTTPRequestHandleSet handles, const char *url, 
	HTTPRequestCompletedContextPack contextPack)
{
	HTTPRequest request = {};
	BurnSessionHandle(pCtx, handles);

	request.plugin = FindPluginByContext(pCtx)->GetMyHandle();
	request.handles = handles;
	request.method = HTTP_GET;
	request.url = url;
	request.contextPack = contextPack;

	m_pWorker->MakeThread(new HTTPAsyncRequestHandler(request), Thread_AutoRelease);
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
	if (callbacks_.DoTryLock())
	{
		if (!callbacks.empty())
		{
			HTTPRequest request = this->callbacks[0];
			HandleError herr;
			IPlugin *parent = plsys->PluginFromHandle(request.plugin, &herr);

			// Is the requesting plugin still alive?
			if (parent != NULL && herr != HandleError_Freed)
			{
				IPluginContext *pCtx = parent->GetBaseContext();
				funcid_t id = request.contextPack.pCallbackFunction->uPluginFunction;
				IPluginFunction *pFunction = pCtx->GetFunctionById(id);

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

			callbacks.remove(0);
		}

		callbacks_.DoUnlock();
	}

	// Do some quick "garbage collection" on finished threads
	this->RemoveFinishedThreads();
}

void HTTPSessionManager::Shutdown()
{
	// Block until all running threads have finished
	this->RemoveFinishedThreads();

	// Destroy all remaining callback calls
	{
		ke::AutoLock lock(&callbacks_);
		this->callbacks.clear();
	}

	threader->DestroyWorker(m_pWorker);
}

void HTTPSessionManager::AddCallback(HTTPRequest request)
{
	ke::AutoLock lock(&callbacks_);
	this->callbacks.append(request);
}

void HTTPSessionManager::RemoveFinishedThreads()
{
	if (threads_.DoTryLock())
	{
		if (!this->threads.empty())
		{
			for (ke::LinkedList<IThreadHandle*>::iterator i(threads.begin()), end(threads.end()); i != end; ++i)
			{
				if ((*i) != NULL)
				{
					if ((*i)->GetState() == Thread_Done)
					{
						(*i)->DestroyThis();
						i = this->threads.erase(i);
					}
				}
			}
		}

		threads_.DoUnlock();
	}
}

void HTTPSessionManager::Initialize()
{
	m_pWorker = threader->MakeWorker(NULL, true);
	m_pWorker->Start();
}

void HTTPSessionManager::HTTPAsyncRequestHandler::RunThread(IThreadHandle *pHandle)
{
	HandleError herr;
	IPlugin *parent = plsys->PluginFromHandle(request.plugin, &herr);

	if (parent == NULL || herr == HandleError_Freed)
	{
		// The parent plugin got unloaded, no more work to do
		return;
	}

	HandleSecurity sec;
	sec.pOwner = parent->GetIdentity();
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

void HTTPHandleDispatcher::OnHandleDestroy(HandleType_t type, void *object)
{
	if (type == g_SessionHandle)
	{
		delete ((IWebTransfer *)object);
	}
	else if (type == g_DownloadHandle)
	{
		delete ((IBaseDownloader *)object);
	}
	else if (type == g_FormHandle)
	{
		delete ((IWebForm *)object);
	}
}

IPlugin *FindPluginByContext(IPluginContext *pContext) {
	IPlugin *pFoundPlugin;

	IPluginIterator *pPluginIterator = plsys->GetPluginIterator();
	while (pPluginIterator->MorePlugins())
	{
		IPlugin *pPlugin = pPluginIterator->GetPlugin();

		if (pPlugin->GetBaseContext() == pContext)
		{
			pFoundPlugin = pPlugin;
			break;
		}

		pPluginIterator->NextPlugin();
	}
	pPluginIterator->Release();

	return pFoundPlugin;
}
