#ifndef PluginManager_h__
#define PluginManager_h__

#include <Windows.h>

#include <string>
#include <list>
#include <vector>

#include "error.hpp"
#include "Plugin.hpp"


class PersonalPluginService : public PluginService
{
private:

  static const int PS_VERSION = 3;   // should be updated with every version

  static const int PSE_NO_ERROR = 0;
  static const int PSE_EDITOR_NOT_EXIST = 1;


private:

  NotepadError myLastError;

  LPCWSTR myPluginGuid;

  std::wstring myPluginRootPath;


public:

  PersonalPluginService(__in LPCWSTR guid, __in const std::wstring &pluginRootPath);

  int GetVersion() const;

  LPCWSTR GetRootPath() const;

  LPCWSTR GetLanguage() const;

  void RequestCallback(bool async, void *param);

  int GetEditorsNumber();

  void GetAllEditorsIds(__out PluginBuffer<int> *buffer);

  int GetCurrentEditor();

  HWND GetEditorWindow(int id);

  void GetEditorFilePath(__in int id, __out wchar_t path[MAX_PATH]);

  void GetEditorName(__in int id, __out wchar_t name[MAX_PATH]);

  void GetEditorCoding(__in int id, __out PluginBuffer<wchar_t> *buffer);

  bool GetEditorBOMFlag(__in int id);

  bool GetEditorModificationFlag(__in int id);

  void SetCurrentEditor(__in int id);

  void SetBOMFlag(__in int id, __in bool bom);

  void SetEditorModificationFlag(__in int id, __in bool modified);

  void OpenEditor(__in LPCWSTR path, __in LPCWSTR coding, __in bool activate, __in bool openInCurrent, __in void *reserved);

  void SaveEditor(__in int id, __in LPCWSTR path, __in LPCWSTR coding, __in bool bom, __in void *reserved);

  void Create(__in LPCWSTR caption);

  void Close(__in int id);

  void NotifyParameterChanged(PluginParameter param);

  bool RegisterEncoder(Encoder *encoder);

  bool UnregisterEncoder(LPCWSTR id);

  int GetLastError() const;

  void GetLastErrorMessage(__out PluginBuffer<wchar_t> *buffer) const;

  void ShowError(const LPCWSTR message) const;


private:

  void checkError();

};


class InternalPlugin
{
private:

  Plugin *myPlugin;

  PersonalPluginService *myService;

  HMODULE myModuleHandle;

  const std::wstring myPath;

  DeletePluginProc myDeleteProc;

  int myFirstMenuId, myLastMenuId;


public:

  InternalPlugin(Plugin *plugin, 
                 PersonalPluginService *service, 
                 HMODULE myModuleHandle, 
                 const std::wstring &path, 
                 DeletePluginProc deleteProc) 
                 : myPlugin(plugin), myService(service), myModuleHandle(myModuleHandle), myPath(path), myDeleteProc(deleteProc) {}

  Plugin* GetPlugin() const
  {
    return myPlugin;
  }

  PersonalPluginService* GetService() const
  {
    return myService;
  }

  HMODULE GetModuleHandle() const
  {
    return myModuleHandle;
  }

  const std::wstring& GetPath() const 
  {
    return myPath;
  }

  DeletePluginProc GetDeletePluginProc() const
  {
    return myDeleteProc;
  }

  int GetPluginMenuFirstId() const
  {
    return myFirstMenuId;
  }

  void SetPluginMenuFirstId(int id)
  {
    myFirstMenuId = id;
  }

  int GetPluginMenuLastId() const
  {
    return myLastMenuId;
  }

  void SetPluginMenuLastId(int id)
  {
    myLastMenuId = id;
  }

  ~InternalPlugin()
  {
    if (myPlugin != 0 && myDeleteProc != 0)
      myDeleteProc(myPlugin);
    if (myService != 0)
      delete myService;   
    if (myModuleHandle != 0)
      FreeLibrary(myModuleHandle); 
  }
};


class CPluginManager 
{
private:

  std::wstring root;

  std::list<InternalPlugin*> myPlugins;


public:

  bool Init(const std::wstring &root);

  const std::list<InternalPlugin*>& ListPlugins();

  const InternalPlugin* GetPlugin(LPCWSTR guid);

  bool EnablePlugin(LPCWSTR guid);  // not implemented

  bool DisablePlugin(LPCWSTR guid);  // not implemented

  void NotifyPlugin(LPCWSTR guid, const PluginNotification &notification);

  void NotifyPlugins(const PluginNotification &notification);

  void ExecuteMenuCommand(int commandId); 

  void ProcessPluginParameterChange(LPCWSTR guid, PluginParameter param);

  ~CPluginManager();


private:

  bool listPluginsInPath(__in const std::wstring &root, __out std::list<std::wstring> &plugins);

  LPCWSTR loadPlugin(__in const std::wstring &path);

  bool unloadPlugin(LPCWSTR guid);

  void preparePlugins();

  void preparePluginsMenuItems();

  void onPluginMenuChange();

};

#endif // PluginManager_h__