#include <stdio.h>
#include <limits.h>
#include <wchar.h>

#include "debug.h"

#pragma warning(disable:4996)



#ifdef SE_LOGGING

#ifdef UNDER_CE
  #define LOG_PATH L"\\cplog.txt"
#else
  #define LOG_PATH L"F:\\cplog.txt"
#endif


static int s_sessionTracker = 0;


extern void LogWriteEntry(wchar_t *format, ...)
{
  FILE *fStream;

  if (s_sessionTracker > 0)
    fStream = _wfopen(LOG_PATH, L"a");  
  else  
    fStream = _wfopen(LOG_PATH, L"w");  

  if (fStream)
  {
    va_list args;
    
    va_start(args, format);
    vfwprintf(fStream, format, args);
    va_end(args);

    fclose(fStream);
    s_sessionTracker++;
  }
}

#endif
