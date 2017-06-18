#include <windows.h>

#include <climits>
#include <map>

#include "debug.h"
#include "seditcontrol.h"
#include "Plugin.hpp"

using namespace std;


#define LW(x) *(WORD*)&(x)          // low word of DWORD

#define HW(x) *((WORD*)(&(x)) + 1)  // high word of DWORD



class EditControlEmulator : public Plugin 
{
private:

  static EditControlEmulator *ourPlugin;


private:

  PluginService *myService;

  map<int, receive_message_function_t> replacedFunctions;
 

public:

  static EditControlEmulator& GetInstance() 
  {
    if (!ourPlugin) 
      ourPlugin = new EditControlEmulator;
    return *ourPlugin;
  }

  static void ReleaseInstance()
  {
    if (ourPlugin)
    {
      delete ourPlugin;
      ourPlugin = 0;
    }
  }


public:

  EditControlEmulator() {}  

  LPCWSTR GetGUID() const
  {
    return L"0e704ed1-d5b4-47fa-9b41-6b2992c21dfd";
  }

  int GetVersion() const
  {
    return 1;
  }

  LPCWSTR GetName() const 
  {
    return L"EditControlEmulator";
  }

  int GetPriority() const 
  {
    return 0x200;
  }

  bool Init(HMODULE hModule, PluginService *service)
  {
    if (service->GetVersion() >= 3)
    {
      myService = service;
      return true;
    }
    return false;
  }

  void ExecuteCommand(__in LPCWSTR name) 
  { 
    /* not in use */ 
  }

  void ExecuteFunction(__in LPCWSTR name, __inout void *param) 
  { 
    /* not in use */ 
  }

  void ExecuteMenuCommand(__in int itemId) 
  { 
    /* not in use */ 
  }

  int GetSupportedCommandsNumber()
  {
    return 0;
  }

  const PluginCommand* GetSupportedCommands()
  {
    return 0;
  }

  int GetSupportedFunctionsNumber()
  {
    return 0;
  }

  const LPCWSTR* GetSupportedFunctions()
  {
    return 0;
  }

  HMENU GetMenuHandle(int firstItemId)
  {
    return 0;
  }

  int GetMenuItemsNumber()
  {
    return 0;
  }

  void ProcessNotification(const PluginNotification *notification) 
  { 
    switch (notification->notification)
    {
    case PN_EDITOR_CREATED:
      {
        int id = *(int*)(notification->data);
        HWND hwnd = myService->GetEditorWindow(id);
        receive_message_function_t nextFunction = (receive_message_function_t)SendMessage(hwnd, SE_GETRECEIVEMSGFUNC, 0, 0);

        if (nextFunction)
          replacedFunctions.insert(pair<int, receive_message_function_t>(id, nextFunction));

        SendMessage(hwnd, SE_SETRECEIVEMSGFUNC, 0, (LPARAM)emulateEditorFunction);
      }
      break;

    case PN_EDITOR_DESTROYED:
      {
        int id = *(int*)(notification->data);
        map<int, receive_message_function_t>::iterator iter = replacedFunctions.find(id);

        if (iter != replacedFunctions.end())
          replacedFunctions.erase(iter);
      }
      break; 
    }
  } 

  ~EditControlEmulator()
  {
  }

private:

  static char SEAPIENTRY emulateEditorFunction(__in HWND hwndFrom,
                                               __in unsigned int idFrom,
                                               __in unsigned int message,
                                               __in WPARAM wParam,
                                               __in LPARAM lParam,
                                               __out LRESULT *presult);


private:

  void sortRange(range_t *prange)
  {
    if (prange->start.y > prange->end.y || (prange->start.y == prange->end.y && prange->start.x > prange->end.x))
    {
      position_t temp = prange->start;
      prange->start = prange->end;
      prange->end = temp;
    }
  }

  bool handleError()
  {
    if (myService->GetLastError() != NO_ERROR)
    {
      VectorPluginBuffer<wchar_t> buffer;
      myService->GetLastErrorMessage(&buffer);
      myService->ShowError(buffer.GetBuffer());
      return true;
    }
    return false;
  }
};

