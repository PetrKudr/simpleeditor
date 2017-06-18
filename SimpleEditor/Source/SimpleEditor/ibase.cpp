#include "ibase.hpp"
#include "resources\res.hpp"


extern int g_LogPixelsY = 1;


extern int Location::MaxX = 0;
extern int Location::MaxY = 0;
static int loc_cellX = 0;
static int loc_cellY = 0;
static int loc_marginX = 0;
static int loc_marginY = 0;
static int loc_fixedX = false;
static int loc_fixedY = false;
static int loc_width = 0;
static int loc_height = 0;

void Location::SetNet(HWND hwnd, int x, int y, int marginX, int marginY, bool fixedX, bool fixedY)
{
  RECT rc;
  GetClientRect(hwnd, &rc);

  loc_width = rc.right;
  loc_height = rc.bottom;
  loc_fixedX = fixedX;
  loc_fixedY = fixedY;
  
  loc_marginX = fixedX ? marginX : 0;
  loc_marginY = fixedY ? marginY : 0;
  loc_cellX = fixedX ? x : rc.right / x;
  loc_cellY = fixedY ? y : rc.bottom / y;
  MaxX = fixedX ? rc.right / loc_cellX : x;
  MaxY = fixedY ? rc.bottom / loc_cellY : y;

}

void Location::Arrange(const HWND *windows, const SWindowPos *positions, size_t number)
{
  int x, y, width, height;

  for (size_t i = 0; i < number; i++)
  {
    if (0 == windows[i] || positions[i].x > MaxX || positions[i].y > MaxY)
      continue;

    x = loc_fixedX ? positions[i].x * loc_cellX + loc_marginX : (positions[i].x * loc_width) / MaxX;
    y = loc_fixedY ? positions[i].y * loc_cellY + loc_marginY : (positions[i].y * loc_height) / MaxY;

    width = loc_fixedX ? positions[i].width * loc_cellX - loc_marginX : (positions[i].width * loc_width) / MaxX;
    if (positions[i].x + positions[i].width == MaxX)
      width = loc_fixedX ? loc_width - x - loc_marginX : loc_width - x;

    height = loc_fixedY ? positions[i].height * loc_cellY - loc_marginY : (positions[i].height * loc_height) / MaxY; 
    if (positions[i].y + positions[i].height == MaxY)
      height = loc_fixedY ? loc_height - y - loc_marginY : loc_height - y;

    MoveWindow(windows[i], x, y, width, height, TRUE);
  }
}


#ifdef _WIN32_WCE
static HWND menu_hwnd = 0;

extern bool CreateOkCancelBar(HWND hwnd)
{
  if (menu_hwnd)
    return true;

  SHMENUBARINFO mbi;
  memset(&mbi, 0, sizeof(SHMENUBARINFO));
  mbi.cbSize = sizeof(SHMENUBARINFO);
  mbi.hwndParent = hwnd;
  mbi.nToolBarId = IDM_OKCANCEL_MENU;
  mbi.hInstRes = g_hResInstance;

  if (!SHCreateMenuBar(&mbi))
    return false;

  menu_hwnd = mbi.hwndMB;
  return true;
}

extern void DestroyOkCancelBar()
{
  if (menu_hwnd)
    DestroyWindow(menu_hwnd);
  menu_hwnd = 0;
}
#endif

extern int GetFileNameIndice(const wchar_t *path)
{
  _ASSERTE(path != 0);
  size_t len = wcslen(path);
  while (len > 0 && path[len] != L'\\')
    --len;
  return len + 1;
}