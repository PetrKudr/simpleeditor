#include "..\..\debug.h"
#include <stdio.h>
#include <wchar.h>
#pragma warning(disable: 4996)

#include "qinsert.h"

#define BUFFER_SIZE 512

#define SEPARATOR_LEN 11  // количество символов в строке SEPARATOR
#define SEPARATOR L"%SEPARATOR%"

#define QI_DAY          1
#define QI_MONTH        2
#define QI_YEAR         3
#define QI_SECOND       4
#define QI_MINUTE       5
#define QI_HOUR         6

typedef struct tag_pos_info_t 
{
  unsigned int i;
  char type;
} pos_info_t;

typedef struct tag_menu_item_t
{
  unsigned int id;
  wchar_t *str;
  size_t back, posNumber;
  pos_info_t *positions;
  char isCaretPos;       // Есть ли в строке "%!"

  struct tag_menu_item_t *next;
} menu_item_t;

static menu_item_t *head = NULL, *curMI = NULL;
static unsigned int curID = 0;
static char isError = 0;


static wchar_t* getPreparedString(wchar_t *str, size_t *back, pos_info_t **_positions, size_t *posNumber, char *isCaretPos)
{
  unsigned int i = 0, newLines = 0;
  wchar_t *prepared = NULL;
  pos_info_t *positions = NULL;
  size_t preparedIndex = 0, posIndex = 0;

  // сначала получим необходимые размеры (конечной строки + количество позиций)
  (*_positions) = NULL;
  (*isCaretPos) = (*back) = (*posNumber) = 0;
  while (str[i])
  {
    if (str[i] == L'%')
    {
      switch (str[++i])
      {
      case L'D':
      case L'M':
      case L'h':
      case L'm':
      case L's':
        posIndex++;
        preparedIndex += 2;
        break;

      case L'Y':
        posIndex++;
        preparedIndex += 4;
        break;

      case L'L':
        preparedIndex += 2;
        if (*isCaretPos)
          newLines++;
        break;

      case L'%':
        preparedIndex += 1;
        break;

      case L'!':
        (*back) = preparedIndex;
        (*isCaretPos) = 1;
        newLines = 0;
        break;
      }
    }
    else
      preparedIndex++;
    i++;
  }
  if (*isCaretPos)
    (*back) = preparedIndex - newLines - (*back);
  (*posNumber) = posIndex;

  if (posIndex)
    if ((positions = malloc(sizeof(pos_info_t) * posIndex)) == NULL)
      return NULL;
  (*_positions) = positions;

  if ((prepared = malloc(sizeof(wchar_t) * (preparedIndex + 1))) == NULL)
  {
    if (positions)
      free(positions);
    return NULL;
  }

  i = posIndex = preparedIndex = 0;

  // теперь заполним массив позиций и конечную строку
  while (str[i])
  {
    if (str[i] == L'%')
    {
      switch (str[++i])
      {
      case L'D':
        positions[posIndex].i = preparedIndex;
        positions[posIndex].type = QI_DAY;
        preparedIndex += 2;
        posIndex++;
        break;

      case L'M':
        positions[posIndex].i = preparedIndex;
        positions[posIndex].type = QI_MONTH;
        preparedIndex += 2;
        posIndex++;
        break;

      case L'Y':
        positions[posIndex].i = preparedIndex;
        positions[posIndex].type = QI_YEAR;
        preparedIndex += 4;
        posIndex++;
        break;

      case L'h':
        positions[posIndex].i = preparedIndex;
        positions[posIndex].type = QI_HOUR;
        preparedIndex += 2;
        posIndex++;
        break;

      case L'm':
        positions[posIndex].i = preparedIndex;
        positions[posIndex].type = QI_MINUTE;
        preparedIndex += 2;
        posIndex++;
        break;

      case L's':
        positions[posIndex].i = preparedIndex;
        positions[posIndex].type = QI_SECOND;
        preparedIndex += 2;
        posIndex++;
        break;

      case L'L':
        prepared[preparedIndex++] = L'\r';
        prepared[preparedIndex++] = L'\n';
        break;

      case L'%':
        prepared[preparedIndex++] = str[i];
        break;
      }
    }
    else
      prepared[preparedIndex++] = str[i];
    i++;
  }
  prepared[preparedIndex] = L'\0';

  return prepared;
}

static unsigned char getTabs(wchar_t *wstr, unsigned int *i)
{
  unsigned char num = 0;

  while (wstr[*i] && wstr[*i] == L' ' || wstr[*i] == L'\t')
  {
    if (wstr[*i] == L'\t')
      num++;
    (*i)++;
  }

  return num;
}

