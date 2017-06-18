#include "hlalloc.hpp"

#include <vector>
#include <queue>
#include <map>
#include <list>
#include <string>
#include <fstream>
#include <windows.h>
using namespace std;


#include "seditcontrol.h"
#include "MessageCollector.hpp"
#include "DocumentHighlighter.hpp"
#include "Options.hpp"
#include "Plugin.hpp"

#define COMPILE_DLL_HIGHLIGHT
#include "syntaxhighlight.h"

#ifndef _WIN32_WCE
#include "codecvt.hpp"
#endif


/****************************************************************************************************/
/*                                 Declarations                                                     */
/****************************************************************************************************/

typedef pair<int, DocumentHighlighter*> MapEntry; 

extern "C"
const COLORREF* __stdcall getTextColorsForString(HWND hwndFrom,
                                                 unsigned int idFrom,
                                                 const wchar_t *str, 
                                                 unsigned int begin, 
                                                 unsigned int end,
                                                 unsigned int realNumber);

extern "C"
const COLORREF* __stdcall getBgColorsForString(HWND hwndFrom,
                                               unsigned int idFrom,
                                               const wchar_t *str, 
                                               unsigned int begin, 
                                               unsigned int end,
                                               unsigned int realNumber);


/****************************************************************************************************/
/*                                 Plugin                                                           */
/****************************************************************************************************/


class HighlightPlugin : public Plugin 
{
private:

  static const int MENU_STATIC_ITEMS = 1;


private:

  static HighlightPlugin *ourPlugin;


private:

  PluginService *myService;

  map<int, DocumentHighlighter*> highlighters;

  Options *myOptions;

  HMENU myMenuHandle;

  int myMenuItemsNumber;

  int myMenuFirstItemId;


public:

