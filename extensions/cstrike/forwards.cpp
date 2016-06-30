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

//Windows CS:GO
// int __userpurge HandleCommand_Buy_Internal<eax>(int a1<ecx>, float a2<xmm0>, int a3, int a4, char a5)
// a1 - this
// a2 - CCSGameRules::GetWarmupPeriodEndTime(g_pGameRules)
// a3 - weapon
// a4 - unknown
// a5 - bRebuy
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
	int val = DETOUR_MEMBER_CALL(DetourHandleBuy)(weapon, iUnknown, bRebuy);
#else
	int val = DETOUR_MEMBER_CALL(DetourHandleBuy)(weapon);
#endif
	lastclient = -1;
	return val;
}

#if SOURCE_ENGINE != SE_CSGO
DETOUR_DECL_MEMBER0(DetourWeaponPrice, int)
#elif defined(WIN32)
DETOUR_DECL_MEMBER2(DetourWeaponPrice, int, CEconItemView *, pEconItem, int, iUnknown)
#else
DETOUR_DECL_MEMBER3(DetourWeaponPrice, int, CEconItemView *, pEconItem, int, iUnknown, float, fUnknown)
#endif
{

#if SOURCE_ENGINE != SE_CSGO
	int price = DETOUR_MEMBER_CALL(DetourWeaponPrice)();
#elif defined(WIN32)
	int price = DETOUR_MEMBER_CALL(DetourWeaponPrice)(pEconItem, iUnknown);
#else
	int price = DETOUR_MEMBER_CALL(DetourWeaponPrice)(pEconItem, iUnknown, fUnknown);
#endif
	
	if (lastclient == -1)
		return price;

	const char *weapon_name = reinterpret_cast<char *>(this+weaponNameOffset);

	return CallPriceForward(lastclient, weapon_name, price);
}

#if SOURCE_ENGINE != SE_CSGO || !defined(WIN32)
DETOUR_DECL_MEMBER2(DetourTerminateRound, void, float, delay, int, reason)
{
	if (g_pIgnoreTerminateDetour)
	{
		g_pIgnoreTerminateDetour = false;
		DETOUR_MEMBER_CALL(DetourTerminateRound)(delay, reason);
		return;
	}
#else
//Windows CSGO
//char __userpurge TerminateRound(int a1@<ecx>, float a2@<xmm1>, int *a3)
// a1 - this
// a2 - delay
// a3 - reason
DETOUR_DECL_MEMBER1(DetourTerminateRound, void, int, reason)
{
	float delay;

	if (g_pIgnoreTerminateDetour)
	{
		g_pIgnoreTerminateDetour = false;
		return DETOUR_MEMBER_CALL(DetourTerminateRound)(reason);
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
	
#if SOURCE_ENGINE != SE_CSGO || !defined(WIN32)
	if (result == Pl_Changed)
		return DETOUR_MEMBER_CALL(DetourTerminateRound)(delay, reason);

	return DETOUR_MEMBER_CALL(DetourTerminateRound)(orgdelay, orgreason);
#else
	if (result == Pl_Changed)
	{
		__asm
		{
			movss xmm1, delay
		}
		return DETOUR_MEMBER_CALL(DetourTerminateRound)(reason);
	}
	__asm
	{
		movss xmm1, orgdelay
	}
	return DETOUR_MEMBER_CALL(DetourTerminateRound)(orgreason);
#endif
}

#if SOURCE_ENGINE == SE_CSGO
DETOUR_DECL_MEMBER3(DetourCSWeaponDrop, void, CBaseEntity *, weapon, Vector, vec, bool, unknown)
#else
DETOUR_DECL_MEMBER3(DetourCSWeaponDrop, void, CBaseEntity *, weapon, bool, bDropShield, bool, bThrowForward)
#endif
{
	if (g_pIgnoreCSWeaponDropDetour)
	{
		g_pIgnoreCSWeaponDropDetour = false;
#if SOURCE_ENGINE == SE_CSGO
		DETOUR_MEMBER_CALL(DetourCSWeaponDrop)(weapon, vec, unknown);
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
	g_pCSWeaponDropForward->Execute(&result);


	if (result == Pl_Continue)
	{
#if SOURCE_ENGINE == SE_CSGO
		DETOUR_MEMBER_CALL(DetourCSWeaponDrop)(weapon, vec, unknown);
#else
		DETOUR_MEMBER_CALL(DetourCSWeaponDrop)(weapon, bDropShield, bThrowForward);
#endif
	}

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

#if SOURCE_ENGINE == SE_CSGO && defined(WIN32)
	void *pGetWeaponPriceAddress = GetWeaponPriceFunction();

	if(!pGetWeaponPriceAddress)
	{
		g_pSM->LogError(myself, "GetWeaponPrice detour could not be initialized - Disabled OnGetWeaponPrice forward.");
	}

	DWeaponPrice = DETOUR_CREATE_MEMBER(DetourWeaponPrice, pGetWeaponPriceAddress);
#else
	DWeaponPrice = DETOUR_CREATE_MEMBER(DetourWeaponPrice, "GetWeaponPrice");
#endif
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
