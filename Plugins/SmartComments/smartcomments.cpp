#include <windows.h>

#include <cwchar>
#include <cctype>
#include <string>
#include <map>


#include "seditcontrol.h"
#include "Plugin.hpp"

#include "PropertyFileParser.hpp"

using namespace std;


enum Command {
  SingleComment = 0,

  CommandsNumber
};

struct CommentEntry
{
  wstring *singleLineComment;
  vector<wstring> extensions;

  CommentEntry() : singleLineComment(0) {}
};


static PluginHotKey s_hotkeys[] = {
  {false, true, 0xbf}  // virtual keycode for '/'
};

static PluginCommand s_commands[] = {
  {L"singleComment", &s_hotkeys[SingleComment]}
};

// Despite of fact that the current function's chain should always remain correct (if in all plugins it is implemented like here), 
// it is better if notepad will be responsible for chain management (plugins shouldn't replace receive_message_function_t by themselves).
static map<int, receive_message_function_t> s_replacedFunctions;



class SmartCommentsPlugin : public Plugin, protected PropertyFileParser
{
private:

  enum Section 
  {
    undefined,
    comments,
    options
  };


private:

  static SmartCommentsPlugin *ourPlugin;


private:

  static const wstring SECTION_COMMENTS;

  static const wstring SECTION_OPTIONS;


private:

  PluginService *myService;

  wstring myRootPath;

  vector<wstring*> myAllocatedResources;

  map<wstring, wstring*> mySingleLineComments;

  HMENU myMenuHandle;

  int myMenuFirstItemId;

  bool myKeepIndent;

  bool myTabAsSpaces;

  bool myOptionsChanged;


  // for loading

  Section myCurrentSection;

  CommentEntry *myCommentEntry;


public:

  static bool IsPluginCreated() 
  {
    return ourPlugin != 0;
  }

