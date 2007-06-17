#include "extension.h"
#include "vcallbuilder.h"

enum SDKLibrary
{
	SDKLibrary_Server,	/**< server.dll/server_i486.so */
	SDKLibrary_Engine,	/**< engine.dll/engine_*.so */
};

enum SDKPassMethod
{
	SDKPass_Pointer,		/**< Pass as a pointer */
	SDKPass_Plain,			/**< Pass as plain data */
	SDKPass_ByValue,		/**< Pass an object by value */
	SDKPass_ByRef,			/**< Pass an object by reference */
};

int s_vtbl_index = 0;
void *s_call_addr = NULL;
ValveCallType s_vcalltype = ValveCall_Static;
bool s_has_return = false;
ValvePassInfo s_return;
unsigned int s_numparams = 0;
ValvePassInfo s_params[SP_MAX_EXEC_PARAMS];

inline void DecodePassMethod(ValveType vtype, SDKPassMethod method, PassType &type, unsigned int &flags)
{
	if (method == SDKPass_Pointer)
	{
		type = PassType_Basic;
		if (vtype == Valve_POD
			|| vtype == Valve_Float
			|| vtype == Valve_Bool)
		{
			flags = PASSFLAG_ASPOINTER;
		} else {
			flags = PASSFLAG_BYVAL;
		}
	} else if (method == SDKPass_Plain) {
		type = PassType_Basic;
		flags = PASSFLAG_BYVAL;
	} else if (method == SDKPass_ByValue) {
		if (vtype == Valve_Vector
			|| vtype == Valve_QAngle)
		{
			type = PassType_Object;
		} else {
			type = PassType_Basic;
		}
		flags = PASSFLAG_BYVAL;
	} else if (method == SDKPass_ByRef) {
		if (vtype == Valve_Vector
			|| vtype == Valve_QAngle)
		{
			type = PassType_Object;
		} else {
			type = PassType_Basic;
		}
		flags = PASSFLAG_BYREF;
	}
}

static cell_t StartPrepSDKCall(IPluginContext *pContext, const cell_t *params)
{
	s_numparams = 0;
	s_vtbl_index = 0;
	s_call_addr = NULL;
	s_has_return = false;
	s_vcalltype = (ValveCallType)params[1];

	return 1;
}

static cell_t PrepSDKCall_SetVirtual(IPluginContext *pContext, const cell_t *params)
{
	s_vtbl_index = params[1];

	return 1;
}

static cell_t PrepSDKCall_SetSignature(IPluginContext *pContext, const cell_t *params)
{
	void *addrInBase = NULL;
	if (params[1] == SDKLibrary_Server)
	{
		addrInBase = (void *)g_SMAPI->serverFactory(false);
	} else if (params[1] == SDKLibrary_Engine) {
		addrInBase = (void *)g_SMAPI->engineFactory(false);
	}
	if (addrInBase == NULL)
	{
		return 0;
	}

	char *sig;
	pContext->LocalToString(params[2], &sig);

#if defined PLATFORM_LINUX
	if (sig[0] == '@')
	{
		Dl_info info;
		if (dladdr(addrInBase, &info) == 0)
		{
			return 0;
		}
		void *handle = dlopen(info.dli_fname, RTLD_NOW);
		if (!handle)
		{
			return 0;
		}
		s_call_addr = dlsym(handle, &sig[1]);
		dlclose(handle);

		return (s_call_addr != NULL) ? 1 : 0;
	}
#endif

	s_call_addr = memutils->FindPattern(addrInBase, sig, params[3]);

	return (s_call_addr != NULL) ? 1 : 0;
}

