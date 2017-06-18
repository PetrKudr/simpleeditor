#include "PluginsManager.hpp"
#include "PluginUtils.hpp"
#include "resources\res.hpp"
#include "notepad.hpp"
#include "base/variables.h"

#include <sstream>
#include <list>
#include <algorithm>
#include <cwchar>

using namespace std;


/*
********************************************************************************************************
*                                           Personal Service                                           *
********************************************************************************************************
*/

PersonalPluginService::PersonalPluginService(__in LPCWSTR guid, __in const std::wstring &pluginRootPath) : myPluginGuid(guid), myPluginRootPath(pluginRootPath) {}

int PersonalPluginService::GetVersion() const
{
  return PS_VERSION;
}

LPCWSTR PersonalPluginService::GetRootPath() const
{
  return myPluginRootPath.c_str();
}

LPCWSTR PersonalPluginService::GetLanguage() const
{
  return g_languageName;
}

void PersonalPluginService::RequestCallback(bool async, void *param)
{
  //wchar_t *box = new wchar_t[wcslen(myPluginGuid) + 1];
  //wcscpy(box, myPluginGuid);

  CNotepad &notepad = CNotepad::Instance();

  if (async)  
    PostMessage(notepad.Interface.GetHWND(), NPD_EMITPLUGINNOTIFICATION, (WPARAM)myPluginGuid, (LPARAM)param);
  else 
    SendMessage(notepad.Interface.GetHWND(), NPD_EMITPLUGINNOTIFICATION, (WPARAM)myPluginGuid, (LPARAM)param);
}

int PersonalPluginService::GetEditorsNumber()
{
  CNotepad &notepad = CNotepad::Instance();
  return notepad.Documents.GetDocsNumber();
}

void PersonalPluginService::GetAllEditorsIds(__out PluginBuffer<int> *buffer)
{
  std::vector<int> ids;
  CNotepad &notepad = CNotepad::Instance();
  notepad.Documents.GetDocIds(ids);

  if (ids.size() > buffer->GetBufferSize())
    buffer->IncreaseBufferSize(ids.size() - buffer->GetBufferSize());

  int *unwrappedBuffer = buffer->GetBuffer();
  for (size_t i = 0; i < ids.size(); i++)
    unwrappedBuffer[i] = ids[i];

  checkError();
}

int PersonalPluginService::GetCurrentEditor()
{
  CNotepad &notepad = CNotepad::Instance();
  int id = notepad.Documents.GetIdFromIndex(notepad.Documents.Current());
  checkError();
  return id;
}

void PersonalPluginService::SetCurrentEditor(__in int id)
{
  CNotepad &notepad = CNotepad::Instance();
  notepad.Documents.SetActive(notepad.Documents.GetIndexFromID(id));
  checkError();
}

void PersonalPluginService::SetBOMFlag(__in int id, __in bool bom)
{
  CNotepad &notepad = CNotepad::Instance();
  notepad.Documents.SetBOM(notepad.Documents.GetIndexFromID(id), bom);
  checkError();
}

void PersonalPluginService::SetEditorModificationFlag(__in int id, __in bool modified)
{
  CNotepad &notepad = CNotepad::Instance();
  notepad.Documents.SetModified(notepad.Documents.GetIndexFromID(id), modified);
  checkError();
}

void PersonalPluginService::OpenEditor(__in LPCWSTR path, __in LPCWSTR coding, __in bool activate, __in bool openInCurrent, __in void *reserved)
{
  CNotepad &notepad = CNotepad::Instance();
  notepad.Documents.Open(path, coding, OPEN, activate, openInCurrent);
  checkError();
}

void PersonalPluginService::SaveEditor(__in int id, __in LPCWSTR path, __in LPCWSTR coding, __in bool bom, __in void *reserved)
{
  CNotepad &notepad = CNotepad::Instance();
  notepad.Documents.Save(notepad.Documents.GetIndexFromID(id), path, coding, bom);
  checkError();
}

void PersonalPluginService::Create(__in LPCWSTR caption)
{
  CNotepad &notepad = CNotepad::Instance();
  notepad.Documents.Create(caption);
  checkError();
}

void PersonalPluginService::Close(__in int id)
{
  CNotepad &notepad = CNotepad::Instance();
  notepad.Documents.Close(notepad.Documents.GetIndexFromID(id));
  checkError();
}

