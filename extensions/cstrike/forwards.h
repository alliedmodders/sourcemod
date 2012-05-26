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

#if SOURCE_ENGINE == SE_CSGO
bool SetWeaponPrice(int weaponID, int price);
bool SetWeaponPrice(const char *weapon, int price);
int CallPriceForwardCSGO(int client, const char *weapon);
int CallPriceForwardCSGO(int client, const char *weapon, int price);
#endif

extern IServerGameEnts *gameents;
extern IForward *g_pHandleBuyForward;
extern IForward *g_pPriceForward;
extern IForward *g_pTerminateRoundForward;
extern IForward *g_pCSWeaponDropForward;
extern int g_iPriceOffset;
#endif //_INCLUDE_CSTRIKE_FORWARDS_H_
