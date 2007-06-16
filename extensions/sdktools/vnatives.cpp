#include "extension.h"
#include "vcallbuilder.h"
#include "vnatives.h"

List<ValveCall *> g_RegCalls;

inline void InitPass(ValvePassInfo &info, ValveType vtype, PassType type, unsigned int flags)
{
	info.decflags = 0;
	info.encflags = 0;
	info.flags = flags;
	info.type = type;
	info.vtype = vtype;
}

#define START_CALL() \
	unsigned char *vptr = pCall->stk_get();

#define FINISH_CALL_SIMPLE(vret) \
	pCall->call->Execute(vptr, vret); \
	pCall->stk_put(vptr);

#define ENCODE_VALVE_PARAM(num, which, vnum) \
	if (EncodeValveParam(pContext, \
			params[num], \
			&pCall->which[vnum], \
			vptr + pCall->which[vnum].offset) \
		== Data_Fail) \
	{ \
		return 0; \
	}

#define DECODE_VALVE_PARAM(num, which, vnum) \
	if (DecodeValveParam(pContext, \
			params[num], \
			&pCall->which[vnum], \
			vptr + pCall->which[vnum].offset) \
		== Data_Fail) \
	{ \
		return 0; \
	}

bool CreateBaseCall(const char *name,
						  ValveCallType vcalltype,
						  const ValvePassInfo *retinfo,
						  const ValvePassInfo params[],
						  unsigned int numParams,
						  ValveCall **vaddr)
{
	int offset;
	ValveCall *call;
	if (g_pGameConf->GetOffset(name, &offset))
	{
		call = CreateValveVCall(offset, vcalltype, retinfo, params, numParams);
		if (call)
		{
			g_RegCalls.push_back(call);
		}
		*vaddr = call;
		return true;
	} else {
		void *addr;
		if (g_pGameConf->GetMemSig(name, &addr))
		{
			call = CreateValveCall(addr, vcalltype, retinfo, params, numParams);
			if (call)
			{
				g_RegCalls.push_back(call);
			}
			*vaddr = call;
			return true;
		}
	}
	return false;
}


static cell_t RemovePlayerItem(IPluginContext *pContext, const cell_t *params)
{
	static ValveCall *pCall = NULL;
	if (!pCall)
	{
		ValvePassInfo pass[2];
		InitPass(pass[0], Valve_CBaseEntity, PassType_Basic, 0);
		InitPass(pass[1], Valve_Bool, PassType_Basic, 0);
		if (!CreateBaseCall("RemovePlayerItem", ValveCall_Player, &pass[1], &pass[0], 1, &pCall))
		{
			return pContext->ThrowNativeError("\"RemovePlayerItem\" not supported by this mod");
		} else if (!pCall) {
			return pContext->ThrowNativeError("\"RemovePlayerItem\" wrapper failed to initialized");
		}
	}

	bool ret;
	START_CALL();
	DECODE_VALVE_PARAM(1, thisinfo, 0);
	DECODE_VALVE_PARAM(2, vparams, 0);
	FINISH_CALL_SIMPLE(&ret);
	return ret ? 1 : 0;
}

static cell_t GiveNamedItem(IPluginContext *pContext, const cell_t *params)
{
	static ValveCall *pCall = NULL;
	if (!pCall)
	{
		ValvePassInfo pass[3];
		InitPass(pass[0], Valve_String, PassType_Basic, PASSFLAG_BYVAL);
		InitPass(pass[1], Valve_POD, PassType_Basic, PASSFLAG_BYVAL);
		InitPass(pass[2], Valve_CBaseEntity, PassType_Basic, PASSFLAG_BYVAL);
		if (!CreateBaseCall("GiveNamedItem", ValveCall_Player, &pass[2], pass, 2, &pCall))
		{
			return pContext->ThrowNativeError("\"GiveNamedItem\" not supported by this mod");
		} else if (!pCall) {
			return pContext->ThrowNativeError("\"GiveNamedItem\" wrapper failed to initialized");
		}
	}

	CBaseEntity *pEntity = NULL;
	START_CALL();
	DECODE_VALVE_PARAM(1, thisinfo, 0);
	DECODE_VALVE_PARAM(2, vparams, 0);
	DECODE_VALVE_PARAM(3, vparams, 1);
	FINISH_CALL_SIMPLE(&pEntity);

	if (pEntity == NULL)
	{
		return -1;
	}

	edict_t *pEdict = gameents->BaseEntityToEdict(pEntity);
	if (!pEdict)
	{
		return -1;
	}

	return engine->IndexOfEdict(pEdict);
}

static cell_t GetPlayerWeaponSlot(IPluginContext *pContext, const cell_t *params)
{
	static ValveCall *pCall = NULL;
	if (!pCall)
	{
		ValvePassInfo pass[2];
		InitPass(pass[0], Valve_POD, PassType_Basic, PASSFLAG_BYVAL);
		InitPass(pass[1], Valve_CBaseEntity, PassType_Basic, PASSFLAG_BYVAL);
		if (!CreateBaseCall("Weapon_GetSlot", ValveCall_Player, &pass[1], pass, 1, &pCall))
		{
			return pContext->ThrowNativeError("\"Weapon_GetSlot\" not supported by this mod");
		} else if (!pCall) {
			return pContext->ThrowNativeError("\"Weapon_GetSlot\" wrapper failed to initialized");
		}
	}

	CBaseEntity *pEntity;
	START_CALL();
	DECODE_VALVE_PARAM(1, thisinfo, 0);
	DECODE_VALVE_PARAM(2, vparams, 0);
	FINISH_CALL_SIMPLE(&pEntity);

	if (pEntity == NULL)
	{
		return -1;
	}

	edict_t *pEdict = gameents->BaseEntityToEdict(pEntity);
	if (!pEdict)
	{
		return -1;
	}

	return engine->IndexOfEdict(pEdict);
}

sp_nativeinfo_t g_Natives[] = 
{
	{"RemovePlayerItem",	RemovePlayerItem},
	{"GivePlayerItem",		GiveNamedItem},
	{"GetPlayerWeaponSlot",	GetPlayerWeaponSlot},
	{NULL,					NULL},
};
