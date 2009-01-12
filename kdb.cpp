#include <stdio.h>
#include "kdb.h"

#define MAXLINELEN 4096

#ifdef MYDLL_RELEASE_BUILD
#define KDB_DEBUG(f,x)
#define KDB_DEBUG_OPEN(f, dir)
#define KDB_DEBUG_CLOSE(f)
#else
#define KDB_DEBUG(f,x) if (f != NULL) fprintf x
#define KDB_DEBUG_OPEN(f, dir) {\
	char buf[MAXLINELEN]; \
	ZeroMemory(buf, MAXLINELEN); \
	lstrcpy(buf, dir); lstrcat(buf, "KDB.debug.log"); \
	f = fopen(buf, "wt"); \
}
#define KDB_DEBUG_CLOSE(f) if (f != NULL) { fclose(f); f = NULL; }
#endif

#define PLAYERS 0
#define GOALKEEPERS 1

static FILE* klog;

// functions
//////////////////////////////////////////

static KitEntry* buildKitList(int id, char* dir, int kitType);
static BOOL ParseColor(char* str, RGBAColor* color);
static BOOL ParseByte(char* str, BYTE* byte);

// Load attributes for kits of specific team
// params:
// (in) kdb - pointer to KDB
// (in) id  - team id.
void kdbLoadUniAttrib(KDB* kdb, int id)
{
	char attFileName[MAXFILENAME];
	ZeroMemory(attFileName, MAXFILENAME);
	lstrcpy(attFileName, kdb->dir); 
	sprintf(attFileName + lstrlen(attFileName), "KDB/uni/%03d/attrib.cfg", id);

	FILE* att = fopen(attFileName, "rt");
	if (att == NULL)
		return;  // no attrib.cfg file => nothing to do.

	KDB_DEBUG(klog, (klog, "Found %s\n", attFileName));

	BOOL isExtra = FALSE;
	Kit* kit = NULL;
	KitEntry* extraKitEntry = NULL;

	// go line by line
	while (!feof(att))
	{
		char buf[MAXLINELEN];
		ZeroMemory(buf, MAXLINELEN);
		fgets(buf, MAXLINELEN, att);
		if (lstrlen(buf) == 0) break;

		// strip off comments
		char* comm = strstr(buf, "#");
		if (comm != NULL) comm[0] = '\0';

		// look for start of the section
		char* start = strstr(buf, "[");
		if (start != NULL) 
		{
			char* end = strstr(start+1, "]");
			if (end != NULL)
			{
				// new section
				char name[MAXFILENAME];
				ZeroMemory(name, MAXFILENAME);
				lstrcpy(name, start+1);
				name[end-start-1] = '\0';

				KDB_DEBUG(klog, (klog, "section: {%s}\n", name));

				// check for special sections for default kits
				isExtra = FALSE;
				kit = NULL; extraKitEntry = NULL;

				if (lstrcmp(name, "texga.bmp")==0 || lstrcmp(name, "texga.bin")==0)
				{
					// 1st-choice gk
					kit = kdb->goalkeepers[id].a;
				}
				else if (lstrcmp(name, "texgb.bmp")==0 || lstrcmp(name, "texgb.bin")==0)
				{
					// 2nd-choice gk
					kit = kdb->goalkeepers[id].b;
				}
				else if (lstrcmp(name, "texpa.bmp")==0 || lstrcmp(name, "texpa.bin")==0)
				{
					// 1st-choice pl
					kit = kdb->players[id].a;
				}
				else if (lstrcmp(name, "texpb.bmp")==0 || lstrcmp(name, "texpb.bin")==0)
				{
					// 2nd-choice pl
					kit = kdb->players[id].b;
				}
				else if (strncmp(name, "gx/", 3)==0)
				{
					// extra gk kit
					// find matching kit among extras
					KitEntry* p = kdb->goalkeepers[id].extra;
					while (p != NULL)
					{
						if (lstrcmp(p->kit->fileName, name)==0)
						{
							kit = p->kit;
							break;
						}
						p = p->next;
					}
				}
				else if (strncmp(name, "px/", 3)==0)
				{
					// extra pl kit
					// find matching kit among extras
					KitEntry* p = kdb->players[id].extra;
					while (p != NULL)
					{
						if (lstrcmp(p->kit->fileName, name)==0)
						{
							kit = p->kit;
							break;
						}
						p = p->next;
					}
				}
			}
		}

		// Otherwise, look for attribute definitions
		else
		{
			if (kit == NULL)
				continue;  // unknown current kit, so nothing can be assigned.

			char key[80]; ZeroMemory(key, 80);
			char value[80]; ZeroMemory(value, 80);

			if (sscanf(buf,"%s = %s",key,value)==2)
			{
				KDB_DEBUG(klog, (klog, "key: {%s} has value: {%s}\n", key, value));

				RGBAColor color;
				if (lstrcmp(key, "shirt.name")==0)
				{
					if (ParseColor(value, &kit->shirtName))
						kit->attDefined |= SHIRT_NAME;
				}
				else if (lstrcmp(key, "shirt.number")==0)
				{
					if (ParseColor(value, &kit->shirtNumber))
						kit->attDefined |= SHIRT_NUMBER;
				}
				else if (lstrcmp(key, "shorts.number")==0)
				{
					if (ParseColor(value, &kit->shortsNumber))
						kit->attDefined |= SHORTS_NUMBER;
				}
				else if (lstrcmp(key, "collar")==0)
				{
					kit->collar = (lstrcmp(value,"yes")==0) ? 0 : 1;
					kit->attDefined |= COLLAR;
				}
				else if (lstrcmp(key, "cuff")==0)
				{
					kit->cuff = (lstrcmp(value,"yes")==0) ? 1 : 0;
					kit->attDefined |= CUFF;
				}
				else if (lstrcmp(key, "shorts.number.location")==0)
				{
					if (lstrcmp(value,"off")==0) {
						kit->shortsNumberLocation = 0;
						kit->attDefined |= SHORTS_NUMBER_LOCATION;
					}
					else if (lstrcmp(value,"left")==0) {
						kit->shortsNumberLocation = 1;
						kit->attDefined |= SHORTS_NUMBER_LOCATION;
					}
					else if (lstrcmp(value,"right")==0) {
						kit->shortsNumberLocation = 2;
						kit->attDefined |= SHORTS_NUMBER_LOCATION;
					}
					else if (lstrcmp(value,"both")==0) {
						kit->shortsNumberLocation = 3;
						kit->attDefined |= SHORTS_NUMBER_LOCATION;
					}
				}
				else if (lstrcmp(key, "number.type")==0)
				{
					if (ParseByte(value, &kit->numberType))
						kit->attDefined |= NUMBER_TYPE;
				}
				else if (lstrcmp(key, "name.type")==0)
				{
					if (ParseByte(value, &kit->nameType))
						kit->attDefined |= NAME_TYPE;
				}
				else if (lstrcmp(key, "name.shape")==0)
				{
					if (lstrcmp(value, "straight")==0) {
						kit->nameShape = 0;
						kit->attDefined |= NAME_SHAPE;
					}
					else if (lstrcmp(value, "curved")==0) {
						kit->nameShape = 1;
						kit->attDefined |= NAME_SHAPE;
					}
				}
				else if (lstrcmp(key, "model")==0)
				{
					if (ParseByte(value, &kit->model))
						kit->attDefined |= MODEL;
				}
			}
		}

		// go to next line
	}

	fclose(att);
}

