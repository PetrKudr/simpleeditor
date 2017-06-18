//#include "base\protect.h"
//#include <varargs.h>
#include "base\files.h"
#include "base\register.h"
#include "seditcontrol.h"
#include "error.hpp"


#define CUSTOM_MESSAGE_MAX_LENGTH 512


// Error messages
static std::wstring errMsg[COUNT] =
{
  L"No error",                                     // NOTEPAD_NO_ERROR
  L"Unknown error code",                           // NOTEPAD_UNKNOWN_ERRORCODE
  L"Register error",                               // NOTEPAD_REGISTER_ERROR
  L"Edit contol initialization error",             // NOTEPAD_INIT_EDITCTRL_ERROR
  L"Common controls initialization error",         // NOTEPAD_INIT_COMMCTRL_ERROR
  L"Limit of the number of documents reached",     // NOTEPAD_DOCUMENTS_LIMIT_ERROR
  L"Failed to create new document",                // NOTEPAD_CREATE_DOCUMENT_ERROR
  L"Requested document does not exist",            // NOTEPAD_DOCUMENT_NOT_EXIST
  L"Document is busy",                             // NOTEPAD_DOCUMENT_IS_BUSY 
  L"Edit contol error",                            // NOTEPAD_EDITCTRL_ERROR
  L"Memory error",                                 // NOTEPAD_MEMORY_ERROR
  L"Window does not exist",                        // NOTEPAD_WINDOW_DOES_NOT_EXIST
  L"Cannot create window",                         // NOTEPAD_CREATE_WINDOW_ERROR
  L"Cannot create thread",                         // NOTEPAD_CREATE_THREAD_ERROR
  L"Cannot load resource",                         // NOTEPAD_LOAD_RESOURCE_ERROR
  L"Cannot open file",                             // NOTEPAD_OPEN_FILE_ERROR
  L"Cannot save file",                             // NOTEPAD_SAVE_FILE_ERROR
  L"Cannot initialize notepad",                    // NOTEPAD_INIT_ERROR
  L"Cannot close document",                        // NOTEPAD_CLOSE_DOC_ERROR
  L"Cannot get submenu",                           // NOTEPAD_GET_SUBMENU_ERROR
  L"Cannot create menu",                           // NOTEPAD_CREATE_MENU_ERROR
  L"Cannot create resource",                       // NOTEPAD_CREATE_RESOURCE_ERROR
  L"Proporties file is corrupted",                 // NOTEPAD_CORRUPTED_OPTIONS_FILE
  L"Dialog error",                                 // NOTEPAD_DIALOG_ERROR
  L"Recent file is corrupted",                     // NOTEPAD_CORRUPTED_RECENT_FILE
  L"Cannot save recent",                           // NOTEPAD_CANNOT_SAVE_RECENT
  L"Cannot register extention",                    // NOTEPAD_REGISTER_EXTENTION_ERROR
  L"Cannot unregister extention",                  // NOTEPAD_UNREGISTER_EXTENTION_ERROR
  L"Cannot save options",                          // NOTEPAD_CANNOT_SAVE_OPTIONS
  L"Invalid language library: %.128s",             // NOTEPAD_CORRUPTED_LANG_DLL
  L"Cannot search files in '%.128s'",              // NOTEPAD_CANNOT_SEARCH_FILES_IN_PATH
  L"Cannot load DLL '%.128s'",                     // NOTEPAD_CANNOT_LOAD_DLL
  L"DLL '%.128s' does not contain plugin",         // NOTEPAD_DLL_DOES_NOT_CONTAIN_PLUGIN
  L"Cannot initialize plugin '%.128s'",            // NOTEPAD_CANNOT_INITIALIZE_PLUGIN
  L"Invalid parameter",                            // NOTEPAD_EDITOR_INVALID_PARAMETER
  L"Error during copying",                         // NOTEPAD_EDITOR_COPY_ERROR
  L"Error during searching",                       // NOTEPAD_EDITOR_SEARCH_ERROR
  L"Cannot create brush",                          // NOTEPAD_EDITOR_CREATE_BRUSH_ERROR
  L"Cannot create pen",                            // NOTEPAD_EDITOR_CREATE_PEN_ERROR
  L"Control is not initialized for appending",     // NOTEPAD_EDITOR_APPEND_NOT_INITIALIZED
  L"Control is not initialized for receiving",     // NOTEPAD_EDITOR_RECEIVE_NOT_INITIALIZED
  L"Editor cannot get its system storage",         // NOTEPAD_EDITOR_WINDOW_INFO_UNAVAILABLE
  L"Too many break symbols",                       // NOTEPAD_EDITOR_BREAKSYMS_LIMIT_REACHED
  L"Editors window is blocked",                    // NOTEPAD_EDITOR_WINDOW_IS_BLOCKED
  L"Wrong replace pattern",                        // NOTEPAD_EDITOR_WRONG_REPLACE_PATTERN
  L"Wrong version of language library: %.128s"     // NOTEPAD_LANG_DLL_VERSION_ERROR
};


static NotepadError s_error;


