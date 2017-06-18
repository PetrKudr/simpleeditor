#ifndef __INTERFACE_BASE_
#define __INTERFACE_BASE_

#include "..\stdgui.h"
#include "base\variables.h"

#include <string>

#ifndef PATH_SIZE
#define PATH_SIZE 512
#endif

struct SWindowPos
{
  int x;
  int y;
  int width;
  int height;
};


/** Window class **/
class CWindow : protected SWindowPos
{
protected:
  HWND hwnd;

public:
  CWindow() {hwnd = 0; x = y = width = height = 0;}
  ~CWindow() {}

  HWND GetHWND() { return hwnd; }

  // Function coordinates size of the window
  void GetPos(SWindowPos &pos) 
  { 
    pos.x = this->x;
    pos.y = this->y;
    pos.width = this->width;
    pos.height = this->height;
  }

  // Function moves window
  void Move(int x, int y, int width, int height)
  {
    _ASSERTE(hwnd);
    this->x = x;
    this->y = y;
    this->width = width;
    this->height = height;
    MoveWindow(hwnd, x, y, width, height, TRUE);
  }
};


namespace Location
{
  extern int MaxX, MaxY;

  void SetNet(HWND hwnd, int x, int y, int marginX, int marginY, bool fixedX, bool fixedY);
  void Arrange(const HWND *windows, const SWindowPos *positions, size_t number);
};


/** Useful functions **/

// Function returns an index of the file name
extern int GetFileNameIndice(const wchar_t *path);

#ifdef _WIN32_WCE
extern bool CreateOkCancelBar(HWND hwnd);
extern void DestroyOkCancelBar();
#endif


/** Global variables **/

extern int g_LogPixelsY;


#endif /* __INTERFACE_BASE_ */