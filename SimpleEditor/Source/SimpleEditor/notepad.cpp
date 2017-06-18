#include "resources\res.hpp"
#include "base\files.h"
#include "notepad.hpp"

#include <cwchar>
#include <ctime>
#include <string>
#include <list>


// Function returns <0 if an error occured, 0 if no error and 
// >0 if notepad is already running
bool CNotepad::InitNotepad(HINSTANCE hInstance, wchar_t *lpCmdLine)
{
  int err = NOTEPAD_NO_ERROR;
  //g_resources = (wchar_t**)lpCmdLine; // for protection

#ifdef UNDER_CE
  SHInitExtraControls();
#endif

  // At first we should try to find another instance of the notepad and,
  // if success, send to it our command line
  HWND hwndExist = FindWindow(ClassName, NULL);
  if (0 != hwndExist)
  {
    int argc;
    wchar_t **argv;

    argv = CommandLineToArgvW(lpCmdLine, &argc);
    SetForegroundWindow((HWND)((ULONG) hwndExist /*| 0x00000001*/));

#ifndef _WIN32_WCE
    if (argc > 1)
    {
      COPYDATASTRUCT cds;
      cds.dwData = 0;
      cds.cbData = (wcslen(argv[1]) + 1) * sizeof(wchar_t);
      cds.lpData = (PVOID)argv[1];
      SendMessage(hwndExist, WM_COPYDATA, 0, (LPARAM)&cds);
    }
#else
    if (argc > 0)
    {
      COPYDATASTRUCT cds;
      cds.dwData = 0;
      _ASSERTE(argc > 1);
      cds.cbData = (wcslen(argv[0]) + 1) * sizeof(wchar_t);
      cds.lpData = (PVOID)argv[0];
      SendMessage(hwndExist, WM_COPYDATA, 0, (LPARAM)&cds);
    }
#endif

    if (argv)
      GlobalFree(argv);
    return false;
  } 
  g_hAppInstance = hInstance;

  if (!setExecPath())
  {
    CError::SetError(NOTEPAD_INIT_ERROR);
    return false;
  }

  // CreateIC is not available on WinCE
  HDC hdc = CreateDC(L"DISPLAY", 0, 0, 0);
  if (!hdc)
  {
    CError::SetError(NOTEPAD_CREATE_RESOURCE_ERROR);
    return false;
  }
  g_LogPixelsY = GetDeviceCaps(hdc, LOGPIXELSY);
  DeleteDC(hdc);
  Documents.SetProgressFunctions();

  // Read options 
  if (!Options.ReadOptions(L"properties.ini"))
  {
    Interface.Dialogs.ShowError(0, CError::GetError().message);
    CError::SetError(NOTEPAD_NO_ERROR);
  }
  if (!Options.ReadOptions(L"external.ini"))
  {
    Interface.Dialogs.ShowError(0, CError::GetError().message);
    CError::SetError(NOTEPAD_NO_ERROR);
  }
  if (!Options.ReadOptions(L"paths.ini"))
  {
    Interface.Dialogs.ShowError(0, CError::GetError().message);
    CError::SetError(NOTEPAD_NO_ERROR);
  }
  if (!Options.InitOptions())
  {
    Interface.Dialogs.ShowError(0, CError::GetError().message);
    CError::SetError(NOTEPAD_NO_ERROR);
  }
  Interface.Controlbar.ReadButtons();

  // Load language
  if (!LoadLangModule())
  {
    CError::SetError(NOTEPAD_INIT_ERROR);
    return false;
  }

  //g_resources = 0; // g_resources must be zero before calling PrepareResources()
  /*if ((err = PrepareLangResources()) != DES_NO_ERROR) // call PrepareResources()
  {
    CError::SetError(CError::TranslateProtect(err));
    return false;
  }*/

  // Register main window class
#if !defined(_WIN32_WCE)
  WNDCLASSEX wc;
#else
  WNDCLASS wc;
#endif

  HBRUSH hbr = CreateSolidBrush(Options.GetProfile().MainColor);
	ZeroMemory(&wc, sizeof(wc));
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = (WNDPROC)windowProc;
  wc.hInstance = g_hAppInstance; 
  wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(APPLICATION_ICON_0));
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = hbr;//(HBRUSH)GetStockObject(WHITE_BRUSH);
  wc.lpszClassName = (LPWSTR)ClassName;

#ifndef _WIN32_WCE
  wc.cbSize = sizeof(wc);

  if (!RegisterClassEx(&wc))
  {
    CError::SetError(NOTEPAD_REGISTER_ERROR);
    return false;
  }
  //uFindReplaceMsg = RegisterWindowMessage(FINDMSGSTRING);
#else

  if (!RegisterClass(&wc))
  {
    CError::SetError(NOTEPAD_REGISTER_ERROR);
    return false;
  }
  //uFindReplaceMsg = FIND_REPLACE_MSG;
#endif


  // Register Simple Edit Control
  if (SEAPIRegister(g_hAppInstance, Options.GetProfile().EditorColor) != SEDIT_NO_ERROR)
  {
    CError::SetError(NOTEPAD_REGISTER_ERROR);
    return false;
  }
  SEAPIAddBreakSyms(Options.BreakSyms().c_str(), Options.BreakSyms().size());
  //SEditSetPopupMenuItems(POPUP_CUT, POPUP_COPY, POPUP_PASTE, POPUP_DELETE, POPUP_SELECT_ALL);

  // Register common controls
  INITCOMMONCONTROLSEX icc;
  icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
  icc.dwICC = ICC_PROGRESS_CLASS;
  if (!InitCommonControlsEx(&icc))
  {
    CError::SetError(NOTEPAD_INIT_COMMCTRL_ERROR);
    return false;
  }

  // Register window class for tabs
  Interface.Tabs.SetColors(Options.GetProfile().TabColors, SET_ALLCOLORS);
  if (!Interface.Tabs.Init())
  {
    CError::SetError(NOTEPAD_REGISTER_ERROR);
    return false;
  }
  Interface.Tabs.SetFont(Options.GetProfile().htabFont);

  Interface.Dialogs.SetFileFilters(Options.TypeStr(), Options.TypeStrSize(), 
                                   Options.OpenFileFilter(), Options.SaveFileFilter());
  Interface.Dialogs.SetDialogsPath(Options.DialogPath());
 
  // Create window
  CreateWindow(ClassName, 
               AppName, 
               NOTEPAD_WINDOW_STYLE, 
               CW_USEDEFAULT, CW_USEDEFAULT,
               CW_USEDEFAULT, CW_USEDEFAULT,
               NULL, 
               NULL,
               g_hAppInstance,
               NULL);
  if (0 == Interface.GetHWND())   // hwnd was initialized in WM_CREATE
    return false;