static wchar_t* constructMenu(FILE *fIn, wchar_t *buffer, HMENU hmenu, unsigned char level, char *isEnd)
{
  char needNewStr = 1;
  wchar_t *str, *tempStr;
  unsigned int i, strIndex;

  while (!(*isEnd) && !isError)
  {
    if (needNewStr)
    {
      tempStr = str = NULL;
      strIndex = 0;
      i = 1;

      do
      {
        if (!fgetws(buffer, BUFFER_SIZE, fIn))
        {
          (*isEnd) = 1;
          break;
        }

        str = (wchar_t*)realloc(tempStr, (size_t)(BUFFER_SIZE * (i++) * sizeof(wchar_t)));
        if(!str) 
        {
          if (tempStr)
            free(tempStr);
          (*isEnd) = isError = 1;
          break;
        }

        wcscpy(&str[strIndex], buffer);
        strIndex += (BUFFER_SIZE - 1);
        tempStr = str;
      } while (BUFFER_SIZE - 1 == wcslen(buffer));
    }

    needNewStr = 1;
    i = 0;

    if (str)
    {
      unsigned char tabs = getTabs(str, &i);
      unsigned int k = i;
      size_t len = wcslen(str);

      if (len >= 2 && str[len - 1] == L'\n')
        str[len - 2] = str[len - 1] =L'\0';

   //   _ASSERTE(tabs <= level);
      if (tabs < level)
        return str;

      // разбор случаев
      if (str[i] == L'\0') // пустая строка
      {
        free(str);
        return NULL;
      }
      else if (str[i] == L'%' && wcsncmp(&str[i], SEPARATOR, SEPARATOR_LEN) == 0) // разделитель
      {
        AppendMenu(hmenu, MF_SEPARATOR, 0, NULL);
        free(str);
      }
      else  // пункт меню или подменю
      {
        while (str[i] && str[i] != L'|')
          i++;
      
        if (str[i] == L'|')  // пункт меню
        {
          menu_item_t *pmi;

          if ((pmi = malloc(sizeof(menu_item_t))) == NULL)
          {
            isError = (*isEnd) = 1;
            free(str);
            return NULL;
          }

          str[i++] = L'\0';
          AppendMenu(hmenu, MF_STRING, curID, &str[k]);

          pmi->id = curID++;
          if ((pmi->str = getPreparedString(&str[i], &pmi->back, &pmi->positions, &pmi->posNumber, &pmi->isCaretPos)) == NULL)
          {
            free(pmi);
            free(str);
            isError = (*isEnd) = 1;
            return NULL;
          }
          pmi->next = NULL;

          if (!head)
            curMI = head = pmi;
          else
          {
            curMI->next = pmi;
            curMI = pmi;
          }
          free(str);
        }
        else  // подменю
        {
          HMENU hpopup = CreatePopupMenu();
          AppendMenu(hmenu, MF_POPUP | MF_STRING, (UINT_PTR)hpopup, &str[k]);
          free(str);

          //str = constructMenu(fIn, buffer, hpopup, level + 1, isEnd);
          str = constructMenu(fIn, buffer, hpopup, tabs + 1, isEnd);
          needNewStr = (str == NULL) ? 1 : 0;
        }
      }
    }
  }

  return str;
}

extern wchar_t* GetInsertString(unsigned int id, unsigned int *back, char *pasteSelection)
{
#ifndef _WIN32_WCE
  #define SWPRINTF(a, b, c, d) swprintf(a, b, c, d)
#else
  #define SWPRINTF(a, b, c, d) swprintf(a, c, d)
#endif

  unsigned int i, j;
  menu_item_t *mi = head;
  SYSTEMTIME st;
  wchar_t time[8];

  (*pasteSelection) = (*back) = 0;

  while (mi && mi->id != id)
    mi = mi->next;

  if (!mi)
    return NULL;

  (*pasteSelection) = mi->isCaretPos;
  (*back) = mi->back;
  if (mi->posNumber) 
    GetLocalTime(&st);

  for (i = 0; i < mi->posNumber; i++)
  {
    j = mi->positions[i].i;

    switch (mi->positions[i].type)
    {
    case QI_DAY:
      SWPRINTF(time, 8, L"%.2i", st.wDay);
      wcsncpy(&mi->str[j], time, 2);
      break;

    case QI_MONTH:
      SWPRINTF(time, 8, L"%.2i", st.wMonth);
      wcsncpy(&mi->str[j], time, 2);
      break;

    case QI_YEAR:
      SWPRINTF(time, 8, L"%.4i", st.wYear);
      wcsncpy(&mi->str[j], time, 4);
      break;

    case QI_SECOND:
      SWPRINTF(time, 8, L"%.2i", st.wSecond);
      wcsncpy(&mi->str[j], time, 2);
      break;

    case QI_MINUTE:
      SWPRINTF(time, 8, L"%.2i", st.wMinute);
      wcsncpy(&mi->str[j], time, 2);
      break;

    case QI_HOUR:
      SWPRINTF(time, 8, L"%.2i", st.wHour);
      wcsncpy(&mi->str[j], time, 2);
      break;
    }
  }

  return mi->str;

#undef SWPRINTF
}

extern char CreateQInsertMenu(HMENU hmenu, const wchar_t *path, int firstID, int *lastID)
{
  wchar_t buffer[BUFFER_SIZE];
  unsigned int id = firstID;
  char isEnd = 0;
  FILE *fIn;

  if (!path)
    return QINSERT_WRONG_PATH;

  if ((fIn = _wfopen(path, L"rb")) == NULL)
    return QINSERT_WRONG_PATH;

  fseek(fIn, 2, SEEK_SET);
  //RemoveMenu(hmenu, 0, MF_BYPOSITION);

  curID = firstID;
  constructMenu(fIn, buffer, hmenu, 0, &isEnd);
  fclose(fIn);

  if (isError) // во время конструирования была ошибка
  {
    while (DeleteMenu(hmenu, 0, MF_BYPOSITION) != 0);
    DestroyQInsertMenu();
    return QINSERT_MENU_ERROR;
  }
  (*lastID) = (int)(curID - 1);

  return QINSERT_NO_ERROR;
}

extern void DestroyQInsertMenu()
{
  while (head)
  {
    curMI = head;

    free(head->str);
    if (head->positions)
      free(head->positions);
    head = head->next;
    free(curMI);
  }
  //if (hmenu)
    //DestroyMenu(hmenu);
  curMI = NULL;
}

