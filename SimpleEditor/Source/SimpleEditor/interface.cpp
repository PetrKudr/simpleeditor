#include "notepad.hpp"

#ifdef _WIN32_WCE
#define TOOLBAR_ADDITIONAL_Y DOC_MARGIN
#else
#define TOOLBAR_ADDITIONAL_Y GetSystemMetrics(SM_CYFIXEDFRAME)
#endif


CInterface::CInterface()
{
  memset(MaxProgress, 1, sizeof(int) * MAX_DOCUMENTS);
  isFullScreen = false;
}



// Function resizes all windows in the application
void CInterface::Resize(int width, int height, bool repaint, bool validateDoc)
{
  CNotepad &Notepad = CNotepad::Instance();

  this->width = width;
  this->height = height;

  // we should resize cuurent document and set flag DOC_RESIZE for others
  SWindowPos coords;

  // resize tabs
  if (Tabs.Available())
  {
    CalcTabCoords(coords);
    Tabs.Move(coords.x, coords.y, coords.width, coords.height);
  }

  // resize documents
  if (Notepad.Documents.GetDocsNumber() > 0)
  {
    CalcDocCoords(coords);
    Notepad.Documents.Move(Notepad.Documents.Current(), coords.x, coords.y, 
                           coords.width, coords.height, repaint, validateDoc);
    Notepad.Documents.SetFlags(DOC_RESIZE, Notepad.Documents.IsBusy(Notepad.Documents.Current()));
  }

  // resize status bar
  if (StatusBar.IsStatusBar())
  {
    CalcStatusBarCoords(coords);
    StatusBar.Move(coords.x, coords.y, coords.width, coords.height);
  }

  // resize find bar
  if (FindBar.IsFindBar())
  {
    CalcFindBarCoords(coords);
    FindBar.Move(coords.x, coords.y, coords.width, coords.height);
  }

  // resize replace bar
  if (ReplaceBar.IsReplaceBar())
  {
    CalcReplaceBarCoords(coords);
    ReplaceBar.Move(coords.x, coords.y, coords.width, coords.height);
  }

#ifndef _WIN32_WCE
  // resize toolbar
  if (Controlbar.IsToolbar())
  {
    CalcToolbarCoords(coords);
    Controlbar.Move(coords.x, coords.y, coords.width, coords.height);
  }
#endif
}

// Function switches program into fullscreen mode
#ifdef _WIN32_WCE
void CInterface::SwitchFullScreen(bool isFullScreen, bool resize)
{
  SHFullScreen(hwnd, isFullScreen ? SHFS_HIDETASKBAR | SHFS_HIDESTARTICON : SHFS_SHOWTASKBAR | SHFS_SHOWSTARTICON);  

  if (resize)
  {
    this->isFullScreen = isFullScreen;

    RECT rc, ourRC;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &rc, FALSE);
    GetClientRect(hwnd, &ourRC);
    ourRC.bottom += isFullScreen ? rc.top : -rc.top;
    //height += isFullScreen ? rc.top : -rc.top;
    MoveWindow(hwnd, 0, isFullScreen ? 0 : rc.top, ourRC.right, ourRC.bottom, TRUE);
  } 
}
#endif

bool CInterface::IdentifyLongTap(HWND hwnd, long int x, long int y)
{
#if defined(UNDER_CE)
  SHRGINFO shrg;

  shrg.cbSize = sizeof(shrg);
  shrg.hwndClient = hwnd;
  shrg.ptDown.x = x;
  shrg.ptDown.y = y;
  shrg.dwFlags = SHRG_RETURNCMD;

  return SHRecognizeGesture(&shrg) == GN_CONTEXTMENU;

#else

  return false;

#endif
}

// Function calculates the size of the area filled by child windows (interface)
void CInterface::CalcInterfaceCoords(SWindowPos &coords, bool responseOnKeyboard)
{
  RECT rc;
  GetClientRect(hwnd, &rc);

  coords.x = coords.y = 0;
  coords.width = rc.right;
  coords.height = rc.bottom;

#ifdef _WIN32_WCE
  if (responseOnKeyboard)
  {
    SIPINFO si;
    memset(&si, 0, sizeof(si));
    si.cbSize = sizeof(si);
    if(SHSipInfo(SPI_GETSIPINFO, 0, &si, 0)) 
    {
      if (si.fdwFlags & SIPF_ON)
        coords.height -= (si.rcSipRect.bottom - si.rcSipRect.top);
    //else
    //{
      //RECT rc;
      //GetClientRect(hwnd, &rc);
      //coords.height = rc.bottom;
      //coords.height = si.rcVisibleDesktop.bottom - si.rcVisibleDesktop.top;
    //}
    }
  }
#endif
}

