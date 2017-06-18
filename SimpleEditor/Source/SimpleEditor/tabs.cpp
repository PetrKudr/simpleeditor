#include "base\files.h"
#include "resources\res.hpp"
#include "notepad.hpp"

const wchar_t TabControlClassName[] = L"TabControlClass";  
const wchar_t TabClassName[]        = L"TabClass"; 

// Colors by default
const COLORREF TAB_NORM_COLOR        = RGB(150, 150, 150);
const COLORREF TAB_MOD_COLOR         = RGB(150, 150, 150);
const COLORREF TAB_CURNORM_COLOR     = RGB(255, 255, 255);
const COLORREF TAB_CURMOD_COLOR      = RGB(255, 255, 200);
const COLORREF TAB_BUSY_COLOR        = RGB(200, 255, 255);
const COLORREF TAB_CURBUSY_COLOR     = RGB(200, 255, 255);
const COLORREF TAB_FRAME_COLOR       = RGB(0, 0, 0);
const COLORREF TAB_CURFRAME_COLOR    = RGB(200, 200, 200);
const COLORREF TAB_TEXT_COLOR        = RGB(0, 0, 0);
const COLORREF TAB_CURTEXT_COLOR     = RGB(0, 0, 0);
const COLORREF TAB_PROGRESSBAR_COLOR = RGB(200, 0, 0);
const COLORREF TAB_BG_COLOR          = RGB(190, 190, 190);

// Messages from tab window

struct TabMessageHeader
{
  int type;
  HWND hwnd;
};

struct TabMouseMessage
{
  TabMessageHeader header;
  int x;
  int y;
};

const int TABMSG_PUSHED     = 1;
const int TABMSG_CLICKED    = 2;
const int TABMSG_DBLCLICKED = 3;
const int TABMSG_LONGTAP    = 4;

const int TAB_MOUSE_MESSAGE = 1;


CTabs::CTabs()
{
  colors.normColor = TAB_NORM_COLOR;
  colors.modColor = TAB_MOD_COLOR;
  colors.activeNormColor = TAB_CURNORM_COLOR;
  colors.activeModColor = TAB_CURMOD_COLOR;
  colors.busyColor = TAB_BUSY_COLOR;
  colors.activeBusyColor = TAB_CURBUSY_COLOR;
  colors.frameColor = TAB_FRAME_COLOR;
  colors.activeFrameColor = TAB_CURFRAME_COLOR;
  colors.textColor = TAB_TEXT_COLOR;
  colors.activeTextColor = TAB_CURTEXT_COLOR;
  colors.progressBarColor = TAB_PROGRESSBAR_COLOR;
  colors.BGColor = TAB_BG_COLOR;
  hfont = (HFONT)GetStockObject(SYSTEM_FONT);

  hwndLeft = hwndRight = 0;
  maxWidth = 0;  // it means that the tab can have any width
  forcedVisibility = false;
}

// Function returns coordinates of buttons
void CTabs::getButtonsPos(SWindowPos &left, SWindowPos &right)
{
  SIZE szLeft, szRight;

  HDC hdc = GetDC(hwnd);
  SelectObject(hdc, hfont);
  GetTextExtentPoint(hdc, LeftCaption, wcslen(LeftCaption), &szLeft);
  GetTextExtentPoint(hdc, RightCaption, wcslen(RightCaption), &szRight);
  ReleaseDC(hwnd, hdc);

  left.y = right.y = 0;
  left.height = right.height = height;
  left.width = szLeft.cx + 2 * BUTTON_MARGINLR;
  right.width = szRight.cx + 2 * BUTTON_MARGINLR;
  right.x = width - right.width;
  left.x = right.x - left.width;

  leftButtonWidth = left.width;
  rightButtonWidth = right.width;
}

