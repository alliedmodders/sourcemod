#include "extension.h"
#include "timeleft.h"

TimeLeftEvents g_TimeLeftEvents;
bool get_new_timeleft_offset = false;
bool round_end_found = false;

bool TimeLeftEvents::LevelInit(char const *pMapName,
							   char const *pMapEntities,
							   char const *pOldLevel,
							   char const *pLandmarkName,
							   bool loadGame,
							   bool background)
{
	round_end_found	= true;
	get_new_timeleft_offset = false;

	return true;
}

void TimeLeftEvents::FireGameEvent(IGameEvent *event)
{
	const char *name = event->GetName();
	
	if (strcmp(name, "round_start") == 0)
	{
		if (get_new_timeleft_offset || !round_end_found)
		{
			get_new_timeleft_offset = false;
			timersys->NotifyOfGameStart();
			timersys->MapTimeLeftChanged();
		}
		round_end_found = false;
	}
	else if (strcmp(name, "round_end") == 0)
	{
		if (event->GetInt("reason") == 16)
		{
			get_new_timeleft_offset = true;
		}
		round_end_found = true;
	}
}