HWND PersonalPluginService::GetEditorWindow(int id)
{
  CNotepad &notepad = CNotepad::Instance();
  HWND hwnd = notepad.Documents.GetHWND(notepad.Documents.GetIndexFromID(id));
  checkError();
  return hwnd;
}

void PersonalPluginService::GetEditorFilePath(__in int id, __out wchar_t path[MAX_PATH])
{
  CNotepad &notepad = CNotepad::Instance();
  wcscpy(path, notepad.Documents.GetPath(notepad.Documents.GetIndexFromID(id)).c_str());
  checkError();
}

void PersonalPluginService::GetEditorName(__in int id, __out wchar_t name[MAX_PATH])
{
  CNotepad &notepad = CNotepad::Instance();
  wcscpy(name, notepad.Documents.GetName(notepad.Documents.GetIndexFromID(id)).c_str());
  checkError();
}

void PersonalPluginService::GetEditorCoding(__in int id, __out PluginBuffer<wchar_t> *buffer)
{
  CNotepad &notepad = CNotepad::Instance();

  LPCWSTR coding = notepad.Documents.GetCoding(notepad.Documents.GetIndexFromID(id));
  size_t codingLength = coding != 0 ? wcslen(coding) : 0;

  if (buffer->GetBufferSize() < codingLength)
    buffer->IncreaseBufferSize(buffer->GetBufferSize() - codingLength + 1);
  wcscpy(buffer->GetBuffer(), coding);
  checkError();
}

bool PersonalPluginService::GetEditorBOMFlag(__in int id)
{
  CNotepad &notepad = CNotepad::Instance();
  bool bomFlag = notepad.Documents.NeedBOM(notepad.Documents.GetIndexFromID(id));
  checkError();
  return bomFlag;
}

bool PersonalPluginService::GetEditorModificationFlag(__in int id)
{
  CNotepad &notepad = CNotepad::Instance();
  bool modified = notepad.Documents.IsModified(notepad.Documents.GetIndexFromID(id));
  checkError();
  return modified;
}

void PersonalPluginService::NotifyParameterChanged(PluginParameter param)
{
  CNotepad &Notepad = CNotepad::Instance();
  Notepad.PluginManager.ProcessPluginParameterChange(myPluginGuid, param);
}

bool PersonalPluginService::RegisterEncoder(Encoder *encoder)
{
  CNotepad &Notepad = CNotepad::Instance();
  return Notepad.Documents.CodingManager.RegisterEncoder(encoder);
}

bool PersonalPluginService::UnregisterEncoder(LPCWSTR id)
{
  CNotepad &Notepad = CNotepad::Instance();
  return Notepad.Documents.CodingManager.UnregisterEncoder(id);
}

int PersonalPluginService::GetLastError() const
{
  return myLastError.code;
}

void PersonalPluginService::GetLastErrorMessage(__out PluginBuffer<wchar_t> *buffer) const
{
  if (buffer->GetBufferSize() < myLastError.message.size())
    buffer->IncreaseBufferSize(myLastError.message.size() - buffer->GetBufferSize() + 1);
  wcscpy(buffer->GetBuffer(), myLastError.message.c_str());
}

void PersonalPluginService::ShowError(const LPCWSTR message) const
{
  CNotepad &notepad = CNotepad::Instance();
  std::wstring msg(message);
  notepad.Interface.Dialogs.ShowError(notepad.Interface.GetHWND(), msg);
}

void PersonalPluginService::checkError()
{
  const NotepadError &error = CError::GetError();

  myLastError.code = error.code;
  if (myLastError.message.compare(error.message) != 0)
    myLastError.message = error.message;

  if (error.code != NOTEPAD_NO_ERROR)
    CError::SetError(NOTEPAD_NO_ERROR);
}

/*
********************************************************************************************************
*                                           Plugin Manager                                             *
********************************************************************************************************
*/

struct PluginRootsCollector
{
  list<wstring> pluginRoots;

  void operator()(const WIN32_FIND_DATA &fd)
  {
    if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
      if (wcscmp(fd.cFileName, L".") != 0 && wcscmp(fd.cFileName, L"..") != 0)
        pluginRoots.push_back(wstring(fd.cFileName));
    }
  }
};

struct PluginsCollector
{
  list<wstring> plugins;