#ifndef _WIN32_WCE
  if (!Interface.Controlbar.CreateMainMenu(Interface.GetHWND()))
  {
    Interface.Dialogs.ShowError(Interface.GetHWND(), CError::GetError().message);
    return false;
  }
#endif

  if (Options.IsToolbar() && !Interface.Controlbar.CreateToolbar(Interface.GetHWND()))
  {
    Interface.Dialogs.ShowError(Interface.GetHWND(), CError::GetError().message);
    CError::SetError(NOTEPAD_NO_ERROR);
  }

#ifdef _WIN32_WCE
  // on pocket pc toolbar == main menu, so if toolbar was created main menu already exists
  // and nothing will be created
  if (!Interface.Controlbar.CreateMainMenu(Interface.GetHWND()) && !Interface.Controlbar.IsToolbar())
  {
    Interface.Dialogs.ShowError(Interface.GetHWND(), CError::GetError().message);
    return false;
  }
#endif

  if (Options.MinForVisibleTabs() >= 0) // if < 0 we should turn off the tab's window
  {
    if (!Interface.Tabs.Create())
    {
      Interface.Dialogs.ShowError(Interface.GetHWND(), CError::GetError().message);
      CError::SetError(NOTEPAD_NO_ERROR);
    }
  }

  if (Options.IsStatusBar())
  {
    if (!Interface.StatusBar.Create(Interface.GetHWND()))
    {
      Interface.Dialogs.ShowError(Interface.GetHWND(), CError::GetError().message);
      CError::SetError(NOTEPAD_NO_ERROR);
    }
  }

#ifdef _WIN32_WCE
  SHDoneButton(Interface.GetHWND(), SHDB_SHOW);
#endif

  // Load plugins
  if (!PluginManager.Init(std::wstring(g_execPath) + std::wstring(FILE_SEPARATOR) + std::wstring(L"plugins")))
  {
    Interface.Dialogs.ShowError(Interface.GetHWND(), CError::GetError().message);
    CError::SetError(NOTEPAD_NO_ERROR);
  }

  // resize main window to arrange all windows
  RECT rc;
  GetClientRect(Interface.GetHWND(), &rc);
  Interface.Resize(rc.right, rc.bottom);

#ifdef UNDER_CE
  if (Options.IsFullScreen())
    Interface.SwitchFullScreen(Options.IsFullScreen());
#endif

  // load recent and session
  Recent.SetMaxRecent(Options.MaxRecent());
  if (Options.SaveSession())
  {
    std::vector<SSession> session;
    int active;

    bool readed = Recent.ReadRecent(session, active);
    if (!readed && CError::GetError().code != NOTEPAD_NO_ERROR)
    {
      Interface.Dialogs.ShowError(Interface.GetHWND(), CError::GetError().message);
      CError::SetError(NOTEPAD_NO_ERROR);
    }
    else if (readed)
    {
      if (!Recent.LoadSession(session, active))
      {
        Interface.Dialogs.ShowError(Interface.GetHWND(), CError::GetError().message);
        CError::SetError(NOTEPAD_NO_ERROR);
      }
    }
    Recent.ClearSession(session);
  }
  else
  {
    if (!Recent.ReadRecent() && CError::GetError().code != NOTEPAD_NO_ERROR)
    {
      Interface.Dialogs.ShowError(Interface.GetHWND(), CError::GetError().message);
      CError::SetError(NOTEPAD_NO_ERROR);
    }
  }

  // open file if the command line is not empty
  int argc;
  wchar_t **argv = CommandLineToArgvW(lpCmdLine, &argc);

#ifndef _WIN32_WCE
  if (argc > 1)
  {
    std::wstring path = argv[1];
    if (!Documents.Open(path, CCodingManager::CODING_UNKNOWN_ID, OPEN))
    {
      Interface.Dialogs.ShowError(Interface.GetHWND(), CError::GetError().message);
      CError::SetError(NOTEPAD_NO_ERROR);
    }
  }
#else
  if (argc > 0)
  {
    std::wstring path = argv[0];
    if (!Documents.Open(path, CCodingManager::CODING_UNKNOWN_ID, OPEN))
    {
      Interface.Dialogs.ShowError(Interface.GetHWND(), CError::GetError().message);
      CError::SetError(NOTEPAD_NO_ERROR);
    }
  }
#endif
  else if (Documents.GetDocsNumber() == 0)
  {
    std::wstring name = Documents.AutoName();
    if (!Documents.Create(name))
      return false;
    Documents.SetActive(0);
  }

  if (argv)
    GlobalFree(argv);

  return true;
}



// Function returns reference to the singleton object
CNotepad& CNotepad::Instance()
{
  static CNotepad Notepad;
  return Notepad;
}

// Function loads messages from resource module
bool CNotepad::LoadLangModule()
{
  bool isError = false;
  std::wstring errorMessage;

  g_hResInstance = g_hAppInstance;

  if (Options.LangLib().compare(DEFAULT_LANGUAGE_PATH) != 0)
  {
    bool isLangLibrary = true;
    bool isVersionCorrect = true;

    if ((g_hResInstance = LoadLibrary(Options.LangLib().c_str())) == 0)
      isLangLibrary = false;
    else if (!LoadString(g_hResInstance, IDS_VERSION, LIBRARY_VERSION, MSG_MAX_LENGTH))
      isLangLibrary = false;
    else if (wcscmp(LIBRARY_VERSION, RESOURCES_VERSION) != 0)
      isVersionCorrect = false;

    if (!isLangLibrary || !isVersionCorrect) 
    {
      isError = true;
      CError::GetErrorMessage(errorMessage, !isLangLibrary ? NOTEPAD_CORRUPTED_LANG_DLL : NOTEPAD_LANG_DLL_VERSION_ERROR, Options.LangLib().c_str());

      g_hResInstance = g_hAppInstance;
      Options.SetLangLib(DEFAULT_LANGUAGE_PATH);
    }
  } 

  if (!LoadString(g_hResInstance, IDS_ERROR_CAPTION,      ERROR_CAPTION,       MSG_MAX_LENGTH))
    return false;
  if (!LoadString(g_hResInstance, IDS_WARNING_CAPTION,    WARNING_CAPTION,     MSG_MAX_LENGTH))
    return false;
  if (!LoadString(g_hResInstance, IDS_INFO_CAPTION,       INFO_CAPTION,        MSG_MAX_LENGTH))
    return false;
  if (!LoadString(g_hResInstance, IDS_DOC_FILE_NOT_SAVED, DOC_FILE_NOT_SAVED,  MSG_MAX_LENGTH))
    return false;
  if (!LoadString(g_hResInstance, IDS_DOC_REWRITE_FILE,   DOC_REWRITE_FILE,    MSG_MAX_LENGTH))
    return false;
  if (!LoadString(g_hResInstance, IDS_DOC_REPLACED,       DOC_REPLACED,        MSG_MAX_LENGTH))
    return false;
  if (!LoadString(g_hResInstance, IDS_DOC_TEXT_NOT_FOUND, DOC_TEXT_NOT_FOUND,  MSG_MAX_LENGTH))
    return false;
  if (!LoadString(g_hResInstance, IDS_NOTEPAD_FORCE_EXIT, NOTEPAD_FORCE_EXIT,  MSG_MAX_LENGTH))
    return false;
  if (!LoadString(g_hResInstance, IDS_GOTOLINE_MESSAGE,   GOTOLINE_MESSAGE,    MSG_MAX_LENGTH))
    return false;

  if (!LoadString(g_hResInstance, IDS_MENU_EMPTY,         MENU_EMPTY,        MSG_MAX_LENGTH))
    return false;

  if (!LoadString(g_hResInstance, IDS_LANGUAGE_NAME, g_languageName,   MSG_MAX_LENGTH))
    return false;

  if (!LoadString(g_hResInstance, IDS_LANGUAGE_ID, g_languageId, MSG_MAX_LENGTH))
    return false;

  if (isError)
    Interface.Dialogs.ShowError(0, errorMessage);

  return true;
}

