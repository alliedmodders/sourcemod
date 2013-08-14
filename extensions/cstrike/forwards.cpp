#include "extension.h"
#include "forwards.h"
#include "util_cstrike.h"

bool g_pTerminateRoundDetoured = false;
bool g_pCSWeaponDropDetoured = false;
bool g_pIgnoreTerminateDetour = false;
bool g_pIgnoreCSWeaponDropDetour = false;
bool g_PriceDetoured = false;
bool g_HandleBuyDetoured = false;
int lastclient = -1;

IForward *g_pHandleBuyForward = NULL;
IForward *g_pPriceForward = NULL;
IForward *g_pTerminateRoundForward = NULL;
IForward *g_pCSWeaponDropForward = NULL;
CDetour *DHandleBuy = NULL;
CDetour *DWeaponPrice = NULL;
CDetour *DTerminateRound = NULL;
CDetour *DCSWeaponDrop = NULL;

int weaponNameOffset = -1;

#if SOURCE_ENGINE == SE_CSGO
DETOUR_DECL_MEMBER3(DetourHandleBuy, int, const char *, weapon, int, iUnknown, bool, bRebuy)
#else
DETOUR_DECL_MEMBER1(DetourHandleBuy, int, const char *, weapon)
#endif
{
	int client = gamehelpers->EntityToBCompatRef(reinterpret_cast<CBaseEntity *>(this));

	lastclient = client;

	cell_t result = Pl_Continue;

	g_pHandleBuyForward->PushCell(client);
	g_pHandleBuyForward->PushString(weapon);
	g_pHandleBuyForward->Execute(&result);

	if (result != Pl_Continue)
	{
		lastclient = -1;
		return 0;
	}

#if SOURCE_ENGINE == SE_CSGO
	int defaultprice = -1;
	if (g_iPriceOffset != -1 && g_PriceDetoured)
	{
		defaultprice = CallPriceForwardCSGO(client, weapon);
	}

	int val = DETOUR_MEMBER_CALL(DetourHandleBuy)(weapon, iUnknown, bRebuy);
#else
	int val = DETOUR_MEMBER_CALL(DetourHandleBuy)(weapon);
#endif


#if SOURCE_ENGINE == SE_CSGO
	if (defaultprice != -1)
		SetWeaponPrice(weapon, defaultprice);
#endif

	lastclient = -1;
	return val;
}

#if SOURCE_ENGINE != SE_CSGO
DETOUR_DECL_MEMBER0(DetourWeaponPrice, int)
{
	int price = DETOUR_MEMBER_CALL(DetourWeaponPrice)();

	if (lastclient == -1)
		return price;

	const char *weapon_name = reinterpret_cast<char *>(this+weaponNameOffset);

	return CallPriceForward(lastclient, weapon_name, price);
}
#endif

DETOUR_DECL_MEMBER2(DetourTerminateRound, void, float, delay, int, reason)
{
	if (g_pIgnoreTerminateDetour)
	{
		g_pIgnoreTerminateDetour = false;
		DETOUR_MEMBER_CALL(DetourTerminateRound)(delay, reason);
		return;
	}
	float orgdelay = delay;
	int orgreason = reason;

	cell_t result = Pl_Continue;

	g_pTerminateRoundForward->PushFloatByRef(&delay);
	g_pTerminateRoundForward->PushCellByRef(&reason);
	g_pTerminateRoundForward->Execute(&result);

	if (result >= Pl_Handled)
		return;

	if (result == Pl_Changed)
		return DETOUR_MEMBER_CALL(DetourTerminateRound)(delay, reason);

	return DETOUR_MEMBER_CALL(DetourTerminateRound)(orgdelay, orgreason);
}

DETOUR_DECL_MEMBER3(DetourCSWeaponDrop, void, CBaseEntity *, weapon, bool, something, bool, toss)
{
	if (g_pIgnoreCSWeaponDropDetour)
	{
		g_pIgnoreCSWeaponDropDetour = false;
		DETOUR_MEMBER_CALL(DetourCSWeaponDrop)(weapon, something, toss);
		return;
	}

	int client = gamehelpers->EntityToBCompatRef(reinterpret_cast<CBaseEntity *>(this));
	int weaponIndex = gamehelpers->EntityToBCompatRef(weapon);

	cell_t result = Pl_Continue;
	g_pCSWeaponDropForward->PushCell(client);
	g_pCSWeaponDropForward->PushCell(weaponIndex);
	g_pCSWeaponDropForward->Execute(&result);


	if (result == Pl_Continue)
		DETOUR_MEMBER_CALL(DetourCSWeaponDrop)(weapon, something, toss);

	return;
}

bool CreateWeaponPriceDetour()
{
	if (weaponNameOffset == -1)
	{
		if (!g_pGameConf->GetOffset("WeaponName", &weaponNameOffset))
		{
			smutils->LogError(myself, "Could not find WeaponName offset - Disabled OnGetWeaponPrice forward");
			return false;
		}
	}

#if SOURCE_ENGINE == SE_CSGO
	if (g_iPriceOffset == -1)
	{
		if (!g_pGameConf->GetOffset("WeaponPrice", &g_iPriceOffset))
		{
			g_iPriceOffset = -1;
			g_pSM->LogError(myself, "Failed to get WeaponPrice offset - Disabled OnGetWeaponPrice forward");
			return false;
		}
	}
	if (!CreateHandleBuyDetour())
	{
		g_pSM->LogError(myself, "HandleCommand_Buy_Internal failed to detour, disabled OnGetWeaponPrice forward.");
		return false;
	}
	else
	{
		g_PriceDetoured = true;
		return true;
	}
#else

	DWeaponPrice = DETOUR_CREATE_MEMBER(DetourWeaponPrice, "GetWeaponPrice");

	if (DWeaponPrice != NULL)
	{
		if (!CreateHandleBuyDetour())
		{
			g_pSM->LogError(myself, "GetWeaponPrice detour could not be initialized - HandleCommand_Buy_Internal failed to detour, disabled OnGetWeaponPrice forward.");
			return false;
		}
		DWeaponPrice->EnableDetour();
		g_PriceDetoured = true;
		return true;
	}

	g_pSM->LogError(myself, "GetWeaponPrice detour could not be initialized - Disabled OnGetWeaponPrice forward.");

	return false;
#endif
}

