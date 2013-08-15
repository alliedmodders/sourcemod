#ifndef _INCLUDE_CSTRIKE_FORWARDS_H_
#define _INCLUDE_CSTRIKE_FORWARDS_H_
bool CreateWeaponPriceDetour();
bool CreateHandleBuyDetour();
bool CreateTerminateRoundDetour();
bool CreateCSWeaponDropDetour();
void RemoveWeaponPriceDetour();
void RemoveHandleBuyDetour();
void RemoveTerminateRoundDetour();
void RemoveCSWeaponDropDetour();	

extern IServerGameEnts *gameents;
extern IForward *g_pHandleBuyForward;
extern IForward *g_pPriceForward;
extern IForward *g_pTerminateRoundForward;
extern IForward *g_pCSWeaponDropForward;
#endif //_INCLUDE_CSTRIKE_FORWARDS_H_