  static SmartCommentsPlugin& GetInstance() 
  {
    if (!ourPlugin) 
      ourPlugin = new SmartCommentsPlugin;
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

  SmartCommentsPlugin() : myCommentEntry(0), myMenuHandle(0), myMenuFirstItemId(0)
  {
    myCurrentSection = undefined;
    myKeepIndent = false;
    myTabAsSpaces = false;
    myOptionsChanged = false;
  }  

  LPCWSTR GetGUID() const
  {
    return L"b8349692-5455-4e58-860d-e1c40f2bac58";
  }

  int GetVersion() const
  {
    return 1;
  }

  LPCWSTR GetName() const 
  {
    return L"Smart Comments";
  }

  int GetPriority() const 
  {
    return 0x7ff;
  }

  bool Init(HMODULE hModule, PluginService *service)
  {
    myService = service;

    if (service->GetVersion() >= 2)
    {
      myRootPath.assign(service->GetRootPath());

      this->ReadFile((myRootPath + DIRECTORY_SEPARATOR + L"smartcomments.ini").c_str());
      this->ReadFile((myRootPath + DIRECTORY_SEPARATOR + L"options.ini").c_str());

      return true;
    }

    return false;
  }

  void ExecuteCommand(__in LPCWSTR name) 
  { 
    if (wcscmp(name, s_commands[SingleComment].name) == 0)
    {
      int id = myService->GetCurrentEditor();
      if (id >= 0)
      {
        HWND hwnd = myService->GetEditorWindow(id);
        
        std::wstring extension;

        if (determineExtension(id, extension))
        {
          map<wstring, wstring*>::iterator iter = mySingleLineComments.find(extension);

          if (iter != mySingleLineComments.end())
          {
            wstring *singleLineComment = iter->second;

            doSingleCommentCommand(id, hwnd, singleLineComment);
          }        
        }        
      }
    }
  }

  void ExecuteFunction(__in LPCWSTR name, __inout void *param) 
  {
    /* not in use */ 
  }

  void ExecuteMenuCommand(__in int itemId)
  { 
    int itemIndex = itemId - myMenuFirstItemId;

    switch (itemIndex)
    {
    case 0:
      myKeepIndent = !myKeepIndent;
      myOptionsChanged = true;
      break;

    case 1:
      myTabAsSpaces = !myTabAsSpaces;
      myOptionsChanged = true;
      break;
    }
  }

  int GetSupportedCommandsNumber()
  {
    return CommandsNumber;
  }

  const PluginCommand* GetSupportedCommands()
  {
    return s_commands;
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
    // destroy old menu, if necessary
    if (myMenuHandle && firstItemId != myMenuFirstItemId)
    {
      DestroyMenu(myMenuHandle);
      myMenuHandle = 0;
    }

    myMenuFirstItemId = firstItemId;

    // create new menu
    if (!myMenuHandle)
      myMenuHandle = CreatePopupMenu();    

    // if menu present, update it
    if (myMenuHandle)
    {
      int itemId = myMenuFirstItemId;

      BOOL isItem = TRUE;
      while (isItem)
        isItem = RemoveMenu(myMenuHandle, 0, MF_BYPOSITION);

      AppendMenu(myMenuHandle, MF_STRING | MF_ENABLED | (myKeepIndent ? MF_CHECKED : 0), itemId++, L"Keep indent");

      AppendMenu(myMenuHandle, MF_STRING | MF_ENABLED | (myTabAsSpaces ? MF_CHECKED : 0), itemId++, L"Replace tab with spaces");
    }

    return myMenuHandle;
  }

  int GetMenuItemsNumber()
  {
    return 2;
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
          s_replacedFunctions.insert(pair<int, receive_message_function_t>(id, nextFunction));

        SendMessage(hwnd, SE_SETRECEIVEMSGFUNC, 0, (LPARAM)preprocessEditorMessage);
      }
      break;

    case PN_EDITOR_DESTROYED:
      {
        int id = *(int*)(notification->data);
        map<int, receive_message_function_t>::iterator iter = s_replacedFunctions.find(id);

        if (iter != s_replacedFunctions.end())
          s_replacedFunctions.erase(iter);
      }
      break; 
    }
  } 

  ~SmartCommentsPlugin()
  {
    saveOptions();

    for (vector<wstring*>::const_iterator iter = myAllocatedResources.begin(); iter != myAllocatedResources.end(); iter++)
      delete (*iter);

    if (myMenuHandle)
    {
      DestroyMenu(myMenuHandle);
      myMenuHandle = 0;
    }
  }


protected:

  void HandleSection(const std::wstring &section) 
  {
    // handle end of section
    if (myCurrentSection == comments)
      handleCommentEntry();

    // enter new section
    if (section.compare(SECTION_COMMENTS) == 0)
    {
      myCurrentSection = comments;
      myCommentEntry = new CommentEntry;
    }
    else if (section.compare(SECTION_OPTIONS) == 0)
      myCurrentSection = options;
    else 
      myCurrentSection = undefined;
  }

  void HandleSectionProperty(const std::wstring &name, const std::wstring &value)
  {
    switch (myCurrentSection) 
    {
    case comments: 
      sectionComments(name, value);
      break;

    case options:
      sectionOptions(name, value);
      break;
    }
  }

  void HandleEndOfFile()
  {
    handleCommentEntry();
  }


