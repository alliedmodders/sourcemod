#include "GamesList.h"
#include "InstallerUtil.h"
#include "InstallerMain.h"
#include <stdio.h>

mod_info_t *g_mod_list = NULL;
unsigned int g_mod_count = 0;

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

void AddModToList(mod_info_t **mod_list,
				  unsigned int *total_mods,
				  const mod_info_t *mod_info)
{
	mod_info_t *mods = *mod_list;
	unsigned int total = *total_mods;

	if (mods == NULL)
	{
		mods = (mod_info_t *)malloc(sizeof(mod_info_t));
	}
	else
	{
		mods = (mod_info_t *)realloc(mods, sizeof(mod_info_t) * (total + 1));
	}

	memcpy(&mods[total], mod_info, sizeof(mod_info_t));
	total++;

	*mod_list = mods;
	*total_mods = total;
}

void TryToAddMod(const TCHAR *path, int eng_type, mod_info_t **mod_list, unsigned *total_mods)
{
	FILE *fp;
	TCHAR gameinfo_path[MAX_PATH];

	UTIL_PathFormat(gameinfo_path,
		sizeof(gameinfo_path),
		_T("%s\\gameinfo.txt"),
		path);

	if ((fp = _tfopen(gameinfo_path, _T("rt"))) == NULL)
	{
		return;
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
			mod_info_t mod;
			AnsiToUnicode(value, mod.name, sizeof(mod.name));
			UTIL_Format(mod.mod_path, sizeof(mod.mod_path), _T("%s"), path);
			mod.source_engine = eng_type;
			AddModToList(mod_list, total_mods, &mod);
		}
	}

	fclose(fp);
}

void AddModsFromFolder(const TCHAR *path, int eng_type, mod_info_t **mod_list, unsigned int *total_mods)
{
	HANDLE hFind;
	WIN32_FIND_DATA fd;
	TCHAR temp_path[MAX_PATH];
	TCHAR search_path[MAX_PATH];

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
		TryToAddMod(temp_path, eng_type, mod_list, total_mods);

	} while (FindNextFile(hFind, &fd));

	FindClose(hFind);
}

void AddValveModsFromFolder(const TCHAR *path,
							unsigned int game_type,
							mod_info_t **mod_list,
							unsigned int *total_mods)
{
	HANDLE hFind;
	WIN32_FIND_DATA fd;
	TCHAR temp_path[MAX_PATH];
	TCHAR search_path[MAX_PATH];

	UTIL_PathFormat(search_path,
		sizeof(search_path) / sizeof(TCHAR),
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

		TCHAR *mod_folder = NULL;
		int eng_type = SOURCE_ENGINE_UNKNOWN;

		if (game_type == GAMES_LISTEN)
		{
			if (tstrcasecmp(fd.cFileName, _T( "counter-strike source")) == 0)
			{
				mod_folder = _T("cstrike");
			}
			else if (tstrcasecmp(fd.cFileName, _T("day of defeat source")) == 0)
			{
				mod_folder = _T("dod");
			}
			else if (tstrcasecmp(fd.cFileName, _T("half-life 2 deathmatch")) == 0)
			{
				mod_folder = _T("hl2mp");
			}
			else if (tstrcasecmp(fd.cFileName, _T("half-life deathmatch source")) == 0)
			{
				mod_folder = _T("hl1mp");
			}
			else if (tstrcasecmp(fd.cFileName, _T("team fortress 2")) == 0)
			{
				mod_folder = _T("tf");
				eng_type = SOURCE_ENGINE_2007;
			}
		}
		else if (game_type == GAMES_DEDICATED)
		{
			if (tstrcasecmp(fd.cFileName, _T("source dedicated server")) == 0)
			{
				UTIL_PathFormat(temp_path,
					sizeof(temp_path) / sizeof(TCHAR),
					_T("%s\\%s"),
					path,
					fd.cFileName);
				AddModsFromFolder(temp_path, SOURCE_ENGINE_2004, mod_list, total_mods);
			}
			else if (tstrcasecmp(fd.cFileName, _T("source 2007 dedicated server")) == 0)
			{
				UTIL_PathFormat(temp_path,
					sizeof(temp_path) / sizeof(TCHAR),
					_T("%s\\%s"),
					path,
					fd.cFileName);
				AddModsFromFolder(temp_path, SOURCE_ENGINE_2007, mod_list, total_mods);
			}
		}

		if (mod_folder != NULL)
		{
			UTIL_PathFormat(temp_path,
				sizeof(temp_path) / sizeof(TCHAR),
				_T("%s\\%s\\%s"),
				path,
				fd.cFileName,
				mod_folder);
			TryToAddMod(temp_path, eng_type, mod_list, total_mods);
		}

	} while (FindNextFile(hFind, &fd));

	FindClose(hFind);
}

