// config.cpp

#include <stdio.h>
#include <string.h>

#include "config.h"
#include "log.h"

/**
 * Returns true if successful.
 */
BOOL ReadConfig(KSERV_CONFIG* config, char* cfgFile)
{
	if (config == NULL) return false;

	bool hasKdbDir = false;
	FILE* cfg = fopen(cfgFile, "rt");
	if (cfg == NULL) return false;

	char str[BUFLEN];
	char name[BUFLEN];
	int value = 0;
	float dvalue = 0.0f;

	char *pName = NULL, *pValue = NULL, *comment = NULL;
	while (true)
	{
		ZeroMemory(str, BUFLEN);
		fgets(str, BUFLEN-1, cfg);
		if (feof(cfg)) break;

		// skip comments
		comment = strstr(str, "#");
		if (comment != NULL) comment[0] = '\0';

		// parse the line
		pName = pValue = NULL;
		ZeroMemory(name, BUFLEN); value = 0;
		char* eq = strstr(str, "=");
		if (eq == NULL || eq[1] == '\0') continue;

		eq[0] = '\0';
		pName = str; pValue = eq + 1;

		ZeroMemory(name, NULL); 
		sscanf(pName, "%s", name);

		if (lstrcmp(name, "debug")==0)
		{
			if (sscanf(pValue, "%d", &value)!=1) continue;
			LogWithNumber("ReadConfig: debug = (%d)\n", value);
			config->debug = value;
		}
		else if (lstrcmp(name, "kdb.dir")==0)
		{
			char* startQuote = strstr(pValue, "\"");
			if (startQuote == NULL) continue;
			char* endQuote = strstr(startQuote + 1, "\"");
			if (endQuote == NULL) continue;

			char buf[BUFLEN];
			ZeroMemory(buf, BUFLEN);
			memcpy(buf, startQuote + 1, endQuote - startQuote - 1);

			LogWithString("ReadConfig: kdbDir = \"%s\"\n", buf);
			ZeroMemory(config->kdbDir, BUFLEN);
			strncpy(config->kdbDir, buf, BUFLEN-1);

			hasKdbDir = true;
		}
		else if (lstrcmp(name, "vKey.homeKit")==0)
		{
			if (sscanf(pValue, "%x", &value)!=1) continue;
			LogWithNumber("ReadConfig: vKeyHomeKit = 0x%02x\n", value);
			config->vKeyHomeKit = value;
		}
		else if (lstrcmp(name, "vKey.awayKit")==0)
		{
			if (sscanf(pValue, "%x", &value)!=1) continue;
			LogWithNumber("ReadConfig: vKeyAwayKit = 0x%02x\n", value);
			config->vKeyAwayKit = value;
		}
		else if (lstrcmp(name, "vKey.gkHomeKit")==0)
		{
			if (sscanf(pValue, "%x", &value)!=1) continue;
			LogWithNumber("ReadConfig: vKeyGKHomeKit = 0x%02x\n", value);
			config->vKeyGKHomeKit = value;
		}
		else if (lstrcmp(name, "vKey.gkAwayKit")==0)
		{
			if (sscanf(pValue, "%x", &value)!=1) continue;
			LogWithNumber("ReadConfig: vKeyGKAwayKit = 0x%02x\n", value);
			config->vKeyGKAwayKit = value;
		}
		else if (lstrcmp(name, "vKey.prevBall")==0)
		{
			if (sscanf(pValue, "%x", &value)!=1) continue;
			LogWithNumber("ReadConfig: vKeyPrevBall = 0x%02x\n", value);
			config->vKeyPrevBall = value;
		}
		else if (lstrcmp(name, "vKey.nextBall")==0)
		{
			if (sscanf(pValue, "%x", &value)!=1) continue;
			LogWithNumber("ReadConfig: vKeyNextBall = 0x%02x\n", value);
			config->vKeyNextBall = value;
		}
		else if (lstrcmp(name, "vKey.randomBall")==0)
		{
			if (sscanf(pValue, "%x", &value)!=1) continue;
			LogWithNumber("ReadConfig: vKeyRandomBall = 0x%02x\n", value);
			config->vKeyRandomBall = value;
		}
		else if (lstrcmp(name, "vKey.resetBall")==0)
		{
			if (sscanf(pValue, "%x", &value)!=1) continue;
			LogWithNumber("ReadConfig: vKeyResetBall = 0x%02x\n", value);
			config->vKeyResetBall = value;
		}
		else if (lstrcmp(name, "kit.useLargeTexture")==0)
		{
			if (sscanf(pValue, "%d", &value)!=1) continue;
			LogWithNumber("ReadConfig: useLargeTexture = %d\n", value);
			config->useLargeTexture = value;
		}
	}
	fclose(cfg);

	// if KDB directory is not specified, assume default
	if (!hasKdbDir)
	{
		strcpy(config->kdbDir, DEFAULT_KDB_DIR);
	}

	return true;
}

/**
 * Returns true if successful.
 */