private:

  static char SEAPIENTRY preprocessEditorMessage(__in HWND hwndFrom,
                                                 __in unsigned int idFrom,
                                                 __in unsigned int message,
                                                 __in WPARAM wParam,
                                                 __in LPARAM lParam,
                                                 __out LRESULT *presult)
  {
    // Here we will filter and process editor's messages

    if (IsPluginCreated())
    {
      SmartCommentsPlugin &plugin = GetInstance();

      if (WM_CHAR == message)
      {      
        if (VK_TAB == wParam)
        {
          // process tab if there is multiline selection
          if (SendMessage(hwndFrom, SE_ISSELECTION, 0, 0))
          {
            range_t selectionRange;
            SendMessage(hwndFrom, SE_GETSEL, (WPARAM)&selectionRange, SEDIT_REALLINE);
            plugin.sortRange(&selectionRange);

            if (selectionRange.start.y != selectionRange.end.y)
            {
              options_t options;
              SendMessage(hwndFrom, SE_GETOPTIONS, (WPARAM)&options, 0);

              if (plugin.myTabAsSpaces)
              {
                wstring prefix;
                prefix.resize(options.tabSize, L' ');
                plugin.addPrefixToRange(hwndFrom, selectionRange, &prefix);
              }
              else
              {
                wstring prefix = L"\t";
                plugin.addPrefixToRange(hwndFrom, selectionRange, &prefix);
              }

              *presult = 0;
              return 1;
            }
          } 

          // process tab in normal situation
          if (plugin.myTabAsSpaces)
          {
            options_t options;
            SendMessage(hwndFrom, SE_GETOPTIONS, (WPARAM)&options, 0);

            wstring prefix;
            prefix.resize(options.tabSize, L' ');
            SendMessage(hwndFrom, SE_PASTEFROMBUFFER, (WPARAM)prefix.c_str(), 1);          

            *presult = 0;
            return 1;
          }
        }
        else if (VK_RETURN == wParam)
        {
          if (plugin.myKeepIndent)
          {
            doc_info_t di;
            di.flags = SEDIT_GETCARETREALPOS;
            SendMessage(hwndFrom, SE_GETDOCINFO, (WPARAM)&di, 0);

            vector<wchar_t> buffer;
            plugin.getLine(hwndFrom, di.caretPos.y, buffer);

            int indentEnd = plugin.findNotEmptyPosition(buffer);
            int breakIndex = min(di.caretPos.x, indentEnd);

            if (breakIndex > 0) 
            {
              if (buffer.size() < breakIndex + 2)
                buffer.resize(breakIndex + 2, 0);

              buffer[breakIndex] = 0;
              for (int i = breakIndex; i >= 0; i--)
                buffer[i + 1] = buffer[i];
              buffer[0] = L'\n';

              SendMessage(hwndFrom, SE_PASTEFROMBUFFER, (WPARAM)&buffer[0], 0);

              *presult = 0;
              return 1;
            }
          }
        }
      }
    }

    char suppress = 0;

    map<int, receive_message_function_t>::iterator iter = s_replacedFunctions.find(idFrom);
    if (iter != s_replacedFunctions.end())
    {
      receive_message_function_t nextFunction = iter->second;
      suppress = nextFunction(hwndFrom, idFrom, message, wParam, lParam, presult);
    }

    return suppress;
  }


