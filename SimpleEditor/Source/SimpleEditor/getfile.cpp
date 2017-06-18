#include "resources\res.hpp"

#include "notepad.hpp"
#include "getfile.hpp"

#include <commctrl.h>
#include <vector>

#pragma warning(disable: 4996)


const int ITEM_UPDIR    = 0;
const int ITEM_FILE     = 1;
const int ITEM_DOWNDIR  = 2;


const int ROOT_DIRECTORY_IN_MENU_ID = 1000;
const int DIRECTORIES_MAX_DEPTH = 128;


#ifdef _WIN32_WCE
const wchar_t ROOT_DIRECTORY[] = L"\\";
#else
const wchar_t ROOT_DIRECTORY[] = L"C:\\";
#endif


static void makeDownDir(wchar_t *pathToDir, const wchar_t *name)
{
  size_t dirLen = wcslen(pathToDir);
  size_t nameLen = wcslen(name);
  if (dirLen + nameLen > MAX_PATH - 2)  // '\' + '\0'
    return;
  wcscpy(&pathToDir[dirLen], name);
  pathToDir[dirLen + nameLen] = L'\\';
  pathToDir[dirLen + nameLen + 1] = L'\0';

}

static void makeUpDir(wchar_t *pathToDir)
{
  size_t len = wcslen(pathToDir);

  _ASSERTE(len > 0);
  if (len == 1)  // it means that path contains only '\'
    return;
  len -= 2;

  while (len > 0 && pathToDir[len] != L'\\')
    --len;
  if (pathToDir[len] == L'\\') // current directory is not root
    pathToDir[len + 1] = L'\0';
}

bool isRootDir(const wchar_t *pathToDir)
{
  size_t len = wcslen(pathToDir);

  _ASSERTE(len > 0);
  if (len == 1)  // it means that path contains only '\'
    return true;
  len -= 2;

  while (len > 0 && pathToDir[len] != L'\\')
    --len;
  if (pathToDir[len] == L'\\') // current directory is not root
    return false;
  return true;
}

static void initList(HWND hlist)
{
  RECT rc;
  GetClientRect(hlist, &rc);
  
  LV_COLUMN lvc;
  memset(&lvc, 0, sizeof(LV_COLUMN));
  lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;    // Type of mask
  lvc.pszText = L"Item";                         
  lvc.cx = rc.right - GetSystemMetrics(SM_CXVSCROLL);     
  SendMessage(hlist, LVM_INSERTCOLUMN, 0, (LPARAM)&lvc); 

  int iconSize = GetSystemMetrics(SM_CYSMICON);
  HIMAGELIST himageList = ImageList_Create(iconSize, iconSize, ILC_COLOR24, 2, 1);
  if (!himageList)
    return;


  HBITMAP hbmp = LoadBitmap(g_hAppInstance, MAKEINTRESOURCE(iconSize == 16 ? IDB_GETFILE16 : IDB_GETFILE32));
  if (!hbmp)
  {
    ImageList_Destroy(himageList);
    return;
  }

  if (ImageList_Add(himageList, hbmp, 0) == -1)
  {
    ImageList_Destroy(himageList);
    return;
  }

  ListView_SetImageList(hlist, himageList, LVSIL_SMALL);

  //SendMessage(hlist,LVM_SETEXTENDEDLISTVIEWSTYLE,
  //         0,LVS_EX_FULLROWSELECT); // Set style
}

static void initFilters(HWND hfilterList, wchar_t *filters, int nFilter)
{
  wchar_t *pch = filters;
  bool filterName = true;
  int last = 0;

  while (*pch || *(pch + 1))
  {
    if (*pch)
    {
      if (filterName)
        SendMessage(hfilterList, CB_ADDSTRING, 0, (LPARAM)pch);
      else
        SendMessage(hfilterList, CB_SETITEMDATA, last++, (LPARAM)(pch - filters));

      while (*pch)
        ++pch;
      --pch;
      filterName = !filterName;
    }
    ++pch;
  }
  SendMessage(hfilterList, CB_SETCURSEL, nFilter, 0);
}

// Function initializes coding list
static void initCodings(HWND hfilterList, LPCWSTR codingId, bool forOpen)
{
  CNotepad &Notepad = CNotepad::Instance();
  Notepad.Documents.CodingManager.FillComboboxWithCodings(hfilterList, codingId, forOpen);
}

