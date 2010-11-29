#include "holiday.h"

CDetour *getHolidayDetour = NULL;

IForward *g_getHolidayForward = NULL;

DETOUR_DECL_STATIC0(GetHoliday, int)
{
	int actualres = DETOUR_STATIC_CALL(GetHoliday)();
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

bool InitialiseGetHolidayDetour()
{
	getHolidayDetour = DETOUR_CREATE_STATIC(GetHoliday, "GetHoliday");

	if (getHolidayDetour != NULL)
	{
		getHolidayDetour->EnableDetour();
		return true;
	}

	g_pSM->LogError(myself, "GetHoliday detour failed");
	return false;
}

void RemoveGetHolidayDetour()
{
	getHolidayDetour->Destroy();
}