/****************************************************************************************************/
/*                                          Filter                                                  */
/****************************************************************************************************/

char SEAPIENTRY EditControlEmulator::emulateEditorFunction(__in HWND hwndFrom,
                                                           __in unsigned int idFrom,
                                                           __in unsigned int message,
                                                           __in WPARAM wParam,
                                                           __in LPARAM lParam,
                                                           __out LRESULT *presult)
{
  switch (message)
  {
  case WM_GETTEXT:
    {
      text_block_t tb = {0};
      tb.size = wParam;
      tb.text = (wchar_t*)lParam;
      SendMessage(hwndFrom, SE_GETPOSFROMOFFSET, wParam - 1, (LPARAM)&tb.position.end); // -1 is a terminating null character
      SendMessage(hwndFrom, SE_GETTEXT, (WPARAM)&tb, SEDIT_REALLINE); 
      *presult = wcslen(tb.text); // more correct is to call SE_GETTEXT with tb.size = 0 to determine real size of the text
      return 1;
    }  

  case WM_GETTEXTLENGTH:
    {
      doc_info_t di = {0};
      SendMessage(hwndFrom, SE_GETDOCINFO, (WPARAM)&di, 0);

      text_block_t tb = {0};
      tb.position.end.y = di.nRealLines - 1;
      tb.position.end.x = SendMessage(hwndFrom, SE_GETLINELENGTH, di.nRealLines - 1, SEDIT_REALLINE);
      *presult = SendMessage(hwndFrom, SE_GETTEXT, (WPARAM)&tb, SEDIT_REALLINE) - 1; // -1 is because of null character

      return 1;
    }

  case EM_CANUNDO:
    {
      *presult = SendMessage(hwndFrom, SE_CANUNDO, 0, 0);
      return 1;
    }

  case EM_CHARFROMPOS:
    {
      // Results not guaranteed to be similar as in real edit control

      position_t position;
      SendMessage(hwndFrom, SE_GETPOSFROMCOORDS, lParam, (LPARAM)&position);
      
      unsigned int offset;
      SendMessage(hwndFrom, SE_GETOFFSETFROMPOS, (WPARAM)&position, (LPARAM)&offset);

      LW(*presult) = (WORD)offset;
      HW(*presult) = (WORD)position.y;

      return 1;
    }

  case EM_GETLINE: 
    {
      // Differences: 
      // 1. Should return zero in case of line index is greater than number of lines

      text_block_t block;
      block.size = *(WORD*)lParam + 1;
      block.text = new wchar_t[block.size];

      block.position.start.y = block.position.end.y = wParam;
      block.position.start.x = 0;
      block.position.end.x = min(block.size - 1, (unsigned)SendMessage(hwndFrom, SE_GETLINELENGTH, wParam, SEDIT_REALLINE));
      SendMessage(hwndFrom, SE_GETTEXT, (WPARAM)&block, SEDIT_REALLINE);

      memcpy((wchar_t*)lParam, block.text, block.position.end.x * sizeof(wchar_t));
      delete[] block.text;

      *presult = block.position.end.x;

      return 1;
    }

  case EM_GETSEL:
    {
      unsigned int startOffset;
      unsigned int endOffset;

      if (SendMessage(hwndFrom, SE_ISSELECTION, 0, 0))
      {
        range_t selection;
        SendMessage(hwndFrom, SE_GETSEL, (WPARAM)&selection, SEDIT_REALLINE);
        SendMessage(hwndFrom, SE_GETOFFSETFROMPOS, (WPARAM)&selection.start, (LPARAM)&startOffset);
        SendMessage(hwndFrom, SE_GETOFFSETFROMPOS, (WPARAM)&selection.end, (LPARAM)&endOffset);
      }
      else
      {
        doc_info_t di;
        di.flags = SEDIT_GETCARETREALPOS;
        SendMessage(hwndFrom, SE_GETDOCINFO, (WPARAM)&di, 0);
        SendMessage(hwndFrom, SE_GETOFFSETFROMPOS, (WPARAM)&di.caretPos, (LPARAM)&startOffset);
        endOffset = startOffset;
      }

      if (wParam && lParam)
        *(unsigned int*)wParam = startOffset;
      if (lParam)
        *(unsigned int*)lParam = endOffset;

      if (startOffset <= USHRT_MAX && endOffset <= USHRT_MAX)
      {
        LW(*presult) = startOffset;
        HW(*presult) = endOffset;
      }
      else
        *presult = -1;
      
      return 1;
    }

  case EM_LINEFROMCHAR:
    {
      if ((int)wParam != -1)
      {
        position_t position;
        SendMessage(hwndFrom, SE_GETPOSFROMOFFSET, wParam, (LPARAM)&position);
        *presult = position.y;
      }
      else
      {
        if (SendMessage(hwndFrom, SE_ISSELECTION, 0, 0))
        {
          range_t selection;
          SendMessage(hwndFrom, SE_GETSEL, (WPARAM)&selection, SEDIT_REALLINE);
          *presult = selection.start.y;
        }
        else
        {
          doc_info_t di;
          di.flags = SEDIT_GETCARETREALPOS;
          SendMessage(hwndFrom, SE_GETDOCINFO, (WPARAM)&di, 0);
          *presult = di.caretPos.y;
        }
      }

      return 1;
    }

  case EM_LINEINDEX:
    {
      if ((int)wParam != -1)
      {
        position_t position = {0, wParam};
        SendMessage(hwndFrom, SE_GETOFFSETFROMPOS, (WPARAM)&position, (LPARAM)presult);
      }
      else
      {
        doc_info_t di;
        di.flags = SEDIT_GETCARETREALPOS;
        SendMessage(hwndFrom, SE_GETDOCINFO, (WPARAM)&di, 0);

        position_t position = {0, di.caretPos.y};
        SendMessage(hwndFrom, SE_GETOFFSETFROMPOS, (WPARAM)&position, (LPARAM)presult);          
      }

      return 1;
    }

  case EM_LINELENGTH:
    {
      // Differences:
      // 1. should return zero in case of line index is greater than number of lines

      EditControlEmulator &emulator = EditControlEmulator::GetInstance();

      if ((int)wParam != -1)
      {
        position_t position;
        SendMessage(hwndFrom, SE_GETPOSFROMOFFSET, wParam, (LPARAM)&position);
        *presult = SendMessage(hwndFrom, SE_GETLINELENGTH, position.y, SEDIT_REALLINE);
      }
      else if (SendMessage(hwndFrom, SE_ISSELECTION, 0, 0))
      {
        range_t selection;
        SendMessage(hwndFrom, SE_GETSEL, (WPARAM)&selection, SEDIT_REALLINE);
        emulator.sortRange(&selection);

        unsigned int endLineLength = SendMessage(hwndFrom, SE_GETLINELENGTH, selection.end.y, SEDIT_REALLINE);

        *presult = selection.start.x + (endLineLength - selection.end.x);
      }
      else
        *presult = 0;  // undefined behavior

      return 1;
    }

  case EM_SETSEL:
    {
      if ((int)wParam != -1 && (int)lParam != -1)
      {
        range_t selection;
        SendMessage(hwndFrom, SE_GETPOSFROMOFFSET, wParam, (LPARAM)&selection.start);
        SendMessage(hwndFrom, SE_GETPOSFROMOFFSET, lParam, (LPARAM)&selection.end);
        SendMessage(hwndFrom, SE_SETSEL, (WPARAM)&selection, SEDIT_REALLINE);
      }
      else if (-1 == (int)wParam) // start is -1
      {
        range_t selection = {0};
        SendMessage(hwndFrom, SE_SETSEL, (WPARAM)&selection, SEDIT_REALLINE);
      }
      else if (0 == wParam && -1 == (int)lParam) // start is 0 and end is -1
        SendMessage(hwndFrom, SE_SELECTALL, 0, 0);
      
      *presult = 0;

      return 1;
    }

  case EM_REPLACESEL:
    {
      // Differences:
      // 1. wParam is ignored (means operation always may be undone)

      SendMessage(hwndFrom, SE_PASTEFROMBUFFER, (WPARAM)lParam, 1);

      *presult = 0;      
      return 1;
    }

  case EM_UNDO:
  case WM_UNDO:
    {
      // Differences:
      // 1. There is no way to get result of the operation => always return true

      SendMessage(hwndFrom, SE_UNDO, 0, 0);
      *presult = TRUE;
      return 1;
    }

  /*
  ********************************************************
  *   Not implemented messages
  ********************************************************
  */

  case EM_EMPTYUNDOBUFFER:
    *presult = 0;
    LOG_ENTRY(L"EM_EMPTYUNDOBUFFER received!");
    return 1;

  case EM_FMTLINES:
    *presult = 0;
    LOG_ENTRY(L"EM_FMTLINES received!");
    return 1;

  case EM_GETFIRSTVISIBLELINE:
    *presult = 0;
    LOG_ENTRY(L"EM_GETFIRSTVISIBLELINE received!");
    return 1;

  case EM_GETLIMITTEXT:
    *presult = 0;
    LOG_ENTRY(L"EM_GETLIMITTEXT received!");
    return 1;

  case EM_GETLINECOUNT:
    *presult = 0;
    LOG_ENTRY(L"EM_GETLINECOUNT received!");
    return 1;

  case EM_GETMARGINS:
    *presult = 0;
    LOG_ENTRY(L"EM_GETMARGINS received!");
    return 1;

  case EM_GETMODIFY:
    *presult = 0;
    LOG_ENTRY(L"EM_GETMODIFY received!");
    return 1;

  case EM_GETPASSWORDCHAR:
    *presult = 0;
    LOG_ENTRY(L"EM_GETPASSWORDCHAR received!");
    return 1;

  case EM_GETRECT:
    *presult = 0;
    LOG_ENTRY(L"EM_GETRECT received!");
    return 1;

  case EM_LIMITTEXT:
    *presult = 0;
    LOG_ENTRY(L"EM_LIMITTEXT received!");
    return 1;

  case EM_LINESCROLL:
    *presult = 0;
    LOG_ENTRY(L"EM_LINESCROLL received!");
    return 1;

  case EM_POSFROMCHAR:
    *presult = 0;
    LOG_ENTRY(L"EM_POSFROMCHAR received!");
    return 1;

  case EM_SCROLL:
    *presult = 0;
    LOG_ENTRY(L"EM_SCROLL received!");
    return 1;

  case EM_SCROLLCARET:
    *presult = 0;
    LOG_ENTRY(L"EM_SCROLLCARET received!");
    return 1;

  case EM_SETMARGINS:
    *presult = 0;
    LOG_ENTRY(L"EM_SETMARGINS received!");
    return 1;

  case EM_SETMODIFY:
    *presult = 0;
    LOG_ENTRY(L"EM_SETMODIFY received!");
    return 1;

  case EM_SETRECT:
    *presult = 0;
    LOG_ENTRY(L"EM_SETRECT received!");
    return 1;

  case EM_SETRECTNP:
    *presult = 0;
    LOG_ENTRY(L"EM_SETRECTNP received!");
    return 1;


  default: 
    {    
      EditControlEmulator &emulator = EditControlEmulator::GetInstance();

      map<int, receive_message_function_t>::iterator iPrevFunction = emulator.replacedFunctions.find(idFrom);
      if (iPrevFunction != emulator.replacedFunctions.end())
        return iPrevFunction->second(hwndFrom, idFrom, message, wParam, lParam, presult);
      return 0;    
    }
  }
}


/****************************************************************************************************/
/*                                 External functions                                               */
/****************************************************************************************************/

extern "C" __declspec(dllexport)
Plugin* __stdcall SECreatePlugin()
{
  return &EditControlEmulator::GetInstance();
}

extern "C" __declspec(dllexport)
void __stdcall SEDeletePlugin(Plugin *plugin)
{
  EditControlEmulator::ReleaseInstance();
}

/****************************************************************************************************/
/*                                 Static initialization                                            */
/****************************************************************************************************/

EditControlEmulator* EditControlEmulator::ourPlugin(0);