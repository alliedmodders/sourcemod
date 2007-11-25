#include "GamesList.h"
#include "InstallerUtil.h"
#include "InstallerMain.h"
#include <stdio.h>

game_database_t g_games = 
{
	NULL, 0, 
	{NULL, 0, GAME_LIST_NO_GAMES}, 
	{NULL, 0, GAME_LIST_NO_GAMES}, 
	{NULL, 0, GAME_LIST_NO_GAMES}
};

valve_game_t valve_game_list[] = 
{
	{_T("counter-strike source"),		_T("cstrike"),	SOURCE_ENGINE_2004},
	{_T("day of defeat source"),		_T("dod"),		SOURCE_ENGINE_2004},
	{_T("half-life 2 deathmatch"),		_T("hl2mp"),	SOURCE_ENGINE_2004},
	{_T("half-life deathmatch source"),	_T("hl1mp"),	SOURCE_ENGINE_2004},
	{_T("team fortress 2"),				_T("tf"),		SOURCE_ENGINE_2007},
	{NULL,								NULL,			0},
};

valve_game_t valve_server_list[] = 
{
	{_T("source dedicated server"),		NULL,			SOURCE_ENGINE_2004},
	{_T("source 2007 dedicated server"), NULL,			SOURCE_ENGINE_2007},
	{NULL,								NULL,			0},
};

