#include "resources\res.hpp"
#include "notepad.hpp"

#include <ctype.h>


#ifndef min
#define min(a,b) ((a) > (b) ? (b) : (a))
#endif

#if defined(_WIN32_WCE)
#include <aygshell.h>
#endif

#pragma warning(disable: 4996)


#define MAX_TEXT_MARGIN_Y 24


// structures for passing information in dialog procedures
struct SMsgBoxData
{
  const wchar_t *caption;
  const wchar_t *message;
  HFONT hfont;

  SMsgBoxData(HFONT hfont, const wchar_t *caption, const wchar_t *message)
  {
    this->caption = caption;
    this->message = message;
    this->hfont = hfont;
  }
};

struct SGetExtData
{
  LPCWSTR cp;   // code page
  bool BOM; // need BOM?

  bool forOpen;
};

struct SGotoLineData
{
  unsigned int maxNumber;
  unsigned int current;
};

struct SFileInfoData
{
  std::wstring path;
  size_t size;
};



int CDialogs::ShowError(HWND hwnd, const std::wstring &errmsg)
{
  return MsgBox(hwnd, errmsg.c_str(), ERROR_CAPTION, MB_OK);
}

void CDialogs::AboutDialog(HWND hwnd)
{
  HWND hfocus = GetFocus();

  // Note that here we are using application instance. (About dialog must be "hardcoded")
  DialogBox(g_hAppInstance, MAKEINTRESOURCE(DLG_ABOUT), hwnd, 
            reinterpret_cast<DLGPROC>(AboutDlgProc));
  SetFocus(hfocus);
}

void CDialogs::FileInfoDialog(HWND hwnd, const std::wstring &path, int size)
{
  HWND hfocus = GetFocus();

  SFileInfoData data;
  data.path.assign(path);
  data.size = size;

  DialogBoxParam(
    g_hResInstance,
    MAKEINTRESOURCE(DLG_FILEINFO), 
    hwnd, 
    reinterpret_cast<DLGPROC>(FileInfoDialogProc),
    reinterpret_cast<LPARAM>(&data)
  );

  SetFocus(hfocus);
}

void CDialogs::GotoLineDialog(HWND hwnd, CDocuments &Documents, bool realLines)
{
  if (Documents.GetDocsNumber() == 0)
    return;

  if (Documents.IsBusy(Documents.Current()))
  {
    std::wstring message;
    CError::GetErrorMessage(message, NOTEPAD_DOCUMENT_IS_BUSY);
    CDialogs::ShowError(hwnd, message);
    return;
  }

  HWND hfocus = GetFocus();

  SGotoLineData data;

  data.maxNumber = Documents.GetDocLinesNumber(Documents.Current(), realLines);
  _ASSERTE(data.maxNumber != UINT_MAX);

  unsigned int x, y;
  Documents.GetCaretPos(Documents.Current(), x, y, realLines);
  data.current = y + 1;

  int retval = DialogBoxParam(g_hResInstance, MAKEINTRESOURCE(DLG_GOTOLINE), hwnd, 
                              reinterpret_cast<DLGPROC>(GotoLineDlgProc), 
                              reinterpret_cast<LPARAM>(&data));
  if (retval)
  {
    // not neccessary (only data.current-1 is neccessary, 
    // because data.current is in range [1, maxLines], but it should be within [0, maxLines-1])
    data.current = min(data.current - 1, data.maxNumber - 1); 

    Documents.SetCaretPos(Documents.Current(), 0, data.current, realLines, true);
  }
  SetFocus(hfocus);
}

int CDialogs::MsgBox(HWND hwnd, const wchar_t *message, const wchar_t *caption, int type)
{
  SMsgBoxData data((HFONT)GetStockObject(SYSTEM_FONT), caption, message);
  HWND hwndFocus = GetFocus();
  int retval = -1;

  switch (type)
  {
  case MB_OK:
    retval = DialogBoxParam(g_hResInstance, MAKEINTRESOURCE(DLG_MSGBOX_OK), hwnd, 
                            reinterpret_cast<DLGPROC>(MsgOKDlgProc),
                            reinterpret_cast<LPARAM>(&data));
    break;

  case MB_YESNO:
    retval = DialogBoxParam(g_hResInstance, MAKEINTRESOURCE(DLG_MSGBOX_YESNO), hwnd, 
                            reinterpret_cast<DLGPROC>(MsgYesNoDlgProc),
                            reinterpret_cast<LPARAM>(&data));
    break;

  case MB_YESNOCANCEL:
    retval = DialogBoxParam(g_hResInstance, MAKEINTRESOURCE(DLG_MSGBOX_YESNOCANCEL), hwnd, 
                            reinterpret_cast<DLGPROC>(MsgYesNoCancelDlgProc),
                            reinterpret_cast<LPARAM>(&data));
    break;
  }

  if (hwndFocus)
    SetFocus(hwndFocus);

  _ASSERTE(retval != -1);
  return retval;
}

void CDialogs::addExtention(std::wstring &path, int iFilter)
{
  if (filter[0] == 0)
    return;

  iFilter <<= 1;
  ++iFilter;
  int i = 0;

  while (iFilter > 0)
  {
    if (0 == filter[i] && 0 == filter[i + 1])
      break;
    else if (0 == filter[i])
      --iFilter;
    ++i;
  }
  if (iFilter)
    return; // error

  wchar_t *point;
  if ((point = wcschr(&filter[i], L'.')) == 0)
    return; // filter does not contain extention

  wchar_t *end = point;
  while (*end++)
    if (*end && !iswalnum(*end))
      return;
  --end;

  size_t extLen = end - point;
  if (extLen <= 1)
    return;  // extention contains only point
  
  size_t len = wcslen(path.c_str());
  const wchar_t *tmp = &(path.c_str()[len - extLen]);
  if (len < extLen || wcscmp(&(path.c_str()[len - extLen]), point) != 0) 
    path += point;
}

bool CDialogs::OpenFileDialog(HWND hwnd, std::wstring &path, LPCWSTR &cp, bool ownDialog)
{
  if (ownDialog)
    return OwnOpenFileDialog(hwnd, path, cp);
  return StdOpenFileDialog(hwnd, path, cp);
}

bool CDialogs::SaveFileDialog(HWND hwnd, const std::wstring &name, std::wstring &path, LPCWSTR &cp, bool &BOM, bool ownDialog)
{
  // update document name (if it is modified we have to remove DOC_MODIFIED_STR from the name)
  std::wstring prepared = name;
  if (prepared.length() > DMS_LEN && prepared.compare(prepared.length() - DMS_LEN, DMS_LEN, DOC_MODIFIED_STR) == 0)
    prepared.erase(prepared.length() - DMS_LEN, DMS_LEN);

  bool retval;
  if (ownDialog)
    retval = OwnSaveFileDialog(hwnd, prepared, path, cp, BOM);
  else
    retval = StdSaveFileDialog(hwnd, prepared, path, cp, BOM);

  if (retval)
  {
    int index = GetFileNameIndice(path.c_str());
    wcsncpy(dialogPath, path.c_str(), index);
    dialogPath[index] = L'\0';

    addExtention(path, iSaveFilter);
  }

  return retval;
}

