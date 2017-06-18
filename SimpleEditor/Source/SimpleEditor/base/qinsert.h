#ifndef _QUICK_INSERT
#define _QUICK_INSERT

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

#define QINSERT_NO_ERROR    0
#define QINSERT_WRONG_PATH -1
#define QINSERT_MENU_ERROR -2

extern char CreateQInsertMenu(HMENU hmenu, const wchar_t *path, int firstID, int *lastID);
extern wchar_t* GetInsertString(unsigned int id, unsigned int *back, char *pasteSelection);
extern void DestroyQInsertMenu();

#ifdef __cplusplus
}
#endif

#endif