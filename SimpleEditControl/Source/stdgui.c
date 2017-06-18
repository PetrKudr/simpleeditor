#include "stdgui.h"

#define ADDR(addr, offs) (wchar_t*)((unsigned)(argv) + (unsigned)(offs))

#if defined(_WIN32_WCE)

// возвращает указатель на меню
extern HMENU GetMenu(HWND hwnd, unsigned int menuID)
{
//  TBBUTTONINFO tbbi;
  HMENU hmenu;
  HWND hmenuWnd = SHFindMenuBar(hwnd);

  /*ZeroMemory(&tbbi, sizeof(TBBUTTONINFO));
  tbbi.cbSize = sizeof(tbbi);
  tbbi.dwMask = TBIF_LPARAM | TBIF_BYINDEX;
  SendMessage(g_g_hwndMenu, TB_GETBUTTONINFO,  0, (LPARAM)&tbbi);*/

  hmenu = (HMENU)SendMessage(hmenuWnd, SHCMBM_GETSUBMENU, (WPARAM)0, (LPARAM)menuID);
  if (!hmenu)
  {
    wchar_t *mess;
    DWORD err = GetLastError();

    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                  NULL, err, LANG_NEUTRAL, (LPWSTR)&mess, 32, NULL);

    MessageBox(hwnd, mess, L"Error", MB_OK);
    LocalFree(mess);
  }

  // return (HMENU)tbbi.lParam;
  return hmenu;
}

// возвращает позицию скроллбара
extern int GetScrollPos(HWND hwnd, int fnBar)
{
  SCROLLINFO sci;

  sci.cbSize = sizeof(SCROLLINFO);
  sci.fMask = SIF_POS;
  GetScrollInfo(hwnd, fnBar, &sci);

  return sci.nPos;
}

static void skipSpaces(wchar_t *str, int *i)
{
  while (str[*i] == L' ' || str[*i] == L'\t')
    (*i)++;
}

extern wchar_t **CommandLineToArgvW(wchar_t *str, int *_argc)
{
  wchar_t **argv;
  int argc = 0, begin = 0, end = 0;
  size_t length = wcslen(str), offset;
  char complexParam = 0, oneParam = 0;

  skipSpaces(str, &end);
  if (str[end] && str[end] != L'"')
  {
    oneParam = 1;      // если первый параметр без кавычек, то, скорее всего, вся строка - один параметр
    complexParam = 1;  
  }

  while (str[end])
  {
    skipSpaces(str, &end);
    if (str[end] == L'"')
    {
      end++;
      complexParam = 1;
    }
    while (str[end] && ((!complexParam && (str[end] != L' ' && str[end] != L'\t')) || (complexParam && str[end] != L'"')))
      end++;
    argc++;
    if (str[end] == L'"')
    {
      end++;
      complexParam = 0;
    }
  }

  offset = (argc * sizeof(wchar_t*));
  if ((argv = (wchar_t**)GlobalAlloc(GMEM_FIXED, (length + 1) * sizeof(wchar_t) + offset)) == NULL)
  {
    *_argc = 0;
    return NULL;
  }

  complexParam = 0;
  if (oneParam)
    complexParam = 1;
  begin = end = 0;
  argc = 0;
  
  while (str[end])
  {
    skipSpaces(str, &end);
    if (str[end] == L'"')
    {
      end++;
      complexParam = 1;
    }
    begin = end;

    while (str[end] && ((!complexParam && (str[end] != L' ' && str[end] != L'\t')) || (complexParam && str[end] != L'"')))
      end++;

    argv[argc++] = ADDR(arrgv, offset);
    wcsncpy(ADDR(argv, offset), &str[begin], end - begin);
    *(ADDR(argv, offset + (end - begin) * sizeof(wchar_t))) = L'\0';
    offset += (end - begin + 1) * sizeof(wchar_t);
    
    if (str[end] == L'"')
    {
      end++;
      complexParam = 0;
    }
  }

  *_argc = argc;
  
  return argv;
}
#endif