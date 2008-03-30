#include "InstallerMain.h"
#include "InstallerUtil.h"
#include "SelectGame.h"
#include "GamesList.h"
#include "ChooseMethod.h"
#include "PerformInstall.h"
#include "LocalCopyMethod.h"

int selected_game_index = -1;

void UpdateGameListBox(HWND hDlg, game_list_t *gl)
{
	HWND lbox = GetDlgItem(hDlg, IDC_SELGAME_LIST);

	SendMessage(lbox, LB_RESETCONTENT, 0, 0);

	for (unsigned int i = 0; i < gl->game_count; i++)
	{
		LRESULT res = SendMessage(lbox,
			LB_ADDSTRING,
			0,
			(LPARAM)g_games.game_list[gl->games[i]].name);

		if (res == LB_ERR || res == LB_ERRSPACE)
		{
			continue;
		}

		SendMessage(lbox, LB_SETITEMDATA, i, gl->games[i]);
	}
	
	UpdateWindow(lbox);
}

#include "windowsx.h"

INT_PTR CALLBACK ChooseGameHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		{
			HWND cbox = GetDlgItem(hDlg, IDC_SELGROUP_ACCOUNT);
			SendMessage(cbox, CB_RESETCONTENT, 0, 0);
			for (unsigned int i = 0; i < g_game_group->list_count; i++)
			{
				LRESULT res = SendMessage(cbox, 
					CB_ADDSTRING, 
					0, 
					(LPARAM)g_game_group->lists[i]->root_name);
				if (res == CB_ERR || res == CB_ERRSPACE)
				{
					continue;
				}
				SendMessage(cbox, CB_SETITEMDATA, i, (LPARAM)g_game_group->lists[i]);
			}
			SendMessage(cbox, CB_SETCURSEL, 0, 0);
			UpdateWindow(cbox);
			UpdateGameListBox(hDlg, g_game_group->lists[0]);

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
			else if (LOWORD(wParam) == IDC_SELGROUP_ACCOUNT)
			{
				if (HIWORD(wParam) == CBN_SELCHANGE)
				{
					HWND cbox = (HWND)lParam;
					LRESULT cursel = SendMessage(cbox, CB_GETCURSEL, 0, 0);

					if (cursel == LB_ERR)
					{
						break;
					}

					LRESULT data = SendMessage(cbox, CB_GETITEMDATA, cursel, 0);
					if (data == CB_ERR)
					{
						break;
					}

					game_list_t *gl = (game_list_t *)data;
					UpdateGameListBox(hDlg, gl);
				}
				break;
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

					LRESULT item = SendMessage(lbox, LB_GETITEMDATA, cursel, 0);
					if (item == LB_ERR)
					{
						break;
					}

					selected_game_index = (int)item;

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

				g_LocalCopier.SetOutputPath(g_games.game_list[selected_game_index].game_path);
				SetInstallMethod(&g_LocalCopier);

				UpdateGlobalPosition(hDlg);
				EndDialog(hDlg, (INT_PTR)DisplayPerformInstall);

				return (INT_PTR)TRUE;
			}
			break;
		}
	case WM_DESTROY:
		{
			ReleaseGameDB();
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