// Function refreshes directories menu
static void refreshDirectoriesMenu(HMENU hmenu, const wchar_t *path)
{
  BOOL isItem = TRUE;
  while (isItem)
    isItem = RemoveMenu(hmenu, 1, MF_BYPOSITION);  // 1 - position of the first directory

  CheckMenuItem(hmenu, ROOT_DIRECTORY_IN_MENU_ID, MF_UNCHECKED);

  const wchar_t *currentCharacter = path, *directoryNameStart = 0;
  wchar_t buffer[MAX_PATH];
  int numberOfItems = 1;

  while (*currentCharacter)
  {
    while (*currentCharacter && *currentCharacter != L'/' && *currentCharacter != L'\\')
      currentCharacter++;

    if (directoryNameStart)
    {
      int length = currentCharacter - directoryNameStart;
      wcsncpy(buffer, directoryNameStart, length);
      buffer[length] = L'\0';

      AppendMenu(hmenu, MF_STRING | MF_ENABLED, ROOT_DIRECTORY_IN_MENU_ID + (numberOfItems++), buffer);
    }

    directoryNameStart = currentCharacter + 1;

    if (*currentCharacter)
      currentCharacter++;
  }

  CheckMenuItem(hmenu, ROOT_DIRECTORY_IN_MENU_ID + numberOfItems - 1, MF_CHECKED);

  /*const wchar_t *currentCharacter = path, *directoryNameStart = 0;
  wchar_t buffer[MAX_PATH];
  int numberOfItems = 0;

  SendMessage(hdirectoriesCombo, CB_RESETCONTENT, 0, 0);

  wcscpy(buffer, ROOT_DIRECTORY);
  SendMessage(hdirectoriesCombo, CB_ADDSTRING, 0, (LPARAM)buffer);
  numberOfItems = 1;

  while (*currentCharacter)
  {
    while (*currentCharacter && *currentCharacter != L'/' && *currentCharacter != L'\\')
      currentCharacter++;

    if (directoryNameStart)
    {
      int length = currentCharacter - directoryNameStart;
      wcsncpy(buffer, directoryNameStart, length);
      buffer[length] = L'\0';

      SendMessage(hdirectoriesCombo, CB_ADDSTRING, 0, (LPARAM)buffer);
      numberOfItems++;
    }
      
    directoryNameStart = currentCharacter + 1;

    if (*currentCharacter)
      currentCharacter++;
  }

  SendMessage(hdirectoriesCombo, CB_SETCURSEL, numberOfItems - 1, 0);*/
}

// Function creates directories menu
static HMENU createDirectoriesMenu(const wchar_t *path)
{
  HMENU hcontainerMenu = CreateMenu();
  HMENU hdirectoriesMenu = CreatePopupMenu();

  AppendMenu(hcontainerMenu, MF_POPUP | MF_ENABLED, (UINT_PTR)hdirectoriesMenu, L"Dummy");
  AppendMenu(hdirectoriesMenu, MF_STRING | MF_ENABLED, ROOT_DIRECTORY_IN_MENU_ID, ROOT_DIRECTORY);

  refreshDirectoriesMenu(hdirectoriesMenu, path);

  return hcontainerMenu;
}

// Function destroys directories menu
static void destroyDirectoriesMenu(HMENU hcontainerMenu)
{
  DestroyMenu(hcontainerMenu);
}

// updates path according to selected value in combobox
static char updatePathFromDirectoriesMenu(HMENU hdirectoriesMenu, int selectedId, wchar_t *currentPath)
{
  int number = GetMenuItemCount(hdirectoriesMenu);

  if (selectedId + 1 < ROOT_DIRECTORY_IN_MENU_ID + number)
  {
    for (int i = selectedId + 1; i < ROOT_DIRECTORY_IN_MENU_ID + number; i++)
      makeUpDir(currentPath);
    return 1;
  }

  return 0;

  /*int selected = SendMessage(hdirectoriesCombobox, CB_GETCURSEL, 0, 0);
  int number = SendMessage(hdirectoriesCombobox, CB_GETCOUNT, 0, 0);

  if (selected != CB_ERR && number != CB_ERR)
  {
    if (selected + 1 < number) 
    {
      for (int i = selected + 1; i < number; i++)
        makeUpDir(currentPath);
      return 1;
    }
  }

  return 0;*/
}