static cell_t PrepSDKCall_SetFromConf(IPluginContext *pContext, const cell_t *params)
{
	IGameConfig *conf;

	if (params[1] == BAD_HANDLE)
	{
		conf = g_pGameConf;
	} else {
		HandleError err;
		if ((conf = gameconfs->ReadHandle(params[1], pContext->GetIdentity(), &err)) == NULL)
		{
			return pContext->ThrowNativeError("Invalid Handle %x (error %d)", params[1], err);
		}
	}
	
	char *key;
	pContext->LocalToString(params[3], &key);

	if (params[2] == 0)
	{
		return conf->GetOffset(key, &s_vtbl_index) ? 1 : 0;
	} else if (params[2] == 1) {
		bool result = conf->GetMemSig(key, &s_call_addr) ? 1 : 0;
		return (result && s_call_addr != NULL) ? 1 : 0;
	}

	return 0;
}

static cell_t PrepSDKCall_SetReturnInfo(IPluginContext *pContext, const cell_t *params)
{
	s_has_return = true;
	s_return.vtype = (ValveType)params[1];
	DecodePassMethod(s_return.vtype, (SDKPassMethod)params[2], s_return.type, s_return.flags);
	s_return.decflags = params[3];
	s_return.encflags = params[4];

	return 1;
}

static cell_t PrepSDKCall_AddParameter(IPluginContext *pContext, const cell_t *params)
{
	if (s_numparams >= SP_MAX_EXEC_PARAMS)
	{
		return pContext->ThrowNativeError("Parameter limit for SDK calls reached");
	}

	ValvePassInfo *info = &s_params[s_numparams++];
	info->vtype = (ValveType)params[1];
	DecodePassMethod(info->vtype, (SDKPassMethod)params[2], info->type, info->flags);
	info->decflags = params[3] | VDECODE_FLAG_BYREF;
	info->encflags = params[4];

	return 1;
}

static cell_t EndPrepSDKCall(IPluginContext *pContext, const cell_t *params)
{
	ValveCall *vc = NULL;
	if (s_vtbl_index)
	{
		vc = CreateValveVCall(s_vtbl_index, s_vcalltype, s_has_return ? &s_return : NULL, s_params, s_numparams);
	} else if (s_call_addr) {
		vc = CreateValveCall(s_call_addr, s_vcalltype, s_has_return ? &s_return : NULL, s_params, s_numparams);
	}

	if (!vc)
	{
		return BAD_HANDLE;
	}

	if (vc->thisinfo)
	{
		vc->thisinfo->decflags |= VDECODE_FLAG_BYREF;
	}

	Handle_t hndl = handlesys->CreateHandle(g_CallHandle, vc, pContext->GetIdentity(), myself->GetIdentity(), NULL);
	if (!hndl)
	{
		delete vc;
	}

	return hndl;
}