bool CNotepad::setExecPath()
{
  int len = GetModuleFileName(NULL, g_execPath, PATH_SIZE);

  if (!len)
    return false;

  wchar_t *c = &g_execPath[len];
  while (*c != L'\\')
    --c;
  *c = 0;
  return true;
}

// Function saves all documents if needed and send exit message to the main window
void CNotepad::OnCmdExit()
{
  if (!Documents.SaveAllDocuments(true))
    return;  
  if (!Documents.WaitAll(NPD_EXIT)) // an error occured, we should offer user to force exit
  {
    Interface.Dialogs.ShowError(Interface.GetHWND(), CError::GetError().message);
    CError::SetError(NOTEPAD_NO_ERROR);
    int answer = Interface.Dialogs.MsgBox(Interface.GetHWND(), NOTEPAD_FORCE_EXIT, WARNING_CAPTION, MB_YESNO);
    if (answer == IDYES)
      SendMessage(Interface.GetHWND(), NPD_EXIT, 0, 0);
  }
  EnableWindow(Interface.GetHWND(), FALSE); // restrict all input
}

// Function saves session (if needed) and destroys all documents
void CNotepad::OnExit()
{
  int number = Documents.GetDocsNumber();

  if (!SaveRecentAndSession())
  {
    Interface.Dialogs.ShowError(Interface.GetHWND(), CError::GetError().message);
    CError::SetError(NOTEPAD_NO_ERROR);
  }

  // save options
  if (!Options.SaveOptions())
  {
    Interface.Dialogs.ShowError(0, CError::GetError().message);
    CError::SetError(NOTEPAD_NO_ERROR);
  }

  EnableWindow(Interface.GetHWND(), TRUE);
  for (int i = number - 1; i >= 0; i--)
  {
    Documents.SetModified(i, false);
    Documents.Close(i);
  }
  Interface.Tabs.Destroy();
  Options.FinalizeOptions();
  Recent.ClearRecent();
  DestroyWindow(Interface.GetHWND());

  //FreeResources();  // for protection
  if (g_hAppInstance != g_hResInstance)
    FreeLibrary(g_hResInstance);
}

// Functions responses on ok butoon
void CNotepad::OnOKButton()
{
  char action = Options.OkButton();
  switch (action)
  {
  case OK_BUTTON_EXIT:
    OnCmdExit();
    break;

  case OK_BUTTON_MINIMIZE:
    ShowWindow(Interface.GetHWND(), SW_MINIMIZE);
    break;

  case OK_BUTTON_CLOSE:
    if (Documents.GetDocsNumber() > 0)
      Documents.Close(Documents.Current());
    break;

  case OK_BUTTON_SMARTCLOSE:
    if (Documents.GetDocsNumber() > 1)
      Documents.Close(Documents.Current());
    else if (Options.CreateEmptyIfNoDocuments())
    {
      if (!Documents.IsBusy(Documents.Current()) && !Documents.IsModified(Documents.Current()) && 
           Documents.GetPath(Documents.Current()).empty())
        OnCmdExit();
      else
        Documents.Close(Documents.Current());
    }
    else
    {
      if (Documents.GetDocsNumber() == 0)
        OnCmdExit();
      else
        Documents.Close(Documents.Current());
    }
    break;
  }
}

// Function resizes interface when window becomes active
void CNotepad::OnActivate()
{
#ifdef _WIN32_WCE
  SWindowPos pos;
  Interface.CalcInterfaceCoords(pos, Options.ResponseOnSIP());
  Interface.Resize(pos.width, pos.height);
  Interface.SwitchFullScreen(Interface.IsFullScreen(), false);
#endif

  Documents.SetFocus();
}

bool CNotepad::SaveRecentAndSession()
{
  bool result;

  // save recent and session
  if (Options.SaveSession())
  {
    std::vector<SSession> session;
    int active;

    result = Recent.GetSession(session, active);
    if (result)
      result = Recent.SaveRecent(session, active);

    Recent.ClearSession(session);
  }
  else 
    result = Recent.SaveRecent();

  return result;
}

// Function modifies file menu
void CNotepad::ModifyFileMenu(HMENU hmenu)
{
  if (Documents.GetDocsNumber() > 0)
  {
    EnableMenuItem(hmenu, CMD_SAVEDOC, MF_ENABLED);
    EnableMenuItem(hmenu, CMD_SAVEASDOC, MF_ENABLED);
    EnableMenuItem(hmenu, CMD_CLOSEDOC, MF_ENABLED);

    if (Interface.Controlbar.IsQInsertMenu())
      EnableMenuItem(hmenu, IDM_QINSERT_M, MF_ENABLED);
    else
      EnableMenuItem(hmenu, IDM_QINSERT_M, MF_GRAYED);
  }
  else
  {
    EnableMenuItem(hmenu, CMD_SAVEDOC, MF_GRAYED);
    EnableMenuItem(hmenu, CMD_SAVEASDOC, MF_GRAYED);
    EnableMenuItem(hmenu, CMD_CLOSEDOC,MF_GRAYED);
    EnableMenuItem(hmenu, IDM_QINSERT_M, MF_GRAYED);
  }
}