// Function returns coordinates of the tab with index "index"
// Return value is true if the tab is visible, false otherwise
bool CTabs::getTabPos(int index, SWindowPos &pos)
{
  int tabsSpace = width - (buttons ? leftButtonWidth + rightButtonWidth : 0);
  
  memset(&pos, 0, sizeof(SWindowPos));
  for (int i = first; i < index; i++)
    pos.x += tabs[i].width;
  if (index >= first && pos.x < tabsSpace)
  {
    pos.width = (pos.x + tabs[index].width > tabsSpace) ? tabsSpace - pos.x : tabs[index].width;
    pos.height = height;
    return true;
  }
  return false;
}

// Function repaints one tab
void CTabs::repaintTab(int index, RECT *prc)
{
  _ASSERTE(index >= 0 && index < number);
  InvalidateRect(tabs[index].hwnd, prc, TRUE);
  UpdateWindow(tabs[index].hwnd);
}

void CTabs::resetTabVisualData(STabData &tab, const wchar_t *wstr, size_t len)
{
  SIZE sz;

  totalWidth -= tab.width;

  HDC hdc = CreateDC(L"DISPLAY", 0, 0, 0);
  SelectObject(hdc, hfont);
  GetTextExtentPoint(hdc, wstr, len, &sz);
  DeleteDC(hdc);

  tab.width = maxWidth > 0 ? min(maxWidth, sz.cx + 2*TAB_MARGINLR) : sz.cx + 2*TAB_MARGINLR;
  tab.strX = (tab.width > sz.cx) ? (tab.width - sz.cx) / 2 : 0;
  tab.strY = (height > sz.cy) ? (height + TABS_PROGRESS_BAR_WIDTH - sz.cy) / 2 : 0;  

  totalWidth += tab.width;
}


// Function registers tab window class
bool CTabs::Init()
{
#if !defined(_WIN32_WCE)
  WNDCLASSEX wc;
#else
  WNDCLASS wc;
#endif

  HBRUSH hbgBrush = CreateSolidBrush(colors.BGColor);
  if (!hbgBrush)
    return false;

  // register control window
	ZeroMemory(&wc, sizeof(wc));
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = (WNDPROC)tabControlWindowProc;
  wc.hInstance = g_hAppInstance; 
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = hbgBrush;
  wc.lpszClassName = (LPWSTR)TabControlClassName;

#ifndef _WIN32_WCE
  wc.cbSize = sizeof(wc);

  if (!RegisterClassEx(&wc))
  {
    DeleteObject(hbgBrush);
    return false;
  }
#else

  if (!RegisterClass(&wc))
    return false;
#endif

  // register tab window
  ZeroMemory(&wc, sizeof(wc));
  wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
  wc.lpfnWndProc = (WNDPROC)tabWindowProc;
  wc.hInstance = g_hAppInstance; 
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
  wc.lpszClassName = (LPWSTR)TabClassName;

#ifndef _WIN32_WCE
  wc.cbSize = sizeof(wc);

  if (!RegisterClassEx(&wc))
    return false;
#else

  if (!RegisterClass(&wc))
    return false;
#endif

  return true;
}

// Function creates tab window
bool CTabs::Create()
{
  CNotepad &Notepad = CNotepad::Instance();

  if (hwnd)
    return true;

  x = y = width = height = 0;
  number = active = first = 0;
  buttons = visible = false;
  totalWidth = 0;

  hwnd = CreateWindow(TabControlClassName, L"", TABS_WINDOW_STYLE, 0, 0, 0, 0, 
                      Notepad.Interface.GetHWND(), 0, g_hAppInstance, 0);

  if (!hwnd)
  {
    CError::SetError(NOTEPAD_CREATE_WINDOW_ERROR);
    return false;
  }

  hwndLeft = CreateWindow(L"BUTTON", L"<", TABS_BUTTON_STYLE, 0, 0, 0, 0,
                          hwnd, 0, g_hAppInstance, 0);
  hwndRight = CreateWindow(L"BUTTON", L">", TABS_BUTTON_STYLE, 0, 0, 0, 0,
                           hwnd, 0, g_hAppInstance, 0);

  if (!hwndLeft || !hwndRight)
  {
    CError::SetError(NOTEPAD_CREATE_WINDOW_ERROR);
    return false;
  }

  return true;
}

