#include "register.h"
#include "variables.h"

#pragma warning(disable: 4996)

#define REG_VERSION L"100"

#define BACKUP L"Backup"
#define CROSSPAD_FILE L"CrossPadFile"
#define STD_OPEN_PATH L"CrossPadFile\\Shell\\Open\\Command"
#define STD_OPEN2_PATH L"CrossPadFile\\Shell\\OpenDoc\\Command"
#define STD_ICON_PATH L"CrossPadFile\\DefaultIcon"
#define STD_OPEN_PARAM L" \"%1\""
#define STD_ICON_PARAM L",101"  // = APPLICATION_ICON_1

#define APP_EXTENTION L".exe"

 
// Функция проверяет наличие и корректность ветки CrossPadFile
extern char TestCrossPadKey()
{
  HKEY key;
  DWORD size = PATH_SIZE * sizeof(wchar_t), error, type;
  wchar_t path[PATH_SIZE] = L"\0";

  // Проверим есть ли в реестре папка CrossPad
  error = RegOpenKeyEx(HKEY_CLASSES_ROOT, CROSSPAD_FILE, 0, KEY_READ, &key);
  if (error == ERROR_SUCCESS)
  {
    error = RegQueryValueEx(key, NULL, NULL, &type, (char*)path, &size);
    RegCloseKey(key);

    if (error != ERROR_SUCCESS || wcscmp(path, REG_VERSION) != 0)
      error = ERROR_SUCCESS + 1;
  }

  if (error != ERROR_SUCCESS)
  {
    // Create CrossPadFile
    error = RegCreateKeyEx(HKEY_CLASSES_ROOT, CROSSPAD_FILE, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &key, NULL);
    if (error != ERROR_SUCCESS)
      return REG_REGISTER_ERROR;

    error = RegSetValueEx(key, NULL, 0, REG_SZ, (BYTE*)REG_VERSION, (wcslen(REG_VERSION) + 1) * sizeof(wchar_t));
    RegCloseKey(key);

    if (error != ERROR_SUCCESS)
      return REG_REGISTER_ERROR;


    // STD_OPEN1
    error = RegCreateKeyEx(HKEY_CLASSES_ROOT, STD_OPEN_PATH, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &key, NULL);
    if (error != ERROR_SUCCESS)
      return REG_REGISTER_ERROR;

    path[0] = L'\"';
    wcscpy(&path[1], g_execPath);
    size = wcslen(path);
    path[size++] = L'\\';
    wcscpy(&path[size], PROGRAM_NAME);
    size += wcslen(PROGRAM_NAME);
    wcscpy(&path[size], APP_EXTENTION);
    size += wcslen(APP_EXTENTION);
    path[size++] = L'\"';
    wcscpy(&path[size], STD_OPEN_PARAM);
    size += wcslen(STD_OPEN_PARAM);
    error = RegSetValueEx(key, NULL, 0, REG_SZ, (BYTE*)path, (size + 1) * sizeof(wchar_t));
    RegCloseKey(key);

    if (error != ERROR_SUCCESS)
      return REG_REGISTER_ERROR;


    // STD_OPEN2
    error = RegCreateKeyEx(HKEY_CLASSES_ROOT, STD_OPEN2_PATH, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &key, NULL);
    if (error != ERROR_SUCCESS)
      return REG_REGISTER_ERROR;

    error = RegSetValueEx(key, NULL, 0, REG_SZ, (BYTE*)path, (size + 1) * sizeof(wchar_t));
    RegCloseKey(key);

    if (error != ERROR_SUCCESS)
      return REG_REGISTER_ERROR;
    

    // Теперь установим иконку
    error = RegCreateKeyEx(HKEY_CLASSES_ROOT, STD_ICON_PATH, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &key, NULL);

    if (error != ERROR_SUCCESS)
      return REG_REGISTER_ERROR;

    wcscpy(path, g_execPath);
    size = wcslen(path);
    path[size++] = L'\\';
    wcscpy(&path[size], PROGRAM_NAME);
    size += wcslen(PROGRAM_NAME);
    wcscpy(&path[size], APP_EXTENTION);
    size += wcslen(APP_EXTENTION);
    wcscpy(&path[size], STD_ICON_PARAM);
    size += wcslen(STD_ICON_PARAM);
    error = RegSetValueEx(key, NULL, 0, REG_SZ, (BYTE*)path, (size + 1) * sizeof(wchar_t));
    RegCloseKey(key);  

    if (error != ERROR_SUCCESS)
      return REG_REGISTER_ERROR;
  }

  return REG_NO_ERROR;
}

