/* mydll */

/**************************************
 * Visual indicators meaning:         *
 * BLUE STAR: KitServer active        *
 **************************************/

#include <windows.h>
#define _COMPILING_MYDLL
#include <stdio.h>
#include <limits.h>
#include <math.h>

#include "mydll.h"
#include "shared.h"
#include "config.h"
#include "imageutil.h"
#include "kdb.h"
#include "log.h"
#include "crc32.h"
#include "afsreader.h"

#include "d3dfont.h"
#include "dxutil.h"
#include "d3d8types.h"
#include "d3d8.h"
#include "d3dx8tex.h"

#include "apihijack.h"
#include <list>

#define SAFEFREE(ptr) if (ptr) { HeapFree(GetProcessHeap(),0,ptr); ptr=NULL; }
#define IS_CUSTOM_TEAM(id) (id == 0xffff) 

#define MAX_ITERATIONS 1000

/**************************************
Shared by all processes variables
**************************************/
#pragma data_seg(".HKT")
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
    DEFAULT_ASPECT_RATIO,
    DEFAULT_GAME_SPEED,
};
#pragma data_seg()
#pragma comment(linker, "/section:.HKT,rws")

/************************** 
End of shared section 
**************************/

bool _aspectRatioSet = false;
bool g_fontInitialized = false;
CD3DFont* g_font = NULL;

extern char* GAME[6];
extern char* GAME_GUID[5];
extern DWORD GAME_GUID_OFFSETS[5];

#define CODELEN 23
#define DATALEN 11

char* WHICH_TEAM[] = { "HOME", "AWAY" };

// code array names
// NOTE: "_CS" stands for: "call site"
enum {
	C_UNIDECODE, C_UNIDECODE_CS, C_UNIDECODE_CS2, 
	C_UNIDECRYPT, C_UNIDECRYPT_CS, C_SETFILEPOINTER_CS,
	C_UNPACK, C_UNPACK_CS, C_SETKITATTRIBUTES,
	C_SETKITATTRIBUTES_CS1, C_SETKITATTRIBUTES_CS2,
	C_SETKITATTRIBUTES_CS3, C_SETKITATTRIBUTES_CS4,
	C_SETKITATTRIBUTES_CS5, C_SETKITATTRIBUTES_CS6,
	C_SETKITATTRIBUTES_CS7, C_SETKITATTRIBUTES_CS8,
	C_FINISHKITPICK, C_FINISHKITPICK_CS,
	C_ALLOCMEM, C_ALLOCMEM_CS,
	C_RESETFLAGS, C_RESETFLAGS_CS,
};

// data array names
enum {
	TEAM_IDS, TEAM_STRIPS, IDIRECT3DDEVICE8, NUMTEAMS, KIT_SLOTS, 
	ANOTHER_KIT_SLOTS, MLDATA_PTRS, TEAM_COLLARS_PTR,
	KIT_CHECK_TRIGGER,
    PROJ_W, CULL_W,
};

// Code addresses. Order: PES4 DEMO 2, PES4 DEMO, PES4 1.10, PES4 1.0
DWORD codeArray[5][CODELEN] = { 
	// PES4 DEMO 2
	{ 0x72f520, 0x6d29ba, 0x6d2a1c, 0x4a2c20, 0x6d29b4, 0x418116, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0x45d580, 0x4748f4},
	// PES4 DEMO
	{ 0x72f1a0, 0x6d2a2a, 0x6d2a8c, 0x4a2d40, 0x6d2a24, 0x418156, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0x45d6a0, 0x47a414},
	// PES4 1.10
	{ 0x92a430, 0x8cdfea, 0x8ce04c, 0, 0, 0x41d7f6, 0x92a3f0, 0x69059b, 0x671c20,
	  0x981743, 0x98174a, 0x5fe68e, 0x5fe6cc, 0x48ea1b, 0x48ea21, 0x93a476, 0x93a47d,
	  0x5f30c0, 0x5fd87a, 0x68ff50, 0x690589, 0x5fb320, 0x60a9f8},
	// PES4 1.0
	{ 0x929370, 0x8ccfda, 0x8cd03c, 0, 0, 0x41d6d6, 0x929330, 0x6902fb, 0x671990,
	  0x980663, 0x98066a, 0x5fe48e, 0x5fe4cc, 0x48e82b, 0x48e831, 0x9393b6, 0x9393bd,
	  0x5f2ec0, 0x5fd67a, 0x68fce0, 0x6902e9, 0x5fb120, 0x60a7b8},
	// WE8I US
	{ 0x92b470, 0x8cef0a, 0x8cef6c, 0, 0, 0x41d7e6, 0x92b430, 0x690cab, 0x672490,
	  0x982783, 0x98278a, 0x5fee8e, 0x5feecc, 0x48e9eb, 0x48e9f1, 0x93b4c6, 0x93b4cd,
	  0x5f38d0, 0x5fe07a, 0x690690, 0x690c99, 0x5fbb20, 0x60b228},
};

// Data addresses. Order: PES4 DEMO 2, PES4 DEMO, PES4 1.10, PES4 1.0
DWORD dataArray[5][DATALEN] = {
	// PES4 DEMO 2
	{ 0x48f69e0, 0x48f7982, 0x1e2f370, 202, 0,
	  0, 0, 0, 0,
      0, 0 },
	// PES4 DEMO
	{ 0x48c7a60, 0x48c8a02, 0x1e019b0, 202, 0,
	  0, 0, 0, 0,
      0, 0 },
	// PES4 1.10
	{ 0x4e40ca0, 0x4e41c42, 0x215d570, 205, 0x232f208,
	  0x4def7a0, 0xa060bc, 0x4e436c4, 0x22fb5f8,
      0x2381398, 0x2381374 },
	// PES4 1.0
	{ 0x4e3ed40, 0x4e3fce2, 0x215b5c0, 205, 0x232d290,
	  0x4ded840, 0xa040bc, 0x4e416c4, 0x22f9698,
      0x237f418, 0x237f3f4 },
	// WE8I US
	{ 0x4e40e40, 0x4e41de2, 0x215d5b0, 205, 0x232f348,
	  0x4def780, 0xa060bc, 0x4e43864, 0x22fb648,
      0x23814d8, 0x23814b4 },
};

// NOTE: when looking for mirror address of 0x4c90c98 (PES4 1.10),
// look for 0x4c90a0c data address in the executable, and then
// adjust by the same difference (0x28c)

DWORD code[CODELEN];
DWORD data[DATALEN];

#define NUM_BALLS 2

IDirect3DDevice8* g_device = NULL;
IDirect3DTexture8* g_lastMediumTex = NULL;
IDirect3DTexture8* g_lastBigTex = NULL;
BOOL g_isBibs = FALSE;

BYTE* g_lastDecodedKit = NULL;
PALETTEENTRY* g_palette = NULL;
BOOL g_bigTextureAvailable = FALSE;

int bigCount = 0;
IDirect3DTexture8* lastBig = NULL;

IDirect3DSurface8* g_med0 = NULL;
IDirect3DSurface8* g_med1 = NULL;

// list for accumulating textures
//list<DWORD> texs;

IDirect3DTexture8* g_bigNumberShape[] = {NULL, NULL, NULL, NULL};
IDirect3DTexture8** g_pSmallNumberShape[] = {NULL, NULL, NULL, NULL};

BYTE* g_tex = NULL;
DWORD g_texSize = 0;
BOOL g_customTex = FALSE;
AFSITEMINFO g_etcEeTex;
BOOL g_boostNumShapes = FALSE;

// Actual number of teams in the game
int g_numTeams = 0; 

DWORD g_sign[] = {0, 0, 0, 0, 0, 0, 0, 0};
char  g_filename[8][BUFLEN];

int g_demoTeamNum[4] = {6, 13, 26, 27}; // England, Italy, Spain, Sweden
char* STANDARD_KIT_NAMES[5] = {"texga.bmp", "texgb.bmp", "texpa.bmp", "texpb.bmp", "gloves.bmp"};

TEAMBUFFERINFO g_teamInfo[256];
AFSITEMINFO g_bibs;
BOOL g_teamOffsetsLoaded = FALSE;
LONG g_offset = 0;
LONG g_kitOffset = 0;
BYTE g_sfpCode[6];
char g_afsFileName[BUFLEN];
UINT g_triggerLoad3rdKit = 0;

WORD g_currTeams[2] = {0xffff, 0xffff};

KitEntry* g_kitExtras[0x500];

//////////////////////////////////////

#define BALL_MDLS_COUNT 9
#define BALL_TEXS_COUNT 7

char* ballMdls[] = { 
	"ball00mdl.bin", "ball01mdl.bin", "ball02mdl.bin", "ball08mdl.bin",
	"ball09mdl.bin", "ball10mdl.bin", "ball14mdl.bin", "ball15mdl.bin",
	"ball_mdl.bin"
};
char* ballTexs[] = {
	"ball00tex.bin", "ball02tex.bin", "ball08tex.bin",
	"ball09tex.bin", "ball10tex.bin", "ball14tex.bin", "ball15tex.bin",
};

AFSITEMINFO g_ballMdls[BALL_MDLS_COUNT];
AFSITEMINFO g_ballTexs[BALL_TEXS_COUNT];

BYTE* g_ballMdl = NULL;
BYTE* g_ballTex = NULL;
BallEntry* g_balls = NULL;

// text labels for default strips
char* DEFAULT_LABEL[] = { "1st default", "2nd default", "auto-pick" };

// Kit Database
KDB* kDB = NULL;

//char g_ballName[255];

///// Graphics ////////////////

struct CUSTOMVERTEX { 
	FLOAT x,y,z,w;
	DWORD color;
};

IDirect3DVertexBuffer8* g_pVB0 = NULL;
DWORD g_dwSavedStateBlock = 0L;
DWORD g_dwDrawTextStateBlock = 0L;

#define IHEIGHT 30.0f
#define IWIDTH IHEIGHT
#define INDX 8.0f
#define INDY 8.0f
float POSX = 0.0f;
float POSY = 0.0f;

// star coordinates.
float PI = 3.1415926f;
float R = ((float)IHEIGHT)/2.0f;
float d18 = PI/10.0f;
float d54 = d18*3.0f;
float y[] = { R*sin(d18), R, R*sin(d18), -R*sin(d54), -R*sin(d54), R*sin(d18), R*sin(d18) }; 
float x[] = { R*cos(d18), 0.0f, -R*cos(d18), -R*cos(d54), R*cos(d54) };
float x5 = x[4]*(y[1] - y[5])/(y[1] - y[4]);
float x6 = -x5;

CUSTOMVERTEX g_Vert0[] =
{
	{POSX+INDX+R + x[1], POSY+INDY+R - y[1], 0.0f, 1.0f, 0xff4488ff },
	{POSX+INDX+R + x6,   POSY+INDY+R - y[6], 0.0f, 1.0f, 0xff4488ff },
	{POSX+INDX+R + x5,   POSY+INDY+R - y[5], 0.0f, 1.0f, 0xff4488ff },
	{POSX+INDX+R + x[2], POSY+INDY+R - y[2], 0.0f, 1.0f, 0xff4488ff },
	{POSX+INDX+R + x[4], POSY+INDY+R - y[4], 0.0f, 1.0f, 0xff4488ff },
	{POSX+INDX+R + x5,   POSY+INDY+R - y[5], 0.0f, 1.0f, 0xff4488ff },
	{POSX+INDX+R + x[3], POSY+INDY+R - y[3], 0.0f, 1.0f, 0xff4488ff },
	{POSX+INDX+R + x[0], POSY+INDY+R - y[0], 0.0f, 1.0f, 0xff4488ff },
	{POSX+INDX+R + x6,   POSY+INDY+R - y[6], 0.0f, 1.0f, 0xff4488ff },
};


#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZRHW|D3DFVF_DIFFUSE)

#define HEAP_SEARCH_INTERVAL 500  // in msec
#define DLLMAP_PAUSE 1000         // in msec

BITMAPINFOHEADER bitmapHeader;

IDirect3DSurface8* g_backBuffer;
D3DFORMAT g_bbFormat;
BOOL g_bGotFormat = false;
int bpp = 0;

typedef DWORD   (STDMETHODCALLTYPE *GETPROCESSHEAPS)(DWORD, PHANDLE);

typedef IDirect3D8* (STDMETHODCALLTYPE *PFNDIRECT3DCREATE8PROC)(UINT sdkVersion);

/* IDirect3DDevice8 method-types */
typedef HRESULT (STDMETHODCALLTYPE *PFNCREATETEXTUREPROC)(IDirect3DDevice8* self, 
UINT width, UINT height, UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool, 
IDirect3DTexture8** ppTexture);
typedef HRESULT (STDMETHODCALLTYPE *PFNSETTEXTUREPROC)(IDirect3DDevice8* self, 
DWORD stage, IDirect3DBaseTexture8* pTexture);
typedef HRESULT (STDMETHODCALLTYPE *PFNSETTEXTURESTAGESTATEPROC)(IDirect3DDevice8* self, 
DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value);
typedef HRESULT (STDMETHODCALLTYPE *PFNUPDATETEXTUREPROC)(IDirect3DDevice8* self,
IDirect3DBaseTexture8* pSrc, IDirect3DBaseTexture8* pDest);
typedef HRESULT (STDMETHODCALLTYPE *PFNPRESENTPROC)(IDirect3DDevice8* self, 
CONST RECT* src, CONST RECT* dest, HWND hWnd, LPVOID unused);
typedef HRESULT (STDMETHODCALLTYPE *PFNRESETPROC)(IDirect3DDevice8* self, LPVOID);
typedef HRESULT (STDMETHODCALLTYPE *PFNCOPYRECTSPROC)(IDirect3DDevice8* self,
IDirect3DSurface8* pSourceSurface, CONST RECT* pSourceRectsArray, UINT cRects,
IDirect3DSurface8* pDestinationSurface, CONST POINT* pDestPointsArray);
typedef HRESULT (STDMETHODCALLTYPE *PFNAPPLYSTATEBLOCKPROC)(IDirect3DDevice8* self, DWORD token);
typedef HRESULT (STDMETHODCALLTYPE *PFNBEGINSCENEPROC)(IDirect3DDevice8* self);
typedef HRESULT (STDMETHODCALLTYPE *PFNENDSCENEPROC)(IDirect3DDevice8* self);
typedef HRESULT (STDMETHODCALLTYPE *PFNGETSURFACELEVELPROC)(IDirect3DTexture8* self,
UINT level, IDirect3DSurface8** ppSurfaceLevel);

typedef DWORD  (*UNIDECRYPT)(DWORD, DWORD);
typedef DWORD  (*UNIDECODE)(DWORD, DWORD, DWORD);
typedef DWORD  (*UNPACK)(DWORD, DWORD, DWORD, DWORD);
typedef DWORD  (*CHECKTEAM)(DWORD);
typedef DWORD  (*SETKITATTRIBUTES)(DWORD);
typedef DWORD  (*FINISHKITPICK)(DWORD);
typedef DWORD  (*ALLOCMEM)(DWORD, DWORD, DWORD);
typedef DWORD  (*RESETFLAGS)(DWORD);

DWORD  JuceUniDecrypt(DWORD, DWORD);
DWORD  JuceUniDecode(DWORD, DWORD, DWORD);
DWORD  JuceUnpack(DWORD, DWORD, DWORD, DWORD);
DWORD  JuceFinalSetKitAttributes(DWORD);
DWORD  JuceEditModeSetKitAttributes(DWORD);
DWORD  JuceKitPickSetKitAttributes(DWORD);
DWORD  JuceFinishKitPick(DWORD);
DWORD  JuceAllocMem(DWORD, DWORD, DWORD);
DWORD  JuceResetFlags(DWORD);

IDirect3D8* STDMETHODCALLTYPE JuceDirect3DCreate8(UINT);
DWORD  STDMETHODCALLTYPE JuceSetFilePointer(HANDLE, LONG, PLONG, DWORD);
HRESULT STDMETHODCALLTYPE JuceCreateTexture(IDirect3DDevice8* self, UINT width, UINT height, 
UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool, IDirect3DTexture8** ppTexture);
HRESULT STDMETHODCALLTYPE JuceSetTexture(IDirect3DDevice8* self, DWORD stage, 
IDirect3DBaseTexture8* pTexture);
HRESULT STDMETHODCALLTYPE JuceSetTextureStageState(IDirect3DDevice8* self, DWORD Stage,
D3DTEXTURESTAGESTATETYPE Type, DWORD Value);
HRESULT STDMETHODCALLTYPE JuceUpdateTexture(IDirect3DDevice8* self,
IDirect3DBaseTexture8* pSrc, IDirect3DBaseTexture8* pDest);
EXTERN_C HRESULT _declspec(dllexport) STDMETHODCALLTYPE JucePresent(
IDirect3DDevice8* self, CONST RECT* src, CONST RECT* dest, HWND hWnd, LPVOID unused);
HRESULT STDMETHODCALLTYPE JuceCopyRects(IDirect3DDevice8* self,
IDirect3DSurface8* pSourceSurface, CONST RECT* pSourceRectsArray, UINT cRects,
IDirect3DSurface8* pDestinationSurface, CONST POINT* pDestPointsArray);
HRESULT STDMETHODCALLTYPE JuceApplyStateBlock(IDirect3DDevice8* self, DWORD token);
HRESULT STDMETHODCALLTYPE JuceBeginScene(IDirect3DDevice8* self);
HRESULT STDMETHODCALLTYPE JuceEndScene(IDirect3DDevice8* self);
HRESULT STDMETHODCALLTYPE JuceGetSurfaceLevel(IDirect3DTexture8* self,
UINT level, IDirect3DSurface8** ppSurfaceLevel);

BOOL WINAPI Override_QueryPerformanceFrequency(
        LARGE_INTEGER *lpPerformanceFrequency);
BOOL SignExists(DWORD sig, char* filename);
DWORD LoadTexture(BYTE** tex, char* filename);
void ApplyBallTexture(BYTE* orgBall, BYTE* ballTex);

