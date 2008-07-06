#include "InstallerMain.h"
#include "Welcome.h"

#define WMU_INIT_INSTALLER		WM_USER+1

HINSTANCE g_hInstance;
NEXT_DIALOG next_dialog = DisplayWelcome;
POINT g_GlobalPosition;

void UpdateGlobalPosition(HWND hWnd)
{
	WINDOWINFO wi;

	wi.cbSize = sizeof(WINDOWINFO);
	if (GetWindowInfo(hWnd, &wi))
	{
		g_GlobalPosition.x = wi.rcWindow.left;
		g_GlobalPosition.y = wi.rcWindow.top;
	}
}

void SetToGlobalPosition(HWND hWnd)
{
	WINDOWINFO wi;
	
	wi.cbSize = sizeof(WINDOWINFO);
	if (GetWindowInfo(hWnd, &wi))
	{
		MoveWindow(hWnd, 
			g_GlobalPosition.x, 
			g_GlobalPosition.y, 
			wi.rcWindow.right - wi.rcWindow.left,
			wi.rcWindow.bottom - wi.rcWindow.top,
			TRUE);
	}
}

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WMU_INIT_INSTALLER:
		{
			UpdateGlobalPosition(hWnd);
			while (next_dialog != NULL)
			{
				next_dialog = (NEXT_DIALOG)next_dialog(hWnd);
			}
			PostQuitMessage(0);
			break;
		}
	case WM_DESTROY:
		{
			PostQuitMessage(0);
			break;
		}
	default:
		{
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}

	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	WNDCLASSEX wcex;
	BOOL bRet;

	wcex.cbSize			= sizeof(wcex);
	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= MainWndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_INSTALLER));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= _T("InstallerMenu");
	wcex.lpszClassName	= _T("Installer");
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	if (!RegisterClassEx(&wcex))
	{
		return 1;
	}

	INITCOMMONCONTROLSEX ccex;
	ccex.dwSize = sizeof(ccex);
	ccex.dwICC = ICC_BAR_CLASSES
		|ICC_HOTKEY_CLASS
		|ICC_LISTVIEW_CLASSES
		|ICC_PROGRESS_CLASS
		|ICC_WIN95_CLASSES
		|ICC_TAB_CLASSES;

	if (!InitCommonControlsEx(&ccex))
	{
		return 1;
	}

	g_hInstance = hInstance;

	HWND hWnd = CreateWindow(
		_T("Installer"),
		_T("InstallerMain"),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		(HWND)NULL,
		(HMENU)NULL,
		hInstance,
		NULL);
	if (hWnd == NULL)
	{
		return 1;
	}

	ShowWindow(hWnd, SW_HIDE);
	UpdateWindow(hWnd);

	PostMessage(hWnd, WMU_INIT_INSTALLER, 0, 0);

	MSG msg;
	while ((bRet = GetMessage(&msg, NULL, 0, 0)) != 0)
	{
		if (bRet == -1)
		{
			return 1;
		}
		else
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}
