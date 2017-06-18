#ifndef __INTERFACE_
#define __INTERFACE_

#include <stdlib.h>

#include "tabs.hpp"
#include "frbar.hpp"
#include "dialogs.hpp"
#include "controlbar.hpp"
#include "statbar.hpp"
#include "ibase.hpp"


// Margins for windows
const int DOC_MARGIN = 0;


class CInterface : public CWindow
{
  bool isFullScreen;

public:
  int MaxProgress[MAX_DOCUMENTS]; // array for max progress values

  CTabs Tabs;
  CDialogs Dialogs;
  CControlbar Controlbar;
  CFindBar FindBar;
  CReplaceBar ReplaceBar;
  CStatusBar StatusBar;


public:

  void SetHWND(HWND hwnd) {this->hwnd = hwnd;}

  void Resize(int width, int height, bool repaint = true, bool validateDoc = false);
  int GetWidth()  { return width;  }
  int GetHeight() { return height; }
  void SwitchFullScreen(bool isFullScreen, bool resize = true);
  bool IsFullScreen() { return isFullScreen; }

  bool IdentifyLongTap(HWND hwnd, long int x, long int y);

  void CalcInterfaceCoords(SWindowPos &coords, bool responseOnKeyboard);
  void CalcDocCoords(SWindowPos &coords);
  void CalcTabCoords(SWindowPos &coords);
  void CalcToolbarCoords(SWindowPos &coords);
  void CalcFindBarCoords(SWindowPos &coords);
  void CalcReplaceBarCoords(SWindowPos &coords);
  void CalcStatusBarCoords(SWindowPos &coords);

  static void _cdecl SetMaxProgress(int id, int max);
  static void _cdecl SetProgress(int id, int progress);
  static void _cdecl ProgressEnd();

  CInterface();
};


#endif /* __INTERFACE_ */