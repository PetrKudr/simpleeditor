#include "..\debug.h"
#include <string.h>
#include <limits.h>

#include "editor.h"
#include "selection.h"
#include "undo.h"
#include "seregexp.h"
#include "find.h"



static void invertString(wchar_t *str, size_t length)
{
  unsigned int i;
  wchar_t tmp;

  for (i = 0; i < length / 2; i++)
  {
    tmp = str[length - i - 1];
    str[length - i - 1] = str[i];
    str[i] = tmp;
  }
}

// функция вычисляет префикс-функцию для строки str
static unsigned int* prefix(wchar_t *str, size_t length)
{
  unsigned int i, j;
  unsigned int *pi = malloc(sizeof(int) * length);

  if (!pi)
    return NULL;

  pi[0] = 0;
  for (i = 1; i < length; i++)
  {
    j = pi[i - 1];
    while (j > 0 && str[i] != str[j])
      j = pi[j - 1];
    if (str[i] == str[j])
      pi[i] = j + 1;
    else
      pi[i] = 0;
  }

  return pi;
}

// функция возвращает индекс первого вхождения подстроки в строку или длину строки, если вхождений нет.
// type - параметры поиска
static unsigned int findFirstEntry(wchar_t *str, unsigned int ind, size_t len, wchar_t *subStr, size_t subLen,
                                   unsigned int *pi, char type)
{
  unsigned int i, j;
  unsigned int currentPi = 0;

  for (i = ind; i < len; i++)
  {
    j = currentPi;

    if (type & FIND_MATCH_CASE)
    {
      while (j > 0 && str[i] != subStr[j])
        j = pi[j - 1];
    }
    else
    {
      while (j > 0 && (_wcsnicmp(&str[i], &subStr[j], 1) != 0))
        j = pi[j - 1];
    }

    if (((type & FIND_MATCH_CASE) && str[i] == subStr[j]) ||
        ((!(type & FIND_MATCH_CASE)) && (_wcsnicmp(&str[i], &subStr[j], 1) == 0)))
      currentPi = j + 1;
    else
      currentPi = 0;

    if (currentPi == subLen)
    {
      _ASSERTE(i + 1 >= currentPi);

      if ((type & FIND_WHOLE_WORD) &&
          (((i + 1) - subLen != 0 && !IsBreakSym(str[i - subLen])) || (i + 1 != len && !IsBreakSym(str[i + 1]))))
        currentPi = pi[subLen - 1];
      else
        return (i + 1) - currentPi;
    }
  }

  return len;
}

static char findStringDown(line_info_t *piStartLine, unsigned int index, wchar_t *subStr, size_t subLen,
                           char type, pos_info_t *pres)
{
  pline_t pline = piStartLine->pline;
  unsigned int number = piStartLine->number, realNumber = piStartLine->realNumber;
  unsigned int *pi = prefix(subStr, subLen);
  size_t len;

  _ASSERTE(pres && pres->piLine);
  if (!pi)
    return SEDIT_MEMORY_ERROR;

  do
  {
    len = RealStringLength(pline);
    index = findFirstEntry(pline->prealLine->str, index, len, subStr, subLen, pi, type);
    if (index == len)  // подстрока не найдена
    {
      pline = pline->next;
      ++number;
      while (pline && pline->prev->prealLine == pline->prealLine)
      {
        pline = pline->next;
        ++number;
      }
      ++realNumber;
      len = index = 0;
    }
  } while (pline && index == len);
  free(pi);

  if (pline && index != len)  // подстрока найдена
  {
    pres->piLine->pline = pline;
    pres->piLine->number = number;
    pres->piLine->realNumber = realNumber;
    pres->index = index;
    pres->pos = POS_BEGIN;
    UpdateLine(pres);  // найдем нужную строку
    return 1;
  }
  return 0;
}

