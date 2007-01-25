#ifndef _INCLUDE_SOURCEMOD_CDBGREPORTER_H_
#define _INCLUDE_SOURCEMOD_CDBGREPORTER_H_

#include "sp_vm_api.h"
#include "sm_globals.h"

class CDbgReporter : 
	public SMGlobalClass, 
	public IDebugListener
{
public: // SMGlobalClass
	void OnSourceModAllInitialized();
public: // IDebugListener
	void OnContextExecuteError(IPluginContext *ctx, IContextTrace *error);
private:
	int _GetPluginIndex(IPluginContext *ctx);
};

#endif // _INCLUDE_SOURCEMOD_CDBGREPORTER_H_