void CTabs::Destroy()
{
  if (hwnd)
    DestroyWindow(hwnd);
  hwnd = 0;
}

// Function returns font
HFONT CTabs::GetFont()
{
  return hfont;
}

// Function adds a new tab
bool CTabs::Add(const std::wstring &name)
{
  CNotepad &Notepad = CNotepad::Instance();

  _ASSERTE(hwnd && number < MAX_DOCUMENTS);

  tabs[number].hwnd = CreateWindow(TabClassName, name.c_str(), TABS_TAB_STYLE, 0, 0, 0, 0,
                                   hwnd, 0, g_hAppInstance, 0);
  if (!tabs[number].hwnd)
  {
    CError::SetError(NOTEPAD_CREATE_WINDOW_ERROR);
    return false;
  }
  tabs[number].type = TAB_NORMAL;
  tabs[number].visible = false;
  tabs[number].progress = 0;
  resetTabVisualData(tabs[number], name.c_str(), name.length());

  SendMessage(tabs[number].hwnd, WM_SETFONT, (WPARAM)hfont, 0);
  ++number;

  if (number >= Notepad.Options.MinForVisibleTabs() && !visible)
  {
    // window is not visible and it's time to make it visible (number > 1)

    visible = true;
    ShowWindow(hwnd, SW_SHOW);
    Notepad.Interface.Resize(Notepad.Interface.GetWidth(), Notepad.Interface.GetHeight(), false);
  } 
  else if (visible)
  {
    SWindowPos pos;

    // turn on buttons if necessary
    getTabPos(number - 1, pos);
    if (!buttons && pos.x + tabs[number - 1].width > width)
    {
      ShowWindow(hwndLeft, SW_SHOW);
      ShowWindow(hwndRight, SW_SHOW);
      buttons = true;
    }
    Move(x, y, width, height);  // just repaint window
  }

  return true;
}

// Function deletes a tab
bool CTabs::Delete(int index)
{
  CNotepad &Notepad = CNotepad::Instance();

  _ASSERTE(hwnd && index >= 0 && index < number);

  if (active == index)
    active = 0; // it does not make tab with index "0" active 
  else if (index < active)
    --active;

  DestroyWindow(tabs[index].hwnd);
  for (int i = index; i < number - 1; i++) 
    tabs[i] = tabs[i + 1];
  --number;
  totalWidth -= tabs[number].width;
  memset(&tabs[number], 0, sizeof(STabData));

  if (number < Notepad.Options.MinForVisibleTabs() && visible && !forcedVisibility)
  {
    visible = false;
    ShowWindow(hwnd, SW_HIDE);
    Notepad.Interface.Resize(Notepad.Interface.GetWidth(), Notepad.Interface.GetHeight(), false);
  }
  else if (visible)
    Move(x, y, width, height);  // just repaint window
  
  return true;
}

// Function moves view point left or right
bool CTabs::MoveViewPoint(int count)
{
  _ASSERTE(hwnd);

  if (first > 0 && count < 0)
  {
    first = max(first + count, 0);
    Move(x, y, width, height);
    return true;
  }
  else if (first < number - 1 && count > 0)
  {
    first = min(first + count, number - 1);
    Move(x, y, width, height);
    return true;
  }
  return false;
}

// Function moves view point so that it could be seen
// Return value is true if all tabs were repainted, false otherwise
bool CTabs::MoveViewPointToTab(int index)
{
  _ASSERTE(hwnd && index >= 0 && index < number);

  int oldFirst = first;

  if (first > index)
    first = index;
  else if (first < index)
  {
    SWindowPos pos;

    while (first < number - 1 && (!getTabPos(index, pos) || pos.width < tabs[index].width))
      ++first;
  }
  
  if (oldFirst == first)
    return false;

   Move(x, y, width, height);

   return true;
}

