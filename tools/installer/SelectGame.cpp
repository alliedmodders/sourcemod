#include "InstallerMain.h"
#include "InstallerUtil.h"
#include "SelectGame.h"
#include "GamesList.h"
#include "ChooseMethod.h"
#include "PerformInstall.h"
#include "LocalCopyMethod.h"

int selected_game_index = -1;
mod_info_t **game_sel_list = NULL;
unsigned int game_sel_count = 0;

void AppendSelectableGame(mod_info_t *mod)
{
	if (game_sel_list == NULL)
	{
		game_sel_list = 
			(mod_info_t **)malloc(sizeof(mod_info_t *) * (game_sel_count + 1));
	}
	else
	{
		game_sel_list = 
			(mod_info_t **)realloc(game_sel_list, 
			sizeof(mod_info_t *) * (game_sel_count + 1));
	}

	game_sel_list[game_sel_count++] = mod;
}

INT_PTR CALLBACK ChooseGameHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		{
			HWND lbox = GetDlgItem(hDlg, IDC_SELGAME_LIST);

			for (unsigned int i = 0; i < g_mod_count; i++)
			{
				LRESULT res = SendMessage(lbox,
					LB_ADDSTRING,
					0,
					(LPARAM)g_mod_list[i].name);

				if (res == LB_ERR || res == LB_ERRSPACE)
				{
					continue;
				}

				AppendSelectableGame(&g_mod_list[i]);
			}

			UpdateWindow(lbox);

			SetToGlobalPosition(hDlg);
			
			return (INT_PTR)TRUE;
		}
	case WM_COMMAND:
		{
			if (LOWORD(wParam) == ID_SELGAME_EXIT
				|| LOWORD(wParam) == ID_CLOSE)
			{
				return AskToExit(hDlg);
			}
			else if (LOWORD(wParam) == ID_SELGAME_BACK)
			{
				UpdateGlobalPosition(hDlg);
				EndDialog(hDlg, (INT_PTR)DisplayChooseMethod);
				return (INT_PTR)TRUE;
			}
			else if (LOWORD(wParam) == IDC_SELGAME_LIST)
			{
				if (HIWORD(wParam) == LBN_SELCHANGE)
				{
					HWND lbox = (HWND)lParam;
					LRESULT cursel = SendMessage(lbox, LB_GETCURSEL, 0, 0);

					selected_game_index = -1;

					if (cursel == LB_ERR)
					{
						break;
					}

					if (cursel >= (LRESULT)g_mod_count)
					{
						break;
					}

					selected_game_index = (int)cursel;

					HWND button = GetDlgItem(hDlg, ID_SELGAME_NEXT);
					EnableWindow(button, TRUE);
				}
			}
			else if (LOWORD(wParam) == ID_SELGAME_NEXT)
			{
				if (selected_game_index == -1)
				{
					break;
				}

				g_LocalCopier.SetOutputPath(game_sel_list[selected_game_index]->mod_path);
				SetInstallMethod(&g_LocalCopier);

				UpdateGlobalPosition(hDlg);
				EndDialog(hDlg, (INT_PTR)DisplayPerformInstall);

				return (INT_PTR)TRUE;
			}
			break;
		}
	case WM_DESTROY:
		{
			ReleaseGamesList();
			free(game_sel_list);
			game_sel_list = NULL;
			game_sel_count = 0;
			break;
		}
	}

	return (INT_PTR)FALSE;
}

void *DisplaySelectGame(HWND hWnd)
{
	INT_PTR val;
	
	if ((val = DialogBox(
		g_hInstance, 
		MAKEINTRESOURCE(IDD_SELECT_GAME),
		hWnd,
		ChooseGameHandler)) == -1)
	{
		return NULL;
	}

	return (void *)val;
}

