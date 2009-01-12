/* KitServer Control Panel */
/* Version 1.0 (with Win32 GUI) by Juce. */

#include <windows.h>
#include <windef.h>
#include <string.h>
#include <stdio.h>

#include "mydll.h"
#include "shared.h"
#include "win32gui.h"
#include "config.h"
#include "kctrl.h"

HWND hWnd = NULL;

// configuration
KSERV_CONFIG g_config = {
	DEFAULT_DEBUG,
	DEFAULT_KDB_DIR,
	DEFAULT_VKEY_HOMEKIT,
	DEFAULT_VKEY_AWAYKIT,
	DEFAULT_VKEY_GKHOMEKIT,
	DEFAULT_VKEY_GKAWAYKIT,
	DEFAULT_VKEY_PREV_BALL,
	DEFAULT_VKEY_NEXT_BALL,
	DEFAULT_VKEY_RANDOM_BALL,
	DEFAULT_VKEY_RESET_BALL,
	DEFAULT_USE_LARGE_TEXTURE,
};

void ApplySettings(void)
{
	char buf[BUFLEN];

	SendMessage(g_kdbDirControl, WM_SETTEXT, 0, (LPARAM)g_config.kdbDir);
	VKEY_TEXT(g_config.vKeyHomeKit, buf, BUFLEN); 
	SendMessage(g_keyHomeKitControl, WM_SETTEXT, 0, (LPARAM)buf);
	VKEY_TEXT(g_config.vKeyAwayKit, buf, BUFLEN); 
	SendMessage(g_keyAwayKitControl, WM_SETTEXT, 0, (LPARAM)buf);
	VKEY_TEXT(g_config.vKeyGKHomeKit, buf, BUFLEN); 
	SendMessage(g_keyGKHomeKitControl, WM_SETTEXT, 0, (LPARAM)buf);
	VKEY_TEXT(g_config.vKeyGKAwayKit, buf, BUFLEN); 
	SendMessage(g_keyGKAwayKitControl, WM_SETTEXT, 0, (LPARAM)buf);
	VKEY_TEXT(g_config.vKeyPrevBall, buf, BUFLEN); 
	SendMessage(g_keyPrevBallControl, WM_SETTEXT, 0, (LPARAM)buf);
	VKEY_TEXT(g_config.vKeyNextBall, buf, BUFLEN); 
	SendMessage(g_keyNextBallControl, WM_SETTEXT, 0, (LPARAM)buf);
	VKEY_TEXT(g_config.vKeyRandomBall, buf, BUFLEN); 
	SendMessage(g_keyRandomBallControl, WM_SETTEXT, 0, (LPARAM)buf);
	VKEY_TEXT(g_config.vKeyResetBall, buf, BUFLEN); 
	SendMessage(g_keyResetBallControl, WM_SETTEXT, 0, (LPARAM)buf);
}

/**
 * Restores last saved settings.
 */
void RestoreSettings(void)
{
	// read optional configuration file
	ReadConfig(&g_config, "kserv.cfg");
	ApplySettings();
}

/**
 * Saves settings.
 */
void SaveSettings(void)
{
	// write configuration file
	WriteConfig(&g_config, "kserv.cfg");
	ApplySettings();
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int home, away, timecode;
	char buf[BUFLEN];

	switch(uMsg)
	{
		case WM_DESTROY:
			// Exit the application when the window closes
			PostQuitMessage(1);
			return true;

		case WM_COMMAND:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				if ((HWND)lParam == g_saveButtonControl)
				{
					ZeroMemory(g_config.kdbDir, BUFLEN);
					SendMessage(g_kdbDirControl, WM_GETTEXT, (WPARAM)BUFLEN, (LPARAM)g_config.kdbDir);
					// make sure it ends with a backslash
					int last = lstrlen(g_config.kdbDir) - 1;
					if ((last == -1) || ((last >= 0) && (g_config.kdbDir[last] != '\\')))
						g_config.kdbDir[last + 1] = '\\';

					SaveSettings();
					// modify status text
					SendMessage(g_statusTextControl, WM_SETTEXT, 0, (LPARAM)"SAVED");
				}
				else if ((HWND)lParam == g_restoreButtonControl)
				{
					RestoreSettings();
					// modify status text
					SendMessage(g_statusTextControl, WM_SETTEXT, 0, (LPARAM)"RESTORED");
				}
			}
			else if (HIWORD(wParam) == EN_CHANGE)
			{
				HWND control = (HWND)lParam;
				// modify status text
				SendMessage(g_statusTextControl, WM_SETTEXT, 0, (LPARAM)"CHANGES MADE");
			}
			else if (HIWORD(wParam) == CBN_EDITCHANGE)
			{
				HWND control = (HWND)lParam;
				// modify status text
				SendMessage(g_statusTextControl, WM_SETTEXT, 0, (LPARAM)"CHANGES MADE");
			}
			break;

		case WM_APP_KEYDEF:
			HWND target = (HWND)lParam;
			ZeroMemory(buf, BUFLEN);
			GetKeyNameText(MapVirtualKey(wParam, 0) << 16, buf, BUFLEN);
			SendMessage(target, WM_SETTEXT, 0, (LPARAM)buf);
			SendMessage(g_statusTextControl, WM_SETTEXT, 0, (LPARAM)"CHANGES MADE");
			// update config
			if (target == g_keyHomeKitControl) g_config.vKeyHomeKit = (WORD)wParam;
			else if (target == g_keyAwayKitControl) g_config.vKeyAwayKit = (WORD)wParam;
			else if (target == g_keyGKHomeKitControl) g_config.vKeyGKHomeKit = (WORD)wParam;
			else if (target == g_keyGKAwayKitControl) g_config.vKeyGKAwayKit = (WORD)wParam;
			else if (target == g_keyPrevBallControl) g_config.vKeyPrevBall = (WORD)wParam;
			else if (target == g_keyNextBallControl) g_config.vKeyNextBall = (WORD)wParam;
			else if (target == g_keyRandomBallControl) g_config.vKeyRandomBall = (WORD)wParam;
			else if (target == g_keyResetBallControl) g_config.vKeyResetBall = (WORD)wParam;
			break;
	}
	return DefWindowProc(hwnd,uMsg,wParam,lParam);
}