// Function modifies documents menu before it will be shown
void CNotepad::ModifyDocMenu(HMENU hmenu)
{
  BOOL isItem = TRUE;
  while (isItem)
    isItem = RemoveMenu(hmenu, 3, MF_BYPOSITION);  // 3 - position of the first document

  for (int i = 0; i < Documents.GetDocsNumber(); i++)
  {
    if (AppendMenu(hmenu, MF_STRING | MF_ENABLED, IDM_FIRST_DOCUMENT + i, Documents.GetName(i).c_str()) == 0)
    {
      int error = GetLastError();  
      std::wstring mess;
      CError::GetSysErrorMessage(error, mess);
      LOG_ENTRY(L"Cannot append item '%s'! Error (%d): %s", Documents.GetName(i).c_str(), error, mess.c_str());
    }
  }

  CheckMenuItem(hmenu, IDM_FIRST_DOCUMENT + Documents.Current(), MF_CHECKED);
}

void CNotepad::ModifyPluginsMenu(HMENU hmenu)
{
  BOOL isItem = TRUE;
  while (isItem)
    isItem = RemoveMenu(hmenu, 0, MF_BYPOSITION);

  const std::list<InternalPlugin*> &plugins = PluginManager.ListPlugins();

  LOG_ENTRY(L"Modifying plugins menu... (%d)\n", plugins.size());

  bool isPluginWithMenu = false;

  if (plugins.size() > 0)
  {
    for (std::list<InternalPlugin*>::const_iterator iter = plugins.begin(); iter != plugins.end(); iter++)
    {
      HMENU hpluginMenu = (*iter)->GetPlugin()->GetMenuHandle((*iter)->GetPluginMenuFirstId());

//      std::wostringstream stringBuilder;
//      stringBuilder << L"Plugin " << (*iter)->GetPlugin()->GetName() << (hpluginMenu ? L" has menu" : L" doesn't have menu");
//      MessageBox(Interface.GetHWND(), stringBuilder.str().c_str(), L"Debug", MB_OK);

      LOG_ENTRY(L"Plugin '%s' %s\n", (*iter)->GetPlugin()->GetName(), hpluginMenu ? L"has menu" : L"doesn't have menu");

      if (hpluginMenu != 0)
      {
        isPluginWithMenu = true;
        if (!AppendMenu(hmenu, MF_POPUP | MF_ENABLED, (UINT_PTR)hpluginMenu, (*iter)->GetPlugin()->GetName()))
        {
          int error = GetLastError();
          std::wstring message;
          CError::GetSysErrorMessage(error, message);
          LOG_ENTRY(L"Cannot append item '%s' to menu! Error (%d): %s", (*iter)->GetPlugin()->GetName(), error, message.c_str());
        }
      }
    }
  }
  
  if (!isPluginWithMenu)
    AppendMenu(hmenu, MF_STRING | MF_ENABLED, IDM_FIRST_PLUGIN_COMMAND, MENU_EMPTY);
}

// Function modifies edit menu before it will be shown
void CNotepad::ModifyEditMenu(HMENU hmenu)
{
  EnableMenuItem(hmenu, CMD_COPY, Documents.IsSelection() ? MF_ENABLED : MF_GRAYED);
  EnableMenuItem(hmenu, CMD_CUT, Documents.IsSelection() ? MF_ENABLED : MF_GRAYED);
  EnableMenuItem(hmenu, CMD_UNDO, Documents.CanUndo() ? MF_ENABLED : MF_GRAYED);
  EnableMenuItem(hmenu, CMD_REDO, Documents.CanRedo() ? MF_ENABLED : MF_GRAYED);
  EnableMenuItem(hmenu, CMD_PASTE, Documents.CanPaste() ? MF_ENABLED : MF_GRAYED);
  CheckMenuItem(hmenu, IDM_REPLACE, Interface.ReplaceBar.IsReplaceBar() ? MF_CHECKED : MF_UNCHECKED);

  if (!Interface.ReplaceBar.IsReplaceBar())
    CheckMenuItem(hmenu, IDM_FIND, Interface.FindBar.IsFindBar() ? MF_CHECKED : MF_UNCHECKED);
  else
    CheckMenuItem(hmenu, IDM_FIND, MF_UNCHECKED); 
}

// Function modifies editor's pop-up menu before it will be shown
void CNotepad::ModifyEditorPopupMenu(HMENU hmenu)
{
  EnableMenuItem(hmenu, CMD_COPY, Documents.IsSelection() ? MF_ENABLED : MF_GRAYED);
  EnableMenuItem(hmenu, CMD_CUT, Documents.IsSelection() ? MF_ENABLED : MF_GRAYED);
  EnableMenuItem(hmenu, CMD_UNDO, Documents.CanUndo() ? MF_ENABLED : MF_GRAYED);
  EnableMenuItem(hmenu, CMD_REDO, Documents.CanRedo() ? MF_ENABLED : MF_GRAYED);
  EnableMenuItem(hmenu, CMD_PASTE, Documents.CanPaste() ? MF_ENABLED : MF_GRAYED);
}

// Function modifies "view" menu before it will be shown
void CNotepad::ModifyViewMenu(HMENU hmenu)
{
  if (!Documents.GetDocsNumber())
  {
    CheckMenuItem(hmenu, CMD_WORDWRAP, MF_UNCHECKED);
    CheckMenuItem(hmenu, CMD_SMARTINPUT, MF_UNCHECKED);
    CheckMenuItem(hmenu, CMD_DRAGMODE, MF_UNCHECKED);
    CheckMenuItem(hmenu, CMD_SCROLLBYLINE, MF_UNCHECKED);
    EnableMenuItem(hmenu, CMD_WORDWRAP, MF_GRAYED);
    EnableMenuItem(hmenu, CMD_SMARTINPUT, MF_GRAYED);
    EnableMenuItem(hmenu, CMD_DRAGMODE, MF_GRAYED);
    EnableMenuItem(hmenu, CMD_SCROLLBYLINE, MF_GRAYED);
  }
  else
  {
    EnableMenuItem(hmenu, CMD_WORDWRAP, MF_ENABLED);
    EnableMenuItem(hmenu, CMD_SMARTINPUT, MF_ENABLED);
    EnableMenuItem(hmenu, CMD_DRAGMODE, MF_ENABLED);
    EnableMenuItem(hmenu, CMD_SCROLLBYLINE, MF_ENABLED);
    CheckMenuItem(hmenu, CMD_WORDWRAP, Documents.IsWordWrap(Documents.Current()) ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hmenu, CMD_SMARTINPUT, Documents.IsSmartInput(Documents.Current()) ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hmenu, CMD_DRAGMODE, Documents.IsDragMode(Documents.Current()) ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hmenu, CMD_SCROLLBYLINE, Documents.IsScrollByLine(Documents.Current()) ? MF_CHECKED : MF_UNCHECKED);
  }
  CheckMenuItem(hmenu, CMD_FULLSCREEN, Interface.IsFullScreen() ? MF_CHECKED : MF_UNCHECKED);
}