// Function sets active tab
bool CTabs::SetActive(int index)
{
  _ASSERTE(hwnd && index >= 0 && index < number);

  // if tab is already active or tabs window is not visible we do nothing and return true
  if ((tabs[index].type & TAB_ACTIVE) || !visible)
    return false;

  tabs[active].type &= (~TAB_ACTIVE); 
  tabs[index].type |= TAB_ACTIVE;
  if (!MoveViewPointToTab(index))
  {
    repaintTab(index, 0);
    repaintTab(active, 0);
  }
  active = index;

  return true;
}

// Function make tab preactive (tab has the same color as active)
bool CTabs::SetPreActive(int index, bool isPreActive)
{
  _ASSERTE(hwnd && index >= 0 && index < number);

  if (isPreActive && (tabs[index].type & (TAB_PREACTIVE | TAB_ACTIVE)))
    return false;
  else if (!isPreActive && !(tabs[index].type & TAB_PREACTIVE))
    return false;
  tabs[index].type ^= TAB_PREACTIVE;
  return true;
}

// Function sets tab type (TAB_NORMAL or TAB_MODIFIED)
bool CTabs::SetModified(int index, bool isModified)
{
  _ASSERTE(hwnd && index >= 0 && index < number);

  if (isModified && (tabs[index].type & TAB_MODIFIED))
    return false;
  else if (!isModified && (tabs[index].type & TAB_NORMAL))
    return false;

  _ASSERTE(((tabs[index].type & TAB_NORMAL) && !(tabs[index].type & TAB_MODIFIED)) || 
           (!(tabs[index].type & TAB_NORMAL) && (tabs[index].type & TAB_MODIFIED)));



  tabs[index].type ^= (TAB_NORMAL | TAB_MODIFIED);
  DocNameChanged(index);

  return true;
}

// Function sets tab type as TAB_BUSY
bool CTabs::SetBusy(int index, bool isBusy)
{
  _ASSERTE(hwnd && index >= 0 && index < number);

  if (isBusy && (tabs[index].type & TAB_BUSY))
    return false; 
  else if (!isBusy && !(tabs[index].type & TAB_BUSY))
    return false;

  tabs[index].type ^= TAB_BUSY;
  if (visible && index >= first)
    repaintTab(index, 0);

  return true;
}


// Function shows operation progress using a tab
bool CTabs::SetProgress(int index, int progress)
{
  _ASSERTE(hwnd && index >= 0 && index < number);

  if (tabs[index].progress == progress)
    return true;

  RECT rc;
  rc.top = 0;
  rc.left = tabs[index].progress;
  rc.bottom = TABS_PROGRESS_BAR_WIDTH;
  rc.right = tabs[index].progress = progress > tabs[index].width ? tabs[index].width : progress;

  repaintTab(index, &rc);
  return true;
}

// Function updates tab data and repaints tabs window
void CTabs::DocNameChanged(int index)
{
 CNotepad &Notepad = CNotepad::Instance();
 const std::wstring &wstr = Notepad.Documents.GetName(index);

 resetTabVisualData(tabs[index], wstr.c_str(), wstr.length());

 if (visible)
   Move(x, y, width, height);
}

// Function 1) shows the tab window despite of the number of tabs
//          2) hides the tab winsow if the number of tabs < MinTabsForVisible
void CTabs::SetForcedVisibility(bool forcedVisibility)
{ 
  this->forcedVisibility = forcedVisibility; 

  if (forcedVisibility && !visible)
  {
    CNotepad &Notepad = CNotepad::Instance();

    visible = true;
    ShowWindow(hwnd, SW_SHOW);
    Notepad.Interface.Resize(Notepad.Interface.GetWidth(), Notepad.Interface.GetHeight());
  } 
  else if (!forcedVisibility && visible)
  {
    CNotepad &Notepad = CNotepad::Instance();

    if (number < Notepad.Options.MinForVisibleTabs())
    {
      visible = false;
      ShowWindow(hwnd, SW_HIDE);
      Notepad.Interface.Resize(Notepad.Interface.GetWidth(), Notepad.Interface.GetHeight());
    }
  }
}


