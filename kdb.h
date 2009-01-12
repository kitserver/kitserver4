#ifndef __JUCE_KDB__
#define __JUCE_KDB__

#include <windows.h>

#define MAXFILENAME 512

// attribute definition flags (bits)
#define SHIRT_NUMBER  0x01 
#define SHIRT_NAME    0x02 
#define SHORTS_NUMBER 0x04 
#define COLLAR        0x08 
#define MODEL         0x10 
#define CUFF          0x20 
#define SHORTS_NUMBER_LOCATION 0x40 
#define NUMBER_TYPE   0x80
#define NAME_TYPE     0x100
#define NAME_SHAPE    0x200

// KDB data structures
///////////////////////////////

typedef struct _RGBAColor {
	BYTE r;
	BYTE g;
	BYTE b;
	BYTE a;
} RGBAColor;

typedef struct _Kit {
	char fileName[MAXFILENAME];
	RGBAColor shirtNumber;
	RGBAColor shirtName;
	RGBAColor shortsNumber;
	BYTE collar;
	BYTE model;
	BYTE cuff;
	BYTE shortsNumberLocation;
	BYTE numberType;
	BYTE nameType;
	BYTE nameShape;
	WORD attDefined;
} Kit;

typedef struct _Ball {
	char name[MAXFILENAME];
	char texFileName[MAXFILENAME];
	char mdlFileName[MAXFILENAME];
	BOOL isTexBmp;
} Ball;

typedef struct _KitEntry {
	Kit* kit;
	struct _KitEntry* next;
} KitEntry;

typedef struct _Kits {
	Kit* a;
	Kit* b;
	KitEntry* extra;
} Kits;

typedef struct _BallEntry {
	Ball* ball;
	struct _BallEntry* prev;
	struct _BallEntry* next;
} BallEntry;

typedef struct _KDB {
	char dir[MAXFILENAME];
	int playerCount;
	int goalkeeperCount;
	Kits players[256];
	Kits goalkeepers[256];
	int ballCount;
	BallEntry* ballsFirst;
	BallEntry* ballsLast;
} KDB;

// KDB functions
//////////////////////////////

KDB* kdbLoad(char* dir);
void kdbUnload(KDB* kdb);
void kdbLoadAttrib(KDB* kdb, int id);

#endif