// функция возвращает индекс первого вхождения подстроки в строку или индекс с которого начался поиск, если вхождений нет.
// поиск идёт в обратном направлении (от str[ind] до  str[0]), type - параметры поиска
static unsigned int findBackFirstEntry(wchar_t *str, unsigned int ind, wchar_t *subStr, size_t subLen,
                                       unsigned int *pi, char type)
{
  unsigned int i = ind, j;
  unsigned int currentPi = 0;
  char isEnd = 0;

  if (i != 0)
    i--;
  else
    isEnd = 1;

  while (!isEnd)
  {
    j = currentPi;

    if (type & FIND_MATCH_CASE)
    {
      while (j > 0 && str[i] != subStr[j])
        j = pi[j - 1];
    }
    else
    {
      while (j > 0 && (_wcsnicmp(&str[i], &subStr[j], 1) != 0))
        j = pi[j - 1];
    }

    if (((type & FIND_MATCH_CASE) && str[i] == subStr[j]) ||
        ((!(type & FIND_MATCH_CASE)) && (_wcsnicmp(&str[i], &subStr[j], 1) == 0)))
      currentPi = j + 1;
    else
      currentPi = 0;

    if (currentPi == subLen)
    {
      if ((type & FIND_WHOLE_WORD) &&
          ((str[i + subLen]  && !IsBreakSym(str[i + subLen])) || (i > 0 && !IsBreakSym(str[i - 1]))))
        currentPi = pi[subLen - 1];
      else
        return i;
    }

    if (i == 0)
      isEnd = 1;
    else
      i--;
  }

  return ind;
}

static char findStringUp(line_info_t *piStartLine, unsigned int index, wchar_t *subStr, size_t subLen,
                         char type, pos_info_t *pres)
{
  pline_t pline = piStartLine->pline;
  unsigned int *pi, number = piStartLine->number, realNumber = piStartLine->realNumber;
  size_t searchIndex = index;
  char searchEnd = 0;

  invertString(subStr, subLen);
  pi = prefix(subStr, subLen);

  if (!pi)
    return SEDIT_MEMORY_ERROR;

  while (!searchEnd)
  {
    index = findBackFirstEntry(pline->prealLine->str, searchIndex, subStr, subLen, pi, type);
    if (index == searchIndex)  // подстрока не найдена
    {
      while (pline->prev->prealLine == pline->prealLine)
      {
        pline = pline->prev;
        number--;
      }

      if (number > 0)
      {
        pline = pline->prev;
        number--;
        realNumber--;
        index = RealStringLength(pline);
        searchIndex = index;
      }
      else
        searchEnd = 1;
    }
    else
      searchEnd = 1;
  }
  free(pi);
  invertString(subStr, subLen);

  if (index != searchIndex)  // подстрока найдена
  {
    pres->piLine->pline = pline;
    pres->piLine->number = number;
    pres->piLine->realNumber = realNumber;
    pres->index = index;
    pres->pos = POS_BEGIN;
    UpdateLine(pres);  // найдем нужную строку
    return 1;
  }
  return 0;
}

/* Функция ищет подстроку, хранящуюся в одном из полей lpfr, в тексте
 * ПАРАМЕТРЫ:
 *   pfr - указатель на структуру, определяющую параметры поиска(заполнять не надо)
 *   pcommon - информация об окне
 *   pcaret - информация о каретке
 *   psp - указатель на структуру, хранящую последнюю позицию поиска
 *   psel - информация о выделении
 *   pfirstLine - первая строка на экране
 *
 * ВОЗВРАЩАЕТ:
 *   1 - если поиск завершён успешно(текст найден)
 *   0 - если текст не найден
 *   <0 - если произошла ошибка
 */