// Function calculates coordinates of the document
void CInterface::CalcDocCoords(SWindowPos &coords)
{
  coords.x = 0;
  coords.y = DOC_MARGIN;
  coords.width = this->width;
  coords.height = this->height - 2*DOC_MARGIN;

  SWindowPos pos;
  if (Tabs.Visible())
  {
    Tabs.GetPos(pos);
    coords.y += pos.height;
    coords.height -= pos.height;
  }

  if (Controlbar.IsToolbar())
  {
    Controlbar.GetPos(pos);
    coords.height -= pos.height + TOOLBAR_ADDITIONAL_Y;
  }

  if (FindBar.IsFindBar())
  {
    FindBar.GetPos(pos);
    coords.height -= pos.height;
  }
  
  if (ReplaceBar.IsReplaceBar())
  {
    ReplaceBar.GetPos(pos);
    coords.height -= pos.height;
  }

  if (StatusBar.IsStatusBar())
  {
    StatusBar.GetPos(pos);
    coords.height -= pos.height;
  }
}

void CInterface::CalcTabCoords(SWindowPos &coords)
{
  CNotepad &Notepad = CNotepad::Instance();

  // user-defined tab window height
  int tabWindowHeight = Notepad.Options.GetProfile().tabWindowHeight;


  // Get tabs font height
  HDC hdc = GetDC(Notepad.Interface.GetHWND());
  SelectObject(hdc, Tabs.GetFont());

  TEXTMETRIC tm;
  GetTextMetrics(hdc, &tm);

  ReleaseDC(Notepad.Interface.GetHWND(), hdc);

  // Fill coords structure
  coords.x = 0;
  coords.y = 0;
  coords.width = width;
  coords.height = max(tabWindowHeight, static_cast<int>(tm.tmHeight * 1.2)) + TABS_PROGRESS_BAR_WIDTH;
}

void CInterface::CalcToolbarCoords(SWindowPos &coords)
{
  SWindowPos pos;
  Controlbar.GetPos(pos);

  coords.x = 0;
  coords.y = height - pos.height;
  coords.width = width;
  coords.height = pos.height;
}

void CInterface::CalcFindBarCoords(SWindowPos &coords)
{
  SWindowPos pos;
  FindBar.GetPos(pos);

  coords.x = 0;
  coords.y = height - pos.height;
  coords.width = width;
  coords.height = pos.height;

  if (ReplaceBar.IsReplaceBar())
  {
    ReplaceBar.GetPos(pos);
    coords.y -= pos.height;
  }

  if (Controlbar.IsToolbar())
  {
    Controlbar.GetPos(pos);
    coords.y -= pos.height + TOOLBAR_ADDITIONAL_Y;
  }
}

void CInterface::CalcReplaceBarCoords(SWindowPos &coords)
{
  SWindowPos pos;
  FindBar.GetPos(pos);

  coords.x = 0;
  coords.y = height - pos.height;
  coords.width = width;
  coords.height = pos.height;

  if (Controlbar.IsToolbar())
  {
    Controlbar.GetPos(pos);
    coords.y -= pos.height + TOOLBAR_ADDITIONAL_Y;
  }
}

void CInterface::CalcStatusBarCoords(SWindowPos &coords)
{
  SWindowPos pos;
  StatusBar.GetPos(pos);

  coords.x = 0;
  coords.y = height - pos.height;
  coords.width = width;
  coords.height = pos.height;

  if (FindBar.IsFindBar())
  {
    FindBar.GetPos(pos);
    coords.y -= pos.height;
  }

  if (ReplaceBar.IsReplaceBar())
  {
    ReplaceBar.GetPos(pos);
    coords.y -= pos.height;
  }

  if (Controlbar.IsToolbar())
  {
    Controlbar.GetPos(pos);
    coords.y -= pos.height + TOOLBAR_ADDITIONAL_Y;
  }
}

// Function sets max progress for operation
void _cdecl CInterface::SetMaxProgress(int id, int max)
{
  CNotepad &Notepad = CNotepad::Instance();
  Notepad.Interface.MaxProgress[id] = max > 0 ? max : 1;

  if (Notepad.Interface.Tabs.Available())
    Notepad.Interface.Tabs.SetForcedVisibility(true);
}

// Function sets current progress for operation
void _cdecl CInterface::SetProgress(int id, int progress)
{
  CNotepad &Notepad = CNotepad::Instance();

  if (Notepad.Interface.Tabs.Available())
  {
    STabData &tabData = Notepad.Interface.Tabs.GetTabData(Notepad.Documents.GetIndexFromID(id));
    int tabProgress = (int)((static_cast<double>(progress) / Notepad.Interface.MaxProgress[id]) * tabData.width);
    Notepad.Interface.Tabs.SetProgress(Notepad.Documents.GetIndexFromID(id),tabProgress);
  }
}

void _cdecl CInterface::ProgressEnd()
{
  CNotepad &Notepad = CNotepad::Instance();

  if (Notepad.Interface.Tabs.Available())
    Notepad.Interface.Tabs.SetForcedVisibility(false);
}