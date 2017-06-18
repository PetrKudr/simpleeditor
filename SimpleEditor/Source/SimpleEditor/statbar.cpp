#include "statbar.hpp"
#include "error.hpp"

const double STATBAR_HEIGHT = 1.2;
const wchar_t LINE_STR[] = L"Line: ";
const wchar_t INDEX_STR[] = L"Index: ";
const int MAX_NUMBER_COUNT = 8;
const int SEPARATE_SPACE = 6; // number of pixels between static windows

// Function creates status bar
bool CStatusBar::Create(HWND hwndOwner)
{
  if (hwnd)
    return true;

  hwnd = CreateWindow(L"STATIC", L"", WS_CHILD | WS_VISIBLE | WS_BORDER, 0, 0, 0, 0, hwndOwner, NULL, g_hResInstance, 0);
  if (!hwnd)
  {
    CError::SetError(NOTEPAD_CREATE_WINDOW_ERROR);
    return false;
  }

  hlineStat = CreateWindow(L"STATIC", LINE_STR, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd, NULL, g_hResInstance, 0);
  hlineWnd = CreateWindow(L"STATIC", L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd, NULL, g_hResInstance, 0);
  hindStat = CreateWindow(L"STATIC", INDEX_STR, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd, NULL, g_hResInstance, 0);
  hindWnd = CreateWindow(L"STATIC", L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd, NULL, g_hResInstance, 0);
  if (!hlineStat || ! hlineWnd || !hindStat || !hindWnd)
  {
    Destroy();
    CError::SetError(NOTEPAD_CREATE_WINDOW_ERROR);
    return false;
  }
  
  SendMessage(hlineStat, WM_SETFONT, (WPARAM)hfont, 0);
  SendMessage(hlineWnd, WM_SETFONT, (WPARAM)hfont, 0);
  SendMessage(hindStat, WM_SETFONT, (WPARAM)hfont, 0);
  SendMessage(hindWnd, WM_SETFONT, (WPARAM)hfont, 0);

  HDC hdc = CreateDC(L"DISPLAY", 0, 0, 0);
  if (!hdc)
  {
    Destroy();
    CError::SetError(NOTEPAD_CREATE_RESOURCE_ERROR);
    return false;
  }
  SelectObject(hdc, hfont);

  TEXTMETRIC tm;
  GetTextMetrics(hdc, &tm);

  SIZE sz;
  GetTextExtentExPoint(hdc, LINE_STR, wcslen(LINE_STR), 0, NULL, NULL, &sz);
  lineStrWidth = sz.cx;
  GetTextExtentExPoint(hdc, INDEX_STR, wcslen(INDEX_STR), 0, NULL, NULL, &sz);
  indStrWidth = sz.cx;

  DeleteDC(hdc);

  aveCharWidth = tm.tmAveCharWidth;
  this->height = static_cast<int>(STATBAR_HEIGHT * tm.tmHeight);

  return true;
}

void CStatusBar::Destroy()
{
  if (hwnd)
    DestroyWindow(hwnd);
  hwnd = 0;
  oldX = oldY = -1;
}

void CStatusBar::Move(int x, int y, int width, int height)
{
  // Move status bar
  MoveWindow(hwnd, x, y, width, height, TRUE);

  // Move child windows
  int currentX = 0, currentWidth = lineStrWidth;

  MoveWindow(hlineStat, currentX, 0, currentWidth, height, TRUE);
  currentX += currentWidth;

  currentWidth = aveCharWidth * MAX_NUMBER_COUNT;
  MoveWindow(hlineWnd, currentX, 0, currentWidth, height, TRUE);
  currentX += currentWidth + SEPARATE_SPACE;

  currentWidth = indStrWidth;
  MoveWindow(hindStat, currentX, 0, currentWidth, height, TRUE);
  currentX += currentWidth;

  currentWidth = aveCharWidth * MAX_NUMBER_COUNT;
  MoveWindow(hindWnd, currentX, 0, currentWidth, height, TRUE);
}

void CStatusBar::SetPosition(unsigned int x, unsigned int y)
{
  if (!hwnd)
    return;

  wchar_t wstr[16];
  wstr[0] = L'\0';


#if !defined(_WIN32_WCE)
  if (oldY != y)
  {
    if (y != -1)
      swprintf(wstr, 16, L"%u\0", y + 1);
    SetWindowText(hlineWnd, wstr);
    oldY = y;
  }
  if (oldX != x)
  {
    if (x != -1)
      swprintf(wstr, 16, L"%u\0", x);
    SetWindowText(hindWnd, wstr);
    oldX = x;
  }
#else
  if (oldY != y)
  {
    if (y != -1)
      swprintf(wstr, L"%u\0", y + 1);
    SetWindowText(hlineWnd, wstr);
    oldY = y;
  }
  if (oldX != x)
  {
    if (x != -1)
      swprintf(wstr, L"%u\0", x);
    SetWindowText(hindWnd, wstr);
    oldX = x;
  }
#endif
}