#include "web_executer.h"
#include "extension.h"

using namespace SourceMod;
using namespace SourcePawn;

IThreadWorker* worker;

bool BeginWorker()
{
	worker = threader->MakeWorker(NULL, true);
	return worker != NULL && worker->Start();
}

void EndWorker()
{
	worker->Stop(false);
	delete worker;
}

static cell_t smn_CreateForm(IPluginContext* pCtx, const cell_t* params)
{
	IWebForm* form = g_webternet.CreateForm();
	if (!form)
	{
		pCtx->ReportError("Unable to create new web form.");
		return BAD_HANDLE;
	}
	Handle_t webFormHandle = handlesys->CreateHandle(WebFormType, form, pCtx->GetIdentity(), myself->GetIdentity(), NULL);
	return webFormHandle;
}

static cell_t smn_AddString(IPluginContext* pCtx, const cell_t* params)
{
	HandleSecurity security(pCtx->GetIdentity(), myself->GetIdentity());
	IWebForm* form;
	if (handlesys->ReadHandle((Handle_t)params[1], WebFormType, &security, (void**)&form) != HandleError_None)
	{
		pCtx->ReportError("Invalid handle %x", params[1]);
		return 0;
	}
	char* paramName, *paramValue;
	pCtx->LocalToString(params[2], &paramName);
	pCtx->LocalToString(params[3], &paramValue);
	return static_cast<cell_t>(form->AddString(paramName, paramValue));
}

static cell_t smn_AddFile(IPluginContext* pCtx, const cell_t* params)
{
	HandleSecurity security(pCtx->GetIdentity(), myself->GetIdentity());
	IWebForm* form;
	if (handlesys->ReadHandle((Handle_t)params[1], WebFormType, &security, (void**)&form) != HandleError_None)
	{
		pCtx->ReportError("Invalid handle %x", params[1]);
		return 0;
	}
	char* paramName, *paramValue;
	pCtx->LocalToString(params[2], &paramName);
	pCtx->LocalToString(params[3], &paramValue);
	return static_cast<cell_t>(form->AddFile(paramName, paramValue));
}

static cell_t smn_CreateSession(IPluginContext* pCtx, const cell_t* params)
{
	IWebTransfer* session = g_webternet.CreateSession();
	if (!session)
	{
		pCtx->ReportError("Unable to create web session.");
		return BAD_HANDLE;
	}
	Handle_t webHandle = handlesys->CreateHandle(WebSessionType, session, pCtx->GetIdentity(), myself->GetIdentity(), NULL);
	return webHandle;
}

static cell_t smn_ReturnHeader(IPluginContext* pCtx, const cell_t* params)
{
	HandleSecurity security(pCtx->GetIdentity(), myself->GetIdentity());
	IWebTransfer* session;
	if (handlesys->ReadHandle((Handle_t)params[1], WebSessionType, &security, (void**)&session) != HandleError_None)
	{
		pCtx->ReportError("Invalid handle %x", params[1]);
		return 0;
	}
	return static_cast<cell_t>(session->SetHeaderReturn(static_cast<bool>(params[2])));
}

static cell_t smn_FailOnHttpError(IPluginContext* pCtx, const cell_t* params)
{
	HandleSecurity security(pCtx->GetIdentity(), myself->GetIdentity());
	IWebTransfer* session;
	if (handlesys->ReadHandle((Handle_t)params[1], WebSessionType, &security, (void**)&session) != HandleError_None)
	{
		pCtx->ReportError("Invalid handle %x", params[1]);
		return 0;
	}
	return static_cast<cell_t>(session->SetFailOnHTTPError(static_cast<bool>(params[2])));
}

static cell_t smn_LastErrorCode(IPluginContext* pCtx, const cell_t* params)
{
	HandleSecurity security(pCtx->GetIdentity(), myself->GetIdentity());
	IWebTransfer* session;
	if (handlesys->ReadHandle((Handle_t)params[1], WebSessionType, &security, (void**)&session) != HandleError_None)
	{
		pCtx->ReportError("Invalid handle %x", params[1]);
		return 0;
	}
	return session->LastErrorCode();
}

static cell_t smn_LastErrorMessage(IPluginContext* pCtx, const cell_t* params)
{
	HandleSecurity security(pCtx->GetIdentity(), myself->GetIdentity());
	IWebTransfer* session;
	if (handlesys->ReadHandle((Handle_t)params[1], WebSessionType, &security, (void**)&session) != HandleError_None)
	{
		pCtx->ReportError("Invalid handle %x", params[1]);
		return 0;
	}
	size_t bytesWritten = 0;
	if (pCtx->StringToLocalUTF8(params[2], params[3], session->LastErrorMessage(), &bytesWritten) != SP_ERROR_NONE)
	{
		pCtx->ReportError("Writing to buffer failed.");
		return 0;
	}
	return bytesWritten;
}

static cell_t smn_QueueDownload(IPluginContext* pCtx, const cell_t* params)
{
	char* url;
	pCtx->LocalToString(params[2], &url);
	WebExecuter* thread = new WebExecuter(url, params[1], pCtx, params[3], params[4], params[6], params[5]);
	worker->MakeThread(thread);
	return 0;
}

void CurlExt::OnHandleDestroy(HandleType_t type, void *object)
{
	if (type == WebSessionType)
		delete reinterpret_cast<IWebTransfer*>(object);
}

sp_nativeinfo_t natives[] =
{
	{ "WebForm.WebForm", smn_CreateForm },
	{ "WebForm.AddString", smn_AddString },
	{ "WebForm.AddFile", smn_AddFile },
	{ "WebSession.WebSession", smn_CreateSession },
	{ "WebSession.ReturnHeader", smn_ReturnHeader },
	{ "WebSession.FailOnHttpError", smn_FailOnHttpError },
	{ "WebSession.LastErrorCode", smn_LastErrorCode },
	{ "WebSession.LastErrorMessage", smn_LastErrorMessage },
	{ "WebSession.QueueDownload", smn_QueueDownload },
	{ NULL, NULL },
};

void CurlExt::SDK_OnAllLoaded()
{
	sharesys->AddNatives(myself, natives);
}