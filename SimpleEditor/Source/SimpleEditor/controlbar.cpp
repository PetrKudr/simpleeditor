#include "..\debug.h"

#include "resources\res.hpp"
#include "base\qinsert.h"
#include "notepad.hpp"

#include <fstream>


#define TOOLBAR_STYLE (WS_CHILD | WS_VISIBLE | WS_BORDER | WS_CLIPSIBLINGS | CCS_BOTTOM)
#define TOOLBAR_FILENAME L"\\toolbar.tlb"
const int MAX_BUTTONS = 25;

#define QINSERT_FILENAME L"\\qinsert.txt"

// Description of button
struct STBButton
{
  char name[16];
  int id;  // number of picture in the bitmap
  int cmd; // command
  int style;
};

static STBButton s_btnInfo[] =
{
  {"FILE",      0,    IDM_FILE,         TBSTYLE_BUTTON},
  {"MENU",      1,    IDM_MENU,         TBSTYLE_BUTTON}, 
  {"NEW",       2,    CMD_NEWDOC,       TBSTYLE_BUTTON},
  {"OPEN",      5,    CMD_OPENDOC,      TBSTYLE_BUTTON},
  {"CLOSE",     3,    CMD_CLOSEDOC,     TBSTYLE_BUTTON},
  {"SAVE",      6,    CMD_SAVEDOC,      TBSTYLE_BUTTON},
  {"SAVEAS",    7,    CMD_SAVEASDOC,    TBSTYLE_BUTTON},
  {"SAVEALL",   8,    CMD_SAVEALL,      TBSTYLE_BUTTON},
  {"UNDO",      9,    CMD_UNDO,         TBSTYLE_BUTTON},
  {"REDO",      10,   CMD_REDO,         TBSTYLE_BUTTON},
  {"QINSERT",   11,   IDM_QINSERT_T,    TBSTYLE_BUTTON},
  {"FIND",      12,   IDM_FIND,         TBSTYLE_CHECK },
  {"COPY",      13,   CMD_COPY,         TBSTYLE_BUTTON},
  {"CUT",       14,   CMD_CUT,          TBSTYLE_BUTTON},
  {"PASTE",     15,   CMD_PASTE,        TBSTYLE_BUTTON},
  {"DRAG",      16,   CMD_DRAGMODE,     TBSTYLE_CHECK },
  {"REPLACE",   17,   IDM_REPLACE,      TBSTYLE_CHECK },
  {"SEPARATOR", 0,    0,                TBSTYLE_SEP   }
};

// Paths to popup menus
static int s_menuPaths[DROPDOWN_MENUS_NUMBER][5] = {
  {0, -1},
  {1, -1},
  {0, 3, -1},
  {0, 4, -1},
  {0, 10, -1},
  {1, 0, -1},
  {1, 1, -1},
  {1, 3, -1}
};


CControlbar::CControlbar()
{
  hdocPopupMenu = htabPopupMenu = hpluginsPopupMenu = hqinsertMenu = 0;
  myHwndOwner = 0;
  lastQInsertID = QINSERT_FIRST_ID;
}