// Function shows standart save file dialog. Returns path and code page. 
// cp must be initialized by default code page before call
bool CDialogs::StdOpenFileDialog(HWND hwnd, std::wstring &path, LPCWSTR &cp)
{
  OPENFILENAME ofn;
  wchar_t name[PATH_SIZE] = L"";

  ZeroMemory(&ofn, sizeof(ofn));
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = hwnd;
  ofn.lpstrFile = name;
  ofn.nMaxFile = PATH_SIZE;
  ofn.lpstrFilter = filter;
  ofn.nFilterIndex = iOpenFilter + 1;
  ofn.hInstance = g_hResInstance;
  ofn.Flags = OFN_EXPLORER;
  ofn.lpstrInitialDir = dialogPath;

  if (!GetOpenFileName(&ofn)) 
  {
/*#if !defined(_WIN32_WCE)
    showSystemError(CommDlgExtendedError());
#else
    if (isFullScreen)
      SendMessage(hmainWnd, WM_COMMAND, IDM_FULL_SCREEN, 0);
#endif
    free(fileName);*/
    return false;     // if error or cancel button
  }
  iOpenFilter = ofn.nFilterIndex - 1;
  path = name;

  int index = GetFileNameIndice(name);
  wcsncpy(dialogPath, name, index);
  dialogPath[index] = L'\0';

  // get coding
  SGetExtData gcd;
  gcd.cp = cp;
  gcd.forOpen = true;
  DialogBoxParam(g_hResInstance, MAKEINTRESOURCE(DLG_GETCODING), hwnd, 
                 reinterpret_cast<DLGPROC>(GetCodingDlgProc),
                 reinterpret_cast<LPARAM>(&gcd));
  cp = gcd.cp;

  return true;
}

// Function shows save file dialog. Returns path and code page
bool CDialogs::StdSaveFileDialog(HWND hwnd, const std::wstring &name, std::wstring &path, LPCWSTR &cp, bool &BOM)
{
  OPENFILENAME ofn;
  wchar_t buff[PATH_SIZE];

  wcscpy(buff, name.c_str());

  ZeroMemory(&ofn, sizeof(ofn));
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = hwnd;
  ofn.lpstrFile = buff;
  ofn.nMaxFile = PATH_SIZE;
  ofn.lpstrFilter = filter;
  ofn.nFilterIndex = iSaveFilter + 1;
  ofn.hInstance = g_hResInstance;
  ofn.Flags = OFN_EXPLORER;
  ofn.lpstrInitialDir = dialogPath;

  if (!GetSaveFileName(&ofn)) 
  {
/*#if !defined(_WIN32_WCE)
    showSystemError(CommDlgExtendedError());
#else
    if (isFullScreen)
      SendMessage(hmainWnd, WM_COMMAND, IDM_FULL_SCREEN, 0);
#endif
    free(fileName);*/
    return false;     // if error or cancel button
  }
  iSaveFilter = ofn.nFilterIndex - 1;
  path = buff;

  // get coding
  SGetExtData ged;
  ged.cp = cp;
  ged.BOM = BOM;
  ged.forOpen = false;
  DialogBoxParam(g_hResInstance, MAKEINTRESOURCE(DLG_GETCODING), hwnd, 
                 reinterpret_cast<DLGPROC>(GetCodingDlgProc),
                 reinterpret_cast<LPARAM>(&ged));
  cp = ged.cp;
  BOM = ged.BOM;

  return true;
}

bool CDialogs::OwnOpenFileDialog(HWND hwnd, std::wstring &path, LPCWSTR &cp)
{
  wchar_t pathStr[PATH_SIZE] = L"";
  SGetFileName gfn;

  memset(&gfn, 0, sizeof(SGetFileName));
  gfn.path = pathStr;
  gfn.pathSize = PATH_SIZE;
  gfn.hwndOwner = hwnd;
  gfn.filter = filter;
  gfn.filterIndex = iOpenFilter;
  gfn.codePage = cp;
  gfn.initialDir = dialogPath;

  int retval = GetFileName(&gfn);
  if (retval <= 0)
  {
    if (retval < 0)
    {
      // TODO: make CError::TranslateGetFileError
      CError::SetError(NOTEPAD_DIALOG_ERROR);
    }
    return false;
  }
  iOpenFilter = gfn.filterIndex;
  cp = gfn.codePage;
  path = pathStr;

  int index = GetFileNameIndice(pathStr);
  wcsncpy(dialogPath, pathStr, index);
  dialogPath[index] = L'\0';

  return true;
}

bool CDialogs::OwnSaveFileDialog(HWND hwnd, const std::wstring &name, std::wstring &path, LPCWSTR &cp, bool &BOM)
{
  wchar_t pathStr[PATH_SIZE] = L"";
  SGetFileName gfn;

  wcscpy(pathStr, name.c_str());

  memset(&gfn, 0, sizeof(SGetFileName));
  gfn.path = pathStr;
  gfn.pathSize = PATH_SIZE;
  gfn.hwndOwner = hwnd;
  gfn.filter = filter;
  gfn.filterIndex = iSaveFilter;
  gfn.saveDialog = true;
  gfn.codePage = cp;
  gfn.BOM = BOM;
  gfn.initialDir = dialogPath;

  int retval = GetFileName(&gfn);
  if (retval <= 0)
  {
    if (retval < 0)
    {
      // TODO: make CError::TranslateGetFileError
      CError::SetError(NOTEPAD_DIALOG_ERROR);
    }
    return false;     // if error or cancel button
  }
  iSaveFilter = gfn.filterIndex;
  cp = gfn.codePage;
  BOM = gfn.BOM;
  path = pathStr;

  return true;
}


