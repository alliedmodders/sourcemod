#ifndef _INCLUDE_INSTALLER_GAMES_LIST_H_
#define _INCLUDE_INSTALLER_GAMES_LIST_H_

#include "platform_headers.h"

#define GAMEINFO_IS_USABLE			0
#define GAMEINFO_DOES_NOT_EXIST		1
#define GAMEINFO_IS_READ_ONLY		2

#define GAME_LIST_HALFLIFE1			-2
#define GAME_LIST_CANT_READ			-1
#define GAME_LIST_NO_GAMES			0

#define GAMES_DEDICATED				1
#define GAMES_LISTEN				2
#define GAMES_STANDALONE			3

#define SOURCE_ENGINE_UNKNOWN		0
#define SOURCE_ENGINE_2004			1
#define SOURCE_ENGINE_2007			2

struct mod_info_t
{
	TCHAR name[128];
	TCHAR mod_path[MAX_PATH];
	int source_engine;
};

int IsValidFolder(const TCHAR *path);
void DisplayBadFolderDialog(HWND hWnd, int reason);

int FindGames(unsigned int game_type);
void DisplayBadGamesDialog(HWND hWnd, unsigned int game_type, int reason);
void ReleaseGamesList();

extern mod_info_t *g_mod_list;
extern unsigned int g_mod_count;

#endif //_INCLUDE_INSTALLER_GAMES_LIST_H_
