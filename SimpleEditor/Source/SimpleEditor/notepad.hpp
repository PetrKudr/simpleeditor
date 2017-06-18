#ifndef __NOTEPAD_
#define __NOTEPAD_

#include "..\debug.h"

#include "documents.hpp"
#include "interface.hpp"
#include "recent.hpp"
#include "options.hpp"
#include "error.hpp"
#include "PluginsManager.hpp"


// Include library which contains Simple Edit Control
#ifdef _DEBUG

#  ifndef _WIN32_WCE
#    pragma comment(lib, "SEditControlDebug.lib")  
#  else   // _WIN32_WCE defined
#    pragma comment(lib, "SEditControlCEDebug.lib")  
#  endif  // end of _WIN32_WCE

#  else   // RELEASE configuration

#  ifndef _WIN32_WCE
#    pragma comment(lib, "SEditControlRelease.lib")  
#  else   // _WIN32_WCE defined
#    pragma comment(lib, "SEditControlCERelease.lib")  
#  endif  // end of _WIN32_WCE

#endif  // end of _DEBUG

// Application-defined messages
#define NPD_OPENTHREADEND           (WM_USER + 1)  // wParam - id of the document, lParam - error code
#define NPD_SAVETHREADEND           (WM_USER + 2)  // wParam - id of the document, lParam - error code

#define NPD_EXIT                    (WM_USER + 3)  // message for WaitAll procedure
#define NPD_DELETEFONTS             (WM_USER + 4)  // message for WaitAll procedure

#define NPD_EMITPLUGINNOTIFICATION  (WM_USER + 5)  // message for emitting plug-in notification on it's request

// Names of the main window and class
const wchar_t AppName[] = L"SimpleEditor";
const wchar_t ClassName[] = L"SimpleEditorClass";  

// Values for OK button (Win CE)
const int OK_BUTTON_EXIT       = 0;
const int OK_BUTTON_MINIMIZE   = 1;
const int OK_BUTTON_CLOSE      = 2;
const int OK_BUTTON_SMARTCLOSE = 3;

// Main window style
#ifndef _WIN32_WCE
#  define NOTEPAD_WINDOW_STYLE (WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_BORDER)
#else
#  define NOTEPAD_WINDOW_STYLE (WS_VISIBLE)
#endif


// Main class
class CNotepad
{
private:
  //CNotepad();
  //~CNotepad();
  //HWND hfocus;
  bool setExecPath();

  static LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

public:
  CDocuments Documents;
  CInterface Interface;
  CRecent Recent;
  COptions Options;
  CPluginManager PluginManager; 

  static CNotepad& Instance();
  bool InitNotepad(HINSTANCE hInstance, wchar_t *lpCmdLine);
  bool LoadLangModule();

  void OnCmdExit();
  void OnExit();
  void OnOKButton();
  void OnActivate();

  bool SaveRecentAndSession();

  void ModifyFileMenu(HMENU hmenu);
  void ModifyDocMenu(HMENU hmenu);
  void ModifyPluginsMenu(HMENU hmenu);
  void ModifyEditMenu(HMENU hmenu);
  void ModifyEditorPopupMenu(HMENU hmenu);
  void ModifyViewMenu(HMENU hmenu);
  void ModifyRecentMenu(HMENU hmenu);
  void ModifyReopenMenu(HMENU hmenu);

  void ClearPluginsMenu(HMENU hmenu);
};


#endif /* __NOTEPAD_ */
