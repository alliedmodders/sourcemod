#include "extension.h"
#include "forwards.h"

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

DETOUR_DECL_MEMBER1(DetourHandleBuy, int, const char *, weapon)
{
	int client = engine->IndexOfEdict(engine->PEntityOfEntIndex(gamehelpers->EntityToBCompatRef(reinterpret_cast<CBaseEntity *>(this))));

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

DETOUR_DECL_MEMBER0(DetourWeaponPrice, int)
{
	int price = DETOUR_MEMBER_CALL(DetourWeaponPrice)();

	if (lastclient == -1)
		return price;

	const char *weapon_name = reinterpret_cast<char *>(this+weaponNameOffset);

	int original = price;

	cell_t result = Pl_Continue;

	g_pPriceForward->PushCell(lastclient);
	g_pPriceForward->PushString(weapon_name);
	g_pPriceForward->PushCellByRef(&price);
	g_pPriceForward->Execute(&result);
	
	if (result == Pl_Continue)
		return original;

	return price;
}

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

	int client = engine->IndexOfEdict(engine->PEntityOfEntIndex(gamehelpers->EntityToBCompatRef(reinterpret_cast<CBaseEntity *>(this))));
	int weaponIndex = engine->IndexOfEdict(engine->PEntityOfEntIndex(gamehelpers->EntityToBCompatRef(weapon)));

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