// Function moves tabs window
void CTabs::Move(int x, int y, int width, int height)
{
  SWindowPos pos;
  int i;

  _ASSERTE(hwnd && first >= 0);

  if (!visible)
    return;

  // reset string positions in all tabs
  if (this->height != height)
  {
    this->width = width;
    this->height = height;

    CNotepad &Notepad = CNotepad::Instance();
    for (i = 0; i < number; i++)
    {
      const std::wstring &wstr = Notepad.Documents.GetName(i);
      resetTabVisualData(tabs[i], wstr.c_str(), wstr.length());
    }
  }

  this->x = x;
  this->y = y;
  this->width = width;
  this->height = height;

  MoveWindow(hwnd, x, y, width, height, FALSE);

  // move buttons
  // at first turn them on/off if necessary
  if (!buttons && totalWidth > width)
  {
    ShowWindow(hwndLeft, SW_SHOW);
    ShowWindow(hwndRight, SW_SHOW);
    buttons = true;
  }
  else if (buttons && totalWidth <= width)
  {
    first = 0;
    ShowWindow(hwndLeft, SW_HIDE);
    ShowWindow(hwndRight, SW_HIDE);
    buttons = false;
  }

  // then move buttons
  if (buttons)
  {
    SWindowPos rightPos;

    getButtonsPos(pos, rightPos);
    MoveWindow(hwndLeft, pos.x, pos.y, pos.width, pos.height, FALSE);
    MoveWindow(hwndRight, rightPos.x, rightPos.y, rightPos.width, rightPos.height, FALSE);
  }

  // move all tabs
  if (first >= number)
    first = max(number - 1, 0);

  for (i = 0; i < first; i++)
  {
    if (tabs[i].visible)
    {
      tabs[i].visible = false;
      ShowWindow(tabs[i].hwnd, SW_HIDE);
    }
  }
  for (i = first; i < number && getTabPos(i, pos); i++)
  {
    if (!tabs[i].visible)
    {
      tabs[i].visible = true;
      ShowWindow(tabs[i].hwnd, SW_SHOW);
    }
    MoveWindow(tabs[i].hwnd, pos.x, pos.y, pos.width, pos.height, FALSE);
  }
  for (int k = i; k < number; k++)
  {
    if (tabs[k].visible)
    {
      tabs[k].visible = false;
      ShowWindow(tabs[k].hwnd, SW_HIDE);
    }
  }

  InvalidateRect(hwnd, NULL, TRUE);
  UpdateWindow(hwnd);
}

void CTabs::Repaint()
{
  _ASSERTE(hwnd);
  if (visible)
  {
    InvalidateRect(hwnd, NULL, TRUE);
    UpdateWindow(hwnd);
  }
}

bool CTabs::Available()
{
  return hwnd ? true : false;
}

bool CTabs::Visible()
{
  return visible;
}

int CTabs::GetIndiceFromWindow(HWND hwndTab)
{
  for (int i = 0; i < number; i++)
    if (tabs[i].hwnd == hwndTab)
      return i;
  return -1;
}

int CTabs::GetButtonFromWindow(HWND hwnd)
{
  if (hwnd == hwndRight) 
    return TABS_RIGHT_BUTTON;
  else if (hwnd == hwndLeft) 
    return TABS_LEFT_BUTTON;
  return TABS_NO_BUTTON;
}

STabData& CTabs::GetTabData(int index)
{
  _ASSERTE(hwnd && index >= 0 && index < number);
  return tabs[index];
}

