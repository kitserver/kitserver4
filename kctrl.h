#ifndef _HOOK_MAIN_
#define _HOOK_MAIN_

#ifdef MYDLL_RELEASE_BUILD
#define KSERV_WINDOW_TITLE "KitServer 4 Control Panel"
#else
#define KSERV_WINDOW_TITLE "KitServer 4 Control Panel (debug build)"
#endif
#define CREDITS "About: v4.3.10 (04/2005) by Juce."

#define LOG(f,x) if (f != NULL) fprintf x

#endif