private:

  void sectionComments(const std::wstring &name, const std::wstring &value)
  {
    if (name.compare(L"SingleLineComment") == 0)
      myCommentEntry->singleLineComment = new wstring(value);
    else if (name.compare(L"Extension") == 0)
      myCommentEntry->extensions.push_back(value);
  }

  void sectionOptions(const std::wstring &name, const std::wstring &value)
  {
    if (name.compare(L"KeepIndent") == 0)
      myKeepIndent = (parseInt(value, 10) != 0);
    else if (name.compare(L"TabAsSpaces") == 0)
      myTabAsSpaces = (parseInt(value, 10) != 0);
  }

  void handleCommentEntry()
  {
    if (myCommentEntry)
    {    
      if (!myCommentEntry->extensions.empty() && myCommentEntry->singleLineComment)
      {      
        vector<wstring>::const_iterator iter = myCommentEntry->extensions.begin();
        for (vector<wstring>::const_iterator iter = myCommentEntry->extensions.begin(); iter != myCommentEntry->extensions.end(); iter++)
          mySingleLineComments.insert(pair<wstring, wstring*>(*iter, myCommentEntry->singleLineComment));          
        myAllocatedResources.push_back(myCommentEntry->singleLineComment);
      }
      else 
      {
        if (myCommentEntry->singleLineComment)
          delete myCommentEntry->singleLineComment;
      }

      delete myCommentEntry;
      myCommentEntry = 0;
    }
  }

  void getLine(__in HWND hwnd, __in int line, __out vector<wchar_t> &buffer)
  {
    size_t lineLength = SendMessage(hwnd, SE_GETLINELENGTH, line, SEDIT_REALLINE);
    text_block_t textBlock;

    if (lineLength + 1 > buffer.size())
      buffer.resize(lineLength + 1);

    textBlock.text = &buffer[0];
    textBlock.size = lineLength + 1;

    textBlock.position.start.x = 0;
    textBlock.position.end.x = lineLength;
    textBlock.position.start.y = textBlock.position.end.y = line;

    SendMessage(hwnd, SE_GETTEXT, (WPARAM)&textBlock, SEDIT_REALLINE);
  }

  int findPrefixPosition(__in const wstring *pprefixLine, __in const vector<wchar_t> &line)
  {
    bool beginsWithComment = false;
    const wchar_t *currentCharacter = &line[0];

    while (*currentCharacter)
    {
      if (!iswspace(*currentCharacter))
      {
        beginsWithComment = (wcsncmp(currentCharacter, pprefixLine->c_str(), pprefixLine->size()) == 0);
        break;
      }
      ++currentCharacter;
    }

    return (beginsWithComment ? (currentCharacter - &line[0]) : -1);
  }

  int findNotEmptyPosition(__in const vector<wchar_t> &line)
  {
    bool beginsWithComment = false;
    const wchar_t *currentCharacter = &line[0];

    while (*currentCharacter && iswspace(*currentCharacter))
      ++currentCharacter;

    return currentCharacter - &line[0];
  }

  // Returns true if range must be commented
  bool analyzeLinesForSingleCommentCommand(HWND hwnd, unsigned int lineFrom, unsigned int lineTo, const wstring *psingleLineComment)
  {
    vector<wchar_t> buffer;

    bool mustBeCommented = false;

    for (unsigned int line = lineFrom; line <= lineTo; line++)
    {
      getLine(hwnd, line, buffer);
      if (findPrefixPosition(psingleLineComment, buffer) == -1)
      {
        mustBeCommented = true;
        break;
      }
    }

    return mustBeCommented;
  }

  void doSingleCommentCommand(int id, HWND hwnd, const wstring *psingleLineComment)
  {
    if (SendMessage(hwnd, SE_ISSELECTION, 0, 0))
    {
      range_t selectionRange;
      SendMessage(hwnd, SE_GETSEL, (WPARAM)&selectionRange, SEDIT_REALLINE);
      sortRange(&selectionRange);

      if (analyzeLinesForSingleCommentCommand(hwnd, selectionRange.start.y, selectionRange.end.y, psingleLineComment))
        addPrefixToRange(hwnd, selectionRange, psingleLineComment);
      else
        removePrefixFromRange(hwnd, selectionRange, psingleLineComment);
    }
    else
    {
      doc_info_t di;
      di.flags = SEDIT_GETCARETREALPOS;
      SendMessage(hwnd, SE_GETDOCINFO, (WPARAM)&di, 0);

      if (analyzeLinesForSingleCommentCommand(hwnd, di.caretPos.y, di.caretPos.y, psingleLineComment))
        addPrefixToLine(hwnd, psingleLineComment);  
      else
        removePrefixFromLine(hwnd, psingleLineComment);
    }
  }

  void addPrefixToRange(HWND hwnd, const range_t &selectionRange, const wstring *plinePrefix)
  {
    doc_info_t di;
    di.flags = SEDIT_GETCARETREALPOS | SEDIT_GETWINDOWPOS;
    SendMessage(hwnd, SE_GETDOCINFO, (WPARAM)&di, 0);

    position_t savedCaretPosition = di.caretPos;
    if (savedCaretPosition.y >= selectionRange.start.y && savedCaretPosition.y <= selectionRange.end.y)
      savedCaretPosition.x += plinePrefix->size();

    position_t savedWindowPosition = di.windowPos;

    di.flags = SEDIT_SETCARETREALPOS;

    SendMessage(hwnd, SE_RESTRICTDRAWING, 0, 0);
    SendMessage(hwnd, SE_STARTUNDOGROUP, 0, 0);
    for (size_t line = selectionRange.start.y; line <= selectionRange.end.y; line++)
    {
      di.caretPos.y = line;
      di.caretPos.x = 0;
      SendMessage(hwnd, SE_SETPOSITIONS, (WPARAM)&di, 0);
      SendMessage(hwnd, SE_PASTEFROMBUFFER, (WPARAM)plinePrefix->c_str(), 0);
    }
    SendMessage(hwnd, SE_ENDUNDOGROUP, 0, 0);
    SendMessage(hwnd, SE_ALLOWDRAWING, 0, 0);

    di.flags = SEDIT_SETWINDOWPOS | SEDIT_SETCARETREALPOS;
    di.windowPos = savedWindowPosition;
    di.caretPos = savedCaretPosition;
    SendMessage(hwnd, SE_SETPOSITIONS, (WPARAM)&di, 0);          

    range_t newSelectionRange = selectionRange;
    newSelectionRange.start.x += plinePrefix->size();
    newSelectionRange.end.x += plinePrefix->size();
    SendMessage(hwnd, SE_SETSEL, (WPARAM)&newSelectionRange, SEDIT_REALLINE);
  }

  void removePrefixFromRange(HWND hwnd, const range_t &selectionRange, const wstring *plinePrefix)
  {
    vector<wchar_t> buffer;

    doc_info_t di;
    di.flags = SEDIT_GETCARETREALPOS | SEDIT_GETWINDOWPOS;
    SendMessage(hwnd, SE_GETDOCINFO, (WPARAM)&di, 0);

    if (di.caretPos.y >= selectionRange.start.y && di.caretPos.y <= selectionRange.end.y)
      di.caretPos.x = (di.caretPos.x = plinePrefix->size() ? di.caretPos.x - plinePrefix->size() : 0);

    SendMessage(hwnd, SE_RESTRICTDRAWING, 0, 0);
    SendMessage(hwnd, SE_STARTUNDOGROUP, 0, 0);
    for (size_t line = selectionRange.start.y; line <= selectionRange.end.y; line++)
    {
      getLine(hwnd, line, buffer);
      int prefixPosition = findPrefixPosition(plinePrefix, buffer);

      range_t prefixRange;
      prefixRange.start.y = prefixRange.end.y = line;
      prefixRange.start.x = prefixPosition;
      prefixRange.end.x = prefixPosition + plinePrefix->size();
      SendMessage(hwnd, SE_SETSEL, (WPARAM)&prefixRange, SEDIT_REALLINE);
      SendMessage(hwnd, SE_DELETE, 0, 0);      
    }
    SendMessage(hwnd, SE_ENDUNDOGROUP, 0, 0);
    SendMessage(hwnd, SE_ALLOWDRAWING, 0, 0);

    di.flags = SEDIT_SETWINDOWPOS | SEDIT_SETCARETREALPOS;
    SendMessage(hwnd, SE_SETPOSITIONS, (WPARAM)&di, 0);          

    range_t newSelectionRange = selectionRange;
    newSelectionRange.start.x = (newSelectionRange.start.x > plinePrefix->size()) ? newSelectionRange.start.x - plinePrefix->size() : 0;
    newSelectionRange.end.x = (newSelectionRange.end.x > plinePrefix->size()) ? newSelectionRange.end.x - plinePrefix->size() : 0;
    SendMessage(hwnd, SE_SETSEL, (WPARAM)&newSelectionRange, SEDIT_REALLINE);
  }

  void addPrefixToLine(HWND hwnd, const wstring *plinePrefix)
  {
    doc_info_t di;
    di.flags = SEDIT_GETCARETREALPOS;
    SendMessage(hwnd, SE_GETDOCINFO, (WPARAM)&di, 0);     

    position_t savedCaretPosition = di.caretPos;
    savedCaretPosition.x += plinePrefix->size();

    di.flags = SEDIT_SETCARETREALPOS;
    di.caretPos.x = 0;
    SendMessage(hwnd, SE_SETPOSITIONS, (WPARAM)&di, 0);
    SendMessage(hwnd, SE_PASTEFROMBUFFER, (WPARAM)plinePrefix->c_str(), 0);

    di.caretPos = savedCaretPosition;
    SendMessage(hwnd, SE_SETPOSITIONS, (WPARAM)&di, 0);
  }

  void removePrefixFromLine(HWND hwnd, const wstring *plinePrefix)
  {
    vector<wchar_t> buffer;

    doc_info_t di;
    di.flags = SEDIT_GETCARETREALPOS | SEDIT_GETWINDOWPOS;
    SendMessage(hwnd, SE_GETDOCINFO, (WPARAM)&di, 0);

    di.caretPos.x = (di.caretPos.x > plinePrefix->size() ? di.caretPos.x - plinePrefix->size(): 0);

    unsigned int line = di.caretPos.y;

    getLine(hwnd, line, buffer);
    int prefixPosition = findPrefixPosition(plinePrefix, buffer);

    range_t prefixRange;
    prefixRange.start.y = prefixRange.end.y = line;
    prefixRange.start.x = prefixPosition;
    prefixRange.end.x = prefixPosition + plinePrefix->size();
    SendMessage(hwnd, SE_SETSEL, (WPARAM)&prefixRange, SEDIT_REALLINE);
    SendMessage(hwnd, SE_DELETE, 0, 0);      

    di.flags = SEDIT_SETWINDOWPOS | SEDIT_SETCARETREALPOS;
    SendMessage(hwnd, SE_SETPOSITIONS, (WPARAM)&di, 0);          
  }

  bool determineExtension(int id, __out wstring &extension)
  {
    wchar_t editorFilePath[MAX_PATH];
    size_t length;

    myService->GetEditorFilePath(id, editorFilePath);
    length = wcslen(editorFilePath);

    extension.clear();

    int extStart = length - 1;
    while (extStart > 0)
    {
      if (editorFilePath[extStart] == L'.')
      {
        extension.assign(&editorFilePath[extStart + 1]);
        break;
      }
      extStart--;
    }

    return extension.size() > 0;
  }

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

  void saveOptions()
  {
    std::wstring pathToFile = (myRootPath + DIRECTORY_SEPARATOR + L"options.ini");

    if (myOptionsChanged)
    {
      FILE *fOut;

      fOut = _wfopen(pathToFile.c_str(), L"wb");
      if (fOut)
      {
        // write bom
        unsigned short bom = 0xFEFF;
        fwrite(&bom, 2, 1, fOut);

        // save options
        fwprintf(fOut, L"[OPTIONS]\n");
        fwprintf(fOut, L"KeepIndent=%d\n", myKeepIndent ? 1 : 0);
        fwprintf(fOut, L"TabAsSpaces=%d\n", myTabAsSpaces ? 1 : 0);

        fclose(fOut);
      }
    }
  }
};

/****************************************************************************************************/
/*                                 External functions                                               */
/****************************************************************************************************/

extern "C" __declspec(dllexport)
Plugin* __stdcall SECreatePlugin()
{
  return &SmartCommentsPlugin::GetInstance();
}

extern "C" __declspec(dllexport)
void __stdcall SEDeletePlugin(Plugin *plugin)
{
  SmartCommentsPlugin::ReleaseInstance();
}

/****************************************************************************************************/
/*                                 Static initialization                                            */
/****************************************************************************************************/

SmartCommentsPlugin* SmartCommentsPlugin::ourPlugin(0);

const wstring SmartCommentsPlugin::SECTION_COMMENTS(L"COMMENTS");

const wstring SmartCommentsPlugin::SECTION_OPTIONS(L"OPTIONS");