  void operator()(const WIN32_FIND_DATA &fd)
  {
    if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
      plugins.push_back(wstring(fd.cFileName));
  }
};

static bool comaparePluginsByPriority(const InternalPlugin *first, const InternalPlugin *second)
{
  return first->GetPlugin()->GetPriority() < second->GetPlugin()->GetPriority();
}



bool CPluginManager::Init(const wstring &root)
{
  this->root = root;

  list<wstring> plugins;
  if (!listPluginsInPath(root, plugins))
    return false;

  for (list<wstring>::const_iterator iter = plugins.begin(); iter != plugins.end(); iter++)
  {
    if (!loadPlugin(*iter))
    {
      CNotepad &Notepad = CNotepad::Instance();
      Notepad.Interface.Dialogs.ShowError(Notepad.Interface.GetHWND(), CError::GetError().message);
      CError::SetError(NOTEPAD_NO_ERROR);
    }
  }

  myPlugins.sort(comaparePluginsByPriority);

  preparePlugins();

  return true;
}

const list<InternalPlugin*>& CPluginManager::ListPlugins()
{
  return myPlugins;
}

const InternalPlugin* CPluginManager::GetPlugin(LPCWSTR guid)
{
  for (list<InternalPlugin*>::const_iterator iter = myPlugins.begin(); iter != myPlugins.end(); iter++)
  {
    if (wcscmp((*iter)->GetPlugin()->GetGUID(), guid) == 0)
      return *iter;
  }
  return 0;
}

bool CPluginManager::EnablePlugin(LPCWSTR guid)
{
  return false;
}

bool CPluginManager::DisablePlugin(LPCWSTR guid)
{
  return false;
}

void CPluginManager::NotifyPlugin(LPCWSTR guid, const PluginNotification &notification)
{
  const InternalPlugin *internalPlugin = GetPlugin(guid);
  if (internalPlugin)
    internalPlugin->GetPlugin()->ProcessNotification(&notification);
}

void CPluginManager::NotifyPlugins(const PluginNotification &notification)
{
  for (list<InternalPlugin*>::const_iterator iter = myPlugins.begin(); iter != myPlugins.end(); iter++)
    (*iter)->GetPlugin()->ProcessNotification(&notification);
}

void CPluginManager::ExecuteMenuCommand(int commandId)
{
  for (list<InternalPlugin*>::const_iterator iter = myPlugins.begin(); iter != myPlugins.end(); iter++)
  {
    InternalPlugin* internalPlugin = *iter;

    if (commandId >= internalPlugin->GetPluginMenuFirstId() && commandId < internalPlugin->GetPluginMenuLastId())
    {
      Plugin *plugin = internalPlugin->GetPlugin();
      plugin->ExecuteMenuCommand(commandId);
      break;
    }
  }
}

void CPluginManager::ProcessPluginParameterChange(LPCWSTR guid, PluginParameter param)
{
  switch (param)
  {
  case PP_MENU:
    break;
  }
}

CPluginManager::~CPluginManager()
{
  list<LPCWSTR> ids;

  for (list<InternalPlugin*>::const_iterator iter = myPlugins.begin(); iter != myPlugins.end(); iter++)
    ids.push_back((*iter)->GetPlugin()->GetGUID());

  for (list<LPCWSTR>::const_iterator iter = ids.begin(); iter != ids.end(); iter++)
    unloadPlugin(*iter);
}

bool CPluginManager::listPluginsInPath(__in const wstring &root, __out list<wstring> &plugins)
{
  wstring errorMessage;

  PluginRootsCollector rootsCollector;

  PluginUtils::SearchFiles(root + wstring(FILE_SEPARATOR) + wstring(L"*"), rootsCollector);

  for (list<wstring>::const_iterator rootIter = rootsCollector.pluginRoots.begin(); rootIter != rootsCollector.pluginRoots.end(); rootIter++)
  {
    PluginsCollector pluginsCollector;
    PluginUtils::SearchFiles(root + FILE_SEPARATOR + (*rootIter) + FILE_SEPARATOR + wstring(L"*.dll"), pluginsCollector);

    for (list<wstring>::const_iterator pluginIter = pluginsCollector.plugins.begin(); pluginIter != pluginsCollector.plugins.end(); pluginIter++)
      plugins.push_back((*rootIter) + FILE_SEPARATOR + (*pluginIter));
  }

  return true;
}

