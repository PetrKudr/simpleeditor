#ifndef PluginService_h__
#define PluginService_h__

#include <string>
#include <vector>


#include "PluginTypes.hpp"
#include "Encoder.hpp"


struct PluginService
{
  virtual int GetVersion() const = 0;

  virtual LPCWSTR GetRootPath() const = 0;

  virtual LPCWSTR GetLanguage() const = 0;

  virtual int GetLastError() const = 0;

  virtual void GetLastErrorMessage(__out PluginBuffer<wchar_t> *buffer) const = 0;

  virtual void ShowError(const LPCWSTR message) const = 0;

/*
  **************************************************************
  *  Feedback from plugins
  **************************************************************
  */

  virtual void NotifyParameterChanged(PluginParameter param) = 0;

  virtual void RequestCallback(bool async, void *param) = 0;

  /*
  **************************************************************
  *  Editor::Getters
  **************************************************************
  */

  virtual int GetEditorsNumber() = 0;

  virtual void GetAllEditorsIds(__out PluginBuffer<int> *buffer) = 0;

  virtual int GetCurrentEditor() = 0;

  virtual HWND GetEditorWindow(__in int id) = 0;

  virtual void GetEditorFilePath(__in int id, __out wchar_t path[MAX_PATH]) = 0;

  virtual void GetEditorName(__in int id, __out wchar_t name[MAX_PATH]) = 0;

  virtual void GetEditorCoding(__in int id, __out PluginBuffer<wchar_t> *buffer) = 0;

  virtual bool GetEditorBOMFlag(__in int id) = 0;

  virtual bool GetEditorModificationFlag(__in int id) = 0;

  /*
  **************************************************************
  *  Editor::Actions
  **************************************************************
  */

  virtual void SetCurrentEditor(__in int id) = 0; 

  virtual void SetBOMFlag(__in int id, __in bool bom) = 0;

  virtual void SetEditorModificationFlag(__in int id, __in bool modified) = 0;

  virtual void OpenEditor(__in LPCWSTR path, __in LPCWSTR coding, __in bool activate, __in bool openInCurrent, __in void *reserved) = 0;

  virtual void SaveEditor(__in int id, __in LPCWSTR path, __in LPCWSTR coding, __in bool bom, __in void *reserved) = 0;

  virtual void Create(__in LPCWSTR caption) = 0;

  virtual void Close(__in int id) = 0;

  /*
  **************************************************************
  *  Encoders
  **************************************************************
  */

  virtual bool RegisterEncoder(Encoder *encoder) = 0;

  virtual bool UnregisterEncoder(LPCWSTR id) = 0;  
};

#endif // PluginService_h__