#include "resources\res.hpp"
#include "notepad.hpp"

const int FRBAR_BTN_MARGIN = 2;
const int MAX_FR_BUFF = 512;


// variables for the find and replace dialogs
static wchar_t findBuff[MAX_FR_BUFF], replaceBuff[MAX_FR_BUFF];
static FINDREPLACE fr;
static bool matchCase = false;
static bool wholeWord = false;


CFindBar::CFindBar() : myRepeatType(NO_REPEAT), leftNotFoundTimes(0), rightNotFoundTimes(0), hNotFoundBgBrush(0)
{
  findBuff[0] = L'\0';
}

CFindBar::~CFindBar()
{
  if (hNotFoundBgBrush)
    DeleteObject(hNotFoundBgBrush);
}


bool CFindBar::Create(HWND hwndOwner, bool resize)
{
  _ASSERTE(!hwnd);

  CNotepad &Notepad = CNotepad::Instance();

  hNotFoundBgBrush = CreateSolidBrush(Notepad.Options.GetProfile().NotFoundEditControlColor);

  wchar_t *buff = Notepad.Documents.GetSelection();
  if (buff && wcslen(buff) > MAX_FR_BUFF - 1)
  {
    delete[] buff;
    buff = 0;
  }

  LPARAM lParam = (LPARAM)findBuff;
  if (buff)
    lParam = (LPARAM)buff;

  hwnd = CreateDialogParam(g_hResInstance, MAKEINTRESOURCE(DLG_FINDBAR), hwndOwner, 
                           reinterpret_cast<DLGPROC>(findBarProc), lParam);

  if (!hwnd)
  {
    //std::wstring errmsg;
    //Notepad.Error.GetSysErrorMessage(GetLastError(), errmsg);
    //Notepad.Interface.Dialogs.ShowError(Notepad.Interface.GetHWND(), errmsg);
    CError::SetError(NOTEPAD_CREATE_WINDOW_ERROR);
    return false;
  }
   
  if (buff)
    delete[] buff;

  /*HDC hdc = CreateDC(L"DISPLAY", 0, 0, 0);
  SelectObject(hdc, (HFONT)GetStockObject(SYSTEM_FONT));
  TEXTMETRIC tm;
  GetTextMetrics(hdc, &tm);
  DeleteDC(hdc);

  x = y = 0;
  width = Notepad.Interface.GetWidth();
  height = static_cast<int>(tm.tmHeight * 1.5);*/

  RECT rc;
  GetClientRect(hwnd, &rc);
  x = y = 0;
  width = Notepad.Interface.GetWidth();
  height = rc.bottom;

  if (resize)
    Notepad.Interface.Resize(Notepad.Interface.GetWidth(), Notepad.Interface.GetHeight(), false, true);

  if (Notepad.Interface.Controlbar.IsToolbar() && !Notepad.Interface.Controlbar.IsTBButtonChecked(IDM_FIND))
    Notepad.Interface.Controlbar.SetTBButtonState(IDM_FIND, true, true);

#ifdef UNDER_CE
  if (Notepad.Options.IsSipOnFindBar())
    SipShowIM(SIPF_ON);
#endif

  return true;
}

void CFindBar::Destroy(bool resize)
{
  if (hwnd)
    DestroyWindow(hwnd);
  hwnd = 0;

  if (hNotFoundBgBrush)
  {
    DeleteObject(hNotFoundBgBrush);
    hNotFoundBgBrush = 0;
  }

  CNotepad &Notepad = CNotepad::Instance();

  if (resize)
    Notepad.Interface.Resize(Notepad.Interface.GetWidth(), Notepad.Interface.GetHeight(), true, true);

  if (Notepad.Interface.Controlbar.IsToolbar() && Notepad.Interface.Controlbar.IsTBButtonChecked(IDM_FIND))
    Notepad.Interface.Controlbar.SetTBButtonState(IDM_FIND, false, true);

  Notepad.Documents.SetFocus();

#ifdef UNDER_CE
  if (Notepad.Options.IsSipOnFindBar())
    SipShowIM(SIPF_OFF);
#endif
}

