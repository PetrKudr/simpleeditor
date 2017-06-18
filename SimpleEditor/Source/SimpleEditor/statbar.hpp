#ifndef __STATBAR_
#define __STATBAR_

#include "ibase.hpp"

class CStatusBar : public CWindow
{
  HWND hlineStat, hlineWnd, hindStat, hindWnd; 
  HFONT hfont;
  int lineStrWidth, indStrWidth; // widths of the line string and index string
  int aveCharWidth;     // average width of a char
  int oldX, oldY;    // old coordinates of a caret

public:
  bool Create(HWND hwndOwner);
  void Destroy();
  bool IsStatusBar()  { return hwnd ? true : false; }

  void SetFont(HFONT hfont);
  void SetPosition(unsigned int x, unsigned int y);
  void Move(int x, int y, int width, int height);

  CStatusBar()
  {
    hlineStat = hindStat = hlineWnd = hindWnd = 0;
    hfont = 0;
    lineStrWidth = indStrWidth = aveCharWidth = 0;
    oldX = oldY = -1;
  }
};

#endif /* __STATBAR_ */