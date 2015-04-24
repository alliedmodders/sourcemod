#ifndef _INCLUDE_SP_CALLER_H_
#define _INCLUDE_SP_CALLER_H_

#include "curlapi.h"
#include <IHandleSys.h>
#include <sp_vm_api.h>

class SPCaller
{
private:
	funcid_t onError;
	funcid_t onDownload;
	SourcePawn::IPluginContext* pCtx;
	SourceMod::Handle_t webSession;
public:
	SPCaller();
	SPCaller(funcid_t onError, funcid_t onDownload, SourcePawn::IPluginContext* pCtx, SourceMod::Handle_t webSession);
public:
	void Call_OnError();
	DownloadWriteStatus Call_OnDownloadWrite(const char* data, cell_t userData);
	SourcePawn::IPluginContext* GetPlugin(){ return pCtx; };
	SourceMod::Handle_t GetSession(){ return webSession; };
};

#endif