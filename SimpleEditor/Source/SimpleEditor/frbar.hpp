#ifndef __FIND_REPLACE_
#define __FIND_REPLACE_

#include "ibase.hpp"
#include <stdlib.h>


class CFindBar : public CWindow
{
  friend class CReplaceBar;

public:

  enum RepeatType {
    NO_REPEAT,
    FROM_BEGINNING,
    FROM_ENDING
  };


private:

  RepeatType myRepeatType;

  int leftNotFoundTimes; // text not found for the last leftNotFoundTimes requests

  int rightNotFoundTimes; // text not found for the last rightNotFoundTimes requests

  HBRUSH hNotFoundBgBrush;


private:

  static UINT_PTR CALLBACK findBarProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);


public:

  bool Create(HWND hwndOwner, bool resize);
  void Destroy(bool resize);
  //void SetRepeatType(RepeatType repeatType);

  bool IsFindBar() { return hwnd ? true : false; }

  CFindBar();
  ~CFindBar();
};

class CReplaceBar : public CWindow
{
  static UINT_PTR CALLBACK replaceBarProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);

public:
  bool Create(HWND hwndOwner, bool resize);
  void Destroy(bool resize);  

  bool IsReplaceBar() { return hwnd ? true : false; }

  CReplaceBar();
};

#endif /* __FIND_REPLACE_ */