int BuildGameList(unsigned int game_type, mod_info_t **mod_list)
{
	unsigned int total_mods = 0;
	
	if (game_type == GAMES_LISTEN
		|| game_type == GAMES_DEDICATED)
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
			DWORD err = GetLastError();
			return GAME_LIST_CANT_READ;
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
			return GAME_LIST_CANT_READ;
		}

		UTIL_PathFormat(steamapps_path,
			sizeof(steamapps_path) / sizeof(TCHAR),
			_T("%s\\steamapps\\*.*"),
			steam_path);

		if ((hFind = FindFirstFile(steamapps_path, &fd)) == INVALID_HANDLE_VALUE)
		{
			RegCloseKey(hkPath);
			return GAME_LIST_CANT_READ;
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
			if (game_type == GAMES_LISTEN 
				&& tstrcasecmp(fd.cFileName, _T("SourceMods")) == 0)
			{
				UTIL_PathFormat(temp_path,
					sizeof(temp_path) / sizeof(TCHAR),
					_T("%s\\steamapps\\%s"),
					steam_path,
					fd.cFileName);
				AddModsFromFolder(temp_path, SOURCE_ENGINE_UNKNOWN, mod_list, &total_mods);
			}
			else
			{
				UTIL_PathFormat(temp_path,
					sizeof(temp_path) / sizeof(TCHAR),
					_T("%s\\steamapps\\%s"),
					steam_path,
					fd.cFileName);
				AddValveModsFromFolder(temp_path, game_type, mod_list, &total_mods);
			}

		} while (FindNextFile(hFind, &fd));

		FindClose(hFind);
		RegCloseKey(hkPath);
	}
	else if (game_type == GAMES_STANDALONE)
	{
		HKEY hkPath;
		int eng_type = SOURCE_ENGINE_UNKNOWN;
		DWORD dwLen, dwType, dwAttr;
		TCHAR temp_path[MAX_PATH];
		TCHAR hlds_path[MAX_PATH];

		if (RegOpenKeyEx(HKEY_CURRENT_USER,
			_T("Software\\Valve\\HLServer"),
			0,
			KEY_READ,
			&hkPath) != ERROR_SUCCESS)
		{
			return GAME_LIST_CANT_READ;
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
			return GAME_LIST_CANT_READ;
		}

		/* Make sure there is a "srcds.exe" file */
		UTIL_PathFormat(temp_path,
			sizeof(temp_path) / sizeof(TCHAR), 
			_T("%s\\srcds.exe"), 
			hlds_path);
		dwAttr = GetFileAttributes(temp_path);
		if (dwAttr == INVALID_FILE_ATTRIBUTES)
		{
			return GAME_LIST_HALFLIFE1;
		}

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
			eng_type = SOURCE_ENGINE_2004;
			AddModsFromFolder(temp_path, SOURCE_ENGINE_2007, mod_list, &total_mods);
		}

		/* Add everything from the server */
		AddModsFromFolder(hlds_path, eng_type, mod_list, &total_mods);

		RegCloseKey(hkPath);
	}

	return (g_mod_list == NULL) ? GAME_LIST_NO_GAMES : (int)total_mods;
}

void FreeGameList(mod_info_t *mod_list)
{
	free(mod_list);
}

int _SortModList(const void *item1, const void *item2)
{
	const mod_info_t *mod1 = (const mod_info_t *)item1;
	const mod_info_t *mod2 = (const mod_info_t *)item2;

	return tstrcasecmp(mod1->name, mod2->name);
}

int FindGames(unsigned int game_type)
{
	int reason;

	ReleaseGamesList();

	if ((reason = BuildGameList(game_type, &g_mod_list)) > 0)
	{
		g_mod_count = (unsigned)reason;
		qsort(g_mod_list, g_mod_count, sizeof(mod_info_t), _SortModList);
	}

	return reason;
}

void ReleaseGamesList()
{
	if (g_mod_list == NULL)
	{
		return;
	}

	free(g_mod_list);
	g_mod_list = NULL;
	g_mod_count = 0;
}

void DisplayBadGamesDialog(HWND hWnd, unsigned int game_type, int reason)
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
