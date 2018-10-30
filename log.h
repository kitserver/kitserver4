#ifndef _JUCE_LOG_
#define _JUCE_LOG_

#ifndef MYDLL_RELEASE_BUILD
#define TRACE(x) Debug(x)
#define TRACE2(x,y) DebugWithNumber(x,y)
#define TRACE3(x,y) DebugWithDouble(x,y)
#define TRACE4(x,y) DebugWithString(x,y)
#define TRACEX(x,a,b,c) DebugWithThreeNumbers(x,a,b,c)
#else
#define TRACE(x) 
#define TRACE2(x,y) 
#define TRACE3(x,y) 
#define TRACE4(x,y) 
#define TRACEX(x,a,b,c)
#endif

void OpenLog(char* logName);
void CloseLog();

void MasterLog(char* logfile, char* procfile, char* msg);
void Log(char* msg);
void LogWithNumber(char* msg, DWORD number);
void LogWithDouble(char* msg, double number);
void LogWithString(char* msg, char* str);

void Debug(char* msg);
void DebugWithNumber(char* msg, DWORD number);
void DebugWithDouble(char* msg, double number);
void DebugWithString(char* msg, char* str);
void DebugWithThreeNumbers(char* msg, DWORD a, DWORD b, DWORD c);

#endif