// Window function for the find bar
UINT_PTR CALLBACK CFindBar::findBarProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
  static bool editControlInitialized = false;
  static wchar_t *buff = 0;

  switch (msg)
  {
  case WM_INITDIALOG:
    {
      editControlInitialized = false;
      buff = (wchar_t*)lParam;

      CNotepad &Notepad = CNotepad::Instance();

      memset(&fr, 0, sizeof(FINDREPLACE));
      if (matchCase)
        SendDlgItemMessage(hdlg, IDD_BTN_2, BM_SETCHECK, TRUE, 0);
      if (wholeWord)
        SendDlgItemMessage(hdlg, IDD_BTN_3, BM_SETCHECK, TRUE, 0);
      if (Notepad.Options.UseRegexp())
        SendDlgItemMessage(hdlg, IDD_BTN_4, BM_SETCHECK, TRUE, 0);
      if (buff)
        SetWindowText(GetDlgItem(hdlg, IDD_EDIT_0), buff);
    }
    return TRUE;

  case WM_SIZE:
    {
      int width = LOWORD(lParam);
      int height = HIWORD(lParam);
      int x, y;
    
      RECT rc, dlgRC;
      HWND hitem = GetDlgItem(hdlg, IDD_BTN_0);
      GetClientRect(hitem, &rc);
      GetClientRect(hdlg, &dlgRC);
      x = 0;
      y = (dlgRC.bottom - rc.bottom) / 2;
      MoveWindow(hitem, x, y, rc.right, rc.bottom, TRUE);

      hitem = GetDlgItem(hdlg, IDD_BTN_4);
      GetClientRect(hitem, &rc);
      x = width - rc.right;
      MoveWindow(hitem, x, y, rc.right, rc.bottom, TRUE);

      hitem = GetDlgItem(hdlg, IDD_BTN_3);
      GetClientRect(hitem, &rc);
      x -= rc.right + FRBAR_BTN_MARGIN;
      MoveWindow(hitem, x, y, rc.right, rc.bottom, TRUE);

      hitem = GetDlgItem(hdlg, IDD_BTN_2);
      GetClientRect(hitem, &rc);
      x -= rc.right + FRBAR_BTN_MARGIN;
      MoveWindow(hitem, x, y, rc.right, rc.bottom, TRUE);

      hitem = GetDlgItem(hdlg, IDD_BTN_1);
      GetClientRect(hitem, &rc);
      x -= rc.right + FRBAR_BTN_MARGIN;
      MoveWindow(hitem, x, y, rc.right, rc.bottom, TRUE);

      hitem = GetDlgItem(hdlg, IDD_EDIT_0);
      MoveWindow(hitem, rc.right + FRBAR_BTN_MARGIN, y, x - rc.right - 2*FRBAR_BTN_MARGIN, rc.bottom, TRUE);
    }
    return TRUE;

  case WM_COMMAND:
    {
      CNotepad &Notepad = CNotepad::Instance();

      switch (LOWORD(wParam))
      {
      /*case IDD_EDIT_0:
        if (!editControlInitialized && HIWORD(wParam) == EN_SETFOCUS)
        {
          if (buff)
          {
            SetWindowText((HWND)lParam, buff);
            // sdasd ??/
            SendMessage((HWND)lParam, EM_SETSEL, 0, -1);
          }
          editControlInitialized = true;
          return TRUE;
        }
        break;*/


      case IDD_BTN_0:  // find back
      case IDD_BTN_1:  // find forward
      { 
        CFindBar &FindBar = CNotepad::Instance().Interface.FindBar;

        bool isFindBack = (LOWORD(wParam) == IDD_BTN_0);

        if (Notepad.Documents.GetDocsNumber() == 0 || Notepad.Documents.IsBusy(Notepad.Documents.Current()))
          return TRUE;

        if (GetWindowTextLength(GetDlgItem(hdlg, IDD_EDIT_0)) == 0)
          return TRUE;

        GetDlgItemText(hdlg, IDD_EDIT_0, findBuff, MAX_FR_BUFF);
        fr.lStructSize = sizeof(FINDREPLACE);
        fr.lpstrFindWhat = findBuff;
        fr.wFindWhatLen = wcslen(findBuff);
        fr.Flags = (FR_FINDNEXT | (isFindBack ? 0 : FR_DOWN));
        if (matchCase)
          fr.Flags |= FR_MATCHCASE;
        if (wholeWord)
          fr.Flags |= FR_WHOLEWORD;

        if (!Notepad.Documents.FindReplace(&fr, ((FindBar.rightNotFoundTimes > 0 && !isFindBack) || (FindBar.leftNotFoundTimes > 0 && isFindBack))))
        {
          if (CError::HasError())
          {
            Notepad.Interface.Dialogs.ShowError(Notepad.Interface.GetHWND(), CError::GetError().message);
            CError::SetError(NOTEPAD_NO_ERROR);
          }
          else 
          {
            if (isFindBack)
              FindBar.leftNotFoundTimes++;
            else
              FindBar.rightNotFoundTimes++;
          }
        }
        else
        {
          FindBar.leftNotFoundTimes = 0;
          FindBar.rightNotFoundTimes = 0;
        }

        RedrawWindow(GetDlgItem(hdlg, IDD_EDIT_0), 0, 0, RDW_INVALIDATE | RDW_ERASE);

        Notepad.Documents.SetFocus(); // restore focus  
        return TRUE;
      }

      case IDD_BTN_2: // match case
        matchCase = !matchCase;
        Notepad.Documents.SetFocus(); // restore focus  
        return TRUE;

      case IDD_BTN_3: // whole word
        wholeWord = !wholeWord;
        Notepad.Documents.SetFocus(); // restore focus  
        return TRUE;

      case IDD_BTN_4:  // regexp
        Notepad.Options.SetRegexp(!Notepad.Options.UseRegexp());
        Notepad.Documents.SetFocus(); // restore focus  
        return TRUE;
      }
    }

  case WM_CTLCOLOREDIT:
    {
      CFindBar &FindBar = CNotepad::Instance().Interface.FindBar;

      if (FindBar.leftNotFoundTimes || FindBar.rightNotFoundTimes)
      {
        //LOG_ENTRY(L"Received WM_CTLCOLOREDIT. Brush handle = 0x%.8x\n", FindBar.hNotFoundBgBrush);
        SetBkColor((HDC)wParam, CNotepad::Instance().Options.GetProfile().NotFoundEditControlColor);
        return (UINT_PTR)FindBar.hNotFoundBgBrush;
      }

      break;
    }

  }

  return FALSE;
}