// size of buff must be MAX_PATH
static void addItemToList(HWND hlist, LV_ITEM *toInsert, int start, int end, wchar_t *buff)
{
  if (end < start)
  {
    toInsert->iItem = start;
    SendMessage(hlist, LVM_INSERTITEM, 0, (LPARAM)toInsert);
    return;
  }

  int i = (start + end) / 2;
  int compare;

  LV_ITEM lvi;
  lvi.iSubItem = 0;
  lvi.mask = LVIF_TEXT;

  while (i > start)
  {
    lvi.cchTextMax = MAX_PATH;
    lvi.pszText = buff;
    lvi.iItem = i;
    SendMessage(hlist, LVM_GETITEM, 0, (LPARAM)&lvi);
 
    compare = _wcsicmp(toInsert->pszText, lvi.pszText);
    if (compare > 0)  // string 1 > string 2
      start = i;
    else if (compare < 0)  // string 1 < string 2
      end = i;

    i = (start + end) / 2;
  }

  lvi.cchTextMax = MAX_PATH;
  lvi.pszText = buff;
  lvi.iItem = i;
  SendMessage(hlist, LVM_GETITEM, 0, (LPARAM)&lvi);
  compare = _wcsicmp(toInsert->pszText, lvi.pszText);
  if (compare > 0)
  {
    lvi.iItem = i + 1;
    if (end == i + 1)
    {
      lvi.cchTextMax = MAX_PATH;
      lvi.pszText = buff;
      SendMessage(hlist, LVM_GETITEM, 0, (LPARAM)&lvi);    
      if (_wcsicmp(toInsert->pszText, lvi.pszText) > 0)
        lvi.iItem = end + 1;
    }
  }
  toInsert->iItem = lvi.iItem;
  SendMessage(hlist, LVM_INSERTITEM, 0, (LPARAM)toInsert);
}

// function creates expression which is used for comparing file name and filter
// length(buff) == length(s)
static void createPatterns(wchar_t *s, wchar_t *buff, std::vector<wchar_t*> &patterns)
{
  wchar_t *e;
  wchar_t *cur = buff, *prev;

  while (*s)
  {
    patterns.push_back(cur);
    *cur = L'\0'; // it's empty pattern
    prev = 0;
    e = s;

    while (*e && *e != L';')
    {
      if (*e == L'*' && (!prev || *prev != L'*'))
      {
        *cur = L'*';
        prev = cur;
        ++cur;
      }
      else if (*e == L'?' && (!prev || *prev != L'*'))
      {
        *cur = L'?';
        prev = cur;
        ++cur;
      }
      else 
      {
        *cur = *e;
        prev = cur;
        ++cur;
      }
      ++e;
    }
    *cur = L'\0';
    ++cur;
    s = *e ? e + 1 : e;
  }
}

static wchar_t* prevStar(wchar_t *pattern, int i)
{
  while (i >= 0)
  {
    if (pattern[i] == L'*')
      return &pattern[i];
    --i;
  }
  return 0;
}

static bool matchPattern(wchar_t *str, std::vector<wchar_t*> &patterns)
{
  wchar_t *pat;
  wchar_t *pch;

  for (size_t i = 0; i < patterns.size(); i++)
  {
    pat = patterns[i];
    pch = str;
    if (!pat)
      continue;
    while (*pch)
    {
      if (*pat == L'*')
        ++pat;
      _ASSERTE(*pat != L'*');

      if (_wcsnicmp(pch, pat, 1) == 0 || *pat == L'?')
        ++pat;
      else if ((pat = prevStar(patterns[i], pat - patterns[i])) == 0)
        break;
      ++pch;
    }
    if (0 == *pch && (0 == *pat || (*pat == L'*' && *(pat+1) == L'\0')))
      return true;
  }
  return false;
}

