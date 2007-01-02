#include "sm_globals.h"
#include "HandleSys.h"
#include "PluginSys.h"

static cell_t sm_IsValidHandle(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);

	HandleError err = g_HandleSys.ReadHandle(hndl, 0, NULL, NULL);

	if (err != HandleError_Access
		&& err != HandleError_None)
	{
		return 0;
	}

	return 1;
}

static cell_t sm_CloseHandle(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);

	/* :TODO: make this a little bit cleaner, eh? */
	IPlugin *pPlugin = g_PluginSys.FindPluginByContext(pContext->GetContext());

	HandleError err = g_HandleSys.FreeHandle(hndl, pPlugin->GetIdentity(), NULL);
  
	if (err == HandleError_None)
	{
		return 1;
	} else if (err == HandleError_Access) {
		return 0;
	} else {
		return pContext->ThrowNativeError("Handle %x is invalid (error %d)", hndl, err);
	}
}

static cell_t sm_CloneHandle(IPluginContext *pContext, const cell_t *params)
{
	Handle_t new_hndl;
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	IPlugin *pPlugin;
	HandleError err;
	
	if (params[2] == 0)
	{
		pPlugin = g_PluginSys.FindPluginByContext(pContext->GetContext());
	} else {
		Handle_t hPlugin = static_cast<Handle_t>(params[2]);
		pPlugin = g_PluginSys.PluginFromHandle(hPlugin, &err);
		if (!pPlugin)
		{
			return pContext->ThrowNativeError("Plugin handle %x is invalid (error %d)", hndl, err);
		}
	}

	err = g_HandleSys.CloneHandle(hndl, &new_hndl, pPlugin->GetIdentity(), NULL);

	if (err == HandleError_Access)
	{
		return 0;
	} else if (err == HandleError_None) {
		return new_hndl;
	} else {
		return pContext->ThrowNativeError("Handle to clone %x is invalid (error %d)", hndl, err);
	}
}

REGISTER_NATIVES(handles)
{
	{"IsValidHandle",			sm_IsValidHandle},
	{"CloseHandle",				sm_CloseHandle},
	{"CloneHandle",				sm_CloneHandle},
	{NULL,						NULL},
};
