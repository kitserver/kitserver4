#ifndef _SETUP_MAIN_
#define _SETUP_MAIN_

#ifdef MYDLL_RELEASE_BUILD
#define SETUP_WINDOW_TITLE "KitServer 4 Setup"
#else
#define SETUP_WINDOW_TITLE "KitServer 4 Setup (debug build)"
#endif
#define CREDITS "About: v4.3.10 (04/2005) by Juce."

#define LOG(f,x) if (f != NULL) fprintf x

#endif

