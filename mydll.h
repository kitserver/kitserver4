#ifndef _DEFINED_MYDLL
#define _DEFINED_MYDLL

#include "afsreader.h"
#include "kdb.h"

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef _COMPILING_MYDLL
#define LIBSPEC __declspec(dllexport)
#else
#define LIBSPEC __declspec(dllimport)
#endif /* _COMPILING_MYDLL */

#define BUFLEN 4096  /* 4K buffer length */
#define MAX_FILEPATH BUFLEN

#define WM_APP_KEYDEF WM_APP + 1

#define VKEY_HOMEKIT 0
#define VKEY_AWAYKIT 1
#define VKEY_GKHOMEKIT 2
#define VKEY_GKAWAYKIT 3
#define VKEY_PREV_BALL 4
#define VKEY_NEXT_BALL 5
#define VKEY_RANDOM_BALL 6
#define VKEY_RESET_BALL 7

LIBSPEC LRESULT CALLBACK KeyboardProc(int code, WPARAM wParam, LPARAM lParam);
LIBSPEC void RestoreDeviceMethods(void);

typedef struct _ENCBUFFERHEADER {
	DWORD dwSig;
	DWORD dwEncSize;
	DWORD dwDecSize;
	BYTE other[20];
} ENCBUFFERHEADER;

typedef struct _DECBUFFERHEADER {
	DWORD dwSig;
	DWORD numTexs;
	DWORD dwDecSize;
	BYTE reserved1[4];
	WORD bitsOffset;
	WORD paletteOffset;
	WORD texWidth;
	WORD texHeight;
	BYTE reserved2[2];
	BYTE bitsPerPixel;
	BYTE reserver3[13];
	WORD blockWidth;
	WORD blockHeight;
	BYTE other[84];
} DECBUFFERHEADER;

#define AFS_FILENAME_LEN 32
typedef struct _TEAMBUFFERINFO {
	char gaFile[AFS_FILENAME_LEN];
    AFSITEMINFO ga;
	char gbFile[AFS_FILENAME_LEN];
	AFSITEMINFO gb;
	char paFile[AFS_FILENAME_LEN];
    AFSITEMINFO pa;
	char pbFile[AFS_FILENAME_LEN];
    AFSITEMINFO pb;
	char vgFile[AFS_FILENAME_LEN];
    AFSITEMINFO vg;
} TEAMBUFFERINFO;

// Attributes for player/goalkeeper record
typedef struct _KitAttr {
	BYTE model;
	BYTE cuffGK;
	BYTE cuffPL;
	BYTE collarGK;
	BYTE collarPL;
	BYTE shortsNumberLocationGK;
	BYTE shortsNumberLocationPL;
} KitAttr;

// Attributes record (one per team)
typedef struct _TeamAttr {
	BYTE numberType; // 0|1|2|3
	BYTE nameType; // 0|1
	BYTE nameShape; // 0|1 (0-straight, 1-curved)
	KitAttr home;
	KitAttr away;
} TeamAttr;

typedef struct _KitSlot {
	BYTE model;
	BYTE _unknown1;
	BYTE shortsNumberLocation;
	BYTE numberType;
	BYTE nameType;
	BYTE nameShape;
	BYTE _unknown2;
	BYTE isEdited;
	BYTE _unknown3;
	BYTE collar;
	WORD kitId;
	BYTE miscEditedData[18];
	RGBAColor shirtNameColor;
	RGBAColor shortsNumberColor;
	RGBAColor shirtNumberColor;
} KitSlot;

typedef struct _BigKitSlot {
	KitSlot kitSlot;
	BYTE _unknown[14];
} BigKitSlot;

/* ... more declarations as needed */
#undef LIBSPEC

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _DEFINED_MYDLL */

