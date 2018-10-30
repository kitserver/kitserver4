#ifndef _HOOK_MAIN_
#define _HOOK_MAIN_

#ifdef MYDLL_RELEASE_BUILD
#define KSERV_WINDOW_TITLE "KitServer 4 Control Panel"
#else
#define KSERV_WINDOW_TITLE "KitServer 4 Control Panel (debug build)"
#endif
#define CREDITS "About: v4.4.1 (10/2018) by Juce."

#define LOG(f,x) if (f != NULL) fprintf x

#endif