extern char Find(LPFINDREPLACE pfr, common_info_t *pcommon, caret_info_t *pcaret, pos_info_t *psp,
                 pselection_t psel, line_info_t *pfirstLine)
{
  line_info_t startLine, endLine;
  pos_info_t startPos, endPos;
  size_t length;
  line_info_t oldLine = pcaret->line;
  char ret, type = 0;
  HDC hdc;

  if (L'\0' == pfr->lpstrFindWhat[0])
    return 0;

  if (pfr->Flags & FR_MATCHCASE)
    type |= FIND_MATCH_CASE;
  if (pfr->Flags & FR_WHOLEWORD)
    type |= FIND_WHOLE_WORD;

  // каретка была перемещена со времени последнего поиска, искать нужно от её позиции
  if (pcaret->type & CARET_IS_MODIFIED)
  {
    *psp->piLine = pcaret->line;
    psp->index = pcaret->index;
  }
  pcaret->type &= (~CARET_IS_MODIFIED);  // сбросим флаг модификации каретки

  startLine = *psp->piLine;
  startPos.piLine = &startLine;

  length = wcslen(pfr->lpstrFindWhat);
  if (pcommon->useRegexp)
  {
    if (pfr->Flags & FR_DOWN)
      ret = RegexpFindMatchDown(psp->piLine, psp->index, pfr->lpstrFindWhat, &startPos);
    else
      ret = RegexpFindMatchUp(psp->piLine, psp->index, pfr->lpstrFindWhat, &startPos);
    if (ret <= 0)
      return ret;
    length = ret;
  }
  else
  {
    if (pfr->Flags & FR_DOWN)
      ret = findStringDown(psp->piLine, psp->index, pfr->lpstrFindWhat, length, type, &startPos);
    else
      ret = findStringUp(psp->piLine, psp->index, pfr->lpstrFindWhat, length, type, &startPos);
    if (ret <= 0)
      return ret;
  }

  endLine = startLine;
  endPos.piLine = &endLine;
  endPos.index = startPos.index + length;
  endPos.pos = POS_BEGIN;
  UpdateLine(&endPos);  // найдём позицию конца найденной строки

  if (pfr->Flags & FR_DOWN)
  {
    *psp->piLine = startLine;
    psp->index = startPos.index;
    psp->index += 1;  // следующий поиск производить со следующего символа
  }
  else
  {
    *psp->piLine = endLine;
    psp->index = endPos.index;
    psp->index -= 1;
  }

  hdc = GetDC(pcommon->hwnd);
  SelectObject(hdc, pcommon->hfont);
  pcaret->line = endLine;
  pcaret->index = endPos.index;
  UpdateCaretSizes(hdc, pcommon, pcaret);
  ReleaseDC(pcommon->hwnd, hdc);

  //GotoCaret(pcommon, pcaret, pfirstLine);

  if (pcaret->line.realNumber != oldLine.realNumber)
  {
    // перерисуем старую и новую линии с кареткой
    RedrawCaretLine(pcommon, pcaret, pfirstLine, &oldLine);
    RedrawCaretLine(pcommon, pcaret, pfirstLine, &pcaret->line);
  }

  if (psel->isSelection) // уберем выделение, если оно есть
  {
    psel->isSelection = 0;
    UpdateSelection(pcommon, pcaret, pfirstLine, psel, NULL);
  }
  StartSelection(psel, &startLine, startPos.index);
  ChangeSelection(psel, &endLine, endPos.index);
  UpdateSelection(pcommon, pcaret, pfirstLine, psel, NULL);

  GotoSelection(pcommon, pcaret, psel, pfirstLine);

  return 1;
}

extern char Replace(LPFINDREPLACE pfr, common_info_t *pcommon, caret_info_t *pcaret, undo_info_t *pundo,
                    pos_info_t *psp, pselection_t psel, line_info_t *pfirstLine)
{
  if (pcommon->useRegexp)
  {
    sedit_regexp_t regexp;
    int retval;

    if ((retval = RegexpCompile(&regexp, pfr->lpstrFindWhat)) < 0)
      return retval;

    if ((retval = CompareSelectionRegexp(&regexp, psel, pfr->lpstrFindWhat)) == 1)
    {
      wchar_t *replaceStr;

      retval = RegexpGetReplaceString(pfr->lpstrReplaceWith, psel->pos.startLine.pline->prealLine->str,
                                      &replaceStr, regexp.subExpr, regexp.nSubExpr);
      if (retval >= 0)
      {
        DeleteSelection(pcommon, pcaret, psel, pfirstLine, pundo);
        PasteFromBuffer(pcommon, pcaret, psel, pfirstLine, pundo, replaceStr);
        free(replaceStr);
      }
    }
    RegexpFree(&regexp);

    if (retval < 0)
      return retval;
  }
  else if (CompareSelection(psel, pfr->lpstrFindWhat, pfr->Flags))
  {
    DeleteSelection(pcommon, pcaret, psel, pfirstLine, pundo);
    PasteFromBuffer(pcommon, pcaret, psel, pfirstLine, pundo, pfr->lpstrReplaceWith);
  }

  return Find(pfr, pcommon, pcaret, psp, psel, pfirstLine);
}