// Function changes color of pixels (pixels with the color of rigth bottom pixel) to bgColor
bool CControlbar::transformBitmap(HBITMAP hBitmap, COLORREF bgColor)
{
  BITMAP bm;
  COLORREF color, transparentColor;
  HBITMAP bmAndBack = 0, bmAndMask = 0, bmBG = 0;
  HDC hdcMem = 0, hdcBack = 0, hdcMask = 0, hdcTemp = 0;
  HBRUSH hbr = 0;
  HPEN hpen = 0;
  POINT ptSize;
  bool success = true;

  hdcTemp = CreateCompatibleDC(NULL);
  hdcBack = CreateCompatibleDC(hdcTemp);
  hdcMask = CreateCompatibleDC(hdcTemp);

  GetObject(hBitmap, sizeof(BITMAP), (LPVOID)&bm);
  ptSize.x = bm.bmWidth;         
  ptSize.y = bm.bmHeight;         
  //DPtoLP(hdcTemp, &ptSize, 1);  


  // Monochrome DCs
  bmAndBack = CreateBitmap(ptSize.x, ptSize.y, 1, 1, 0);
  bmAndMask = CreateBitmap(ptSize.x, ptSize.y, 1, 1, 0);

  if (!hdcTemp || !hdcBack || !hdcMask || !bmAndBack || !bmAndMask)
  {
    CError::SetError(NOTEPAD_CREATE_RESOURCE_ERROR);
    goto LABEL_TBM_ERROR;
  }
 
  // Select each bitmap in proper DC
  SelectObject(hdcTemp, hBitmap);
  SelectObject(hdcBack, bmAndBack);
  SelectObject(hdcMask, bmAndMask);


  // Create bitmap for background
  hbr = CreateSolidBrush(bgColor);
  hpen = CreatePen(PS_SOLID, 1, bgColor);
  hdcMem = CreateCompatibleDC(hdcTemp);
  bmBG = CreateCompatibleBitmap(hdcTemp, ptSize.x, ptSize.y);

  if (!hdcMem || !bmBG || !hbr || !hpen)
  {
    CError::SetError(NOTEPAD_CREATE_RESOURCE_ERROR);
    goto LABEL_TBM_ERROR;
  }

  SelectObject(hdcMem, bmBG);
  SelectObject(hdcMem, hbr);
  SelectObject(hdcMem, hpen);
  Rectangle(hdcMem, 0, 0, ptSize.x, ptSize.y);


  // Transparent color will be color of the last pixel in the bitmap
  transparentColor = GetPixel(hdcTemp, ptSize.x - 1, ptSize.y - 1); 

  //SetMapMode(hdcTemp, GetMapMode(hdc));

  // Create mask
  color = SetBkColor(hdcTemp, transparentColor);
  BitBlt(hdcMask, 0, 0, ptSize.x, ptSize.y, hdcTemp, 0, 0, SRCCOPY);
  SetBkColor(hdcTemp, color);

  // Create inversed mask
  BitBlt(hdcBack, 0, 0, ptSize.x, ptSize.y, hdcMask, 0, 0, NOTSRCCOPY);

  // apply mask to the BG bitmap;
  BitBlt(hdcMem, 0, 0, ptSize.x, ptSize.y, hdcMask, 0, 0, SRCAND);

  // apply inversed mask to the original bitmap
  BitBlt(hdcTemp, 0, 0, ptSize.x, ptSize.y, hdcBack, 0, 0, SRCAND);

  // merge original bitmap and BG bitmap 
  BitBlt(hdcTemp, 0, 0, ptSize.x, ptSize.y, hdcMem, 0, 0, SRCPAINT);

  goto LABEL_TBM_NO_ERROR;

LABEL_TBM_ERROR:
  success = false;

LABEL_TBM_NO_ERROR:
  if (hdcMem)
    DeleteDC(hdcMem);
  if (hdcBack)
    DeleteDC(hdcBack);
  if (hdcMask)
    DeleteDC(hdcMask);
  if (hdcTemp)
    DeleteDC(hdcTemp);

  if (bmAndBack)
    DeleteObject(bmAndBack);
  if (bmAndMask)
    DeleteObject(bmAndMask);
  if (bmBG)
    DeleteObject(bmBG);
  if (hbr)
    DeleteObject(hbr);
  if (hpen)
    DeleteObject(hpen);

  return success;
}