/* function pointers */
PFNCREATETEXTUREPROC g_orgCreateTexture = NULL;
PFNSETTEXTUREPROC g_orgSetTexture = NULL;
PFNSETTEXTURESTAGESTATEPROC g_orgSetTextureStageState = NULL;
PFNPRESENTPROC g_orgPresent = NULL;
PFNRESETPROC g_orgReset = NULL;
PFNCOPYRECTSPROC g_orgCopyRects = NULL;
PFNAPPLYSTATEBLOCKPROC g_orgApplyStateBlock = NULL;
PFNBEGINSCENEPROC g_orgBeginScene = NULL;
PFNENDSCENEPROC g_orgEndScene = NULL;
PFNGETSURFACELEVELPROC g_orgGetSurfaceLevel = NULL;
PFNUPDATETEXTUREPROC g_orgUpdateTexture = NULL;
PFNDIRECT3DCREATE8PROC g_orgDirect3DCreate8 = NULL;

BYTE g_codeFragment[5] = {0,0,0,0,0};
BYTE g_jmp[5] = {0,0,0,0,0};
DWORD g_savedProtection = 0;

UNIDECRYPT UniDecrypt = NULL;
UNIDECODE UniDecode = NULL;
UNPACK Unpack = NULL;
SETKITATTRIBUTES SetKitAttributes = NULL;
FINISHKITPICK FinishKitPick = NULL;
ALLOCMEM AllocMem = NULL;
RESETFLAGS ResetFlags = NULL;

bool bUniDecodeHooked = false;
bool bUnpackHooked = false;
bool bUniDecryptHooked = false;
bool bSetFilePointerHooked = false;
bool bSetColorsHooked = false;
bool bFinishKitPickHooked = false;
bool bAllocMemHooked = false;
bool bResetFlagsHooked = false;

int g_ballIndex = 0;

HINSTANCE hInst, hProc;
HMODULE hD3D8 = NULL, utilDLL = NULL;
HWND hProcWnd = NULL;
HANDLE procHeap = NULL;

char myfile[MAX_FILEPATH];
char mydir[MAX_FILEPATH];
char processfile[MAX_FILEPATH];
char *shortProcessfile;
char shortProcessfileNoExt[BUFLEN];
char buf[BUFLEN];

char logName[BUFLEN];

// keyboard hook handle
HHOOK g_hKeyboardHook = NULL;
BOOL bIndicate = false;

UINT g_bbWidth = 0;
UINT g_bbHeight = 0;
UINT g_labelsYShift = 0;

// buffer to keep current frame 
BYTE* g_rgbBuf = NULL;
INT g_rgbSize = 0;
INT g_Pitch = 0;

BOOL GoodTeamId(WORD id);
void VerifyTeams(void);
int GetGameVersion(void);
void Initialize(int v);
void ResetCyclesAndBuffers2(void);
void LoadKDB();
HRESULT SaveAs8bitBMP(char*, BYTE*);
HRESULT SaveAs8bitBMP(char*, BYTE*, BYTE*, LONG, LONG);
HRESULT SaveAsBMP(char*, BYTE*, SIZE_T, LONG, LONG, int);

void ShuffleBallTexture(BYTE* dest, BYTE* src);
void DeshuffleBallTexture(BYTE* dest, BYTE* src);

Kit* utilGetKit(WORD kitId);
TeamAttr* utilGetTeamAttr(WORD teamId);
BOOL IsEditedKit(int slot);

//////////////////////////////////////////////////////////////
// FUNCTIONS /////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

// loads the ball model and texture into 2 global buffers
void LoadBall(Ball* ball)
{
	FILE* f;
	DWORD size;

	char* mdl = ball->mdlFileName;
	char* tex = ball->texFileName;

	// clear out the pointers
	g_ballMdl = g_ballTex = NULL; 

	// load model file (optional)
	if (mdl != NULL && mdl[0] != '\0')
	{
		char filename[BUFLEN];
		ZeroMemory(filename, BUFLEN);
		lstrcpy(filename, g_config.kdbDir);
		lstrcat(filename, "KDB/balls/"); lstrcat(filename, mdl);

		f = fopen(filename, "rb");
		if (f != NULL)
		{
			fseek(f, 4, SEEK_SET);
			fread(&size, 4, 1, f);
			fseek(f, 0, SEEK_SET);
			g_ballMdl = (BYTE*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size + 32);
			fread(g_ballMdl, size + 32, 1, f);
			fclose(f);
			TRACE2("LoadBall: g_ballMdl address = %08x", (DWORD)g_ballMdl);
		}
		else
			LogWithString("LoadBall: unable to open ball model file: %s.", mdl);
	}

	// load texture file (required)
	if (ball->isTexBmp)
	{
		// ball texture is a BMP
		char filename[BUFLEN];
		ZeroMemory(filename, BUFLEN);
		lstrcpy(filename, g_config.kdbDir);
		lstrcat(filename, "KDB/balls/"); lstrcat(filename, tex);

		LoadTexture(&g_ballTex, filename);
	}
	else
	{
		// ball texture is a BIN
		char filename[BUFLEN];
		ZeroMemory(filename, BUFLEN);
		lstrcpy(filename, g_config.kdbDir);
		lstrcat(filename, "KDB/balls/"); lstrcat(filename, tex);
		
		f = fopen(filename, "rb");
		if (f != NULL)
		{
			fseek(f, 4, SEEK_SET);
			fread(&size, 4, 1, f);
			fseek(f, 0, SEEK_SET);
			g_ballTex = (BYTE*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size + 32);
			fread(g_ballTex, size + 32, 1, f);
			fclose(f);
			TRACE2("LoadBall: g_ballTex address = %08x", (DWORD)g_ballTex);
		}
		else
			LogWithString("LoadBall: unable to open ball texture file: %s.", tex);
	}
}

// frees the ball buffers
void FreeBall()
{
	if (g_ballMdl) HeapFree(GetProcessHeap(), 0, g_ballMdl);
	if (g_ballTex) HeapFree(GetProcessHeap(), 0, g_ballTex);
	g_ballMdl = g_ballTex = NULL;
	Log("Ball memory freed.");
}

// Calls IUnknown::Release() on an instance
void SafeRelease(LPVOID ppObj)
{
	IUnknown** ppIUnknown = (IUnknown**)ppObj;
	if (ppIUnknown == NULL)
	{
		Log("Address of IUnknown reference is 0");
		return;
	}
	if (*ppIUnknown != NULL)
	{
		(*ppIUnknown)->Release();
		*ppIUnknown = NULL;
	}
}

/* determine window handle */
void GetProcWnd(IDirect3DDevice8* d3dDevice)
{
	TRACE("GetProcWnd: called.");
	D3DDEVICE_CREATION_PARAMETERS params;	
	if (SUCCEEDED(d3dDevice->GetCreationParameters(&params)))
	{
		hProcWnd = params.hFocusWindow;
	}
}

/* remove keyboard hook */
void UninstallKeyboardHook(void)
{
	if (g_hKeyboardHook != NULL)
	{
		UnhookWindowsHookEx( g_hKeyboardHook );
		Log("Keyboard hook uninstalled.");
		g_hKeyboardHook = NULL;
	}
}

/* creates vertex buffers */
HRESULT InitVB(IDirect3DDevice8* dev)
{
	VOID* pVertices;

	// create vertex buffers
	// #0
	if (FAILED(dev->CreateVertexBuffer(9*sizeof(CUSTOMVERTEX), D3DUSAGE_WRITEONLY, 
					D3DFVF_CUSTOMVERTEX, D3DPOOL_MANAGED, &g_pVB0)))
	{
		Log("CreateVertexBuffer() failed.");
		return E_FAIL;
	}
	Log("CreateVertexBuffer() done.");

	if (FAILED(g_pVB0->Lock(0, sizeof(g_Vert0), (BYTE**)&pVertices, 0)))
	{
		Log("g_pVB0->Lock() failed.");
		return E_FAIL;
	}
	memcpy(pVertices, g_Vert0, sizeof(g_Vert0));
	g_pVB0->Unlock();

	return S_OK;
}

//-----------------------------------------------------------------------------
// Name: RestoreDeviceObjects()
// Desc:
//-----------------------------------------------------------------------------
HRESULT RestoreDeviceObjects(IDirect3DDevice8* dev)
{
    HRESULT hr = InitVB(dev);
    if (FAILED(hr))
    {
		Log("InitVB() failed.");
        return hr;
    }
	Log("InitVB() done.");

	if (!g_fontInitialized) 
	{
		g_font->InitDeviceObjects(dev);
		g_fontInitialized = true;
	}
	g_font->RestoreDeviceObjects();

	TRACE("g_font->RestoreDeviceObjects() returned.");

	D3DVIEWPORT8 vp;
	vp.X      = POSX;
	vp.Y      = POSY;
	vp.Width  = INDX*2 + IWIDTH;
	vp.Height = INDY*2 + IHEIGHT;
	vp.MinZ   = 0.0f;
	vp.MaxZ   = 1.0f;

	TRACE2("vp.X = %d", (int)vp.X);
	TRACE2("vp.Y = %d", (int)vp.Y);
	TRACE2("vp.Width = %d", (int)vp.Width);
	TRACE2("vp.Height = %d", (int)vp.Height);

	// Create the state blocks for rendering text
	for( UINT which=0; which<2; which++ )
	{
		dev->BeginStateBlock();

		dev->SetViewport( &vp );
		dev->SetRenderState( D3DRS_ZENABLE, D3DZB_FALSE );
		dev->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
		dev->SetRenderState( D3DRS_FILLMODE,   D3DFILL_SOLID );
		dev->SetRenderState( D3DRS_CULLMODE,   D3DCULL_CW );
		dev->SetRenderState( D3DRS_STENCILENABLE,    FALSE );
		dev->SetRenderState( D3DRS_FOGENABLE,        FALSE );

		dev->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_DISABLE );
		dev->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_DISABLE );

		dev->SetVertexShader( D3DFVF_CUSTOMVERTEX );
		dev->SetPixelShader ( NULL );
		dev->SetStreamSource( 0, g_pVB0, sizeof(CUSTOMVERTEX) );

		if( which==0 )
			dev->EndStateBlock( &g_dwSavedStateBlock );
		else
			dev->EndStateBlock( &g_dwDrawTextStateBlock );
	}

    return S_OK;
}

//-----------------------------------------------------------------------------
// Name: InvalidateDeviceObjects()
// Desc: Destroys all device-dependent objects
//-----------------------------------------------------------------------------
HRESULT InvalidateDeviceObjects(IDirect3DDevice8* dev)
{
	TRACE("InvalidateDeviceObjects called.");
	if (dev == NULL)
	{
		TRACE("InvalidateDeviceObjects: nothing to invalidate.");
		return S_OK;
	}

	SafeRelease( &g_pVB0 );
	Log("InvalidateDeviceObjects: SafeRelease(s) done.");

	// Delete the state blocks
	try
	{
		if( g_dwSavedStateBlock )
			dev->DeleteStateBlock( g_dwSavedStateBlock );
		if( g_dwDrawTextStateBlock )
			dev->DeleteStateBlock( g_dwDrawTextStateBlock );
		Log("InvalidateDeviceObjects: DeleteStateBlock(s) done.");

		if (g_font) g_font->InvalidateDeviceObjects();
	}
	catch (...)
	{
		TRACE("InvalidateDeviceObjects: problems with invalidating");
	}

	g_dwSavedStateBlock    = 0L;
	g_dwDrawTextStateBlock = 0L;

    return S_OK;
}

//-----------------------------------------------------------------------------
// Name: DeleteDeviceObjects()
// Desc: Destroys all device-dependent objects
//-----------------------------------------------------------------------------
HRESULT DeleteDeviceObjects(IDirect3DDevice8* dev)
{
	if (g_font) g_font->DeleteDeviceObjects();
	g_fontInitialized = false;

    return S_OK;
}

void DrawIndicator(LPVOID self)
{
	IDirect3DDevice8* dev = (IDirect3DDevice8*)self;

	if (g_pVB0 == NULL) 
	{
		if (FAILED(RestoreDeviceObjects(dev)))
		{
			Log("DrawIndicator: RestoreDeviceObjects() failed.");
		}
		Log("DrawIndicator: RestoreDeviceObjects() done.");
	}

	// setup renderstate
	dev->CaptureStateBlock( g_dwSavedStateBlock );
	dev->ApplyStateBlock( g_dwDrawTextStateBlock );

	// render
	dev->BeginScene();

	dev->SetStreamSource( 0, g_pVB0, sizeof(CUSTOMVERTEX) );
	dev->DrawPrimitive( D3DPT_TRIANGLELIST, 0, 3 );

	dev->EndScene();

	// restore the modified renderstates
	dev->ApplyStateBlock( g_dwSavedStateBlock );
}

void DrawBallLabel(IDirect3DDevice8* dev)
{
	if (g_pVB0 == NULL) 
	{
		if (FAILED(RestoreDeviceObjects(dev)))
		{
			Log("DrawBallLabel: RestoreDeviceObjects() failed.");
		}
		Log("DrawBallLabel: RestoreDeviceObjects() done.");
	}

	// draw ball label
	//TRACE("DrawBallLabel: About to draw text.");
	char buf[255];
	ZeroMemory(buf, 255);
	lstrcpy(buf, "Ball: ");
	DWORD color = 0xffa0a0a0; // light grey - for default

	if (g_balls == NULL || g_balls->ball == NULL)
	{
		lstrcat(buf, "game choice");
	}
	else
	{
		lstrcat(buf, g_balls->ball->name);
		color = 0xff4488ff; // blue
	}

	SIZE size;
	g_font->GetTextExtent(buf, &size);
	g_font->DrawText( g_bbWidth/2 - size.cx/2,  POSY + IHEIGHT + 10, color, buf);
	//Log("DrawBallLabel: Text drawn.");
}

// sanity-check for team id
BOOL GoodTeamId(WORD id)
{
	//TRACE2("GoodTeamId: checking id = %04x", id);
	return (id>=0 && id<256);
}

void DrawKitLabels(IDirect3DDevice8* dev)
{
	if (g_pVB0 == NULL) 
	{
		if (FAILED(RestoreDeviceObjects(dev)))
		{
			Log("DrawKitLabels: RestoreDeviceObjects() failed.");
		}
		Log("DrawKitLabels: RestoreDeviceObjects() done.");
	}

	//TRACE("DrawKitLabels: About to draw text.");
	char buf[255];
	SIZE size;
	DWORD color;

	WORD id = 0xffff;
	WORD kitId = 0xffff;
	BYTE* strips = (BYTE*)data[TEAM_STRIPS];

	// HOME PLAYER
	ZeroMemory(buf, 255);
	lstrcpy(buf, "PL: ");
	id = g_currTeams[0];
	// only show the label, if extra kits are available, and the kit is not edited
	if (GoodTeamId(id) && !IsEditedKit(1) && kDB->players[id].extra != NULL)
	{
		kitId = id * 5 + 2 + (strips[0] & 0x01);
		if (g_kitExtras[kitId] == NULL)
		{
			lstrcat(buf, DEFAULT_LABEL[strips[0] & 0x01]);
			color = 0xffa0a0a0; // light grey - for default
		}
		else 
		{
			lstrcat(buf, g_kitExtras[kitId]->kit->fileName);
			color = 0xff4488ff; // blue;
		}
		g_font->GetTextExtent(buf, &size);
		g_font->DrawText( g_bbWidth/2 - 15 - size.cx,  POSY + IHEIGHT + 35, color, buf);
	}

	// AWAY PLAYER
	ZeroMemory(buf, 255);
	lstrcpy(buf, "PL: ");
	id = g_currTeams[1];
	// only show the label, if extra kits are available, and the kit is not edited
	if (GoodTeamId(id) && !IsEditedKit(3) && kDB->players[id].extra != NULL)
	{
		kitId = id * 5 + 2 + (strips[1] & 0x01);
		if (g_kitExtras[kitId] == NULL)
		{
			lstrcat(buf, DEFAULT_LABEL[strips[1] & 0x01]);
			color = 0xffa0a0a0; // light grey - for default
		}
		else 
		{
			lstrcat(buf, g_kitExtras[kitId]->kit->fileName);
			color = 0xff4488ff; // blue;
		}
		g_font->GetTextExtent(buf, &size);
		g_font->DrawText( g_bbWidth/2 + 15,  POSY + IHEIGHT + 35, color, buf);
	}

	// HOME GOALKEEPER
	ZeroMemory(buf, 255);
	lstrcpy(buf, "GK: ");
	id = g_currTeams[0];
	kitId = id * 5;
	// only show the label
	if (GoodTeamId(id))
	{
		if (g_kitExtras[kitId] == NULL)
		{
			lstrcat(buf, DEFAULT_LABEL[2]);
			color = 0xffa0a0a0; // light grey - for default
		}
		else 
		{
			lstrcat(buf, g_kitExtras[kitId]->kit->fileName);
			color = 0xff4488ff; // blue;
		}
		g_font->GetTextExtent(buf, &size);
		g_font->DrawText( g_bbWidth/2 - 15 - size.cx,  POSY + IHEIGHT + 50, color, buf);
	}

	// AWAY GOALKEEPER
	ZeroMemory(buf, 255);
	lstrcpy(buf, "GK: ");
	id = g_currTeams[1];
	kitId = id * 5;
	// only show the label
	if (GoodTeamId(id))
	{
		if (g_kitExtras[kitId] == NULL)
		{
			lstrcat(buf, DEFAULT_LABEL[2]);
			color = 0xffa0a0a0; // light grey - for default
		}
		else 
		{
			lstrcat(buf, g_kitExtras[kitId]->kit->fileName);
			color = 0xff4488ff; // blue;
		}
		g_font->GetTextExtent(buf, &size);
		g_font->DrawText( g_bbWidth/2 + 15,  POSY + IHEIGHT + 50, color, buf);
	}

	//Log("DrawKitLabels: Text drawn.");
}

