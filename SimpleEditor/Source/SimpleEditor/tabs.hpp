#ifndef __TABS_
#define __TABS_

#include <vector>
#include <windows.h>

#include "documents.hpp"
#include "ibase.hpp"


#define TABS_WINDOW_STYLE  (WS_CHILD)
#define TABS_TAB_STYLE     (WS_CHILD | BS_OWNERDRAW)
#define TABS_BUTTON_STYLE  (WS_CHILD | BS_PUSHBUTTON)

// Possible tab types
#define TAB_NORMAL    1
#define TAB_MODIFIED  2
#define TAB_PREACTIVE 4
#define TAB_ACTIVE    8
#define TAB_BUSY      16

// Margins
#define TAB_MARGINLR  5
#define BUTTON_MARGINLR 10

// Button strings
const wchar_t LeftCaption[] = L"<";
const wchar_t RightCaption[] = L">";

// Button indentifiers
const int TABS_NO_BUTTON    = 0;
const int TABS_LEFT_BUTTON  = 1;
const int TABS_RIGHT_BUTTON = 2;

// Width in pixels of the progress bar area
const int TABS_PROGRESS_BAR_WIDTH = 3;


// Information about one tab
struct STabData
{
  HWND hwnd;
  bool visible;
  char type;
  int width;

  int strX;
  int strY;
  int progress;  // maxProgress = width
};


// for SetColors function
const int SET_NORMCOLOR        = 1;
const int SET_MODCOLOR         = 2;
const int SET_ACTNORMCOLOR     = 4;
const int SET_ACTMODCOLOR      = 8;
const int SET_FRAMECOLOR       = 16;
const int SET_ACTFRAMECOLOR    = 32;
const int SET_TEXTCOLOR        = 64;
const int SET_ACTTEXTCOLOR     = 128;
const int SET_BUSYCOLOR        = 256;
const int SET_ACTBUSYCOLOR     = 512;
const int SET_PROGRESSBARCOLOR = 1024;
const int SET_BGCOLOR          = 2048;
const int SET_ALLCOLORS     = (1 | 2 | 4 | 8 | 16 | 32 | 64 | 128 | 256 | 512 | 1024 | 2048);

// Colors
struct CTabColors
{
  COLORREF normColor;
  COLORREF modColor;
  COLORREF activeNormColor;
  COLORREF activeModColor;
  COLORREF busyColor;
  COLORREF activeBusyColor;
  COLORREF frameColor;
  COLORREF activeFrameColor;
  COLORREF textColor;
  COLORREF activeTextColor;
  COLORREF progressBarColor;
  COLORREF BGColor;
};

class CTabs : public CWindow
{
  HFONT hfont;
  HWND hwndLeft, hwndRight;
  CTabColors colors;
  STabData tabs[MAX_DOCUMENTS];
  int leftButtonWidth, rightButtonWidth;
  int totalWidth; // total width of all tabs
  int maxWidth; // max tab width
  int first;   // index of the leftmost tab
  int active;  // index of an active tab
  int number;  // number of tabs
  bool buttons; // if buttons left/right are visible
  bool visible; // if tab widnow is visible
  bool forcedVisibility; // if tabs window is visible, when number < MinTabsForVisible


  void getButtonsPos(SWindowPos &left, SWindowPos &right);
  bool getTabPos(int index, SWindowPos &pos);
  void repaintTab(int index, RECT *prc);
  void resetTabVisualData(STabData &tab, const wchar_t *wstr, size_t len);
  static LRESULT CALLBACK tabControlWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
  static LRESULT CALLBACK tabWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

public:
  bool Available();
  bool Visible();

  bool Init();
  bool Create();
  bool SetActive(int index);
  bool SetPreActive(int index, bool isPreActive);
  bool SetModified(int index, bool isModified);
  bool SetBusy(int index, bool isBusy);
  bool SetProgress(int index, int progress);
  bool MoveViewPoint(int count);
  bool MoveViewPointToTab(int index);
  bool Add(const std::wstring &name);
  bool Delete(int index);
  void Move(int x, int y, int width, int height);
  void Repaint();
  void SetForcedVisibility(bool forcedVisibility);
  void Destroy();

  int GetButtonFromWindow(HWND hwnd);
  int GetIndiceFromWindow(HWND hwndTab);
  STabData& GetTabData(int index); 
  COLORREF GetTabColor(int index);
  COLORREF GetFrameColor(int index);
  COLORREF GetTextColor(int index);
  COLORREF GetProgressBarColor(int index);
  COLORREF GetBGColor();
  const CTabColors& GetColors();
  HFONT GetFont();
  void DocNameChanged(int index);

  void SetColors(const CTabColors &colors, unsigned int flags);
  void SetFont(HFONT hfont);
  CTabs();
};

#endif /* __TABS_ */