bool InitApp(HINSTANCE hInstance, LPSTR lpCmdLine)
{
	WNDCLASSEX wcx;

	// cbSize - the size of the structure.
	wcx.cbSize = sizeof(WNDCLASSEX);
	wcx.style = CS_HREDRAW | CS_VREDRAW;
	wcx.lpfnWndProc = (WNDPROC)WindowProc;
	wcx.cbClsExtra = 0;
	wcx.cbWndExtra = 0;
	wcx.hInstance = hInstance;
	wcx.hIcon = LoadIcon(NULL,IDI_APPLICATION);
	wcx.hCursor = LoadCursor(NULL,IDC_ARROW);
	wcx.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	wcx.lpszMenuName = NULL;
	wcx.lpszClassName = "KSERVCLS";
	wcx.hIconSm = NULL;

	// Register the class with Windows
	if(!RegisterClassEx(&wcx))
		return false;

	// read optional configuration file
	ReadConfig(&g_config, "kserv.cfg");

	return true;
}

HWND BuildWindow(int nCmdShow)
{
	DWORD style, xstyle;
	HWND retval;

	style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
	xstyle = WS_EX_LEFT;

	retval = CreateWindowEx(xstyle,
        "KSERVCLS",      // class name
        KSERV_WINDOW_TITLE, // title for our window (appears in the titlebar)
        style,
        CW_USEDEFAULT,  // initial x coordinate
        CW_USEDEFAULT,  // initial y coordinate
        WIN_WIDTH, WIN_HEIGHT,   // width and height of the window
        NULL,           // no parent window.
        NULL,           // no menu
        NULL,           // no creator
        NULL);          // no extra data

	if (retval == NULL) return NULL;  // BAD.

	ShowWindow(retval,nCmdShow);  // Show the window
	return retval; // return its handle for future use.
}

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	MSG msg; int retval;

 	if(InitApp(hInstance, lpCmdLine) == false)
		return 0;

	hWnd = BuildWindow(nCmdShow);
	if(hWnd == NULL)
		return 0;

	// build GUI
	if (!BuildControls(hWnd))
		return 0;

	// Initialize all controls
	ApplySettings();

	// show credits
	char buf[BUFLEN];
	ZeroMemory(buf, BUFLEN);
	strncpy(buf, CREDITS, BUFLEN-1);
	SendMessage(g_statusTextControl, WM_SETTEXT, 0, (LPARAM)buf);

	while((retval = GetMessage(&msg,NULL,0,0)) != 0)
	{
		// capture key-def events
		if ((msg.hwnd == g_keyHomeKitControl || msg.hwnd == g_keyAwayKitControl ||
			 msg.hwnd == g_keyGKHomeKitControl || msg.hwnd == g_keyGKAwayKitControl ||
			 msg.hwnd == g_keyPrevBallControl || msg.hwnd == g_keyNextBallControl ||
			 msg.hwnd == g_keyRandomBallControl || msg.hwnd == g_keyResetBallControl) &&
		    (msg.message == WM_KEYDOWN || msg.message == WM_SYSKEYDOWN))
		{
			PostMessage(hWnd, WM_APP_KEYDEF, msg.wParam, (LPARAM)msg.hwnd);
			continue;
		}

		if(retval == -1)
			return 0;	// an error occured while getting a message

		if (!IsDialogMessage(hWnd, &msg)) // need to call this to make WS_TABSTOP work
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return 0;
}

