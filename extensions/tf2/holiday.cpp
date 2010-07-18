#include "holiday.h"

CDetour *getHolidayDetour = NULL;

IForward *g_getHolidayForward = NULL;

DETOUR_DECL_MEMBER0(GetHoliday, int)
{
	int actualres = DETOUR_MEMBER_CALL(GetHoliday)();
	if (!g_getHolidayForward)
	{
		g_pSM->LogMessage(myself, "Invalid Forward");
		return actualres;
	}

	cell_t result = 0;
	int newres = actualres;

	g_getHolidayForward->PushCellByRef(&newres);
	g_getHolidayForward->Execute(&result);
	
	if (result == Pl_Changed)
	{
		return newres;
	}

	return actualres;
}

void InitialiseGetHolidayDetour()
{
	getHolidayDetour = DETOUR_CREATE_MEMBER(GetHoliday, "GetHoliday");

	if (!getHolidayDetour)
	{
		g_pSM->LogError(myself, "GetHoliday detour failed");
		return;
	}

	getHolidayDetour->EnableDetour();
}

void RemoveGetHolidayDetour()
{
	getHolidayDetour->Destroy();
}
