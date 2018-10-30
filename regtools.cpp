// regtools.cpp

#define BUFLEN 4096

#include <stdio.h>
#include <string.h>
#include "regtools.h"

enum { GK_PES3, GK_WE7U, GK_WE7K };
char GAME_KEYS[3] = { "PES3", "WE7U", "WE7K" };

// This functions looks up the game in the Registry.
// Returns the installation directory for a specified game key
BOOL GetInstallDirFromReg(char* gameKey, char* installDir)
{
	HKEY hKey = NULL;
	DWORD bc = BUFLEN;
	BOOL res = false;
	
	char buf[BUFLEN];
	ZeroMemory(buf, BUFLEN);
	lstrcpy(buf, "SOFTWARE\\KONAMI\\");
	lstrcat(buf, gameKey);

	// first look for PES3, if found take it
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
			buf, 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
	{
		if (RegQueryValueEx(hKey, "installdir", NULL, NULL, 
					(LPBYTE)installDir, &bc) == ERROR_SUCCESS)
		{
			res = true;
		}
		RegCloseKey(hKey);
	}
	return res;
}