static cell_t SDKCall(IPluginContext *pContext, const cell_t *params)
{
	ValveCall *vc;
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	if ((err = handlesys->ReadHandle(params[1], g_CallHandle, &sec, (void **)&vc)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", params[1], err);
	}

	unsigned char *ptr = vc->stk_get();

	unsigned int numparams = (unsigned)params[0];
	unsigned int startparam = 2;
	/* Do we need to write a thispointer?  */

	if (vc->thisinfo && 
		(vc->thisinfo->vtype == Valve_CBaseEntity
		 || vc->thisinfo->vtype == Valve_CBasePlayer))
	{
		if (startparam > numparams)
		{
			vc->stk_put(ptr);
			return pContext->ThrowNativeError("Expected 1 parameter for entity pointer; found none");
		}
		if (DecodeValveParam(pContext, 
			params[startparam], 
			vc->thisinfo,
			ptr) == Data_Fail)
		{
			vc->stk_put(ptr);
			return 0;
		}
		startparam++;
	}

	/* See if we need to skip any more parameters */
	unsigned int retparam = startparam;
	if (vc->retinfo)
	{
		if (vc->retinfo->vtype == Valve_String)
		{
			startparam += 2;
		} else if (vc->retinfo->vtype == Valve_Vector
					|| vc->retinfo->vtype == Valve_QAngle)
		{
			startparam += 1;
		}
	}

	unsigned int callparams = vc->call->GetParamCount();
	bool will_copyback = false;
	for (unsigned int i=0; i<callparams; i++)
	{
		unsigned int p = startparam + i;
		if (p > numparams)
		{
			vc->stk_put(ptr);
			return pContext->ThrowNativeError("Expected %dth parameter, found none", p);
		}
		if (DecodeValveParam(pContext,
			params[p],
			&(vc->vparams[i]),
			ptr + vc->vparams[i].offset) == Data_Fail)
		{
			vc->stk_put(ptr);
			return 0;
		}
		if (vc->vparams[i].encflags & VENCODE_FLAG_COPYBACK)
		{
			will_copyback = true;
		}
	}

	/* Make the actual call */
	vc->call->Execute(ptr, vc->retbuf);

	/* Do we need to copy anything back? */
	if (will_copyback)
	{
		for (unsigned int i=0; i<callparams; i++)
		{
			if (vc->vparams[i].encflags & VENCODE_FLAG_COPYBACK)
			{
				if (EncodeValveParam(pContext, 
					startparam + i, 
					&vc->vparams[i],
					ptr + vc->vparams[i].offset) == Data_Fail)
				{
					vc->stk_put(ptr);
					return 0;
				}
			}
		}
	}

	/* Save stack once and for all */
	vc->stk_put(ptr);

	/* Figure out how to decode the return information */
	if (vc->retinfo)
	{
		if (vc->retinfo->vtype == Valve_String)
		{
			if (numparams < 3)
			{
				return pContext->ThrowNativeError("Expected arguments (2,3) for string storage");
			}
			cell_t *addr;
			size_t written;
			pContext->LocalToPhysAddr(params[retparam+1], &addr);
			pContext->StringToLocalUTF8(params[retparam], *addr, *(char **)vc->retbuf, &written);
			return (cell_t)written;
		} else if (vc->retinfo->vtype == Valve_Vector
					|| vc->retinfo->vtype == Valve_QAngle)
		{
			if (numparams < 2)
			{
				return pContext->ThrowNativeError("Expected argument (2) for Float[3] storage");
			}
			if (EncodeValveParam(pContext, params[retparam], vc->retinfo, vc->retbuf)
				== Data_Fail)
			{
				return 0;
			}
		} else if (vc->retinfo->vtype == Valve_CBaseEntity
					|| vc->retinfo->vtype == Valve_CBasePlayer)
		{
			CBaseEntity *pEntity = *(CBaseEntity **)(vc->retbuf);
			if (!pEntity)
			{
				return -1;
			}
			edict_t *pEdict = gameents->BaseEntityToEdict(pEntity);
			if (!pEdict || pEdict->IsFree())
			{
				return -1;
			}
			return engine->IndexOfEdict(pEdict);
		} else if (vc->retinfo->vtype == Valve_Edict) {
			edict_t *pEdict = *(edict_t **)(vc->retbuf);
			if (!pEdict || pEdict->IsFree())
			{
				return -1;
			}
			return engine->IndexOfEdict(pEdict);
		} else if (vc->retinfo->vtype == Valve_Bool) {
			bool *addr = (bool  *)vc->retbuf;
			if (vc->retinfo->flags & PASSFLAG_BYREF)
			{
				addr = *(bool **)addr;
			}
			return *addr ? 1 : 0;
		} else {
			cell_t *addr = (cell_t *)vc->retbuf;
			if (vc->retinfo->flags & PASSFLAG_BYREF)
			{
				addr = *(cell_t **)addr;
			}
			return *addr;
		}
	}

	return 0;
}

sp_nativeinfo_t g_CallNatives[] = 
{
	{"StartPrepSDKCall",			StartPrepSDKCall},
	{"PrepSDKCall_SetVirtual",		PrepSDKCall_SetVirtual},
	{"PrepSDKCall_SetSignature",	PrepSDKCall_SetSignature},
	{"PrepSDKCall_SetFromConf",		PrepSDKCall_SetFromConf},
	{"PrepSDKCall_SetReturnInfo",	PrepSDKCall_SetReturnInfo},
	{"PrepSDKCall_AddParameter",	PrepSDKCall_AddParameter},
	{"EndPrepSDKCall",				EndPrepSDKCall},
	{"SDKCall",						SDKCall},
	{NULL,							NULL},
};