/* New Reset function */
EXTERN_C HRESULT _declspec(dllexport) STDMETHODCALLTYPE JuceReset(
IDirect3DDevice8* self, LPVOID params)
{
	TRACE("JuceReset: called.");
	Log("JuceReset: cleaning-up.");

	// drop HWND 
	hProcWnd = NULL;

	// clean-up
	if (g_rgbBuf != NULL) 
	{
		HeapFree(procHeap, 0, g_rgbBuf);
		g_rgbBuf = NULL;
	}

	InvalidateDeviceObjects(self);
	DeleteDeviceObjects(self);

	g_bGotFormat = false;

	// CALL ORIGINAL FUNCTION
	HRESULT res = g_orgReset(self, params);

	TRACE("JuceReset: Reset() is done. About to return.");
	return res;
}

/**
 * Determine format of back buffer and its dimensions.
 */
void GetBackBufferInfo(IDirect3DDevice8* d3dDevice)
{
	TRACE("GetBackBufferInfo: called.");

	// get the 0th backbuffer.
	if (SUCCEEDED(d3dDevice->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &g_backBuffer)))
	{
		D3DSURFACE_DESC desc;
		g_backBuffer->GetDesc(&desc);
		g_bbFormat = desc.Format;
		g_bbWidth = desc.Width;
		g_bbHeight = desc.Height;

		// vertical offset from bottom for uniform labels
		g_labelsYShift = (UINT)(g_bbHeight * 0.185);

		// adjust indicator coords
		float OLDPOSX = POSX;
		float OLDPOSY = POSY;
		POSX = g_bbWidth/2 - INDX - IWIDTH/2;
		POSY = g_bbHeight/2 - INDY - IHEIGHT/2 - 20;
		for (int k=0; k<9; k++)
		{
			g_Vert0[k].x += POSX - OLDPOSX;
			g_Vert0[k].y += POSY - OLDPOSY;
		}

		// check dest.surface format
		bpp = 0;
		if (g_bbFormat == D3DFMT_R8G8B8) bpp = 3;
		else if (g_bbFormat == D3DFMT_R5G6B5 || g_bbFormat == D3DFMT_X1R5G5B5) bpp = 2;
		else if (g_bbFormat == D3DFMT_A8R8G8B8 || g_bbFormat == D3DFMT_X8R8G8B8) bpp = 4;

		// release backbuffer
		g_backBuffer->Release();

		Log("GetBackBufferInfo: got new back buffer format and info.");
		g_bGotFormat = true;


	}
}

/** 
 * Finds slot for given kitId.
 * Returns -1, if slot not found.
 */
int GetKitSlot(int kitId)
{
	int slot = -1;
	for (int i=0; i<4; i++) 
	{
		WORD id = *((WORD*)(data[KIT_SLOTS] + i*0x38 + 0x0a));
		if (kitId == id) { slot = i; break; }
	}
	return slot;
}

/* New Present function */
EXTERN_C HRESULT _declspec(dllexport) STDMETHODCALLTYPE JucePresent(
IDirect3DDevice8* self, CONST RECT* src, CONST RECT* dest, HWND hWnd, LPVOID unused)
{
	// determine backbuffer's format and dimensions, if not done yet.
	if (!g_bGotFormat) 
	{
		GetBackBufferInfo(self);
		LogWithNumber("JucePresent: IDirect3DDevice8* = %08x", (DWORD)self);
	}

	// install keyboard hook, if not done yet.
	if (g_hKeyboardHook == NULL)
	{
		g_hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD, KeyboardProc, hInst, GetCurrentThreadId());
		LogWithNumber("Installed keyboard hook: g_hKeyboardHook = %d", (DWORD)g_hKeyboardHook);
	}

	/* draw the star indicator */
	if (bIndicate) DrawIndicator(self);

	/* draw the uniform labels */
	if (bIndicate) DrawKitLabels(self);

	/* draw ball label, if we have extra balls */
	if (bIndicate && kDB->ballCount > 0) DrawBallLabel(self);

	// CALL ORIGINAL FUNCTION ///////////////////
	HRESULT res = g_orgPresent(self, src, dest, hWnd, unused);

	if (g_triggerLoad3rdKit > 0)
	{
		int kitId = 0, slot = -1;
		BYTE* strips = (BYTE*)data[TEAM_STRIPS];

		switch (g_triggerLoad3rdKit)
		{
			case 1:
				g_triggerLoad3rdKit = 0;
				// clear kit slot to force uni-file reload
				kitId = (strips[0] & 0x01) ? g_currTeams[0] * 5 + 3 : g_currTeams[0] * 5 + 2;
				slot = GetKitSlot(kitId);
				if (slot != -1)
				{
					ZeroMemory((BYTE*)data[KIT_SLOTS] + slot * 0x38, 0x38);
					// set uni-reload flag
					*((DWORD*)data[KIT_CHECK_TRIGGER]) = 1;
				}
				break;

			case 2:
				g_triggerLoad3rdKit = 0;
				// clear kit info to force uni-file reload
				kitId = (strips[1] & 0x01) ? g_currTeams[1] * 5 + 3 : g_currTeams[1] * 5 + 2;
				slot = GetKitSlot(kitId);
				if (slot != -1)
				{
					ZeroMemory((BYTE*)data[KIT_SLOTS] + slot * 0x38, 0x38);
					// set uni-reload flag
					*((DWORD*)data[KIT_CHECK_TRIGGER]) = 1;
				}
				break;
		}
	}

	TRACE("=========================frame============================");
	return res;
}

/* CopyRects tracker. */
HRESULT STDMETHODCALLTYPE JuceCopyRects(IDirect3DDevice8* self,
IDirect3DSurface8* pSourceSurface, CONST RECT* pSourceRectsArray, UINT cRects,
IDirect3DSurface8* pDestinationSurface, CONST POINT* pDestPointsArray)
{
	TRACEX("JuceCopyRects: rect(%dx%d), numRects = %d", pSourceRectsArray[0].right,
			pSourceRectsArray[0].bottom, cRects);
	return g_orgCopyRects(self, pSourceSurface, pSourceRectsArray, cRects,
			pDestinationSurface, pDestPointsArray);
}

/* ApplyStateBlock tracker. */
HRESULT STDMETHODCALLTYPE JuceApplyStateBlock(IDirect3DDevice8* self, DWORD token)
{
	TRACE2("JuceApplyStateBlock: token = %d", token);
	return g_orgApplyStateBlock(self, token);
}

/* BeginScene tracker. */
HRESULT STDMETHODCALLTYPE JuceBeginScene(IDirect3DDevice8* self)
{
	TRACE("JuceBeginScene: CALLED.");
	return g_orgBeginScene(self);
}

/* EndScene tracker. */
HRESULT STDMETHODCALLTYPE JuceEndScene(IDirect3DDevice8* self)
{
	TRACE("JuceEndScene: CALLED.");
	return g_orgEndScene(self);
}

/* Restore original Reset() and Present() */
EXTERN_C _declspec(dllexport) void RestoreDeviceMethods()
{
	if (g_device == NULL) 
		return;

	DWORD* vtable = (DWORD*)(*((DWORD*)g_device));

	TRACE2("RestoreDeviceMethods: vtable = %08x", (DWORD)vtable);

	try
	{
		if (g_orgReset != NULL) vtable[14] = (DWORD)g_orgReset;
		TRACE("RestoreDeviceMethods: Reset restored.");
		if (g_orgPresent != NULL) vtable[15] = (DWORD)g_orgPresent;
		TRACE("RestoreDeviceMethods: Present restored.");
		if (g_orgCreateTexture != NULL) vtable[20] = (DWORD)g_orgCreateTexture;
		TRACE("RestoreDeviceMethods: CreateTexture restored.");
	}
	catch (...) {} // ignore exceptions at this point

	Log("RestoreDeviceMethods: done.");
}

/************
 * This function initializes kitserver.
 ************/
void InitKserv()
{
	// check if code section is writeable
	IMAGE_SECTION_HEADER* sechdr = GetCodeSectionHeader();
	TRACE2("sechdr = %08x", (DWORD)sechdr);
	if (sechdr && (sechdr->Characteristics & 0x80000000))
	{
		// Determine the game version
		int v = GetGameVersion();
		if (v != -1)
		{
			LogWithString("Game version: %s", GAME[v]);
			Initialize(v);

			// hook code[C_SETFILEPOINTER]
			if (code[C_SETFILEPOINTER_CS] != 0)
			{
				BYTE* bptr = (BYTE*)code[C_SETFILEPOINTER_CS];
				// save original code for CALL SetFilePointer
				memcpy(g_sfpCode, bptr, 6);

				bptr[0] = 0xe8; bptr[5] = 0x90; // NOP
				DWORD* ptr = (DWORD*)(code[C_SETFILEPOINTER_CS] + 1);
				ptr[0] = (DWORD)JuceSetFilePointer - (DWORD)(code[C_SETFILEPOINTER_CS] + 5);
				bSetFilePointerHooked = true;
				Log("SetFilePointer HOOKED at code[C_SETFILEPOINTER_CS]");
			}

			/*
			// hook code[C_UNIDECRYPT]
			{
				DWORD* ptr = (DWORD*)(code[C_UNIDECRYPT_CS] + 1);
				ptr[0] = (DWORD)JuceUniDecrypt - (DWORD)(code[C_UNIDECRYPT_CS] + 5);
				bUniDecryptHooked = true;
				Log("code[C_UNIDECRYPT] HOOKED at code[C_UNIDECRYPT_CS]");
			}
			*/

			// hook code[C_UNIDECODE]
			if (code[C_UNIDECODE] != 0 && code[C_UNIDECODE_CS] != 0)
			{
				DWORD* ptr = (DWORD*)(code[C_UNIDECODE_CS] + 1);
				ptr[0] = (DWORD)JuceUniDecode - (DWORD)(code[C_UNIDECODE_CS] + 5);
				bUniDecodeHooked = true;
				Log("code[C_UNIDECODE] HOOKED at code[C_UNIDECODE_CS]");
			}
			if (code[C_UNIDECODE] != 0 && code[C_UNIDECODE_CS2] != 0)
			{
				DWORD* ptr = (DWORD*)(code[C_UNIDECODE_CS2] + 1);
				ptr[0] = (DWORD)JuceUniDecode - (DWORD)(code[C_UNIDECODE_CS2] + 5);
				bUniDecodeHooked = true;
				Log("code[C_UNIDECODE] HOOKED at code[C_UNIDECODE_CS2]");
			}

			// hook code[C_SETKITATTRIBUTES]
			if (code[C_SETKITATTRIBUTES] != 0 && code[C_SETKITATTRIBUTES_CS1] && code[C_SETKITATTRIBUTES_CS2] != 0)
			{
				DWORD* ptr = (DWORD*)(code[C_SETKITATTRIBUTES_CS1] + 1);
				ptr[0] = (DWORD)JuceFinalSetKitAttributes - (DWORD)(code[C_SETKITATTRIBUTES_CS1] + 5);
				bSetColorsHooked = true;
				Log("code[C_SETKITATTRIBUTES] HOOKED at code[C_SETKITATTRIBUTES_CS1]");

				ptr = (DWORD*)(code[C_SETKITATTRIBUTES_CS2] + 1);
				ptr[0] = (DWORD)JuceFinalSetKitAttributes - (DWORD)(code[C_SETKITATTRIBUTES_CS2] + 5);
				bSetColorsHooked = true;
				Log("code[C_SETKITATTRIBUTES] HOOKED at code[C_SETKITATTRIBUTES_CS2]");

				ptr = (DWORD*)(code[C_SETKITATTRIBUTES_CS3] + 1);
				ptr[0] = (DWORD)JuceKitPickSetKitAttributes - (DWORD)(code[C_SETKITATTRIBUTES_CS3] + 5);
				bSetColorsHooked = true;
				Log("code[C_SETKITATTRIBUTES] HOOKED at code[C_SETKITATTRIBUTES_CS3]");

				ptr = (DWORD*)(code[C_SETKITATTRIBUTES_CS4] + 1);
				ptr[0] = (DWORD)JuceKitPickSetKitAttributes - (DWORD)(code[C_SETKITATTRIBUTES_CS4] + 5);
				bSetColorsHooked = true;
				Log("code[C_SETKITATTRIBUTES] HOOKED at code[C_SETKITATTRIBUTES_CS4]");

				ptr = (DWORD*)(code[C_SETKITATTRIBUTES_CS5] + 1);
				ptr[0] = (DWORD)JuceFinalSetKitAttributes - (DWORD)(code[C_SETKITATTRIBUTES_CS5] + 5);
				bSetColorsHooked = true;
				Log("code[C_SETKITATTRIBUTES] HOOKED at code[C_SETKITATTRIBUTES_CS5]");

				ptr = (DWORD*)(code[C_SETKITATTRIBUTES_CS6] + 1);
				ptr[0] = (DWORD)JuceFinalSetKitAttributes - (DWORD)(code[C_SETKITATTRIBUTES_CS6] + 5);
				bSetColorsHooked = true;
				Log("code[C_SETKITATTRIBUTES] HOOKED at code[C_SETKITATTRIBUTES_CS6]");

				ptr = (DWORD*)(code[C_SETKITATTRIBUTES_CS7] + 1);
				ptr[0] = (DWORD)JuceEditModeSetKitAttributes - (DWORD)(code[C_SETKITATTRIBUTES_CS7] + 5);
				bSetColorsHooked = true;
				Log("code[C_SETKITATTRIBUTES] HOOKED at code[C_SETKITATTRIBUTES_CS7]");

				ptr = (DWORD*)(code[C_SETKITATTRIBUTES_CS8] + 1);
				ptr[0] = (DWORD)JuceEditModeSetKitAttributes - (DWORD)(code[C_SETKITATTRIBUTES_CS8] + 5);
				bSetColorsHooked = true;
				Log("code[C_SETKITATTRIBUTES] HOOKED at code[C_SETKITATTRIBUTES_CS8]");
			}

			// hook code[C_FINISHKITPICK]
			if (code[C_FINISHKITPICK] != 0 && code[C_FINISHKITPICK_CS] != 0)
			{
				DWORD* ptr = (DWORD*)(code[C_FINISHKITPICK_CS] + 1);
				ptr[0] = (DWORD)JuceFinishKitPick - (DWORD)(code[C_FINISHKITPICK_CS] + 5);
				bFinishKitPickHooked = true;
				Log("code[C_FINISHKITPICK] HOOKED at code[C_FINISHKITPICK_CS]");
			}

			// hook code[C_UNPACK]
			if (code[C_UNPACK] != 0 && code[C_UNPACK_CS] != 0)
			{
				DWORD* ptr = (DWORD*)(code[C_UNPACK_CS] + 1);
				ptr[0] = (DWORD)JuceUnpack - (DWORD)(code[C_UNPACK_CS] + 5);
				bUnpackHooked = true;
				Log("code[C_UNPACK] HOOKED at code[C_UNPACK_CS]");
			}

			// hook code[C_ALLOCMEM]
			if (code[C_ALLOCMEM] != 0 && code[C_ALLOCMEM_CS] != 0)
			{
				DWORD* ptr = (DWORD*)(code[C_ALLOCMEM_CS] + 1);
				ptr[0] = (DWORD)JuceAllocMem - (DWORD)(code[C_ALLOCMEM_CS] + 5);
				bAllocMemHooked = true;
				Log("code[C_ALLOCMEM] HOOKED at code[C_ALLOCMEM_CS]");
			}

			// hook code[C_RESETFLAGS]
			if (code[C_RESETFLAGS] != 0 && code[C_RESETFLAGS_CS] != 0)
			{
				DWORD* ptr = (DWORD*)(code[C_RESETFLAGS_CS] + 1);
				ptr[0] = (DWORD)JuceResetFlags - (DWORD)(code[C_RESETFLAGS_CS] + 5);
				bResetFlagsHooked = true;
				Log("code[C_RESETFLAGS] HOOKED at code[C_RESETFLAGS_CS]");
			}

			// Load KDB database
			LoadKDB();

			// create font instance
			g_font = new CD3DFont( _T("Arial"), 10, D3DFONT_BOLD);
			TRACE2("g_font = %08x" , (DWORD)g_font);
			//ZeroMemory(g_ballName, 255);
			//lstrcpy(g_ballName, "Default");
		}
		else
		{
			Log("Game version not recognized. Nothing is hooked");
		}
	}
	else Log("Code section is not writeable.");

	// allocate a buffer to hold big texture.
	g_lastDecodedKit = (BYTE*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 0x28000);
	g_palette = (PALETTEENTRY*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 0x400);

	// prepare team info structurs
	ZeroMemory(g_teamInfo, sizeof(TEAMBUFFERINFO) * 256);

	ZeroMemory(g_afsFileName, sizeof(BUFLEN));
	lstrcpy(g_afsFileName, mydir);
	lstrcat(g_afsFileName, "..\\dat\\0_text.afs");

	AFSITEMINFO itemInfo[256*5]; // each team has 5 kit-files.
	ZeroMemory(itemInfo, sizeof(AFSITEMINFO) * 5 * 256);

	g_numTeams = data[NUMTEAMS];

	Log("Calculating kit offsets");
	DWORD result = GetKitInfo(g_afsFileName, itemInfo, g_numTeams * 5);
	if (result != AFS_OK)
		LogWithString("ERROR: %s", GetAfsErrorText(result));

	for (int i=0; i<g_numTeams; i++)
	{
		sprintf(g_teamInfo[i].gaFile, "uni%03dga.bin", i);
		sprintf(g_teamInfo[i].gbFile, "uni%03dgb.bin", i);
		sprintf(g_teamInfo[i].paFile, "uni%03dpa.bin", i);
		sprintf(g_teamInfo[i].pbFile, "uni%03dpb.bin", i);
		sprintf(g_teamInfo[i].vgFile, "uni%03dvg.bin", i);

		memcpy(&(g_teamInfo[i].ga), &(itemInfo[i*5]), sizeof(AFSITEMINFO));
		memcpy(&(g_teamInfo[i].gb), &(itemInfo[i*5 + 1]), sizeof(AFSITEMINFO));
		memcpy(&(g_teamInfo[i].pa), &(itemInfo[i*5 + 2]), sizeof(AFSITEMINFO));
		memcpy(&(g_teamInfo[i].pb), &(itemInfo[i*5 + 3]), sizeof(AFSITEMINFO));
		memcpy(&(g_teamInfo[i].vg), &(itemInfo[i*5 + 4]), sizeof(AFSITEMINFO));
	}

	// We need to treat bibs differently, and disable texture
	// boost for them, because by the time texture boost needs to
	// be performed, the colored textures are not yet available.
	Log("Calculating bibs offset");
	DWORD base;
	ZeroMemory(&g_bibs, sizeof(AFSITEMINFO));
	result = GetItemInfo(g_afsFileName, "unibibs.bin", &g_bibs, &base);
	if (result != AFS_OK)
		LogWithString("ERROR: %s", GetAfsErrorText(result));

	TRACE2("The bibs offset is: %08x", g_bibs.dwOffset); 

	TRACE2("top: %08x", g_teamInfo[0].ga.dwOffset);
	TRACE2("btm: %08x", g_teamInfo[g_numTeams-1].vg.dwOffset);

	// set flag
	g_teamOffsetsLoaded = TRUE;

	// Determine ball model offsets
	Log("Calculating ball model offsets");
	for (int b=0; b<BALL_MDLS_COUNT; b++)
	{
		ZeroMemory(&g_ballMdls[b], sizeof(AFSITEMINFO));
		result = GetItemInfo(g_afsFileName, ballMdls[b], &g_ballMdls[b], &base);
		if (result != AFS_OK)
			LogWithString("ERROR: %s", GetAfsErrorText(result));
		TRACE2("ball model offset is: %08x", g_ballMdls[b].dwOffset); 
	}

	// Determine ball texture offsets
	Log("Calculating ball texture offsets");
	for (b=0; b<BALL_TEXS_COUNT; b++)
	{
		ZeroMemory(&g_ballTexs[b], sizeof(AFSITEMINFO));
		result = GetItemInfo(g_afsFileName, ballTexs[b], &g_ballTexs[b], &base);
		if (result != AFS_OK)
			LogWithString("ERROR: %s", GetAfsErrorText(result));
		TRACE2("ball texture offset is: %08x", g_ballTexs[b].dwOffset); 
	}

	Log("Calculating etc_ee_tex.bin offset");
	ZeroMemory(&g_etcEeTex, sizeof(AFSITEMINFO));
	result = GetItemInfo(g_afsFileName, "etc_ee_tex.bin", &g_etcEeTex, &base);
	if (result != AFS_OK)
		LogWithString("ERROR: %s", GetAfsErrorText(result));

	// clear-out kit extras
	ZeroMemory(g_kitExtras, sizeof(KitEntry*)*0x500);

    // read configuration
    if (g_config.gameSpeed >= 0.0001)
    {
       SDLLHook Kernel32Hook = 
       {
          "KERNEL32.DLL",
          false, NULL,		// Default hook disabled, NULL function pointer.
          {
              { "QueryPerformanceFrequency", 
                  Override_QueryPerformanceFrequency },
              { NULL, NULL }
          }
       };
       HookAPICalls( &Kernel32Hook );
    }
    LogWithDouble("game.speed = %0.2f", (double)g_config.gameSpeed);
}