BOOL WriteConfig(KSERV_CONFIG* config, char* cfgFile)
{
	char* buf = NULL;
	DWORD size = 0;

	// first read all lines
	HANDLE oldCfg = CreateFile(cfgFile, GENERIC_READ, 0, NULL,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (oldCfg != INVALID_HANDLE_VALUE)
	{
		size = GetFileSize(oldCfg, NULL);
		buf = (char*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
		if (buf == NULL) return false;

		DWORD dwBytesRead = 0;
		ReadFile(oldCfg, buf, size, &dwBytesRead, NULL);
		if (dwBytesRead != size) 
		{
			HeapFree(GetProcessHeap(), 0, buf);
			return false;
		}
		CloseHandle(oldCfg);
	}

	// create new file
	FILE* cfg = fopen(cfgFile, "wt");

	// loop over every line from the old file, and overwrite it in the new file
	// if necessary. Otherwise - copy the old line.
	
	BOOL bWrittenKdbDir = false; 
	BOOL bWrittenVKeyHomeKit = false; BOOL bWrittenVKeyAwayKit = false;
	BOOL bWrittenVKeyGKHomeKit = false; BOOL bWrittenVKeyGKAwayKit = false;
	BOOL bWrittenVKeyPrevBall = false;
	BOOL bWrittenVKeyNextBall = false;
	BOOL bWrittenVKeyRandomBall = false;
	BOOL bWrittenVKeyResetBall = false;
	BOOL bWrittenUseLargeTexture = false;

	char* line = buf; BOOL done = false;
	char* comment = NULL;
	if (buf != NULL) while (!done && line < buf + size)
	{
		char* endline = strstr(line, "\r\n");
		if (endline != NULL) endline[0] = '\0'; else done = true;
		char* comment = strstr(line, "#");
		char* setting = NULL;

		if ((setting = strstr(line, "kdb.dir")) && 
			(comment == NULL || setting < comment))
		{
			fprintf(cfg, "kdb.dir = \"%s\"\n", config->kdbDir);
			bWrittenKdbDir = true;
		}
		else if ((setting = strstr(line, "vKey.homeKit")) && 
				 (comment == NULL || setting < comment))
		{
			fprintf(cfg, "vKey.homeKit = 0x%02x\n", config->vKeyHomeKit);
			bWrittenVKeyHomeKit = true;
		}
		else if ((setting = strstr(line, "vKey.awayKit")) && 
				 (comment == NULL || setting < comment))
		{
			fprintf(cfg, "vKey.awayKit = 0x%02x\n", config->vKeyAwayKit);
			bWrittenVKeyAwayKit = true;
		}
		else if ((setting = strstr(line, "vKey.gkHomeKit")) && 
				 (comment == NULL || setting < comment))
		{
			fprintf(cfg, "vKey.gkHomeKit = 0x%02x\n", config->vKeyGKHomeKit);
			bWrittenVKeyGKHomeKit = true;
		}
		else if ((setting = strstr(line, "vKey.gkAwayKit")) && 
				 (comment == NULL || setting < comment))
		{
			fprintf(cfg, "vKey.gkAwayKit = 0x%02x\n", config->vKeyGKAwayKit);
			bWrittenVKeyGKAwayKit = true;
		}
		else if ((setting = strstr(line, "vKey.prevBall")) && 
				 (comment == NULL || setting < comment))
		{
			fprintf(cfg, "vKey.prevBall = 0x%02x\n", config->vKeyPrevBall);
			bWrittenVKeyPrevBall = true;
		}
		else if ((setting = strstr(line, "vKey.nextBall")) && 
				 (comment == NULL || setting < comment))
		{
			fprintf(cfg, "vKey.nextBall = 0x%02x\n", config->vKeyNextBall);
			bWrittenVKeyNextBall = true;
		}
		else if ((setting = strstr(line, "vKey.randomBall")) && 
				 (comment == NULL || setting < comment))
		{
			fprintf(cfg, "vKey.randomBall = 0x%02x\n", config->vKeyRandomBall);
			bWrittenVKeyRandomBall = true;
		}
		else if ((setting = strstr(line, "vKey.resetBall")) && 
				 (comment == NULL || setting < comment))
		{
			fprintf(cfg, "vKey.resetBall = 0x%02x\n", config->vKeyResetBall);
			bWrittenVKeyResetBall = true;
		}
		else if ((setting = strstr(line, "kit.useLargeTexture")) && 
				 (comment == NULL || setting < comment))
		{
			fprintf(cfg, "kit.useLargeTexture = %d\n", config->useLargeTexture);
			bWrittenUseLargeTexture = true;
		}

		else
		{
			// take the old line
			fprintf(cfg, "%s\n", line);
		}

		if (endline != NULL) { endline[0] = '\r'; line = endline + 2; }
	}

	// if something wasn't written, make sure we write it.
	if (!bWrittenKdbDir) 
		fprintf(cfg, "kdb.dir = \"%s\"\n", config->kdbDir);
	if (!bWrittenVKeyHomeKit)
		fprintf(cfg, "vKey.homeKit = 0x%02x\n", config->vKeyHomeKit);
	if (!bWrittenVKeyAwayKit)
		fprintf(cfg, "vKey.awayKit = 0x%02x\n", config->vKeyAwayKit);
	if (!bWrittenVKeyGKHomeKit)
		fprintf(cfg, "vKey.gkHomeKit = 0x%02x\n", config->vKeyGKHomeKit);
	if (!bWrittenVKeyGKAwayKit)
		fprintf(cfg, "vKey.gkAwayKit = 0x%02x\n", config->vKeyGKAwayKit);
	if (!bWrittenVKeyPrevBall)
		fprintf(cfg, "vKey.prevBall = 0x%02x\n", config->vKeyPrevBall);
	if (!bWrittenVKeyNextBall)
		fprintf(cfg, "vKey.nextBall = 0x%02x\n", config->vKeyNextBall);
	if (!bWrittenVKeyRandomBall)
		fprintf(cfg, "vKey.randomBall = 0x%02x\n", config->vKeyRandomBall);
	if (!bWrittenVKeyResetBall)
		fprintf(cfg, "vKey.resetBall = 0x%02x\n", config->vKeyResetBall);

	// release buffer
	HeapFree(GetProcessHeap(), 0, buf);

	fclose(cfg);

	return true;
}