// Load balls
// (in) kdb - pointer to KDB
void kdbLoadBalls(KDB* kdb)
{
	HANDLE heap = GetProcessHeap();

	char attFileName[MAXFILENAME];
	ZeroMemory(attFileName, MAXFILENAME);
	lstrcpy(attFileName, kdb->dir); 
	sprintf(attFileName + lstrlen(attFileName), "KDB/balls/attrib.cfg");

	FILE* att = fopen(attFileName, "rt");
	if (att == NULL)
		return;  // no attrib.cfg file => nothing to do.

	KDB_DEBUG(klog, (klog, "Found %s\n", attFileName));

	Ball* ball = NULL;
	BallEntry* ballEntry = NULL;

	// go line by line
	while (!feof(att))
	{
		char buf[MAXLINELEN];
		ZeroMemory(buf, MAXLINELEN);
		fgets(buf, MAXLINELEN, att);
		if (lstrlen(buf) == 0) break;

		// strip off comments
		char* comm = strstr(buf, "#");
		if (comm != NULL) comm[0] = '\0';

		// look for start of the section
		char* start = strstr(buf, "[");
		if (start != NULL) 
		{
			char* end = strstr(start+1, "]");
			if (end != NULL)
			{
				// new section
				char name[MAXFILENAME];
				ZeroMemory(name, MAXFILENAME);
				lstrcpy(name, start+1);
				name[end-start-1] = '\0';

				KDB_DEBUG(klog, (klog, "section: {%s}\n", name));

				// store previous ball in BallEntry
				if (ball != NULL)
				{
					kdb->ballsLast = (BallEntry*)HeapAlloc(heap, HEAP_ZERO_MEMORY, sizeof(BallEntry));
					kdb->ballsLast->ball = ball;
					kdb->ballsLast->next = ballEntry;
					kdb->ballCount++;

					ballEntry = kdb->ballsLast;
				}

				// set new current ball
				ball = (Ball*)HeapAlloc(heap, HEAP_ZERO_MEMORY, sizeof(Ball));
				lstrcpy(ball->name, name);
			}
		}

		// Otherwise, look for attribute definitions
		else
		{
			if (ball == NULL)
				continue;  // unknown current ball, so nothing can be assigned.

			char key[80]; ZeroMemory(key, 80);
			char value[80]; ZeroMemory(value, 80);

			if (sscanf(buf,"%s = %s",key,value)==2)
			{
				KDB_DEBUG(klog, (klog, "key: {%s} has value: {%s}\n", key, value));

				// strip off double quotes, if present
				if (value[0]=='"' && value[strlen(value)-1]=='"')
				{
					char copy[80]; ZeroMemory(copy, 80);
					memcpy(copy, value+1, strlen(value)-2);
					ZeroMemory(value, 80);
					lstrcpy(value, copy);
				}

				if (lstrcmp(key, "ball.model")==0)
				{
					lstrcpy(ball->mdlFileName, value);
				}
				else if (lstrcmp(key, "ball.texture")==0)
				{
					lstrcpy(ball->texFileName, value);

					// see if it's a BMP file
					char* ext = value + lstrlen(value) - 4;
					if (lstrcmpi(ext, ".bmp")==0)
						ball->isTexBmp = TRUE;
				}
			}
		}

		// go to next line
	}

	// store last ball in BallEntry
	if (ball != NULL)
	{
		kdb->ballsLast = (BallEntry*)HeapAlloc(heap, HEAP_ZERO_MEMORY, sizeof(BallEntry));
		kdb->ballsLast->ball = ball;
		kdb->ballsLast->next = ballEntry;
		kdb->ballCount++;

		ballEntry = kdb->ballsLast;
	}

	// reverse the ball list so that balls appear in the 
	// same order as they are configured in attrib.cfg file
	// Also, initialize the "prev" fields so that we have
	// a proper double-linked list
	BallEntry* bp = kdb->ballsLast;
	kdb->ballsFirst = NULL;

	while (bp != NULL)
	{
		BallEntry* bq = bp->next;
		BallEntry* newb = (BallEntry*)HeapAlloc(heap, HEAP_ZERO_MEMORY, sizeof(BallEntry));
		newb->ball = bp->ball;
		// forward link
		newb->next = kdb->ballsFirst;
		// backward link for previos item
		if (kdb->ballsFirst != NULL)
			kdb->ballsFirst->prev = newb;
		else
			kdb->ballsLast = newb;

		kdb->ballsFirst = newb;
		HeapFree(heap, 0, bp);
		bp = bq;
	}

	fclose(att);
}