static void constructErrorMessage(__out std::wstring &errmsg, __in int err, __in va_list params)
{
  if (err >= 0 && err < COUNT)
  {
    wchar_t buffer[CUSTOM_MESSAGE_MAX_LENGTH];
#ifdef UNDER_CE
    vswprintf(buffer, errMsg[err].c_str(), params);
#else
    vswprintf(buffer, CUSTOM_MESSAGE_MAX_LENGTH, errMsg[err].c_str(), params);
#endif
    errmsg.assign(buffer);
  }
  else 
    errmsg.assign(errMsg[NOTEPAD_UNKNOWN_ERRORCODE]);
}


extern void CError::SetError(int err, ...)
{
  s_error.code = err;  
  if (NOTEPAD_NO_ERROR != err)
  {
    va_list params;
    va_start(params, err);
    constructErrorMessage(s_error.message, err, params);
    va_end(params);
  }
  else
    s_error.message.assign(errMsg[NOTEPAD_NO_ERROR]);
}

extern void CError::SetError(const NotepadError &error)
{
  s_error.code = error.code;
  s_error.message.assign(error.message);
}

extern const NotepadError& CError::GetError()
{
  return s_error;
}

extern bool CError::HasError()
{
  return s_error.code != NOTEPAD_NO_ERROR;
}

// Function converts error from Simple Edit Control to Notepad error
extern int CError::TranslateEditor(int err)
{
  switch (err)
  {
  case SEDIT_MEMORY_ERROR:
    return NOTEPAD_MEMORY_ERROR;

  case SEDIT_INITIALIZE_ERROR:
    return NOTEPAD_INIT_EDITCTRL_ERROR;

  case SEDIT_REGISTER_CLASS_ERROR:
    return NOTEPAD_REGISTER_ERROR;

  case SEDIT_INVALID_PARAM:
    return NOTEPAD_EDITOR_INVALID_PARAMETER;

  case SEDIT_COPY_ERROR:
    return NOTEPAD_EDITOR_COPY_ERROR;    

  case SEDIT_SEARCH_ERROR:
    return NOTEPAD_EDITOR_SEARCH_ERROR;

  case SEDIT_CREATE_BRUSH_ERROR:
    return NOTEPAD_EDITOR_CREATE_BRUSH_ERROR;

  case SEDIT_CREATE_PEN_ERROR:
    return NOTEPAD_EDITOR_CREATE_PEN_ERROR;

  case SEDIT_APPEND_NOT_INITIALIZED:  
    return NOTEPAD_EDITOR_APPEND_NOT_INITIALIZED;

  case SEDIT_RECEIVE_NOT_INITIALIZED:
    return NOTEPAD_EDITOR_RECEIVE_NOT_INITIALIZED;

  case SEDIT_WINDOW_INFO_UNAVAILABLE:
    return NOTEPAD_EDITOR_WINDOW_INFO_UNAVAILABLE;

  case SEDIT_BREAKSYMS_LIMIT_REACHED:
    return NOTEPAD_EDITOR_BREAKSYMS_LIMIT_REACHED;

  case SEDIT_WINDOW_IS_BLOCKED:
    return NOTEPAD_EDITOR_WINDOW_IS_BLOCKED;

  case SEDIT_WRONG_REPLACE_PATTERN:
    return NOTEPAD_EDITOR_WRONG_REPLACE_PATTERN;
  }

  return NOTEPAD_NO_ERROR;
}

// Function converts file error to notepad error
extern int CError::TranslateFile(int err)
{
  switch (err)
  {
  case FILES_MEMORY_ERROR:
    return NOTEPAD_MEMORY_ERROR;

  case FILE_SEEKING_ERROR:
  case FILES_OPEN_FILE_ERROR:
    return NOTEPAD_OPEN_FILE_ERROR;

  case FILES_SAVE_FILE_ERROR:
    return NOTEPAD_SAVE_FILE_ERROR;

  case FILES_SEDIT_ERROR:
    return NOTEPAD_EDITCTRL_ERROR;
  }

  return NOTEPAD_NO_ERROR;
}

// Function converts register error to notepad error
extern int CError::TranslateRegister(int err)
{
  switch (err)
  {
  case REG_REGISTER_ERROR:
    return NOTEPAD_REGISTER_EXTENTION_ERROR;

  case REG_UNREGISTER_ERROR:
    return NOTEPAD_UNREGISTER_EXTENTION_ERROR;
  }

  return NOTEPAD_NO_ERROR;
}

extern void CError::GetErrorMessage(__out std::wstring &errmsg, __in int err, ...)
{
  if (NOTEPAD_NO_ERROR != err)
  {
    va_list params;
    va_start(params, err);
    constructErrorMessage(errmsg, err, params);
    va_end(params);
  }
  else
    errmsg.assign(errMsg[NOTEPAD_NO_ERROR]);
}

extern void CError::GetSysErrorMessage(int err, std::wstring &errmsg)
{
  #define MIN_MESS_SIZE 32
  wchar_t *mess;

  FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                NULL, err, LANG_NEUTRAL, (LPWSTR)&mess, MIN_MESS_SIZE, NULL);
  errmsg = mess;
  LocalFree(mess);

  #undef MIN_MESS_SIZE
}