CReplaceBar::CReplaceBar()
{
  replaceBuff[0] = L'\0';
}

bool CReplaceBar::Create(HWND hwndOwner, bool resize)
{
  _ASSERTE(!hwnd);

  CNotepad &Notepad = CNotepad::Instance();

  if (!Notepad.Interface.FindBar.IsFindBar())
  {
    if (!Notepad.Interface.FindBar.Create(hwndOwner, false))
      return false;
  }

  hwnd = CreateDialogParam(g_hResInstance, MAKEINTRESOURCE(DLG_REPLACEBAR), hwndOwner, 
                           reinterpret_cast<DLGPROC>(replaceBarProc), (LPARAM)replaceBuff);

  if (!hwnd)
  {
    CError::SetError(NOTEPAD_CREATE_WINDOW_ERROR);
    return false;
  }

  RECT rc;
  GetClientRect(hwnd, &rc);
  x = y = 0;
  width = Notepad.Interface.GetWidth();
  height = rc.bottom;

  if (resize)
    Notepad.Interface.Resize(Notepad.Interface.GetWidth(), Notepad.Interface.GetHeight(), false, true);

  if (Notepad.Interface.Controlbar.IsToolbar())
  {
    if (Notepad.Interface.Controlbar.IsTBButtonChecked(IDM_FIND))
      Notepad.Interface.Controlbar.SetTBButtonState(IDM_FIND, false, true);

    if (!Notepad.Interface.Controlbar.IsTBButtonChecked(IDM_REPLACE))
      Notepad.Interface.Controlbar.SetTBButtonState(IDM_REPLACE, true, true);
  }

  return true;
}

void CReplaceBar::Destroy(bool resize)
{
  if (hwnd)
    DestroyWindow(hwnd);
  hwnd = 0;

  CNotepad &Notepad = CNotepad::Instance();
  if (resize)
    Notepad.Interface.Resize(Notepad.Interface.GetWidth(), Notepad.Interface.GetHeight(), true, true);

  if (Notepad.Interface.Controlbar.IsToolbar())
  {
    if (Notepad.Interface.Controlbar.IsTBButtonChecked(IDM_REPLACE))
      Notepad.Interface.Controlbar.SetTBButtonState(IDM_REPLACE, false, true);
  }
  Notepad.Documents.SetFocus();
}

