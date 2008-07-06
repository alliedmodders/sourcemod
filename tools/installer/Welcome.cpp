#include "InstallerMain.h"
#include "Welcome.h"
#include "ChooseMethod.h"

bool g_bIsFirstRun = true;

INT_PTR CALLBACK WelcomeHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		{
			if (!g_bIsFirstRun)
			{
				SetToGlobalPosition(hDlg);
			}
			else
			{
				g_bIsFirstRun = false;
			}
			return (INT_PTR)TRUE;
		}
	case WM_COMMAND:
		{
			if (LOWORD(wParam) == ID_WELCOME_EXIT
				|| LOWORD(wParam) == ID_CLOSE)
			{
				UpdateGlobalPosition(hDlg);
				EndDialog(hDlg, NULL);
				return (INT_PTR)TRUE;
			}
			else if (LOWORD(wParam) == ID_WELCOME_NEXT)
			{
				UpdateGlobalPosition(hDlg);
				EndDialog(hDlg, (INT_PTR)DisplayChooseMethod);
				return (INT_PTR)TRUE;
			}
			break;
		}
	}

	return (INT_PTR)FALSE;
}

void *DisplayWelcome(HWND hWnd)
{
	INT_PTR val;
	
	if ((val = DialogBox(
		g_hInstance, 
		MAKEINTRESOURCE(IDD_WELCOME),
		hWnd,
		WelcomeHandler)) == -1)
	{
		return NULL;
	}

	return (void *)val;
}

