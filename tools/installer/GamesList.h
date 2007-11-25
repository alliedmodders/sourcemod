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

struct valve_game_t
{
	const TCHAR *folder;
	const TCHAR *subfolder;
	int eng_type;
};

/* One game */
struct game_info_t
{
	TCHAR name[128];
	TCHAR game_path[MAX_PATH];
	int source_engine;
};

/* A list of games under one "account" */
struct game_list_t
{
	TCHAR root_name[128];
	unsigned int *games;
	unsigned int game_count;
};

/* A list of accounts */
struct game_group_t
{
	game_list_t **lists;
	unsigned int list_count;
	int error_code;
};

/* All games on the system */
struct game_database_t
{
	game_info_t *game_list;
	unsigned int game_count;
	game_group_t dedicated;
	game_group_t listen;
	game_group_t standalone;
};

int IsValidFolder(const TCHAR *path);
void DisplayBadFolderDialog(HWND hWnd, int reason);

void BuildGameDB();
void ReleaseGameDB();
void DisplayBadGamesDialog(HWND hWnd, int reason);

extern game_database_t g_games;

#endif //_INCLUDE_INSTALLER_GAMES_LIST_H_
