#ifndef _JUCE_REGTOOLS
#define _JUCE_REGTOOLS

// This functions looks up the game in the Registry.
// Returns the installation directory for a specified game key
BOOL GetExeNameFromReg(char* gameKey, char* installDir);

#endif