LPCWSTR CPluginManager::loadPlugin(__in const wstring &path)
{
  // Check if plugin is already loaded
  for (list<InternalPlugin*>::const_iterator iter = myPlugins.begin(); iter != myPlugins.end(); iter++)
  {
    if ((*iter)->GetPath().compare(path) == 0)
      return (*iter)->GetPlugin()->GetGUID();
  }

  std::wstring pluginPath = root + FILE_SEPARATOR + path;

  HMODULE hmodule = LoadLibrary(pluginPath.c_str());

  if (!hmodule)
  {
    CError::SetError(NOTEPAD_CANNOT_LOAD_DLL, pluginPath.c_str());    
    goto plugin_load_error;
  }

  Plugin *plugin = 0;
  PersonalPluginService *service = 0;

  CreatePluginProc createProc = (CreatePluginProc)GetProcAddress(hmodule, CREATE_PLUGIN_PROC);
  DeletePluginProc deleteProc = (DeletePluginProc)GetProcAddress(hmodule, DELETE_PLUGIN_PROC);

  if (0 != createProc && 0 != deleteProc)
  {
    plugin = createProc();

    if (plugin != 0)
    {
      // Check if we already have plugin with the same guid
      for (list<InternalPlugin*>::const_iterator iter = myPlugins.begin(); iter != myPlugins.end(); iter++)
      {
        if (wcscmp((*iter)->GetPlugin()->GetGUID(), plugin->GetGUID()) == 0)
        {
          CError::SetError(NOTEPAD_CANNOT_INITIALIZE_PLUGIN, plugin->GetName());
          goto plugin_load_error;
        }
      }

      // Initialize plugin
      service = new PersonalPluginService(plugin->GetGUID(), pluginPath.substr(0, pluginPath.rfind(L'\\')));
      if (plugin->Init(hmodule, service))
      {
        InternalPlugin *internalPlugin = new InternalPlugin(plugin, service, hmodule, path, deleteProc);  
        myPlugins.push_back(internalPlugin);
        return plugin->GetGUID();
      }
      else
      {
        CError::SetError(NOTEPAD_CANNOT_INITIALIZE_PLUGIN, plugin->GetName());
        goto plugin_load_error;
      }  
    }
  }
  else 
  {
    CError::SetError(NOTEPAD_DLL_DOES_NOT_CONTAIN_PLUGIN, pluginPath.c_str());
    goto plugin_load_error;
  }


plugin_load_error:

  if (plugin)
    deleteProc(plugin);
  if (service)
    delete service;
  if (hmodule)
    FreeLibrary(hmodule);  

  return 0;
}

bool CPluginManager::unloadPlugin(LPCWSTR guid)
{
  InternalPlugin *descriptor = 0;

  for (list<InternalPlugin*>::const_iterator iter = myPlugins.begin(); iter != myPlugins.end(); iter++)
  {
    if (wcscmp((*iter)->GetPlugin()->GetGUID(), guid) == 0)
    {
      descriptor = *iter;
      myPlugins.erase(iter);
      break;
    }
  }

  if (descriptor != 0)
    delete descriptor;

  return true;
}

 void CPluginManager::preparePlugins()
 {
   preparePluginsMenuItems();
 }

void CPluginManager::preparePluginsMenuItems()
{
  int currentMenuId = IDM_FIRST_PLUGIN_COMMAND;

  for (list<InternalPlugin*>::const_iterator iter = myPlugins.begin(); iter != myPlugins.end(); iter++)
  {
    InternalPlugin *internalPlugin = *iter;

    int itemsNumber = internalPlugin->GetPlugin()->GetMenuItemsNumber();
    if (itemsNumber > 0)
    {
      internalPlugin->SetPluginMenuFirstId(currentMenuId);
      internalPlugin->SetPluginMenuLastId(currentMenuId += itemsNumber);
    }
    else 
    {
      internalPlugin->SetPluginMenuFirstId(-1);
      internalPlugin->SetPluginMenuLastId(-1);
    }
  }
}

void CPluginManager::onPluginMenuChange()
{
  CNotepad &Notepad = CNotepad::Instance();
  Notepad.ClearPluginsMenu(Notepad.Interface.Controlbar.GetPluginsMenu());
  Notepad.ClearPluginsMenu(Notepad.Interface.Controlbar.GetPluginsPopupMenu());

  preparePluginsMenuItems();
}