KDB* kdbLoad(char* dir)
{
	KDB_DEBUG_OPEN(klog, dir);
	KDB_DEBUG(klog, (klog, "Loading KDB...\n"));

	HANDLE heap = GetProcessHeap();

	// initialize an empty database
	KDB* kdb = (KDB*)HeapAlloc(heap, HEAP_ZERO_MEMORY, sizeof(KDB));
	if (kdb == NULL) { fclose(klog); return NULL; }
	strncpy(kdb->dir, dir, MAXFILENAME);
	kdb->playerCount = kdb->goalkeeperCount = kdb->ballCount = 0;
	ZeroMemory(kdb->players, sizeof(Kits) * 256);
	ZeroMemory(kdb->goalkeepers, sizeof(Kits) * 256);
	kdb->ballsFirst = NULL;
	kdb->ballsLast = NULL;

	// Kits
	for (int n=0; n<256; n++)
	{
		// create Kit structures for default goalkeeper kits
		kdb->goalkeepers[n].a = (Kit*)HeapAlloc(heap, HEAP_ZERO_MEMORY, sizeof(Kit));
		if (kdb->goalkeepers[n].a == NULL)
		{
			KDB_DEBUG(klog, (klog, "memory allocation problem\n"));
		}
		else
			lstrcpy(kdb->goalkeepers[n].a->fileName, "texga.bmp");
		kdb->goalkeepers[n].b = (Kit*)HeapAlloc(heap, HEAP_ZERO_MEMORY, sizeof(Kit));
		if (kdb->goalkeepers[n].b == NULL)
		{
			KDB_DEBUG(klog, (klog, "memory allocation problem\n"));
		}
		else
			lstrcpy(kdb->goalkeepers[n].b->fileName, "texgb.bmp");

		// create Kit structures for default player kits
		kdb->players[n].a = (Kit*)HeapAlloc(heap, HEAP_ZERO_MEMORY, sizeof(Kit));
		if (kdb->players[n].a == NULL)
		{
			KDB_DEBUG(klog, (klog, "memory allocation problem\n"));
		}
		else
			lstrcpy(kdb->players[n].a->fileName, "texpa.bmp");
		kdb->players[n].b = (Kit*)HeapAlloc(heap, HEAP_ZERO_MEMORY, sizeof(Kit));
		if (kdb->players[n].b == NULL)
		{
			KDB_DEBUG(klog, (klog, "memory allocation problem\n"));
		}
		else
			lstrcpy(kdb->players[n].b->fileName, "texpb.bmp");

		// create list of gx kits
		KitEntry* gx = buildKitList(n, dir, GOALKEEPERS);

		// traverse the list and place the kits into the database
		KitEntry* p = gx;
		KitEntry* q = NULL;
		while (p != NULL)
		{
			q = p->next;
			KDB_DEBUG(klog, (klog, "p->kit->filename = %s\n", p->kit->fileName));

			p->next = kdb->goalkeepers[n].extra;
			kdb->goalkeepers[n].extra = p;
			kdb->goalkeeperCount += 1;

			KDB_DEBUG(klog, (klog, "[%03d] : %s\n", n, p->kit->fileName));
			p = q;
		}

		// create list of px kits
		KitEntry* px = buildKitList(n, dir, PLAYERS);

		// traverse the list and place the kits into the database
		p = px;
		q = NULL;
		while (p != NULL)
		{
			q = p->next;
			KDB_DEBUG(klog, (klog, "p->kit->filename = %s\n", p->kit->fileName));

			p->next = kdb->players[n].extra;
			kdb->players[n].extra = p;
			kdb->playerCount += 1;

			KDB_DEBUG(klog, (klog, "[%03d] : %s\n", n, p->kit->fileName));
			p = q;
		}

		// read the attributes from attrib.cfg file
		kdbLoadUniAttrib(kdb, n);

		// place 2 GK kits into "extra" list to allow for explicit switching
		KitEntry* aKitEntry = (KitEntry*)HeapAlloc(heap, HEAP_ZERO_MEMORY, sizeof(KitEntry));
		if (aKitEntry == NULL)
		{
			KDB_DEBUG(klog, (klog, "memory allocation problem\n"));
		}
		else
		{
			aKitEntry->kit = kdb->goalkeepers[n].a;
			if (n == 78)
			{
				KDB_DEBUG(klog, (klog, "Newcastle GK: ke->kit = %08x\n", (DWORD)aKitEntry->kit));
			}
		}
		KitEntry* bKitEntry = (KitEntry*)HeapAlloc(heap, HEAP_ZERO_MEMORY, sizeof(KitEntry));
		if (bKitEntry == NULL)
		{
			KDB_DEBUG(klog, (klog, "memory allocation problem\n"));
		}
		else
		{
			bKitEntry->kit = kdb->goalkeepers[n].b;
			if (n == 78)
			{
				KDB_DEBUG(klog, (klog, "Newcastle GK: ke->kit = %08x\n", (DWORD)bKitEntry->kit));
			}
		}
		bKitEntry->next = kdb->goalkeepers[n].extra;
		aKitEntry->next = bKitEntry;
		kdb->goalkeepers[n].extra = aKitEntry;

		if (n == 78) 
		{
			KDB_DEBUG(klog, (klog, "Newcastle goalkeepers:\n"));
			KitEntry* ke = kdb->goalkeepers[n].extra;
			while (ke != NULL)
			{
				KDB_DEBUG(klog, (klog, "ke->kit = %08x\n", (DWORD)ke->kit));
				ke = ke->next;
			}
		}
	}

	// Balls
	kdbLoadBalls(kdb);

	KDB_DEBUG(klog, (klog, "KDB loaded.\n"));
	KDB_DEBUG_CLOSE(klog);
	return kdb;
}

