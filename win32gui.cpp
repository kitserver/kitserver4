#include <windows.h>
#include <stdio.h>
#include "mydll.h"
#include "shared.h"
#include "win32gui.h"

HWND g_kdbDirControl;               // displays path to folder that contains KDB

HWND g_keyHomeKitControl;           // displays key binding for home kit switch
HWND g_keyAwayKitControl;           // displays key binding for away kit switch
HWND g_keyGKHomeKitControl;         // displays key binding for home GK kit switch
HWND g_keyGKAwayKitControl;         // displays key binding for away GK kit switch
HWND g_keyPrevBallControl;          // displays key binding for previous ball switch
HWND g_keyNextBallControl;          // displays key binding for next ball switch
HWND g_keyRandomBallControl;        // displays key binding for random ball switch
HWND g_keyResetBallControl;         // displays key binding for reset ball switch

HWND g_statusTextControl;           // displays status messages
HWND g_restoreButtonControl;        // restore settings button
HWND g_saveButtonControl;           // save settings button


/**
 * build all controls
 */
bool BuildControls(HWND parent)
{
	HGDIOBJ hObj;
	DWORD style, xstyle;
	int x, y, spacer;
	int boxW, boxH, statW, statH, borW, borH, butW, butH;

	spacer = 6; 
	x = spacer, y = spacer;
	butH = 20;

	// use default extended style
	xstyle = WS_EX_LEFT;
	style = WS_CHILD | WS_VISIBLE;

	// TOP SECTION

	borW = WIN_WIDTH - spacer*3;
	statW = 70;
	boxW = borW - statW - spacer*3; boxH = 20; statH = 16;
	borH = spacer*6 + boxH*5;

	HWND staticBorderTopControl = CreateWindowEx(
			xstyle, "Static", "", WS_CHILD | WS_VISIBLE | SS_ETCHEDFRAME,
			x, y, borW, borH,
			parent, NULL, NULL, NULL);

	x += spacer; y += spacer;

	HWND kdbDirLabel = CreateWindowEx(
			xstyle, "Static", "KDB location:", style,
			x, y+4, statW, statH, 
			parent, NULL, NULL, NULL);

	x += statW + spacer;

	char kdbDir[BUFLEN];
	ZeroMemory(kdbDir, BUFLEN);

	g_kdbDirControl = CreateWindowEx(
			xstyle | WS_EX_CLIENTEDGE, "Edit", kdbDir, style | ES_AUTOHSCROLL | WS_TABSTOP,
			x, y, boxW, boxH,
			parent, NULL, NULL, NULL);

	x = spacer*2;
	y += boxH + spacer;
	boxW = 70;

	// MIDDLE SECTION: key bindings
	
	char keyName[BUFLEN];

	HWND keyHomeKitLabel = CreateWindowEx(
			xstyle, "Static", "PL Home Kit", style,
			x, y+4, statW, statH,
			parent, NULL, NULL, NULL);

	x += statW + spacer;

	VKEY_TEXT(VKEY_HOMEKIT, keyName, BUFLEN);

	g_keyHomeKitControl = CreateWindowEx(
			xstyle | WS_EX_CLIENTEDGE, "Edit", keyName, style,
			x, y, boxW, boxH,
			parent, NULL, NULL, NULL);

	x += boxW + spacer*3 + 10;
	statW = 70; boxW = 70;

	HWND keyAwayKitLabel = CreateWindowEx(
			xstyle, "Static", "PL Away Kit", style,
			x, y+4, statW, statH,
			parent, NULL, NULL, NULL);

	x += statW + spacer;

	VKEY_TEXT(VKEY_AWAYKIT, keyName, BUFLEN);

	g_keyAwayKitControl = CreateWindowEx(
			xstyle | WS_EX_CLIENTEDGE, "Edit", keyName, style,
			x, y, boxW, boxH,
			parent, NULL, NULL, NULL);

	statW = 70; boxW = 70;

	y += boxH + spacer;
	x = spacer*2;

	HWND keyGKHomeKitLabel = CreateWindowEx(
			xstyle, "Static", "GK Home Kit", style,
			x, y+4, statW, statH,
			parent, NULL, NULL, NULL);

	x += statW + spacer;

	VKEY_TEXT(VKEY_GKHOMEKIT, keyName, BUFLEN);

	g_keyGKHomeKitControl = CreateWindowEx(
			xstyle | WS_EX_CLIENTEDGE, "Edit", keyName, style,
			x, y, boxW, boxH,
			parent, NULL, NULL, NULL);

	x += boxW + spacer*3 + 10;
	statW = 70; boxW = 70;

	HWND keyGKAwayKitLabel = CreateWindowEx(
			xstyle, "Static", "GK Away Kit", style,
			x, y+4, statW, statH,
			parent, NULL, NULL, NULL);

	x += statW + spacer;

	VKEY_TEXT(VKEY_GKAWAYKIT, keyName, BUFLEN);

	g_keyGKAwayKitControl = CreateWindowEx(
			xstyle | WS_EX_CLIENTEDGE, "Edit", keyName, style,
			x, y, boxW, boxH,
			parent, NULL, NULL, NULL);

	x += boxW + spacer*3;
	statW = 70; boxW = 70;

	y += boxH + spacer;
	x = spacer*2;

	HWND keyPrevBallLabel = CreateWindowEx(
			xstyle, "Static", "Previous Ball", style,
			x, y+4, statW, statH,
			parent, NULL, NULL, NULL);

	x += statW + spacer;

	VKEY_TEXT(VKEY_PREV_BALL, keyName, BUFLEN);

	g_keyPrevBallControl = CreateWindowEx(
			xstyle | WS_EX_CLIENTEDGE, "Edit", keyName, style,
			x, y, boxW, boxH,
			parent, NULL, NULL, NULL);

	x += boxW + spacer*3 + 10;
	statW = 70; boxW = 70;

	HWND keyNextBallLabel = CreateWindowEx(
			xstyle, "Static", "Next Ball", style,
			x, y+4, statW, statH,
			parent, NULL, NULL, NULL);

	x += statW + spacer;

	VKEY_TEXT(VKEY_NEXT_BALL, keyName, BUFLEN);

	g_keyNextBallControl = CreateWindowEx(
			xstyle | WS_EX_CLIENTEDGE, "Edit", keyName, style,
			x, y, boxW, boxH,
			parent, NULL, NULL, NULL);

	y += boxH + spacer;
	x = spacer*2;

	HWND keyRandomBallLabel = CreateWindowEx(
			xstyle, "Static", "Random Ball", style,
			x, y+4, statW, statH,
			parent, NULL, NULL, NULL);

	x += statW + spacer;

	VKEY_TEXT(VKEY_RANDOM_BALL, keyName, BUFLEN);

	g_keyRandomBallControl = CreateWindowEx(
			xstyle | WS_EX_CLIENTEDGE, "Edit", keyName, style,
			x, y, boxW, boxH,
			parent, NULL, NULL, NULL);

	x += boxW + spacer*3 + 10;
	statW = 70; boxW = 70;

	HWND keyResetBallLabel = CreateWindowEx(
			xstyle, "Static", "Reset Ball", style,
			x, y+4, statW, statH,
			parent, NULL, NULL, NULL);

	x += statW + spacer;

	VKEY_TEXT(VKEY_RESET_BALL, keyName, BUFLEN);

	g_keyResetBallControl = CreateWindowEx(
			xstyle | WS_EX_CLIENTEDGE, "Edit", keyName, style,
			x, y, boxW, boxH,
			parent, NULL, NULL, NULL);

	y += boxH + spacer*2;
	x = spacer;

	// BOTTOM SECTION - buttons

	butW = 60; butH = 24;
	x = WIN_WIDTH - spacer*2 - butW;

	g_saveButtonControl = CreateWindowEx(
			xstyle, "Button", "Save", style | WS_TABSTOP,
			x, y, butW, butH,
			parent, NULL, NULL, NULL);

	butW = 60;
	x -= butW + spacer;

	g_restoreButtonControl = CreateWindowEx(
			xstyle, "Button", "Restore", style | WS_TABSTOP,
			x, y, butW, butH,
			parent, NULL, NULL, NULL);

	x = spacer;
	statW = WIN_WIDTH - spacer*4 - 160;

	g_statusTextControl = CreateWindowEx(
			xstyle, "Static", "", style,
			x, y+6, statW, statH,
			parent, NULL, NULL, NULL);

	// If any control wasn't created, return false
	if (kdbDirLabel == NULL || g_kdbDirControl == NULL ||
		keyHomeKitLabel == NULL || g_keyHomeKitControl == NULL ||
		keyAwayKitLabel == NULL || g_keyAwayKitControl == NULL ||
		g_statusTextControl == NULL ||
		g_restoreButtonControl == NULL ||
		g_saveButtonControl == NULL)
		return false;

	// Get a handle to the stock font object
	hObj = GetStockObject(DEFAULT_GUI_FONT);

	SendMessage(kdbDirLabel, WM_SETFONT, (WPARAM)hObj, true);
	SendMessage(g_kdbDirControl, WM_SETFONT, (WPARAM)hObj, true);

	SendMessage(keyHomeKitLabel, WM_SETFONT, (WPARAM)hObj, true);
	SendMessage(g_keyHomeKitControl, WM_SETFONT, (WPARAM)hObj, true);
	SendMessage(keyAwayKitLabel, WM_SETFONT, (WPARAM)hObj, true);
	SendMessage(g_keyAwayKitControl, WM_SETFONT, (WPARAM)hObj, true);
	SendMessage(keyGKHomeKitLabel, WM_SETFONT, (WPARAM)hObj, true);
	SendMessage(g_keyGKHomeKitControl, WM_SETFONT, (WPARAM)hObj, true);
	SendMessage(keyGKAwayKitLabel, WM_SETFONT, (WPARAM)hObj, true);
	SendMessage(g_keyGKAwayKitControl, WM_SETFONT, (WPARAM)hObj, true);
	SendMessage(keyPrevBallLabel, WM_SETFONT, (WPARAM)hObj, true);
	SendMessage(g_keyPrevBallControl, WM_SETFONT, (WPARAM)hObj, true);
	SendMessage(keyNextBallLabel, WM_SETFONT, (WPARAM)hObj, true);
	SendMessage(g_keyNextBallControl, WM_SETFONT, (WPARAM)hObj, true);
	SendMessage(keyRandomBallLabel, WM_SETFONT, (WPARAM)hObj, true);
	SendMessage(g_keyRandomBallControl, WM_SETFONT, (WPARAM)hObj, true);
	SendMessage(keyResetBallLabel, WM_SETFONT, (WPARAM)hObj, true);
	SendMessage(g_keyResetBallControl, WM_SETFONT, (WPARAM)hObj, true);

	SendMessage(g_statusTextControl, WM_SETFONT, (WPARAM)hObj, true);
	SendMessage(g_restoreButtonControl, WM_SETFONT, (WPARAM)hObj, true);
	SendMessage(g_saveButtonControl, WM_SETFONT, (WPARAM)hObj, true);

	return true;
}