COLORREF CTabs::GetTabColor(int index)
{
  _ASSERTE(hwnd && index >= 0 && index < number);

  if (tabs[index].type & (TAB_ACTIVE | TAB_PREACTIVE))
  {
    if (tabs[index].type & TAB_BUSY)
      return colors.activeBusyColor;
    if (tabs[index].type & TAB_MODIFIED)
      return colors.activeModColor;
    return colors.activeNormColor;
  }


  switch (tabs[index].type)
  {
  case TAB_NORMAL:
    return colors.normColor;

  case TAB_MODIFIED:
    return colors.modColor;

  case (TAB_NORMAL | TAB_BUSY):
  case (TAB_MODIFIED | TAB_BUSY):
    return colors.busyColor;
  }

  return RGB(255, 255, 255);
}

COLORREF CTabs::GetFrameColor(int index)
{
  if (tabs[index].type & (TAB_ACTIVE | TAB_PREACTIVE))
    return colors.activeFrameColor;
  return colors.frameColor;
}

COLORREF CTabs::GetTextColor(int index)
{
  if (tabs[index].type & (TAB_ACTIVE | TAB_PREACTIVE))
    return colors.activeTextColor;
  return colors.textColor;
}

COLORREF CTabs::GetProgressBarColor(int index)
{
  return colors.progressBarColor;
}

COLORREF CTabs::GetBGColor()
{
  return colors.BGColor;
}

const CTabColors& CTabs::GetColors()
{
  return colors;
}

void CTabs::SetColors(const CTabColors &colors, unsigned int flags)
{
  if (SET_NORMCOLOR & flags)
    this->colors.normColor = colors.normColor;
  if (SET_MODCOLOR & flags)
    this->colors.modColor = colors.modColor;
  if (SET_ACTNORMCOLOR & flags)
    this->colors.activeNormColor = colors.activeNormColor;
  if (SET_ACTMODCOLOR & flags)
    this->colors.activeModColor = colors.activeModColor;
  if (SET_BUSYCOLOR & flags)
    this->colors.busyColor = colors.busyColor;
  if (SET_ACTBUSYCOLOR & flags)
    this->colors.activeBusyColor = colors.activeBusyColor;
  if (SET_FRAMECOLOR & flags)
    this->colors.frameColor = colors.frameColor;
  if (SET_ACTFRAMECOLOR & flags)
    this->colors.activeFrameColor = colors.activeFrameColor;
  if (SET_TEXTCOLOR & flags)
    this->colors.textColor = colors.textColor;
  if (SET_ACTTEXTCOLOR & flags)
    this->colors.activeTextColor = colors.activeTextColor;
  if (SET_PROGRESSBARCOLOR & flags)
    this->colors.progressBarColor = colors.progressBarColor;
  if (SET_BGCOLOR & flags)
    this->colors.BGColor = colors.BGColor;
}

void CTabs::SetFont(HFONT hfont)
{
  this->hfont = hfont != 0 ? hfont : (HFONT)GetStockObject(SYSTEM_FONT);
}


LRESULT CALLBACK CTabs::tabControlWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
  case WM_COMMAND:
    {
      CNotepad &Notepad = CNotepad::Instance();
      int button = Notepad.Interface.Tabs.GetButtonFromWindow((HWND)lParam);

      if (button != TABS_NO_BUTTON)
      {
        if (TABS_LEFT_BUTTON == button)
          Notepad.Interface.Tabs.MoveViewPoint(-1);
        else
          Notepad.Interface.Tabs.MoveViewPoint(1);
        Notepad.Documents.SetActive(Notepad.Documents.Current()); // restore focus
      }
      else if (TAB_MOUSE_MESSAGE == LOWORD(wParam)) // this is a message from tab
      {
        TabMouseMessage *pmouseMessage = (TabMouseMessage*)lParam;
        int tabIndex = Notepad.Interface.Tabs.GetIndiceFromWindow(pmouseMessage->header.hwnd);

        if (TABMSG_CLICKED == pmouseMessage->header.type)
          Notepad.Documents.SetActive(tabIndex);
        else if (TABMSG_DBLCLICKED == pmouseMessage->header.type)
          Notepad.Documents.Close(tabIndex);
        else if (TABMSG_LONGTAP == pmouseMessage->header.type)
        {
          POINT pt;
          pt.x = pmouseMessage->x;
          pt.y = pmouseMessage->y;

          ClientToScreen(pmouseMessage->header.hwnd, &pt);
          TrackPopupMenu(Notepad.Interface.Controlbar.GetTabPopupMenu(), TPM_LEFTALIGN, pt.x, pt.y, 0, pmouseMessage->header.hwnd, NULL); 
        }
      }
    }
    break;

  default:
    return DefWindowProc(hwnd, msg, wParam, lParam);
  }
  return 0;
}