// Function modifies "recent" menu
void CNotepad::ModifyRecentMenu(HMENU hmenu)
{
  int number = Recent.NumberOfRecent();

  int isItem = 1;
  while (isItem)
    isItem = RemoveMenu(hmenu, 2, MF_BYPOSITION);

  const wchar_t *path;
  for (int i = 0; i < number; ++i)
  {
    path = Recent.GetRecent(i);
    _ASSERTE(path != 0);
    AppendMenu(hmenu, MF_STRING | MF_ENABLED, IDM_FIRST_RECENT + i, &path[GetFileNameIndice(path)]);
  }

  if (number == 0)
    AppendMenu(hmenu, MF_STRING | MF_GRAYED, IDM_FIRST_RECENT, MENU_EMPTY);
}

// Function modifies "reopen" menu
void CNotepad::ModifyReopenMenu(HMENU hmenu)
{
  int isItem = 1;
  while (isItem)
    isItem = RemoveMenu(hmenu, 1, MF_BYPOSITION);

  bool areDocuments = Documents.GetDocsNumber() > 0; 
  bool isCurrentOpened = areDocuments ? !Documents.GetPath(Documents.Current()).empty() : false;
  LPCWSTR currentCodingId = isCurrentOpened ? Documents.GetCoding(Documents.Current()) : 0;

  const std::vector<Encoder*> &encoders = Documents.CodingManager.GetEncoders();

  LOG_ENTRY(L"Modifying reopen menu... (%d)\n", encoders.size());

  Encoder *defaultEncoder = Documents.CodingManager.GetEncoderById(Options.DefaultCP().c_str());
  const wchar_t *defaultCoding = (defaultEncoder != 0) ? defaultEncoder->GetCodingId() : CCodingManager::CODING_DEFAULT_ID;

  int menuItemId = IDM_FIRST_PLUGIN_CODING;
  for (std::vector<Encoder*>::const_iterator iter = encoders.begin(); iter != encoders.end(); iter++)
  {
    int retval = 1;

    if (areDocuments)
    {
      if (isCurrentOpened)
      {
        if (currentCodingId && wcscmp(currentCodingId, (*iter)->GetCodingId()) == 0)
          retval = AppendMenu(hmenu, MF_STRING | MF_ENABLED | MF_CHECKED, menuItemId++, (*iter)->GetCodingName());
        else
          retval = AppendMenu(hmenu, MF_STRING | MF_ENABLED, menuItemId++, (*iter)->GetCodingName());
      }
      else
      {
        if (wcscmp(defaultCoding, (*iter)->GetCodingId()) == 0)
          retval = AppendMenu(hmenu, MF_STRING | MF_GRAYED | MF_CHECKED, menuItemId++, (*iter)->GetCodingName());
        else
          retval = AppendMenu(hmenu, MF_STRING | MF_GRAYED, menuItemId++, (*iter)->GetCodingName());
      }
    }
    else 
      retval = AppendMenu(hmenu, MF_STRING | MF_GRAYED, menuItemId++, (*iter)->GetCodingName());   

    if (!retval)
    {
      int error = GetLastError();      
      std::wstring message;
      CError::GetSysErrorMessage(error, message);
      LOG_ENTRY(L"Cannot append item '%s' to menu! Error (%d): %s", (*iter)->GetCodingName(), error, message.c_str());
    }
  }

  EnableMenuItem(hmenu, CMD_REOPEN_AUTO, isCurrentOpened ? MF_ENABLED : MF_GRAYED);
}

void CNotepad::ClearPluginsMenu(HMENU hmenu)
{
  BOOL isItem = TRUE;
  while (isItem)
    isItem = RemoveMenu(hmenu, 0, MF_BYPOSITION);
  AppendMenu(hmenu, MF_STRING | MF_ENABLED, IDM_FIRST_PLUGIN_COMMAND, MENU_EMPTY);
}