int IsValidFolder(const TCHAR *path)
{
	DWORD attr;
	TCHAR gameinfo_file[MAX_PATH];

	UTIL_PathFormat(gameinfo_file, sizeof(gameinfo_file), _T("%s\\gameinfo.txt"), path);

	if ((attr = GetFileAttributes(gameinfo_file)) == INVALID_FILE_ATTRIBUTES)
	{
		return GAMEINFO_DOES_NOT_EXIST;
	}

	if ((attr & FILE_ATTRIBUTE_READONLY) == FILE_ATTRIBUTE_READONLY)
	{
		return GAMEINFO_IS_READ_ONLY;
	}

	if ((attr & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
	{
		return GAMEINFO_DOES_NOT_EXIST;
	}

	return GAMEINFO_IS_USABLE;
}

void DisplayBadFolderDialog(HWND hDlg, int reason)
{
	TCHAR message_string[255];
	UINT resource;

	if (reason == GAMEINFO_DOES_NOT_EXIST)
	{
		resource = IDS_NO_GAMEINFO;
	}
	else if (reason == GAMEINFO_IS_READ_ONLY)
	{
		resource = IDS_READONLY_GAMEINFO;
	}
	else
	{
		return;
	}

	if (LoadString(g_hInstance,
		resource,
		message_string,
		sizeof(message_string) / sizeof(TCHAR)
		) == 0)
	{
		return;
	}

	MessageBox(hDlg, 
		message_string, 
		_T("SourceMod Installer"),
		MB_OK|MB_ICONWARNING);
}

game_list_t *MakeGameList(const TCHAR *name)
{
	game_list_t *gl = (game_list_t *)malloc(sizeof(game_list_t));

	UTIL_Format(gl->root_name, 
		sizeof(gl->root_name) / sizeof(TCHAR),
		_T("%s"),
		name);
	gl->game_count = 0;
	gl->games = NULL;

	return gl;
}

void AttachGameListToGroup(game_group_t *group, game_list_t *gl)
{
	if (group->lists == NULL)
	{
		group->lists = (game_list_t **)malloc(sizeof(game_list_t *));
	}
	else
	{
		group->lists = (game_list_t **)realloc(group->lists,
			sizeof(game_list_t *) * (group->list_count + 1));
	}

	group->lists[group->list_count] = gl;
	group->list_count++;
}

void AttachModToGameList(game_list_t *gl, unsigned int mod_id)
{
	if (gl->games == NULL)
	{
		gl->games = (unsigned int *)malloc(sizeof(unsigned int));
	}
	else
	{
		gl->games = (unsigned int *)realloc(gl->games, 
			sizeof(unsigned int) * (gl->game_count + 1));
	}

	gl->games[gl->game_count] = mod_id;
	gl->game_count++;
}

unsigned int AddModToList(game_database_t *db, const game_info_t *mod_info)
{
	/* Check if a matching game already exists */
	for (unsigned int i = 0; i < db->game_count; i++)
	{
		if (tstrcasecmp(mod_info->game_path, db->game_list[i].game_path) == 0)
		{
			return i;
		}
	}

	if (db->game_list == NULL)
	{
		db->game_list = (game_info_t *)malloc(sizeof(game_info_t));
	}
	else
	{
		db->game_list = (game_info_t *)realloc(db->game_list, 
			sizeof(game_info_t) * (db->game_count + 1));
	}

	memcpy(&db->game_list[db->game_count], mod_info, sizeof(game_info_t));
	db->game_count++;

	return db->game_count - 1;
}

bool TryToAddMod(const TCHAR *path, int eng_type, game_database_t *db, unsigned int *id)
{
	FILE *fp;
	TCHAR gameinfo_path[MAX_PATH];

	UTIL_PathFormat(gameinfo_path,
		sizeof(gameinfo_path),
		_T("%s\\gameinfo.txt"),
		path);

	if ((fp = _tfopen(gameinfo_path, _T("rt"))) == NULL)
	{
		return false;
	}

	int pos;
	char buffer[512];
	char key[256], value[256];
	while (!feof(fp) && fgets(buffer, sizeof(buffer), fp) != NULL)
	{
		if ((pos = BreakStringA(buffer, key, sizeof(key))) == -1)
		{
			continue;
		}
		if ((pos = BreakStringA(&buffer[pos], value, sizeof(value))) == -1)
		{
			continue;
		}
		if (strcmp(key, "game") == 0)
		{
			game_info_t mod;
			unsigned int got_id;

			AnsiToUnicode(value, mod.name, sizeof(mod.name));
			UTIL_Format(mod.game_path, sizeof(mod.game_path), _T("%s"), path);
			mod.source_engine = eng_type;

			got_id = AddModToList(db, &mod);

			if (id != NULL)
			{
				*id = got_id;
			}

			fclose(fp);

			return true;
		}
	}

	fclose(fp);

	return false;
}

void AddModsFromFolder(const TCHAR *path,
					   int eng_type,
					   game_database_t *db,
					   game_list_t *gl)
{
	HANDLE hFind;
	WIN32_FIND_DATA fd;
	TCHAR temp_path[MAX_PATH];
	TCHAR search_path[MAX_PATH];
	unsigned int mod_id;

	UTIL_Format(search_path,
		sizeof(search_path),
		_T("%s\\*.*"),
		path);

	if ((hFind = FindFirstFile(search_path, &fd)) == INVALID_HANDLE_VALUE)
	{
		return;
	}

	do 
	{
		if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY)
		{
			continue;
		}

		if (tstrcasecmp(fd.cFileName, _T(".")) == 0
			|| tstrcasecmp(fd.cFileName, _T("..")) == 0)
		{
			continue;
		}

		UTIL_PathFormat(temp_path,
			sizeof(temp_path),
			_T("%s\\%s"),
			path,
			fd.cFileName);
		if (TryToAddMod(temp_path, eng_type, db, &mod_id))
		{
			AttachModToGameList(gl, mod_id);
		}
	} while (FindNextFile(hFind, &fd));

	FindClose(hFind);
}

void GetSteamGames(game_database_t *db)
{
	HKEY hkPath;
	DWORD dwLen, dwType;
	HANDLE hFind;
	WIN32_FIND_DATA fd;
	TCHAR temp_path[MAX_PATH];
	TCHAR steam_path[MAX_PATH];
	TCHAR steamapps_path[MAX_PATH];

	if (RegOpenKeyEx(HKEY_CURRENT_USER,
		_T("Software\\Valve\\Steam"),
		0,
		KEY_READ,
		&hkPath) != ERROR_SUCCESS)
	{
		db->listen.error_code = GAME_LIST_CANT_READ;
		db->dedicated.error_code = GAME_LIST_CANT_READ;
		return;
	}

	dwLen = sizeof(steam_path) / sizeof(TCHAR);
	if (RegQueryValueEx(hkPath,
		_T("SteamPath"),
		NULL,
		&dwType,
		(LPBYTE)steam_path,
		&dwLen) != ERROR_SUCCESS)
	{
		RegCloseKey(hkPath);
		db->listen.error_code = GAME_LIST_CANT_READ;
		db->dedicated.error_code = GAME_LIST_CANT_READ;
		return;
	}

	UTIL_PathFormat(steamapps_path,
		sizeof(steamapps_path) / sizeof(TCHAR),
		_T("%s\\steamapps\\*.*"),
		steam_path);

	if ((hFind = FindFirstFile(steamapps_path, &fd)) == INVALID_HANDLE_VALUE)
	{
		RegCloseKey(hkPath);
		db->listen.error_code = GAME_LIST_CANT_READ;
		db->dedicated.error_code = GAME_LIST_CANT_READ;
		return;
	}

	do 
	{
		if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY)
		{
			continue;
		}

		if (tstrcasecmp(fd.cFileName, _T(".")) == 0
			|| tstrcasecmp(fd.cFileName, _T("..")) == 0)
		{
			continue;
		}

		/* If we get a folder called "SourceMods," look for third party mods */
		if (tstrcasecmp(fd.cFileName, _T("SourceMods")) == 0)
		{
			game_list_t *gl = MakeGameList(_T("Third-Party Games"));

			UTIL_PathFormat(temp_path,
				sizeof(temp_path) / sizeof(TCHAR),
				_T("%s\\steamapps\\%s"),
				steam_path,
				fd.cFileName);

			AddModsFromFolder(temp_path, SOURCE_ENGINE_UNKNOWN, db, gl);

			if (gl->game_count)
			{
				AttachGameListToGroup(&db->listen, gl);
			}
			else
			{
				free(gl);
			}
		}
		else
		{
			/* Look for listenserver games */
			game_list_t *gl = MakeGameList(fd.cFileName);

			for (unsigned int i = 0; valve_game_list[i].folder != NULL; i++)
			{
				unsigned int mod_id;
				UTIL_PathFormat(temp_path,
					sizeof(temp_path) / sizeof(TCHAR),
					_T("%s\\steamapps\\%s\\%s\\%s"),
					steam_path,
					fd.cFileName,
					valve_game_list[i].folder,
					valve_game_list[i].subfolder);
				if (TryToAddMod(temp_path, valve_game_list[i].eng_type, db, &mod_id))
				{
					AttachModToGameList(gl, mod_id);
				}
			}

			if (gl->game_count)
			{
				AttachGameListToGroup(&db->listen, gl);
			}
			else
			{
				free(gl);
			}

			/* Look for dedicated games */
			gl = MakeGameList(fd.cFileName);

			for (unsigned int i = 0; valve_server_list[i].folder != NULL; i++)
			{
				UTIL_PathFormat(temp_path,
					sizeof(temp_path) / sizeof(TCHAR),
					_T("%s\\steamapps\\%s\\%s"),
					steam_path,
					fd.cFileName,
					valve_server_list[i].folder);
				AddModsFromFolder(temp_path, valve_server_list[i].eng_type, db, gl);
			}

			if (gl->game_count)
			{
				AttachGameListToGroup(&db->dedicated, gl);
			}
			else
			{
				free(gl);
			}
		}

	} while (FindNextFile(hFind, &fd));

	FindClose(hFind);
	RegCloseKey(hkPath);
}

