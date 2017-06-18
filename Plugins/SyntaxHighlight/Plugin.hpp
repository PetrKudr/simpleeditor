#ifndef PluginStructures_h__
#define PluginStructures_h__

#include "seditcontrol.h"
#include "PluginTypes.hpp"
#include "PluginService.hpp"

#include <vector>
#include <string>


struct PluginHotKey
{
  bool shift;
  bool ctrl;
  wchar_t key;
};

struct PluginCommand
{
  LPCWSTR name;
  PluginHotKey *hotKey;
};

struct PluginAccessor
{
  virtual LPCWSTR GetGUID() const = 0;

  virtual int GetVersion() const = 0;

  virtual LPCWSTR GetName() const = 0;

  virtual int GetPriority() const = 0;

  virtual void ExecuteCommand(__in LPCWSTR name) = 0;

  virtual void ExecuteFunction(__in LPCWSTR name, __inout void *param) = 0;

  virtual void ExecuteMenuCommand(__in int itemId) = 0;
};

struct Plugin : PluginAccessor
{
  virtual bool Init(HMODULE hModule, PluginService *service) = 0;

  virtual const PluginCommand* GetSupportedCommands() = 0;

  virtual int GetSupportedCommandsNumber() = 0;

  virtual const LPCWSTR* GetSupportedFunctions() = 0;

  virtual int GetSupportedFunctionsNumber() = 0;

  virtual HMENU GetMenuHandle(int firstItemId) = 0;

  virtual int GetMenuItemsNumber() = 0;

  virtual void ProcessNotification(const PluginNotification *notification) = 0;

  virtual ~Plugin() {}
};


#ifdef UNDER_CE

  #define CREATE_PLUGIN_PROC L"SECreatePlugin"

  #define DELETE_PLUGIN_PROC L"SEDeletePlugin"

#else

  #define CREATE_PLUGIN_PROC "SECreatePlugin"

  #define DELETE_PLUGIN_PROC "SEDeletePlugin"

#endif


#ifdef __cplusplus
extern "C" {
#endif

typedef Plugin* (__stdcall *CreatePluginProc)();

typedef void (__stdcall *DeletePluginProc)(Plugin *plugin);

#ifdef __cplusplus
};
#endif

#endif // PluginStructures_h__