UINT_PTR CDialogs::MsgOKDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
  static SMsgBoxData *pdata;

  #define BUTTONS_MARGIN 10

  switch(msg)
  {
  case WM_INITDIALOG:
    {
      pdata = reinterpret_cast<SMsgBoxData*>(lParam);

      HWND hctrl = GetDlgItem(hdlg, IDD_TEXT_0);
      SendMessage(hctrl, WM_SETFONT, (WPARAM)pdata->hfont, 0);

      TEXTMETRIC tm;
      HDC hdc = GetDC(hctrl);
      SelectObject(hdc, pdata->hfont);
      GetTextMetrics(hdc, &tm);
      ReleaseDC(hctrl, hdc);

      int linesNumber = 1;

      if (pdata->message)
      {
        SetWindowText(hctrl, pdata->message);
        linesNumber = SendMessage(hctrl, EM_GETLINECOUNT, 0, 0);
      }

      if (pdata->caption)
        SetWindowText(hdlg, pdata->caption);

      HWND hparentWnd = GetParent(hdlg);
      if (!hparentWnd) 
        hparentWnd = GetDesktopWindow();

      if (hparentWnd)
      {
        int x, y, width, height, buttonY;
        RECT rc, rcParent;

        GetWindowRect(hparentWnd, &rcParent);
        GetWindowRect(hdlg, &rc);

        width = rc.right - rc.left;
        height = min(rc.bottom - rc.top + tm.tmHeight * (linesNumber - 1) + BUTTONS_MARGIN, rcParent.bottom - rcParent.top);
        x = (rcParent.right - rcParent.left - width) / 2;
        y = (rcParent.bottom - rcParent.top - height) / 2;
        MoveWindow(hdlg, x, y, width, height, TRUE); 
        GetClientRect(hdlg, &rcParent);

        buttonY = moveButton(GetDlgItem(hdlg, IDOK), &rcParent, CENTER);

        GetClientRect(hctrl, &rc);
        x = rc.left;
        y = min((rcParent.bottom - rcParent.top) / 5, MAX_TEXT_MARGIN_Y);
        MoveWindow(hctrl, x, y, rc.right - rc.left, buttonY - y - BUTTONS_MARGIN, TRUE);
      }
    }
    return TRUE;

  case WM_COMMAND:
    switch(LOWORD(wParam))
    {
    case IDCANCEL:
    case IDOK :
      EndDialog(hdlg, IDOK);
      return TRUE;
    }
    break;

  case WM_CTLCOLORDLG:
  case WM_CTLCOLORSTATIC: 
      SetBkColor((HDC)wParam, (COLORREF)GetSysColor(COLOR_3DFACE));
      return (UINT_PTR)GetSysColorBrush(COLOR_3DFACE);

  }

  #undef BUTTONS_MARGIN

  return FALSE;
}

UINT_PTR CALLBACK CDialogs::MsgYesNoDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
  static SMsgBoxData *pdata;

  #define BUTTONS_MARGIN 10

  switch(msg)
  {
  case WM_INITDIALOG:
    {
      pdata = reinterpret_cast<SMsgBoxData*>(lParam);

      HWND hctrl = GetDlgItem(hdlg, IDD_TEXT_0);
      SendMessage(hctrl, WM_SETFONT, (WPARAM)pdata->hfont, 0);

      TEXTMETRIC tm;
      HDC hdc = GetDC(hctrl);
      SelectObject(hdc, pdata->hfont);
      GetTextMetrics(hdc, &tm);
      ReleaseDC(hctrl, hdc);

      int linesNumber = 1;

      if (pdata->message)
      {
        SetWindowText(hctrl, pdata->message);
        linesNumber = SendMessage(hctrl, EM_GETLINECOUNT, 0, 0);
      }
      if (pdata->caption)
        SetWindowText(hdlg, pdata->caption);

      HWND hparentWnd = GetParent(hdlg);
      if (!hparentWnd) 
        hparentWnd = GetDesktopWindow();

      if (hparentWnd)
      {
        int x, y, width, height, buttonY;
        RECT rc, rcParent;

        GetWindowRect(hparentWnd, &rcParent);
        GetWindowRect(hdlg, &rc);

        width = rc.right - rc.left;
        height = min(rc.bottom - rc.top + tm.tmHeight * (linesNumber - 1) + BUTTONS_MARGIN, rcParent.bottom - rcParent.top);
        x = (rcParent.right - rcParent.left - width) / 2;
        y = (rcParent.bottom - rcParent.top - height) / 2;
        MoveWindow(hdlg, x, y, width, height, TRUE); 
        GetClientRect(hdlg, &rcParent);
        
        moveButton(GetDlgItem(hdlg, IDYES), &rcParent, LEFT);
        buttonY = moveButton(GetDlgItem(hdlg, IDNO), &rcParent, RIGHT);

        GetClientRect(hctrl, &rc);
        x = rc.left + 1;
        y = min((rcParent.bottom - rcParent.top) / 5, MAX_TEXT_MARGIN_Y);
        MoveWindow(hctrl, x, y, rc.right - rc.left, buttonY - y - BUTTONS_MARGIN, TRUE);
      }
    }
    return TRUE;

  case WM_COMMAND:
    switch(LOWORD(wParam))
    {
    case IDYES:
    case IDNO:
      EndDialog(hdlg, LOWORD(wParam));
      return TRUE;
    }
    break;

  case WM_CTLCOLORDLG:
  case WM_CTLCOLORSTATIC: 
    SetBkColor((HDC)wParam, (COLORREF)GetSysColor(COLOR_3DFACE));
    return (UINT_PTR)GetSysColorBrush(COLOR_3DFACE);

  }

  #undef BUTTONS_MARGIN

  return FALSE;
}

// function moves button and returns y of the top border of a button.
int CDialogs::moveButton(HWND hwnd, RECT *parentRect, ButtonPosition position)
{
  int x, y, width, height;
  RECT rect;

  GetClientRect(hwnd, &rect);
  width = rect.right - rect.left;
  height = rect.bottom - rect.top;
  switch (position)
  {
  case LEFT:
    x = ((parentRect->right / 3) - width) / 2;
    break;
  case CENTER:
    x = (parentRect->right / 3) + ((parentRect->right / 3) - width) / 2;
    break;
  case RIGHT:
    x = (2 * parentRect->right / 3) + ((parentRect->right / 3) - width) / 2;
    break;
  }
  y = parentRect->bottom - (int)(1.4 * height);
  MoveWindow(hwnd, x, y, width, height, TRUE);

  return y;
}


