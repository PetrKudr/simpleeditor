#ifndef PluginService_h__
#define PluginService_h__


#include "PluginTypes.hpp"
#include "Encoder.hpp"

/**
 *  Service provides interface for interaction with CrossPad.
 *
 *  Please note that most functions of service (except the simplest ones) must be used only from the main thread of CrossPad. 
 *  If threads created by plugin itself need to perform complex operations via service, there is a special function @RequestCallback.
 */
struct PluginService
{
  /**
   * Returns current version of the service. 
   * Should be used for restricting work with old versions of CrossPad
   */
  virtual int GetVersion() const = 0;

  /**
   * Returns full path to the folder with plugin's library.
   */
  virtual LPCWSTR GetRootPath() const = 0;

  /**
   * Returns current language code (supposed to be ISO 639-1).
   */
  virtual LPCWSTR GetLanguage() const = 0;

  /**
   * Returns error code of the last error.
   */
  virtual int GetLastError() const = 0;

  /**
   * Fills buffer with error message of the last error.
   * Params:
   *   buffer - pointer to the buffer for the message
   */
  virtual void GetLastErrorMessage(__out PluginBuffer<wchar_t> *buffer) const = 0;

  /**
   * Shows error to a user.
   * Params:
   *   message - error message
   */
  virtual void ShowError(const LPCWSTR message) const = 0;

  /*
  **************************************************************
  *  Feedback from plugins
  **************************************************************
  */

  /**
   * Notifies other plugins that one of the parameters which could affect on them has been changed.
   * Params:
   *   PluginParameter - specifies changed parameter
   */
  virtual void NotifyParameterChanged(PluginParameter param) = 0;

  /**
   * Requests callback. Must be used only within threads created by plugin because they cannot directly call most of the functions of service.
   * After this call plugin receives notification PN_CALLBACK where it could call whatever it wants from service.
   * If parameter async is false then thread stops until notification processed, otherwise it returns immediately.
   *
   * Params:
   *   async - true to return immediately, false to stay while notification is not processed.
   *   param - parameter which would be in notification (see structure PluginNotification, field data).
   */
  virtual void RequestCallback(bool async, void *param) = 0;

  /*
  **************************************************************
  *  Editor::Getters
  **************************************************************
  */

  /**
   * Returns number of editors
   */
  virtual int GetEditorsNumber() = 0;

  /**
   * Returns ids of all editors
   * Params:
   *   buffer - buffer to store ids
   */
  virtual void GetAllEditorsIds(__out PluginBuffer<int> *buffer) = 0;

  /**
   * Returns id of the current editor
   */
  virtual int GetCurrentEditor() = 0;

  /**
   * Returns window handle of the specified editor
   * Params:
   *   id - id of the editor
   */
  virtual HWND GetEditorWindow(__in int id) = 0;

  /**
   * Returns path to the file opened in editor
   * Params:
   *   id - id of the editor
   *   path - buffer for path
   */
  virtual void GetEditorFilePath(__in int id, __out wchar_t path[MAX_PATH]) = 0;

  /**
   * Returns name of the editor (for editors with files inside them it is name of file)
   * Params:
   *   id - id of the editor
   *   name - buffer for name
   */
  virtual void GetEditorName(__in int id, __out wchar_t name[MAX_PATH]) = 0;

  /**
   * Returns id of the coding of editor
   * Params:
   *   id - id of the editor
   *   buffer - buffer for id of the coding
   */
  virtual void GetEditorCoding(__in int id, __out PluginBuffer<wchar_t> *buffer) = 0;

  /**
   * Returns true if editor should be saved with BOM (in fact true only if editor contains file and this file has BOM).
   * Params:
   *   id - id of the editor
   */
  virtual bool GetEditorBOMFlag(__in int id) = 0;

  /**
   * Returns true if content of the editor is modified.
   * Params:
   *   id - id of the editor
   */
  virtual bool GetEditorModificationFlag(__in int id) = 0;

  /*
  **************************************************************
  *  Editor::Actions
  **************************************************************
  */

  /**
   * Changes active editor.
   * Params:
   *   id - id of the editor
   */
  virtual void SetCurrentEditor(__in int id) = 0; 

  /**
   * Changes BOM flag of the editor.
   * Params:
   *   id - id of the editor
   *   bom - BOM flag
   */
  virtual void SetBOMFlag(__in int id, __in bool bom) = 0;

  /**
   * Changes modification flag of the editor.
   * Params:
   *   id - id of the editor
   *   modified - modification flag
   */
  virtual void SetEditorModificationFlag(__in int id, __in bool modified) = 0;

  /**
   * Opens file within new editor.
   * Params:
   *   path - path to the file
   *   coding - id of the coding to use
   *   activate - if true this editor would be active
   *   openInCurrent - if true new editor wouldn't be created
   *   reverved - not in use (must be null).
   */
  virtual void OpenEditor(__in LPCWSTR path, __in LPCWSTR coding, __in bool activate, __in bool openInCurrent, __in void *reserved) = 0;

  /**
   * Saves content of the editor.
   * Params:
   *   id - id of the editor
   *   path - path to the file
   *   coding - id of the coding to use
   *   bom - true if BOM is needed
   *   reverved - not in use (must be null).
   */
  virtual void SaveEditor(__in int id, __in LPCWSTR path, __in LPCWSTR coding, __in bool bom, __in void *reserved) = 0;

  /**
   * Creates new editor.
   * Params:
   *   caption - name of the editor
   */
  virtual void Create(__in LPCWSTR caption) = 0;

  /**
   * Closes editor.
   * Params:
   *   id - id of the editor
   */
  virtual void Close(__in int id) = 0;

  /*
  **************************************************************
  *  Encoders
  **************************************************************
  */

  /**
   * Registers encoder.
   * Params:
   *   encoder - encoder
   * Returns true if encoder registered successfully
   */
  virtual bool RegisterEncoder(Encoder *encoder) = 0;

  /**
   * Unregisters encoder.
   * Params:
   *   id - id of encoder
   * Returns true if encoder unregistered successfully
   */
  virtual bool UnregisterEncoder(LPCWSTR id) = 0;  

};

#endif // PluginService_h__