HBITMAP CControlbar::loadBitmap()
{
  std::wstring path = g_execPath;
  int iconSize = GetSystemMetrics(SM_CYSMICON);

  if (iconSize <= 16)
    path += L"\\toolbar16.bmp";
  else if (iconSize <= 24)
    path += L"\\toolbar24.bmp";
  else if (iconSize <= 32)
    path += L"\\toolbar32.bmp";
  else 
    return 0;

#ifndef _WIN32_WCE
  HBITMAP hbmp = (HBITMAP)LoadImage(0, path.c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
#else
  HBITMAP hbmp = SHLoadImageFile(path.c_str());
#endif

  return hbmp;
}

HMENU CControlbar::getTopMenuHandle(int position)
{
#ifdef _WIN32_WCE
  if (!hwnd)
    return 0;

  int menuId;

  switch (position)
  {
  case 0:
    menuId = IDM_FILE;
    break;

  case 1:
    menuId = IDM_MENU;
    break;

  default:
    return 0;    
  }
  
  return (HMENU)SendMessage(hwnd, SHCMBM_GETSUBMENU, (WPARAM)0, menuId);

#else

  if (!myHwndOwner)
    return 0;

  HMENU hmenu = GetMenu(myHwndOwner);
  
  if (!hmenu)
    return 0;

  return GetSubMenu(hmenu, position);

#endif
}


// Function reads list of buttons from file
void CControlbar::ReadButtons()
{
  std::wstring path = g_execPath;
  path += TOOLBAR_FILENAME;

  std::ifstream fIn(path.c_str());

  if (!fIn.is_open())
    return;

  
  std::string btnName;
  while (!fIn.eof())
  {
    std::getline(fIn, btnName);
    for (int i = 0; i < MAX_BUTTONS; i++)
      if (btnName.compare(s_btnInfo[i].name) == 0)
        btnIndices.push_back(i);
  }
  

  fIn.close();
}

// Function creates standard WM5/6 menu
bool CControlbar::CreateMainMenu(HWND hwndOwner)
{
  myHwndOwner = hwndOwner;

  CNotepad &Notepad = CNotepad::Instance();

#ifdef _WIN32_WCE
  if (hwnd)
    return true;

  SHMENUBARINFO mbi;

  memset(&mbi, 0, sizeof(SHMENUBARINFO));
  mbi.cbSize = sizeof(SHMENUBARINFO);
  mbi.hwndParent = hwndOwner;
  mbi.nToolBarId = IDM_MAIN_MENU;
  mbi.hInstRes = g_hResInstance;
  mbi.clrBk = Notepad.Options.GetProfile().ToolbarColor;
  mbi.dwFlags = SHCMBF_COLORBK;

  if (!SHCreateMenuBar(&mbi))
  {
    CError::SetError(NOTEPAD_CREATE_MENU_ERROR);
    return false;
  }
  hwnd = mbi.hwndMB;

  RECT rc;
  GetClientRect(hwnd, &rc);
  x = y = 0;
  width = rc.right;
  height = rc.bottom;


  HMENU hfileMenu = (HMENU)SendMessage(hwnd, SHCMBM_GETSUBMENU, (WPARAM)0, IDM_FILE);
  HMENU hmenuMenu = (HMENU)SendMessage(hwnd, SHCMBM_GETSUBMENU, (WPARAM)0, IDM_MENU);

  if (!hfileMenu || !hmenuMenu)
  {
    CError::SetError(NOTEPAD_GET_SUBMENU_ERROR);
    return false;
  }
#else

  HMENU hmenu = LoadMenu(g_hResInstance, MAKEINTRESOURCE(IDM_MAIN_MENU));
  if (!hmenu)
  {
    CError::SetError(NOTEPAD_LOAD_RESOURCE_ERROR);
    return false;
  }
  if (!SetMenu(hwndOwner, hmenu))
  {
    CError::SetError(NOTEPAD_CREATE_MENU_ERROR);
    DestroyMenu(hmenu);
    return false;
  }

  HMENU hfileMenu = GetSubMenu(hmenu, 0);
  HMENU hmenuMenu = GetSubMenu(hmenu, 1);

  if (!hfileMenu || !hmenuMenu)
  {
    CError::SetError(NOTEPAD_GET_SUBMENU_ERROR);
    return false;
  }
#endif

  std::wstring path = g_execPath;
  path += QINSERT_FILENAME;

  hqinsertMenu = CreatePopupMenu();
  if (hqinsertMenu)
  {
    char retval = ::CreateQInsertMenu(hqinsertMenu, path.c_str(), QINSERT_FIRST_ID, &lastQInsertID);
    if (retval < 0)
    {
      DestroyMenu(hqinsertMenu);
      hqinsertMenu = 0;
      if (retval == QINSERT_MENU_ERROR)
      {
        CError::SetError(NOTEPAD_CREATE_MENU_ERROR);
        return false;
      }
    }
  }

  HMENU hdocPopupMenuContainer = LoadMenu(g_hResInstance, MAKEINTRESOURCE(IDM_DOCPOPUP_MENU));
  if (hdocPopupMenuContainer)
  {
    hdocPopupMenu = GetSubMenu(hdocPopupMenuContainer, 0);
    RemoveMenu(hdocPopupMenuContainer, 0, MF_BYPOSITION);
    DestroyMenu(hdocPopupMenuContainer);
  }

  HMENU hpluginsPopupMenuContainer = LoadMenu(g_hResInstance, MAKEINTRESOURCE(IDM_PLUGINSPOPUP_MENU));
  if (hpluginsPopupMenuContainer)
  {
    hpluginsPopupMenu = GetSubMenu(hpluginsPopupMenuContainer, 0);
    RemoveMenu(hpluginsPopupMenuContainer, 0, MF_BYPOSITION);
    DestroyMenu(hpluginsPopupMenuContainer);
  }

  HMENU htabPopupMenuContainer = LoadMenu(g_hResInstance, MAKEINTRESOURCE(IDM_TABPOPUP_MENU));
  if (htabPopupMenuContainer)
  {
    htabPopupMenu = GetSubMenu(htabPopupMenuContainer, 0);
    RemoveMenu(htabPopupMenuContainer, 0, MF_BYPOSITION);
    DestroyMenu(htabPopupMenuContainer);
  }

  if (!hdocPopupMenu || !hpluginsPopupMenu || !htabPopupMenu)
  {
    CError::SetError(NOTEPAD_GET_SUBMENU_ERROR);
    return false;
  }

  return true;
}

// Function creates toolbar
bool CControlbar::CreateToolbar(HWND hwndOwner)
{
  if (hwnd)
    return true;

  HBITMAP hbmp = loadBitmap();
  if (!hbmp)
  {
    CError::SetError(NOTEPAD_LOAD_RESOURCE_ERROR);
    return false;
  }

  CNotepad &Notepad = CNotepad::Instance();

  if (Notepad.Options.IsToolbarTransparent())
  {
    // Paint transparent bits in the bitmap with toolbar color
    COLORREF toolbarColor = Notepad.Options.GetProfile().ToolbarColor;

    if (!transformBitmap(hbmp, toolbarColor))
    {
      DeleteObject(hbmp);
      return false;
    }
  }

  BITMAP bm;
  GetObject(hbmp, sizeof(BITMAP), (LPVOID)&bm);

#ifdef _WIN32_WCE
  if (!CreateMainMenu(hwndOwner))
    return false;

  HIMAGELIST himl = ImageList_Create(bm.bmHeight, bm.bmHeight, ILC_COLOR24, 16, 4);
  if (!himl)
  {
    DeleteObject(hbmp);
    return false;
  }
  ImageList_Add(himl, hbmp, 0);
  SendMessage(hwnd, TB_SETIMAGELIST, 0, (LPARAM)himl);
  DeleteObject(hbmp);

  TBBUTTONINFO tbbi;
  tbbi.cbSize = sizeof(tbbi);
  tbbi.dwMask = TBIF_IMAGE | TBIF_TEXT;
  tbbi.pszText = NULL;
  tbbi.cchText = 0;

  tbbi.iImage = s_btnInfo[0].id;
  SendMessage(hwnd, TB_SETBUTTONINFO, IDM_FILE,(LPARAM)&tbbi);
  tbbi.iImage = s_btnInfo[1].id;
  SendMessage(hwnd, TB_SETBUTTONINFO, IDM_MENU,(LPARAM)&tbbi);
#else        

  hwnd = CreateWindow(TOOLBARCLASSNAME, NULL, TOOLBAR_STYLE, 0, 0, 0, 0, hwndOwner, (HMENU)0, g_hResInstance, 0);
  if (!hwnd)
  {
    DeleteObject(hbmp);
    CError::SetError(NOTEPAD_CREATE_WINDOW_ERROR);
    return false;
  }

  SendMessage(hwnd, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
  SendMessage(hwnd, TB_SETBITMAPSIZE, 0, MAKELONG(bm.bmHeight, bm.bmHeight));

  // Set bitmap
  TBADDBITMAP tbbm;
  tbbm.hInst = 0;
  tbbm.nID = reinterpret_cast<UINT_PTR>(hbmp);
  SendMessage(hwnd, TB_ADDBITMAP, MAX_BUTTONS, (LPARAM)&tbbm);

  // Add two mandatory buttons
  //btnIndices.push_back(0);
  //btnIndices.push_back(1);
#endif

  TBBUTTON *buttons = new TBBUTTON[btnIndices.size()];
  if (!buttons)
  {
    DeleteObject(hbmp);
    DestroyToolbar();
    CError::SetError(NOTEPAD_MEMORY_ERROR);
    return false;
  }

  for (unsigned int i = 0; i < btnIndices.size(); i++)
  {
    buttons[i].iBitmap = s_btnInfo[btnIndices[i]].id;
    buttons[i].idCommand = s_btnInfo[btnIndices[i]].cmd;

    buttons[i].fsState = TBSTATE_ENABLED;
    if (buttons[i].idCommand == IDM_QINSERT_T && !hqinsertMenu)
      buttons[i].fsState = 0;

    buttons[i].fsStyle = (BYTE)(TBSTYLE_FLAT | TBSTYLE_AUTOSIZE | s_btnInfo[btnIndices[i]].style);
    buttons[i].dwData = 0;
    buttons[i].iString = 0;
  }
  SendMessage(hwnd, TB_ADDBUTTONS, btnIndices.size(), (LPARAM)buttons);
  delete[] buttons;

#ifndef _WIN32_WCE
  SendMessage(hwnd, TB_AUTOSIZE, 0, 0);

  RECT rc;
  GetClientRect(hwnd, &rc);
  x = y = 0;
  width = rc.right;
  height = rc.bottom;
#endif
  
  return true;
}

// Function destroys window menu or control bar if we are under mobile system
void CControlbar::DestroyMenuBar(HWND hwndOwner)
{
#ifdef _WIN32_WCE
  DestroyToolbar();

#else

  HMENU hmenu = GetMenu(hwndOwner);
  if (!hmenu)
    return;
  DestroyMenu(hmenu);
  SetMenu(hwndOwner, 0);

  if (hqinsertMenu)
  {
    DestroyMenu(hqinsertMenu);
    hqinsertMenu = 0;
  }
  if (hdocPopupMenu)
  {
    DestroyMenu(hdocPopupMenu);
    hdocPopupMenu = 0;
  }
  if (hpluginsPopupMenu)
  {
    DestroyMenu(hpluginsPopupMenu);
    hpluginsPopupMenu = 0;
  }
  if (htabPopupMenu)
  {
    DestroyMenu(htabPopupMenu);
    htabPopupMenu = 0;
  }
#endif
}

// Function destroys toolbar
void CControlbar::DestroyToolbar()
{
  if (!hwnd)
    return;

#ifdef _WIN32_WCE
  if (hqinsertMenu)
  {
    DestroyMenu(hqinsertMenu);
    hqinsertMenu = 0;
  }
  if (hdocPopupMenu)
  {
    DestroyMenu(hdocPopupMenu);
    hdocPopupMenu = 0;
  }
  if (hpluginsPopupMenu)
  {
    DestroyMenu(hpluginsPopupMenu);
    hpluginsPopupMenu = 0;
  }
  if (htabPopupMenu)
  {
    DestroyMenu(htabPopupMenu);
    htabPopupMenu = 0;
  }
#endif

  DestroyWindow(hwnd);
  hwnd = 0;
}

bool CControlbar::IsTBButtonChecked(int id)
{
  _ASSERTE(hwnd);

  bool buttonExists = false;
  for (size_t i = 0; i < btnIndices.size(); ++i)
  {
    if (s_btnInfo[btnIndices[i]].cmd == id)
    {
      buttonExists = true;
      break;
    }
  }
  if (!buttonExists)
    return false;

  TBBUTTONINFO tbbi;
  tbbi.cbSize = sizeof(TBBUTTONINFO);
  tbbi.dwMask = TBIF_STATE;
  SendMessage(hwnd, TB_GETBUTTONINFO, id, (LPARAM)&tbbi);
  return (tbbi.fsState & TBSTATE_CHECKED) ? true : false;
}

bool CControlbar::IsTBButtonEnabled(int id)
{
  _ASSERTE(hwnd);

  bool buttonExists = false;
  for (size_t i = 0; i < btnIndices.size(); ++i)
  {
    if (s_btnInfo[btnIndices[i]].cmd == id)
    {
      buttonExists = true;
      break;
    }
  }
  if (!buttonExists)
    return false;

  TBBUTTONINFO tbbi;
  tbbi.cbSize = sizeof(TBBUTTONINFO);
  tbbi.dwMask = TBIF_STATE;
  SendMessage(hwnd, TB_GETBUTTONINFO, id, (LPARAM)&tbbi);
  return (tbbi.fsState & TBSTATE_ENABLED) ? true : false;
}

void CControlbar::SetTBButtonState(int id, bool checked, bool enabled)
{
  _ASSERTE(hwnd);

  bool buttonExists = false;
  for (size_t i = 0; i < btnIndices.size(); ++i)
  {
    if (s_btnInfo[btnIndices[i]].cmd == id)
    {
      buttonExists = true;
      break;
    }
  }
  if (!buttonExists)
    return;

  TBBUTTONINFO tbbi;
  tbbi.cbSize = sizeof(TBBUTTONINFO);
  tbbi.dwMask = TBIF_STATE;
  tbbi.fsState = (checked ? TBSTATE_CHECKED : 0) | (enabled ? TBSTATE_ENABLED : 0);
  SendMessage(hwnd, TB_SETBUTTONINFO, id, (LPARAM)&tbbi);
}

int CControlbar::GetTBButtonX(int id)
{
  _ASSERTE(hwnd);

  bool buttonExists = false;
  int buttonIndice = 0;
  for (size_t i = 0; i < btnIndices.size(); ++i)
  {
    if (s_btnInfo[btnIndices[i]].cmd == id)
    {
      buttonExists = true;
      buttonIndice = i;
      break;
    }
  }
  if (!buttonExists)
    return -1;

  return LOWORD(SendMessage(hwnd, TB_GETBUTTONSIZE, 0, 0)) * (buttonIndice + 1);
}

void CControlbar::ShowPopupMenu(HWND hwnd, HMENU hmenu, int x, int y)
{
  if (!hmenu)
    return;

  POINT pt = {x, y};
  ClientToScreen(hwnd, &pt);

  LOG_ENTRY(L"Track popup menu\n");

  int retval = TrackPopupMenu(hmenu, TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, NULL); 

#ifdef SE_LOGGING
  if (!retval)
  {
    int error = GetLastError();
    std::wstring message;
    CError::GetSysErrorMessage(error, message);
    LOG_ENTRY(L"Track popup menu returns 0! Error (%d): %s", error, message.c_str());
  }
#endif
}

wchar_t* CControlbar::GetInsertString(unsigned int id, unsigned int *back, char *pasteSelection)
{
  return ::GetInsertString(id, back, pasteSelection);
}

void CControlbar::GetMenuPos(HWND hwnd, int pos, POINT &pt)
{
  RECT rc;
  if (pos >= 0)
  {
    switch (pos)
    {
    case MENUPOS_UPLEFT:
      pt.x = pt.y = 0;
      ClientToScreen(hwnd, (LPPOINT) &pt); 
      break;

    case MENUPOS_UPRIGHT:
      GetClientRect(hwnd, &rc);
      pt.x = rc.right;
      pt.y = 0;
      ClientToScreen(hwnd, (LPPOINT) &pt); 
      break;

    case MENUPOS_DOWNLEFT:
      GetClientRect(hwnd, &rc);
      pt.x = 0;
      pt.y = rc.bottom;
      ClientToScreen(hwnd, (LPPOINT) &pt); 
      break;

    case MENUPOS_DOWNRIGHT:
      GetClientRect(hwnd, &rc);
      pt.x = rc.right;
      pt.y = rc.bottom;
      ClientToScreen(hwnd, (LPPOINT) &pt); 
      break;

    case MENUPOS_SCRUPLEFT:
      pt.x = pt.y = 0;
      break;

    case MENUPOS_SCRUPRIGHT:
      pt.x = GetSystemMetrics(SM_CXSCREEN);
      pt.y = 0;
      break;

    case MENUPOS_SCRDOWNLEFT:
      pt.x = 0;
      pt.y = GetSystemMetrics(SM_CYSCREEN);
      break;

    case MENUPOS_SCRDOWNRIGHT:
      pt.x = GetSystemMetrics(SM_CXSCREEN);
      pt.y = GetSystemMetrics(SM_CYSCREEN);
      break;
    }
  }
  else
  {
    pos = abs(pos);
    pt.x = LOWORD(pos);
    pt.y = HIWORD(pos);
  }
}

// Function updates the buttons on a toolbar
void CControlbar::UpdateDocDependButtons()
{
  CDocuments &Documents = CNotepad::Instance().Documents;

  bool isDragMode = false;
  //bool canUndo = false;
  //bool canRedo = false;

  if (Documents.GetDocsNumber() > 0)
  {
    int current = Documents.Current();

    isDragMode = Documents.IsDragMode(current);
    //canUndo = Documents.CanUndo();
    //canRedo = Documents.CanRedo();
  }

  if (isDragMode != IsTBButtonChecked(CMD_DRAGMODE))
    SetTBButtonState(CMD_DRAGMODE, isDragMode, true);

  //if (canUndo != IsTBButtonEnabled(CMD_UNDO))
  //  SetTBButtonState(CMD_UNDO, false, canUndo);

  //if (canRedo != IsTBButtonEnabled(CMD_REDO))
  //  SetTBButtonState(CMD_REDO, false, canRedo);
}

HMENU CControlbar::GetDropDownMenu(int menuId)
{
  if (menuId >= 0 && menuId < DROPDOWN_MENUS_NUMBER)
  {
    int *sequence = s_menuPaths[menuId];

    int currentPosition;
    HMENU currentMenu = getTopMenuHandle(*sequence++);

    while (currentMenu && (currentPosition = *sequence++) > -1)
      currentMenu = GetSubMenu(currentMenu, currentPosition);

    return currentMenu;
  }

  return 0;
}

int CControlbar::DetermineMenu(HMENU hmenu)
{
  for (int menuId = 0; menuId < DROPDOWN_MENUS_NUMBER; menuId++)
  {
    if (GetDropDownMenu(menuId) == hmenu)
      return menuId;
  }

  return -1;
}

HMENU CControlbar::GetDocMenu()
{
  return GetDropDownMenu(MENU_DOCUMENTS);
}

HMENU CControlbar::GetRecentMenu()
{
  return GetDropDownMenu(MENU_RECENT);
}

HMENU CControlbar::GetEditMenu()
{
  return GetDropDownMenu(MENU_EDIT);
}

HMENU CControlbar::GetViewMenu()
{
  return GetDropDownMenu(MENU_VIEW);
}

HMENU CControlbar::GetReopenMenu()
{
  return GetDropDownMenu(MENU_REOPEN);
}

HMENU CControlbar::GetFileMenu()
{
  return GetDropDownMenu(MENU_FILE);
}

HMENU CControlbar::GetPluginsMenu()
{
  return GetDropDownMenu(MENU_PLUGINS);
}