UINT_PTR CALLBACK CDialogs::MsgYesNoCancelDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
  static SMsgBoxData *pdata;

  #define BUTTONS_MARGIN 10

  switch(msg)
  {
  case WM_INITDIALOG:
    {
      pdata = reinterpret_cast<SMsgBoxData*>(lParam);

      HWND hctrl = GetDlgItem(hdlg, IDD_TEXT_0);
      SendMessage(hctrl, WM_SETFONT, (WPARAM)pdata->hfont, 0);

      TEXTMETRIC tm;
      HDC hdc = GetDC(hctrl);
      SelectObject(hdc, pdata->hfont);
      GetTextMetrics(hdc, &tm);
      ReleaseDC(hctrl, hdc);

      int linesNumber = 1;

      if (pdata->message)
      {
        SetWindowText(hctrl, pdata->message);
        linesNumber = SendMessage(hctrl, EM_GETLINECOUNT, 0, 0);
      }

      if (pdata->caption)
        SetWindowText(hdlg, pdata->caption);

      HWND hparentWnd = GetParent(hdlg);
      if (!hparentWnd) 
        hparentWnd = GetDesktopWindow();

      if (hparentWnd)
      {
        int x, y, width, height, buttonY;
        RECT rc, rcParent;

        GetWindowRect(hparentWnd, &rcParent);
        GetWindowRect(hdlg, &rc);

        width = rc.right - rc.left;
        height = min(rc.bottom - rc.top + tm.tmHeight * (linesNumber - 1) + BUTTONS_MARGIN, rcParent.bottom - rcParent.top);
        x = (rcParent.right - rcParent.left - width) / 2;
        y = (rcParent.bottom - rcParent.top - height) / 2;
        MoveWindow(hdlg, x, y, width, height, TRUE); 
        GetClientRect(hdlg, &rcParent);
        
        moveButton(GetDlgItem(hdlg, IDYES), &rcParent, LEFT);
        moveButton(GetDlgItem(hdlg, IDNO), &rcParent, CENTER);
        buttonY = moveButton(GetDlgItem(hdlg, IDCANCEL), &rcParent, RIGHT);

        GetClientRect(hctrl, &rc);
        x = rc.left;
        y = min((rcParent.bottom - rcParent.top) / 5, MAX_TEXT_MARGIN_Y);
        MoveWindow(hctrl, x, y, rc.right - rc.left, buttonY - y - BUTTONS_MARGIN, TRUE);
      }
    }
    return TRUE;

  case WM_COMMAND:
    switch(LOWORD(wParam))
    {
    case IDYES:
    case IDNO:
    case IDCANCEL:
      EndDialog(hdlg, LOWORD(wParam));
      return TRUE;
    }
    break;

  case WM_CTLCOLORDLG:
  case WM_CTLCOLORSTATIC: 
    SetBkColor((HDC)wParam, (COLORREF)GetSysColor(COLOR_3DFACE));
    return (UINT_PTR)GetSysColorBrush(COLOR_3DFACE);

  }

  #undef BUTTONS_MARGIN

  return FALSE;
}


//#define DONATE_MESSAGE L"Если Вы желаете поддержать проект:\r\nYM: 41001586724267\r\n\
//WMR: R365124318337\r\nWMZ: Z360185849377\r\nWME: E327666982480"
//#define DONATE_MESSAGE L"YM: 41001586724267\r\nWMR: R365124318337\r\nWMZ: Z360185849377\r\nWME: E327666982480"

#define COPYRIGHTS_MESSAGE L"Regexp engine: Copyright (c) 1986, 1993, 1995 by University of Toronto.\
 Written by Henry Spencer. Altered by Benoit Goudreault-Emond, bge@videotron.ca\r\n\r\n\
Icons: Copyright(c) by David_Vignoni, license: LGPL\r\n\r\n\
Icons: Copyright(c) by Visual_Pharm, license: Creative Commons Attribution-No Derivative Works 3.0 Unported\r\n\r\n\
Спасибо за иконку программы: Mr.Anderson, 4pda.ru"

UINT_PTR CALLBACK CDialogs::AboutDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
  static bool copyrights, selection;

  switch(msg)
  {
  case WM_INITDIALOG:
    {
#ifdef _WIN32_WCE
      SHINITDLGINFO shidi;
      shidi.dwMask = SHIDIM_FLAGS;
      shidi.dwFlags = SHIDIF_DONEBUTTON | SHIDIF_SIZEDLGFULLSCREEN | SHIDIF_EMPTYMENU;
      shidi.hDlg = hdlg;
      SHInitDialog(&shidi);
#endif
      HWND hitems[6] = 
      {
        GetDlgItem(hdlg, IDD_PROGRAM_NAME),
        GetDlgItem(hdlg, IDD_AUTHOR),
        GetDlgItem(hdlg, IDD_EMAIL),
        GetDlgItem(hdlg, IDD_EDIT_0),
//#ifdef _WIN32_WCE
//        GetDlgItem(hdlg, IDD_BTN_0),
//#endif
        GetDlgItem(hdlg, IDOK)
      };
      RECT rc;
      GetClientRect(hitems[0], &rc);
      Location::SetNet(hdlg, 100, rc.bottom, 0, rc.bottom / 6, false, true);

      SWindowPos positions[6] = 
      {
        {10, 1, 100, 2},
        {10, 3, 100, 2},
        {10, 5, 100, 2},
        {10, 7, 80, Location::MaxY - 7 - 4},
//#ifdef _WIN32_WCE
//        {52, Location::MaxY - 2, 38, 2},
//        {10, Location::MaxY - 2, 38, 2}
//#else
        {20, Location::MaxY - 3, 60, 2}
//#endif
      };

      Location::Arrange(hitems, positions, 5);

      copyrights = true;
      selection = true;
      SetWindowText(hitems[3], COPYRIGHTS_MESSAGE);
    }
    return TRUE;

  case WM_COMMAND:
    switch(LOWORD(wParam))
    {
    case IDOK :
    case IDCANCEL :
      EndDialog(hdlg, 0);
      return TRUE;

    /*case IDD_BTN_0:  // Copyrights / Donate
      if (copyrights)
      {
        SetWindowText(GetDlgItem(hdlg, IDD_EDIT_0), DONATE_MESSAGE);
        SetWindowText(GetDlgItem(hdlg, IDD_BTN_0), L"Copyrights");
      }
      else
      {
        SetWindowText(GetDlgItem(hdlg, IDD_EDIT_0), COPYRIGHTS_MESSAGE);
        SetWindowText(GetDlgItem(hdlg, IDD_BTN_0), L"Donate Info");
      }
      copyrights = !copyrights;
      return TRUE;*/
    }
    break;

  case EN_UPDATE:
    if (selection)  // this is hack. Text in edit box is selected after first SetWindowText call in WM_INITDIALOG
    {
      SendMessage(GetDlgItem(hdlg, IDD_EDIT_0), EM_SETSEL, -1, 0);
      selection = false;
    }
    return TRUE;
  }

  return FALSE;
}

UINT_PTR CALLBACK CDialogs::GetCodingDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
  static SGetExtData *pcd;

  switch (msg)
  {
  case WM_INITDIALOG: 
    {
      CNotepad &Notepad = CNotepad::Instance();

      pcd = reinterpret_cast<SGetExtData*>(lParam);

      Notepad.Documents.CodingManager.FillComboboxWithCodings(GetDlgItem(hdlg, IDD_CODING), pcd->cp, pcd->forOpen);
      if (!pcd->forOpen)
        CheckDlgButton(hdlg, IDD_BOM, pcd->BOM ? MF_CHECKED : MF_UNCHECKED);
      else
        ShowWindow(GetDlgItem(hdlg, IDD_BOM), SW_HIDE);
      return TRUE;
    }

  case WM_COMMAND:
    if (LOWORD(wParam) == IDOK)
    {
      // set coding
      HWND hcoding = GetDlgItem(hdlg, IDD_CODING);
      int selectedItem = SendMessage(hcoding, CB_GETCURSEL, 0, 0);
      LRESULT codingId = SendMessage(hcoding, CB_GETITEMDATA, selectedItem, 0);
      if (codingId != CB_ERR)
        pcd->cp = (LPCWSTR)codingId;

      // set bom flag
      if (!pcd->forOpen)
        pcd->BOM = IsDlgButtonChecked(hdlg, IDD_BOM) ? true : false;
      EndDialog(hdlg, 0);
    }
    return TRUE;
  }
  return FALSE;
}