// Window function for the replace bar
UINT_PTR CALLBACK CReplaceBar::replaceBarProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
  case WM_INITDIALOG:
    if (lParam)
      SetWindowText(GetDlgItem(hdlg, IDD_EDIT_0), (wchar_t*)lParam);
    return TRUE;

  case WM_SIZE:
    {
      int width = LOWORD(lParam);
      int height = HIWORD(lParam);
      int x, y;
    
      RECT rc, dlgRC;

      HWND hitem = GetDlgItem(hdlg, IDD_BTN_0);
      GetClientRect(hitem, &rc);
      GetClientRect(hdlg, &dlgRC);
      x = 0;
      y = (dlgRC.bottom - rc.bottom) / 2;
      MoveWindow(hitem, x, y, rc.right, rc.bottom, TRUE);

      hitem = GetDlgItem(hdlg, IDD_BTN_2);
      GetClientRect(hitem, &rc);
      x = width - rc.right;
      MoveWindow(hitem, x, y, rc.right, rc.bottom, TRUE);

      hitem = GetDlgItem(hdlg, IDD_BTN_1);
      GetClientRect(hitem, &rc);
      x -= rc.right + FRBAR_BTN_MARGIN;
      MoveWindow(hitem, x, y, rc.right, rc.bottom, TRUE);

      hitem = GetDlgItem(hdlg, IDD_EDIT_0);
      MoveWindow(hitem, FRBAR_BTN_MARGIN + rc.right, y, x - rc.right - 2*FRBAR_BTN_MARGIN, rc.bottom, TRUE);
    }
    return TRUE;

  case WM_COMMAND:
    {
      CNotepad &Notepad = CNotepad::Instance();
      HWND hfindDlg = Notepad.Interface.FindBar.GetHWND();

      switch (LOWORD(wParam))
      {
      case IDD_BTN_0:  // replace back
      case IDD_BTN_1:  // replace forward
      case IDD_BTN_2:  // replace all
        { 
          CFindBar &FindBar = CNotepad::Instance().Interface.FindBar;

          bool isReplaceBack = LOWORD(wParam) == IDD_BTN_0;
          bool isReplaceForward = LOWORD(wParam) == IDD_BTN_1;
          bool isReplaceAll = LOWORD(wParam) == IDD_BTN_2;

          if (Notepad.Documents.GetDocsNumber() == 0 || Notepad.Documents.IsBusy(Notepad.Documents.Current()))
            return TRUE;

          fr.lStructSize = sizeof(FINDREPLACE);

          GetDlgItemText(hfindDlg, IDD_EDIT_0, findBuff, MAX_FR_BUFF);
          fr.lpstrFindWhat = findBuff;
          fr.wFindWhatLen = wcslen(findBuff);
          
          GetDlgItemText(hdlg, IDD_EDIT_0, replaceBuff, MAX_FR_BUFF);
          fr.lpstrReplaceWith = replaceBuff;
          fr.wReplaceWithLen = wcslen(replaceBuff);
          fr.Flags = isReplaceAll ? FR_REPLACEALL : FR_REPLACE;
          if (matchCase)
            fr.Flags |= FR_MATCHCASE;
          if (wholeWord)
            fr.Flags |= FR_WHOLEWORD;

          if (isReplaceBack || isReplaceForward) // replace
          {
            if (isReplaceForward)
              fr.Flags |= FR_DOWN;

            if (!Notepad.Documents.FindReplace(&fr, ((FindBar.rightNotFoundTimes > 0 && isReplaceForward) || (FindBar.leftNotFoundTimes > 0 && isReplaceBack))))
            {
              if (CError::HasError())
              {
                Notepad.Interface.Dialogs.ShowError(Notepad.Interface.GetHWND(), CError::GetError().message);
                CError::SetError(NOTEPAD_NO_ERROR);
              }
              else 
              {
                if (isReplaceBack)
                  FindBar.leftNotFoundTimes++;
                else
                  FindBar.rightNotFoundTimes++;

                // todo: change color and find from beginning/ending
              }
            }
            else
            {
              FindBar.leftNotFoundTimes = 0;
              FindBar.rightNotFoundTimes = 0;
            }

            // Redraw find window (if nothing was not found, we need to redraw edit box)
            RedrawWindow(FindBar.GetHWND(), 0, 0, RDW_INVALIDATE | RDW_ERASE);
          }
          else  // replace all
          {
            if (!Notepad.Documents.ReplaceAll(&fr))
            {
              Notepad.Interface.Dialogs.ShowError(Notepad.Interface.GetHWND(), 
                                                  CError::GetError().message);
              CError::SetError(NOTEPAD_NO_ERROR);
            }
          }

          return TRUE;
        }
      }
    }
  }

  return FALSE;
}