LRESULT CALLBACK CTabs::tabWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
  case WM_PAINT:
    { 
      CNotepad &Notepad = CNotepad::Instance();

      int index = Notepad.Interface.Tabs.GetIndiceFromWindow(hwnd);
      STabData &tabData = Notepad.Interface.Tabs.GetTabData(index);

      COLORREF tabColor = Notepad.Interface.Tabs.GetTabColor(index);
      COLORREF tabTextColor = Notepad.Interface.Tabs.GetTextColor(index);

      HBRUSH hbrTab = CreateSolidBrush(tabColor);
      if (!hbrTab)
      {
        // TODO: handle error
        ValidateRect(hwnd, NULL);
        break;
      }

      COLORREF frameColor = Notepad.Interface.Tabs.GetFrameColor(index);

      PAINTSTRUCT ps;
      HDC hdc = BeginPaint(hwnd, &ps);
      SelectObject(hdc, Notepad.Interface.Tabs.GetFont());

      RECT rc;
      GetClientRect(hwnd, &rc);
      if (ps.rcPaint.bottom > TABS_PROGRESS_BAR_WIDTH)
      {
        FillRect(hdc, &rc, hbrTab);
        DrawFrameRect(hdc, &rc, frameColor);

        const std::wstring &wstr = Notepad.Documents.GetName(index);
        SetBkColor(hdc, tabColor); 
        SetTextColor(hdc, tabTextColor);
        ExtTextOut(hdc, tabData.strX, tabData.strY, 0, 0, wstr.c_str(), wstr.length(), 0);
      }
      else
      {
        FillRect(hdc, &ps.rcPaint, hbrTab);
        DrawFrameRect(hdc, &rc, frameColor);
      }

      // paint the progress bar
      if (ps.rcPaint.left < tabData.progress &&  ps.rcPaint.top < TABS_PROGRESS_BAR_WIDTH)
      {
        HBRUSH hbrProg = CreateSolidBrush(Notepad.Interface.Tabs.GetProgressBarColor(index));
        if (hbrProg)
        {
          rc.left = ps.rcPaint.left;
          rc.right = tabData.progress;
          rc.bottom = rc.top + TABS_PROGRESS_BAR_WIDTH;
          FillRect(hdc, &rc, hbrProg);
          DeleteObject(hbrProg);
        }
        // else {} TODO: handle error
      }

      EndPaint(hwnd, &ps);
      DeleteObject(hbrTab);

      /*hbmp = LoadBitmap(Notepad.GetHInstance(), bmpName);
      if (!hbmp)
      {
        // TODO: show error
        showSystemError(GetLastError());

        break;
      }

      hmemDC = CreateCompatibleDC(pdis->hDC);
      if (!hmemDC)
      {
        // TODO: show error
        DeleteObject(hbmp);
        break;
      }

      SelectObject(hmemDC, hbmp);
      StretchBlt(pdis->hDC, 
                 pdis->rcItem.left, pdis->rcItem.top, 
                 pdis->rcItem.right - pdis->rcItem.left, pdis->rcItem.bottom - pdis->rcItem.top,
                 hmemDC,
                 0, 0, TAB_BMP_WIDTH, TAB_BMP_HEIGHT,
                 SRCCOPY);

      DeleteDC(hmemDC);
      DeleteObject(hbmp);*/
    }
    break;

  case WM_LBUTTONDOWN:
    {
      SetWindowLong(hwnd, GWL_USERDATA, 1); // flag that this window receive LBUTTONDOWN message
      SetCapture(hwnd);

      CNotepad &Notepad = CNotepad::Instance();
      if (Notepad.Interface.Tabs.SetPreActive(Notepad.Interface.Tabs.GetIndiceFromWindow(hwnd), true))
      {
        InvalidateRect(hwnd, NULL, FALSE);
        UpdateWindow(hwnd);
      }

#if defined(_WIN32_WCE)
      if (Notepad.Interface.IdentifyLongTap(hwnd, (short)(LOWORD(lParam)), (short)(HIWORD(lParam))))
      {
        if (Notepad.Interface.Tabs.SetPreActive(Notepad.Interface.Tabs.GetIndiceFromWindow(hwnd), false))
        {
          InvalidateRect(hwnd, NULL, FALSE);
          UpdateWindow(hwnd);
        }

        TabMouseMessage mm;
        mm.header.type = TABMSG_LONGTAP;
        mm.header.hwnd = hwnd;
        mm.x = (int)(LOWORD(lParam));
        mm.y = (int)(HIWORD(lParam));
        SendMessage(GetParent(hwnd), WM_COMMAND, TAB_MOUSE_MESSAGE, reinterpret_cast<LPARAM>(&mm));  
      }
#endif      
    }
    break;

  case WM_LBUTTONUP:
    if (GetWindowLong(hwnd, GWL_USERDATA))
    {
      SetWindowLong(hwnd, GWL_USERDATA, 0);
      ReleaseCapture();

      CNotepad &Notepad = CNotepad::Instance();
      Notepad.Interface.Tabs.SetPreActive(Notepad.Interface.Tabs.GetIndiceFromWindow(hwnd), false);

      RECT rc;
      if (GetClientRect(hwnd, &rc) && LOWORD(lParam) <= rc.right && HIWORD(lParam) <= rc.bottom)
      {
        TabMouseMessage mm;
        mm.header.type = TABMSG_CLICKED;
        mm.header.hwnd = hwnd;
        mm.x = (int)(LOWORD(lParam));
        mm.y = (int)(HIWORD(lParam));
        SendMessage(GetParent(hwnd), WM_COMMAND, TAB_MOUSE_MESSAGE, reinterpret_cast<LPARAM>(&mm));  
      }
      else
      {
        InvalidateRect(hwnd, NULL, FALSE);
        UpdateWindow(hwnd);
      }
    }
    break;

  case WM_LBUTTONDBLCLK:
    {
      TabMouseMessage mm;
      mm.header.type = TABMSG_DBLCLICKED;
      mm.header.hwnd = hwnd;
      mm.x = (int)(LOWORD(lParam));
      mm.y = (int)(HIWORD(lParam));
      SendMessage(GetParent(hwnd), WM_COMMAND, TAB_MOUSE_MESSAGE, reinterpret_cast<LPARAM>(&mm));  
    }
    break;

#ifndef UNDER_CE

  case WM_RBUTTONUP:
    {
      TabMouseMessage mm;
      mm.header.type = TABMSG_LONGTAP;
      mm.header.hwnd = hwnd;
      mm.x = (int)(LOWORD(lParam));
      mm.y = (int)(HIWORD(lParam));
      SendMessage(GetParent(hwnd), WM_COMMAND, TAB_MOUSE_MESSAGE, reinterpret_cast<LPARAM>(&mm));  
    }
    break;

#endif

  case WM_COMMAND:
    {
      CNotepad &Notepad = CNotepad::Instance();

      switch (LOWORD(wParam))
      {
      case IDM_FILEINFO:
        { 
          const std::wstring& path = Notepad.Documents.GetPath(Notepad.Interface.Tabs.GetIndiceFromWindow(hwnd));

          Notepad.Interface.Dialogs.FileInfoDialog(
            Notepad.Interface.GetHWND(), 
            path,
            RetrieveFileSize(path.c_str())
          );
        }
        break;
      }
    }
    break;

  default:
    return DefWindowProc(hwnd, msg, wParam, lParam);
  }
  return 0;
}