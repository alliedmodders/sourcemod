#include "extension.h"
#include "forwards.h"
#include "util_cstrike.h"

bool g_pTerminateRoundDetoured = false;
bool g_pCSWeaponDropDetoured = false;
bool g_pIgnoreTerminateDetour = false;
bool g_pIgnoreCSWeaponDropDetour = false;
bool g_PriceDetoured = false;
bool g_HandleBuyDetoured = false;
#if SOURCE_ENGINE != SE_CSGO
int lastclient = -1;
#endif

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
DETOUR_DECL_MEMBER4(DetourHandleBuy, int, int, iLoadoutSlot, void *, pWpnDataRef, bool, bRebuy, bool, bDrop)
{
	CBaseEntity *pEntity = reinterpret_cast<CBaseEntity *>(this);
	int client = gamehelpers->EntityToBCompatRef(pEntity);

	CEconItemView *pView = GetEconItemView(pEntity, iLoadoutSlot);

	if (!pView)
	{
		return DETOUR_MEMBER_CALL(DetourHandleBuy)(iLoadoutSlot, pWpnDataRef, bRebuy, bDrop);
	}

	CCSWeaponData *pWpnData = GetCCSWeaponData(pView);

	if (!pWpnData)
	{
		return DETOUR_MEMBER_CALL(DetourHandleBuy)(iLoadoutSlot, pWpnDataRef, bRebuy, bDrop);
	}

	const char *szClassname = *(const char **)((intptr_t)pWpnData + weaponNameOffset);

	char weaponName[128];

	if (strstr(szClassname, "knife"))
	{
		Q_strncpy(weaponName, "knife", sizeof(weaponName));
	}
	else
	{
		Q_strncpy(weaponName, GetWeaponNameFromClassname(szClassname), sizeof(weaponName));
	}

	cell_t result = Pl_Continue;

	g_pHandleBuyForward->PushCell(client);
	g_pHandleBuyForward->PushString(weaponName);
	g_pHandleBuyForward->Execute(&result);

	if (result != Pl_Continue)
	{
		return 0;
	}
	
	int originalPrice = 0;

	if (g_iPriceOffset != -1)
	{
		originalPrice = *(int *)((intptr_t)pWpnData + g_iPriceOffset);

		int changedPrice = CallPriceForward(client, weaponName, originalPrice);

		if (originalPrice != changedPrice)
			*(int *)((intptr_t)pWpnData + g_iPriceOffset) = changedPrice;
	}

	int ret = DETOUR_MEMBER_CALL(DetourHandleBuy)(iLoadoutSlot, pWpnDataRef, bRebuy, bDrop);

	if (g_iPriceOffset != -1)
		*(int *)((intptr_t)pWpnData + g_iPriceOffset) = originalPrice;

	return ret;
}
#else
DETOUR_DECL_MEMBER1(DetourHandleBuy, int, const char *, weapon)
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

	int val = DETOUR_MEMBER_CALL(DetourHandleBuy)(weapon);

	lastclient = -1;
	return val;
}
#endif

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

