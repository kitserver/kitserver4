#ifndef _JUCE_CONFIG
#define _JUCE_CONFIG

#include <windows.h>

#define BUFLEN 4096

#define CONFIG_FILE "kserv.cfg"

#define DEFAULT_DEBUG 0
#define DEFAULT_KDB_DIR ".\\"
#define DEFAULT_VKEY_HOMEKIT 0x31
#define DEFAULT_VKEY_AWAYKIT 0x32
#define DEFAULT_VKEY_GKHOMEKIT 0x33
#define DEFAULT_VKEY_GKAWAYKIT 0x34
#define DEFAULT_VKEY_NEXT_BALL 0x42
#define DEFAULT_VKEY_PREV_BALL 0x44
#define DEFAULT_VKEY_RANDOM_BALL 0x52
#define DEFAULT_VKEY_RESET_BALL 0x43
#define DEFAULT_USE_LARGE_TEXTURE TRUE
#define DEFAULT_ASPECT_RATIO -1.0f
#define DEFAULT_GAME_SPEED 1.0f
#define DEFAULT_INTRES_WIDTH 0
#define DEFAULT_INTRES_HEIGHT 0
#define DEFAULT_FULLSCREEN_WIDTH 0
#define DEFAULT_FULLSCREEN_HEIGHT 0
#define DEFAULT_RESERVED_MEMORY 0
#define DEFAULT_LOD -1

typedef struct _KSERV_CONFIG_STRUCT {
	DWORD  debug;
	char   kdbDir[BUFLEN];
	WORD   vKeyHomeKit;
	WORD   vKeyAwayKit;
	WORD   vKeyGKHomeKit;
	WORD   vKeyGKAwayKit;
	WORD   vKeyPrevBall;
	WORD   vKeyNextBall;
	WORD   vKeyRandomBall;
	WORD   vKeyResetBall;
	BOOL   useLargeTexture;
    float  aspectRatio;
    float  gameSpeed;
    DWORD  internalResolutionWidth;
    DWORD  internalResolutionHeight;
    DWORD  fullscreenWidth;
    DWORD  fullscreenHeight;
	DWORD  newResMem;
	int    lod1;
	int    lod2;
	int    lod3;
	int    lod4;
	int    lod5;

} KSERV_CONFIG;

BOOL ReadConfig(KSERV_CONFIG* config, char* cfgFile);
BOOL WriteConfig(KSERV_CONFIG* config, char* cfgFile);

#endif