bool CreateTerminateRoundDetour()
{
	DTerminateRound = DETOUR_CREATE_MEMBER(DetourTerminateRound, "TerminateRound");

	if (DTerminateRound != NULL)
	{
		DTerminateRound->EnableDetour();
		g_pTerminateRoundDetoured = true;
		return true;
	}
	g_pSM->LogError(myself, "TerminateRound detour could not be initialized - Disabled OnTerminateRound forward");
	return false;
}

bool CreateHandleBuyDetour()
{
	if (g_HandleBuyDetoured)
		return true;

	DHandleBuy = DETOUR_CREATE_MEMBER(DetourHandleBuy, "HandleCommand_Buy_Internal");

	if (DHandleBuy != NULL)
	{
		DHandleBuy->EnableDetour();
		g_HandleBuyDetoured = true;
		return true;
	}
	g_pSM->LogError(myself, "HandleCommand_Buy_Internal detour could not be initialized - Disabled OnBuyCommand forward");
	return false;
}

bool CreateCSWeaponDropDetour()
{
	DCSWeaponDrop = DETOUR_CREATE_MEMBER(DetourCSWeaponDrop, "CSWeaponDrop");

	if (DCSWeaponDrop != NULL)
	{
		DCSWeaponDrop->EnableDetour();
		g_pCSWeaponDropDetoured = true;
		return true;
	}

	g_pSM->LogError(myself, "CSWeaponDrop detour could not be initialized - Disabled OnCSWeaponDrop forward");
	return false;
}

void RemoveWeaponPriceDetour()	
{
#if SOURCE_ENGINE != SE_CSGO
	if (DWeaponPrice != NULL)
	{
		DWeaponPrice->Destroy();
		DWeaponPrice = NULL;
	}
	g_PriceDetoured = false;
#endif
}

void RemoveHandleBuyDetour()	
{
	if (g_PriceDetoured)
		return;

	if (DHandleBuy != NULL)
	{
		DHandleBuy->Destroy();
		DHandleBuy = NULL;
	}
	g_HandleBuyDetoured = false;
}

void RemoveTerminateRoundDetour()	
{
	if (DTerminateRound != NULL)
	{
		DTerminateRound->Destroy();
		DTerminateRound = NULL;
	}
	g_pTerminateRoundDetoured = false;
}

void RemoveCSWeaponDropDetour()	
{
	if (DCSWeaponDrop != NULL)
	{
		DCSWeaponDrop->Destroy();
		DCSWeaponDrop = NULL;
	}
	g_pCSWeaponDropDetoured = false;
}

#if SOURCE_ENGINE == SE_CSGO
bool SetWeaponPrice(int weaponID, int price)
{
	void *info = GetWeaponInfo(weaponID);
	if (!info)
	{
		return false;
	}
	*(int *)((intptr_t)info+g_iPriceOffset) = price;
	return true;
}

bool SetWeaponPrice(const char *weapon, int price)
{
	const char *weaponalias = GetTranslatedWeaponAlias(weapon);
	int weaponID = AliasToWeaponID(weaponalias);
	if (weaponID <= 0)
	{
		return false;
	}
	void *info = GetWeaponInfo(weaponID);
	if (!info)
	{
		return false;
	}
	*(int *)((intptr_t)info+g_iPriceOffset) = price;
	return true;

}

int CallPriceForwardCSGO(int client, const char *weapon, int price)
{
	int changedprice = price;
	cell_t result = Pl_Continue;

	g_pPriceForward->PushCell(client);
	g_pPriceForward->PushString(weapon);
	g_pPriceForward->PushCellByRef(&changedprice);
	g_pPriceForward->Execute(&result);
	
	if (result == Pl_Continue)
		return price;

	return changedprice;
}

int CallPriceForwardCSGO(int client, const char *weapon)
{
	const char *weaponalias = GetTranslatedWeaponAlias(weapon);
	int weaponID = AliasToWeaponID(weaponalias);
	if (weaponID <= 0)
	{
		return -1;
	}
	void *info = GetWeaponInfo(weaponID);
	if (!info)
	{
		return -1;
	}
	const char *weapon_name = (const char *)((intptr_t)info + weaponNameOffset);
	int price = *(int *)((intptr_t)info + g_iPriceOffset);
	int changedprice = price;
	cell_t result = Pl_Continue;

	g_pPriceForward->PushCell(client);
	g_pPriceForward->PushString(weapon_name);
	g_pPriceForward->PushCellByRef(&changedprice);
	g_pPriceForward->Execute(&result);
		
	if (result == Pl_Continue)
		return -1;
	if (SetWeaponPrice(weaponID, changedprice))
		return price;
	else
		return -1;
}
#else

int CallPriceForward(int client, const char *weapon_name, int price)
{
	int changedprice = price;

	cell_t result = Pl_Continue;

	g_pPriceForward->PushCell(client);
	g_pPriceForward->PushString(weapon_name);
	g_pPriceForward->PushCellByRef(&changedprice);
	g_pPriceForward->Execute(&result);
	
	if (result == Pl_Continue)
		return price;

	return changedprice;
}
#endif
