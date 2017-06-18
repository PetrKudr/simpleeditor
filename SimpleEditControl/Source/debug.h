#ifndef __DEBUG_HEADER_
#define __DEBUG_HEADER_


#pragma warning(disable: 4996)


//#define SE_LOGGING



#ifdef __cplusplus
extern "C" {
#endif

#if !defined(_WIN32_WCE)

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#else // _WIN32_WCE

#include <stdlib.h>

#ifdef _DEBUG
  #include <assert.h>
  #define _ASSERTE(expr) assert(expr)
#else
  #define _ASSERTE(expr)
#endif

#endif // _WIN32_WCE

#ifdef SE_LOGGING

  #include <stdarg.h>


  extern void LogWriteEntry(wchar_t *format, ...);

  #define LOG_ENTRY(format, ...) LogWriteEntry(format, __VA_ARGS__)

#else // SE_LOGGING

  #define LOG_ENTRY(...)

#endif // SE_LOGGING

#ifdef __cplusplus
}
#endif

#endif