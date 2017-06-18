#include "variables.h"

extern HINSTANCE g_hAppInstance = 0;
extern HINSTANCE g_hResInstance = 0;
extern wchar_t g_execPath[PATH_SIZE] = L"";

extern wchar_t g_languageName      [MSG_MAX_LENGTH] = L"English";
extern wchar_t g_languageId    [MSG_MAX_LENGTH] = L"en";

extern wchar_t LIBRARY_VERSION [MSG_MAX_LENGTH] = L"\0";
extern wchar_t ERROR_CAPTION   [MSG_MAX_LENGTH] = L"\0";
extern wchar_t WARNING_CAPTION [MSG_MAX_LENGTH] = L"\0";
extern wchar_t INFO_CAPTION    [MSG_MAX_LENGTH] = L"\0";

extern wchar_t DOC_FILE_NOT_SAVED [MSG_MAX_LENGTH] = L"\0";
extern wchar_t DOC_REPLACED       [MSG_MAX_LENGTH] = L"\0";
extern wchar_t DOC_TEXT_NOT_FOUND [MSG_MAX_LENGTH] = L"\0";
extern wchar_t DOC_REWRITE_FILE   [MSG_MAX_LENGTH] = L"\0";
extern wchar_t NOTEPAD_FORCE_EXIT [MSG_MAX_LENGTH] = L"\0";
extern wchar_t GOTOLINE_MESSAGE   [MSG_MAX_LENGTH] = L"\0";

extern wchar_t MENU_EMPTY       [MSG_MAX_LENGTH] = L"\0";