BOOL WINAPI Override_QueryPerformanceFrequency(
        LARGE_INTEGER *lpPerformanceFrequency)
{
    LARGE_INTEGER metric;
    BOOL result = QueryPerformanceFrequency(&metric);
    //LogWithTwoNumbers( 
    //        "(old) hi=%08x, lo=%08x", metric.HighPart, metric.LowPart);
    if (fabs(g_config.gameSpeed-1.0)>0.0001)
    {
        Log("Changing frequency");
        metric.HighPart /= g_config.gameSpeed;
        metric.LowPart /= g_config.gameSpeed;
    }
    //LogWithTwoNumbers(
    //        "(new) hi=%08x, lo=%08x", metric.HighPart, metric.LowPart);
    *lpPerformanceFrequency = metric;
    return result;
}

/**
 * Tracker for Direct3DCreate8 function.
 */
IDirect3D8* STDMETHODCALLTYPE JuceDirect3DCreate8(UINT sdkVersion)
{
	Log("JuceDirect3DCreate8 called.");
	BYTE* dest = (BYTE*)g_orgDirect3DCreate8;

	// put back saved code fragment
	dest[0] = g_codeFragment[0];
	*((DWORD*)(dest + 1)) = *((DWORD*)(g_codeFragment + 1));

	// initialize kitserver.
	InitKserv();

	// call the original function.
	IDirect3D8* result = g_orgDirect3DCreate8(sdkVersion);

	return result;
}


/*******************/
/* DLL Entry Point */
/*******************/
EXTERN_C BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	DWORD wbytes, procId; 

	if (dwReason == DLL_PROCESS_ATTACH)
	{
		hInst = hInstance;

		//texs.clear();

		/* determine process full path */
		ZeroMemory(processfile, MAX_FILEPATH);
		GetModuleFileName(NULL, processfile, MAX_FILEPATH);
		char* zero = processfile + lstrlen(processfile);
		char* p = zero; while ((p != processfile) && (*p != '\\')) p--;
		if (*p == '\\') p++;
		shortProcessfile = p;
		
		// save short filename without ".exe" extension.
		ZeroMemory(shortProcessfileNoExt, BUFLEN);
		char* ext = shortProcessfile + lstrlen(shortProcessfile) - 4;
		if (lstrcmpi(ext, ".exe")==0) {
			memcpy(shortProcessfileNoExt, shortProcessfile, ext - shortProcessfile); 
		}
		else {
			lstrcpy(shortProcessfileNoExt, shortProcessfile);
		}

		/* determine process handle */
		hProc = GetModuleHandle(NULL);

		/* determine my directory */
		ZeroMemory(mydir, BUFLEN);
		ZeroMemory(myfile, BUFLEN);
		GetModuleFileName(hInst, myfile, MAX_FILEPATH);
		lstrcpy(mydir, myfile);
		char *q = mydir + lstrlen(mydir);
		while ((q != mydir) && (*q != '\\')) { *q = '\0'; q--; }

		// read configuration
		char cfgFile[BUFLEN];
		ZeroMemory(cfgFile, BUFLEN);
		lstrcpy(cfgFile, mydir); 
		lstrcat(cfgFile, CONFIG_FILE);

		ReadConfig(&g_config, cfgFile);

		// adjust kdbDir, if it is specified as relative path
		if (g_config.kdbDir[0] == '.')
		{
			// it's a relative path. Therefore do it relative to mydir
			char temp[BUFLEN];
			ZeroMemory(temp, BUFLEN);
			lstrcpy(temp, mydir); 
			lstrcat(temp, g_config.kdbDir);
			ZeroMemory(g_config.kdbDir, BUFLEN);
			lstrcpy(g_config.kdbDir, temp);
		}

		/* open log file, specific for this process */
		ZeroMemory(logName, BUFLEN);
		lstrcpy(logName, mydir);
		lstrcat(logName, shortProcessfileNoExt); 
		lstrcat(logName, ".log");

		OpenLog(logName);

		Log("Log started.");
		LogWithNumber("g_config.debug = %d", g_config.debug);

		// Put a JMP-hook on Direct3DCreate8
		Log("JMP-hooking Direct3DCreate8...");
        HMODULE hD3D8 = GetModuleHandle("d3d8");
		g_orgDirect3DCreate8 = (PFNDIRECT3DCREATE8PROC)GetProcAddress(hD3D8, "Direct3DCreate8");

		// unconditional JMP to relative address is 5 bytes.
		g_jmp[0] = 0xe9;
		DWORD addr = (DWORD)JuceDirect3DCreate8 - (DWORD)g_orgDirect3DCreate8 - 5;
		TRACE2("JMP %08x", addr);
		memcpy(g_jmp + 1, &addr, sizeof(DWORD));

		memcpy(g_codeFragment, g_orgDirect3DCreate8, 5);
		DWORD newProtection = PAGE_EXECUTE_READWRITE;
		if (VirtualProtect(g_orgDirect3DCreate8, 8, newProtection, &g_savedProtection))
		{
			memcpy(g_orgDirect3DCreate8, g_jmp, 5);
			Log("JMP-hook planted.");
		}
	}

	else if (dwReason == DLL_PROCESS_DETACH)
	{
		Log("DLL detaching...");

		// free test uniform memory
		if (g_tex) HeapFree(GetProcessHeap(), 0, g_tex);

		// free ball buffers
		FreeBall();

		// unhook code[C_RESETFLAGS]
		if (bResetFlagsHooked)
		{
			DWORD* ptr = (DWORD*)(code[C_RESETFLAGS_CS] + 1);
			ptr[0] = (DWORD)code[C_RESETFLAGS] - (DWORD)(code[C_RESETFLAGS_CS] + 5);
			Log("code[C_RESETFLAGS] UNHOOKED");
		}

		// unhook code[C_ALLOCMEM]
		if (bAllocMemHooked)
		{
			DWORD* ptr = (DWORD*)(code[C_ALLOCMEM_CS] + 1);
			ptr[0] = (DWORD)code[C_ALLOCMEM] - (DWORD)(code[C_ALLOCMEM_CS] + 5);
			Log("code[C_ALLOCMEM] UNHOOKED");
		}

		// unhook code[C_UNPACK]
		if (bUnpackHooked)
		{
			DWORD* ptr = (DWORD*)(code[C_UNPACK_CS] + 1);
			ptr[0] = (DWORD)code[C_UNPACK] - (DWORD)(code[C_UNPACK_CS] + 5);
			Log("code[C_UNPACK] UNHOOKED");
		}

		// unhook code[C_FINISHKITPICK]
		if (bFinishKitPickHooked)
		{
			DWORD* ptr = (DWORD*)(code[C_FINISHKITPICK_CS] + 1);
			ptr[0] = (DWORD)code[C_FINISHKITPICK] - (DWORD)(code[C_FINISHKITPICK_CS] + 5);
			Log("code[C_FINISHKITPICK] UNHOOKED");
		}

		// unhook code[C_UNIDECODE]
		if (bUniDecodeHooked)
		{
			DWORD* ptr = (DWORD*)(code[C_UNIDECODE_CS] + 1);
			ptr[0] = (DWORD)code[C_UNIDECODE] - (DWORD)(code[C_UNIDECODE_CS] + 5);
			Log("code[C_UNIDECODE] UNHOOKED at C_UNIDECODE_CS");
		}
		if (bUniDecodeHooked)
		{
			DWORD* ptr = (DWORD*)(code[C_UNIDECODE_CS2] + 1);
			ptr[0] = (DWORD)code[C_UNIDECODE] - (DWORD)(code[C_UNIDECODE_CS2] + 5);
			Log("code[C_UNIDECODE] UNHOOKED at C_UNIDECODE_CS2");
		}


		// unhook code[C_SETKITATTRIBUTES]
		if (bSetColorsHooked)
		{
			DWORD* ptr = (DWORD*)(code[C_SETKITATTRIBUTES_CS1] + 1);
			ptr[0] = (DWORD)code[C_SETKITATTRIBUTES] - (DWORD)(code[C_SETKITATTRIBUTES_CS1] + 5);
			Log("code[C_SETKITATTRIBUTES] UNHOOKED at call-site 1");

			ptr = (DWORD*)(code[C_SETKITATTRIBUTES_CS2] + 1);
			ptr[0] = (DWORD)code[C_SETKITATTRIBUTES] - (DWORD)(code[C_SETKITATTRIBUTES_CS2] + 5);
			Log("code[C_SETKITATTRIBUTES] UNHOOKED at call-site 2");

			ptr = (DWORD*)(code[C_SETKITATTRIBUTES_CS3] + 1);
			ptr[0] = (DWORD)code[C_SETKITATTRIBUTES] - (DWORD)(code[C_SETKITATTRIBUTES_CS3] + 5);
			Log("code[C_SETKITATTRIBUTES] UNHOOKED at call-site 3");

			ptr = (DWORD*)(code[C_SETKITATTRIBUTES_CS4] + 1);
			ptr[0] = (DWORD)code[C_SETKITATTRIBUTES] - (DWORD)(code[C_SETKITATTRIBUTES_CS4] + 5);
			Log("code[C_SETKITATTRIBUTES] UNHOOKED at call-site 4");

			ptr = (DWORD*)(code[C_SETKITATTRIBUTES_CS5] + 1);
			ptr[0] = (DWORD)code[C_SETKITATTRIBUTES] - (DWORD)(code[C_SETKITATTRIBUTES_CS5] + 5);
			Log("code[C_SETKITATTRIBUTES] UNHOOKED at call-site 5");

			ptr = (DWORD*)(code[C_SETKITATTRIBUTES_CS6] + 1);
			ptr[0] = (DWORD)code[C_SETKITATTRIBUTES] - (DWORD)(code[C_SETKITATTRIBUTES_CS6] + 5);
			Log("code[C_SETKITATTRIBUTES] UNHOOKED at call-site 6");

			ptr = (DWORD*)(code[C_SETKITATTRIBUTES_CS7] + 1);
			ptr[0] = (DWORD)code[C_SETKITATTRIBUTES] - (DWORD)(code[C_SETKITATTRIBUTES_CS7] + 5);
			Log("code[C_SETKITATTRIBUTES] UNHOOKED at call-site 7");

			ptr = (DWORD*)(code[C_SETKITATTRIBUTES_CS8] + 1);
			ptr[0] = (DWORD)code[C_SETKITATTRIBUTES] - (DWORD)(code[C_SETKITATTRIBUTES_CS8] + 5);
			Log("code[C_SETKITATTRIBUTES] UNHOOKED at call-site 8");
		}

		/*
		// unhook code[C_UNIDECRYPT]
		if (bUniDecryptHooked)
		{
			DWORD* ptr = (DWORD*)(code[C_UNIDECRYPT_CS] + 1);
			ptr[0] = (DWORD)code[C_UNIDECRYPT] - (DWORD)(code[C_UNIDECRYPT_CS] + 5);
			Log("code[C_UNIDECRYPT] UNHOOKED");
		}
		*/

		// unhook SetFilePointer
		if (bSetFilePointerHooked)
		{
			BYTE* bptr = (BYTE*)code[C_SETFILEPOINTER_CS];
			memcpy(bptr, g_sfpCode, 6);
			Log("SetFilePointer UNHOOKED");
		}

		// free the texture kit buffer
		SAFEFREE(g_lastDecodedKit);
		SAFEFREE(g_palette);

		Log("kit and palette memory freed.");

		/* uninstall keyboard hook */
		UninstallKeyboardHook();

		/* restore original pointers */
		RestoreDeviceMethods();

		Log("Device methods restored.");

		InvalidateDeviceObjects(g_device);
		Log("InvalideDeviceObjects done.");
		DeleteDeviceObjects(g_device);
		Log("DeleteDeviceObjects done.");

		/* release interfaces */
		if (g_rgbBuf != NULL)
		{
			HeapFree(procHeap, 0, g_rgbBuf);
			g_rgbBuf = NULL;
			Log("g_rgbBuf freed.");
		}

		// unload KDB
		TRACE("Unloading KDB...");
		kdbUnload(kDB);
		Log("KDB unloaded.");

		SAFE_DELETE( g_font );
		TRACE("g_font SAFE_DELETEd.");

		/* close specific log file */
		Log("Closing log.");
		CloseLog();
	}

	return TRUE;    /* ok */
}

// helper function
// returns TRUE if specified slot contains edited kit.
BOOL IsEditedKit(int slot) 
{
	KitSlot* kitSlot = (KitSlot*)(data[ANOTHER_KIT_SLOTS] + slot*sizeof(KitSlot));
	return kitSlot->isEdited;
}

/**************************************************************** 
 * WH_KEYBOARD hook procedure                                   *
 ****************************************************************/ 

