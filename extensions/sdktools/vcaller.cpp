/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * =============================================================================
 * SourceMod SDKTools Extension
 * Copyright (C) 2004-2009 AlliedModders LLC.  All rights reserved.
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
 */

#include "extension.h"
#include "vcallbuilder.h"
#include "vglobals.h"

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

//memory addresses below 0x10000 are automatically considered invalid for dereferencing
#define VALID_MINIMUM_MEMORY_ADDRESS 0x10000

int s_vtbl_index = -1;
void *s_call_addr = NULL;
ValveCallType s_vcalltype = ValveCall_Static;
bool s_has_return = false;
ValvePassInfo s_return;
unsigned int s_numparams = 0;
ValvePassInfo s_params[SP_MAX_EXEC_PARAMS];

inline void DecodePassMethod(ValveType vtype, SDKPassMethod method, PassType &type, unsigned int &flags)
{
	if (method == SDKPass_Pointer || method == SDKPass_ByRef)
	{
		type = PassType_Basic;
		if (vtype == Valve_POD
			|| vtype == Valve_Float
			|| vtype == Valve_Bool)
		{
			flags = PASSFLAG_BYVAL | PASSFLAG_ASPOINTER;
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
	}
}

static cell_t StartPrepSDKCall(IPluginContext *pContext, const cell_t *params)
{
	s_numparams = 0;
	s_vtbl_index = -1;
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
		addrInBase = (void *)g_SMAPI->GetServerFactory(false);
	} else if (params[1] == SDKLibrary_Engine) {
		addrInBase = (void *)g_SMAPI->GetEngineFactory(false);
	}
	if (addrInBase == NULL)
	{
		return 0;
	}

	char *sig;
	pContext->LocalToString(params[2], &sig);

	if (sig[0] == '@')
	{
#if defined PLATFORM_WINDOWS
		MEMORY_BASIC_INFORMATION mem;
		if (VirtualQuery(addrInBase, &mem, sizeof(mem)))
		{
			s_call_addr = memutils->ResolveSymbol(mem.AllocationBase, &sig[1]);
		}
#elif defined PLATFORM_POSIX
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

#if SOURCE_ENGINE == SE_CSS            \
	|| SOURCE_ENGINE == SE_HL2DM       \
	|| SOURCE_ENGINE == SE_DODS        \
	|| SOURCE_ENGINE == SE_SDK2013     \
	|| SOURCE_ENGINE == SE_BMS         \
	|| SOURCE_ENGINE == SE_TF2         \
	|| SOURCE_ENGINE == SE_LEFT4DEAD   \
	|| SOURCE_ENGINE == SE_LEFT4DEAD2  \
	|| SOURCE_ENGINE == SE_NUCLEARDAWN \
	|| SOURCE_ENGINE == SE_BLADE       \
	|| SOURCE_ENGINE == SE_INSURGENCY  \
	|| SOURCE_ENGINE == SE_DOI         \
	|| SOURCE_ENGINE == SE_CSGO
		s_call_addr = memutils->ResolveSymbol(handle, &sig[1]);
#else
		s_call_addr = dlsym(handle, &sig[1]);
#endif /* SOURCE_ENGINE */

		dlclose(handle);
#endif

		return (s_call_addr != NULL) ? 1 : 0;
	}

	s_call_addr = memutils->FindPattern(addrInBase, sig, params[3]);

	return (s_call_addr != NULL) ? 1 : 0;
}

static cell_t PrepSDKCall_SetAddress(IPluginContext *pContext, const cell_t *params)
{
#ifdef PLATFORM_X86
	s_call_addr = reinterpret_cast<void *>(params[1]);
#else
	s_call_addr = g_pSM->FromPseudoAddress(params[1]);
#endif

	return (s_call_addr != NULL) ? 1 : 0;
}

// Must match same enum in sdktools.inc
enum SDKFuncConfSource
{
	SDKConf_Virtual,
	SDKConf_Signature,
	SDKConf_Address
};

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

	switch (params[2])
	{
	case SDKConf_Virtual:
		if (conf->GetOffset(key, &s_vtbl_index))
		{
			return 1;
		}
		break;
	case SDKConf_Signature:
		if (conf->GetMemSig(key, &s_call_addr) && s_call_addr)
		{
			return 1;
		}
		break;
	case SDKConf_Address:
		if (conf->GetAddress(key, &s_call_addr) && s_call_addr)
		{
			return 1;
		}
		break;
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
	SDKPassMethod method = (SDKPassMethod)params[2];
	DecodePassMethod(info->vtype, method, info->type, info->flags);
	info->decflags = params[3] | VDECODE_FLAG_BYREF;
	info->encflags = params[4];

	/* Since SDKPass_ByRef acts like SDKPass_Pointer we can't allow NULL, just in case */
	if (method == SDKPass_ByRef)
	{
		info->decflags &= ~VDECODE_FLAG_ALLOWNULL;
	}

	return 1;
}

