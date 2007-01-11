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
	HandleSecurity sec;

	sec.pIdentity = NULL;
	sec.pOwner = pContext->GetIdentity();

	HandleError err = g_HandleSys.FreeHandle(hndl, &sec);
  
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
	HandleError err;
	IdentityToken_t *pNewOwner;
	
	if (params[2] == 0)
	{
		pNewOwner = pContext->GetIdentity();
	} else {
		Handle_t hPlugin = static_cast<Handle_t>(params[2]);
		IPlugin *pPlugin = g_PluginSys.PluginFromHandle(hPlugin, &err);
		if (!pPlugin)
		{
			return pContext->ThrowNativeError("Plugin handle %x is invalid (error %d)", hndl, err);
		}
		pNewOwner = pPlugin->GetIdentity();
	}

	err = g_HandleSys.CloneHandle(hndl, &new_hndl, pNewOwner, NULL);

	if (err == HandleError_Access)
	{
		return 0;
	} else if (err == HandleError_None) {
		return new_hndl;
	} else {
		return pContext->ThrowNativeError("Handle %x cannot be cloned because it is invalid (error %d)", hndl, err);
	}
}

REGISTER_NATIVES(handles)
{
	{"IsValidHandle",			sm_IsValidHandle},
	{"CloseHandle",				sm_CloseHandle},
	{"CloneHandle",				sm_CloneHandle},
	{NULL,						NULL},
};