UINT_PTR CALLBACK CDialogs::GotoLineDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
  static SGotoLineData *pgl;
  switch (msg)
  {
case WM_INITDIALOG:
    {
      pgl = reinterpret_cast<SGotoLineData*>(lParam);

      wchar_t str[64];
      swprintf(str, GOTOLINE_MESSAGE, 1, pgl->maxNumber);
      SetDlgItemText(hdlg, IDD_TEXT_0, str);

      SetDlgItemInt(hdlg, IDD_EDIT_0, pgl->current, FALSE);

      HWND hparentWnd = GetParent(hdlg);
      if (hparentWnd)
      {
        RECT parentRect, rect;
        GetClientRect(hparentWnd, &parentRect);
        GetClientRect(hdlg, &rect);

        int x = (parentRect.right - rect.right) / 2;
        int y = (parentRect.bottom - rect.bottom) / 2;
        MoveWindow(hdlg, x, y, rect.right, rect.bottom, TRUE); 
      }
    }
    return TRUE;

  case WM_COMMAND:
    switch(LOWORD(wParam))
    {
    case IDOK :
      pgl->current = GetDlgItemInt(hdlg, IDD_EDIT_0, 0, FALSE);
      EndDialog(hdlg, 1);
      return TRUE;

    case IDCANCEL :
      EndDialog(hdlg, 0);
      return TRUE;
    }
    break;
  }

  return FALSE;
}


UINT_PTR CALLBACK CDialogs::FileInfoDialogProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
  static SFileInfoData *pfi;

  switch (msg)
  {
  case WM_INITDIALOG:
    {
      pfi = reinterpret_cast<SFileInfoData*>(lParam);

      // File path
      SetDlgItemText(hdlg, IDD_PATHTOFILE_FIELD, pfi->path.c_str());

      // File size
      if (pfi->size > 1024 * 1024)
      {
        wchar_t msg[32];
        swprintf(msg, L"%.2f MB", ((float)pfi->size / (1024 * 1024)));
        SetDlgItemText(hdlg, IDD_FILESIZE_FIELD, msg);
      }
      else if (pfi->size > 1024)
      {
        wchar_t msg[32];
        swprintf(msg, L"%.2f KB", ((float)pfi->size / 1024));
        SetDlgItemText(hdlg, IDD_FILESIZE_FIELD, msg);
      }
      else
      {
        wchar_t msg[32];
        swprintf(msg, L"%d B", pfi->size);
        SetDlgItemText(hdlg, IDD_FILESIZE_FIELD, msg);
      }


      // Adjust dialog size
#ifdef UNDER_CE
      SHINITDLGINFO shidi;
      shidi.dwMask = SHIDIM_FLAGS;
      shidi.dwFlags = SHIDIF_DONEBUTTON | SHIDIF_SIZEDLGFULLSCREEN | SHIDIF_EMPTYMENU;
      shidi.hDlg = hdlg;
      SHInitDialog(&shidi);
#endif

      // Arrange items
      HWND hitems[5] = {
        GetDlgItem(hdlg, IDD_PATHTOFILE_TITLE),
        GetDlgItem(hdlg, IDD_PATHTOFILE_FIELD),
        GetDlgItem(hdlg, IDD_FILESIZE_TITLE),
        GetDlgItem(hdlg, IDD_FILESIZE_FIELD),
        GetDlgItem(hdlg, IDOK),
      };

      RECT rc;
      GetClientRect(hitems[0], &rc);
      Location::SetNet(hdlg, 100, rc.bottom / 4, 0, 4, false, true);

      SWindowPos positions[5] = {
        {4, 1, 92, 4},
        {4, 5, 92, 4},
        {4, 10, 20, 4},
        {24, 10, 60, 4},
        {30, Location::MaxY - 5, 40, 4}
      };

      Location::Arrange(hitems, positions, 5);
    }
    return TRUE;

  case WM_COMMAND:
    switch(LOWORD(wParam))
    {
    case IDOK :
      EndDialog(hdlg, 0);
      return TRUE;
    }
    break;
  }

  return FALSE;
}


// Options routine

// prototypes
static int CALLBACK PropSheetHeaderProc(HWND hdlg, UINT uMsg, LPARAM lParam);
static BOOL CALLBACK CommonDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL CALLBACK AssotiationsDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL CALLBACK FontDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL CALLBACK AppearanceDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL CALLBACK LanguageDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);


struct SCommonOptions 
{
  bool saveSession;
  bool defaultWordWrap;
  bool defaultSmartInput;
  bool defaultDragMode;
  bool defaultScrollByLine;
  bool responseOnSIP;
};

struct SFontOptions
{
  LOGFONT leditorFont;
  LOGFONT ltabFont;
  int tabSize;
};

struct SAppearanceOptions
{
  int leftIndent;
  bool isToolbar;
  bool isStatusBar;
  bool permanentScrollBars;
};

struct SLanguageOptions
{
  wchar_t pathToDll[MAX_PATH];
};


