#ifndef _INCLUDE_SOURCEMOD_CSTRIKE_EVENTS_H_
#define _INCLUDE_SOURCEMOD_CSTRIKE_EVENTS_H_

#include <edict.h>
#include <igameevents.h>

class TimeLeftEvents : public IGameEventListener2
{
public:
	bool LevelInit(char const *pMapName, char const *pMapEntities, char const *pOldLevel, char const *pLandmarkName, bool loadGame, bool background);
	virtual void FireGameEvent(IGameEvent *event);
};

extern TimeLeftEvents g_TimeLeftEvents;

#endif //_INCLUDE_SOURCEMOD_CSTRIKE_EVENTS_H_