// parses a RRGGBB string into RGBAColor structure
BOOL ParseColor(char* str, RGBAColor* color)
{
	int len = lstrlen(str);
	if (!(len == 6 || len == 8)) 
		return FALSE;

	int num = 0;
	if (sscanf(str,"%x",&num)!=1) return FALSE;

	if (len == 6) {
		// assume alpha as fully opaque.
		color->r = (BYTE)((num >> 16) & 0xff);
		color->g = (BYTE)((num >> 8) & 0xff);
		color->b = (BYTE)(num & 0xff);
		color->a = 0xff; // set alpha to opaque
	}
	else {
		color->r = (BYTE)((num >> 24) & 0xff);
		color->g = (BYTE)((num >> 16) & 0xff);
		color->b = (BYTE)((num >> 8) & 0xff);
		color->a = (BYTE)(num & 0xff);
	}

	KDB_DEBUG(klog, (klog, "RGBA color: %02x,%02x,%02x,%02x\n",
				color->r, color->g, color->b, color->a));
	return TRUE;
}

// parses a decimal number string into actual BYTE value
BOOL ParseByte(char* str, BYTE* byte)
{
	int num = 0;
	if (sscanf(str,"%d",&num)!=1) return FALSE;
	*byte = (BYTE)num;
	return TRUE;
}