void CDialogs::OptionsDialog(HWND hwnd, COptions &Options)
{
#define NPAGES 5

  SCommonOptions co;
  co.saveSession = Options.SaveSession();
  co.defaultDragMode = Options.DefaultDragMode();
  co.defaultScrollByLine = Options.DefaultScrollByLine();
  co.defaultSmartInput = Options.DefaultSmartInput();
  co.defaultWordWrap = Options.DefaultWordWrap();
  co.responseOnSIP = Options.ResponseOnSIP();

  SFontOptions fo;
  memcpy(&fo.leditorFont, &Options.GetProfile().leditorFont, sizeof(LOGFONT));
  memcpy(&fo.ltabFont, &Options.GetProfile().ltabFont, sizeof(LOGFONT));
  fo.tabSize = Options.GetProfile().tabSize;

  SAppearanceOptions ao;
  ao.isStatusBar = Options.IsStatusBar();
  ao.isToolbar = Options.IsToolbar();
  ao.permanentScrollBars = Options.PermanentScrollBars();
  ao.leftIndent = Options.LeftIndent();

  SLanguageOptions lo;
  wcscpy(lo.pathToDll, Options.LangLib().c_str());

  
  PROPSHEETHEADER psh;
  PROPSHEETPAGE psp[NPAGES];

  ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
  psh.dwSize = sizeof(PROPSHEETHEADER);
  psh.dwFlags = PSH_PROPSHEETPAGE | PSH_USECALLBACK;
  psh.hInstance = g_hResInstance;
  psh.hwndParent = hwnd;
  psh.nPages = NPAGES;
  psh.pfnCallback = PropSheetHeaderProc;
  psh.ppsp = psp;

  ZeroMemory(psp, NPAGES * sizeof(PROPSHEETPAGE));

  // Common
  psp[0].dwSize = sizeof(PROPSHEETPAGE);
  psp[0].hInstance = g_hResInstance;
  psp[0].pszTemplate = MAKEINTRESOURCE(DLG_COMMON_PAGE);
  psp[0].pfnDlgProc = CommonDlgProc;
  psp[0].lParam =(LPARAM)&co;

  // Fonts
  psp[1].dwSize = sizeof(PROPSHEETPAGE);
  psp[1].hInstance = g_hResInstance;
  psp[1].pszTemplate = MAKEINTRESOURCE(DLG_FONT_PAGE);
  psp[1].pfnDlgProc = FontDlgProc;
  psp[1].lParam =(LPARAM)&fo;

  // Appearance
  psp[2].dwSize = sizeof(PROPSHEETPAGE);
  psp[2].hInstance = g_hResInstance;
  psp[2].pszTemplate = MAKEINTRESOURCE(DLG_APPEARANCE_PAGE);
  psp[2].pfnDlgProc = AppearanceDlgProc;
  psp[2].lParam = (LPARAM)&ao;

  // Associations
  psp[3].dwSize = sizeof(PROPSHEETPAGE);
  psp[3].hInstance = g_hResInstance;
  psp[3].pszTemplate = MAKEINTRESOURCE(DLG_ASSOCIATIONS_PAGE);
  psp[3].pfnDlgProc = AssotiationsDlgProc;

  // Language
  psp[4].dwSize = sizeof(PROPSHEETPAGE);
  psp[4].hInstance = g_hResInstance;
  psp[4].pszTemplate = MAKEINTRESOURCE(DLG_LANGUAGE_PAGE);
  psp[4].pfnDlgProc = LanguageDlgProc;
  psp[4].lParam = (LPARAM)&lo;

  if(!PropertySheet(&psh))
  {
    //if (isFullScreen)
    //  SendMessage(hmainWnd, WM_COMMAND, IDM_FULL_SCREEN, 0);
#ifdef _WIN32_WCE
    DestroyOkCancelBar();
#endif

    return;
  }

#ifdef _WIN32_WCE
    DestroyOkCancelBar();
#endif

  // set common options
  Options.SetSaveSession(co.saveSession);
  Options.SetDefaultWordWrap(co.defaultWordWrap);
  Options.SetDefaultSmartInput(co.defaultSmartInput);
  Options.SetDefaultDragMode(co.defaultDragMode);
  Options.SetDefaultScrollByLine(co.defaultScrollByLine);
  Options.SetResponseOnSIP(co.responseOnSIP);
  Options.SetLangLib(lo.pathToDll);

  // set font options
  Options.SetEditorFont(&fo.leditorFont);
  Options.SetTabsFont(&fo.ltabFont);
  Options.SetTabulationSize(fo.tabSize);

  // set appearance options
  Options.SetToolbar(ao.isToolbar);
  Options.SetStatusBar(ao.isStatusBar);
  Options.SetLeftIndent(ao.leftIndent);
  Options.SetPermanentScrollBars(ao.permanentScrollBars);
}



static int CALLBACK PropSheetHeaderProc(HWND hdlg, UINT uMsg, LPARAM lParam)
{
  if (uMsg == PSCB_INITIALIZED)
  {
#ifdef _WIN32_WCE
    SHINITDLGINFO shidi;
    shidi.dwMask = SHIDIM_FLAGS;
    shidi.dwFlags = SHIDIF_DONEBUTTON | SHIDIF_SIZEDLGFULLSCREEN/* | SHIDIF_EMPTYMENU*/;
    shidi.hDlg = hdlg;
    SHInitDialog(&shidi);

    CreateOkCancelBar(hdlg);
#endif
  }
  return 0;
}


static BOOL CALLBACK CommonDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
  static SCommonOptions *pco;
  static bool initialized = false;

  switch (msg)
  {
  case WM_INITDIALOG:
    {
      initialized = true;

#ifdef _WIN32_WCE
      SHINITDLGINFO shidi;
      shidi.dwMask = SHIDIM_FLAGS;
      shidi.dwFlags = SHIDIF_SIZEDLGFULLSCREEN;
      shidi.hDlg = hdlg;
      SHInitDialog(&shidi);
#endif

      pco = (SCommonOptions*)((LPPROPSHEETPAGE)lParam)->lParam;

      CheckDlgButton(hdlg, IDD_DEF_WORDWRAP, pco->defaultWordWrap ? MF_CHECKED : MF_UNCHECKED);
      CheckDlgButton(hdlg, IDD_DEF_SMARTINPUT, pco->defaultSmartInput ? MF_CHECKED : MF_UNCHECKED);
      CheckDlgButton(hdlg, IDD_DEF_DRAGMODE, pco->defaultDragMode ? MF_CHECKED : MF_UNCHECKED);
      CheckDlgButton(hdlg, IDD_DEF_SCROLLBYLINE, pco->defaultScrollByLine ? MF_CHECKED : MF_UNCHECKED);
      CheckDlgButton(hdlg, IDD_SAVE_SESSION, pco->saveSession ? MF_CHECKED : MF_UNCHECKED);
      CheckDlgButton(hdlg, IDD_RESPONSE_ON_SIP, pco->responseOnSIP ? MF_CHECKED : MF_UNCHECKED);
    }
    return TRUE;

  case WM_NOTIFY:
    {
      LPNMHDR pnmh =(LPNMHDR) lParam;

      if (pnmh->code == PSN_APPLY && initialized)
      {
        pco->defaultWordWrap = IsDlgButtonChecked(hdlg, IDD_DEF_WORDWRAP) ? true : false;
        pco->defaultSmartInput = IsDlgButtonChecked(hdlg, IDD_DEF_SMARTINPUT) ? true : false;
        pco->defaultDragMode = IsDlgButtonChecked(hdlg, IDD_DEF_DRAGMODE) ? true : false;
        pco->defaultScrollByLine = IsDlgButtonChecked(hdlg, IDD_DEF_SCROLLBYLINE) ? true : false;
        pco->saveSession = IsDlgButtonChecked(hdlg, IDD_SAVE_SESSION) ? true : false;
        pco->responseOnSIP = IsDlgButtonChecked(hdlg, IDD_RESPONSE_ON_SIP) ? true : false;
        initialized = false;
        return TRUE;
      }
    }

  }
  return FALSE;
}