// Window procedure
LRESULT CALLBACK CNotepad::windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
  case WM_CREATE:
    {
      CNotepad &Notepad = CNotepad::Instance();
      Notepad.Interface.SetHWND(hwnd);
    }
    break;

  case WM_ACTIVATE:
    if (LOWORD(wParam) != WA_INACTIVE)
    {
      CNotepad &Notepad = CNotepad::Instance();
      Notepad.OnActivate();
    }
    break;

  case WM_SIZE:
    if (wParam != SIZE_MINIMIZED)
    {
      CNotepad &Notepad = CNotepad::Instance();
      SWindowPos coords;
      Notepad.Interface.CalcInterfaceCoords(coords, Notepad.Options.ResponseOnSIP());
      Notepad.Interface.Resize(coords.width, coords.height, false);
    }
    break;

  case WM_COMMAND:
    if (LOWORD(wParam) >= IDM_FIRST_DOCUMENT && LOWORD(wParam) < IDM_FIRST_DOCUMENT + MAX_DOCUMENTS)
    {
      // change current document
      CNotepad &Notepad = CNotepad::Instance();
      Notepad.Documents.SetActive(LOWORD(wParam) - IDM_FIRST_DOCUMENT);
    }
    else if (LOWORD(wParam) >= IDM_FIRST_RECENT && LOWORD(wParam) < IDM_FIRST_RECENT + MAX_RECENT) 
    {
      // open recent document
      CNotepad &Notepad = CNotepad::Instance();
      std::wstring path = Notepad.Recent.GetRecent(LOWORD(wParam) - IDM_FIRST_RECENT);
      if (!path.empty())
      {
        if (!Notepad.Documents.Open(path, CCodingManager::CODING_UNKNOWN_ID, RECENT))
        {
          Notepad.Interface.Dialogs.ShowError(hwnd, CError::GetError().message);
          CError::SetError(NOTEPAD_NO_ERROR);
        }
      }
    }
    else if (LOWORD(wParam) >= IDM_FIRST_PLUGIN_COMMAND && LOWORD(wParam) < IDM_FIRST_PLUGIN_CODING) 
    {
      // execute plugin's command
      CNotepad &Notepad = CNotepad::Instance();
      Notepad.PluginManager.ExecuteMenuCommand(LOWORD(wParam));
    }
    else if (LOWORD(wParam) >= IDM_FIRST_PLUGIN_CODING && LOWORD(wParam) < QINSERT_FIRST_ID) 
    {
      // reopen current document using selected coding
      CNotepad &Notepad = CNotepad::Instance();
      if (Notepad.Documents.GetDocsNumber() > 0 && !Notepad.Documents.GetPath(Notepad.Documents.Current()).empty())
      {
        int codingIndex = LOWORD(wParam) - IDM_FIRST_PLUGIN_CODING;
        const std::vector<Encoder*> &encoders = Notepad.Documents.CodingManager.GetEncoders();
        if (encoders.size() > codingIndex) // always is true under normal circumstances
        {
          Encoder *encoder = encoders[codingIndex];
          if (!Notepad.Documents.Open(Notepad.Documents.GetPath(Notepad.Documents.Current()), encoder->GetCodingId(), REOPEN, true, true, false))
          {
            Notepad.Interface.Dialogs.ShowError(hwnd, CError::GetError().message);
            CError::SetError(NOTEPAD_NO_ERROR);
          }
        }
      }
    }
    else if (LOWORD(wParam) >= QINSERT_FIRST_ID) 
    { 
      // paste from quick insert
      CNotepad &Notepad = CNotepad::Instance();
      if (Notepad.Documents.GetDocsNumber() > 0 && Notepad.Interface.Controlbar.IsQInsertMenu() &&
        LOWORD(wParam) <= Notepad.Interface.Controlbar.GetLastQInsertID())
      {
        unsigned int back;
        char pasteSelection;
        wchar_t *buff = 0;
        wchar_t *str = Notepad.Interface.Controlbar.GetInsertString(LOWORD(wParam), &back, &pasteSelection);

        if (pasteSelection)
          buff = Notepad.Documents.GetSelection();

        Notepad.Documents.StartUndoGroup();
        Notepad.Documents.Paste(str);
        Notepad.Documents.MoveCaret(Notepad.Documents.Current(), back, true);
        if (buff)
        {
          Notepad.Documents.Paste(buff);
          delete[] buff;
        }
        Notepad.Documents.EndUndoGroup();
      }
    }
    else
    {
      CNotepad &Notepad = CNotepad::Instance();

      switch (LOWORD(wParam))
      {
      case CMD_NEWDOC:
        {
          std::wstring name = Notepad.Documents.AutoName();
          if (Notepad.Documents.Create(name))
            Notepad.Documents.SetActive(Notepad.Documents.GetDocsNumber() - 1);
          else
          {
            Notepad.Interface.Dialogs.ShowError(hwnd, CError::GetError().message);
            CError::SetError(NOTEPAD_NO_ERROR);
          }
        }
        break;

      case CMD_CLOSEDOC:
        if (Notepad.Documents.GetDocsNumber() > 0)
          Notepad.Documents.Close(Notepad.Documents.Current());
        break;

      case CMD_OPENDOC:
        {
          std::wstring path;
          LPCWSTR cp = CCodingManager::CODING_UNKNOWN_ID;  // auto detection by default
          if (Notepad.Interface.Dialogs.OpenFileDialog(hwnd, path, cp, Notepad.Options.OwnDialogs()))
          {
            if (!Notepad.Documents.Open(path, cp, OPEN))
            {
              Notepad.Interface.Dialogs.ShowError(hwnd, CError::GetError().message);
              CError::SetError(NOTEPAD_NO_ERROR);
            }
          }
        }
        break;

      case CMD_SAVEDOC:
        if (Notepad.Documents.GetDocsNumber() > 0)
        {
          std::wstring path = Notepad.Documents.GetPath(Notepad.Documents.Current());
          LPCWSTR cp = Notepad.Documents.GetCoding(Notepad.Documents.Current());
          bool BOM = Notepad.Documents.NeedBOM(Notepad.Documents.Current());

          bool isPathEmpty = path.empty();

          if (isPathEmpty && 
              !Notepad.Interface.Dialogs.SaveFileDialog(
                  hwnd, 
                  Notepad.Documents.GetName(Notepad.Documents.Current()), 
                  path, cp, BOM, Notepad.Options.OwnDialogs())
             ) 
          {
            break;
          }

          if (isPathEmpty && CheckFileExists(path.c_str()))
          {     
            std::wstring warning = DOC_REWRITE_FILE;
            warning += path;
            warning += L"?";
            if (Notepad.Interface.Dialogs.MsgBox(hwnd, warning.c_str(), INFO_CAPTION, MB_YESNO) != IDYES)
              break;
          }

          if (!Notepad.Documents.Save(Notepad.Documents.Current(), path, cp, BOM))
          {
            Notepad.Interface.Dialogs.ShowError(hwnd, CError::GetError().message);
            CError::SetError(NOTEPAD_NO_ERROR);
          }
        }
        break;

      case CMD_SAVEASDOC:
        if (Notepad.Documents.GetDocsNumber() > 0)
        {
          std::wstring path;
          LPCWSTR cp = Notepad.Documents.GetCoding(Notepad.Documents.Current());
          bool BOM = Notepad.Documents.NeedBOM(Notepad.Documents.Current());

          if (Notepad.Interface.Dialogs.SaveFileDialog(hwnd, Notepad.Documents.GetName(Notepad.Documents.Current()), 
                path, cp, BOM, Notepad.Options.OwnDialogs()))
          {
            if (CheckFileExists(path.c_str()))
            {     
              std::wstring warning = DOC_REWRITE_FILE;
              warning += path;
              warning += L"?";
              if (Notepad.Interface.Dialogs.MsgBox(hwnd, warning.c_str(), INFO_CAPTION, MB_YESNO) != IDYES)
                break;
            }

            if (!Notepad.Documents.Save(Notepad.Documents.Current(), path, cp, BOM))
            {
              Notepad.Interface.Dialogs.ShowError(hwnd, CError::GetError().message);
              CError::SetError(NOTEPAD_NO_ERROR);
            }
          }
        }
        break;

      case CMD_UNDO:        
        Notepad.Documents.Undo();
        break;

      case CMD_REDO:
        Notepad.Documents.Redo();
        break;

      case CMD_COPY:
        Notepad.Documents.Copy();
        break;

      case CMD_CUT:
        Notepad.Documents.Cut();
        break;

      case CMD_PASTE:
        Notepad.Documents.Paste();
        break;

      case CMD_DELETE: 
        Notepad.Documents.Delete();
        break;

      case CMD_SELECTALL:
        Notepad.Documents.SelectAll();
        break;

      case CMD_WORDWRAP:
        if (Notepad.Documents.GetDocsNumber() > 0)
          Notepad.Documents.SetWordWrap(Notepad.Documents.Current(),
            !Notepad.Documents.IsWordWrap(Notepad.Documents.Current()));
        break;

      case CMD_SMARTINPUT:
        if (Notepad.Documents.GetDocsNumber() > 0)
          Notepad.Documents.SetSmartInput(Notepad.Documents.Current(), 
            !Notepad.Documents.IsSmartInput(Notepad.Documents.Current()));
        break;

      case CMD_DRAGMODE:
        if (Notepad.Documents.GetDocsNumber() > 0)
          Notepad.Documents.SetDragMode(Notepad.Documents.Current(),
            !Notepad.Documents.IsDragMode(Notepad.Documents.Current()));
        break;

      case CMD_SCROLLBYLINE:
        if (Notepad.Documents.GetDocsNumber() > 0)
          Notepad.Documents.SetScrollByLine(Notepad.Documents.Current(), 
            !Notepad.Documents.IsScrollByLine(Notepad.Documents.Current()));
        break;

      case CMD_CLEARRECENT:
        Notepad.Recent.ClearRecent();
        break;

      case CMD_GOTOBEGIN:
        Notepad.Documents.GotoBegin();
        break;

      case CMD_GOTOEND:
        Notepad.Documents.GotoEnd();
        break;

      case CMD_EXIT:
        Notepad.OnCmdExit();
        break;

      case CMD_SAVEALL:
        Notepad.Documents.SaveAllDocuments(false);
        break;

      case CMD_CLOSEALL:
        Notepad.Documents.CloseAllDocuments();
        break;

      case CMD_REOPEN_AUTO:
        if (Notepad.Documents.GetDocsNumber() > 0 && !Notepad.Documents.GetPath(Notepad.Documents.Current()).empty())
        {
          if (!Notepad.Documents.Open(Notepad.Documents.GetPath(Notepad.Documents.Current()), CCodingManager::CODING_UNKNOWN_ID, REOPEN, true, true, false))
          {
            Notepad.Interface.Dialogs.ShowError(hwnd, CError::GetError().message);
            CError::SetError(NOTEPAD_NO_ERROR);
          }
        }
        break;

      case CMD_SHOWDOCMENU:
        if (Notepad.Interface.Controlbar.GetDocPopupMenu())
        {
          POINT pt;
          Notepad.Interface.Controlbar.GetMenuPos(hwnd, Notepad.Options.DocMenuPos(), pt);
          Notepad.Interface.Controlbar.ShowPopupMenu(hwnd, Notepad.Interface.Controlbar.GetDocPopupMenu(), pt.x, pt.y);
        }
        break;

      case CMD_SHOWPLUGINSMENU:
        if (Notepad.Interface.Controlbar.GetPluginsPopupMenu())
        {
          POINT pt;
          Notepad.Interface.Controlbar.GetMenuPos(hwnd, Notepad.Options.PluginsMenuPos(), pt);
          Notepad.Interface.Controlbar.ShowPopupMenu(hwnd, Notepad.Interface.Controlbar.GetPluginsPopupMenu(), pt.x, pt.y);
        }
        break;

#ifdef _WIN32_WCE
      case CMD_FULLSCREEN:
        //Notepad.Interface.SwitchFullScreen(!Notepad.Interface.IsFullScreen());
        Notepad.Options.SetFullScreen(!Notepad.Options.IsFullScreen());
        break;
#endif

      case IDOK: // OK button (for pocket pc)
        Notepad.OnOKButton();
        break;

      case IDM_GOTOLINE:
        Notepad.Interface.Dialogs.GotoLineDialog(hwnd, Notepad.Documents, Notepad.Options.CaretRealPos());
        break;

      case IDM_FIND:
        if (Notepad.Interface.ReplaceBar.IsReplaceBar())
          Notepad.Interface.ReplaceBar.Destroy(true);  // destroy replace bar and leave find bar
        else if (Notepad.Interface.FindBar.IsFindBar())
          Notepad.Interface.FindBar.Destroy(true);
        else
          Notepad.Interface.FindBar.Create(hwnd, true);
        break;

      case IDM_REPLACE:
        if (!Notepad.Interface.ReplaceBar.IsReplaceBar())
          Notepad.Interface.ReplaceBar.Create(hwnd, true);
        else
        {
          Notepad.Interface.ReplaceBar.Destroy(false);
          Notepad.Interface.FindBar.Destroy(true);
        }
        break;

      case IDM_QINSERT_M:  // qinsert from menu
        if (Notepad.Interface.Controlbar.IsQInsertMenu())
        {
          POINT pt;
          Notepad.Interface.Controlbar.GetMenuPos(hwnd, Notepad.Options.QinsertMenuPos(), pt);
          Notepad.Interface.Controlbar.ShowPopupMenu(hwnd, Notepad.Interface.Controlbar.GetQInsertMenu(), pt.x, pt.y);
        }
        break;

      case IDM_QINSERT_T:  // qinsert from toolbar
        if (Notepad.Interface.Controlbar.IsQInsertMenu())
        {
          int x = Notepad.Interface.Controlbar.GetTBButtonX(IDM_QINSERT_T);
          int y = Notepad.Interface.GetHeight();
          Notepad.Interface.Controlbar.ShowPopupMenu(hwnd, Notepad.Interface.Controlbar.GetQInsertMenu(), x, y);
        }
        break;

      case IDM_OPTIONS:
        Notepad.Interface.Dialogs.OptionsDialog(hwnd, Notepad.Options);
        break;

      case IDM_ABOUT:
        Notepad.Interface.Dialogs.AboutDialog(hwnd);
        break;

    /*case CMD_NEXTDOC:
        if (Notepad.Documents.Number() > 0)
        {
          int index = (Notepad.Documents.Current() + 1) % Notepad.Documents.Number();
          Notepad.Documents.SetActive(index);
        }
        break;*/
      }
    }
    break;