  static HighlightPlugin& GetInstance() 
  {
    if (!ourPlugin) 
      ourPlugin = new HighlightPlugin;
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

  HighlightPlugin() : myMenuHandle(0), myMenuFirstItemId(-1), myOptions(0) 
  {
    HL_INIT_MEMORY_SYSTEM();
  }  

  LPCWSTR GetGUID() const
  {
    return L"79f18d47-4419-4126-a2c2-43c45a2c19c4";
  }

  int GetVersion() const
  {
    return 2;
  }

  LPCWSTR GetName() const 
  {
    return L"Highlighter";
  }

  int GetPriority() const 
  {
    return 0xff;
  }

  bool Init(HMODULE hModule, PluginService *service)
  {
    std::wstring pluginRootPath = service->GetRootPath();

    myService = service;

    if (service->GetVersion() >= 2)
    { 
      HighlightParsersSource::getInstance().SetRootPath(pluginRootPath);
      TagWarehousesSource::GetInstance().SetRootPath(pluginRootPath);

      if (!HighlightParsersSource::getInstance().ReadFile((pluginRootPath + DIRECTORY_SEPARATOR + L"highlighters.syh").c_str()))
      {
        service->ShowError(L"Cannot read file highlighters.syh!");
        return false;
      }

      if (!TagWarehousesSource::GetInstance().ReadFile((pluginRootPath + DIRECTORY_SEPARATOR + L"tagwarehouses.syh").c_str()))
      {
        service->ShowError(L"Cannot read file tagwarehouses.syh!");
        return false;
      }

      if (!LanguagesSource::GetInstance().ReadFile((pluginRootPath + DIRECTORY_SEPARATOR + L"languages.syh").c_str())) 
      {
        service->ShowError(L"Cannot read file languages.syh!");
        return false;  
      }

      if (!TagWarehousesSource::GetInstance().Init())
      {
        std::wstring errorMessage;
        if (MessageCollector::GetInstance().GetMessage(error, errorMessage))
          service->ShowError(errorMessage.c_str());
        return false;
      }

      if (!LanguagesSource::GetInstance().Init())
      {
        std::wstring errorMessage;
        if (MessageCollector::GetInstance().GetMessage(error, errorMessage))
          service->ShowError(errorMessage.c_str());
        return false;
      }

      myOptions = new Options(pluginRootPath);
      myOptions->ReadOptions();

      initPluginParameters();
    }

    return true;    
  }

  void ExecuteCommand(__in LPCWSTR name) { /* not in use */ }

  void ExecuteFunction(__in LPCWSTR name, __inout void *param) { /* not in use */ }

  void ExecuteMenuCommand(__in int itemId)
  {
    int currentEditorId = myService->GetCurrentEditor();

    if (!handleError())
    {
      int itemIndex = itemId - myMenuFirstItemId;

      if (itemIndex == 0)  // auto-detect language
      {
        myOptions->SetDetectLanguageByExtension(!myOptions->IsDetectLanguageByExtension());
      } 
      /*else if (itemIndex == 1)
      {
        makeReport();
      }*/
      else if (itemIndex >= MENU_STATIC_ITEMS) // languages
      {
        int selectedLanguage = itemIndex - MENU_STATIC_ITEMS;

        const std::map<std::wstring, Language*> &languages = LanguagesSource::GetInstance().GetLanguagesMap();

        std::map<std::wstring, Language*>::const_iterator iterLanguage = languages.begin();
        int i = iterLanguage->second->IsExported() ? 0 : -1;

        if (i < selectedLanguage)
        {        
          do
          {
            iterLanguage++;
            if (iterLanguage->second->IsExported())         
              i++;
          } while (i < selectedLanguage);
        }

        setHighlighterForEditor(currentEditorId, iterLanguage->second, false);
      }
    }
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
    if (myMenuHandle && firstItemId != myMenuFirstItemId)
    {
      DestroyMenu(myMenuHandle);
      myMenuHandle = 0;
    }

    myMenuFirstItemId = firstItemId;

    if (!myMenuHandle && myMenuItemsNumber > 0)
      myMenuHandle = CreatePopupMenu();

    prepareMenu();

    return myMenuHandle;
  }

  int GetMenuItemsNumber()
  {
    return myMenuItemsNumber;
  }

  void ProcessNotification(const PluginNotification *notification)
  {
    switch (notification->notification)
    {
    case PN_FROM_EDITOR:
      onEditorNotification((NMHDR*)(notification->data));
      break;

    case PN_EDITOR_CREATED:
      onEditorFileChanged(*(int*)(notification->data), true);
      break;

    case PN_EDITOR_DESTROYED:
      onEditorClosed(*(int*)(notification->data));
      break;

    case PN_EDITOR_FILE_OPENED:
      onEditorFileChanged(*(int*)(notification->data), true);
      break;

    case PN_EDITOR_FILE_SAVED:
      onEditorFileChanged(*(int*)(notification->data), false);
      break;
    }
  }

  const COLORREF* GetTextColorsForString(HWND hwndFrom,
                                         unsigned int idFrom,
                                         const wchar_t *str, 
                                         unsigned int begin, 
                                         unsigned int end,
                                         unsigned int realNumber)
  {
    const DocumentHighlighter *phighlighter = prepareHighlighter(hwndFrom, idFrom, str, begin, end, realNumber);
    if (phighlighter != 0)
      return &phighlighter->GetTextColorBuffer()[0];
    return 0;
  }

  const COLORREF* GetBackgroundColorsForString(HWND hwndFrom,
                                               unsigned int idFrom,
                                               const wchar_t *str, 
                                               unsigned int begin, 
                                               unsigned int end,
                                               unsigned int realNumber)
  {
    const DocumentHighlighter *phighlighter = prepareHighlighter(hwndFrom, idFrom, str, begin, end, realNumber);
    if (phighlighter != 0)
      return &phighlighter->GetBgColorBuffer()[0];
    return 0;
  }


  ~HighlightPlugin()
  {
    for (map<int, DocumentHighlighter*>::const_iterator phighlighter = highlighters.begin(); phighlighter != highlighters.end(); ++phighlighter)
      delete phighlighter->second;
    highlighters.clear();

    if (myMenuHandle)
      DestroyMenu(myMenuHandle);

    TagWarehousesSource::GetInstance().Clear();    
    LanguagesSource::GetInstance().Clear();

    if (myOptions)
    {
      myOptions->SaveOptions();
      delete myOptions;
    }

    HL_CLOSE_MEMORY_SYSTEM();
  }


private:

  void onEditorNotification(const NMHDR *pnotification)
  {
    // Try to find highlighter for the document
    map<int, DocumentHighlighter*>::iterator iHighlighter = highlighters.find(pnotification->idFrom);
    if (iHighlighter == highlighters.end())
      return;

    DocumentHighlighter *phighlighter = iHighlighter->second;

    if (pnotification->code == SEN_CARETMODIFIED)
    {
      const sen_caret_modified_t *pcaretModifiedNotification = (const sen_caret_modified_t*)pnotification;
      phighlighter->HandleCaretModification(pcaretModifiedNotification->realPos.x, pcaretModifiedNotification->realPos.y);
    }
    else if (pnotification->code == SEN_DOCCHANGED)
    {
      const sen_doc_changed_t *pdocChangedNotification = (const sen_doc_changed_t*)pnotification;

      //MessageCollector::GetInstance().ClearMessages();

      switch (pdocChangedNotification->action)
      {
      case SEDIT_ACTION_INPUTCHAR:
      case SEDIT_ACTION_PASTE:
      case SEDIT_ACTION_NEWLINE:
        phighlighter->HandleTextModification(pdocChangedNotification->range, false);
        break;

      case SEDIT_ACTION_DELETECHAR:
      case SEDIT_ACTION_DELETEAREA:
      case SEDIT_ACTION_DELETELINE:
        phighlighter->HandleTextModification(pdocChangedNotification->range, true);
        break;

      case SEDIT_ACTION_REPLACEALL:
        phighlighter->HandleTextModification(pdocChangedNotification->range, false, true);
        break;
      }

      //makeReport();
    }
    else if (pnotification->code == SEN_DOCCLOSED)
    {
      delete phighlighter;
      highlighters.erase(iHighlighter); 
    }
  }

  void makeReport()
  {
    MessageCollector &collector = MessageCollector::GetInstance();

    int reportEditorId = findEditorWithName(L"hl_report");
    if (reportEditorId == -1)
    {
      reportEditorId = findEditorWithName(L"hl_report *");
      if (reportEditorId == -1)
      {     
        myService->Create(L"hl_report");
        reportEditorId = findEditorWithName(L"hl_report");
      }
    }

    HWND reportEditorWindow = myService->GetEditorWindow(reportEditorId);

    wstring message;

    while (collector.GetMessage(information, message))
    {
      SendMessage(reportEditorWindow, SE_PASTEFROMBUFFER, (WPARAM)message.c_str(), 0);
      SendMessage(reportEditorWindow, SE_PASTEFROMBUFFER, (WPARAM)L"\r\n", 0);
    }

    // Memory info
    /*static wchar_t buffer[256];
    wsprintf(buffer, L"Native allocations: %d\n", GetNativeAllocateCount());
    SendMessage(reportEditorWindow, SE_PASTEFROMBUFFER, (WPARAM)buffer, 0);

    wsprintf(buffer, L"Native deallocations: %d\n", GetNativeFreeCount());
    SendMessage(reportEditorWindow, SE_PASTEFROMBUFFER, (WPARAM)buffer, 0);*/
  }

  int findEditorWithName(const wstring &name)
  {
    static wchar_t editorNameBuffer[MAX_PATH];

    VectorPluginBuffer<int> ids;
    int editorId = -1;

    myService->GetAllEditorsIds(&ids);

    for (int i = 0; i < ids.GetBufferSize(); i++)
    {
      myService->GetEditorName(ids.GetBuffer()[i], editorNameBuffer);

      if (name.compare(editorNameBuffer) == 0)
      {
        editorId = ids.GetBuffer()[i];
        break;
      }
    }

    return editorId;
  }

  void onEditorFileChanged(int id, bool isLoading)
  {
    if (myOptions->IsDetectLanguageByExtension())
    {
      const Language *lang = determineLanguage(id);
      setHighlighterForEditor(id, lang, isLoading);
    }
    else if (isLoading)
      setHighlighterForEditor(id, 0, isLoading);
  }

  void onEditorClosed(int id)
  {
    deleteHighlighter(id);
  }

  const Language* determineLanguage(int id)
  {
    wchar_t editorFilePath[MAX_PATH];
    size_t length;

    myService->GetEditorFilePath(id, editorFilePath);
    length = wcslen(editorFilePath);

    if (handleError())
      return 0;

    int extStart = length - 1;
    std::wstring extension;
    while (extStart > 0)
    {
      if (editorFilePath[extStart] == L'.')
      {
        extension.assign(&editorFilePath[extStart + 1]);
        break;
      }
      extStart--;
    }

    if (extension.size() > 0)
      return LanguagesSource::GetInstance().GetLanguageByExtension(extension);
    return 0;
  }

  void setHighlighterForEditor(int id, const Language *lang, bool refreshAnyway)
  {
    DocumentHighlighter *phighlighter = findHighlighter(id);

    if (lang != 0)
    {
      HWND hEditorWnd = myService->GetEditorWindow(id);

      if (handleError())
        return;

      if (phighlighter == 0 || refreshAnyway || (phighlighter->GetDefaultLanguage()->GetName().compare(lang->GetName()) != 0))
      {
        deleteHighlighter(id);

        DocumentHighlighter *phighlighter = new DocumentHighlighter(hEditorWnd, lang->GetName(), myOptions->GetCaretLineTextColor(), myOptions->GetCaretLineBackgroundColor());
        highlighters.insert(MapEntry(id, phighlighter));
        SendMessage(hEditorWnd, SE_SETCOLORSFUNC, 0, (LPARAM)getTextColorsForString);
        SendMessage(hEditorWnd, SE_SETBKCOLORSFUNC, 0, (LPARAM)getBgColorsForString);

        //MessageCollector::GetInstance().ClearMessages();
        range_t range = {0};
        phighlighter->HandleTextModification(range, false, true);
        //makeReport();
      }
    } 
    else if (phighlighter != 0)
    {
      // just delete old highlighter.
      range_t fullRange = phighlighter->GetTextRange();
      HWND hwnd = phighlighter->GetEditorWindow();

      deleteHighlighter(id);
      
      SendMessage(hwnd, SE_UPDATERANGE, (WPARAM)SEDIT_REALLINE, (LPARAM)&fullRange);
    }
  }

  DocumentHighlighter* findHighlighter(int id)
  {
    map<int, DocumentHighlighter*>::iterator iHighlighter = highlighters.find(id);
    if (iHighlighter != highlighters.end())
      return iHighlighter->second;
    return 0;
  }

  void deleteHighlighter(int id)
  {
    map<int, DocumentHighlighter*>::iterator iHighlighter = highlighters.find(id);
    if (iHighlighter == highlighters.end())
      return;

    delete iHighlighter->second;
    highlighters.erase(iHighlighter);    
  }

  void initPluginParameters()
  {
    myMenuItemsNumber = MENU_STATIC_ITEMS;  // auto-detect language option

    const std::map<std::wstring, Language*> &languages = LanguagesSource::GetInstance().GetLanguagesMap();

    for (std::map<std::wstring, Language*>::const_iterator iterLanguage = languages.begin(); iterLanguage != languages.end(); ++iterLanguage)
    {
      if (iterLanguage->second->IsExported())
        myMenuItemsNumber++;
    }
  }

  void prepareMenu()
  {
    if (myMenuHandle)
    {
      int currentEditorId = myService->GetCurrentEditor();

      if (!handleError())
      {
        BOOL isItem = TRUE;
        while (isItem)
          isItem = RemoveMenu(myMenuHandle, 0, MF_BYPOSITION);

        int itemId = myMenuFirstItemId;

        // add options

        AppendMenu(myMenuHandle, MF_STRING | MF_ENABLED | (myOptions->IsDetectLanguageByExtension() ? MF_CHECKED : 0), itemId++, L"Auto-detect language");

        //AppendMenu(myMenuHandle, MF_STRING | MF_ENABLED, itemId++, L"Show log");

        AppendMenu(myMenuHandle, MF_SEPARATOR, 0, 0);


        // add languages

        DocumentHighlighter *phighlighter = currentEditorId >= 0 ? findHighlighter(currentEditorId) : 0;

        const std::map<std::wstring, Language*> &languages = LanguagesSource::GetInstance().GetLanguagesMap();

        for (std::map<std::wstring, Language*>::const_iterator iter = languages.begin(); iter != languages.end(); ++iter)
        {
          Language *language = iter->second;

          if (language->IsExported())
          {
            if (phighlighter && phighlighter->GetDefaultLanguage()->GetName().compare(language->GetName()) == 0)
              AppendMenu(myMenuHandle, MF_STRING | MF_ENABLED | MF_CHECKED, itemId++, language->GetExportName().c_str());
            else if (currentEditorId >= 0)
              AppendMenu(myMenuHandle, MF_STRING | MF_ENABLED, itemId++, language->GetExportName().c_str());
            else
              AppendMenu(myMenuHandle, MF_STRING | MF_GRAYED, itemId++, language->GetExportName().c_str());
          }
        }
      }
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

  const DocumentHighlighter* HighlightPlugin::prepareHighlighter(HWND hwndFrom,
                                                                 unsigned int idFrom,
                                                                 const wchar_t *str, 
                                                                 unsigned int begin, 
                                                                 unsigned int end,
                                                                 unsigned int realNumber)
  {
    // Try to find highlighter for the document
    map<int, DocumentHighlighter*>::iterator iter = highlighters.find(idFrom);
    if (iter == highlighters.end())
      return 0;

    DocumentHighlighter *phighlighter = iter->second;

    if (phighlighter != 0) 
    {
      range_t range;
      range.start.y = range.end.y = realNumber;
      range.start.x = begin;
      range.end.x = end;

      if (!HighlighterTools::AreEqual(phighlighter->GetLastColoredRange(), range))
      {
        // Lets cache the whole string
        size_t length = wcslen(str); // more correct is phighlighter->GetStringLength(realNumber);

        SimpleTextBlock textBlock(
            str,
            length,
            HighlighterTools::ConstructRange(0, realNumber, length, realNumber)
        );
        
        phighlighter->CacheText(textBlock);

        phighlighter->ColorRange(range);
      }
    }

    return phighlighter;  
  }
};

/****************************************************************************************************/
/*                                 External functions                                               */
/****************************************************************************************************/

extern "C"
Plugin* __stdcall SECreatePlugin()
{
  return &HighlightPlugin::GetInstance();
}

extern "C"
void __stdcall SEDeletePlugin(Plugin *plugin)
{
  HighlightPlugin::ReleaseInstance();
}

extern "C"
const COLORREF* __stdcall getTextColorsForString(HWND hwndFrom,
                                                 unsigned int idFrom,
                                                 const wchar_t *str, 
                                                 unsigned int begin, 
                                                 unsigned int end,
                                                 unsigned int realNumber)
{
  return HighlightPlugin::GetInstance().GetTextColorsForString(hwndFrom, idFrom, str, begin, end, realNumber);
}

extern "C"
const COLORREF* __stdcall getBgColorsForString(HWND hwndFrom,
                                               unsigned int idFrom,
                                               const wchar_t *str, 
                                               unsigned int begin, 
                                               unsigned int end,
                                               unsigned int realNumber)
{
  return HighlightPlugin::GetInstance().GetBackgroundColorsForString(hwndFrom, idFrom, str, begin, end, realNumber);
}

/****************************************************************************************************/
/*                                 Static initialization                                            */
/****************************************************************************************************/

HighlightPlugin* HighlightPlugin::ourPlugin(0);