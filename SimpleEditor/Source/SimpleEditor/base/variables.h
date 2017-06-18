#ifndef __VARIABLES_
#define __VARIABLES_

//#include "protect.h"
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef PATH_SIZE
  #define PATH_SIZE 512
#endif

#define PROGRAM_NAME L"CrossPad"
#define DEFAULT_LANGUAGE L"English"
#define DEFAULT_LANGUAGE_PATH L"default"
#define FILE_SEPARATOR L"\\"

extern HINSTANCE g_hAppInstance;
extern HINSTANCE g_hResInstance;
extern wchar_t g_execPath[PATH_SIZE];


#define MSG_MAX_LENGTH 128

extern wchar_t g_languageId [MSG_MAX_LENGTH];
extern wchar_t g_languageName [MSG_MAX_LENGTH];

extern wchar_t LIBRARY_VERSION [MSG_MAX_LENGTH];
extern wchar_t ERROR_CAPTION   [MSG_MAX_LENGTH];
extern wchar_t WARNING_CAPTION [MSG_MAX_LENGTH];
extern wchar_t INFO_CAPTION    [MSG_MAX_LENGTH];

extern wchar_t DOC_FILE_NOT_SAVED [MSG_MAX_LENGTH];
extern wchar_t DOC_REPLACED       [MSG_MAX_LENGTH];
extern wchar_t DOC_TEXT_NOT_FOUND [MSG_MAX_LENGTH];
extern wchar_t DOC_REWRITE_FILE   [MSG_MAX_LENGTH];
extern wchar_t NOTEPAD_FORCE_EXIT [MSG_MAX_LENGTH];
extern wchar_t GOTOLINE_MESSAGE   [MSG_MAX_LENGTH];

/* Menu items */
extern wchar_t MENU_EMPTY       [MSG_MAX_LENGTH];

#ifdef __cplusplus
}
#endif

#endif /* __VARIABLES_ */