#ifndef _INCLUDED_WEB_CALLBACK_H_
#define _INCLUDED_WEB_CALLBACK_H_
#include "curlapi.h"
#include <IHandleSys.h>
#include <sp_vm_api.h>

class WebCallback
{
public:
	SourcePawn::IPluginContext* pCtx;
private:
	SourceMod::Handle_t webHandle;
	SourcePawn::IPluginFunction* callback;
	SourcePawn::IPluginFunction* error;
	cell_t userData;
public:
	WebCallback(SourceMod::Handle_t handle, SourcePawn::IPluginContext* plugin, funcid_t callbackId, funcid_t errorId, cell_t data) : pCtx(plugin), webHandle(handle), userData(data)
	{
		callback = plugin->GetFunctionById(callbackId);
		error = plugin->GetFunctionById(errorId);
	};
public:
	IWebTransfer* GetSession();
	DownloadWriteStatus ExecuteCallback(const char* data);
	void ExecuteError();
};

extern SourceMod::HandleType_t WebSessionType;
extern SourceMod::HandleType_t WebFormType;
#endif//_INCLUDED_WEB_CALLBACK_H_