EXTERN_C _declspec(dllexport) LRESULT CALLBACK KeyboardProc(int code, WPARAM wParam, LPARAM lParam)
{
	//TRACE2("KeyboardProc CALLED. code = %d", code);

    if (code < 0) // do not process message 
        return CallNextHookEx(g_hKeyboardHook, code, wParam, lParam); 

	switch (code)
	{
		case HC_ACTION:
			/* process the key events */
			if (lParam & 0x80000000)
			{
				if (wParam == g_config.vKeyHomeKit && GoodTeamId(g_currTeams[0]) && !IsEditedKit(1))
				{
					g_triggerLoad3rdKit = 1;

					// pick next extra kit
					BYTE strip = ((BYTE*)data[TEAM_STRIPS])[0];
					int kitId = (strip & 0x01) ? g_currTeams[0] * 5 + 3 : g_currTeams[0] * 5 + 2;
					g_kitExtras[kitId] = (g_kitExtras[kitId] == NULL) ? 
						kDB->players[g_currTeams[0]].extra : g_kitExtras[kitId]->next;
				}
				else if (wParam == g_config.vKeyAwayKit && GoodTeamId(g_currTeams[1]) && !IsEditedKit(3))
				{
					g_triggerLoad3rdKit = 2;

					// pick next extra kit
					BYTE strip = ((BYTE*)data[TEAM_STRIPS])[1];
					int kitId = (strip & 0x01) ? g_currTeams[1] * 5 + 3 : g_currTeams[1] * 5 + 2;
					g_kitExtras[kitId] = (g_kitExtras[kitId] == NULL) ? 
						kDB->players[g_currTeams[1]].extra : g_kitExtras[kitId]->next;
				}
				else if (wParam == g_config.vKeyGKHomeKit && GoodTeamId(g_currTeams[0]) && !IsEditedKit(0))
				{
					g_triggerLoad3rdKit = 3;

					// pick next extra GK kit
					int kitId = g_currTeams[0] * 5;
					g_kitExtras[kitId] = (g_kitExtras[kitId] == NULL) ? 
						kDB->goalkeepers[g_currTeams[0]].extra : g_kitExtras[kitId]->next;
					g_kitExtras[kitId + 1] = g_kitExtras[kitId];
				}
				else if (wParam == g_config.vKeyGKAwayKit && GoodTeamId(g_currTeams[1]) && !IsEditedKit(2))
				{
					g_triggerLoad3rdKit = 4;

					// pick next extra GK kit
					int kitId = g_currTeams[1] * 5;
					g_kitExtras[kitId] = (g_kitExtras[kitId] == NULL) ? 
						kDB->goalkeepers[g_currTeams[1]].extra : g_kitExtras[kitId]->next;
					g_kitExtras[kitId + 1] = g_kitExtras[kitId];
				}
				else if (wParam == g_config.vKeyPrevBall)
				{
					// pick previous extra ball
					g_balls = (g_balls == NULL) ? kDB->ballsLast : g_balls->prev;

					// load ball
					FreeBall();
					if (g_balls != NULL && g_balls->ball != NULL)
						LoadBall(g_balls->ball);
				}
				else if (wParam == g_config.vKeyNextBall)
				{
					// pick next extra ball
					g_balls = (g_balls == NULL) ? kDB->ballsFirst : g_balls->next;

					// load ball
					FreeBall();
					if (g_balls != NULL && g_balls->ball != NULL)
						LoadBall(g_balls->ball);
				}
				else if (wParam == g_config.vKeyRandomBall)
				{
					// pick a random extra ball
					LARGE_INTEGER num;
					QueryPerformanceCounter(&num);
					int iterations = num.LowPart % MAX_ITERATIONS;
					TRACE2("Random iterations: %d", iterations);
					for (int j=0; j < iterations; j++)
					{
						g_balls = (g_balls == NULL) ? kDB->ballsFirst : g_balls->next;
					}

					// load ball
					FreeBall();
					if (g_balls != NULL && g_balls->ball != NULL)
						LoadBall(g_balls->ball);
				}
				else if (wParam == g_config.vKeyResetBall)
				{
					// reset ball selection back to "game choice"
					g_balls = NULL;

					// free ball
					FreeBall();
				}
			}
			break;
	}

	// We must pass the all messages on to CallNextHookEx.
	return CallNextHookEx(g_hKeyboardHook, code, wParam, lParam);
}

/* Set debug flag */
EXTERN_C LIBSPEC void SetDebug(DWORD flag)
{
	//LogWithNumber("Setting g_config.debug flag to %d", flag);
	g_config.debug = flag;
}

EXTERN_C LIBSPEC DWORD GetDebug(void)
{
	return g_config.debug;
}

/* Set key */
EXTERN_C LIBSPEC void SetKey(int whichKey, int code)
{
	switch (whichKey)
	{
		case VKEY_HOMEKIT: g_config.vKeyHomeKit = code; break;
		case VKEY_AWAYKIT: g_config.vKeyAwayKit = code; break;
		case VKEY_GKHOMEKIT: g_config.vKeyGKHomeKit = code; break;
		case VKEY_GKAWAYKIT: g_config.vKeyGKAwayKit = code; break;
		case VKEY_PREV_BALL: g_config.vKeyPrevBall = code; break;
		case VKEY_NEXT_BALL: g_config.vKeyNextBall = code; break;
		case VKEY_RANDOM_BALL: g_config.vKeyRandomBall = code; break;
		case VKEY_RESET_BALL: g_config.vKeyResetBall = code; break;
	}
}

EXTERN_C LIBSPEC int GetKey(int whichKey)
{
	switch (whichKey)
	{
		case VKEY_HOMEKIT: return g_config.vKeyHomeKit;
		case VKEY_AWAYKIT: return g_config.vKeyAwayKit;
		case VKEY_GKHOMEKIT: return g_config.vKeyGKHomeKit;
		case VKEY_GKAWAYKIT: return g_config.vKeyGKAwayKit;
		case VKEY_PREV_BALL: return g_config.vKeyPrevBall;
		case VKEY_NEXT_BALL: return g_config.vKeyNextBall;
		case VKEY_RANDOM_BALL: return g_config.vKeyRandomBall;
		case VKEY_RESET_BALL: return g_config.vKeyResetBall;
	}
	return -1;
}

/* Set kdb dir */
EXTERN_C LIBSPEC void SetKdbDir(char* kdbDir)
{
	if (kdbDir == NULL) return;
	ZeroMemory(g_config.kdbDir, BUFLEN);
	lstrcpy(g_config.kdbDir, kdbDir);
}

EXTERN_C LIBSPEC void GetKdbDir(char* kdbDir)
{
	if (kdbDir == NULL) return;
	ZeroMemory(kdbDir, BUFLEN);
	lstrcpy(kdbDir, g_config.kdbDir);
}

/**
 * Load the KDB database.
 */
void LoadKDB()
{
	kDB = kdbLoad(g_config.kdbDir);
	if (kDB != NULL)
	{
		//print kDB
		TRACE2("KDB player count: %d", kDB->playerCount);
		TRACE2("KDB goalkeeper count: %d", kDB->goalkeeperCount);

		BallEntry* p = kDB->ballsFirst;
		if (p != NULL) TRACE2("KDB balls [%d]:", kDB->ballCount);
		while (p != NULL)
		{
			TRACE4("ball: %s", p->ball->name);
			p = p->next;
		}

		// initialize ball-cycle
		FreeBall(); // free any currently loaded ball
		g_balls = NULL;

		TRACE("KDB loaded.");
	}
	else
	{
		TRACE("Problem loading KDB (kDB == NULL).");
	}
}

/**
 * Utility function to dump any data to file.
 */
void DumpData(BYTE* addr, DWORD size)
{
	static int count = 0;

	// prepare filename
	char name[BUFLEN];
	ZeroMemory(name, BUFLEN);
	sprintf(name, "%s%s%02d.dat", mydir, "dd", count++);

	FILE* f = fopen(name, "wb");
	if (f != NULL)
	{
		fwrite(addr, size, 1, f);
		fclose(f);
	}
}

/**
 * Utility function to dump any data to a named file.
 */
void DumpData(BYTE* addr, DWORD size, char* filename)
{
	// prepare filename
	char name[BUFLEN];
	ZeroMemory(name, BUFLEN);
	sprintf(name, "%s%s", mydir, filename);

	FILE* f = fopen(name, "wb");
	if (f != NULL)
	{
		fwrite(addr, size, 1, f);
		fclose(f);
	}
}

/**
 * This function saves the textures from decoded
 * uniform buffer into two BMP files: 512x256x8 and 256x128x8
 * (with palette).
 */
void DumpUniforms(BYTE* buffer)
{
	static int count = 0;

	// prepare 1st filename
	char name[BUFLEN];
	ZeroMemory(name, BUFLEN);
	sprintf(name, "%s%s%02d.bmp", mydir, "du", count++);

	SaveAs8bitBMP(name, buffer + 0x480, buffer + 0x80, 512, 256);

	/*
	// prepare 2nd filename
	ZeroMemory(name, BUFLEN);
	sprintf(name, "%s%s%02d.bmp", mydir, "du", count++);

	SaveAs8bitBMP(name, buffer + 0x20480, buffer + 0x80, 256, 128);

	// prepare 3rd filename
	ZeroMemory(name, BUFLEN);
	sprintf(name, "%s%s%02d.bmp", mydir, "du", count++);

	SaveAs8bitBMP(name, buffer + 0x28480, buffer + 0x80, 128, 64);
	*/
}

/**
 * This function saves the texture from decoded
 * ball buffer into BMP file: 256x256x8 (with palette).
 */
void DumpBall(BYTE* buffer)
{
	static int count = 0;

	// prepare 1st filename
	char name[BUFLEN];
	ZeroMemory(name, BUFLEN);
	sprintf(name, "%s%s%02d.bmp", mydir, "bdu", count++);

	//SaveAs8bitBMP(name, buffer + 0x480, buffer + 0x80, 256, 256);
	SaveAs8bitBMP(name, buffer);
}