// Эту поцедуру необходимо переписать с использованием Paste и DeleteArea
// (как только можно будет удалять реальные строки)
extern int ReplaceAll(LPFINDREPLACE pfr, common_info_t *pcommon, caret_info_t *pcaret, pselection_t psel,
                      line_info_t *pfirstLine, undo_info_t *pundo, pline_t head)
{
  HDC hdc;
  pos_info_t caretPos, flinePos;
  int count = 0, reduced, added, retval;
  unsigned int subStrLen, newStrLen, index = 0, len, rnum = 0;
  long int maxLength = pcommon->isWordWrap ? pcommon->clientX - pcommon->di.leftIndent : MAX_LENGTH;
  char modified = 0; // в строке была сделана замена
  char type = 0;     // опции поиска
  pline_t pline = head->next, pmaxLine = NULL, ptmpLine = NULL;
  position_t replacePos; // для сохранения позиций в стеке undo
  void *searchData;
  wchar_t *replaceString = NULL;
  range_t range;

  range.start.x = 0;
  range.start.y = 0;
  range.end.x = UINT_MAX;
  range.end.y = UINT_MAX;

  if (pcommon->useRegexp)
  {
    searchData = malloc(sizeof(sedit_regexp_t));
    if (searchData && RegexpCompile((sedit_regexp_t*)searchData, pfr->lpstrFindWhat) < 0)
    {
      free(searchData);
      searchData = NULL;
    }
  }
  else
  {
    subStrLen = wcslen(pfr->lpstrFindWhat);
    searchData = (void*)prefix(pfr->lpstrFindWhat, subStrLen);
    newStrLen = wcslen(pfr->lpstrReplaceWith);
    replaceString = pfr->lpstrReplaceWith;
  }
  if (!searchData)
    return SEDIT_MEMORY_ERROR;

  if (psel->isSelection)  // уберём выделение
  {
    psel->isSelection = 0;
    UpdateSelection(pcommon, pcaret, pfirstLine, psel, NULL);
  }

  if (pfr->Flags & FR_MATCHCASE)
    type = type | FIND_MATCH_CASE;
  if (pfr->Flags & FR_WHOLEWORD)
    type = type | FIND_WHOLE_WORD;

  caretPos.piLine = &pcaret->line;
  caretPos.index = pcaret->index;
  SetStartLine(&caretPos);

  flinePos.piLine = pfirstLine;
  flinePos.index = pfirstLine->pline->begin;
  SetStartLine(&flinePos);

  if (!PutFindReplaceText(pundo, pfr->lpstrFindWhat, pfr->lpstrReplaceWith, pcommon->useRegexp))
    UNDO_RESTRICT_SAVE(pundo);

  len = RealStringLength(pline);
  hdc = GetDC(pcommon->hwnd);
  SelectObject(hdc, pcommon->hfont);
  while (pline)
  {
    if (pcommon->useRegexp)
    {
      retval = RegexpGetMatch((sedit_regexp_t*)searchData, pline->prealLine->str, index);
      if (retval > 0)
      {
        index = (unsigned int)((sedit_regexp_t*)searchData)->subExpr[0].x;
        subStrLen = ((sedit_regexp_t*)searchData)->subExpr[0].y - ((sedit_regexp_t*)searchData)->subExpr[0].x;
      }
      else
        index = len; // сигнал о том, что подстрока не найдена
    }
    else
      index = findFirstEntry(pline->prealLine->str, index, len, pfr->lpstrFindWhat,
                             subStrLen, (unsigned int*)searchData, type);

    if (index == len)  // подстрока не найдена
    {
      if (modified)
      {
        ptmpLine = pline;
        pline = ReduceLine(hdc, &pcommon->di, pline, maxLength, &reduced, &added);
        pcommon->nLines += added;

        _ASSERTE(pline == NULL || pline->prealLine != ptmpLine->prealLine);

        if (!pcommon->isWordWrap)
        {
          unsigned long sizeX;

          while (ptmpLine != pline)
          {
            sizeX = GetStringSizeX(hdc, &pcommon->di, ptmpLine->prealLine->str, ptmpLine->begin, ptmpLine->end, NULL);
            if (sizeX > pcommon->maxStringSizeX)
              pcommon->maxStringSizeX = sizeX;
            ptmpLine = ptmpLine->next;
          }
        }
      }
      else
      {
        pline = pline->next;
        while (pline && pline->prev->prealLine == pline->prealLine)
          pline = pline->next;
      }

      ++rnum; // следующая реальная строка

      if (pline)
      {
        len = RealStringLength(pline);
        index = pline->begin;
        modified = 0;
      }
    }
    else  // подстрока найдена
    {
      if (!modified)  // расширим строку, если это еще не сделано
        pcommon->nLines -= UniteLine(pline);
      modified = 1;

      replacePos.x = index;
      replacePos.y = rnum;
      if (!PutReplacePos(pundo, &pline->prealLine->str[index], subStrLen, &replacePos))
        UNDO_RESTRICT_SAVE(pundo);

      // получим строку для замены, если используются регекспы
      if (pcommon->useRegexp)
      {
        newStrLen = RegexpGetReplaceString(pfr->lpstrReplaceWith, pline->prealLine->str, &replaceString,
                          ((sedit_regexp_t*)searchData)->subExpr, ((sedit_regexp_t*)searchData)->nSubExpr);
        if (newStrLen < 0) // произошла ошибка
        {
          if (modified)
          {
            pline = ReduceLine(hdc, &pcommon->di, pline, maxLength, &reduced, &added);
            pcommon->nLines += added;
            // по идее ещё надо проапдейтить pcommon->maxStringSizeX, но это не критично,
            // а вероятность ошибки в этом месте стремится к нулю; тем более, что после переделывания
            // под Paste и DeleteArea, весь "низкоуровневый" код пропадёт
          }
          count = newStrLen; // возвращаемое значение - ошибка
          break;
        }
      }
      DeleteSubString(pline, len, index, subStrLen);
      InsertSubString(pline, len - subStrLen, index, replaceString, newStrLen);
      len += newStrLen - subStrLen;
      count++;

      // теперь освободим строку для замены
      if (pcommon->useRegexp)
        free(replaceString);

      index += newStrLen;
    }
  }

  if (pcommon->useRegexp)
    RegexpFree((sedit_regexp_t*)searchData);
  free(searchData);

  UpdateLine(&flinePos);
  GetNumbersFromLines(head->next, 0, 1, &pfirstLine->pline, &pfirstLine->number);

  if (!UpdateLine(&caretPos))
    pcaret->index = pcaret->line.pline->end;
  GetNumbersFromLines(head->next, 0, 1, &pcaret->line.pline, &pcaret->line.number);
  UpdateCaretSizes(hdc, pcommon, pcaret);
  ReleaseDC(pcommon->hwnd, hdc);

  NextCell(pundo, ACTION_REPLACE_ALL, pcommon->isModified);
  UNDO_ALLOW_SAVE(pundo);  // если сохранение было запрещено, его надо восстановить

  UpdateCaretPos(pcommon, pcaret, pfirstLine);

  if (IS_SHIFT_PRESSED)
    StartSelection(psel, &pcaret->line, pcaret->index);

  SEInvalidateRect(pcommon, NULL, FALSE);

  if (count != 0)
    DocModified(pcommon, SEDIT_ACTION_REPLACEALL, &range);

  SEUpdateWindow(pcommon, pcaret, pfirstLine);
  UpdateScrollBars(pcommon, pcaret, pfirstLine);

  CaretModified(pcommon, pcaret, pfirstLine, 0);

  if (count != 0)
    DocUpdated(pcommon, SEDIT_ACTION_REPLACEALL, &range, 1);

  return count;
}
