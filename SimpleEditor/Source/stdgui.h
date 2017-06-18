#if !defined(_STDGUI__) 
#define _STDGUI__

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>
#include <commctrl.h>

#ifndef _WIN32_WCE
#pragma comment(lib, "comctl32.lib")  
#else
#include <aygshell.h>
#pragma comment(lib, "aygshell.lib") 

#define FR_FINDNEXT 1
#define FR_REPLACE 2
#define FR_REPLACEALL 4
#define FR_DOWN 8
#define FR_MATCHCASE 16
#define FR_WHOLEWORD 32
#define FR_DIALOGTERM 64

typedef struct tag_FINDREPLACE
{
  size_t lStructSize;
  HWND hwndOwner;
  wchar_t *lpstrFindWhat;
  size_t wFindWhatLen;
  wchar_t *lpstrReplaceWith;
  size_t wReplaceWithLen;
  DWORD Flags;
} FINDREPLACE, *LPFINDREPLACE;


extern HMENU GetMenu(HWND hwnd, unsigned int menuID);
extern int GetMenuItemCount(HMENU hMenu);
extern int GetScrollPos(HWND hwnd, int fnBar);
extern wchar_t **CommandLineToArgvW(wchar_t *str, int *_argc);

#endif

extern int DrawFrameRect(HDC hdc, const RECT *prc, COLORREF hbrush);

#ifdef __cplusplus
}
#endif



#endif // _STDGUI__