void GetStandaloneGames(game_database_t *db)
{
	HKEY hkPath;
	DWORD dwLen, dwType, dwAttr;
	TCHAR temp_path[MAX_PATH];
	TCHAR hlds_path[MAX_PATH];
	game_list_t *games_standalone;

	if (RegOpenKeyEx(HKEY_CURRENT_USER,
		_T("Software\\Valve\\HLServer"),
		0,
		KEY_READ,
		&hkPath) != ERROR_SUCCESS)
	{
		db->standalone.error_code = GAME_LIST_CANT_READ;
		return;
	}

	dwLen = sizeof(hlds_path) / sizeof(TCHAR);
	if (RegQueryValueEx(hkPath,
		_T("InstallPath"),
		NULL,
		&dwType,
		(LPBYTE)hlds_path,
		&dwLen) != ERROR_SUCCESS)
	{
		RegCloseKey(hkPath);
		db->standalone.error_code = GAME_LIST_CANT_READ;
		return;
	}

	/* Make sure there is a "srcds.exe" file */
	UTIL_PathFormat(temp_path,
		sizeof(temp_path) / sizeof(TCHAR), 
		_T("%s\\srcds.exe"), 
		hlds_path);
	dwAttr = GetFileAttributes(temp_path);
	if (dwAttr == INVALID_FILE_ATTRIBUTES)
	{
		db->standalone.error_code = GAME_LIST_HALFLIFE1;
		return;
	}

	games_standalone = MakeGameList(_T("Standalone"));

	/* If there is an "orangebox" sub folder, we can make a better guess 
	 * at the engine state.
	 */
	UTIL_PathFormat(temp_path,
		sizeof(temp_path) / sizeof(TCHAR), 
		_T("%s\\orangebox"), 
		hlds_path);
	dwAttr = GetFileAttributes(temp_path);
	if (dwAttr != INVALID_FILE_ATTRIBUTES
		&& ((dwAttr & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY))
	{
		AddModsFromFolder(temp_path, SOURCE_ENGINE_2007, db, games_standalone);
	}

	/* Add everything from the server */
	AddModsFromFolder(hlds_path, SOURCE_ENGINE_2004, db, games_standalone);

	if (games_standalone->game_count)
	{
		AttachGameListToGroup(&db->standalone, games_standalone);
	}
	else
	{
		free(games_standalone);
	}

	RegCloseKey(hkPath);
}

void DisplayBadGamesDialog(HWND hWnd, int reason)
{
	TCHAR message[256];
	UINT idc = 0;

	if (reason == GAME_LIST_CANT_READ)
	{
		idc = IDS_GAME_FAIL_READ;
	}
	else if (reason == GAME_LIST_HALFLIFE1)
	{
		idc = IDS_GAME_FAIL_HL1;
	}
	else if (reason == GAME_LIST_NO_GAMES)
	{
		idc = IDS_GAME_FAIL_NONE;
	}
	else
	{
		return;
	}

	if (LoadString(g_hInstance,
		idc,
		message,
		sizeof(message) / sizeof(TCHAR)) == 0)
	{
		return;
	}

	MessageBox(hWnd, 
		message, 
		_T("SourceMod Installer"),
		MB_OK|MB_ICONWARNING);
}

int _ModIdCompare(const void *item1, const void *item2)
{
	unsigned int mod_id1 = *(unsigned int *)item1;
	unsigned int mod_id2 = *(unsigned int *)item2;

	return tstrcasecmp(g_games.game_list[mod_id1].name, g_games.game_list[mod_id2].name);
}

int _GroupCompare(const void *item1, const void *item2)
{
	game_list_t *g1 = *(game_list_t **)item1;
	game_list_t *g2 = *(game_list_t **)item2;

	return tstrcasecmp(g1->root_name, g2->root_name);
}

void SortGameGroup(game_group_t *group)
{
	qsort(group->lists, group->list_count, sizeof(game_list_t *), _GroupCompare);

	for (unsigned int i = 0; i < group->list_count; i++)
	{
		qsort(group->lists[i]->games,
			  group->lists[i]->game_count,
			  sizeof(unsigned int),
			  _ModIdCompare);
	}
}

void BuildGameDB()
{
	ReleaseGameDB();
	GetStandaloneGames(&g_games);
	GetSteamGames(&g_games);
	SortGameGroup(&g_games.dedicated);
	SortGameGroup(&g_games.listen);
	SortGameGroup(&g_games.standalone);
}

void ReleaseGameGroup(game_group_t *group)
{
	for (unsigned int i = 0; i < group->list_count; i++)
	{
		free(group->lists[i]->games);
		free(group->lists[i]);
	}
	free(group->lists);
}

void ReleaseGameDB()
{
	ReleaseGameGroup(&g_games.dedicated);
	ReleaseGameGroup(&g_games.listen);
	ReleaseGameGroup(&g_games.standalone);
	free(g_games.game_list);
	memset(&g_games, 0, sizeof(g_games));
}
