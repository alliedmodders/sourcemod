#include "InstallerMain.h"
#include "InstallerUtil.h"
#include "ChooseMethod.h"
#include "Welcome.h"
#include "GamesList.h"
#include "SelectGame.h"
#include "PerformInstall.h"
#include "LocalCopyMethod.h"

game_group_t *g_game_group = NULL;
unsigned int method_chosen = 0;
TCHAR method_path[MAX_PATH];

bool SelectFolder(HWND hOwner)
{
	BROWSEINFO info;
	LPITEMIDLIST pidlist;
	TCHAR path[MAX_PATH];

	if (FAILED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED)))
	{
		return false;
	}
	
	info.hwndOwner = hOwner;
	info.pidlRoot = NULL;
	info.pszDisplayName = path;
	info.lpszTitle = _T("Select a game/mod folder");
	info.ulFlags = BIF_EDITBOX | BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
	info.lpfn = NULL;
	info.lParam = 0;
	info.iImage = 0;

	if ((pidlist = SHBrowseForFolder(&info)) == NULL)
	{
		CoUninitialize();
		return false;
	}

	/* This hellish code is from MSDN and translate shortcuts to real targets.
	 * God almighty, I wish Window used real symlinks.
	 */
	bool acquire_success = false;
	bool is_link = false;
	IShellFolder *psf = NULL;
	LPCITEMIDLIST new_item_list;
	HRESULT hr;
	
	hr = SHBindToParent(pidlist, IID_IShellFolder, (void **)&psf, &new_item_list);
	if (SUCCEEDED(hr))
	{
		IShellLink *psl = NULL;

		hr = psf->GetUIObjectOf(hOwner, 1, &new_item_list, IID_IShellLink, NULL, (void **)&psl);
		if (SUCCEEDED(hr))
		{
			LPITEMIDLIST new_item_list;

			hr = psl->GetIDList(&new_item_list);
			if (SUCCEEDED(hr))
			{
				is_link = true;

				hr = SHGetPathFromIDList(new_item_list, method_path);
				if (SUCCEEDED(hr))
				{
					acquire_success = true;
				}

				CoTaskMemFree(new_item_list);
			}
			psl->Release();
		}
		psf->Release();
	}

	if (!acquire_success && !is_link)
	{
		hr = SHGetPathFromIDList(pidlist, method_path);
		if (SUCCEEDED(hr))
		{
			acquire_success = true;
		}
	}

	/* That was awful.  shoo, shoo, COM */
	CoTaskMemFree(pidlist);
	CoUninitialize();

	return acquire_success;
}

INT_PTR CALLBACK ChooseMethodHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
		{
			if (LOWORD(wParam) == ID_METHOD_BACK)
			{
				UpdateGlobalPosition(hDlg);
				EndDialog(hDlg, (INT_PTR)DisplayWelcome);
				return (INT_PTR)TRUE;
			}
			else if (LOWORD(wParam) == ID_METHOD_EXIT
					 || LOWORD(wParam) == ID_CLOSE)
			{
				return AskToExit(hDlg);
			}
			else if (LOWORD(wParam) == IDC_METHOD_DED_SERVER
					 || LOWORD(wParam) == IDC_METHOD_ALONE_SERVER
					 || LOWORD(wParam) == IDC_METHOD_LISTEN_SERVER
					 || LOWORD(wParam) == IDC_METHOD_UPLOAD_FTP
					 || LOWORD(wParam) == IDC_METHOD_CUSTOM_FOLDER)
			{
				method_chosen = LOWORD(wParam);
				HWND button = GetDlgItem(hDlg, ID_METHOD_NEXT);
				EnableWindow(button, TRUE);
				break;
			}
			else if (LOWORD(wParam) == ID_METHOD_NEXT)
			{
				unsigned int game_type = 0;

				switch (method_chosen)
				{
				case IDC_METHOD_DED_SERVER:
					{
						game_type = GAMES_DEDICATED;
						break;
					}
				case IDC_METHOD_ALONE_SERVER:
					{
						game_type = GAMES_STANDALONE;
						break;
					}
				case IDC_METHOD_LISTEN_SERVER:
					{
						game_type = GAMES_LISTEN;
						break;
					}
				case IDC_METHOD_UPLOAD_FTP:
					{
						break;
					}
				case IDC_METHOD_CUSTOM_FOLDER:
					{
						int val;

						if (!SelectFolder(hDlg))
						{
							break;
						}
						
						val = IsValidFolder(method_path);
						if (val != GAMEINFO_IS_USABLE)
						{
							DisplayBadFolderDialog(hDlg, val);
							break;
						}

						g_LocalCopier.SetOutputPath(method_path);
						SetInstallMethod(&g_LocalCopier);

						UpdateGlobalPosition(hDlg);
						EndDialog(hDlg, (INT_PTR)DisplayPerformInstall);
					}
				}

				if (game_type != 0)
				{
					g_game_group = NULL;

					BuildGameDB();

					if (game_type == GAMES_DEDICATED)
					{
						g_game_group = &g_games.dedicated;
					}
					else if (game_type == GAMES_LISTEN)
					{
						g_game_group = &g_games.listen;
					}
					else if (game_type == GAMES_STANDALONE)
					{
						g_game_group = &g_games.standalone;
					}
					
					if (g_game_group == NULL)
					{
						return (INT_PTR)TRUE;
					}

					if (g_game_group->list_count == 0)
					{
						DisplayBadGamesDialog(hDlg, g_game_group->error_code);
						return (INT_PTR)TRUE;
					}

					/* If we got a valid games list, we can display the next 
					 * dialog box.
					 */
					UpdateGlobalPosition(hDlg);
					EndDialog(hDlg, (INT_PTR)DisplaySelectGame);
					return (INT_PTR)TRUE;
				}
			}
			break;
		}
	case WM_INITDIALOG:
		{
			SetToGlobalPosition(hDlg);
			return (INT_PTR)TRUE;
		}
	}

	return (INT_PTR)FALSE;
}

void *DisplayChooseMethod(HWND hWnd)
{
	INT_PTR val;
	
	if ((val = DialogBox(
		g_hInstance, 
		MAKEINTRESOURCE(IDD_CHOOSE_METHOD),
		hWnd,
		ChooseMethodHandler)) == -1)
	{
		return NULL;
	}

	return (void *)val;
}