static BOOL CALLBACK AssotiationsDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
  const int EXTENSIONS_NUMBER = 11;
  static char associated[EXTENSIONS_NUMBER];
  static extension_t extensions[EXTENSIONS_NUMBER] = {{L".txt",  0,  EXT_TXT}, 
                                                      {L".ini",  0,  EXT_INI},
                                                      {L".c",    0,  EXT_C},
                                                      {L".cpp",  0,  EXT_CPP},
                                                      {L".h",    0,  EXT_H},
                                                      {L".hpp",  0,  EXT_HPP},
                                                      {L".htm",  0,  EXT_HTM},
                                                      {L".html", 0,  EXT_HTML},
                                                      {L".php",  0,  EXT_PHP},
                                                      {L".asm",  0,  EXT_ASM},
                                                      {L".pas",  0,  EXT_PAS}};

  switch(msg)
  {
  case WM_INITDIALOG:
    {
      char error;

#ifdef _WIN32_WCE
      SHINITDLGINFO shidi;
      shidi.dwMask = SHIDIM_FLAGS;
      shidi.dwFlags = SHIDIF_SIZEDLGFULLSCREEN;
      shidi.hDlg = hdlg;
      SHInitDialog(&shidi);
#endif
      if ((error = TestCrossPadKey()) != REG_NO_ERROR)
      {
        std::wstring message;
        CError::GetErrorMessage(message, CError::TranslateRegister(error));
        CDialogs::ShowError(hdlg, message);
      }

      for (int i = 0; i < EXTENSIONS_NUMBER; i++)
      {
        if ((associated[i] = TestAssociation(&extensions[i])) > 0)
          CheckDlgButton(hdlg, extensions[i].id, MF_CHECKED);
        else if (associated[i] < 0) // error
        {
          std::wstring message;
          CError::GetErrorMessage(message, CError::TranslateRegister(associated[i]));
          CDialogs::ShowError(hdlg, message);
          associated[i] = 0;
        }
      }
    }
    return TRUE;

  case WM_NOTIFY:
    {
      LPNMHDR pnmh =(LPNMHDR) lParam;

      switch (pnmh->code)
      {
      case PSN_APPLY:
        {
          int i;
          char err;

          for (i = 0; i < EXTENSIONS_NUMBER; i++)
          {
            if (IsDlgButtonChecked(hdlg, extensions[i].id))
            {
              if (!associated[i])
              {
                err = Associate(&extensions[i]);
                if (err != REG_NO_ERROR)
                {
                  std::wstring message;
                  CError::GetErrorMessage(message, CError::TranslateRegister(err));
                  CDialogs::ShowError(hdlg, message);
                }
              }
            }
            else
            {
              if (associated[i])
              {
                err = Deassociate(&extensions[i]);
                if (err != REG_NO_ERROR)
                {
                  std::wstring message;
                  CError::GetErrorMessage(message, CError::TranslateRegister(err));
                  CDialogs::ShowError(hdlg, message);
                }
              }
            }
          }
        }
        return TRUE;
      }
    }
  }
  return FALSE;
}

static BOOL CALLBACK LanguageDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
  static wchar_t path[MAX_PATH];
  static wchar_t libStr[MSG_MAX_LENGTH];
  static std::vector<std::wstring> names;
  static SLanguageOptions *plo;

  switch(msg)
  {
  case WM_INITDIALOG:
    {
      HWND hitems[2] = {GetDlgItem(hdlg, IDD_LANGUAGE), GetDlgItem(hdlg, IDD_RESTART_WARNING)};

      names.clear();
      plo = (SLanguageOptions*)((LPPROPSHEETPAGE)lParam)->lParam;

      SendMessage(hitems[0], CB_ADDSTRING, 0, (LPARAM)DEFAULT_LANGUAGE);
      SendMessage(hitems[0], CB_SETCURSEL, 0, 0);

      size_t len = wcslen(g_execPath);
      wcscpy(path, g_execPath);
      wcscpy(&path[len], L"\\lang\\*.dll");

#ifdef _WIN32_WCE
      SHINITDLGINFO shidi;
      shidi.dwMask = SHIDIM_FLAGS;
      shidi.dwFlags = SHIDIF_SIZEDLGFULLSCREEN;
      shidi.hDlg = hdlg;
      SHInitDialog(&shidi);
#endif

      RECT rc;
      GetClientRect(hitems[0], &rc);
      Location::SetNet(hdlg, 100, rc.bottom, 0, 4, false, true);
      SWindowPos positions[2] = 
      {
        {10, 1, 80, 1}, // combobox
        {10, 3, 80, 4}  // static
      };
      Location::Arrange(hitems, positions, 2);

      WIN32_FIND_DATA fd;
      memset(&fd, 0, sizeof(WIN32_FIND_DATA));

      int i = 0;
      HANDLE hfind = FindFirstFile(path, &fd);
      if (INVALID_HANDLE_VALUE != hfind)
      {
        HINSTANCE hInstance;

        do
        {
          wcscpy(&path[GetFileNameIndice(path)], fd.cFileName);
          if ((hInstance = LoadLibrary(path)) != 0 && LoadString(hInstance, IDS_LANGUAGE_NAME, libStr, MSG_MAX_LENGTH))
          {
            names.push_back(fd.cFileName);
            SendMessage(hitems[0], CB_ADDSTRING, 0, (LPARAM)libStr);
            if (wcscmp(libStr, g_languageName) == 0)
              SendMessage(hitems[0], CB_SETCURSEL, names.size(), 0);
          }
          if (hInstance)
            FreeLibrary(hInstance);
        } while (FindNextFile(hfind, &fd) > 0);

        FindClose(hfind);
      }
    }
    return TRUE;

    case WM_NOTIFY:
      if (((LPNMHDR)lParam)->code == PSN_APPLY)
      {
        int i = SendMessage(GetDlgItem(hdlg, IDD_LANGUAGE), CB_GETCURSEL, 0, 0);

        if (i > 0)
        {
          size_t len = wcslen(g_execPath);
          wcscpy(plo->pathToDll, g_execPath);

          wcscpy(&plo->pathToDll[len], L"\\lang\\");
          len += 6;

          wcscpy(&plo->pathToDll[len], names[i - 1].c_str());
        }
        else
          wcscpy(plo->pathToDll, DEFAULT_LANGUAGE_PATH); // we should set default language

        return TRUE;
      }
      break;
  }
  return FALSE;
}

// for the font dialog
//struct SEnumFontData
//{
//  HWND hcombobox[2];
//};

static int CALLBACK enumFontFamProc(const LOGFONT *plf, const TEXTMETRIC *tm, DWORD FontType, LPARAM lParam)
{
  //SEnumFontData efd = (HWND*)lParam;
  HWND *hctrls = (HWND*)lParam;

  SendMessage(hctrls[0], CB_ADDSTRING, 0, (LPARAM)plf->lfFaceName);
  SendMessage(hctrls[1], CB_ADDSTRING, 0, (LPARAM)plf->lfFaceName);

  return TRUE;
}