static cell_t EndPrepSDKCall(IPluginContext *pContext, const cell_t *params)
{
	ValveCall *vc = NULL;
	if (s_vtbl_index > -1)
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

	const unsigned int numparams = (unsigned)params[0];
	unsigned int startparam = 2;
	/* Do we need to write a thispointer?  */

	if (vc->thisinfo)
	{
		switch (vc->type)
		{
		case ValveCall_Entity:
		case ValveCall_Player:
			{
				if (startparam > numparams)
				{
					vc->stk_put(ptr);
					return pContext->ThrowNativeError("Expected 1 parameter for entity pointer; found none");
				}

				if (DecodeValveParam(pContext, 
					params[startparam],
					vc,
					vc->thisinfo,
					ptr) == Data_Fail)
				{
					vc->stk_put(ptr);
					return 0;
				}
				startparam++;
			}
			break;
		case ValveCall_Server:
            {
                if (iserver == NULL)
                {
                    vc->stk_put(ptr);
                    return pContext->ThrowNativeError("Server unsupported or not available; file a bug report");
                }
                *(void **)ptr = iserver;
            }
            break;
		case ValveCall_GameRules:
			{
				void *pGameRules = GameRules();
				if (pGameRules == NULL)
				{
					vc->stk_put(ptr);

					if (g_SdkTools.HasAnyLevelInited())
						return pContext->ThrowNativeError("GameRules unsupported or not available; file a bug report");
					else
						return pContext->ThrowNativeError("GameRules not available before map is loaded");
				}
				*(void **)ptr = pGameRules;
			}
			break;
		case ValveCall_EntityList:
			{
				if (g_EntList == NULL)
				{
					vc->stk_put(ptr);
					return pContext->ThrowNativeError("EntityList unsupported or not available; file a bug report");
				}

				*(void **)ptr = g_EntList;
			}
			break;
		case ValveCall_Raw:
			{
				//params[startparam] is an address to a pointer to THIS
				//params following this are params to the method we will invoke later
				if (startparam > numparams)
				{
					vc->stk_put(ptr);
					return pContext->ThrowNativeError("Expected a ThisPtr address, it wasn't found");
				}

				//note: varargs pawn args are passed by-ref
				cell_t *cell;
				pContext->LocalToPhysAddr(params[startparam], &cell);
				void *thisptr = reinterpret_cast<void*>(*cell);

				if (thisptr == nullptr)
				{
					vc->stk_put(ptr);
					return pContext->ThrowNativeError("ThisPtr address cannot be null");
				}
				else if (reinterpret_cast<uintptr_t>(thisptr) < VALID_MINIMUM_MEMORY_ADDRESS)
				{
					vc->stk_put(ptr);
					return pContext->ThrowNativeError("Invalid ThisPtr address 0x%x is pointing to reserved memory.", thisptr);
				}

				*(void **)ptr = thisptr;
				startparam++;
			}
			break;
		default:
			{
				vc->stk_put(ptr);
				return pContext->ThrowNativeError("Unrecognized SDK Call type (%d)", vc->type);
			}
		}
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
			vc,
			&(vc->vparams[i]),
			ptr) == Data_Fail)
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
					params[startparam + i], 
					vc,
					&vc->vparams[i],
					ptr) == Data_Fail)
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
			if (!(*(char **)vc->retbuf))
			{
				pContext->StringToLocalUTF8(params[retparam], *addr, "", &written);
				return -1;
			}
			pContext->StringToLocalUTF8(params[retparam], *addr, *(char **)vc->retbuf, &written);
			return (cell_t)written;
		} else if (vc->retinfo->vtype == Valve_Vector
					|| vc->retinfo->vtype == Valve_QAngle)
		{
			if (numparams < 2)
			{
				return pContext->ThrowNativeError("Expected argument (2) for Float[3] storage");
			}
			if (EncodeValveParam(pContext, params[retparam], vc, vc->retinfo, vc->retbuf)
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
			return IndexOfEdict(pEdict);
		} else if (vc->retinfo->vtype == Valve_Edict) {
			edict_t *pEdict = *(edict_t **)(vc->retbuf);
			if (!pEdict || pEdict->IsFree())
			{
				return -1;
			}
			return IndexOfEdict(pEdict);
		} else if (vc->retinfo->vtype == Valve_Bool) {
			bool *addr = (bool  *)vc->retbuf;
			if (vc->retinfo->flags & PASSFLAG_ASPOINTER)
			{
				addr = *(bool **)addr;
			}
			return *addr ? 1 : 0;
		} else {
			cell_t *addr = (cell_t *)vc->retbuf;
			if (vc->retinfo->flags & PASSFLAG_ASPOINTER)
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
	{"PrepSDKCall_SetAddress",		PrepSDKCall_SetAddress},
	{"PrepSDKCall_SetFromConf",		PrepSDKCall_SetFromConf},
	{"PrepSDKCall_SetReturnInfo",	PrepSDKCall_SetReturnInfo},
	{"PrepSDKCall_AddParameter",	PrepSDKCall_AddParameter},
	{"EndPrepSDKCall",				EndPrepSDKCall},
	{"SDKCall",						SDKCall},
	{NULL,							NULL},
};