static char fillListView(HWND hlist, HWND hfilterList, wchar_t *pathToDir, wchar_t *filter)
{ 
  static wchar_t fileName[MAX_PATH] = {0};
  int number = 0, numberOfDirs = 0, firstDir = 0;

  if (!hlist || !hfilterList)
    return false;

  int filterIndex = SendMessage(hfilterList, CB_GETCURSEL, 0, 0);
  if (filterIndex != CB_ERR)
    filterIndex = SendMessage(hfilterList, CB_GETITEMDATA, filterIndex, 0);

  LV_ITEM lvi;
  memset(&lvi, 0, sizeof(LV_ITEM));
  lvi.cchTextMax = MAX_PATH;
  lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
  lvi.iGroupId = I_GROUPIDNONE;

  SendMessage(hlist, LVM_DELETEALLITEMS, 0, 0);
  if (!isRootDir(pathToDir))
  {
    lvi.pszText = L"..";
    lvi.lParam = ITEM_UPDIR;
    lvi.iItem = number;
    lvi.iImage = 1;
    SendMessage(hlist, LVM_INSERTITEM, 0, (LPARAM)&lvi);
    firstDir = numberOfDirs = number = 1;
  }

  WIN32_FIND_DATA fd;
  memset(&fd, 0, sizeof(WIN32_FIND_DATA));

  wchar_t *buff;
  std::vector<wchar_t*> patterns;

  buff = new wchar_t[filterIndex != CB_ERR ? wcslen(&filter[filterIndex]) + 1 : 2];
  if (!buff)
    return false;
  createPatterns(filterIndex != CB_ERR ? &filter[filterIndex] : L"*", buff, patterns);

  size_t len = wcslen(pathToDir);
  wcscpy(&pathToDir[len], L"*");

  HANDLE hfind = FindFirstFile(pathToDir, &fd);
  if (INVALID_HANDLE_VALUE != hfind)
  {
    do 
    {
      if (0 != (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
      {
        // for desktop PC
        if (wcscmp(fd.cFileName, L"..") == 0 || wcscmp(fd.cFileName, L".") == 0)
          continue;

        lvi.pszText = fd.cFileName;
        lvi.lParam = ITEM_DOWNDIR;
        lvi.iImage = 1;

        addItemToList(hlist, &lvi, firstDir, numberOfDirs - 1, fileName);
        ++numberOfDirs;
        ++number;
      } 
      else if (matchPattern(fd.cFileName, patterns))
      {
        lvi.pszText = fd.cFileName;
        lvi.lParam = ITEM_FILE;
        lvi.iImage = 0;

        addItemToList(hlist, &lvi, numberOfDirs, number - 1, fileName);
        ++number;
      }
    } while (FindNextFile(hfind, &fd) > 0);

    FindClose(hfind);
  }

  delete[] buff;
  pathToDir[len] = L'\0';

  return true;
}

static void fillGFNAndExit(HWND hdlg, wchar_t *pathToDir, SGetFileName *gfn)
{
  // set codepage
  int retval = SendMessage(GetDlgItem(hdlg, IDD_CODING), CB_GETCURSEL, 0, 0);
  if (retval != CB_ERR)
    retval = SendMessage(GetDlgItem(hdlg, IDD_CODING), CB_GETITEMDATA, retval, 0);
  gfn->codePage = retval == CB_ERR ? CCodingManager::CODING_UNKNOWN_ID : (const wchar_t*)retval;

  // update filterindex
  retval = SendMessage(GetDlgItem(hdlg, IDD_FILTER), CB_GETCURSEL, 0, 0);
  gfn->filterIndex = retval == CB_ERR ? 0 : retval; // if an error occures, we choose the first filter

  // set bom flag
  if (gfn->saveDialog)
    gfn->BOM = IsDlgButtonChecked(hdlg, IDD_BOM) ? true : false;

  // fill path string
  size_t len = wcslen(pathToDir);
  if (gfn->pathSize < len + 1)
    EndDialog(hdlg, GETFILE_FILL_FIELDS_ERROR);
  wcsncpy(gfn->path, pathToDir, len);
  if (!GetWindowText(GetDlgItem(hdlg, IDD_NAME), &gfn->path[len], gfn->pathSize - len))
    EndDialog(hdlg, GETFILE_FILL_FIELDS_ERROR);

  EndDialog(hdlg, GETFILE_SUCCESS);
}

static UINT_PTR CALLBACK getFileDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
  static SGetFileName *gfn = 0;
  static wchar_t pathToDir[MAX_PATH + 1]; 
  static bool firstTime = true;
  static HBITMAP hdirBmp;

  static HMENU hcontainerMenu;


  switch(msg)
  {
  case WM_INITDIALOG:
    {

      gfn = reinterpret_cast<SGetFileName*>(lParam);
#ifdef _WIN32_WCE
      SHINITDLGINFO shidi;
      shidi.dwMask = SHIDIM_FLAGS;
      shidi.dwFlags = SHIDIF_DONEBUTTON | SHIDIF_SIZEDLGFULLSCREEN | SHIDIF_EMPTYMENU;
      shidi.hDlg = hdlg;
      SHInitDialog(&shidi);
#endif
      HWND hitems[9] = 
      {
        GetDlgItem(hdlg, IDOK),
        GetDlgItem(hdlg, IDCANCEL),
        GetDlgItem(hdlg, IDD_NAME),
        GetDlgItem(hdlg, IDD_FILTER),
        GetDlgItem(hdlg, IDD_CODING),
        GetDlgItem(hdlg, IDD_PATH),
        GetDlgItem(hdlg, IDD_DIRECTORIES),
        GetDlgItem(hdlg, IDD_LIST),
        GetDlgItem(hdlg, IDD_BOM)
      };
      RECT rc;
      GetClientRect(hitems[0], &rc);
      Location::SetNet(hdlg, 100, rc.bottom, 0, 4, false, true);

      SWindowPos positions[9] = 
      {
        {51, 0, 23, 1},  // ok
        {75, 0, 23, 1},  // cancel
        {2, 0, 48, 1},   // file name
        {2, 1, 35, Location::MaxY / 2},   // filter (combobox)
        {39, 1, 35, Location::MaxY / 2},   // coding (combobox)
        {2, 2, 80, 1},                     // path line
        {84, 2, 14, 1},                    // directories menu button
        {2, 3, 96, Location::MaxY - 3},    // list view
        {75, 1, 23, 1},                    // bom checkbox
      };
      Location::Arrange(hitems, positions, 9);

      // initialization

      pathToDir[MAX_PATH] = 0;
      if (gfn->initialDir && gfn->initialDir[0])
        wcsncpy(pathToDir, gfn->initialDir, min(wcslen(gfn->initialDir) + 1, MAX_PATH));
      else if (firstTime || !pathToDir[0])
        wcscpy(pathToDir, ROOT_DIRECTORY);

      initList(hitems[7]);
      initFilters(hitems[3], gfn->filter, gfn->filterIndex);
      initCodings(hitems[4], gfn->codePage, gfn->saveDialog ? false : true);
      hcontainerMenu = createDirectoriesMenu(pathToDir);

      SetWindowText(hitems[2], gfn->path);
      if (gfn->path[0] == L'\0')
        EnableWindow(hitems[0], FALSE);

      if (!gfn->saveDialog)
        ShowWindow(hitems[8], SW_HIDE);
      else
        CheckDlgButton(hdlg, IDD_BOM, gfn->BOM ? MF_CHECKED : MF_UNCHECKED);

      SetWindowText(hitems[5], pathToDir);
      if (!fillListView(hitems[7], hitems[3], pathToDir, gfn->filter))
      {
        destroyDirectoriesMenu(hcontainerMenu);
        EndDialog(hdlg, GETFILE_MEMORY_ERROR);
      }

      firstTime = false;
    }
    return TRUE;

  case WM_COMMAND:
    if (LOWORD(wParam) == IDOK && GetWindowTextLength(GetDlgItem(hdlg, IDD_NAME)))
    {
      fillGFNAndExit(hdlg, pathToDir, gfn);
      destroyDirectoriesMenu(hcontainerMenu);
      return TRUE;
    }
    else if (LOWORD(wParam) == IDD_FILTER && HIWORD(wParam) == CBN_SELCHANGE)
    {
      if (!fillListView(GetDlgItem(hdlg, IDD_LIST), (HWND)lParam, pathToDir, gfn->filter))
      {
        destroyDirectoriesMenu(hcontainerMenu);
        EndDialog(hdlg, GETFILE_MEMORY_ERROR);
      }
    }
    else if (LOWORD(wParam) == IDD_DIRECTORIES)
    {
      refreshDirectoriesMenu(GetSubMenu(hcontainerMenu, 0), pathToDir);

      RECT absoluteDialogRect;
      RECT absoluteMenuButtonRect;

      GetWindowRect(hdlg, &absoluteDialogRect);
      GetWindowRect(GetDlgItem(hdlg, IDD_DIRECTORIES), &absoluteMenuButtonRect);

      POINT pt;
      pt.x = absoluteMenuButtonRect.left - absoluteDialogRect.left;
      pt.y = absoluteMenuButtonRect.bottom - absoluteDialogRect.top;

      ClientToScreen(hdlg, &pt);
      TrackPopupMenu(GetSubMenu(hcontainerMenu, 0), TPM_LEFTALIGN, pt.x, pt.y, 0, hdlg, NULL); 
    }
    else if (LOWORD(wParam) == IDD_NAME && HIWORD(wParam) == EN_CHANGE)
    {
      if (GetWindowTextLength((HWND)lParam))
        EnableWindow(GetDlgItem(hdlg, IDOK), TRUE);
      else
        EnableWindow(GetDlgItem(hdlg, IDOK), FALSE);
    }
    else if (LOWORD(wParam) == IDCANCEL)
    {
      destroyDirectoriesMenu(hcontainerMenu);
      EndDialog(hdlg, GETFILE_CANCEL);
    }
    else if (LOWORD(wParam) >= ROOT_DIRECTORY_IN_MENU_ID && LOWORD(wParam) < ROOT_DIRECTORY_IN_MENU_ID + DIRECTORIES_MAX_DEPTH)
    {
      HMENU hdirectoriesMenu = GetSubMenu(hcontainerMenu, 0);

      if (updatePathFromDirectoriesMenu(hdirectoriesMenu, LOWORD(wParam), pathToDir))
      {
        SetWindowText(GetDlgItem(hdlg, IDD_PATH), pathToDir);
        if (!fillListView(GetDlgItem(hdlg, IDD_LIST), GetDlgItem(hdlg, IDD_FILTER), pathToDir, gfn->filter))
        {
          destroyDirectoriesMenu(hcontainerMenu);
          EndDialog(hdlg, GETFILE_MEMORY_ERROR);
        }
      }
    }
    break;

  case WM_NOTIFY: 
    if (LOWORD(wParam) == IDD_LIST)
    {
      LPNMHDR pnm = (LPNMHDR)lParam;
      if (pnm->code == NM_DBLCLK)  // double click on file or folder
      {
        int current = SendMessage(pnm->hwndFrom, LVM_GETSELECTIONMARK, 0, 0);
        if (current < 0)
          return TRUE;

        wchar_t name[MAX_PATH];
        LV_ITEM lvi;

        lvi.iSubItem = 0;
        lvi.iItem = current;
        lvi.mask = LVIF_TEXT | LVIF_PARAM;
        lvi.cchTextMax = MAX_PATH;
        lvi.pszText = name;
        SendMessage(pnm->hwndFrom, LVM_GETITEM, 0, (LPARAM)&lvi);

        if (lvi.pszText != name) // ListView behaviour could be strange
        {
          size_t len = wcslen(lvi.pszText);
          wcsncpy(name, lvi.pszText, min(MAX_PATH, len));
          name[len] = 0;
        }

        if (lvi.lParam == ITEM_DOWNDIR)
          makeDownDir(pathToDir, name);
        else if (lvi.lParam == ITEM_UPDIR)
          makeUpDir(pathToDir);
        else if (lvi.lParam == ITEM_FILE)
        {
          destroyDirectoriesMenu(hcontainerMenu);
          fillGFNAndExit(hdlg, pathToDir, gfn);
          return TRUE;
        }

        SetWindowText(GetDlgItem(hdlg, IDD_PATH), pathToDir);
        if (!fillListView(pnm->hwndFrom, GetDlgItem(hdlg, IDD_FILTER), pathToDir, gfn->filter))
        {
          destroyDirectoriesMenu(hcontainerMenu);
          EndDialog(hdlg, GETFILE_MEMORY_ERROR);
        }

        return TRUE;
      }
      else if (pnm->code == LVN_ITEMCHANGED && ((LPNMLISTVIEW)lParam)->lParam == ITEM_FILE)
      {
        wchar_t name[MAX_PATH];
        LV_ITEM lvi;

        lvi.iSubItem = 0;
        lvi.iItem = ((LPNMLISTVIEW)lParam)->iItem;
        lvi.mask = LVIF_TEXT;
        lvi.cchTextMax = MAX_PATH;
        lvi.pszText = name;
        SendMessage(pnm->hwndFrom, LVM_GETITEM, 0, (LPARAM)&lvi);
        SetWindowText(GetDlgItem(hdlg, IDD_NAME), name);

        return TRUE;
      }
      else if (pnm->code == NM_RETURN)
      {
        SendMessage(hdlg, WM_COMMAND, IDOK, 0);
        return TRUE;
      }
    }
  }
  return FALSE;
}


extern int GetFileName(SGetFileName *gfn)
{
  _ASSERTE(gfn);

  return DialogBoxParam(g_hResInstance, MAKEINTRESOURCE(DLG_GETFILE), gfn->hwndOwner, 
                        reinterpret_cast<DLGPROC>(getFileDlgProc),
                        reinterpret_cast<LPARAM>(gfn));
}