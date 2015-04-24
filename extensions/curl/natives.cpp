#include "extension.h"
#include "curlapi.h"
#include "web_thread.h"

static cell_t smn_CreateWebSession(IPluginContext* pCtx, const cell_t* params)
{
	Handle_t ret = BAD_HANDLE;
	IWebTransfer* session = g_webternet.CreateSession();
	if (session == NULL)
	{
		pCtx->ReportError("Unable to create web session.");
		return BAD_HANDLE;
	}
	WebHandle webHandle(session);
	HandleError err = HandleError_None;
	ret = handlesys->CreateHandle(WebSessionType, (void*)&webHandle, pCtx->GetIdentity(), myself->GetIdentity(), &err);
	if (ret == BAD_HANDLE)
		pCtx->ReportError("Unable to create new handle.");
	return ret;
}

static cell_t smn_SetPostParam(IPluginContext* pCtx, const cell_t* params)
{
	HandleSecurity sec(pCtx->GetIdentity(), myself->GetIdentity());
	WebHandle session;
	if (handlesys->ReadHandle(params[1], WebSessionType, &sec, (void**)&session) != HandleError_None)
	{
		pCtx->ReportError("Invalid web session handle %x", params[1]);
		return (cell_t)false;
	}
	if (session.webForm != BAD_HANDLE)
	{
		pCtx->ReportError("Session already has params set.");
		return (cell_t)false;
	}
	session.webForm = params[2];
	return (cell_t)true;
}

static cell_t smn_SetHttpError(IPluginContext* pCtx, const cell_t* params)
{
	HandleSecurity sec(pCtx->GetIdentity(), myself->GetIdentity());
	WebHandle session;
	if (handlesys->ReadHandle(params[1], WebSessionType, &sec, (void**)&session) != HandleError_None)
	{
		pCtx->ReportError("Invalid web session handle %x", params[1]);
		return (cell_t)false;
	}
	return (cell_t)session.GetSession()->SetFailOnHTTPError((bool)params[2]);
}

static cell_t smn_SetHeaderReturn(IPluginContext* pCtx, const cell_t* params)
{
	HandleSecurity sec(pCtx->GetIdentity(), myself->GetIdentity());
	WebHandle session;
	if (handlesys->ReadHandle(params[1], WebSessionType, &sec, (void**)&session) != HandleError_None)
	{
		pCtx->ReportError("Invalid web session handle %x", params[1]);
		return (cell_t)false;
	}
	return (cell_t)session.GetSession()->SetHeaderReturn((bool)params[2]);
}

static cell_t smn_GetLastErrorCode(IPluginContext* pCtx, const cell_t* params)
{
	HandleSecurity sec(pCtx->GetIdentity(), myself->GetIdentity());
	WebHandle session;
	if (handlesys->ReadHandle(params[1], WebSessionType, &sec, (void**)&session) != HandleError_None)
	{
		pCtx->ReportError("Invalid web session handle %x", params[1]);
		return (cell_t)false;
	}
	return session.GetSession()->LastErrorCode();
}

static cell_t smn_GetLastErrorMessage(IPluginContext* pCtx, const cell_t* params)
{
	HandleSecurity sec(pCtx->GetIdentity(), myself->GetIdentity());
	WebHandle session;
	if (handlesys->ReadHandle(params[1], WebSessionType, &sec, (void**)&session) != HandleError_None)
	{
		pCtx->ReportError("Invalid web session handle %x", params[1]);
		return (cell_t)false;
	}
	size_t bytesWritten;
	pCtx->StringToLocalUTF8(params[2], params[3], session.GetSession()->LastErrorMessage(), &bytesWritten);
	return bytesWritten;
}

static cell_t smn_QueueDownloadString(IPluginContext* pCtx, const cell_t* params)
{
	SPCaller bridge(params[3], params[4], pCtx, params[1]);
	char* url;
	pCtx->LocalToString(params[2], &url);
	WebQueueObject obj(url, bridge, params[5]);
	return (cell_t)gWebThread->InsertIntoQueue(obj);
}

static cell_t smn_CreateWebForm(IPluginContext* pCtx, const cell_t* params)
{
	Handle_t ret = BAD_HANDLE;
	IWebForm* form = g_webternet.CreateForm();
	if (form == NULL)
	{
		pCtx->ReportError("Unable to create web form.");
		return BAD_HANDLE;
	}
	HandleError err = HandleError_None;
	ret = handlesys->CreateHandle(WebFormType, (void*)&form, pCtx->GetIdentity(), myself->GetIdentity(), &err);
	if (ret == BAD_HANDLE)
		pCtx->ReportError("Unable to create new handle.");
	return ret;
}

static cell_t smn_Add(IPluginContext* pCtx, const cell_t* params)
{
	HandleSecurity sec(pCtx->GetIdentity(), myself->GetIdentity());
	IWebForm* form;
	if (handlesys->ReadHandle(params[1], WebFormType, &sec, (void**)&form) != HandleError_None)
	{
		pCtx->ReportError("Invalid web form handle %x", params[1]);
		return (cell_t)false;
	}
	char* param, *value;
	pCtx->LocalToString(params[2], &param);
	pCtx->LocalToString(params[2], &value);
	return form->AddString(param, value);
}

sp_nativeinfo_t webNatives[] = {
	{ "WebSession.WebSession", smn_CreateWebSession },
	{ "WebSession.SetPostParam", smn_SetPostParam },
	{ "WebSession.SetErrorOnHttpError", smn_SetHttpError },
	{ "WebSession.SetHeaderReturn", smn_SetHeaderReturn },
	{ "WebSession.GetLastErrorCode", smn_GetLastErrorCode },
	{ "WebSession.GetLastErrorMessage", smn_GetLastErrorMessage },
	{ "WebSession.QueueDownloadString", smn_QueueDownloadString },
	{ "WebForm.WebForm", smn_CreateWebForm },
	{ "WebForm.AddString", smn_Add },
	{ "WebForm.AddFile", smn_Add },
	{ NULL, NULL },
};

void CurlExt::SDK_OnAllLoaded()
{
	sharesys->AddNatives(myself, webNatives);
}

void CurlExt::OnHandleDestroy(HandleType_t type, void *object)
{
	if (type == WebSessionType)
		delete static_cast<WebHandle*>(object);
	if (type == WebFormType)
		delete static_cast<IWebForm*>(object);
}