// должна быть вызвана до использования функций Associate и Deassociate
extern char TestAssociation(extension_t *ext)
{
  HKEY key;
  DWORD type, size = PATH_SIZE * sizeof(wchar_t), error, closeErr;
  wchar_t path[PATH_SIZE];

  error = RegOpenKeyEx(HKEY_CLASSES_ROOT, ext->name, 0, KEY_READ, &key);

  if (error != ERROR_SUCCESS)
  {
    ext->exist = 0;
    return 0;
  }
  ext->exist = 1;

  error = RegQueryValueEx(key, NULL, NULL, &type, (char*)path, &size);
  closeErr = RegCloseKey(key);

  if (error != ERROR_SUCCESS || closeErr != ERROR_SUCCESS)
    return REG_REGISTER_ERROR;

  if (wcsncmp(path, CROSSPAD_FILE, wcslen(CROSSPAD_FILE)) == 0)
    return 1;

  return 0;
}

extern char Associate(const extension_t *ext)
{
  
  HKEY key;
  wchar_t path[PATH_SIZE];
  DWORD size = PATH_SIZE * sizeof(wchar_t), error, closeErr;

  if (ext->exist)
  {
    error = RegOpenKeyEx(HKEY_CLASSES_ROOT, ext->name, 0, KEY_ALL_ACCESS, &key);
    if (error != ERROR_SUCCESS)
      return REG_REGISTER_ERROR;
    
    error = RegQueryValueEx(key, NULL, NULL, NULL, (char*)path, &size);
    if (error != ERROR_SUCCESS)
    {
      RegCloseKey(key);
      return REG_REGISTER_ERROR;
    }

    error = RegSetValueEx(key, BACKUP, 0, REG_SZ, (BYTE*)path, size);
    if (error != ERROR_SUCCESS)
    {
      RegCloseKey(key);
      return REG_REGISTER_ERROR;
    }
  }
  else
  {
    error = RegCreateKeyEx(HKEY_CLASSES_ROOT, ext->name, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &key, NULL);
    if (error != ERROR_SUCCESS)
      return REG_REGISTER_ERROR;
  }

  wcscpy(path, CROSSPAD_FILE);
  size = wcslen(path);
  error = RegSetValueEx(key, NULL, 0, REG_SZ, (BYTE*)path, (size + 1) * sizeof(wchar_t));
  closeErr = RegCloseKey(key);
    
  if (error != ERROR_SUCCESS || closeErr != ERROR_SUCCESS)
    return REG_REGISTER_ERROR;

  return REG_NO_ERROR;
}

extern char Deassociate(const extension_t *ext)
{
  HKEY key;
  wchar_t path[PATH_SIZE];
  DWORD size = PATH_SIZE, error;
  int i = 0;

  if (ext->exist)
  {
    error = RegOpenKeyEx(HKEY_CLASSES_ROOT, ext->name, 0, KEY_ALL_ACCESS, &key);
    if (error != ERROR_SUCCESS)
      return REG_UNREGISTER_ERROR;

    while (i < 2 && (error = RegEnumValue(key, i, path, &size, 0, NULL, NULL, 0)) == ERROR_SUCCESS)
    {
      i++;
      size = PATH_SIZE;
    }

    if (error != ERROR_SUCCESS && error != ERROR_NO_MORE_ITEMS)
    {
      RegCloseKey(key);
      return REG_UNREGISTER_ERROR;
    }

    size = PATH_SIZE * sizeof(wchar_t);
    
    if (i > 1)  // в папке больше одного параметра
    {
      error = RegQueryValueEx(key, BACKUP, NULL, NULL, (BYTE*)path, &size);
      if (error != ERROR_SUCCESS)
      {
        RegCloseKey(key);
        return REG_UNREGISTER_ERROR;
      }

      error = RegSetValueEx(key, NULL, 0, REG_SZ, (BYTE*)path, size);
      if (error != ERROR_SUCCESS)
      {
        RegCloseKey(key);
        return REG_UNREGISTER_ERROR;
      }

      RegDeleteValue(key, BACKUP);
      RegCloseKey(key);
    }
    else
    {
      if (RegCloseKey(key) != ERROR_SUCCESS)
        return REG_UNREGISTER_ERROR;

      if (RegDeleteKey(HKEY_CLASSES_ROOT, ext->name) != ERROR_SUCCESS)
        return REG_UNREGISTER_ERROR;
    }
  }

  return REG_NO_ERROR;
}