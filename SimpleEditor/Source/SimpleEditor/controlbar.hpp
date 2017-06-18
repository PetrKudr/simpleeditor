#ifndef __MENU_
#define __MENU_

#include "ibase.hpp"

#include <windows.h>

#include <string>
#include <vector>


// Positions of popup menus
const int MENUPOS_UPLEFT       = 0;
const int MENUPOS_UPRIGHT      = 1;
const int MENUPOS_DOWNLEFT     = 2;
const int MENUPOS_DOWNRIGHT    = 3;
const int MENUPOS_SCRUPLEFT    = 4;
const int MENUPOS_SCRUPRIGHT   = 5;
const int MENUPOS_SCRDOWNLEFT  = 6;
const int MENUPOS_SCRDOWNRIGHT = 7;


enum DropDownMenu 
{
  MENU_FILE = 0,
  MENU_MENU,
  MENU_REOPEN,
  MENU_RECENT,
  MENU_EDIT,
  MENU_DOCUMENTS,
  MENU_PLUGINS,
  MENU_VIEW,

  DROPDOWN_MENUS_NUMBER
};


class CControlbar : public CWindow
{
  HMENU hqinsertMenu,
        hdocPopupMenu, 
        hpluginsPopupMenu,
        htabPopupMenu;

  HWND myHwndOwner;

  int lastQInsertID;
  std::vector<int> btnIndices;  // list of buttons on a toolbar

  // for protection
  //void initDocMenu(HMENU hmenu);
  //bool constructFileMenu(HMENU hmenu);
  //bool constructMenuMenu(HMENU hmenu);

  bool transformBitmap(HBITMAP hBitmap, COLORREF bgColor);
  HBITMAP loadBitmap();
  HMENU getTopMenuHandle(int position);

public:
  void ReadButtons();
  bool CreateMainMenu(HWND hwndOwner);
  bool CreateToolbar(HWND hwndOwner);
  void DestroyMenuBar(HWND hwndOwner);
  void DestroyToolbar();

  HMENU GetDocPopupMenu()       { return hdocPopupMenu;      }
  HMENU GetPluginsPopupMenu()   { return hpluginsPopupMenu;  }
  HMENU GetTabPopupMenu()       { return htabPopupMenu;      }

  //HMENU GetDocMenu()            { return hdocMenu;           }
  //HMENU GetRecentMenu()         { return hrecentMenu;        }
  //HMENU GetEditMenu()           { return heditMenu;          }
  //HMENU GetViewMenu()           { return hviewMenu;          }
  //HMENU GetReopenMenu()         { return hreopenMenu;        }
  //HMENU GetFileMenu()           { return hfileMenu;          }
  //HMENU GetPluginsMenu()        { return hpluginsMenu;       }

  HMENU GetDropDownMenu(int menuId);
  int DetermineMenu(HMENU hmenu);

  HMENU GetDocMenu();
  HMENU GetRecentMenu();
  HMENU GetEditMenu();
  HMENU GetViewMenu();
  HMENU GetReopenMenu();
  HMENU GetFileMenu();
  HMENU GetPluginsMenu();

  HMENU GetQInsertMenu() { return hqinsertMenu;  }
  int GetLastQInsertID() { return lastQInsertID; }
  wchar_t* GetInsertString(unsigned int id, unsigned int *back, char *pasteSelection);

  void ShowPopupMenu(HWND hwnd, HMENU hmenu, int x, int y);

  void GetMenuPos(HWND hwnd, int pos, POINT &pt);
  bool IsQInsertMenu() { return hqinsertMenu ? true : false; }
  bool IsToolbar()     { return hwnd ? true : false; }
  bool IsTBButtonChecked(int id);
  bool IsTBButtonEnabled(int id);
  void SetTBButtonState(int id, bool checked, bool enabled);
  void UpdateDocDependButtons();
  int GetTBButtonX(int id);

  CControlbar();
};


#endif /* __MENU_ */