static BOOL CALLBACK FontDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
  static SFontOptions *pfo;
  static bool initialized = false;

  switch (msg)
  {
  case WM_INITDIALOG:
    {
      initialized = true;

#ifdef _WIN32_WCE
      SHINITDLGINFO shidi;
      shidi.dwMask = SHIDIM_FLAGS;
      shidi.dwFlags = SHIDIF_SIZEDLGFULLSCREEN;
      shidi.hDlg = hdlg;
      SHInitDialog(&shidi);
#endif

      HWND hitems[8] = 
      {
        GetDlgItem(hdlg, IDD_TEXT_0),
        GetDlgItem(hdlg, IDD_EDITOR_FONT),
        GetDlgItem(hdlg, IDD_EDITOR_SIZE),
        GetDlgItem(hdlg, IDD_TEXT_1),
        GetDlgItem(hdlg, IDD_TABULATION),
        GetDlgItem(hdlg, IDD_TEXT_2),
        GetDlgItem(hdlg, IDD_TAB_FONT),
        GetDlgItem(hdlg, IDD_TAB_SIZE)
      };
      RECT rc;
      GetClientRect(hitems[0], &rc);
      Location::SetNet(hdlg, 100, rc.bottom, 0, 4, false, true);

      SWindowPos positions[8] = 
      {
        {2, 1, 96, 1},  // editor font
        {2, 2, 66, Location::MaxY - 2},  // font combobox
        {70, 2, 24, 1},  // font size
        {2, 3, 28, 1},  // tabulation
        {70, 3, 24, 1},  // tabulation size
        {2, 4, 96, 1},  // tabs font
        {2, 5, 66, Location::MaxY - 2},  // tabs font combobox
        {70, 5, 24, 1}   // tabs font size
      };
      Location::Arrange(hitems, positions, 8);

      pfo = (SFontOptions*)((LPPROPSHEETPAGE)lParam)->lParam;

      HWND hctrls[2] = {hitems[1], hitems[6]};
      HDC hdc = CreateDC(L"DISPLAY", 0, 0, 0);
      EnumFontFamilies(hdc, NULL, enumFontFamProc, (LPARAM)hctrls);
      DeleteDC(hdc);

      // set current editor font
      int current = SendMessage(hctrls[0], CB_FINDSTRING, -1, (LPARAM)pfo->leditorFont.lfFaceName);
      SendMessage(hctrls[0], CB_SETCURSEL, current != CB_ERR ? current : 0, 0);

      // set current tab font
      current = SendMessage(hctrls[1], CB_FINDSTRING, -1, (LPARAM)pfo->ltabFont.lfFaceName);
      SendMessage(hctrls[1], CB_SETCURSEL, current != CB_ERR ? current : 0, 0);

      int value = -MulDiv(pfo->leditorFont.lfHeight, 72, g_LogPixelsY);
      SetDlgItemInt(hdlg, IDD_EDITOR_SIZE, value, TRUE); 

      value = -MulDiv(pfo->ltabFont.lfHeight, 72, g_LogPixelsY);
      SetDlgItemInt(hdlg, IDD_TAB_SIZE, value, TRUE);

      value = pfo->tabSize;
      SetDlgItemInt(hdlg, IDD_TABULATION, value, TRUE);
    }
    return TRUE;

  case WM_NOTIFY:
    if (((LPNMHDR)lParam)->code == PSN_APPLY && initialized)
    {
      int index;

      // get editor font parameters
      if ((index = SendDlgItemMessage(hdlg, IDD_EDITOR_FONT, CB_GETCURSEL, 0, 0)) != CB_ERR)
        SendDlgItemMessage(hdlg, IDD_EDITOR_FONT, CB_GETLBTEXT, index, (LPARAM)pfo->leditorFont.lfFaceName);

      int value = GetDlgItemInt(hdlg, IDD_EDITOR_SIZE, 0, TRUE);
      pfo->leditorFont.lfHeight = -MulDiv(value, g_LogPixelsY, 72);

      // get tab font parameters
      if ((index = SendDlgItemMessage(hdlg, IDD_TAB_FONT, CB_GETCURSEL, 0, 0)) != CB_ERR)
        SendDlgItemMessage(hdlg, IDD_TAB_FONT, CB_GETLBTEXT, index, (LPARAM)pfo->ltabFont.lfFaceName);

      value = GetDlgItemInt(hdlg, IDD_TAB_SIZE, 0, TRUE);
      pfo->ltabFont.lfHeight = -MulDiv(value, g_LogPixelsY, 72);

      // get tabulation size
      pfo->tabSize = GetDlgItemInt(hdlg, IDD_TABULATION, 0, FALSE);

      initialized = false;
      return TRUE; 
    }
  }
  return FALSE;
}

static BOOL CALLBACK AppearanceDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
  static SAppearanceOptions *pao;
  static bool initialized = false;

  switch (msg)
  {
  case WM_INITDIALOG:
    {
      initialized = true;

#ifdef _WIN32_WCE
      SHINITDLGINFO shidi;
      shidi.dwMask = SHIDIM_FLAGS;
      shidi.dwFlags = SHIDIF_SIZEDLGFULLSCREEN;
      shidi.hDlg = hdlg;
      SHInitDialog(&shidi);
#endif

      HWND hitems[5] = 
      {
        GetDlgItem(hdlg, IDD_STATUSBAR),
        GetDlgItem(hdlg, IDD_TOOLBAR),
        GetDlgItem(hdlg, IDD_PERMSCROLLBARS),
        GetDlgItem(hdlg, IDD_INDENT_TEXT),
        GetDlgItem(hdlg, IDD_INDENT),
      };
      RECT rc;
      GetClientRect(hitems[0], &rc);
      Location::SetNet(hdlg, 100, rc.bottom, 0, 4, false, true);

      SWindowPos positions[5] = 
      {
        {2, 1, 96, 1},  // statusbar
        {2, 2, 96, 1},  // toolbar
        {2, 3, 96, 1},  // peramnent scrollbars
        {2, 4, 50, 1},  // indent message
        {70, 4, 24, 1},  // indent edit
      };
      Location::Arrange(hitems, positions, 5);

      pao = (SAppearanceOptions*)((LPPROPSHEETPAGE)lParam)->lParam;

      CheckDlgButton(hdlg, IDD_STATUSBAR, pao->isStatusBar ? MF_CHECKED : MF_UNCHECKED);
      CheckDlgButton(hdlg, IDD_TOOLBAR, pao->isToolbar ? MF_CHECKED : MF_UNCHECKED);
      CheckDlgButton(hdlg, IDD_PERMSCROLLBARS, pao->permanentScrollBars ? MF_CHECKED : MF_UNCHECKED);
      SetDlgItemInt(hdlg, IDD_INDENT, pao->leftIndent, TRUE);
    }
    return TRUE;

  case WM_NOTIFY:
    if (((LPNMHDR)lParam)->code == PSN_APPLY && initialized)
    {
      pao->isStatusBar = IsDlgButtonChecked(hdlg, IDD_STATUSBAR) ? true : false;
      pao->isToolbar = IsDlgButtonChecked(hdlg, IDD_TOOLBAR) ? true : false;
      pao->permanentScrollBars = IsDlgButtonChecked(hdlg, IDD_PERMSCROLLBARS) ? true : false;
      pao->leftIndent = GetDlgItemInt(hdlg, IDD_INDENT, 0, FALSE);
      initialized = false;
      return TRUE; 
    }
  }
  return FALSE;
}