KitEntry* buildKitList(int id, char* dir, int kitType)
{
	WIN32_FIND_DATA fData;
	char pattern[MAXLINELEN];
	ZeroMemory(pattern, MAXLINELEN);

	KitEntry* list = NULL;
	KitEntry* p = NULL;
	HANDLE heap = GetProcessHeap();

	lstrcpy(pattern, dir); 
	if (kitType == PLAYERS) 
		sprintf(pattern + lstrlen(pattern), "KDB/uni/%03d/px/*.bmp", id);
	else if (kitType == GOALKEEPERS) 
		sprintf(pattern + lstrlen(pattern), "KDB/uni/%03d/gx/*.bmp", id);

	//KDB_DEBUG(klog, (klog, "pattern = {%s}\n", pattern));

	HANDLE hff = FindFirstFile(pattern, &fData);
	if (hff == INVALID_HANDLE_VALUE) 
	{
		// none found.
		return NULL;
	}

	while(true)
	{
		// build new list element
		Kit* kit = (Kit*)HeapAlloc(heap, HEAP_ZERO_MEMORY, sizeof(Kit));
		if (kit != NULL)
		{
			ZeroMemory(kit->fileName, MAXFILENAME);
			lstrcpy(kit->fileName, (kitType == PLAYERS) ? "px/" : "gx/");
			lstrcat(kit->fileName, fData.cFileName);

			ZeroMemory(&kit->shirtName, sizeof(RGBAColor));
			ZeroMemory(&kit->shirtNumber, sizeof(RGBAColor));
			ZeroMemory(&kit->shortsNumber, sizeof(RGBAColor));
			kit->attDefined = 0;

			p = (KitEntry*)HeapAlloc(heap, HEAP_ZERO_MEMORY, sizeof(KitEntry));
			if (p != NULL)
			{
				// insert new element
				p->kit = kit;
				p->next = list;
				list = p;

				KDB_DEBUG(klog, (klog, "list->kit->fileName = {%s}\n", list->kit->fileName));
			}
			else 
			{
				HeapFree(heap, 0, kit);
			}
		}

		// proceed to next file
		if (!FindNextFile(hff, &fData)) break;
	}

	FindClose(hff);
	return list;
}

void kdbUnload(KDB* kdb)
{
	if (kdb == NULL) return;
	HANDLE heap = GetProcessHeap();
	KitEntry* q = NULL;
	BallEntry* bq = NULL;

	// free player and goalkeeper kits memory
	for (int i=0; i<256; i++)
	{
		// default kits
		if (kdb->goalkeepers[i].a != NULL)
			HeapFree(heap, 0, kdb->goalkeepers[i].a);
		if (kdb->goalkeepers[i].b != NULL)
			HeapFree(heap, 0, kdb->goalkeepers[i].b);
		if (kdb->players[i].a != NULL)
			HeapFree(heap, 0, kdb->players[i].a);
		if (kdb->players[i].b != NULL)
			HeapFree(heap, 0, kdb->players[i].b);

		// extra PLAYERS
		KitEntry* p = kdb->players[i].extra;
		while (p != NULL)
		{
			q = p->next;
			if (p->kit != NULL) HeapFree(heap, 0, p->kit);
			HeapFree(heap, 0, p);
			p = q;
		}

		// extra GOALKEEPERS
		int count = 0;
		p = kdb->goalkeepers[i].extra;
		while (p != NULL)
		{
			q = p->next;
			if (count++ >= 2) HeapFree(heap, 0, p->kit); // first 2 are already freed earlier
			HeapFree(heap, 0, p);
			p = q;
		}
	}

	// free balls memory
	BallEntry* be = kdb->ballsFirst;
	while (be != NULL)
	{
		bq = be->next;
		HeapFree(heap, 0, be->ball);
		HeapFree(heap, 0, be);
		be = bq;
	}
}