#if defined(_WIN32_WCE)
  case WM_SETTINGCHANGE:     // response on SIP up/down
    if (wParam == SPI_SETSIPINFO)
    {
      SWindowPos pos;
      CNotepad &Notepad = CNotepad::Instance();
      Notepad.Interface.CalcInterfaceCoords(pos, Notepad.Options.ResponseOnSIP());
      Notepad.Interface.Resize(pos.width, pos.height, true, true);
    }
    break;
#endif

   case WM_INITMENUPOPUP:
     {
       CNotepad &Notepad = CNotepad::Instance();
       HMENU hmenu = reinterpret_cast<HMENU>(wParam);

       int determinedMenu = Notepad.Interface.Controlbar.DetermineMenu(hmenu);

       LOG_ENTRY(
         L"Received WM_INITMENUPOPUP. Menu handle = 0x%.8x, determined menu = %d\nQInsert = 0x%.8x, Plugins = 0x%.8x, Documents = 0x%.8x\n", 
         (unsigned)hmenu, 
         determinedMenu, 
         Notepad.Interface.Controlbar.GetQInsertMenu(), 
         Notepad.Interface.Controlbar.GetPluginsPopupMenu(), 
         Notepad.Interface.Controlbar.GetDocPopupMenu()
       );

       if (determinedMenu > -1)
       {
         if (Notepad.Options.BugWithPopupMenusExists())
         {
           // only top level menu could be determined

           LOG_ENTRY(L"Avoiding bug with popup menus\n");

           switch (determinedMenu)
           {
           case MENU_FILE:
             Notepad.ModifyFileMenu(hmenu);
             Notepad.ModifyEditMenu(Notepad.Interface.Controlbar.GetEditMenu());
             Notepad.ModifyRecentMenu(Notepad.Interface.Controlbar.GetRecentMenu());
             Notepad.ModifyReopenMenu(Notepad.Interface.Controlbar.GetReopenMenu());
             break;

           case MENU_MENU:
             Notepad.ModifyDocMenu(Notepad.Interface.Controlbar.GetDocMenu());
             Notepad.ModifyPluginsMenu(Notepad.Interface.Controlbar.GetPluginsMenu());
             Notepad.ModifyViewMenu(Notepad.Interface.Controlbar.GetViewMenu());
             break;
           }
         }
         else
         {
           // modify only appropriate menu

           switch (determinedMenu)
           {
           case MENU_FILE:
             Notepad.ModifyFileMenu(hmenu);
             break;

           case MENU_EDIT:
             Notepad.ModifyEditMenu(hmenu);
             break;

           case MENU_RECENT:
             Notepad.ModifyRecentMenu(hmenu);
             break;

           case MENU_REOPEN:
             Notepad.ModifyReopenMenu(hmenu);
             break;

           case MENU_DOCUMENTS:
             Notepad.ModifyDocMenu(hmenu);
             break;

           case MENU_PLUGINS:
             Notepad.ModifyPluginsMenu(hmenu);
             break;

           case MENU_VIEW:
             Notepad.ModifyViewMenu(hmenu);
             break;
           }
         }
       }
       else 
       {
         // it is not sub menu from main menu, it is standalone menu

         if (hmenu == Notepad.Interface.Controlbar.GetDocPopupMenu())
           Notepad.ModifyDocMenu(hmenu);
         else if (hmenu == Notepad.Interface.Controlbar.GetPluginsPopupMenu())
           Notepad.ModifyPluginsMenu(hmenu);
       }
     }
     break;

  case WM_COPYDATA:
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    if (0 != lParam && 0 != ((PCOPYDATASTRUCT)lParam)->lpData && 0 == ((PCOPYDATASTRUCT)lParam)->dwData)
    {
      CNotepad &Notepad = CNotepad::Instance();
      std::wstring path = reinterpret_cast<wchar_t*>(((PCOPYDATASTRUCT)lParam)->lpData);
      if (!Notepad.Documents.Open(path, CCodingManager::CODING_UNKNOWN_ID, OPEN))
      {
        Notepad.Interface.Dialogs.ShowError(hwnd, CError::GetError().message);
        CError::SetError(NOTEPAD_NO_ERROR);
      }
    }
    break;

  case WM_NOTIFY:
    if (((NMHDR*)lParam)->code == SEN_CARETMODIFIED)
    {
      sen_caret_modified_t *psen = (sen_caret_modified_t*)lParam;

      CNotepad &Notepad = CNotepad::Instance();
      int index = Notepad.Documents.GetIndexFromID(psen->header.idFrom);

      if (Notepad.Interface.StatusBar.IsStatusBar() && Notepad.Documents.Current() == index)
      {
        unsigned int x = Notepad.Options.CaretRealPos() ? psen->realPos.x : psen->screenPos.x;
        unsigned int y = Notepad.Options.CaretRealPos() ? psen->realPos.y : psen->screenPos.y;
        Notepad.Interface.StatusBar.SetPosition(x, y);
      }

      PluginNotification pn;
      pn.notification = PN_FROM_EDITOR;
      pn.data = (void*)lParam;
      Notepad.PluginManager.NotifyPlugins(pn);      
    }
    else if (((NMHDR*)lParam)->code == SEN_DOCCHANGED)
    {
      CNotepad &Notepad = CNotepad::Instance();

      PluginNotification pn;
      pn.notification = PN_FROM_EDITOR;
      pn.data = (void*)lParam;
      Notepad.PluginManager.NotifyPlugins(pn);
    }
    else if (((NMHDR*)lParam)->code == SEN_DOCUPDATED)
    {
      sen_doc_updated_t *psen = (sen_doc_updated_t*)lParam;
      CNotepad &Notepad = CNotepad::Instance();
      int index = Notepad.Documents.GetIndexFromID(psen->header.idFrom);
      Notepad.Documents.SetModified(index, psen->isDocModified != 0);

      if (Notepad.Interface.Controlbar.IsToolbar())
        Notepad.Interface.Controlbar.UpdateDocDependButtons();

      PluginNotification pn;
      pn.notification = PN_FROM_EDITOR;
      pn.data = (void*)lParam;
      Notepad.PluginManager.NotifyPlugins(pn);
    }
    else if (((NMHDR*)lParam)->code == SEN_DOCCLOSED) 
    {
      CNotepad &Notepad = CNotepad::Instance();
      PluginNotification pn;
      pn.notification = PN_FROM_EDITOR;
      pn.data = (void*)lParam;
      Notepad.PluginManager.NotifyPlugins(pn);
    }
    break;

  case WM_CLOSE:
    {
      CNotepad &Notepad = CNotepad::Instance();
      Notepad.OnCmdExit();
    }
    break;

  case WM_DESTROY:
    PostQuitMessage(EXIT_SUCCESS);
    break;

  case NPD_EXIT:
    {
      CNotepad &Notepad = CNotepad::Instance();
      Notepad.OnExit();
    }
    break;

  case NPD_DELETEFONTS: // delete old fonts
    {
      CNotepad &Notepad = CNotepad::Instance();
      Notepad.Options.DeleteOldFonts();
    }
    break;


  case NPD_OPENTHREADEND:  // wParam - id, lParam - error
    {
      SOpenEndData openEndData = *((SOpenEndData*)lParam); // copy of open end data (original will be deleted in OpenEnd)

      CNotepad &Notepad = CNotepad::Instance();
      Notepad.Documents.OpenEnd((SOpenEndData*)lParam);

      if (openEndData.error != NOTEPAD_NO_ERROR)
      {
        std::wstring message;
        CError::GetErrorMessage(message, openEndData.error);
        Notepad.Interface.Dialogs.ShowError(hwnd, message);
      }
    }
    break;

  case NPD_SAVETHREADEND:  // wParam - id, lParam - error
    {
      CNotepad &Notepad = CNotepad::Instance();
      Notepad.Documents.SaveEnd(wParam, lParam != NOTEPAD_NO_ERROR);
      if (lParam != NOTEPAD_NO_ERROR)
      {
        std::wstring message;
        CError::GetErrorMessage(message, lParam);
        Notepad.Interface.Dialogs.ShowError(hwnd, message);
      }
    }
    break;

  case NPD_EMITPLUGINNOTIFICATION: // wParam - plugin id, lParam - opaque param
    {
      const wchar_t *guid = (const wchar_t*)wParam;

      PluginNotification pn;
      pn.notification = PN_CALLBACK;
      pn.data = (void*)lParam;

      CNotepad &Notepad = CNotepad::Instance();
      Notepad.PluginManager.NotifyPlugin(guid, pn);
    }
    break;

  default:
    return DefWindowProc(hwnd, msg, wParam, lParam);
  }
  return 0;
}