// Load texture. Returns the size of loaded texture
DWORD LoadTexture(BYTE** tex, char* filename)
{
	TRACE4("LoadTexture: loading %s", filename);

	FILE* f = fopen(filename, "rb");
	if (f == NULL)
	{
		TRACE("LoadTexture: ERROR - unable to open file.");
		return 0;
	}
	BITMAPFILEHEADER bfh;
	fread(&bfh, sizeof(BITMAPFILEHEADER), 1, f);
	DWORD size = bfh.bfSize;

	TRACE2("LoadTexture: filesize = %08x", size);

	*tex = (BYTE*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
	fseek(f, 0, SEEK_SET);
	fread(*tex, size, 1, f);
	fclose(f);

	TRACE("LoadTexture: done");
	return size;
}

// Substiture kit textures with data from BMPs.
void ApplyTextures(BYTE* orgKit)
{
	if (g_texSize == 0) 
		return;

	// copy palette from tex0
	BITMAPINFOHEADER* bih = (BITMAPINFOHEADER*)(g_tex + sizeof(BITMAPFILEHEADER));
	BITMAPFILEHEADER* bfh = (BITMAPFILEHEADER*)g_tex;
	DWORD bitsOff = bfh->bfOffBits;
	DWORD palOff = bih->biSize + sizeof(BITMAPFILEHEADER);

	TRACE2("bitsOff = %08x", bitsOff);
	TRACE2("palOff  = %08x", palOff);

	for (int bank=0; bank<8; bank++)
	{
		memcpy(orgKit + 0x80 + bank*32*4,        g_tex + palOff + bank*32*4,        8*4);
		memcpy(orgKit + 0x80 + bank*32*4 + 16*4, g_tex + palOff + bank*32*4 + 8*4,  8*4);
		memcpy(orgKit + 0x80 + bank*32*4 + 8*4,  g_tex + palOff + bank*32*4 + 16*4, 8*4);
		memcpy(orgKit + 0x80 + bank*32*4 + 24*4, g_tex + palOff + bank*32*4 + 24*4, 8*4);
	}
	// swap R and B
	for (int i=0; i<0x100; i++) 
	{
		BYTE blue = orgKit[0x80 + i*4];
		BYTE red = orgKit[0x80 + i*4 + 2];
		orgKit[0x80 + i*4] = red;
		orgKit[0x80 + i*4 + 2] = blue;
	}
	TRACE("Palette copied.");

	int k, m, j, w;
	int height, width;
	int imageWidth;

	// copy tex0
	width = imageWidth = 512; height = 256;
	for (k=0, m=bih->biHeight-1; k<height, m>=bih->biHeight - height; k++, m--)
	{
		memcpy(orgKit + 0x480 + k*width, g_tex + bitsOff + m*imageWidth, width);
	}
	TRACE("Texture 512x256 replaced.");

	if (bih->biHeight == 384) // MipMap kit 
	{
		// copy tex1
		width = 256; height = 128;
		for (k=0, m=127; k<height, m>=0; k++, m--)
		{
			memcpy(orgKit + 0x20480 + k*width, g_tex + bitsOff + m*imageWidth, width);
		}
		TRACE("Texture 256x128 replaced.");

		// copy tex2
		width = 128; height = 64;
		for (k=0, m=127; k<height, m>=64; k++, m--)
		{
			memcpy(orgKit + 0x28480 + k*width, g_tex + bitsOff + 256 + m*imageWidth, width);
		}
		TRACE("Texture 128x64 replaced.");
	}
	else // Single-texture kit
	{
		// resample the texture at half-size in each dimension
		int W = width/2, H = height/2;
		for (k=0, m=height-2; k<H, m>=0; k++, m-=2)
		{
			for (w=0, j=0; w<W, j<width-1; w++, j+=2)
			{
				orgKit[0x20480 + k*W + w] = g_tex[bitsOff + m*width + j];
			}
		}
		TRACE("Texture 512x256 resampled to 256x128.");
		TRACE("Texture 256x128 replaced.");

		// resample the texture at 1/4-size in each dimension
		W = width/4, H = height/4;
		for (k=0, m=height-4; k<H, m>=0; k++, m-=4)
		{
			for (w=0, j=0; w<W, j<width-3; w++, j+=4)
			{
				orgKit[0x28480 + k*W + w] = g_tex[bitsOff + m*width + j];
			}
		}
		TRACE("Texture 256x128 resampled to 128x64.");
		TRACE("Texture 128x64 replaced.");
	}
}

// Substiture glove textures with data from BMPs.
void ApplyGloveTextures(BYTE* orgKit)
{
	if (g_texSize == 0) 
		return;

	// copy palette from tex0
	BITMAPINFOHEADER* bih = (BITMAPINFOHEADER*)(g_tex + sizeof(BITMAPFILEHEADER));
	BITMAPFILEHEADER* bfh = (BITMAPFILEHEADER*)g_tex;
	DWORD bitsOff = bfh->bfOffBits;
	DWORD palOff = bih->biSize + sizeof(BITMAPFILEHEADER);

	TRACE2("bitsOff = %08x", bitsOff);
	TRACE2("palOff  = %08x", palOff);

	for (int bank=0; bank<8; bank++)
	{
		memcpy(orgKit + 0x80 + bank*32*4,        g_tex + palOff + bank*32*4,        8*4);
		memcpy(orgKit + 0x80 + bank*32*4 + 16*4, g_tex + palOff + bank*32*4 + 8*4,  8*4);
		memcpy(orgKit + 0x80 + bank*32*4 + 8*4,  g_tex + palOff + bank*32*4 + 16*4, 8*4);
		memcpy(orgKit + 0x80 + bank*32*4 + 24*4, g_tex + palOff + bank*32*4 + 24*4, 8*4);
	}
	// swap R and B
	for (int i=0; i<0x100; i++) 
	{
		BYTE blue = orgKit[0x80 + i*4];
		BYTE red = orgKit[0x80 + i*4 + 2];
		orgKit[0x80 + i*4] = red;
		orgKit[0x80 + i*4 + 2] = blue;
	}
	TRACE("Palette copied.");

	int k, m, j, w;
	int height, width;
	int imageWidth;

	// copy tex0
	width = imageWidth = 64; height = 128;
	for (k=0, m=bih->biHeight-1; k<height, m>=bih->biHeight - height; k++, m--)
	{
		memcpy(orgKit + 0x480 + k*width, g_tex + bitsOff + m*imageWidth, width);
	}
	TRACE("Texture 64x128 replaced.");

	if (bih->biHeight == 192) // MipMap kit 
	{
		// copy tex1
		width = 32; height = 64;
		for (k=0, m=63; k<height, m>=0; k++, m--)
		{
			memcpy(orgKit + 0x2480 + k*width, g_tex + bitsOff + m*imageWidth, width);
		}
		TRACE("Texture 32x64 replaced.");
	}
	else // Single-texture kit
	{
		// resample the texture at half-size in each dimension
		int W = width/2, H = height/2;
		for (k=0, m=height-2; k<H, m>=0; k++, m-=2)
		{
			for (w=0, j=0; w<W, j<width-1; w++, j+=2)
			{
				orgKit[0x2480 + k*W + w] = g_tex[bitsOff + m*width + j];
			}
		}
		TRACE("Texture 64x128 resampled to 32x64.");
		TRACE("Texture 32x64 replaced.");
	}
}

// Substiture ball texture with data from BMP.
void ApplyBallTexture(BYTE* orgBall, BYTE* ballTex)
{
	if (ballTex == 0) 
		return;

	// copy palette from bitmap
	BITMAPINFOHEADER* bih = (BITMAPINFOHEADER*)(ballTex + sizeof(BITMAPFILEHEADER));
	BITMAPFILEHEADER* bfh = (BITMAPFILEHEADER*)ballTex;
	DWORD bitsOff = bfh->bfOffBits;
	DWORD palOff = bih->biSize + sizeof(BITMAPFILEHEADER);

	TRACE2("bitsOff = %08x", bitsOff);
	TRACE2("palOff  = %08x", palOff);

	for (int bank=0; bank<8; bank++)
	{
		memcpy(orgBall + 0x80 + bank*32*4,        ballTex + palOff + bank*32*4,        8*4);
		memcpy(orgBall + 0x80 + bank*32*4 + 16*4, ballTex + palOff + bank*32*4 + 8*4,  8*4);
		memcpy(orgBall + 0x80 + bank*32*4 + 8*4,  ballTex + palOff + bank*32*4 + 16*4, 8*4);
		memcpy(orgBall + 0x80 + bank*32*4 + 24*4, ballTex + palOff + bank*32*4 + 24*4, 8*4);
	}
	// swap R and B
	for (int i=0; i<0x100; i++) 
	{
		BYTE blue = orgBall[0x80 + i*4];
		BYTE red = orgBall[0x80 + i*4 + 2];
		orgBall[0x80 + i*4] = red;
		orgBall[0x80 + i*4 + 2] = blue;
	}
	TRACE("Palette copied.");

	// copy the texture and shuffle it
	ShuffleBallTexture(orgBall, ballTex);
}

BOOL SignExists(DWORD sig, char* filename)
{
	int k = -1;
	for (int i=0; i<8; i++)
		if (sig == g_sign[i]) { k = i; break; }

	if (k == -1) return FALSE; // not found

	ZeroMemory(filename, BUFLEN);
	lstrcpy(filename, g_filename[i]);
	return TRUE;
}

/**
 * Tracker for IDirect3DTexture8::GetSurfaceLevel method.
 */
HRESULT STDMETHODCALLTYPE JuceGetSurfaceLevel(IDirect3DTexture8* self, UINT level,
IDirect3DSurface8** ppSurfaceLevel)
{
	TRACE("JuceGetSurfaceLevel: CALLED.");
	TRACE2("JuceGetSurfaceLevel: texture(self) = %08x", (DWORD)self);
	TRACE2("JuceGetSurfaceLevel: level = %d", level);

	return g_orgGetSurfaceLevel(self, level, ppSurfaceLevel);
}

/**
 * Saves surface into a auto-named file.
 */
void DumpSurface(int count, IDirect3DSurface8* surf)
{
	char filename[BUFLEN];
	ZeroMemory(filename, BUFLEN);
	sprintf(filename, "%ssurf-%03d.bmp", mydir, count);
	D3DXSaveSurfaceToFile(filename, D3DXIFF_BMP, surf, NULL, NULL);
}

/**
 * Tracker for IDirect3DDevice8::CreateTexture method.
 */
HRESULT STDMETHODCALLTYPE JuceCreateTexture(IDirect3DDevice8* self, UINT width, UINT height,
UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool, IDirect3DTexture8** ppTexture)
{
	HRESULT res = S_OK;
	IDirect3DTexture8* texToBoost = NULL;

	TRACEX("JuceCreateTexture: (%dx%d), levels = %d", width, height, levels);
	
	// Boost the kit texture
	// We're assuming here that by the time next texture is created, the
	// medium and big textures are already fully prepared.
	if (g_config.useLargeTexture == 2 || (g_customTex && g_config.useLargeTexture == 1))
	{
		texToBoost = g_lastMediumTex;
	}
	// reset the pointer
	g_lastMediumTex = NULL;

	// Kit Texture Booster
	if (!g_isBibs && g_bigTextureAvailable &&
		(g_config.useLargeTexture == 2 || 
		(g_customTex && g_config.useLargeTexture == 1)))
	{
		if (width == 256 && height == 128 && levels == 2)
		{
			TRACE("JuceCreateTexture: Using LARGER middle texture for the kit.");

			UINT w = 512; UINT h = 256;
			res = g_orgCreateTexture(self, w, h, levels, usage, format, pool, ppTexture);
			g_lastMediumTex = *ppTexture;

			TRACE2("width  = %d", (DWORD)w);
			TRACE2("height = %d", (DWORD)h);
			TRACE2("levels = %d", (DWORD)levels);
			TRACE2("usage  = %d", (DWORD)usage);
			TRACE2("format = %d", (DWORD)format);
			TRACE2("pool   = %d", (DWORD)pool);

			TRACE2("med tex = %08x", (DWORD)g_lastMediumTex);
		}
	}

	// General case
	if (g_lastMediumTex == NULL)
		res = g_orgCreateTexture(self, width, height, levels, usage, format, pool, ppTexture);

	if (width == 512 && height == 256)
		TRACE2("BIG-tex (512x256): %08x", (DWORD)*ppTexture);

	/*
	if (g_orgGetSurfaceLevel == NULL)
	{
		TRACE("JuceCreateTexture: hooking GetSurfaceLevel");
		TRACE2("JuceCreateTexture: *ppTexture = %08x", (DWORD)*ppTexture);

		DWORD* tex = (DWORD*)(*ppTexture);
		DWORD* vtable = (DWORD*)(*tex);

		TRACE2("JuceCreateTexture: vtable = %08x", (DWORD)vtable);

		// hook GetSurfaceLevel method
		g_orgGetSurfaceLevel = (PFNGETSURFACELEVELPROC)vtable[15];
		TRACE2("g_orgGetSurfaceLevel = %08x", (DWORD)g_orgGetSurfaceLevel);

		DWORD oldProt;
		VirtualProtect(vtable + 15, sizeof(DWORD), PAGE_READWRITE, &oldProt); 
		vtable[15] = (DWORD)JuceGetSurfaceLevel;
		TRACE2("JuceGetSurfaceLevel = %08x", (DWORD)JuceGetSurfaceLevel);
	}

	// Dump all 512x256 texture for debugging.
	if (lastBig != NULL)
	{
		IDirect3DSurface8* s = NULL;
		lastBig->GetSurfaceLevel(0, &s);
		DumpSurface(bigCount++, s);
		lastBig = NULL;
	}
	if (width == 512 && height == 256)
		lastBig = *ppTexture;
	*/

	if (texToBoost != NULL)
	{
		// shift the surfaces.
		// MED,level 1 <- MED,level 0
		// MED,level 0 <- big texture buffer data

		HRESULT r1 = texToBoost->GetSurfaceLevel(0, &g_med0);
		TRACE2("g_med0 = %08x", (DWORD)g_med0);
		//DumpSurface(bigCount++, g_med0);
		HRESULT r2 = texToBoost->GetSurfaceLevel(1, &g_med1);
		TRACE2("g_med1 = %08x", (DWORD)g_med1);
		//DumpSurface(bigCount++, g_med1);

		if (SUCCEEDED(r1) && SUCCEEDED(r2))
		{
			// copy pixels
			RECT rect = {0, 0, 256, 128};
			if (SUCCEEDED(D3DXLoadSurfaceFromMemory(
							g_med1, NULL, NULL, 
							g_lastDecodedKit + 0x20000, D3DFMT_P8, 256, 
							g_palette, &rect, D3DX_FILTER_NONE, 0)))
			{
				TRACE("g_med1 <- g_med0 COMPLETE");
			}
			else
			{
				TRACE("g_med1 <- g_med0 FAILED");
			}

			RECT bigRect = {0, 0, 512, 256};
			if (SUCCEEDED(D3DXLoadSurfaceFromMemory(
							g_med0, NULL, NULL, 
							g_lastDecodedKit, D3DFMT_P8, 512, 
							g_palette, &bigRect, D3DX_FILTER_NONE, 0)))
			{
				TRACE("g_med0 <- buffer COMPLETE");
			}
			else
			{
				TRACE("g_med0 <- buffer FAILED");
			}

			//DumpSurface(bigCount++, g_med0);
		}

		if (SUCCEEDED(r1)) g_med0->Release();
		if (SUCCEEDED(r2)) g_med1->Release();

		// reset big texture flag
		g_bigTextureAvailable = FALSE;
	}

	return res;
}

/***
 * Tracker for IDirect3DDevice8::SetTextureStageState method
 */
HRESULT STDMETHODCALLTYPE JuceSetTextureStageState(IDirect3DDevice8* self, DWORD Stage,
D3DTEXTURESTAGESTATETYPE Type, DWORD Value)
{
	TRACE("JuceSetTextureStageState: CALLED.");
	HRESULT res = g_orgSetTextureStageState(self, Stage, Type, Value);
	return res;
}

/***
 * Tracker for IDirect3DDevice8::UpdateTexture method
 */
HRESULT STDMETHODCALLTYPE JuceUpdateTexture(IDirect3DDevice8* self,
IDirect3DBaseTexture8* pSrc, IDirect3DBaseTexture8* pDest)
{
	TRACE("JuceUpdateTexture: CALLED.");
	HRESULT res = g_orgUpdateTexture(self, pSrc, pDest);
	return res;
}

/**
 * Tracker for IDirect3DDevice8::SetTexture method.
 */
HRESULT STDMETHODCALLTYPE JuceSetTexture(IDirect3DDevice8* self, DWORD stage,
IDirect3DBaseTexture8* pTexture)
{
	TRACE("JuceSetTexture: CALLED.");
	TRACE2("JuceSetTexture: stage = %d", stage);
	TRACE2("JuceSetTexture: pTexture = %08x", (DWORD)pTexture);

	HRESULT res = g_orgSetTexture(self, stage, pTexture);
	return res;
}

/**
 * Checks if offset matches any of the kits offsets.
 */
BOOL IsKitOffset(DWORD offset)
{
	BOOL result = false;

	// first check: bibs
	if (g_bibs.dwOffset == offset)
		return true;

	// rough approximation (boundary) check:
	if (offset < g_teamInfo[0].ga.dwOffset || g_teamInfo[g_numTeams-1].vg.dwOffset < offset)
		return false;

	// precise check: check every kit
	for (int i=0; i<g_numTeams; i++) 
	{
		if (g_teamInfo[i].ga.dwOffset == offset) return true;
		if (g_teamInfo[i].gb.dwOffset == offset) return true;
		if (g_teamInfo[i].pa.dwOffset == offset) return true;
		if (g_teamInfo[i].pb.dwOffset == offset) return true;
		if (g_teamInfo[i].vg.dwOffset == offset) return true;
	}

	return false;
}

/**
 * This function monitors file pointer movement.
 * (which is then used by JuceUniDecode.)
 */
DWORD STDMETHODCALLTYPE JuceSetFilePointer(HANDLE handle, LONG offset, PLONG upper32, DWORD method)
{
	if (g_device == NULL)
	{
		// try to hook device methods
		DWORD* pdev = (DWORD*)(*((DWORD*)data[IDIRECT3DDEVICE8]));
		if (pdev != 0)
		{
			g_device = (IDirect3DDevice8*)pdev;
			TRACE2("g_device = %08x", (DWORD)g_device);

			DWORD* vtable = (DWORD*)(*((DWORD*)g_device));

			// hook CreateTexture method
			g_orgCreateTexture = (PFNCREATETEXTUREPROC)vtable[20];
			vtable[20] = (DWORD)JuceCreateTexture;

			TRACE2("g_orgCreateTexture = %08x", (DWORD)g_orgCreateTexture);
			TRACE2("JuceCreateTexture = %08x", (DWORD)JuceCreateTexture);

			/*
			// hook SetTexture method
			g_orgSetTexture = (PFNSETTEXTUREPROC)vtable[61];
			vtable[61] = (DWORD)JuceSetTexture;

			TRACE2("g_orgSetTexture = %08x", (DWORD)g_orgSetTexture);
			TRACE2("JuceSetTexture = %08x", (DWORD)JuceSetTexture);

			// hook SetTextureStageState method
			g_orgSetTextureStageState = (PFNSETTEXTURESTAGESTATEPROC)vtable[63];
			vtable[63] = (DWORD)JuceSetTextureStageState;

			TRACE2("g_orgSetTextureStageState = %08x", (DWORD)g_orgSetTextureStageState);
			TRACE2("JuceSetTextureStageState = %08x", (DWORD)JuceSetTextureStageState);

			// hook UpdateTexture method
			g_orgUpdateTexture = (PFNUPDATETEXTUREPROC)vtable[29];
			vtable[29] = (DWORD)JuceUpdateTexture;

			TRACE2("g_orgUpdateTexture = %08x", (DWORD)g_orgUpdateTexture);
			TRACE2("JuceUpdateTexture = %08x", (DWORD)JuceUpdateTexture);

			// hook Present method
			g_orgPresent = (PFNPRESENTPROC)vtable[15];
			vtable[15] = (DWORD)JucePresent;

			// hook Present method
			g_orgReset = (PFNRESETPROC)vtable[14];
			vtable[14] = (DWORD)JuceReset;

			TRACE2("g_orgPresent = %08x", (DWORD)g_orgPresent);
			TRACE2("JucePresent = %08x", (DWORD)JucePresent);

			// hook CopyRects method
			g_orgCopyRects = (PFNCOPYRECTSPROC)vtable[28];
			vtable[28] = (DWORD)JuceCopyRects;

			TRACE2("g_orgCopyRects = %08x", (DWORD)g_orgCopyRects);
			TRACE2("JuceCopyRects = %08x", (DWORD)JuceCopyRects);

			// hook ApplyStateBlock method
			g_orgApplyStateBlock = (PFNAPPLYSTATEBLOCKPROC)vtable[54];
			vtable[54] = (DWORD)JuceApplyStateBlock;

			TRACE2("g_orgApplyStateBlock = %08x", (DWORD)g_orgApplyStateBlock);
			TRACE2("JuceApplyStateBlock = %08x", (DWORD)JuceApplyStateBlock);

			// hook BeginScene method
			g_orgBeginScene = (PFNBEGINSCENEPROC)vtable[34];
			vtable[34] = (DWORD)JuceBeginScene;

			TRACE2("g_orgBeginScene = %08x", (DWORD)g_orgBeginScene);
			TRACE2("JuceBeginScene = %08x", (DWORD)JuceBeginScene);

			// hook EndScene method
			g_orgEndScene = (PFNENDSCENEPROC)vtable[35];
			vtable[35] = (DWORD)JuceEndScene;

			TRACE2("g_orgEndScene = %08x", (DWORD)g_orgEndScene);
			TRACE2("JuceEndScene = %08x", (DWORD)JuceEndScene);
			*/
		}
	}

	g_offset = offset;
	if (g_teamOffsetsLoaded)
	{
		// see if this is a kit BIN about to be read
		if (IsKitOffset(offset))
		{
			g_kitOffset = offset;
			TRACE2("JuceSetFilePointer: new g_kitOffset = %08x", (DWORD)offset);
		}
	}
	TRACE2("JuceSetFilePointer: offset = %08x", (DWORD)offset);

	// TEMP.test
	if (offset == g_etcEeTex.dwOffset)
	{
		Log("JuceSetFilePointer: etc_ee_tex.bin is being loaded.");
	}

	return SetFilePointer(handle, offset, upper32, method);
}

DWORD JuceUniDecrypt(DWORD addr, DWORD size)
{
	TRACE("JuceUniDecrypt: CALLED.");
	TRACE2("JuceUniDecode: addr = %08x", addr);

	DWORD result = UniDecrypt(addr, size);

	return result;
}

// helper function
BOOL IsBallModelOffset(DWORD offset)
{
	for (int i=0; i<BALL_MDLS_COUNT; i++)
		if (g_ballMdls[i].dwOffset == offset) return true;

	return false;
}

// helper function
BOOL IsBallTextureOffset(DWORD offset)
{
	for (int i=0; i<BALL_TEXS_COUNT; i++)
		if (g_ballTexs[i].dwOffset == offset) return true;

	return false;
}

/**
 * This function calls the unpack function for non-kits.
 * Parameters:
 *   addr1   - address of the encoded buffer (without header)
 *   addr2   - address of the decoded buffer
 *   size1   - size of the encoded buffer (not counting header)
 *   size2   - size of the decoded buffer
 */
DWORD JuceUnpack(DWORD addr1, DWORD addr2, DWORD size1, DWORD size2)
{
	TRACE2("JuceUnpack: CALLED (afs file offset = %08x).", g_offset);
	BOOL isBallTex = FALSE;

	if (IsBallModelOffset(g_offset) && g_ballMdl != NULL)
	{
		Log("JuceUnpack: unpacking ball model");

		// swap source buffer and sizes
		addr1 = (DWORD)g_ballMdl + sizeof(ENCBUFFERHEADER);
		size1 = ((ENCBUFFERHEADER*)g_ballMdl)->dwEncSize;
		size2 = ((ENCBUFFERHEADER*)g_ballMdl)->dwDecSize;
	}
	else 
	{
		isBallTex = IsBallTextureOffset(g_offset);
		if (isBallTex && g_ballTex != NULL)
		{
			Log("JuceUnpack: unpacking ball texture");

			if (g_balls->ball != NULL && !g_balls->ball->isTexBmp)
			{
				// swap source buffer and sizes
				// (ONLY, if ball texture is a BIN file - not BMP.)
				addr1 = (DWORD)g_ballTex + sizeof(ENCBUFFERHEADER);
				size1 = ((ENCBUFFERHEADER*)g_ballTex)->dwEncSize;
				size2 = ((ENCBUFFERHEADER*)g_ballTex)->dwDecSize;
			}
		}
	}

	// call target function
	DWORD result = Unpack(addr1, addr2, size1, size2);

	// handle the case when ball texture is provided as BMP.
	if (isBallTex && g_ballTex != NULL && g_balls->ball->isTexBmp)
	{
		//DumpData((BYTE*)addr2, size2);
		//DumpBall((BYTE*)addr2);
		ApplyBallTexture((BYTE*)addr2, g_ballTex);
		//DumpData((BYTE*)addr2, size2);
		//DumpBall((BYTE*)addr2);
	}

	return result;
}

/**
 * This function is seemingly responsible for allocating memory
 * for decoded buffer (when unpacking BIN files)
 * Parameters:
 *   infoBlock  - address of some information block
 *                What is of interest of in that block: infoBlock[60]
 *                contains an address of encoded (src) BIN.
 *   param2     - unknown param. Possibly count of buffers to allocate
 *   size       - size in bytes of the block needed.
 * Returns:
 *   address of newly allocated buffer. Also, this address is stored
 *   at infoBlock[64] location, which is IMPORTANT.
 */
DWORD JuceAllocMem(DWORD infoBlock, DWORD param2, DWORD size)
{
	TRACE2("JuceAllocMem CALLED. g_offset = %08x", g_offset);
	BYTE* info = (BYTE*)infoBlock;

	if (IsBallModelOffset(g_offset) && g_ballMdl != NULL)
	{
		TRACE2("JuceAllocMem: ball-mdl: Size needed: %08x", size);
		// overwrite size
		ENCBUFFERHEADER* header = (ENCBUFFERHEADER*)g_ballMdl;
		size = header->dwDecSize;
		TRACE2("JuceAllocMem: requesting size: %08x", size);
	}
	else if (IsBallTextureOffset(g_offset) && g_ballTex != NULL)
	{
		if (g_balls->ball != NULL && !g_balls->ball->isTexBmp)
		{
			TRACE2("JuceAllocMem: ball-tex: Size needed: %08x", size);
			// overwrite size
			// (ONLY for ball textures, provided as BIN - not BMP)
			ENCBUFFERHEADER* header = (ENCBUFFERHEADER*)g_ballTex;
			size = header->dwDecSize;
			TRACE2("JuceAllocMem: requesting size: %08x", size);
		}
	}

	// call the target function
	DWORD result = AllocMem(infoBlock, param2, size);

	return result;
}

DWORD JuceResetFlags(DWORD index)
{
	Log("JuceResetFlags: CALLED.");

	// call original function
	DWORD result = ResetFlags(index);

	// clear out the slots
	ZeroMemory((BYTE*)data[KIT_SLOTS], 0x38 * 4);
	ZeroMemory((BYTE*)data[ANOTHER_KIT_SLOTS], sizeof(KitSlot) * 4);

	return result;
}

// determines kit id, based on offset.
DWORD FindKitId(DWORD offset, DWORD* size)
{
	// return quickly, if we know we're dealing with bibs
	if (g_isBibs) return 0xffff;

	for (int num=0; num < g_numTeams; num++)
	{
		if (g_teamInfo[num].ga.dwOffset == g_kitOffset && g_teamInfo[num].ga.dwSize > 0) {
            *size = g_teamInfo[num].ga.dwSize;
            return num * 5;
        }
		if (g_teamInfo[num].gb.dwOffset == g_kitOffset && g_teamInfo[num].gb.dwSize > 0) {
            *size = g_teamInfo[num].gb.dwSize;
            return num * 5 + 1;
        }
		if (g_teamInfo[num].pa.dwOffset == g_kitOffset && g_teamInfo[num].pa.dwSize > 0) {
            *size = g_teamInfo[num].pa.dwSize;
            return num * 5 + 2;
        }
		if (g_teamInfo[num].pb.dwOffset == g_kitOffset && g_teamInfo[num].pb.dwSize > 0) {
            *size = g_teamInfo[num].pb.dwSize;
            return num * 5 + 3;
        }
		if (g_teamInfo[num].vg.dwOffset == g_kitOffset && g_teamInfo[num].vg.dwSize > 0) {
            *size = g_teamInfo[num].vg.dwSize;
            return num * 5 + 4;
        }
	}
	return 0xffff; // unknown kit id
}

/**
 * This function calls the unpack function.
 * Parameters:
 *   addr1   - address of the encoded buffer header
 *   addr2   - address of the encoded buffer
 *   size    - size of the encoded buffer
 * Return value:
 *   address of the decoded buffer
 */
DWORD JuceUniDecode(DWORD addr1, DWORD addr2, DWORD size)
{
    if (!_aspectRatioSet) {
        // adjust aspect ratio
        if (data[PROJ_W] && data[CULL_W] && g_config.aspectRatio >0.0f) {
            LogWithDouble("Setting aspect-ratio: %0.4f", 
                    (double)g_config.aspectRatio);
            float factor = g_config.aspectRatio / (512.0/384.0);

            DWORD newProtection = PAGE_EXECUTE_READWRITE;
            float* ar_p = (float*)data[PROJ_W];
            if (VirtualProtect(ar_p,4,newProtection,&g_savedProtection)) {
                *ar_p = 512.0*factor;
                LogWithDouble("PROJECTION_W: %0.2f", (double)*ar_p);
            }
            else {
                Log("FAILED to change aspect-ratio (PROJECTION)");
            }
            DWORD* ar_c = (DWORD*)data[CULL_W];
            if (VirtualProtect(ar_c,4,newProtection,&g_savedProtection)) {
                *ar_c = (int)(512*factor)+1;
                LogWithNumber("CULLING_W: %d", *ar_c);
            }
            else {
                Log("FAILED to change aspect-ratio (CULLING)");
            }
        }
        _aspectRatioSet = true;
    }

	TRACE("JuceUniDecode: CALLED.");
	TRACE2("JuceUniDecode: addr1 = %08x", addr1);
	TRACE2("JuceUniDecode: addr2 = %08x", addr2);

	// call the hooked function
	DWORD result = UniDecode(addr1, addr2, size);
	BYTE* orgKit = (BYTE*)result;

	/*
	// save encoded buffer as raw data
	ENCBUFFERHEADER* header = (ENCBUFFERHEADER*)addr1;
	DumpData((BYTE*)addr1, header->dwEncSize + sizeof(ENCBUFFERHEADER));

	// save decoded buffer as raw data
	DumpData((BYTE*)result, 0x2a480);
	*/

	TRACE2("JuceUniDecode: g_offset = %08x", g_offset);
	TRACE2("JuceUniDecode: g_kitOffset = %08x", g_kitOffset);

	// find corresponding file
	char fileClue[BUFLEN];
	ZeroMemory(fileClue, BUFLEN);
	WORD* teams = (WORD*)data[TEAM_IDS];

	BOOL found = FALSE;
	g_isBibs = FALSE;

	// check if these are bibs
	if (g_bibs.dwOffset == g_kitOffset)
	{
		found = TRUE;
		g_isBibs = TRUE;
	}

    DWORD kitSize = 0;

	// determine which team the loading kit belongs to, and 
	// which particular kit (ga,gb,pa,pb,vg) it is.
	WORD kitId = FindKitId(g_kitOffset, &kitSize);

	ENCBUFFERHEADER* header = (ENCBUFFERHEADER*)addr1;
    TRACE2("JuceUniDecode: src buffer size = %08x", header->dwEncSize + sizeof(ENCBUFFERHEADER));
    TRACE2("JuceUniDecode: matched kitSize = %08x", kitSize);

	// determine which team the loading kit belongs to
	// (N*5 - ga, N*5+1 - gb, N*5+2 - pa, N*5+3 - pb, where N is team id.)
	WORD teamId = kitId / 5;

	// sanity check
	if (teamId >= 0 && teamId <= g_numTeams)
	{
		// prepare file clue
		if (g_kitExtras[kitId] == NULL)
		{
			sprintf(fileClue, "%03d/%s", teamId, STANDARD_KIT_NAMES[kitId % 5]);
		}
		else
		{
			// switch to extra kit
			KitEntry* ke = g_kitExtras[kitId];
			if (ke->kit != NULL)
				sprintf(fileClue, "%03d/%s", teamId, ke->kit->fileName);
			else
				// ERROR condition. Shouldn't ever happen
				sprintf(fileClue, "%03d/BAD.bmp", teamId); 
		}
	}

	TRACE2("teamId = %04x", teamId);
	TRACE2("teams[0] = %04x", teams[0]);
	TRACE2("teams[1] = %04x", teams[1]);

	// Build kit filename
	if (fileClue[0] != '\0')
	{
		LogWithString("JuceUniDecode: fileClue: %s", fileClue);

		char filename[BUFLEN];
		ZeroMemory(filename, BUFLEN);
		lstrcpy(filename, g_config.kdbDir);
		lstrcat(filename, "KDB/uni/");
		lstrcat(filename, fileClue);

		g_texSize = LoadTexture(&g_tex, filename);

		if (kitId % 5 == 4) {
			ApplyGloveTextures(orgKit); // gloves
			// we're done now, because gloves don't have texture boost.
			return result;
		}
		ApplyTextures(orgKit); // uniform

		g_customTex = (g_texSize > 0);
	}

	// copy largest texture from the buffer to a safe place;
	// so that we can use it for texture boost.
	BYTE* pal = (BYTE*)g_palette;
	for (int bank=0; bank<8; bank++)
	{
		memcpy(pal + bank*32*4,        orgKit + 0x80 + bank*32*4,        8*4);
		memcpy(pal + bank*32*4 + 8*4,  orgKit + 0x80 + bank*32*4 + 16*4, 8*4);
		memcpy(pal + bank*32*4 + 16*4, orgKit + 0x80 + bank*32*4 + 8*4,  8*4);
		memcpy(pal + bank*32*4 + 24*4, orgKit + 0x80 + bank*32*4 + 24*4, 8*4);
	}

	// enhance opaque values
	for (int c=0; c<256; c++)
		if (pal[c*4 + 3] == 0x80) pal[c*4 + 3] = 0xff;

	memcpy(g_lastDecodedKit, orgKit + 0x480, 0x28000);
	g_bigTextureAvailable = TRUE;

	// save decoded uniforms as BMPs
	//DumpUniforms(orgKit);

	// dump palette
	//DumpData((BYTE*)g_palette, 0x400);
	
	return result;
}

// Returns the game version id
int GetGameVersion(void)
{
	HMODULE hMod = GetModuleHandle(NULL);
	for (int i=0; i<5; i++)
	{
		char* guid = (char*)((DWORD)hMod + GAME_GUID_OFFSETS[i]);
		if (strncmp(guid, GAME_GUID[i], lstrlen(GAME_GUID[i]))==0)
			return i;
	}
	return -1;
}

// Initialize pointers and data structures.
void Initialize(int v)
{
	// select correct addresses
	memcpy(code, codeArray[v], sizeof(code));
	memcpy(data, dataArray[v], sizeof(data));

	// assign pointers
	UniDecrypt = (UNIDECRYPT)code[C_UNIDECRYPT];
	UniDecode = (UNIDECODE)code[C_UNIDECODE];
	Unpack = (UNPACK)code[C_UNPACK];
	SetKitAttributes = (SETKITATTRIBUTES)code[C_SETKITATTRIBUTES];
	FinishKitPick = (FINISHKITPICK)code[C_FINISHKITPICK];
	AllocMem = (ALLOCMEM)code[C_ALLOCMEM];
	ResetFlags = (RESETFLAGS)code[C_RESETFLAGS];
}

/* Writes an indexed 8-bit BMP file (with palette) */
HRESULT SaveAs8bitBMP(char* filename, BYTE* buffer)
{
	DECBUFFERHEADER* header = (DECBUFFERHEADER*)buffer;
	BYTE* buf = (BYTE*)header + header->bitsOffset;
	BYTE* pal = (BYTE*)header + header->paletteOffset;
	LONG width = header->texWidth;
	LONG height = header->texHeight;

	return SaveAs8bitBMP(filename, buf, pal, width, height);
}

/* Writes an indexed 8-bit BMP file (with palette) */
HRESULT SaveAs8bitBMP(char* filename, BYTE* buf, BYTE* pal, LONG width, LONG height)
{
	BITMAPFILEHEADER fhdr;
	BITMAPINFOHEADER infoheader;
	SIZE_T size = width * height; // size of data in bytes

	// fill in the headers
	fhdr.bfType = 0x4D42; // "BM"
	fhdr.bfSize = sizeof(fhdr) + sizeof(infoheader) + 0x400 + size;
	fhdr.bfReserved1 = 0;
	fhdr.bfReserved2 = 0;
	fhdr.bfOffBits = sizeof(fhdr) + sizeof(infoheader) + 0x400;

	infoheader.biSize = sizeof(infoheader);
	infoheader.biWidth = width;
	infoheader.biHeight = height;
	infoheader.biPlanes = 1;
	infoheader.biBitCount = 8;
	infoheader.biCompression = BI_RGB;
	infoheader.biSizeImage = 0;
	infoheader.biXPelsPerMeter = 0;
	infoheader.biYPelsPerMeter = 0;
	infoheader.biClrUsed = 256;
	infoheader.biClrImportant = 0;

	// prepare filename
	char name[BUFLEN];
	if (filename == NULL)
	{
		SYSTEMTIME time;
		GetLocalTime(&time);
		ZeroMemory(name, BUFLEN);
		sprintf(name, "%s%s-%d%02d%02d-%02d%02d%02d%03d.bmp", mydir, "kserv", 
				time.wYear, time.wMonth, time.wDay,
				time.wHour, time.wMinute, time.wSecond, time.wMilliseconds); 
		filename = name;
	}

	// save to file
	DWORD wbytes;
	HANDLE hFile = CreateFile(filename,            // file to create 
					 GENERIC_WRITE,                // open for writing 
					 0,                            // do not share 
					 NULL,                         // default security 
					 OPEN_ALWAYS,                  // overwrite existing 
					 FILE_ATTRIBUTE_NORMAL,        // normal file 
					 NULL);                        // no attr. template 

	if (hFile != INVALID_HANDLE_VALUE) 
	{
		WriteFile(hFile, &fhdr, sizeof(fhdr), (LPDWORD)&wbytes, NULL);
		WriteFile(hFile, &infoheader, sizeof(infoheader), (LPDWORD)&wbytes, NULL);

		// write palette
		for (int bank=0; bank<8; bank++)
		{
			for (int i=0; i<8; i++)
			{
				WriteFile(hFile, pal + bank*32*4 + i*4 + 2, 1, (LPDWORD)&wbytes, NULL);
				WriteFile(hFile, pal + bank*32*4 + i*4 + 1, 1, (LPDWORD)&wbytes, NULL);
				WriteFile(hFile, pal + bank*32*4 + i*4, 1, (LPDWORD)&wbytes, NULL);
				WriteFile(hFile, pal + bank*32*4 + i*4 + 3, 1, (LPDWORD)&wbytes, NULL);
			}
			for (i=16; i<24; i++)
			{
				WriteFile(hFile, pal + bank*32*4 + i*4 + 2, 1, (LPDWORD)&wbytes, NULL);
				WriteFile(hFile, pal + bank*32*4 + i*4 + 1, 1, (LPDWORD)&wbytes, NULL);
				WriteFile(hFile, pal + bank*32*4 + i*4, 1, (LPDWORD)&wbytes, NULL);
				WriteFile(hFile, pal + bank*32*4 + i*4 + 3, 1, (LPDWORD)&wbytes, NULL);
			}
			for (i=8; i<16; i++)
			{
				WriteFile(hFile, pal + bank*32*4 + i*4 + 2, 1, (LPDWORD)&wbytes, NULL);
				WriteFile(hFile, pal + bank*32*4 + i*4 + 1, 1, (LPDWORD)&wbytes, NULL);
				WriteFile(hFile, pal + bank*32*4 + i*4, 1, (LPDWORD)&wbytes, NULL);
				WriteFile(hFile, pal + bank*32*4 + i*4 + 3, 1, (LPDWORD)&wbytes, NULL);
			}
			for (i=24; i<32; i++)
			{
				WriteFile(hFile, pal + bank*32*4 + i*4 + 2, 1, (LPDWORD)&wbytes, NULL);
				WriteFile(hFile, pal + bank*32*4 + i*4 + 1, 1, (LPDWORD)&wbytes, NULL);
				WriteFile(hFile, pal + bank*32*4 + i*4, 1, (LPDWORD)&wbytes, NULL);
				WriteFile(hFile, pal + bank*32*4 + i*4 + 3, 1, (LPDWORD)&wbytes, NULL);
			}
		}
		// write pixel data
		for (int k=height-1; k>=0; k--)
		{
			WriteFile(hFile, buf + k*width, width, (LPDWORD)&wbytes, NULL);
		}
		CloseHandle(hFile);
	}
	else 
	{
		Log("SaveAs8bitBMP: failed to save to file.");
		return E_FAIL;
	}
	return S_OK;
}

/**
 * Shuffle the ball texture data.
 */
void ShuffleBallTexture(BYTE* dest, BYTE* src)
{
	BITMAPFILEHEADER* srcHeader = (BITMAPFILEHEADER*)src;
	DECBUFFERHEADER* destHeader = (DECBUFFERHEADER*)dest;

	BYTE* srcData = (BYTE*)srcHeader + srcHeader->bfOffBits;
	BYTE* destData = (BYTE*)destHeader + destHeader->bitsOffset;

	int unitX = destHeader->texWidth / destHeader->blockWidth;
	int unitY = destHeader->texHeight / destHeader->blockHeight;
	int pitch = destHeader->texWidth;

	TRACE2("ShuffleBallTexture: unitX = %d", unitX);
	TRACE2("ShuffleBallTexture: unitY = %d", unitY);
	TRACE2("ShuffleBallTexture: pitch = %d", pitch);
	TRACE2("ShuffleBallTexture: texHeight = %d", destHeader->texHeight);

	int w = destHeader->texWidth;
	int h = destHeader->texHeight;
    int y;

	// copy all pixels.
	// NOTE: we need a loop here to reverse the order of lines
	for (y=0; y<h; y++)
	{
		memcpy(destData + (h-y-1)*w, srcData + y*w, w);
	}

	/*
	"""
	block swap
	"""
	def blockSwap(self, image, pixels):
		w = image.GetWidth()
		h = image.GetHeight()

		lines = [n for n in range(h) if n >= 2 and n < h-2 and ((n-2)/4)%2 == 0]

		for y in lines:
			for k in range(w/8):
				for x in range(4):
					a, b = y*w + k*8 + x, y*w + k*8 + x + 4
					pixels[a], pixels[b] = pixels[b], pixels[a]
	*/
	// block swap
	for (y=0; y<h; y++)
	{
		if (y < 2 || y >= h-2 || ((y-2)/4)%2 == 1)
			continue;

		for (int k=0; k<w/8; k++)
		{
			BYTE copy[4];
			memcpy(copy, destData + y*w + k*8, 4);
			memcpy(destData + y*w + k*8, destData + y*w + k*8 + 4, 4);
			memcpy(destData + y*w + k*8 + 4, copy, 4);
		}
	}

	/*
	"""
	block interlace
	"""
	def blockInterlace(self, image, pixels):
		w = image.GetWidth()
		h = image.GetHeight()

		for y in range(h):
			for k in range(w/16):
				left = [0 for num in range(8)]
				right = [0 for num in range(8)]
				for x in range(8):
					a, b = y*w + k*16 + x, y*w + k*16 + x + 8
					left[x], right[x] = pixels[a], pixels[b]
				for x in range(8):
					a, b = y*w + k*16 + x*2, y*w + k*16 + x*2 + 1
					pixels[a], pixels[b] = left[x], right[x]
	*/
	// block interlace
	for (y=0; y<h; y++)
	{
		for (int k=0; k<w/16; k++)
		{
			BYTE left[8], right[8];
			memcpy(left, destData + y*w + k*16, 8);
			memcpy(right, destData + y*w + k*16 + 8, 8);

			for (int x=0; x<8; x++)
			{
				destData[y*w + k*16 + x*2] = left[x];
				destData[y*w + k*16 + x*2 + 1] = right[x];
			}
		}
	}

	// make a extra buffer
	BYTE* copy = (BYTE*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, w*h);
	if (copy == NULL)
	{
		Log("ShuffleBallTexture: Out of memory ERROR.");
		return;
	}

	/*
	"""
	swap lines
	"""
	def swapLines(self, image, pixels):
		w = image.GetWidth()
		h = image.GetHeight()

		for y in range(h/4):
			for x in range(w):
				a, b = (y*4 + 1)*w + x, (y*4 + 2)*w + x
				pixels[a], pixels[b] = pixels[b], pixels[a]
	*/
	// swap lines
	for (y=0; y<h/4; y++)
	{
		memcpy(copy + y*4*w, destData + y*4*w, w);
		memcpy(copy + (y*4 + 1)*w, destData + (y*4 + 2)*w, w);
		memcpy(copy + (y*4 + 2)*w, destData + (y*4 + 1)*w, w);
		memcpy(copy + (y*4 + 3)*w, destData + (y*4 + 3)*w, w);
	}

	/*
	"""
	reverse fourliner transform
	"""
	def reverseFourliner(self, image, pixels):
		w = image.GetWidth()
		h = image.GetHeight()

		# make a copy, because otherwise we screw up
		srcPixels = [pixel for pixel in pixels]

		# clear
		for i in range(len(pixels)):
			pixels[i] = ('\0','\0','\0')

		count = 0
		for y in range(h/2):
			for x in range(w/2):
				pixels[y*2*w + x*2] = srcPixels[y*2*w + x] 
				pixels[y*2*w + x*2 + 1] = srcPixels[(y*2+1)*w + x]
				pixels[(y*2+1)*w + x*2] = srcPixels[y*2*w + x + w/2]
				pixels[(y*2+1)*w + x*2 + 1] = srcPixels[(y*2+1)*w + x + w/2]
	*/
	// reverse fourliner
	// NOTE: as done in previous operation, "copy" contains the result
	// of all previous steps.
	for (y=0; y<h/2; y++)
	{
		for (int x=0; x<w/2; x++)
		{
			destData[y*2*w + x*2] = copy[y*2*w + x];
			destData[y*2*w + x*2 + 1] = copy[(y*2+1)*w + x];
			destData[(y*2+1)*w + x*2] = copy[y*2*w + x + w/2];
			destData[(y*2+1)*w + x*2 + 1] = copy[(y*2+1)*w + x + w/2];
		}
	}

	// free buffer
	HeapFree(GetProcessHeap(), 0, copy);
}

/**
 * De-shuffle the ball texture data.
 */
void DeshuffleBallTexture(BYTE* dest, BYTE* src)
{
	DECBUFFERHEADER* srcHeader = (DECBUFFERHEADER*)src;
	DECBUFFERHEADER* destHeader = (DECBUFFERHEADER*)dest;

	BYTE* srcData = (BYTE*)srcHeader + srcHeader->bitsOffset;
	BYTE* destData = (BYTE*)destHeader + destHeader->bitsOffset;

	int unitX = srcHeader->texWidth / srcHeader->blockWidth;
	int unitY = srcHeader->texHeight / srcHeader->blockHeight;
	int pitch = srcHeader->texWidth;

	TRACE2("DeshuffleBallTexture: unitX = %d", unitX);
	TRACE2("DeshuffleBallTexture: unitY = %d", unitY);
	TRACE2("DeshuffleBallTexture: pitch = %d", pitch);
	TRACE2("DeshuffleBallTexture: texHeight = %d", srcHeader->texHeight);

	int w = srcHeader->texWidth;
	int h = srcHeader->texHeight;

	// copy all pixels
	memcpy(destData, srcData, w*h);
}

/* Writes a BMP file */
HRESULT SaveAsBMP(char* filename, BYTE* rgbBuf, SIZE_T size, LONG width, LONG height, int bpp)
{
	BITMAPFILEHEADER fhdr;
	BITMAPINFOHEADER infoheader;

	// fill in the headers
	fhdr.bfType = 0x4D42; // "BM"
	fhdr.bfSize = sizeof(fhdr) + sizeof(infoheader) + size;
	fhdr.bfReserved1 = 0;
	fhdr.bfReserved2 = 0;
	fhdr.bfOffBits = sizeof(fhdr) + sizeof(infoheader);

	infoheader.biSize = sizeof(infoheader);
	infoheader.biWidth = width;
	infoheader.biHeight = height;
	infoheader.biPlanes = 1;
	infoheader.biBitCount = bpp*8;
	infoheader.biCompression = BI_RGB;
	infoheader.biSizeImage = 0;
	infoheader.biXPelsPerMeter = 0;
	infoheader.biYPelsPerMeter = 0;
	infoheader.biClrUsed = 0;
	infoheader.biClrImportant = 0;

	// prepare filename
	char name[BUFLEN];
	if (filename == NULL)
	{
		SYSTEMTIME time;
		GetLocalTime(&time);
		ZeroMemory(name, BUFLEN);
		sprintf(name, "%s%s-%d%02d%02d-%02d%02d%02d%03d.bmp", mydir, "kserv", 
				time.wYear, time.wMonth, time.wDay,
				time.wHour, time.wMinute, time.wSecond, time.wMilliseconds); 
		filename = name;
	}

	// save to file
	DWORD wbytes;
	HANDLE hFile = CreateFile(filename,            // file to create 
					 GENERIC_WRITE,                // open for writing 
					 0,                            // do not share 
					 NULL,                         // default security 
					 OPEN_ALWAYS,                  // overwrite existing 
					 FILE_ATTRIBUTE_NORMAL,        // normal file 
					 NULL);                        // no attr. template 

	if (hFile != INVALID_HANDLE_VALUE) 
	{
		WriteFile(hFile, &fhdr, sizeof(fhdr), (LPDWORD)&wbytes, NULL);
		WriteFile(hFile, &infoheader, sizeof(infoheader), (LPDWORD)&wbytes, NULL);
		WriteFile(hFile, rgbBuf, size, (LPDWORD)&wbytes, NULL);
		CloseHandle(hFile);
	}
	else 
	{
		Log("SaveAsBMP: failed to save to file.");
		return E_FAIL;
	}
	return S_OK;
}

// helper function
Kit* utilGetKit(WORD kitId)
{
	int id = kitId / 5; int idRem = kitId % 5;

	// sanity check
	if (id < 0 || id > 255)
		return NULL;

	// select corresponding kit
	Kit* kit = NULL;
	if (g_kitExtras[kitId] != NULL) 
		kit = g_kitExtras[kitId]->kit;
	if (kit == NULL)
	{
		switch (idRem)
		{
			case 0: kit = kDB->goalkeepers[id].a; break;
			case 1: kit = kDB->goalkeepers[id].b; break;
			case 2: kit = kDB->players[id].a; break;
			case 3: kit = kDB->players[id].b; break;
		}
	}

	return kit;
}

// helper function
TeamAttr* utilGetTeamAttr(WORD id)
{
	// sanity check
	if (id < 0 || id > 255)
		return NULL;

	DWORD teamAttrBase = *((DWORD*)data[TEAM_COLLARS_PTR]);
	return (TeamAttr*)(teamAttrBase + id*sizeof(TeamAttr));
}

/**
 * This function gets called when kit selection is complete.
 */
DWORD JuceFinishKitPick(DWORD param)
{
	TRACE("JuceFinishKitPick: CALLED.");

	if (bIndicate && g_device != NULL)
	{
		// uninstall keyboard hook
		UninstallKeyboardHook();

		// reset all objects
		InvalidateDeviceObjects(g_device);
		DeleteDeviceObjects(g_device);

		DWORD* vtable = (DWORD*)(*((DWORD*)g_device));

		// unhook Present method
		if (g_orgPresent != NULL) vtable[15] = (DWORD)g_orgPresent;

		// unhook Reset method
		if (g_orgReset != NULL) vtable[14] = (DWORD)g_orgReset;

		bIndicate = false;
	}
	return FinishKitPick(param);
}

/**
 * This function sets uniform colors/collars/3d-models.
 */
DWORD JuceCommonSetKitAttributes(DWORD n, BOOL useN)
{
	// make sure we know which teams are selected.
	VerifyTeams();

	// save the team attributes here
	TeamAttr saveTeamAttr[2];
	for (int k=0; k<2; k++) {
		BOOL customTeam = IS_CUSTOM_TEAM(g_currTeams[k]);
		if (customTeam)
			continue;

		TeamAttr* teamAttr = utilGetTeamAttr(g_currTeams[k]);
		memcpy(&(saveTeamAttr[k]), teamAttr, sizeof(TeamAttr));
	}

	// set team attributes
	for (k=0; k<2; k++) {
		BOOL customTeam = IS_CUSTOM_TEAM(g_currTeams[k]);
		if (customTeam)
			continue;

		TeamAttr* teamAttr = utilGetTeamAttr(g_currTeams[k]);

		for (int i=0; i<4; i++) {
			WORD kitId = g_currTeams[k] * 5 + i;
			Kit* kit = utilGetKit(kitId);
			if (kit == NULL)
				continue;

			switch (i) {
				case 0:
					// 1st GK
					if (kit->attDefined & COLLAR) teamAttr->home.collarGK = kit->collar;
					if (kit->attDefined & CUFF)   teamAttr->home.cuffGK = kit->cuff;
					break;
				case 1:
					// 2nd GK
					if (kit->attDefined & COLLAR) teamAttr->away.collarGK = kit->collar;
					if (kit->attDefined & CUFF)   teamAttr->away.cuffGK = kit->cuff;
					break;
				case 2:
					// 1st PL
					if (kit->attDefined & MODEL) teamAttr->home.model = kit->model;
					if (kit->attDefined & COLLAR) teamAttr->home.collarPL = kit->collar;
					if (kit->attDefined & CUFF)   teamAttr->home.cuffPL = kit->cuff;
					break;
				case 3:
					// 2nd PL
					if (kit->attDefined & MODEL) teamAttr->away.model = kit->model;
					if (kit->attDefined & COLLAR) teamAttr->away.collarPL = kit->collar;
					if (kit->attDefined & CUFF)   teamAttr->away.cuffPL = kit->cuff;
					break;
			}
		}
	}
	
	// CALL ORIGINAL FUNCTION
	DWORD result = SetKitAttributes(n);

	// reset team attributes
	for (k=0; k<2; k++) {
		BOOL customTeam = IS_CUSTOM_TEAM(g_currTeams[k]);
		if (customTeam)
			continue;

		TeamAttr* teamAttr = utilGetTeamAttr(g_currTeams[k]);
		memcpy(teamAttr, &(saveTeamAttr[k]), sizeof(TeamAttr));
	}

	BOOL customTeam = IS_CUSTOM_TEAM(g_currTeams[n]);
	TRACE2("customTeam = %d", customTeam);

	// determine which slots to apply the colors to.
	int index = (useN) ? n : 0;

	if (!customTeam)
	{
		// PLAYER
		BYTE* strips = (BYTE*)data[TEAM_STRIPS];
		WORD kitId = g_currTeams[index] * 5 + ((strips[index] & 0x01) != 0) + 2;
		Kit* kit = utilGetKit(kitId);

		KitSlot* kitSlots = (KitSlot*)(data[ANOTHER_KIT_SLOTS] + index*sizeof(KitSlot)*2);

		if (kit != NULL && !kitSlots[1].isEdited)
		{
			// set colors
			if (kit->attDefined & SHIRT_NAME)
				memcpy(&kitSlots[1].shirtNameColor, &kit->shirtName, sizeof(RGBAColor));
			if (kit->attDefined & SHORTS_NUMBER)
				memcpy(&kitSlots[1].shortsNumberColor, &kit->shortsNumber, sizeof(RGBAColor));
			if (kit->attDefined & SHIRT_NUMBER)
				memcpy(&kitSlots[1].shirtNumberColor, &kit->shirtNumber, sizeof(RGBAColor));

			// set shorts-number-location/number-type/name-type/name-shape
			if (kit->attDefined & SHORTS_NUMBER_LOCATION)
				kitSlots[1].shortsNumberLocation = kit->shortsNumberLocation;
			if (kit->attDefined & NUMBER_TYPE)
				kitSlots[1].numberType = kit->numberType;
			if (kit->attDefined & NAME_TYPE)
				kitSlots[1].nameType = kit->nameType;
			if (kit->attDefined & NAME_SHAPE)
				kitSlots[1].nameShape = kit->nameShape;
		}

		// GOALKEEPER
		kitId = g_currTeams[index] * 5 + ((strips[index] & 0x02) != 0);
		kit = utilGetKit(kitId);

		if (kit != NULL && !kitSlots[0].isEdited)
		{
			// set goalkeeper colors
			if (kit->attDefined & SHIRT_NAME)
				memcpy(&kitSlots[0].shirtNameColor, &kit->shirtName, sizeof(RGBAColor));
			if (kit->attDefined & SHORTS_NUMBER)
				memcpy(&kitSlots[0].shortsNumberColor, &kit->shortsNumber, sizeof(RGBAColor));
			if (kit->attDefined & SHIRT_NUMBER)
				memcpy(&kitSlots[0].shirtNumberColor, &kit->shirtNumber, sizeof(RGBAColor));

			// set shorts-number-location/number-type/name-type/name-shape
			if (kit->attDefined & SHORTS_NUMBER_LOCATION)
				kitSlots[0].shortsNumberLocation = kit->shortsNumberLocation;
			if (kit->attDefined & NUMBER_TYPE)
				kitSlots[0].numberType = kit->numberType;
			if (kit->attDefined & NAME_TYPE)
				kitSlots[0].nameType = kit->nameType;
			if (kit->attDefined & NAME_SHAPE)
				kitSlots[0].nameShape = kit->nameShape;
		}

	} // if (!customTeam)

	return result;
}

/**
 * This function sets uniform colors/collars/3d-models.
 * It is called right before the match begins.
 */
DWORD JuceFinalSetKitAttributes(DWORD n)
{
	TRACE2("JuceFinalSetKitAttributes CALLED: n = %d", n);

	DWORD result = JuceCommonSetKitAttributes(n, TRUE);

	// clear 2 unused slots, so that goalkeeper kits are predictably loaded
	if (n == 1)
	{
		int takenSlots[2] = {-1, -1};
		for (int i=0; i<2; i++)
		{
			KitSlot* kitSlot = (KitSlot*)(data[ANOTHER_KIT_SLOTS] + i*2*sizeof(KitSlot) + sizeof(KitSlot));
			takenSlots[i] = GetKitSlot(kitSlot->kitId);
		}
		for (int k=0; k<4; k++)
		{
			if (k == takenSlots[0] || k == takenSlots[1]) continue;
			ZeroMemory((BYTE*)data[KIT_SLOTS] + k * 0x38, 0x38);
			TRACE2("JuceFinalSetKitAttributes: slot #%d cleared.", k);
		}
		TRACE("JuceFinalSetKitAttributes: unused slots cleared.");
	}

	return result;
}

/**
 * This function sets uniform collars/3d-models.
 * It is only called when player kits are chosen by the user on
 * kit selection screen.
 */
DWORD JuceKitPickSetKitAttributes(DWORD n)
{
	TRACE2("JuceKitPickSetKitAttributes CALLED: n = %d", n);

	if (!bIndicate)
	{
		TRACE("JuceKitPickSetKitAttributes: Hooking device methods");
		DWORD* vtable = (DWORD*)(*((DWORD*)g_device));

		// hook Present method
		g_orgPresent = (PFNPRESENTPROC)vtable[15];
		vtable[15] = (DWORD)JucePresent;

		TRACE("JuceKitPickSetKitAttributes: Present hooked.");

		// hook Present method
		g_orgReset = (PFNRESETPROC)vtable[14];
		vtable[14] = (DWORD)JuceReset;

		TRACE("JuceKitPickSetKitAttributes: Reset hooked.");

		bIndicate = true;
	}

	DWORD result = JuceCommonSetKitAttributes(n, TRUE);

	TRACE("JuceKitPickSetKitAttributes: DONE");
	return result;
}

/**
 * This function sets uniform colors/collars/3d-models in edit mode.
 */
DWORD JuceEditModeSetKitAttributes(DWORD n)
{
	TRACE2("JuceEditModeSetKitAttributes CALLED: n = %d", n);

	VerifyTeams();
	BOOL customTeam = IS_CUSTOM_TEAM(g_currTeams[0]);
	TRACE2("customTeam = %d", customTeam);
	
	// We're in edit mode. Therefore, reset all 3rd-kits to defaults
	// Also, set collars/3d-models for all default kits
	for (int i=0; i<4; i++)
	{
		if (customTeam) 
			break; // don't do anything for custom team

		WORD kitId = g_currTeams[0]*5 + i;
		g_kitExtras[kitId] = NULL;
	}

	return JuceCommonSetKitAttributes(n, FALSE);
}

// this function makes sure we have the most up-to-date information
// about what teams are currently chosen.
void VerifyTeams()
{
	g_currTeams[0] = g_currTeams[1] = 0xffff;
	WORD* teams = (WORD*)data[TEAM_IDS];

	// try to define both teams.
	if (teams[0] >=0 && teams[0] <= g_numTeams) g_currTeams[0] = teams[0];
	if (teams[1] >=0 && teams[1] <= g_numTeams) g_currTeams[1] = teams[1];

	// check if home team is still undefined.
	if (g_currTeams[0] == 0xffff)
	{
		int which = (teams[0] == 0x0122) ? 1 : 0;

		// looks like it's an ML team.
		DWORD* mlPtrs = (DWORD*)data[MLDATA_PTRS];
		if (mlPtrs[which] != NULL)
		{
			WORD* mlData = (WORD*)(mlPtrs[which] + 0x278);
			WORD teamId = mlData[0];
			BOOL originalTeam = mlData[1];
			if (!originalTeam) g_currTeams[0] = teamId;
		}
	}

	// check if away team is still undefined.
	if (g_currTeams[1] == 0xffff)
	{
		int which = (teams[1] == 0x0122) ? 1 : 0;

		// looks like it's an ML team.
		DWORD* mlPtrs = (DWORD*)data[MLDATA_PTRS];
		if (mlPtrs[which] != NULL)
		{
			WORD* mlData = (WORD*)(mlPtrs[which] + 0x278);
			WORD teamId = mlData[0];
			BOOL originalTeam = mlData[1];
			if (!originalTeam) g_currTeams[1] = teamId;
		}
	}
}