#if SOURCE_ENGINE == SE_CSS
DETOUR_DECL_MEMBER2(DetourTerminateRound, void, float, delay, int, reason)
{
	if (g_pIgnoreTerminateDetour)
	{
		g_pIgnoreTerminateDetour = false;
		DETOUR_MEMBER_CALL(DetourTerminateRound)(delay, reason);
		return;
	}
#elif SOURCE_ENGINE == SE_CSGO && !defined(WIN32)
DETOUR_DECL_MEMBER4(DetourTerminateRound, void, float, delay, int, reason, int, unknown, int, unknown2)
{
	if (g_pIgnoreTerminateDetour)
	{
		g_pIgnoreTerminateDetour = false;
		DETOUR_MEMBER_CALL(DetourTerminateRound)(delay, reason, unknown, unknown2);
		return;
	}
#else
//Windows CSGO
//char __userpurge TerminateRound(int a1@<ecx>, float a2@<xmm1>, int *a3)
// a1 - this
// a2 - delay
// a3 - reason
// a4 - unknown
// a5 - unknown
DETOUR_DECL_MEMBER3(DetourTerminateRound, void, int, reason, int, unknown, int, unknown2)
{
	float delay;

	if (g_pIgnoreTerminateDetour)
	{
		g_pIgnoreTerminateDetour = false;
		return DETOUR_MEMBER_CALL(DetourTerminateRound)(reason, unknown, unknown2);
	}

	//Save the delay
	__asm
	{
		movss delay, xmm1
	}
#endif

	float orgdelay = delay;
	int orgreason = reason;

	cell_t result = Pl_Continue;

#if SOURCE_ENGINE == SE_CSGO
	reason--;
#endif
	
	g_pTerminateRoundForward->PushFloatByRef(&delay);
	g_pTerminateRoundForward->PushCellByRef(&reason);
	g_pTerminateRoundForward->Execute(&result);

	if (result >= Pl_Handled)
		return;

#if SOURCE_ENGINE == SE_CSGO
	reason++;
#endif
	
#if SOURCE_ENGINE == SE_CSS
	if (result == Pl_Changed)
		return DETOUR_MEMBER_CALL(DetourTerminateRound)(delay, reason);

	return DETOUR_MEMBER_CALL(DetourTerminateRound)(orgdelay, orgreason);
#elif SOURCE_ENGINE == SE_CSGO && !defined(WIN32)
	if (result == Pl_Changed)
		return DETOUR_MEMBER_CALL(DetourTerminateRound)(delay, reason, unknown, unknown2);

	return DETOUR_MEMBER_CALL(DetourTerminateRound)(orgdelay, orgreason, unknown, unknown2);
#else
	if (result == Pl_Changed)
	{
		__asm
		{
			movss xmm1, delay
		}
		return DETOUR_MEMBER_CALL(DetourTerminateRound)(reason, unknown, unknown2);
	}
	__asm
	{
		movss xmm1, orgdelay
	}
	return DETOUR_MEMBER_CALL(DetourTerminateRound)(orgreason, unknown, unknown2);
#endif
}

#if SOURCE_ENGINE == SE_CSGO
DETOUR_DECL_MEMBER3(DetourCSWeaponDrop, void, CBaseEntity *, weapon, bool, bThrowForward, bool, bDonated)
#else
DETOUR_DECL_MEMBER3(DetourCSWeaponDrop, void, CBaseEntity *, weapon, bool, bDropShield, bool, bThrowForward)
#endif
{
	if (g_pIgnoreCSWeaponDropDetour)
	{
		g_pIgnoreCSWeaponDropDetour = false;
#if SOURCE_ENGINE == SE_CSGO
		DETOUR_MEMBER_CALL(DetourCSWeaponDrop)(weapon, bThrowForward, bDonated);
#else
		DETOUR_MEMBER_CALL(DetourCSWeaponDrop)(weapon, bDropShield, bThrowForward);
#endif
		return;
	}

	int client = gamehelpers->EntityToBCompatRef(reinterpret_cast<CBaseEntity *>(this));
	int weaponIndex = gamehelpers->EntityToBCompatRef(weapon);

	cell_t result = Pl_Continue;
	g_pCSWeaponDropForward->PushCell(client);
	g_pCSWeaponDropForward->PushCell(weaponIndex);
#if SOURCE_ENGINE == SE_CSGO
	g_pCSWeaponDropForward->PushCell(bDonated);
#else
	g_pCSWeaponDropForward->PushCell(false);
#endif
	g_pCSWeaponDropForward->Execute(&result);


	if (result == Pl_Continue)
	{
#if SOURCE_ENGINE == SE_CSGO
		DETOUR_MEMBER_CALL(DetourCSWeaponDrop)(weapon, bThrowForward, bDonated);
#else
		DETOUR_MEMBER_CALL(DetourCSWeaponDrop)(weapon, bDropShield, bThrowForward);
#endif
	}

	return;
}

bool CreateWeaponPriceDetour()
{
#if SOURCE_ENGINE != SE_CSGO
	if (weaponNameOffset == -1)
	{
		if (!g_pGameConf->GetOffset("WeaponName", &weaponNameOffset))
		{
			smutils->LogError(myself, "Could not find WeaponName offset - Disabled OnGetWeaponPrice forward");
			return false;
		}
	}

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
#else
	if (g_iPriceOffset == -1)
	{
		if (!g_pGameConf->GetOffset("WeaponPrice", &g_iPriceOffset))
		{
			smutils->LogError(myself, "Could not find WeaponPrice offset - Disabled OnGetWeaponPrice forward");
			return false;
		}
	}

	if (!CreateHandleBuyDetour())
	{
		g_pSM->LogError(myself, "GetWeaponPrice detour could not be initialized - HandleCommand_Buy_Internal failed to detour, disabled OnGetWeaponPrice forward.");
		return false;
	}
	else
	{
		g_PriceDetoured = true;
		return true;
	}
#endif
	g_pSM->LogError(myself, "GetWeaponPrice detour could not be initialized - Disabled OnGetWeaponPrice forward.");

	return false;
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

#if SOURCE_ENGINE == SE_CSGO
	if (weaponNameOffset == -1)
	{
		if (!g_pGameConf->GetOffset("WeaponName", &weaponNameOffset))
		{
			smutils->LogError(myself, "Could not find WeaponName offset - Disabled OnBuyCommand forward");
			return false;
		}
	}
#endif
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
	DCSWeaponDrop = DETOUR_CREATE_MEMBER(DetourCSWeaponDrop, WEAPONDROP_GAMEDATA_NAME);

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
	if (DWeaponPrice != NULL)
	{
		DWeaponPrice->Destroy();
		DWeaponPrice = NULL;
	}
	g_PriceDetoured = false;
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
