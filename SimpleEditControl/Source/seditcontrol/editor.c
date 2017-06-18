#include "..\debug.h"
#if defined(_WIN32_WCE)
#include <aygshell.h>
#include <commdlg.h>
#pragma comment(lib, "aygshell.lib")
#endif
#include <wchar.h>
#include <limits.h>

#include "selection.h"
#include "undo.h"
#include "mouse.h"
#include "kinetic.h"
#include "editor.h"

/* Функция копирует подстроку, начиная с конца
 * ПАРАМЕТРЫ:
 *   destination - строка назначения
 *   source - исходная строка
 *   n - длина подстроки, которая будет скопирована
 */
static void bwcsncpy(wchar_t *destination, const wchar_t *source, size_t n)
{
  if (n)
  {
    while (n--)
      destination[n] = source[n];
  }
}

/* Функция вставляет новую строку в список исходных строк
 * ПАРАМЕТРЫ:
 *   prline - указатель на исходную строку, после которой нужно вставить новую
 *   str - строка или NULL, тогда будет создана пустая строка
 */
static char insertRealLine(preal_line_t prline, wchar_t *str)
{
  preal_line_t ptmp = calloc(1, sizeof(real_line_t));

  if (ptmp)
  {
    ptmp->next = prline->next;
    prline->next = ptmp;
    ptmp->str = str ? str : calloc(1, sizeof(wchar_t));
    return 1;
  }
  return 0;
}

/* Функция вставляет новую строку в список отображаемых строк
 * ПАРАМЕТРЫ:
 *   pline - указатель на отображаемую строку, после которой нужно вставить новую
 *   prealLine - указатель на реальную строку, к которой будет относиться новая отображаемая
 */
static char insertLine(pline_t pline, preal_line_t prealLine)
{
  pline_t ptmp = calloc(1, sizeof(line_t));

  if (ptmp)
  {
    ptmp->prev = pline;
    ptmp->next = pline->next;
    ptmp->prealLine = prealLine;
    pline->next = ptmp;
    if (ptmp->next)
      ptmp->next->prev = ptmp;
    return 1;
  }
  return 0;
}

/* Функция удаляет строку из списка отображаемых строк
 * ПАРАМЕТРЫ:
 *   pline - указатель на отображаемую строку, которую надо удалить
 */
static void deleteLine(pline_t pline)
{
  if (pline->prev)
    pline->prev->next = pline->next;

  if (pline->next)
    pline->next->prev = pline->prev;

  free(pline);
}


/* Функция очищает открытый документ
 * ПАРАМЕТРЫ:
 *   prealHead - указатель на список исходных строк
 *   pfakeHead - указатель на список отображаемых строк
 */
static void clearDocument(preal_line_t prealHead, pline_t pfakeHead)
{
  pline_t pline = pfakeHead->next, pftmp;

  while (pline)
  {
    pftmp = pline;
    pline = pline->next;

    if (!pline || (pline->prealLine != pftmp->prealLine))
    {
      free(pftmp->prealLine->str);
      free(pftmp->prealLine);
    }

    free(pftmp);
  }

  prealHead->next = NULL;
  pfakeHead->next = NULL;
}

/* Функция проверяет состоит ли отображаемая строка из одного слова (без пробельных символов)
 * ПАРАМЕТРЫ:
 *   pline - указатель на отображаемую строку
 * ВОЗВРАЩАЕТ:
 *   1 - если строка состоит из одного слова
 *   0 - если в строке есть пробельные символы
 */
static char isSolidString(pline_t pline)
{
  unsigned int i = pline->begin;

  while (i < pline->end && !IsBreakSym(pline->prealLine->str[i]))
    i++;

  if (i != pline->end)
    return 0;  // в строке есть хотя бы 1 пробел
  return 1;    // строка сплошная
}

// функция возвращает позицию начала или конца слова в строке
// pos = POS_WORDBEGIN или POS_WORDEND
static unsigned int getWordPos(wchar_t *str, unsigned int index, char pos)
{
  unsigned int i = index;

  switch (pos)
  {
  case POS_WORDBEGIN:
    if (i > 0)
      --i;
    if (IsBreakSym(str[i]))
    {
      while (i > 0 && IsBreakSym(str[i]))
        --i;
      if (!IsBreakSym(str[i]) && i + 1 < index)
        ++i;
    }
    else
    {
      while (i > 0 && !IsBreakSym(str[i]))
        --i;
      if (IsBreakSym(str[i]) && i + 1 < index)
        ++i;
    }
    break;

  case POS_WORDEND:
    if (IsBreakSym(str[i]))
    {
      while (str[i] && IsBreakSym(str[i]))
        ++i;
    }
    else
    {
      while (str[i] && !IsBreakSym(str[i]))
        ++i;
    }
    //if (i == index && str[i])
    //  ++i;
    break;
  }
  return i;
}


extern void RefreshFontParams(HDC hdc, display_info_t *pdi)
{
  SIZE sz;
  TEXTMETRIC tm;

  GetTextExtentPoint32(hdc, L" ", 1, &sz);
  pdi->specSymWidths[SPACE] = (unsigned short)sz.cx;
  pdi->specSymWidths[TABULATION] = (unsigned short)sz.cx * pdi->tabSize;

  GetTextMetrics(hdc, &tm);
  pdi->averageCharX = tm.tmAveCharWidth;
  pdi->averageCharY = tm.tmHeight;
  pdi->isItalic = tm.tmItalic ? 1 : 0;

  // в качестве максимального выступа берем среднюю ширину символа
  pdi->maxOverhang = tm.tmItalic ? tm.tmAveCharWidth : 0;
}

static unsigned long getLinePlacement(HDC hdc, display_info_t *pdi, wchar_t *str, int len, int *dx)
{
  SIZE sz;
  int i, curCharWidth, nextCharWidth;

  dx[0] = 0;
  if (!len)
    return 0;

  GetTextExtentExPoint(hdc, str, len, 0, NULL, &dx[1], &sz);

  dx[0] = 0;
  nextCharWidth = dx[1] - dx[0];
  for (i = 1; i < len + 1; i++)
  {
    curCharWidth = nextCharWidth;
    if (i < len)
      nextCharWidth = dx[i + 1] - dx[i];

    if (str[i - 1] == L'\t')
      curCharWidth = pdi->specSymWidths[TABULATION] - (dx[i - 1] % pdi->specSymWidths[TABULATION]);

    dx[i] = dx[i - 1] + curCharWidth;
  }

  return (unsigned long)dx[len];
}

static int reallocDX(int **dx, size_t *dxSize, size_t len)
{
  if (len + 1 > *dxSize)
  {
    void *p = (void*)*dx;
    *dxSize = len + 1;
    if ((*dx = realloc(p, sizeof(int) * (*dxSize))) == NULL)
    {
      *dxSize = 0;
      if (p)
        free(p);
      return SEDIT_MEMORY_ERROR;
    }
  }
  return SEDIT_NO_ERROR;
}

static int reallocStrPositions(line_point_t **strPositions, size_t *size, size_t newSize)
{
  if (newSize > *size)
  {
    void *p = (void*)(*strPositions);
    *size = newSize;
    if ((*strPositions = realloc(p, (*size) * sizeof(line_point_t))) == NULL)
    {
      if (p)
        free(p);
      //*strPositions = (line_point_t*)p;
      return SEDIT_MEMORY_ERROR;
    }
  }
  return SEDIT_NO_ERROR;
}

static unsigned long getCaretLineParameters(HDC hdc, common_info_t *pcommon, caret_info_t *pcaret)
{
  if (pcaret->dxSize < pcaret->line.pline->end - pcaret->line.pline->begin + 1)
  {
    pcaret->dx = realloc(pcaret->dx, (pcaret->line.pline->end - pcaret->line.pline->begin + 1) * sizeof(int));
    pcaret->dxSize = pcaret->line.pline->end - pcaret->line.pline->begin + 1;
  }

  pcaret->length = RealStringLength(pcaret->line.pline);
  return getLinePlacement(hdc, &pcommon->di,
                           &pcaret->line.pline->prealLine->str[pcaret->line.pline->begin],
                           pcaret->line.pline->end - pcaret->line.pline->begin, pcaret->dx);
}

static void hideCaret(common_info_t *pcommon)
{
  if (!pcommon->di.isDrawingRestricted)
    SetCaretPos(-20, 0);
}

// функция подгоняет значение vscrollPos под строку pfirstLine, причём так, чтобы pfirstLine Была на экране;
// (pfirstLine может быть изменена на одну позицию)
// ВОЗВРАЩАЕТ: 0 если позиция корректная; 1 если была корректировка
static char updateVScrollPos(common_info_t *pcommon, line_info_t *pfirstLine)
{
  long int diff = SCREEN_DISTANCE(pcommon) - LINE_DISTANCE(pcommon, pfirstLine->number);

  if (diff < 0 || diff >= pcommon->di.averageCharY)
  {
    long int lineDistance = LINE_DISTANCE(pcommon, pfirstLine->number);
    pcommon->vscrollPos = lineDistance;
    return 1;
  }
  return 0;
}

static char isEmptyRectangle(const RECT *prect)
{
  return (!prect || (prect->left == prect->right && prect->bottom == prect->top)) ? 1 : 0;
}

static char isRectangleInside(const RECT *poutside, const RECT *pinside)
{
  return poutside->top <= pinside->top &&
         poutside->bottom >= pinside->bottom &&
         poutside->left <= pinside->left &&
         poutside->right >= pinside->right;
}

static void addInvalidRectangle(common_info_t *pcommon, const RECT *prect)
{
  if (!isEmptyRectangle(prect))
  {
    int i, k, removed = 0;

    for (i = 0; i < pcommon->di.numberOfInvalidRectangles; i++)
    {
      if (isRectangleInside(&pcommon->di.invalidRectangles[i], prect))
        return; // nothing to do
    }

    for (i = 0; i < pcommon->di.numberOfInvalidRectangles; i++)
    {
      if (isRectangleInside(prect, &pcommon->di.invalidRectangles[i]))
      {
        memset(&pcommon->di.invalidRectangles[i], 0, sizeof(RECT));
        removed++;
      }
    }

    i = 0;  // empty rectangle
    k = pcommon->di.numberOfInvalidRectangles - 1;  // not empty rectangle

    while (TRUE)
    {
      while (i < pcommon->di.numberOfInvalidRectangles && !isEmptyRectangle(&pcommon->di.invalidRectangles[i]))
        i++;

      while (k > i && isEmptyRectangle(&pcommon->di.invalidRectangles[k]))
        k--;

      if (k > i)
      {
        pcommon->di.invalidRectangles[i] = pcommon->di.invalidRectangles[k];
        memset(&pcommon->di.invalidRectangles[k], 0, sizeof(RECT));
      }
      else
        break;
    }

    pcommon->di.numberOfInvalidRectangles -= removed;

    if (pcommon->di.numberOfInvalidRectangles < INVALID_RECTANGLES_HISTORY_SIZE)
    {
      pcommon->di.invalidRectangles[pcommon->di.numberOfInvalidRectangles] = *prect;
      pcommon->di.numberOfInvalidRectangles++;
    }
    else
    {
      RECT merged;
      merged.top = min(pcommon->di.invalidRectangles[0].top, prect->top);
      merged.left = min(pcommon->di.invalidRectangles[0].left, prect->left);
      merged.bottom = max(pcommon->di.invalidRectangles[0].bottom, prect->bottom);
      merged.right = max(pcommon->di.invalidRectangles[0].right, prect->right);
      addInvalidRectangle(pcommon, &merged);
    }
  }
}

static void scrollInvalidRectangles(common_info_t *pcommon, int scrollX, int scrollY)
{
  if (scrollX != 0 || scrollY != 0)
  {
    int i;

    for (i = 0; i < pcommon->di.numberOfInvalidRectangles; i++)
    {
      RECT *prect = &pcommon->di.invalidRectangles[i];
      prect->left += scrollX;
      prect->right += scrollX;
      prect->top += scrollY;
      prect->bottom += scrollY;

      prect->left = max(min(prect->left, pcommon->clientX), 0);
      prect->right = max(min(prect->right, pcommon->clientX), 0);
      prect->top = max(min(prect->top, pcommon->clientY), 0);
      prect->bottom = max(min(prect->bottom, pcommon->clientY), 0);

      // todo: remove empty rectangles
    }
  }
}


// функция делает недействительной, но не перерисовывает часть окна;
extern void SEInvalidateRect(common_info_t *pcommon, const RECT *prect, int bErase)
{
  if (prect)
  {
    addInvalidRectangle(pcommon, prect);
  }
  else
  {
    RECT rc;
    rc.top = 0;
    rc.bottom = pcommon->clientY;
    rc.left = 0;
    rc.right = pcommon->clientX;
    addInvalidRectangle(pcommon, &rc);
  }
}

// функция перерисовывает часть окна rc; изменяет vscrollPos, если это необходимо
extern void SEUpdateWindow(common_info_t *pcommon, caret_info_t *pcaret, line_info_t *pfirstLine)
{
  char fullInvalidating = 0;

  if (updateVScrollPos(pcommon, pfirstLine))
  {
    UpdateCaretPos(pcommon, pcaret, pfirstLine);
    SEInvalidateRect(pcommon, NULL, FALSE);
    fullInvalidating = 1;
  }

  // We must scroll window if all of next conditions are true:
  // 1. if it is possible - drawing is permitted and there are no real invalid regions on screen
  // 2. if it makes sense - scrollX and scrollY variables are non zero and not whole screen is invalid
  if (!pcommon->di.isDrawingRestricted && !fullInvalidating && (pcommon->di.scrollX != 0 || pcommon->di.scrollY != 0) && !GetUpdateRect(pcommon->hwnd, NULL, FALSE))
  {
    scrollInvalidRectangles(pcommon, pcommon->di.scrollX, pcommon->di.scrollY);

#ifndef UNDER_CE

    ScrollWindowEx(pcommon->hwnd, pcommon->di.scrollX, pcommon->di.scrollY, NULL, NULL, 0, NULL, SW_INVALIDATE);

#else

    // Win CE feature - both horizontal and vertical scrolling is not acceptable in ScrollWindowEx. Here is a workaround.
    {
      HDC hdc = GetDC(pcommon->hwnd);
      int destX = pcommon->di.scrollX > 0 ? pcommon->di.scrollX : 0;
      int destY = pcommon->di.scrollY > 0 ? pcommon->di.scrollY : 0;
      int srcX = pcommon->di.scrollX > 0 ? 0 : -pcommon->di.scrollX;
      int srcY = pcommon->di.scrollY > 0 ? 0 : -pcommon->di.scrollY;
      int width = pcommon->clientX - (pcommon->di.scrollX > 0 ? pcommon->di.scrollX : -pcommon->di.scrollX);
      int height = pcommon->clientY - (pcommon->di.scrollY > 0 ? pcommon->di.scrollY : -pcommon->di.scrollY);

      BitBlt(hdc, destX, destY, width, height, hdc, srcX, srcY, SRCCOPY);

      ReleaseDC(pcommon->hwnd, hdc);
    }

    // Let's invalidate region manually
    if (pcommon->di.scrollX)
    {
      RECT rect;
      rect.top = 0;
      rect.bottom = pcommon->clientY;
      rect.left = pcommon->di.scrollX > 0 ? 0 : pcommon->clientX + pcommon->di.scrollX;
      rect.right = pcommon->di.scrollX > 0 ? pcommon->di.scrollX : pcommon->clientX;
      SEInvalidateRect(pcommon, &rect, FALSE);
    }
    if (pcommon->di.scrollY)
    {
      RECT rect;
      rect.left = 0;
      rect.right = pcommon->clientX;
      rect.top = pcommon->di.scrollY > 0 ? 0 : pcommon->clientY + pcommon->di.scrollY;
      rect.bottom = pcommon->di.scrollY > 0 ? pcommon->di.scrollY : pcommon->clientY;
      SEInvalidateRect(pcommon, &rect, FALSE);
    }

#endif

    pcommon->di.scrollX = pcommon->di.scrollY = 0;
  }

  if (!pcommon->di.isDrawingRestricted && pcommon->di.numberOfInvalidRectangles > 0)
  {
    int i;

    for (i = 0; i < pcommon->di.numberOfInvalidRectangles; i++)
      InvalidateRect(pcommon->hwnd, &pcommon->di.invalidRectangles[i], FALSE);

    pcommon->di.numberOfInvalidRectangles = 0;
  }

  UpdateWindow(pcommon->hwnd);
}

// функция перерисовывает регион
// range - регион
// posType - SEDIT_VISUAL_LINE/SEDIT_REALLINE
extern char SEInvalidateRange(window_t *pwindow, const range_t *prange, char posType)
{
  if (prange != 0)
  {
    RECT rc;
    pline_t pline;
    range_t range = *prange;

    SortRange(&range);

    // переведём позицию в отображаемые линии, если она задана в реальных
    if (posType == SEDIT_REALLINE)
    {
      pline = ConvertRealPosToVisual(GetNearestRealLine(&pwindow->caret, &pwindow->firstLine, &pwindow->lists, range.start.y), &range.start);
      (void)ConvertRealPosToVisual(GetNearestRealLine(&pwindow->caret, &pwindow->firstLine, &pwindow->lists, range.end.y), &range.end);
    }
    else
      pline = GetLineFromVisualPos(GetNearestVisualLine(&pwindow->caret, &pwindow->firstLine, &pwindow->lists, range.start.y), range.start.y, NULL);

    rc.left = 0;
    rc.right = pwindow->common.clientX;
    rc.top = LINE_Y(&pwindow->common, range.start.y);
    rc.bottom = LINE_Y(&pwindow->common, range.end.y + 1);

    if (range.start.y == range.end.y)
    {
      int *dx = (pline == pwindow->caret.line.pline) ? pwindow->caret.dx : NULL;
      HDC hdc = GetDC(pwindow->common.hwnd);
      SelectObject(hdc, pwindow->common.hfont);

      rc.left = (int)GetStringSizeX(hdc, &pwindow->common.di, pline->prealLine->str, pline->begin, range.start.x, dx)
        - 1 - pwindow->common.hscrollPos + pwindow->common.di.leftIndent;
      rc.left = max(rc.left, 0);
      rc.right = (int)GetStringSizeX(hdc, &pwindow->common.di, pline->prealLine->str, pline->begin, range.end.x, dx)
        + 1 - pwindow->common.hscrollPos + pwindow->common.di.leftIndent;
      rc.right = min(rc.right, (int)pwindow->common.clientX);

      ReleaseDC(pwindow->common.hwnd, hdc);
    }

    SEInvalidateRect(&pwindow->common, &rc, FALSE);
  }
  else
    SEInvalidateRect(&pwindow->common, NULL, FALSE); // if no range for update given, let's update all window.

  //if (updateImmediately)
  //  SEInvalidateAndUpdate(&pwindow->common, &pwindow->caret, &pwindow->firstLine, &rc);
  //else

  return SEDIT_NO_ERROR;
}

// функция сортирует диапазон по возрастанию
extern void SortRange(range_t *prange)
{
  if (prange->start.y > prange->end.y || (prange->start.y == prange->end.y && prange->start.x > prange->end.x))
  {
    position_t temp = prange->start;
    prange->start = prange->end;
    prange->end = temp;
  }
}

// функция запрещает отображение. Все области для перерисовки суммируются и сохраняются
extern void RestrictDrawing(common_info_t *pcommon, caret_info_t *pcaret, line_info_t *pfirstLine)
{
  if (!pcommon->di.isDrawingRestricted)
  {
    //pcommon->di.wasCaretVisibleBeforeDrawingRestriction = pcaret->type & CARET_VISIBLE;
    pcommon->di.numberOfInvalidRectangles = 0;
    pcommon->di.isDrawingRestricted = 1;
  }
}

// функция разрешает отображение. Все накопленные области будут перерисованы.
extern void AllowDrawing(common_info_t *pcommon, caret_info_t *pcaret, line_info_t *pfirstLine)
{
  if (pcommon->di.isDrawingRestricted)
  {
    pcommon->di.isDrawingRestricted = 0;

    hideCaret(pcommon);

    pcommon->SBFlags |= UPDATE_SCROLLBARS;

    // Update Scrollbars
    UpdateScrollBars(pcommon, pcaret, pfirstLine);

    // Update text in the window
    SEUpdateWindow(pcommon, pcaret, pfirstLine);

    // Update caret position
    UpdateCaretPos(pcommon, pcaret, pfirstLine);
  }
}

// определяет на сколько строк нужно передвинуть pfirstLine, если позиция по Y изменится на diff пикселей
extern int GetVScrollCount(common_info_t *pcommon, line_info_t *pfirstLine, long int diff)
{
  long int vsurplus = SCREEN_DISTANCE(pcommon) - LINE_DISTANCE(pcommon, pfirstLine->number);
  int count;

  _ASSERTE(vsurplus >= 0 && vsurplus < pcommon->di.averageCharY);

  /*_ASSERTE((vsurplus >= 0 && vsurplus < pcommon->di.averageCharY) ||
           (vsurplus < 0 && screenDistance < pcommon->di.upIndent));

  if (screenDistance < pcommon->di.upIndent)
  {
    if (diff > 0)
    {
      diff = max(0, diff - pcommon->di.upIndent + screenDistance);
      count = diff / pcommon->di.averageCharY;
    }
    else
      count = 0;
  }
  else
  {*/

  if (diff > 0)
    count = (vsurplus + diff) / pcommon->di.averageCharY;
  else if (diff < 0)
  {
    count = (diff + vsurplus - pcommon->di.averageCharY) / pcommon->di.averageCharY;
    if ((diff + vsurplus - pcommon->di.averageCharY) % pcommon->di.averageCharY == 0)
      ++count;  // count < 0 = > ++count;
    _ASSERTE(count <= 0);
  }
  else
    count = 0;

  return count;
}

/* Функция вычисляет необходимую позицию по Y, чтобы строка number была на экране
 * ПАРАМЕТРЫ:
 *  pcommon - общая информация об окне
 *  pfirstLine - верхняя строка
 *  number - строка, которая должна быть видна
 *
 * ВОЗВРАЩАЕТ: позицию ползунка по Y
 */
extern int GetVertScrollPos(common_info_t *pcommon, line_info_t *pfirstLine, unsigned int number)
{
  long int lineDistance = LINE_DISTANCE(pcommon, number);
  long int screenDistance = SCREEN_DISTANCE(pcommon);
  int newPos;

  if (lineDistance - screenDistance < 0 || pcommon->clientY <= pcommon->di.averageCharY)
    newPos = lineDistance;
  else if (lineDistance - screenDistance > pcommon->clientY - pcommon->di.averageCharY)
    newPos = lineDistance + pcommon->di.averageCharY - pcommon->clientY;
  else
    newPos = pcommon->vscrollPos;

  return newPos;
}

// перемещает окно к позиции newPos
extern void VScrollWindow(common_info_t *pcommon, caret_info_t *pcaret, line_info_t *pfirstLine, int newPos)
{
  int count;
  int lastPos = pcommon->vscrollPos;

  newPos = max(newPos, 0);
  newPos = min(newPos, pcommon->vscrollRange);
  if (newPos == pcommon->vscrollPos)
    return;

  count = GetVScrollCount(pcommon, pfirstLine, newPos - pcommon->vscrollPos);

  while (count < 0 && pfirstLine->number > 0)
  {
    pfirstLine->pline = pfirstLine->pline->prev;
    pfirstLine->number--;
    if (pfirstLine->pline->prealLine != pfirstLine->pline->next->prealLine)
      pfirstLine->realNumber--;
    count++;
  }
  while (count > 0 && pfirstLine->pline->next)
  {
    pfirstLine->pline = pfirstLine->pline->next;
    pfirstLine->number++;
    if (pfirstLine->pline->prealLine != pfirstLine->pline->prev->prealLine)
      pfirstLine->realNumber++;
    count--;
  }

  if (count > 0) // переместить до конца не удалось - сработало ограничение по тексту
    updateVScrollPos(pcommon, pfirstLine);
  else
    pcommon->vscrollPos = newPos;

  //if (!pcommon->di.isDrawingRestricted && pcommon->di.isInvalidatedRectEmpty)
  if (!pcommon->di.isDrawingRestricted && 0 == pcommon->di.numberOfInvalidRectangles)
  {
    ScrollWindowEx(pcommon->hwnd, 0, -(pcommon->vscrollPos - lastPos), 0, NULL, NULL, NULL, SW_INVALIDATE);
    SetScrollPos(pcommon->hwnd, SB_VERT, pcommon->vscrollPos / pcommon->vscrollCoef, TRUE);
    UpdateWindow(pcommon->hwnd);
    UpdateCaretPos(pcommon, pcaret, pfirstLine);
  }
  else
  {
    pcommon->di.scrollY -= (pcommon->vscrollPos - lastPos);
    //SetScrollPos(pcommon->hwnd, SB_VERT, pcommon->vscrollPos / pcommon->vscrollCoef, TRUE);
    //UpdateCaretPos(pcommon, pcaret, pfirstLine);

    //SetScrollPos(pcommon->hwnd, SB_VERT, pcommon->vscrollPos / pcommon->vscrollCoef, TRUE);
    //UpdateCaretPos(pcommon, pcaret, pfirstLine);
    //SEInvalidateRect(pcommon, NULL, 0);
  }
}


/* Функция вычисляет необходимую позицию по X, чтобы символ с индексом index был на экране
 * ПАРАМЕТРЫ:
 *  pcommon - общая информация об окне
 *  pline - строка
 *  index - индекс символа в строке
 *  dx - массив ширин символов для строки pline
 *
 * ВОЗВРАЩАЕТ: позицию ползунка по X
 */
extern int GetHorzScrollPos(HDC hdc, common_info_t *pcommon, pline_t pline, unsigned int index, int *dx)
{
  unsigned long sizeX;
  int scrollPos = pcommon->hscrollPos;

  _ASSERTE(index <= pline->end && (hdc || dx));

  sizeX = GetStringSizeX(hdc, &pcommon->di, pline->prealLine->str, pline->begin, index, dx);

  if (sizeX + pcommon->di.leftIndent> (unsigned)pcommon->hscrollPos + pcommon->clientX)
    scrollPos = sizeX + pcommon->di.leftIndent - pcommon->clientX + 1; // прокрутка вправо
  else if (sizeX + pcommon->di.leftIndent < (unsigned)pcommon->hscrollPos)  // прокрутка влево
    scrollPos = sizeX + pcommon->di.leftIndent;  // прокрутка влево

  return scrollPos;
}

// перемещает окно к позиции newPos по оси X
extern void HScrollWindow(common_info_t *pcommon, caret_info_t *pcaret, line_info_t *pfirstLine, int newPos)
{
  int lastHScrollPos = pcommon->hscrollPos;

  pcommon->hscrollPos = newPos;
  pcommon->hscrollPos = max(pcommon->hscrollPos, 0);
  pcommon->hscrollPos = min(pcommon->hscrollPos, pcommon->hscrollRange);

  if (pcommon->hscrollPos != lastHScrollPos)
  {
    //if (!pcommon->di.isDrawingRestricted && pcommon->di.isInvalidatedRectEmpty)
    if (!pcommon->di.isDrawingRestricted && 0 == pcommon->di.numberOfInvalidRectangles)
    {
      ScrollWindowEx(pcommon->hwnd, - (pcommon->hscrollPos - lastHScrollPos), 0, 0, NULL, NULL, NULL, SW_INVALIDATE);
      SetScrollPos(pcommon->hwnd, SB_HORZ, pcommon->hscrollPos / pcommon->hscrollCoef, TRUE);
      UpdateWindow(pcommon->hwnd);
      UpdateCaretPos(pcommon, pcaret, pfirstLine);
    }
    else
    {
      pcommon->di.scrollX -= (pcommon->hscrollPos - lastHScrollPos);
      //SetScrollPos(pcommon->hwnd, SB_HORZ, pcommon->hscrollPos / pcommon->hscrollCoef, TRUE);
      //UpdateCaretPos(pcommon, pcaret, pfirstLine);
      //SEInvalidateRect(pcommon, NULL, 0);
    }
  }
}


/* Функция удаляет строки, начиная с psLine с индексом si до peLine c индексом ei
 * ПАРАМЕТРЫ:
 *   pline - строка из которой удалять
 *   si - индекс в строке pline, откуда начинать удаление
 *   peLine - последняя строка для удаления
 *   ei - индекс в строке peline, докуда будут удалены символы
 */
static void deleteStrings(pline_t psLine, unsigned int si, pline_t peLine, unsigned int ei,
                          unsigned int *nDeletedLines, unsigned int *nDeletedRealLines)
{
  unsigned int nDelLines = 0, nDelRealLines = 0;
  pline_t pline, ptmpLine;
  size_t psRealLength, peRealLength, n;

//  peRealLength = psRealLength = wcslen(psLine->prealLine->str);
  peRealLength = psRealLength = RealStringLength(psLine);
  if (psLine->prealLine != peLine->prealLine)
    peRealLength = RealStringLength(peLine);
    //peRealLength = wcslen(peLine->prealLine->str);

  if (psLine != peLine)  // удаляем много строк
  {
    n = psLine->end - si;
    DeleteSubString(psLine, psRealLength, si, n);
    psRealLength -= n;
    if (psLine->prealLine == peLine->prealLine)
    {
      peRealLength -= n;
      ei -= n;
    }

    pline = psLine->next;
    while (pline != peLine)
    {
      if (pline->prealLine != psLine->prealLine && pline->prealLine != peLine->prealLine)
      {
        // удалим реальную строку
        psLine->prealLine->next = pline->prealLine->next;
        free(pline->prealLine->str);
        free(pline->prealLine);
        ++nDelRealLines;

        // удалим все соответствующие ей отобаражаемые
        while (pline->prealLine == pline->next->prealLine)
        {
          nDelLines++;
          ptmpLine = pline->next;
          deleteLine(pline);
          pline = ptmpLine;
        }
        nDelLines++;
        ptmpLine = pline->next;
        deleteLine(pline);
        pline = ptmpLine;
      }
      else if (pline->prealLine == psLine->prealLine && pline->prealLine != peLine->prealLine)
      {
        nDelLines++;
        n = pline->end - pline->begin;
        DeleteSubString(pline, psRealLength, pline->begin, pline->end - pline->begin);
        psRealLength -= n;
        ptmpLine = pline->next;
        deleteLine(pline);
        pline = ptmpLine;
      }
      else if (pline->prealLine == peLine->prealLine)
      {
        nDelLines++;
        n = pline->end - pline->begin;
        DeleteSubString(pline, peRealLength, pline->begin, pline->end - pline->begin);
        peRealLength -= n;
        ei -= n;
        ptmpLine = pline->next;
        deleteLine(pline);
        pline = ptmpLine;
      }
      else
        MessageBox(NULL, L"Delete Strings Loop!", NULL, MB_OK);
    }

    n = ei - pline->begin;
    DeleteSubString(pline, peRealLength, pline->begin, n);
    peRealLength -= n;

    if (psLine->prealLine != peLine->prealLine)  // надо совместить строки
    {
      preal_line_t peRealLine = peLine->prealLine;

      InsertSubString(psLine, psRealLength, si, peLine->prealLine->str, peRealLength);
      psLine->prealLine->next = peLine->prealLine->next;
      free(peLine->prealLine->str);
      free(peLine->prealLine);
      ++nDelRealLines;
      while (pline && pline->prealLine == peRealLine)
      {
        nDelLines++;
        ptmpLine = pline->next;
        deleteLine(pline);
        pline = ptmpLine;
      }
    }
  }
  else // удаляем подстроку
    DeleteSubString(psLine, psRealLength, si, ei - si);

  *nDeletedLines = nDelLines;
  *nDeletedRealLines = nDelRealLines;
}

/* Функция устанавливает параметры отображения по умолчанию и обрабатывает текущий текст(word wrap)
 * ПАРАМЕТРЫ:
 *   pcommon - общая информация об окне
 *   pcaret - информация о каретке
 *   pfirstLine - первая строка на экране
 *   pfakeHead - голова списка отображаемых строк в документе
 */
static void resetDocument(common_info_t *pcommon,
                          caret_info_t *pcaret,
                          pselection_t pselection,
                          line_info_t *pfirstLine,
                          undo_info_t *pundo,
                          pline_t pfakeHead)
{
  HDC hdc;
  range_t range = {0};

  // Установим верхнюю линию в начало
  pfirstLine->pline = pfakeHead->next;
  pfirstLine->realNumber = pfirstLine->number = 0;

  // Уcтановим каретку в начало
  pcaret->line.pline = pfakeHead->next;
  pcaret->line.realNumber = pcaret->line.number = 0;
  pcaret->index = pfakeHead->next->begin;
  pcaret->realSizeX = pcaret->sizeX = 0;
  pcaret->length = 0;
  pcaret->type |= CARET_IS_MODIFIED;

  pselection->isSelection = 0;  // аналагично ChangeSelection(..., FALSE)
  ClearSavedSel(pselection);

  // очистим стек undo
  ClearUndo(pundo);

  // Обновим остальные параметры и обработаем строки
  pcommon->SBFlags |= UPDATE_SCROLLBARS;  // чтобы UpdateScrollBars точно обновила полосы прокрутки
  pcommon->SBFlags &= PERMANENT_SCROLLBARS | UPDATE_SCROLLBARS;

  hdc = GetDC(pcommon->hwnd);
  SelectObject(hdc, pcommon->hfont);
  RewrapText(hdc, pcommon, pcaret, pselection, pfirstLine, pfakeHead);
  ReleaseDC(pcommon->hwnd, hdc);

  UpdateCaretPos(pcommon, pcaret, pfirstLine);

  SEInvalidateRect(pcommon, NULL, FALSE);
  SEUpdateWindow(pcommon, pcaret, pfirstLine);
  UpdateScrollBars(pcommon, pcaret, pfirstLine);

  DocUpdated(pcommon, SEDIT_ACTION_NONE, &range, 0);
}

/* Функция выводит символ форматирования
 * ПАРАМЕТРЫ:
 *   hdc - контекст устройства
 *   pline - строка
 *   dx - массив с расстановкой символов в строке
 *   index - индекс символа в строке
 *   x - указатель на переменную с позицией по X для вывода (меняется)
 *   y - позиция по Y для вывода
 */
static void drawFormatSymbol(HDC hdc,
                             const pline_t pline,
                             const int *dx,
                             unsigned int index,
                             unsigned int *x,
                             unsigned int y)
{
  _ASSERTE(index >= pline->begin && index < pline->end);

  if (isSpaceSym(pline->prealLine->str[index]))
  {
    *x += dx[index + 1 - pline->begin] - dx[index - pline->begin];
  }
}

/* Функция вставляет позицию в массив символов форматирования
 * ПАРАМЕТРЫ:
 *   fsym - массив позиций в строке
 *   n - количество элементов в массиве
 *   point - позиция, которую надо добавить
 *   type - тип позиции
 */
static void insertPoint(line_point_t *linePoints, unsigned int n, unsigned int point, const unsigned char data[4])
{
  unsigned int i;
  line_point_t tmp;

  _ASSERTE(data);

  linePoints[n].i = point;
  for (i = 0; i < 4; ++i)
    linePoints[n].data[i] = data[i];

  i = n;
  while (i > 0 && linePoints[i - 1].i >= point)
  {
    tmp = linePoints[i - 1];
    linePoints[i - 1] = linePoints[i];
    linePoints[i] = tmp;
    i--;
  }
}

/* Функция возвращает массив индексов позиций в строке, в которых меняется какое-либо свойство отображения
 * ПАРАМЕТРЫ:
 *   pcommon - общая информация об окне
 *   piLine - информация о строке
 *   pstrRange - подстрока для вывода строки (символов)
 *   pbkRange - подстрока для вывода фона
 *   [out] nLinePoints - количество позиций в pcommon->linePoints
 *   [out] nBkLinePoints - количество позиций в pcommon->bkLinePoints
 *
 * ВОЗВРАЩАЕТ: код ошибки
 */
static char getStringSpecialPoints(common_info_t *pcommon,
                                   line_info_t *piLine,
                                   pselection_t psel,
                                   const position_t *pstrRange,
                                   const position_t *pbkRange,
                                   int *nLinePoints,
                                   int *nBkLinePoints)
{
  #define MAS_SIZE 10

  const wchar_t *str = piLine->pline->prealLine->str;
  const COLORREF* colors = NULL;
  unsigned int n = 0, m = 0;
  unsigned int i, si, ei;

  // сначала заполним массив символами форматирования (обычно их немного)
  for (i = pstrRange->x; i < pstrRange->y; i++)
  {
    if (isFormatSym(str[i]))
    {
      if (n >= pcommon->linePointsSize && reallocStrPositions(&pcommon->linePoints, &pcommon->linePointsSize, n + MAS_SIZE) != SEDIT_NO_ERROR)
        return SEDIT_MEMORY_ERROR;
      pcommon->linePoints[n].i = i;
      pcommon->linePoints[n].data[0] = FORMAT_SYM;
      ++n;
    }
  }

  // затем добавим цвета
  if (pstrRange->x < pstrRange->y && pcommon->di.GetColorsForString &&
    (colors =  pcommon->di.GetColorsForString(pcommon->hwnd, pcommon->id, str, pstrRange->x, pstrRange->y, piLine->realNumber)) != NULL)
  {
    unsigned char data[4] = {COLOR_CHANGE, 0, 0, 0};

    if (n >= pcommon->linePointsSize && reallocStrPositions(&pcommon->linePoints, &pcommon->linePointsSize, n + MAS_SIZE) != SEDIT_NO_ERROR)
      return SEDIT_MEMORY_ERROR;
    data[1] = GetRValue(colors[0]);
    data[2] = GetGValue(colors[0]);
    data[3] = GetBValue(colors[0]);
    insertPoint(pcommon->linePoints, n, pstrRange->x, data);
    ++n;

    for (i = 1; i < pstrRange->y - pstrRange->x; ++i)
    {
      if (colors[i] != colors[i - 1])
      {
        if (n >= pcommon->linePointsSize && reallocStrPositions(&pcommon->linePoints, &pcommon->linePointsSize, n + MAS_SIZE) != SEDIT_NO_ERROR)
          return SEDIT_MEMORY_ERROR;
        data[1] = GetRValue(colors[i]);
        data[2] = GetGValue(colors[i]);
        data[3] = GetBValue(colors[i]);
        insertPoint(pcommon->linePoints, n, i + pstrRange->x, data);
        ++n;
      }
    }
  }

  // затем получим карту фона
  if (pbkRange->x < pbkRange->y && pcommon->di.GetBkColorsForString &&
      (colors =  pcommon->di.GetBkColorsForString(pcommon->hwnd, pcommon->id, str, pbkRange->x, pbkRange->y, piLine->realNumber)) != NULL)
  {
    unsigned char data[4] = {COLOR_CHANGE, 0, 0, 0};

    if (m + 1 >= pcommon->bkLinePointsSize && reallocStrPositions(&pcommon->bkLinePoints, &pcommon->bkLinePointsSize, m + MAS_SIZE) != SEDIT_NO_ERROR)
      return SEDIT_MEMORY_ERROR;

    // фон может выводится в области вне строки. Поэтому для начала добавим смену цвета в конце строки.
    data[1] = GetRValue(pcommon->di.bgColor);
    data[2] = GetGValue(pcommon->di.bgColor);
    data[3] = GetBValue(pcommon->di.bgColor);
    insertPoint(pcommon->bkLinePoints, m++, pbkRange->y, data);

    // затем добавим первый цвет
    data[1] = GetRValue(colors[0]);
    data[2] = GetGValue(colors[0]);
    data[3] = GetBValue(colors[0]);
    insertPoint(pcommon->bkLinePoints, m++, pbkRange->x, data);


    for (i = 1; i < pbkRange->y - pbkRange->x; ++i)
    {
      if (colors[i] != colors[i - 1])
      {
        if (m >= pcommon->bkLinePointsSize && reallocStrPositions(&pcommon->bkLinePoints, &pcommon->bkLinePointsSize, m + MAS_SIZE) != SEDIT_NO_ERROR)
          return SEDIT_MEMORY_ERROR;
        data[1] = GetRValue(colors[i]);
        data[2] = GetGValue(colors[i]);
        data[3] = GetBValue(colors[i]);
        insertPoint(pcommon->bkLinePoints, m++, i + pbkRange->x, data);
      }
    }
  }

  // теперь добавим выделение (для строки)
  if (GetSelectionBorders(psel, piLine, pstrRange->x, pstrRange->y, &si, &ei) == TRUE)
  {
    char data[4];

    if (!(pcommon->di.selectionTransparency & SEDIT_SELTEXTTRANSPARENT))
    {
      if (pcommon->linePointsSize < n + 2 && reallocStrPositions(&pcommon->linePoints, &pcommon->linePointsSize, n + 2) != SEDIT_NO_ERROR)
        return SEDIT_MEMORY_ERROR;
      data[0] = SELECTION_END;
      insertPoint(pcommon->linePoints, n++, ei, data);
      data[0] = SELECTION_BEGIN;
      insertPoint(pcommon->linePoints, n++, si, data);
    }
  }

  // теперь добавим выделение (для фона)
  if (GetSelectionBorders(psel, piLine, pbkRange->x, pbkRange->y, &si, &ei) == TRUE)
  {
    char data[4];

    if (!(pcommon->di.selectionTransparency & SEDIT_SELBGTRANSPARENT))
    {
      if (pcommon->bkLinePointsSize < m + 2 && reallocStrPositions(&pcommon->bkLinePoints, &pcommon->bkLinePointsSize, m + 2) != SEDIT_NO_ERROR)
        return SEDIT_MEMORY_ERROR;
      data[0] = SELECTION_END;
      insertPoint(pcommon->bkLinePoints, m++, ei, data);
      data[0] = SELECTION_BEGIN;
      insertPoint(pcommon->bkLinePoints, m++, si, data);
    }
  }

  *nLinePoints = n;
  *nBkLinePoints = m;

  return SEDIT_NO_ERROR;
  #undef MAS_SIZE
}


/* Функция находит первый символ в подстроке, который не помещается в максимальную длину отображаемой строки
 * ПАРАМЕТРЫ:
 *   hdc - контекст устройства или NULL
 *   str - строка
 *   begin - индекс начала подстроки
 *   end - индекс конца подстроки
 *   dx - массив с размерами символьных ячеек или NULL
 *   maxLength - максималная длина отображаемой строки в пикселях
 *   size - указатель на переменную, принимающую размер подстроки
 *
 * ВОЗВРАЩАЕТ: индекс первого непоместившегося символа
 */
static int getFirstFitIndex(HDC hdc,
                            display_info_t *pdi,
                            wchar_t *str,
                            unsigned int begin,
                            unsigned int end,
                            long maxLength,
                            int *dx,
                            long *size)
{
  unsigned int i = 0, oldBegin;

  _ASSERTE((hdc || dx) && end >= begin);

  if (maxLength <= 0 || end == begin)
  {
    if (size)
      *size = 0;
    return begin;
  }

  if (!dx)
  {
    if (reallocDX(&pdi->dx, &pdi->dxSize, end - begin) != SEDIT_NO_ERROR)
      return SEDIT_MEMORY_ERROR;
    getLinePlacement(hdc, pdi, &str[begin], end - begin, pdi->dx);
    dx = pdi->dx;
  }

  // попытаемся угадать, откуда начинать искать
  oldBegin = begin;
  i = min(begin + maxLength / pdi->averageCharX, end);
  i = max(i, begin + 1);

  while (i > begin)
  {
    if (dx[i - oldBegin] - dx[0] < maxLength)
      begin = i;
    else if (dx[i - oldBegin] - dx[0] > maxLength)
      end = i;
    else
    {
      if (size)
        *size = maxLength;
      return i;
    }
    i = (begin + end) / 2;
  }

  if (begin < end && dx[end - oldBegin] - dx[0] <= maxLength)
  {
    _ASSERTE(begin + 1 == end);
    begin = end;
  }
  _ASSERTE(dx[begin - oldBegin] - dx[0] <= maxLength);

  if (size)
    *size = dx[begin - oldBegin] - dx[0];
  return begin;
}

/* Функция находит первое слово в подстроке, которое не помещается в максимальную длину отображаемой строки
 * ПАРАМЕТРЫ:
 *   hdc - контекст устройства или NULL
 *   str - строка
 *   begin - индекс начала подстроки
 *   end - индекс конца подстроки
 *   dx - массив с размерами ячеек или NULL
 *   maxLength - максималная длина отображаемой строки в пикселях
 *
 * ВОЗВРАЩАЕТ: индекс первого непоместившегося слова
 */
static unsigned int getSeparateIndex(HDC hdc,
                                     display_info_t *pdi,
                                     wchar_t *str,
                                     unsigned int begin,
                                     unsigned int end,
                                     long maxLength,
                                     int *dx)
{
  unsigned int i = 0;

  _ASSERTE(hdc || dx);

  i = getFirstFitIndex(hdc, pdi, str, begin, end, maxLength, dx, NULL);

  if (i < end && !IsBreakSym(str[i]))
  {
    while (i > begin && !IsBreakSym(str[i]))
      i--;
    if (IsBreakSym(str[i]))   // это слово в середине строки => перенос надо начинать с начала слова, а не с пробела
      i++;
  }

  return i;
}

static long int getLineDrawParams(HDC hdc,
                                  common_info_t *pcommon, line_info_t *piLine,
                                  long int startX, long int endX,
                                  position_t *pstrRange, position_t *pbkRange,
                                  pix_position_t *pstrRangeX, pix_position_t *pbkRangeX,
                                  int *dx)
{
  //long int startX = *_startX, endX = *_endX;
  unsigned int si, ei;
  wchar_t *str = piLine->pline->prealLine->str;
  long int distance = pcommon->hscrollPos + startX - pcommon->di.leftIndent;
  long int sizeX;

  _ASSERTE(hdc && dx);

  // 1. Найдём индекс начала и, соответственно, координату на экране для вывода
  si = getFirstFitIndex(hdc, &pcommon->di, str, piLine->pline->begin, piLine->pline->end, distance, dx, &sizeX);
  startX = sizeX + pcommon->di.leftIndent - pcommon->hscrollPos;

  //2. Найдём индекс и координату конца вывода.
  ei = getFirstFitIndex(hdc, &pcommon->di, str, si, piLine->pline->end, endX - startX, &dx[si - piLine->pline->begin], &sizeX);
  endX = startX + sizeX;

  pstrRange->x = si;
  pbkRange->x = si;
  pstrRangeX->x = startX;
  pbkRangeX->x = startX;

  if (pcommon->di.isItalic && si > piLine->pline->begin)
  {
    // добавим для вывода лишний символ к строке (тк он может залезать в область фона => будет затёрт)
    --pstrRange->x;
    pstrRangeX->x -= dx[si - piLine->pline->begin] - dx[si - piLine->pline->begin - 1];
  }

  if (ei < piLine->pline->end) // italic == 0 (overhang == 0)
  {
    // просто добавим для вывода лишний символ (и к фону, и к строке)
    ++ei;
    endX += dx[ei - piLine->pline->begin] - dx[ei - piLine->pline->begin - 1];
  }

  pstrRange->y = ei;
  pbkRange->y = ei;
  pbkRangeX->y = endX;
  pbkRangeX->y = endX;

  return distance < 0 ? -distance : 0;  // left indent area
}


static void destroyOutBuffer(display_info_t *pdi)
{
  if (pdi->hlineDC)
  {
    DeleteDC(pdi->hlineDC);
    pdi->hlineDC = 0;
  }

  if (pdi->hlineBitmap)
  {
    DeleteObject(pdi->hlineBitmap);
    pdi->hlineBitmap = 0;
  }
  pdi->bmHeight = pdi->bmWidth = 0;
}

// Функция создаёт контекст устройства и битмап для вывода строк
static char createOutBuffer(HDC hdc, display_info_t *pdi, long int width, long int height)
{
  //int maxDimension = max(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
  _ASSERTE(pdi->hlineBitmap == 0 && pdi->hlineDC == 0);

  if ((pdi->hlineDC = CreateCompatibleDC(hdc)) == 0)
    return 0;

  if (!UpdateBitmaps(hdc, pdi, width, height))
  {
    destroyOutBuffer(pdi);
    return 0;
  }

  return 1;
}

/*static void drawSelection(HDC hdc, RECT *prc, display_info_t *pdi, COLORREF bgColor)
{
  COLORREF color;
  int width = prc->right - prc->left;
  int height = prc->bottom - prc->top;
  RECT rc;

  rc.left = rc.top = 0;
  rc.right = width + 1;
  rc.bottom = height + 1;

  FillRect(pdi->hselDC, &rc, pdi->hselBGBrush);

  //SetMapMode(hdcTemp, GetMapMode(hdc));

  // Create mask
  color = SetBkColor(hdc, bgColor);
  BitBlt(pdi->hmaskDC, 0, 0, width, height, hdc, prc->left, prc->top, SRCCOPY);
  SetBkColor(hdc, color);

  // Create inversed mask
  BitBlt(pdi->hbackDC, 0, 0, width, height, pdi->hmaskDC, 0, 0, NOTSRCCOPY);

  // apply mask to the BG bitmap;
  BitBlt(pdi->hselDC, 0, 0, width, height, pdi->hmaskDC, 0, 0, SRCAND);

  // fill area in original hdc with the selected text color
  FillRect(hdc, prc, pdi->hselTextBrush);

  // apply inversed mask to the original bitmap
  BitBlt(hdc, prc->left, prc->top, width, height, pdi->hbackDC, 0, 0, SRCAND);

  // merge original bitmap and BG bitmap
  BitBlt(hdc, prc->left, prc->top, width, height, pdi->hselDC, 0, 0, SRCPAINT);
}*/

/************************************************************************************************************/
/************************************************************************************************************/

extern unsigned int InsertSubString(pline_t pline, unsigned int length, unsigned int index, wchar_t *str, unsigned int n)
{
  unsigned int i = 1;

  _ASSERTE(index <= length);

  if (pline->prealLine->surplus < n)  //нет свободного места
  {
    pline->prealLine->surplus = n + STRING_SURPLUS;
    pline->prealLine->str  = (wchar_t*)realloc(pline->prealLine->str, sizeof(wchar_t) * (length + pline->prealLine->surplus + 1));
    _ASSERTE(pline->prealLine->str);
  }

  // сдвигаем оставшуюся часть строки
  bwcsncpy(&pline->prealLine->str[index + n], &pline->prealLine->str[index], length - index + 1);

  // вставляем подстроку
  wcsncpy(&pline->prealLine->str[index], str, n);

  // уменьшаем количество свободного пространства
  pline->prealLine->surplus -= n;

  pline->end += n;
  while (pline->next && pline->prealLine == pline->next->prealLine)
  {
    pline = pline->next;

    pline->begin += n;
    pline->end += n;
    i++;
  }

  return i;
}

extern unsigned int DeleteSubString(pline_t pline, unsigned int length, unsigned int index, unsigned int n)
{
  unsigned int i = 1;

  _ASSERTE(index + n <= length);

  wcsncpy(&pline->prealLine->str[index], &pline->prealLine->str[index + n], length - index - n + 1);

  pline->prealLine->surplus += n;
  if (pline->prealLine->surplus > 2 * STRING_SURPLUS)
  {
    pline->prealLine->surplus = STRING_SURPLUS;
    pline->prealLine->str = realloc(pline->prealLine->str, sizeof(wchar_t) * (length - n + pline->prealLine->surplus + 1));
    _ASSERTE(pline->prealLine->str);
  }

  pline->end -= n;
  while (pline->next && pline->prealLine == pline->next->prealLine)
  {
    pline = pline->next;

    pline->begin -= n;
    pline->end -= n;
    i++;
  }

  return i;
}

extern char InitRealLineList(preal_line_t *head)
{
  *head = calloc(1, sizeof(real_line_t));
  if (NULL == (*head))
    return 0;
  return 1;
}

extern void DestroyRealLineList(preal_line_t head)
{
  preal_line_t ptmp;

  while (head)
  {
    ptmp = head;
    head = head->next;
    if (ptmp->str)
      free(ptmp->str);
    free(ptmp);
  }
}

extern char InitLineList(pline_t *head)
{
  *head = calloc(1, sizeof(line_t));
  if (NULL == (*head))
    return 0;
  return 1;
}

extern void DestroyLineList(pline_t head)
{
  pline_t ptmp;

  while (head)
  {
    ptmp = head;
    head = head->next;
    free(ptmp);
  }
}

extern pline_t AddRealLine(window_t *pw, pline_t pline, const wchar_t *str, char type)
{
  HDC hdc;
  long int maxLength = pw->common.isWordWrap ? WRAP_MAX_LENGTH(&pw->common) : MAX_LENGTH;
  int processed, added;
  size_t length;
  wchar_t *wstr;

  _ASSERTE(pline != NULL);

  length = wcslen(str);
  if ((wstr = malloc(sizeof(wchar_t) * (length + 1))) == NULL)
    return NULL;
  wcscpy(wstr, str);

  hdc = GetDC(pw->common.hwnd);
  SelectObject(hdc, pw->common.hfont);

  if (type == SEDIT_APPEND)
  {
    while (pline->next && pline->next->prealLine == pline->prealLine)
      pline = pline->next;

    if (!insertRealLine(pline->prealLine, wstr))
    {
      ReleaseDC(pw->common.hwnd, hdc);
      CloseDocument(&pw->common, &pw->caret, &pw->selection, &pw->firstLine, &pw->undo, &pw->lists);
      return NULL;
    }
    if (!insertLine(pline, pline->prealLine->next))
    {
      //preal_line_t prLine = pline->prealLine->next;
      //pline->prealLine->next = prLine->next;
      //free(prLine);
      ReleaseDC(pw->common.hwnd, hdc);
      CloseDocument(&pw->common, &pw->caret, &pw->selection, &pw->firstLine, &pw->undo, &pw->lists);
      return NULL;
    }
    pline->next->begin = 0;
    pline->next->end = length;

    ReduceLine(hdc, &pw->common.di, pline->next, maxLength, &processed, &added);
    if (!pw->common.isWordWrap)
    {
      unsigned int sizeX;
      pline_t ptmpLine = pline->next;

      while (ptmpLine && ptmpLine->prealLine == pline->next->prealLine)
      {
        sizeX = GetStringSizeX(hdc, &pw->common.di, ptmpLine->prealLine->str, ptmpLine->begin, ptmpLine->end, NULL);
        if (sizeX > pw->common.maxStringSizeX)
          pw->common.maxStringSizeX = sizeX;
        ptmpLine = ptmpLine->next;
      }
    }
    pw->common.nLines += added + 1;
    ++pw->common.nRealLines;

    pline = pline->next;
  }
  else // SEDIT_REPLACE
  {
    pos_info_t pos;

    if (pw->firstLine.pline->prealLine == pline->prealLine)
    {
      pos.piLine = &pw->firstLine;
      SetStartLine(&pos);
      updateVScrollPos(&pw->common, &pw->firstLine);
    }

    if (pw->caret.line.pline->prealLine == pline->prealLine)
    {
      pos.piLine = &pw->caret.line;
      SetStartLine(&pos);
      pw->caret.index = 0;
    }

    while (pline->prev->prealLine == pline->prealLine)
      pline = pline->prev;

    pw->common.nLines -= UniteLine(pline);
    free(pline->prealLine->str);
    pline->prealLine->surplus = 0;
    pline->prealLine->str = wstr;
    pline->begin = 0;
    pline->end = length;
    ReduceLine(hdc, &pw->common.di, pline, maxLength, &processed, &added);
    if (!pw->common.isWordWrap)
    {
      unsigned int sizeX;
      pline_t ptmpLine = pline;

      while (ptmpLine && ptmpLine->prealLine == pline->prealLine)
      {
        sizeX = GetStringSizeX(hdc, &pw->common.di, ptmpLine->prealLine->str, ptmpLine->begin, ptmpLine->end, NULL);
        if (sizeX > pw->common.maxStringSizeX)
          pw->common.maxStringSizeX = sizeX;
        ptmpLine = ptmpLine->next;
      }
    }
    pw->common.nLines += added;

    if (pw->caret.line.pline->prealLine == pline->prealLine)
      getCaretLineParameters(hdc, &pw->common, &pw->caret);

    // если позиции выеления должны быть правильными, то такая замена строки может их нарушить
    if (pw->selection.isSelection || IS_SHIFT_PRESSED /*pw->selection.shiftPressed*/)
    {
      ClearSavedSel(&pw->selection);
      StartSelection(&pw->selection, &pw->caret.line, pw->caret.index);
    }
  }

  ReleaseDC(pw->common.hwnd, hdc);

  return pline;
}

extern size_t RealStringLength(pline_t pline)
{
  _ASSERTE(pline->prealLine);
  while (pline->next && pline->next->prealLine == pline->prealLine)
    pline = pline->next;

  return pline->end;
}

extern unsigned int SetStartLine(pos_info_t *ppos)
{
  unsigned int k = 0;
  unsigned int number = ppos->piLine->number;
  pline_t pline = ppos->piLine->pline;

  ppos->pos = POS_BEGIN;
  if (ppos->index == pline->end)
    ppos->pos = POS_END;

  while (pline->prev->prealLine == pline->prealLine)
  {
    pline = pline->prev;
    number--;
    k++;
    _ASSERTE(pline != NULL);
  }

  ppos->piLine->pline = pline;
  ppos->piLine->number = number;

  return k;
}

extern char UpdateLine(pos_info_t *ppos)
{
  unsigned int number = ppos->piLine->number;
  pline_t pline = ppos->piLine->pline;

  while (pline->begin > ppos->index && pline->prev->prealLine == pline->prealLine)
  {
    pline = pline->prev;
    number--;
    _ASSERTE(pline != NULL);
  }

  while (pline->next && pline->next->prealLine == pline->prealLine && pline->end < ppos->index)
  {
    pline = pline->next;
    number++;
  }

  if (ppos->pos == POS_BEGIN && ppos->index != pline->begin && ppos->index == pline->end)
  {
    if (pline->next && pline->next->prealLine == pline->prealLine)
    {
      pline = pline->next;
      number++;
    }
  }

  ppos->piLine->pline = pline;
  ppos->piLine->number = number;

  if (ppos->index > pline->end)
    return 0;
  return 1;
}


// возвращает длину подстроки
extern unsigned long GetStringSizeX(HDC hdc, display_info_t *pdi, wchar_t *str,
                                    unsigned int begin, unsigned int end, int *dx)
{
  unsigned long sizeX = 0;

  if (end <= begin)
    return 0;

  if (!dx)
  {
    if (reallocDX(&pdi->dx, &pdi->dxSize, end - begin) != SEDIT_NO_ERROR)
      return SEDIT_MEMORY_ERROR;
    getLinePlacement(hdc, pdi, &str[begin], end - begin, pdi->dx);
    dx = pdi->dx;
  }
  sizeX = dx[end - begin] - dx[0];

  return sizeX;
}

extern unsigned int GetLinesNumber(pline_t pline, line_info_t *piLine, char type)
{
  line_info_t line;

  line.number = line.realNumber = 0;
  line.pline = pline->next;

  if (piLine)
    line = *piLine;

  while (line.pline->next)
  {
    line.pline = line.pline->next;
    line.number++;
    if (line.pline->prev->prealLine != line.pline->prealLine)
      line.realNumber++;
  }

  if (type)
    return line.realNumber + 1;  // отсчёт с 0
  return line.number + 1;
}

extern char GetNumbersFromLines(pline_t pstartLine, char type, int size, pline_t *plines, unsigned int *numbers)
{
  line_info_t iLine;
  int i, founded = 0;

  iLine.pline = pstartLine;
  iLine.number = 0;
  iLine.realNumber = 0;
  while (founded != size)
  {
    for (i = 0; i < size; i++)
    {
      if (iLine.pline == plines[i])
      {
        numbers[i] = type ? iLine.realNumber : iLine.number;
        ++founded;
      }
    }

    if ((iLine.pline = iLine.pline->next) == NULL)
      break;
    iLine.number++;
    if (iLine.pline->prev->prealLine != iLine.pline->prealLine)
      iLine.realNumber++;
  }

  if (founded != size)
    return 0;
  return 1;
}

extern pline_t GetLineFromRealPos(const line_info_t *piStartLine,
                                  unsigned int realNumber,
                                  unsigned int index,
                                  unsigned int *number)
{
  line_info_t iLine = *piStartLine;
  pos_info_t pos = {&iLine, index, POS_BEGIN};

  while (iLine.realNumber > realNumber)
  {
    iLine.pline = iLine.pline->prev;
    if (iLine.pline->next->prealLine != iLine.pline->prealLine)
      --iLine.realNumber;
    --iLine.number;
  }
  SetStartLine(&pos);

  while (iLine.pline->next && iLine.realNumber < realNumber)
  {
    iLine.pline = iLine.pline->next;
    if (iLine.pline->prealLine != iLine.pline->prev->prealLine)
      ++iLine.realNumber;
    ++iLine.number;
  }
  UpdateLine(&pos);

  if (number)
    *number = iLine.number;

  return iLine.pline;
}

extern pline_t GetLineFromVisualPos(const line_info_t *piStartLine, unsigned int number, unsigned int *prealNumber)
{
  line_info_t iLine = *piStartLine;

  while (iLine.pline->prev && iLine.number > number)
  {
    iLine.pline = iLine.pline->prev;
    if (iLine.pline->next->prealLine == iLine.pline->prealLine)
      --iLine.realNumber;
    --iLine.number;
  }
  while (iLine.pline->next && iLine.number < number)
  {
    iLine.pline = iLine.pline->next;
    if (iLine.pline->prev->prealLine == iLine.pline->prealLine)
      ++iLine.realNumber;
    ++iLine.number;
  }

  if (prealNumber)
    *prealNumber = iLine.realNumber;

  return iLine.pline;
}

extern pline_t ConvertRealPosToVisual(const line_info_t *piStartLine, position_t *ppos)
{
  pline_t pline = GetLineFromRealPos(piStartLine, ppos->y, ppos->x, &ppos->y);
  ppos->x -= pline->begin;
  return pline;
}

extern line_info_t* GetNearestVisualLine(caret_info_t *pcaret, line_info_t *pfirstLine, text_t *ptext, unsigned int number)
{
  line_info_t *pnearest = &ptext->textFirstLine;
  int distance = number - pnearest->number;

  _ASSERTE(ptext->textFirstLine.pline == ptext->pfakeHead->next);

  if (abs((int)(pfirstLine->number - number)) < distance)
  {
    pnearest = pfirstLine;
    distance = abs((int)(pnearest->number - number));
  }

  if (abs((int)(pcaret->line.number - number)) < distance)
    pnearest = &pcaret->line;

  return pnearest;
}

extern line_info_t* GetNearestRealLine(caret_info_t *pcaret, line_info_t *pfirstLine, text_t *ptext, unsigned int number)
{
  line_info_t *pnearest = &ptext->textFirstLine;
  int distance = number - pnearest->realNumber;

  _ASSERTE(ptext->textFirstLine.pline == ptext->pfakeHead->next);

  if (abs((int)(pfirstLine->realNumber - number)) < distance)
  {
    pnearest = pfirstLine;
    distance = abs((int)(pnearest->realNumber - number));
  }

  if (abs((int)(pcaret->line.realNumber - number)) < distance)
    pnearest = &pcaret->line;

  return pnearest;
}

static unsigned int getReducePosition(HDC hdc, display_info_t *pdi, wchar_t *str, unsigned int begin, unsigned int end,
                                      long int maxLength, int *dx)
{
  unsigned int i;

  if (pdi->wordWrapType == SEDIT_WORDWRAP)
  {
    i = getSeparateIndex(hdc, pdi, str, begin, end, maxLength, dx);
    if (i == begin && i != end)  // строка не пустая и слово очень длинное
      i = getFirstFitIndex(hdc, pdi, str, begin, end, maxLength, dx, NULL);
  }
  else
  {
    _ASSERTE(pdi->wordWrapType == SEDIT_SYMBOLWRAP);
    i = getFirstFitIndex(hdc, pdi, str, begin, end, maxLength, dx, NULL);
  }
  if (i == begin && i < end) // символ слишком большой, чтобы вместиться в экран
    ++i;
  if (pdi->hideSpaceSyms && i < end && isSpaceSym(str[i]))
    ++i;

  return i;
}

extern pline_t ReduceLine(HDC hdc, display_info_t *pdi, pline_t pline, long maxLength, int *processed, int *added)
{
  pline_t pcurrentLine = pline, pnextLine;
  unsigned int i = pline->begin, k = 0, len = 0;

  _ASSERTE(hdc != NULL && processed != NULL);
  *added = *processed = 0;

  len = min(pcurrentLine->end - pcurrentLine->begin, MAX_CHARACTERS);
  reallocDX(&pdi->dx, &pdi->dxSize, len + 1);
  getLinePlacement(hdc, pdi, &pcurrentLine->prealLine->str[pcurrentLine->begin], len, pdi->dx);

  i = getReducePosition(hdc, pdi, pcurrentLine->prealLine->str, pcurrentLine->begin, pcurrentLine->begin + len,
                        maxLength, pdi->dx);

  /*if (i == pcurrentLine->begin && i < pcurrentLine->end) // символ слишком большой, чтобы вместиться в экран
    ++i;
  if (pdi->hideSpaceSyms && i < pcurrentLine->end && isSpaceSym(pcurrentLine->prealLine->str[i]))
    ++i;*/

  while (i < pcurrentLine->end)
  {
    pnextLine = pcurrentLine->next;

    if (!pnextLine || pnextLine->prealLine != pcurrentLine->prealLine)
    {
      ++k;
      insertLine(pcurrentLine, pcurrentLine->prealLine);
      pnextLine = pcurrentLine->next;
      pnextLine->end = pcurrentLine->end;
      ++(*added);
    }

    _ASSERTE(len >= (i - pcurrentLine->begin));
    if (len - (i - pcurrentLine->begin) < pnextLine->end - i)
    {
      // следующая строка содержит символы, для которых нет массива dx
      len = min(pnextLine->end - i, MAX_CHARACTERS);
      reallocDX(&pdi->dx, &pdi->dxSize, len + 1);
      getLinePlacement(hdc, pdi, &pnextLine->prealLine->str[i], len, pdi->dx);
    }
    else
    {
      int temp = pdi->dx[i - pcurrentLine->begin];
      unsigned int j;
      for (j = i - pcurrentLine->begin; j < len + 1; j++)
        pdi->dx[j - (i - pcurrentLine->begin)] = pdi->dx[j] - temp;
      len -= (i - pcurrentLine->begin);
    }

    pcurrentLine->end = i;
    pnextLine->begin = i;
    pcurrentLine = pnextLine;
    (*processed)++;

    i = getReducePosition(hdc, pdi, pcurrentLine->prealLine->str, pcurrentLine->begin, pcurrentLine->begin + len,
                          maxLength, pdi->dx);
  }
  pcurrentLine->end = i;

  return pcurrentLine->next;
}

extern pline_t EnlargeLine(HDC hdc, display_info_t *pdi, pline_t pline, long maxLength, int *count)
{
  pline_t pcurrentLine = pline, pnextLine = pline->next;
  unsigned int i = pline->begin, subStrLen;
  long strSizeX, difference;

  _ASSERTE(hdc != NULL && count != NULL);
  *count = -1;

  while (pnextLine && pnextLine->prealLine == pcurrentLine->prealLine)
  {
    _ASSERTE(pcurrentLine->end - pcurrentLine->begin <= MAX_CHARACTERS);
    strSizeX = GetStringSizeX(hdc, pdi, pcurrentLine->prealLine->str, pcurrentLine->begin, pcurrentLine->end, NULL);

    //if (strSizeX > maxLength)
    //  return pnextLine;

    subStrLen = min(MAX_CHARACTERS - (pcurrentLine->end - pcurrentLine->begin), pnextLine->end - pnextLine->begin);
    difference = maxLength - strSizeX;

    if (pdi->wordWrapType == SEDIT_WORDWRAP)
    {
      i = getSeparateIndex(hdc, pdi, pnextLine->prealLine->str, pnextLine->begin, pnextLine->begin + subStrLen,
                           difference, NULL);
      if (i == pnextLine->begin && isSolidString(pcurrentLine))
        i = getFirstFitIndex(hdc, pdi, pnextLine->prealLine->str, pnextLine->begin, pnextLine->begin + subStrLen,
                             difference, NULL, NULL);
    }
    else
    {
      _ASSERTE(pdi->wordWrapType == SEDIT_SYMBOLWRAP);
      i = getFirstFitIndex(hdc, pdi, pnextLine->prealLine->str, pnextLine->begin, pnextLine->begin + subStrLen,
                           difference, NULL, NULL);
    }
    if (pcurrentLine->end == pcurrentLine->begin && i == pnextLine->begin && i < pnextLine->end)
      ++i;

    // 1. Если текущий символ пробельный и включено прятанье пробельных символов, и...
    // 2. ...если в предыдущую строку ещё можно добавлять и там ещё ничего не спрятано,
    //    то добавим туда лишний символ
    if (isSpaceSym(pnextLine->prealLine->str[i]) && pdi->hideSpaceSyms)
      if (i < pnextLine->begin + subStrLen && difference >= 0)
        ++i;

    if (pnextLine->begin == i && pnextLine->end != i)
      break;  // не хватает места, чтобы перенести слово

    if (*count == -1) // что-то уже точно будет перенесено (но не факт, что строка будет удалена)
      (*count) = 0;

    // i - индекс с которого будет начинаться следующая строка

    pcurrentLine->end = i;

    if (i != pnextLine->end)
    {
      pnextLine->begin = i;  // строка ещё осталась
      break;
    }
    else
    {
      deleteLine(pnextLine); // мы перенесли всю следующую строку
      pnextLine = pcurrentLine->next;
      (*count)++;
    }
  }

  return pnextLine;
}

extern int ProcessLine(HDC hdc, display_info_t *pdi, pline_t psLine, long maxLength)
{
  int reduced, n = 1, added;
  unsigned int len;
  pline_t ptmpLine, pline = psLine->next;
  char lastLineFounded;

  _ASSERTE(psLine);
  len = psLine->end = RealStringLength(psLine);
  while (pline && pline->prealLine == psLine->prealLine)
  {
    pline->end = psLine->end;
    pline = pline->next;
    n++;
  }

  ReduceLine(hdc, pdi, psLine, maxLength, &reduced, &added);

  pline = psLine;
  lastLineFounded = 0;
  while (pline && pline->prealLine == psLine->prealLine)
  {
    if (!lastLineFounded)
    {
      if (pline->end == len)
        lastLineFounded = 1;
      pline = pline->next;
    }
    else
    {
      ptmpLine = pline->next;
      deleteLine(pline);
      pline = ptmpLine;
    }
  }

  return reduced + 1 - n;  // reduced + 1 <=> на одной он остановился => она не считается
}

extern unsigned int PrepareLines(HDC hdc, display_info_t *pdi, pline_t pfakeHead, char prepareType, long maxLength)
{
  int a, b; // не используются, нужны только для передачи в функцию
  unsigned int nLines = 0;
  pline_t pcurrentLine = pfakeHead->next;

  if (REDUCE == prepareType)
  {
    pline_t pnextLine;

    while (pcurrentLine)
    {
      pnextLine = ReduceLine(hdc, pdi, pcurrentLine, maxLength, &a, &b);

      while (pcurrentLine != pnextLine)
      {
        nLines++;
        pcurrentLine = pcurrentLine->next;
      }
    }
  }
  else if (ENLARGE == prepareType)
  {
    while (pcurrentLine)
    {
      pcurrentLine = EnlargeLine(hdc, pdi, pcurrentLine, maxLength, &a);
      nLines++;
    }
  }

  return nLines;
}

/* Функция находит максимальную длину (в пикселях) и номер отображаемой строки, начиная с pline
 * ПАРАМЕТРЫ:
 *   pline - отображаемая строка с которой начинать поиск
 *
 * ВОЗВРАЩАЕТ: длину в пикселях
 */
extern unsigned long GetMaxStringSizeX(HDC hdc, display_info_t *pdi, pline_t pline)
{
  unsigned long maxSize = 0, size;

  _ASSERTE(hdc != NULL);

  while (pline)
  {
    size = GetStringSizeX(hdc, pdi, pline->prealLine->str, pline->begin, pline->end, NULL);
    if (maxSize < size)
      maxSize = size;
    pline = pline->next;
  }

  return maxSize;
}

static void turnOnScrollBar(common_info_t *pcommon, int nBar)
{
  if (!pcommon->di.isDrawingRestricted)
  {
    SCROLLINFO si;

    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_PAGE | SIF_RANGE | SIF_POS;
    si.nPos = 0;
    si.nMin = 0;
    si.nMax = 100;
    si.nPage = 10;
    SetScrollInfo(pcommon->hwnd, nBar, &si, TRUE);
  }
}


/* Функция обновляет параметры скроллбаров
 * ПАРАМЕТРЫ:
 *   hwnd - дескриптор окна
 *   pcommon - указатель на структуру с общей информацией об окне
 *   pcaret - указатель на структуру с информацией о каретке
 *   pfirstLine - первая строка на экране
 *
 * ВОЗВРАЩАЕТ: количество пролистанных строк
 */
extern void UpdateScrollBars(common_info_t *pcommon, caret_info_t *pcaret, line_info_t *pfirstLine)
{
  long int range;
  int oldHScrollRange = pcommon->hscrollRange, oldVScrollRange = pcommon->vscrollRange;
  char reset = pcommon->SBFlags & RESET_SCROLLBARS, update = pcommon->SBFlags & UPDATE_SCROLLBARS;
  SCROLLINFO si;

  _ASSERTE(pcommon->vscrollPos >= 0 && pcommon->hscrollPos >= 0);

  pcommon->SBFlags &= (~(RESET_SCROLLBARS | UPDATE_SCROLLBARS));

  if (reset)
  {
    update = 1;
    turnOnScrollBar(pcommon, SB_HORZ);
    turnOnScrollBar(pcommon, SB_VERT);
  }

  range = 0;
  if ((long)pcommon->nLines * pcommon->di.averageCharY > pcommon->clientY)
    range = (long)pcommon->nLines * pcommon->di.averageCharY;
  pcommon->vscrollCoef = (unsigned short)(range / VSCROLL_MAX_RANGE + DEFAULT_VSCROLL_COEF);
  si.nMin = 0;
  si.nMax = range / pcommon->vscrollCoef;
  if (range % pcommon->vscrollCoef != 0)
    ++si.nMax;
  si.nPage = pcommon->clientY / pcommon->vscrollCoef;
  if (!si.nPage)
    si.nPage = 1;
  pcommon->vscrollRange = si.nMax > (int)si.nPage ? (si.nMax - si.nPage + 1) * pcommon->vscrollCoef : 0;

  if (!pcommon->vscrollRange)
    VScrollWindow(pcommon, pcaret, pfirstLine, 0);
  else
    VScrollWindow(pcommon, pcaret, pfirstLine, min(pcommon->vscrollRange, pcommon->vscrollPos));

  if (!pcommon->di.isDrawingRestricted && (pcommon->vscrollRange != oldVScrollRange || update))
  {
    si.cbSize = sizeof(SCROLLINFO);
    si.nPos = pcommon->vscrollPos / pcommon->vscrollCoef;
    si.fMask = SIF_PAGE | SIF_RANGE | SIF_POS;
    if ((pcommon->SBFlags & PERMANENT_SCROLLBARS))
      si.fMask |= SIF_DISABLENOSCROLL;
    SetScrollInfo(pcommon->hwnd, SB_VERT, &si, TRUE);
  }


  range = 0;
  if (pcommon->clientX && (long)pcommon->maxStringSizeX + pcommon->di.leftIndent > pcommon->clientX)
    range = (long)pcommon->maxStringSizeX + pcommon->di.leftIndent;
  pcommon->hscrollCoef = (unsigned short)(range / HSCROLL_MAX_RANGE + DEFAULT_HSCROLL_COEF);
  si.nMin = 0;
  si.nMax = range / pcommon->hscrollCoef;
  if (range % pcommon->hscrollCoef != 0)
    ++si.nMax;
  si.nPage = pcommon->clientX / pcommon->hscrollCoef;
  if (!si.nPage)
    si.nPage = 1;
  pcommon->hscrollRange = si.nMax > (int)si.nPage ? (si.nMax - si.nPage + 1) * pcommon->hscrollCoef : 0;

  if (!pcommon->hscrollRange)
    HScrollWindow(pcommon, pcaret, pfirstLine, 0);
  else
    HScrollWindow(pcommon, pcaret, pfirstLine, min(pcommon->hscrollRange, pcommon->hscrollPos));

  if (!pcommon->di.isDrawingRestricted && (pcommon->hscrollRange != oldHScrollRange || update))
  {
    si.cbSize = sizeof(SCROLLINFO);
    si.nPos = pcommon->hscrollPos / pcommon->hscrollCoef;
    si.fMask = SIF_PAGE | SIF_RANGE | SIF_POS;
    if ((pcommon->SBFlags & PERMANENT_SCROLLBARS))
      si.fMask |= SIF_DISABLENOSCROLL;
    SetScrollInfo(pcommon->hwnd, SB_HORZ, &si, TRUE);
  }
}

/* Функция вычисляет новые координаты каретки
 * ПАРАМЕТРЫ:
 *   pcommon - указатель на структуру с общей информацией об окне
 *   pcaret - информация о каретки
 *   pfirstLine - первая строка на экране
 */
extern void UpdateCaretPos(common_info_t *pcommon, caret_info_t *pcaret, line_info_t *pfirstLine)
{
  long int diffY = pcommon->di.averageCharY * pcaret->line.number - pcommon->vscrollPos;
  long int distanceX = pcommon->hscrollPos;

  if (!(pcaret->type & CARET_EXIST))
    return;

  pcaret->type = pcaret->type & (~(CARET_VISIBLE | CARET_PARTIALLY_VISIBLE));

  if (diffY > pcommon->clientY || (diffY < 0 && abs(diffY) >= pcommon->di.averageCharY))
  {
    // каретка не находится на экране
    hideCaret(pcommon);
    return;
  }

  if (distanceX > pcaret->sizeX + pcommon->di.leftIndent || distanceX + pcommon->clientX < pcaret->sizeX + pcommon->di.leftIndent)
  {
    // каретка не видна, так как находится по оси X левее или правее экрана
    hideCaret(pcommon);
    return;
  }

  if (diffY >= 0 && diffY > pcommon->clientY - pcommon->di.averageCharY)
    pcaret->type = pcaret->type | CARET_PARTIALLY_VISIBLE;
  else if (diffY < 0 && abs(diffY) < pcommon->di.averageCharY)
    pcaret->type = pcaret->type | CARET_PARTIALLY_VISIBLE;
  else
    pcaret->type = pcaret->type | CARET_VISIBLE;

  pcaret->posY = diffY;
  pcaret->posX = pcaret->sizeX + pcommon->di.leftIndent - pcommon->hscrollPos;

  if (!pcommon->di.isDrawingRestricted)
    SetCaretPos(pcaret->posX, pcaret->posY);
}

/* Функция по линии и индексу вычисляет расстояния до каретки
 * ПАРАМЕТРЫ:
 *   pcommon - указатель на структуру с общей информацией об окне
 *   pcaret - информация о каретки
 */
extern void UpdateCaretSizes(HDC hdc, common_info_t *pcommon, caret_info_t *pcaret)
{
  getCaretLineParameters(hdc, pcommon, pcaret);
  pcaret->realSizeX = pcaret->sizeX = GetStringSizeX(hdc,
                                                     &pcommon->di,
                                                     pcaret->line.pline->prealLine->str,
                                                     pcaret->line.pline->begin,
                                                     pcaret->index,
                                                     pcaret->dx);
}

/* Функция возвращает экран к каретке если её не видно
 * ПАРАМЕТРЫ:
 *   pcommon - указатель на структуру с общей информацией об окне
 *   pcaret - информация о каретки
 *   pfirstLine - первая строка на экране
 *   hscrollPos - позиция по оси X
 */
extern void GotoCaret(common_info_t *pcommon, caret_info_t *pcaret, line_info_t *pfirstLine)
{
  _ASSERTE(pcaret->line.pline && pfirstLine->pline);

  UpdateCaretPos(pcommon, pcaret, pfirstLine);
  if (pcaret->type & CARET_VISIBLE)
    return;  // каретка и так на экране

  VScrollWindow(pcommon, pcaret, pfirstLine, GetVertScrollPos(pcommon, pfirstLine, pcaret->line.number));
  HScrollWindow(pcommon, pcaret, pfirstLine, GetHorzScrollPos(NULL, pcommon, pcaret->line.pline, pcaret->index, pcaret->dx));

  _ASSERTE(pcaret->line.pline && pfirstLine->pline);
  _ASSERTE(pcaret->line.number >= pfirstLine->number);

  UpdateCaretPos(pcommon, pcaret, pfirstLine); // установим каретку
}

/* Функция получает позицию для каретки из координат
 * ПАРАМЕТРЫ:
 *   pcommon - указатель на структуру с общей информацией об окне
 *   pfirstLine - первая строка на экране
 *   presultPosition - для результата
 *   pindex - для результата
 *   x - координата по X
 *   y - координата по Y
 */
extern void GetCaretPosFromCoords(common_info_t *pcommon,
                                  line_info_t *pfirstLine,
                                  line_info_t *presultPosition,
                                  int *pindex,
                                  long x,
                                  long y)
{
  HDC hdc;
  long int sizeX;
  int count = GetVScrollCount(pcommon, pfirstLine, y);

  *presultPosition = *pfirstLine;
  _ASSERTE(count >= 0);

  while (count > 0 && presultPosition->pline->next)
  {
    presultPosition->pline = presultPosition->pline->next;
    presultPosition->number++;
    if (presultPosition->pline->prealLine != presultPosition->pline->prev->prealLine)
      presultPosition->realNumber++;
    --count;
  }

  hdc = GetDC(pcommon->hwnd);
  SelectObject(hdc, pcommon->hfont);
  reallocDX(&pcommon->di.dx, &pcommon->di.dxSize, presultPosition->pline->end - presultPosition->pline->begin);
  getLinePlacement(hdc, &pcommon->di, &presultPosition->pline->prealLine->str[presultPosition->pline->begin], presultPosition->pline->end - presultPosition->pline->begin, pcommon->di.dx);
  ReleaseDC(pcommon->hwnd, hdc);

  *pindex = getFirstFitIndex(
    NULL,
    &pcommon->di,
    presultPosition->pline->prealLine->str,
    presultPosition->pline->begin,
    presultPosition->pline->end,
    x + pcommon->hscrollPos - pcommon->di.leftIndent,
    pcommon->di.dx,
    &sizeX
  );
}

/* Функция устанавливает курсор по координатам в окне
 * ПАРАМЕТРЫ:
 *   pcommon - указатель на структуру с общей информацией об окне
 *   pcaret - информация о каретки
 *   pfirstLine - первая строка на экране
 *   x - координата по X
 *   y - координата по Y
 */
extern void SetCaretPosFromCoords(common_info_t *pcommon,
                                  caret_info_t *pcaret,
                                  line_info_t *pfirstLine,
                                  long x,
                                  long y)
{
  long int sizeX;
  line_info_t oldLine = pcaret->line;
  int count = GetVScrollCount(pcommon, pfirstLine, y);

  pcaret->line = *pfirstLine;
  _ASSERTE(count >= 0);

  while (count > 0 && pcaret->line.pline->next)
  {
    pcaret->line.pline = pcaret->line.pline->next;
    pcaret->line.number++;
    if (pcaret->line.pline->prealLine != pcaret->line.pline->prev->prealLine)
      pcaret->line.realNumber++;
    --count;
  }

  if (oldLine.number != pcaret->line.number)
  {
    HDC hdc = GetDC(pcommon->hwnd);
    SelectObject(hdc, pcommon->hfont);
    getCaretLineParameters(hdc, pcommon, pcaret);
    ReleaseDC(pcommon->hwnd, hdc);
  }
  pcaret->index = getFirstFitIndex(NULL,
                                   &pcommon->di,
                                   pcaret->line.pline->prealLine->str,
                                   pcaret->line.pline->begin,
                                   pcaret->line.pline->end,
                                   x + pcommon->hscrollPos - pcommon->di.leftIndent,
                                   pcaret->dx,
                                   &sizeX);

  if (pcaret->index < pcaret->line.pline->end && (pcaret->dx[pcaret->index + 1 - pcaret->line.pline->begin] -
    pcaret->dx[pcaret->index - pcaret->line.pline->begin])/2 < x + pcommon->hscrollPos - pcommon->di.leftIndent - sizeX)
  {
    sizeX += pcaret->dx[pcaret->index + 1 - pcaret->line.pline->begin] - pcaret->dx[pcaret->index - pcaret->line.pline->begin];
    ++pcaret->index;
  }
  pcaret->realSizeX = pcaret->sizeX = sizeX;
  GotoCaret(pcommon, pcaret, pfirstLine);

  if (oldLine.realNumber != pcaret->line.realNumber)
  {
    // перерисуем старую и новую линии с кареткой
    RedrawCaretLine(pcommon, pcaret, pfirstLine, &oldLine);
    RedrawCaretLine(pcommon, pcaret, pfirstLine, &pcaret->line);
  }

  CaretModified(pcommon, pcaret, pfirstLine, CARET_LEFT);
}

extern void SetMainPositions(common_info_t *pcommon,
                             caret_info_t *pcaret,
                             line_info_t *pfirstLine,
                             pselection_t pselection,
                             doc_info_t *pdoc)
{
  char shiftPressed = IS_SHIFT_PRESSED;

  if (pdoc->flags & SEDIT_SETWINDOWPOS)
  {
    VScrollWindow(pcommon, pcaret, pfirstLine, pdoc->windowPos.y);
    HScrollWindow(pcommon, pcaret, pfirstLine, pdoc->windowPos.x);
  }

  if (pdoc->flags & SEDIT_SETCARETPOS)
  {
    line_info_t oldLine = pcaret->line;
    HDC hdc;

    if (!shiftPressed && pselection->isSelection)
    {
      pselection->isSelection = 0;
      UpdateSelection(pcommon, pcaret, pfirstLine, pselection, pcaret->dx);
    }

    while (pcaret->line.pline->next && pcaret->line.number < pdoc->caretPos.y)
    {
      pcaret->line.pline = pcaret->line.pline->next;
      pcaret->line.number++;
      if (pcaret->line.pline->prev->prealLine != pcaret->line.pline->prealLine)
        pcaret->line.realNumber++;
    }

    while (pcaret->line.number > pdoc->caretPos.y)
    {
      pcaret->line.pline = pcaret->line.pline->prev;
      pcaret->line.number--;
      if (pcaret->line.pline->next->prealLine != pcaret->line.pline->prealLine)
        pcaret->line.realNumber--;
    }

    pcaret->index = max(pdoc->caretPos.x, pcaret->line.pline->begin);
    pcaret->index = min(pcaret->index, pcaret->line.pline->end);

    hdc = GetDC(pcommon->hwnd);
    SelectObject(hdc, pcommon->hfont);
    UpdateCaretSizes(hdc, pcommon, pcaret);
    ReleaseDC(pcommon->hwnd, hdc);
    UpdateCaretPos(pcommon, pcaret, pfirstLine);

    if (shiftPressed)
    {
      ChangeSelection(pselection, &pcaret->line, pcaret->index);
      UpdateSelection(pcommon, pcaret, pfirstLine, pselection, pcaret->dx);
    }

    if (oldLine.realNumber != pcaret->line.realNumber)
    {
      // перерисуем старую и новую линии с кареткой
      RedrawCaretLine(pcommon, pcaret, pfirstLine, &oldLine);
      RedrawCaretLine(pcommon, pcaret, pfirstLine, &pcaret->line);
    }

    CaretModified(pcommon, pcaret, pfirstLine, 0);
  }
  else if (pdoc->flags & SEDIT_SETCARETREALPOS)
  {
    pos_info_t caretPos;
    line_info_t oldLine = pcaret->line;
    HDC hdc;

    if (!shiftPressed && pselection->isSelection)
    {
      pselection->isSelection = 0;
      UpdateSelection(pcommon, pcaret, pfirstLine, pselection, pcaret->dx);
    }

    while (pcaret->line.pline->next && pcaret->line.realNumber < pdoc->caretPos.y)
    {
      pcaret->line.pline = pcaret->line.pline->next;
      pcaret->line.number++;
      if (pcaret->line.pline->prev->prealLine != pcaret->line.pline->prealLine)
        pcaret->line.realNumber++;
    }

    while (pcaret->line.realNumber > pdoc->caretPos.y)
    {
      pcaret->line.pline = pcaret->line.pline->prev;
      pcaret->line.number--;
      if (pcaret->line.pline->next->prealLine != pcaret->line.pline->prealLine)
        pcaret->line.realNumber--;
    }

    pcaret->index = caretPos.index = pdoc->caretPos.x;
    caretPos.piLine = &pcaret->line;
    caretPos.pos = POS_BEGIN;
    if (!UpdateLine(&caretPos))
      pcaret->index = pcaret->line.pline->end;

    hdc = GetDC(pcommon->hwnd);
    SelectObject(hdc, pcommon->hfont);
    UpdateCaretSizes(hdc, pcommon, pcaret);
    ReleaseDC(pcommon->hwnd, hdc);
    UpdateCaretPos(pcommon, pcaret, pfirstLine);

    if (shiftPressed)
    {
      ChangeSelection(pselection, &pcaret->line, pcaret->index);
      UpdateSelection(pcommon, pcaret, pfirstLine, pselection, pcaret->dx);
    }

    if (oldLine.realNumber != pcaret->line.realNumber)
    {
      // перерисуем старую и новую линии с кареткой
      RedrawCaretLine(pcommon, pcaret, pfirstLine, &oldLine);
      RedrawCaretLine(pcommon, pcaret, pfirstLine, &pcaret->line);
    }

    CaretModified(pcommon, pcaret, pfirstLine, 0);
  }
}

/* Функция перемещает каретку на один символ влево
 * ПАРАМЕТРЫ:
 *   pcommon - указатель на структуру с общей информацией об окне
 *   pcaret - информация о каретки
 *   pfirsLine - указатель на первую экранную строку
 */
extern void MoveCaret(common_info_t *pcommon, caret_info_t *pcaret, line_info_t *pfirstLine, unsigned int n, char dir)
{
  unsigned int symbols = 0;
  char isLineChanged = 0;
  line_info_t oldLine = pcaret->line;

  _ASSERTE(pfirstLine->pline && pcaret->line.pline);

  if (dir == CARET_LEFT || dir == CARET_LEFT_WITHOUT_MODIFICATION)  // влево
  {
    while (n != symbols)
    {
      if (pcaret->line.pline->begin == pcaret->index)
      {
        if (!pcaret->line.number)
          break;

        if (pcaret->line.pline->prealLine != pcaret->line.pline->prev->prealLine)
        {
          symbols++;   // переход на новую реальную строку - один символ
          pcaret->line.realNumber--;
        }
        pcaret->line.pline = pcaret->line.pline->prev;
        pcaret->line.number--;
        pcaret->index = pcaret->line.pline->end;
        isLineChanged = 1;
      }
      else
      {
        pcaret->index--;
        symbols++;
      }
    }
  }
  else if (dir == CARET_RIGHT || dir == CARET_RIGHT_WITHOUT_MODIFICATION)  // вправо
  {
    while (n != symbols)
    {
      if (pcaret->line.pline->end == pcaret->index)
      {
        if (!pcaret->line.pline->next)
          break;

        if (pcaret->line.pline->prealLine != pcaret->line.pline->next->prealLine)
        {
          symbols++;   // переход на новую реальную строку - один символ
          pcaret->line.realNumber++;
        }
        pcaret->line.pline = pcaret->line.pline->next;
        pcaret->line.number++;
        pcaret->index = pcaret->line.pline->begin;
        isLineChanged = 1;
      }
      else
      {
        pcaret->index++;
        symbols++;
      }
    }
  }

  // в принципе, можно воспользоваться UpdateCaretSizes, но она обязательно вызовет getCaretLineParameters
  if (isLineChanged)
  {
    HDC hdc = GetDC(pcommon->hwnd);
    SelectObject(hdc, pcommon->hfont);
    getCaretLineParameters(hdc, pcommon, pcaret);
    ReleaseDC(pcommon->hwnd, hdc);
  }
  pcaret->realSizeX = pcaret->sizeX = GetStringSizeX(NULL,
                                                     &pcommon->di,
                                                     pcaret->line.pline->prealLine->str,
                                                     pcaret->line.pline->begin,
                                                     pcaret->index,
                                                     pcaret->dx);
  GotoCaret(pcommon, pcaret, pfirstLine);

  if (oldLine.realNumber != pcaret->line.realNumber)
  {
    // перерисуем старую и новую линии с кареткой
    RedrawCaretLine(pcommon, pcaret, pfirstLine, &oldLine);
    RedrawCaretLine(pcommon, pcaret, pfirstLine, &pcaret->line);
  }

  _ASSERTE(pfirstLine->pline && pcaret->line.pline);
  CaretModified(pcommon, pcaret, pfirstLine, dir);
}

// передвигает курсор к определённой позиции в тексте. (например в начало/конец слова)
extern void MoveCaretToPosition(common_info_t *pcommon, caret_info_t *pcaret, line_info_t *pfirstLine, char position)
{
  unsigned int i = pcaret->index;

  _ASSERTE(pfirstLine->pline && pcaret->line.pline);
  switch (position)
  {
  case POS_WORDBEGIN:
    i = getWordPos(pcaret->line.pline->prealLine->str, pcaret->index, POS_WORDBEGIN);
    MoveCaret(pcommon, pcaret, pfirstLine, pcaret->index - i, CARET_LEFT);
    break;

  case POS_WORDEND:
    i = getWordPos(pcaret->line.pline->prealLine->str, pcaret->index, POS_WORDEND);
    MoveCaret(pcommon, pcaret, pfirstLine, i - pcaret->index, CARET_RIGHT);
    break;

  case POS_STRINGBEGIN:
    pcaret->index = pcaret->line.pline->begin;
    pcaret->realSizeX = pcaret->sizeX = 0;          // обновим расстояния до каретки
    GotoCaret(pcommon, pcaret, pfirstLine);   // если она не на экране, то перейдём к ней
    CaretModified(pcommon, pcaret, pfirstLine, 0);
    break;

  case POS_STRINGEND:
    pcaret->index = pcaret->line.pline->end;     // новое положение каретки

    // обновим расстояния до каретки
    pcaret->realSizeX = pcaret->sizeX = GetStringSizeX(NULL,
                                                       &pcommon->di,
                                                       pcaret->line.pline->prealLine->str,
                                                       pcaret->line.pline->begin,
                                                       pcaret->index,
                                                       pcaret->dx);
    GotoCaret(pcommon, pcaret, pfirstLine);   // если она не на экране, то перейдём к ней
    CaretModified(pcommon, pcaret, pfirstLine, 1); // 1 потому что иначе будет переход на след. строку
    break;

  case POS_REALLINEBEGIN:
    MoveCaret(pcommon, pcaret, pfirstLine, pcaret->index, CARET_LEFT_WITHOUT_MODIFICATION);
    break;

  case POS_REALLINEEND:
    MoveCaret(pcommon, pcaret, pfirstLine, pcaret->length - pcaret->index, CARET_RIGHT_WITHOUT_MODIFICATION);
    break;
  }
  _ASSERTE(pfirstLine->pline && pcaret->line.pline);
}

/* Функция вызывается при любой модификации положения каретки
 * ПАРАМЕТРЫ:
 *   pcommon - указатель на структуру с общей информацией об окне
 *   pcaret - информация о каретки
 *   direction - в какую сторону произошла модификация каретки (0/1 - направо/налево)
 */

extern void CaretModified(common_info_t *pcommon, caret_info_t *pcaret, line_info_t *pfirstLine, char direction)
{
  HWND hwndParent;

  _ASSERTE(pcaret->line.pline);
  _ASSERTE(pcaret->length == RealStringLength(pcaret->line.pline));
  _ASSERTE(pcaret->index <= pcaret->length);

  pcaret->type = pcaret->type | CARET_IS_MODIFIED;
  if ((hwndParent = GetParent(pcommon->hwnd)) != NULL)
  {
    sen_caret_modified_t sen;
    sen.header.idFrom = pcommon->id;
    sen.header.hwndFrom = pcommon->hwnd;
    sen.header.code = SEN_CARETMODIFIED;
    sen.realPos.x = pcaret->index;
    sen.realPos.y = pcaret->line.realNumber;
    sen.screenPos.x = pcaret->index - pcaret->line.pline->begin;
    sen.screenPos.y = pcaret->line.number;
    SendMessage(hwndParent, WM_NOTIFY, (WPARAM)pcommon->id, (LPARAM)&sen);
  }

  ActionOnCaretModified(pcommon);

  if (pcommon->di.hideSpaceSyms &&
      (pcaret->line.pline->next && pcaret->line.pline->next->prealLine == pcaret->line.pline->prealLine) &&
      pcaret->index > 0 && pcaret->index == pcaret->line.pline->end &&
      isSpaceSym(pcaret->line.pline->prealLine->str[pcaret->index - 1]))
  {
    if ((long)GetStringSizeX(NULL, &pcommon->di, pcaret->line.pline->prealLine->str,
          pcaret->line.pline->begin, pcaret->index, pcaret->dx) + pcommon->di.leftIndent >=
          pcommon->clientX + pcommon->hscrollPos)
    {
      switch (direction)
      {
      case CARET_RIGHT:
        MoveCaretRight(pcommon, pcaret, pfirstLine);
        break;

      case CARET_LEFT:
        MoveCaretLeft(pcommon, pcaret, pfirstLine);
        break;
      }
    }
  }
}

/* Функция перемещает каретку на один символ влево
 * ПАРАМЕТРЫ:
 *   pcommon - указатель на структуру с общей информацией об окне
 *   pcaret - информация о каретки
 *   hscrollPos - позиция по оси X
 */
extern void MoveCaretLeft(common_info_t *pcommon, caret_info_t *pcaret, line_info_t *pfirstLine)
{
  _ASSERTE(pcaret->line.pline->prealLine->str);
  if (pcaret->index > pcaret->line.pline->begin)
  {
    pcaret->index--;
    pcaret->posX -= pcaret->dx[pcaret->index - pcaret->line.pline->begin + 1] - pcaret->dx[pcaret->index - pcaret->line.pline->begin];
    pcaret->realSizeX = pcaret->sizeX = pcaret->dx[pcaret->index - pcaret->line.pline->begin];

    // тут лучше не использовать GotoCaret, так как там есть вызов GetHorzScrollPos, которая вычисляет длину всей строки
    if (pcaret->posX <= 0)  // прокрутка
      HScrollWindow(pcommon, pcaret, pfirstLine, max(0, pcommon->hscrollPos + pcaret->posX - 1));

    UpdateCaretPos(pcommon,pcaret, pfirstLine);
    CaretModified(pcommon, pcaret, pfirstLine, CARET_LEFT);
  }
  else if (pcaret->line.number > 0)
  {
    MoveCaretUp(pcommon, pcaret, pfirstLine);
    MoveCaretToPosition(pcommon, pcaret, pfirstLine, POS_STRINGEND);
  }
}

/* Функция перемещает каретку на один символ вправо
 * ПАРАМЕТРЫ:
 *   pcommon - указатель на структуру с общей информацией об окне
 *   pcaret - информация о каретки
 *   hscrollPos - позиция по оси X
 */
extern void MoveCaretRight(common_info_t *pcommon, caret_info_t *pcaret, line_info_t *pfirstLine)
{
  _ASSERTE(pfirstLine->pline && pcaret->line.pline);
  _ASSERTE(pcaret->line.pline->prealLine->str);
  if (pcaret->index < pcaret->line.pline->end)
  {
    pcaret->index++;
    pcaret->posX += pcaret->dx[pcaret->index - pcaret->line.pline->begin] - pcaret->dx[pcaret->index - pcaret->line.pline->begin - 1];
    pcaret->realSizeX = pcaret->sizeX = pcaret->dx[pcaret->index - pcaret->line.pline->begin];

    // тут лучше не использовать GotoCaret, так как там есть вызов GetHorzScrollPos, которая вычисляет длину всей строки
    if (pcaret->posX > pcommon->clientX)
      HScrollWindow(pcommon, pcaret, pfirstLine, pcommon->hscrollPos + pcaret->posX - pcommon->clientX + 1);
    UpdateCaretPos(pcommon,pcaret, pfirstLine);
    CaretModified(pcommon, pcaret, pfirstLine, CARET_RIGHT);
  }
  else if (pcaret->line.number + 1 < pcommon->nLines)
  {
    MoveCaretDown(pcommon, pcaret, pfirstLine);
    MoveCaretToPosition(pcommon, pcaret, pfirstLine, POS_STRINGBEGIN);
  }
  _ASSERTE(pfirstLine->pline && pcaret->line.pline);
}

/* Функция перемещает каретку на одну строку вверх
 * ПАРАМЕТРЫ:
 *   pcommon - указатель на структуру с общей информацией об окне
 *   pcaret - информация о каретки
 *   pfirstLine - первая строка на экране
 */
extern void MoveCaretUp(common_info_t *pcommon, caret_info_t *pcaret, line_info_t  *pfirstLine)
{
  _ASSERTE(pfirstLine->pline && pcaret->line.pline);
  if (pcaret->line.number > 0)
  {
    long sizeX;
    line_info_t oldLine = pcaret->line;
    HDC hdc;

    pcaret->line.pline = pcaret->line.pline->prev;
    pcaret->line.number--;
    if (pcaret->line.pline->prealLine != pcaret->line.pline->next->prealLine)
      pcaret->line.realNumber--;

    hdc = GetDC(pcommon->hwnd);
    SelectObject(hdc, pcommon->hfont);
    getCaretLineParameters(hdc, pcommon, pcaret);
    ReleaseDC(pcommon->hwnd, hdc);
    pcaret->index = getFirstFitIndex(NULL,
                                     &pcommon->di,
                                     pcaret->line.pline->prealLine->str,
                                     pcaret->line.pline->begin,
                                     pcaret->line.pline->end,
                                     pcaret->realSizeX,
                                     pcaret->dx,
                                     &sizeX);

    pcaret->sizeX = pcaret->posX = sizeX;

    HScrollWindow(pcommon, pcaret, pfirstLine, GetHorzScrollPos(NULL, pcommon, pcaret->line.pline, pcaret->index, pcaret->dx));
    VScrollWindow(pcommon, pcaret, pfirstLine, GetVertScrollPos(pcommon, pfirstLine, pcaret->line.number));
    UpdateCaretPos(pcommon,pcaret, pfirstLine);

    // перерисуем старую и новую линии с кареткой
    if (oldLine.realNumber != pcaret->line.realNumber)
    {
      RedrawCaretLine(pcommon, pcaret, pfirstLine, &oldLine);
      RedrawCaretLine(pcommon, pcaret, pfirstLine, &pcaret->line);
    }
    CaretModified(pcommon, pcaret, pfirstLine, CARET_LEFT);
  }
  _ASSERTE(pfirstLine->pline && pcaret->line.pline);
}

/* Функция перемещает каретку на одну строку вниз
 * ПАРАМЕТРЫ:
 *   pcommon - указатель на структуру с общей информацией об окне
 *   pcaret - информация о каретки
 *   pfirstLine - первая строка на экране
 */
extern void MoveCaretDown(common_info_t *pcommon, caret_info_t *pcaret, line_info_t  *pfirstLine)
{
  _ASSERTE(pfirstLine->pline && pcaret->line.pline);
  if (pcaret->line.number + 1 < pcommon->nLines)  // отсчёт линий с 0
  {
    long sizeX;
    line_info_t oldLine = pcaret->line;
    HDC hdc;

    pcaret->line.pline = pcaret->line.pline->next;
    pcaret->line.number++;
    if (pcaret->line.pline->prealLine != pcaret->line.pline->prev->prealLine)
      pcaret->line.realNumber++;

    hdc = GetDC(pcommon->hwnd);
    SelectObject(hdc, pcommon->hfont);
    getCaretLineParameters(hdc, pcommon, pcaret);
    ReleaseDC(pcommon->hwnd, hdc);
    pcaret->index = getFirstFitIndex(NULL,
                                     &pcommon->di,
                                     pcaret->line.pline->prealLine->str,
                                     pcaret->line.pline->begin,
                                     pcaret->line.pline->end,
                                     pcaret->realSizeX,
                                     pcaret->dx,
                                     &sizeX);

    pcaret->sizeX = pcaret->posX = sizeX;

    HScrollWindow(pcommon, pcaret, pfirstLine, GetHorzScrollPos(NULL, pcommon, pcaret->line.pline, pcaret->index, pcaret->dx));
    VScrollWindow(pcommon, pcaret, pfirstLine, GetVertScrollPos(pcommon, pfirstLine, pcaret->line.number));
    UpdateCaretPos(pcommon, pcaret, pfirstLine);

    // перерисуем старую и новую линии с кареткой
    if (oldLine.realNumber != pcaret->line.realNumber)
    {
      RedrawCaretLine(pcommon, pcaret, pfirstLine, &oldLine);
      RedrawCaretLine(pcommon, pcaret, pfirstLine, &pcaret->line);
    }
    CaretModified(pcommon, pcaret, pfirstLine, CARET_LEFT);
  }
  _ASSERTE(pfirstLine->pline && pcaret->line.pline);
}

/* Функция перемещает каретку и экран вверх на nscreenLines строк
 * ПАРАМЕТРЫ:
 *   pcommon - указатель на структуру с общей информацией об окне
 *   pcaret - информация о каретки
 *   pfirstLine - первая строка на экране
 *   hscrollPos - позиция по оси X
 *   vscrollPos - позиция по оси Y
 */
extern void PageUp(common_info_t *pcommon, caret_info_t *pcaret, line_info_t *pfirstLine)
{
  int caretCount = (pcaret->line.number >= pcommon->nscreenLines) ? pcommon->nscreenLines : pcaret->line.number;
  long sizeX;
  line_info_t oldLine = pcaret->line;
  HDC hdc;

  pcaret->line.number -= caretCount;

  while (caretCount--)
  {
    pcaret->line.pline = pcaret->line.pline->prev;
    if (pcaret->line.pline->prealLine != pcaret->line.pline->next->prealLine)
      pcaret->line.realNumber--;
  }

  hdc = GetDC(pcommon->hwnd);
  SelectObject(hdc, pcommon->hfont);
  getCaretLineParameters(hdc, pcommon, pcaret);
  ReleaseDC(pcommon->hwnd, hdc);
  pcaret->index = getFirstFitIndex(NULL,
                                   &pcommon->di,
                                   pcaret->line.pline->prealLine->str,
                                   pcaret->line.pline->begin,
                                   pcaret->line.pline->end,
                                   pcaret->realSizeX,
                                   pcaret->dx,
                                   &sizeX);

  pcaret->sizeX = sizeX;

  GotoCaret(pcommon, pcaret, pfirstLine);

  // перерисуем старую и новую линии с кареткой
  if (oldLine.realNumber != pcaret->line.realNumber)
  {
    RedrawCaretLine(pcommon, pcaret, pfirstLine, &oldLine);
    RedrawCaretLine(pcommon, pcaret, pfirstLine, &pcaret->line);
  }
  CaretModified(pcommon, pcaret, pfirstLine, 0);
}

/* Функция перемещает каретку и экран вниз на nscreenLines строк
 * ПАРАМЕТРЫ:
 *   pcommon - указатель на структуру с общей информацией об окне
 *   pcaret - информация о каретки
 *   pfirstLine - первая строка на экране
 *   hscrollPos - позиция по оси X
 *   vscrollPos - позиция по оси Y
 *   vscrollRange - диапазон скроллбара по оси Y
 */
extern void PageDown(common_info_t *pcommon, caret_info_t *pcaret, line_info_t *pfirstLine)
{
  int caretCount = pcommon->nscreenLines;
  long sizeX;
  line_info_t oldLine = pcaret->line;
  HDC hdc;

  while (caretCount-- > 0 && pcaret->line.pline->next)
  {
    pcaret->line.pline = pcaret->line.pline->next;
    pcaret->line.number++;
    if (pcaret->line.pline->prealLine != pcaret->line.pline->prev->prealLine)
      pcaret->line.realNumber++;
  }

  hdc = GetDC(pcommon->hwnd);
  SelectObject(hdc, pcommon->hfont);
  getCaretLineParameters(hdc, pcommon, pcaret);
  ReleaseDC(pcommon->hwnd, hdc);
  pcaret->index = getFirstFitIndex(NULL,
                                   &pcommon->di,
                                   pcaret->line.pline->prealLine->str,
                                   pcaret->line.pline->begin,
                                   pcaret->line.pline->end,
                                   pcaret->realSizeX,
                                   pcaret->dx,
                                   &sizeX);

  pcaret->sizeX = sizeX;

  GotoCaret(pcommon, pcaret, pfirstLine);

  // перерисуем старую и новую линии с кареткой
  if (oldLine.realNumber != pcaret->line.realNumber)
  {
    RedrawCaretLine(pcommon, pcaret, pfirstLine, &oldLine);
    RedrawCaretLine(pcommon, pcaret, pfirstLine, &pcaret->line);
  }
  CaretModified(pcommon, pcaret, pfirstLine, 0);
}

/* Функция очищает текущий документ и инициализирует пустой
 * ПАРАМЕТРЫ:
 *   ptext - указатель на структуру с текстом
 */
extern char SetEmptyDocument(text_t *ptext, __out int *pnWrappedLines, __out int *pnRealLines)
{
  clearDocument(ptext->prealHead, ptext->pfakeHead);

  if ((ptext->prealHead->next = calloc(1, sizeof(real_line_t))) == NULL)
    return SEDIT_MEMORY_ERROR;

  if ((ptext->prealHead->next->str = calloc(1, sizeof(wchar_t))) == NULL)
  {
    free(ptext->prealHead->next);
    ptext->prealHead->next = NULL;
    return SEDIT_MEMORY_ERROR;
  }

  if ((ptext->pfakeHead->next = calloc(1, sizeof(line_t))) == NULL)
  {
    free(ptext->prealHead->next->str);
    free(ptext->prealHead->next);
    ptext->prealHead->next = NULL;
    return SEDIT_MEMORY_ERROR;
  }

  ptext->pfakeHead->next->prev = ptext->pfakeHead;
  ptext->pfakeHead->next->prealLine = ptext->prealHead->next;

  ptext->textFirstLine.number = ptext->textFirstLine.realNumber = 0;
  ptext->textFirstLine.pline = ptext->pfakeHead->next;

  if (pnWrappedLines)
    *pnWrappedLines = 1;
  if (pnRealLines)
    *pnRealLines = 1;

  return SEDIT_NO_ERROR;
}

/* Функция создающая из обычного текста списки исходных и отображаемых строк
 * ПАРАМЕТРЫ:
 *   text - буфер с текстом для обработки
 *   pstartLine - указатель на переменную для хранения указателя на начало списка отображаемых строк
 *   pstartLine - указатель на переменную для хранения указателя на начало списка исходных строк
 */
extern unsigned int ParseTextInMemory(wchar_t *text, pline_t *pstartLine, preal_line_t *pstartRealLine)
{
  unsigned int nLines = 0;
  preal_line_t prealLine = NULL;
  pline_t pline = NULL;
  int i = 0, tmp = 0, length;
  wchar_t *str;
  char isEnd = 0;
  unsigned int count = 0;

  _ASSERTE(pstartLine && pstartRealLine);

  while (text[i] != L'\0')
  {
    while (!isSpecialSymW(text[i]))
      i++;

    length = i - tmp;

    str = malloc((length + 1) * sizeof(wchar_t));
    wcsncpy(str, &text[tmp], length);
    str[length] = L'\0';

    tmp = i;
    while (i - tmp < CRLF_SIZE && isCRLFSym(text[i]))
    {
      if (i > tmp && isCRLFSym(text[i - 1]) && text [i - 1] == text[i])
        break;
      ++i;
    }
    tmp = i;

    if (prealLine && pline)
    {
      prealLine->next = calloc(1, sizeof(real_line_t));
      prealLine->next->str = str;
      prealLine = prealLine->next;

      pline->next = calloc(1, sizeof(line_t));
      pline->next->prealLine = prealLine;
      pline->next->prev = pline;
      pline->next->end = length;
      pline = pline->next;
      count++;
    }
    else
    {
      prealLine = calloc(1, sizeof(real_line_t));
      prealLine->str = str;
      (*pstartRealLine) = prealLine;

      pline = calloc(1, sizeof(line_t));
      pline->prealLine = prealLine;
      pline->end = length;
      (*pstartLine) = pline;
      count++;
    }
  }

  //if (i >= CRLF_SIZE && isCRLFSym(text[i - CRLF_SIZE])) // в самом конце был символ переноса строки
  if (i > 0 && isCRLFSym(text[i - 1]))
  {
    prealLine->next = calloc(1, sizeof(real_line_t));
    prealLine->next->str = calloc(1, sizeof(wchar_t));
    prealLine = prealLine->next;

    pline->next = calloc(1, sizeof(line_t));
    pline->next->prealLine = prealLine;
    pline->next->prev = pline;
    pline->next->end = 0;
    pline = pline->next;
    count++;
  }

  return count;
}

/* Функция вставляет непустой текст в позицию каретки (текст не должен быть обработан -
 * исходные строки не отличаются от отображаемых)
 * ПАРАМЕТРЫ:
 *   pcommon - общая информация об окне
 *   pcaret - информация о каретке
 *   pfirstLine - первая строка на экране
 *   pnewLine - указатель на список отображаемых строк (текст для вставки)
 *   pnewRealLine - указатель на список исходных строк (текст для вставки)
 *   count - количество новых строк
 *   prect - указатель на RECT с областью для обновления
 *   prange - указатель на range_t с измененной областью в тексте
 */
extern void Paste(HDC hdc, common_info_t *pcommon, caret_info_t *pcaret, line_info_t *pfirstLine, undo_info_t *pundo,
                  pline_t pnewLine, preal_line_t pnewRealLine, unsigned int count, RECT *prect, range_t *prange)
{
  unsigned int plastIndex, fLineNumber = pfirstLine->number, k;
  int i, back = 0, deleted = 0, added = 0, reduced;
  long int maxLength = pcommon->isWordWrap ? WRAP_MAX_LENGTH(pcommon) : MAX_LENGTH;
  pos_info_t pos;
  pline_t pline, plastLine;
  position_t startPos;  // для undo

  if (pcaret->line.number < pfirstLine->number)
    GotoCaret(pcommon, pcaret, pfirstLine);

  _ASSERTE(pcaret->line.number >= pfirstLine->number); // для удобства обработки положения первой линии
  _ASSERTE(pnewLine && pnewRealLine);
  _ASSERTE(pcaret->length == RealStringLength(pcaret->line.pline));

  if (0 == count)  // вставлять нечего
    return;

  // сохраним первую линию
  pos.piLine = pfirstLine;
  pos.index = pfirstLine->pline->begin;
  SetStartLine(&pos);

  // сохраним место, где стояла каретка
  startPos.x = pcaret->index;
  startPos.y = pcaret->line.realNumber;

  // для начала соберём строку каретки
  while (pcaret->line.pline->prev->prealLine == pcaret->line.pline->prealLine)
  {
    pcaret->line.number--;
    pcaret->line.pline = pcaret->line.pline->prev;
    back++;
  }
  deleted = UniteLine(pcaret->line.pline);

  prect->left = 0;
  prect->right = pcommon->clientX;
  prect->top = max(LINE_Y(pcommon, pcaret->line.number + back), 0);
  prect->bottom = prect->top + pcommon->di.averageCharY;
  if (back)
  {
    // Если есть back, значит были строки перед кареткой, относящиеся к той же исходной строке.
    // После вёрстки та, которая была непосредствнно перед кареткой может измениться
    prect->top = max((long)(prect->top - pcommon->di.averageCharY), 0);
  }

  pline = pnewLine;
  while (pline->next)
    pline = pline->next;
  plastLine = (count > 1) ? pline : pcaret->line.pline;
  plastIndex = (count > 1) ? pline->end : pline->end + pcaret->index;

  // присоединим к новым строкам строки после каретки (если новых строк больше одной)
  if (count > 1)
  {
    pline->next = pcaret->line.pline->next;
    if (pline->next)
      pline->next->prev = pline;
    pline->prealLine->next = pcaret->line.pline->prealLine->next;
  }

  // вставим в конец новых строк окончание строки с кареткой и удалим его
  InsertSubString(pline, pline->end, pline->end,
               &pcaret->line.pline->prealLine->str[pcaret->index], pcaret->length - pcaret->index);
  DeleteSubString(pcaret->line.pline, pcaret->length, pcaret->index, pcaret->length - pcaret->index);
  pcaret->length = pcaret->index;

  // вставим в строку с кареткой первую новую строку и удалим её
  InsertSubString(pcaret->line.pline, pcaret->length, pcaret->index, pnewLine->prealLine->str, pnewLine->end);
  pcaret->length += pnewLine->end;
  pline = pnewLine->next;
  free(pnewRealLine->str);
  free(pnewRealLine);
  free(pnewLine);

  if (count > 1)
  {
    // присоединим к строке каретки новые строки
    pline->prev = pcaret->line.pline;
    pcaret->line.pline->next = pline;
    pcaret->line.pline->prealLine->next = pline->prealLine;
  }

  pline = pcaret->line.pline;

  // разбиваем строки, каждая из этих строк цельная,
  // поэтому ReduceLine будет разбивать каждую до перехода на следующую исходную строку
  for (k = 0; k < count; k++)
  {
    pline = ReduceLine(hdc, &pcommon->di, pline, maxLength, &reduced, &i);
    added += i + 1;
  }
  // added на 1 больше, чем реально добавлено

  if (!pcommon->isWordWrap)
  {
    unsigned long sizeX;

    pline = pcaret->line.pline;
    while (pline != plastLine->next)
    {
      sizeX = GetStringSizeX(hdc, &pcommon->di, pline->prealLine->str, pline->begin, pline->end, NULL);
      if (sizeX > pcommon->maxStringSizeX)
        pcommon->maxStringSizeX = sizeX;
      pline = pline->next;
    }
  }

  UpdateLine(&pos); // восстановим первую линию

  // 1) количество новых строк не равно тому, что было и 2) сместился экран
  if (added != deleted + 1 || fLineNumber != pfirstLine->number)
    prect->bottom = pcommon->clientY;
  else
    prect->bottom += (added - 1) * pcommon->di.averageCharY; // added - 1 строка могла измениться
  prect->bottom = min(pcommon->clientY, prect->bottom);
  pcommon->nLines += added - 1 - deleted;
  pcommon->nRealLines += count - 1;

  //  поставим каретку в конец вставленной части
  while (pcaret->line.pline != plastLine)
  {
    pcaret->line.pline = pcaret->line.pline->next;
    pcaret->line.number++;
    if (pcaret->line.pline->prealLine != pcaret->line.pline->prev->prealLine)
      pcaret->line.realNumber++;
  }
  while (pcaret->line.pline->prealLine == plastLine->prealLine && pcaret->line.pline->end < plastIndex)
  {
    pcaret->line.pline = pcaret->line.pline->next;
    pcaret->line.number++;
  }
  pcaret->index = plastIndex;
  UpdateCaretSizes(hdc, pcommon, pcaret);
  UpdateCaretPos(pcommon, pcaret, pfirstLine);

  PutArea(pundo, startPos.y, startPos.x, pcaret->line.realNumber, pcaret->index);
  NextCell(pundo, ACTION_TEXT_PASTE, pcommon->isModified);

  prange->start.x = startPos.x;
  prange->start.y = startPos.y;
  prange->end.x = pcaret->index;
  prange->end.y = pcaret->line.realNumber;

  //DocModified(pcommon, SEDIT_ACTION_PASTE, &range);

#ifdef _DEBUG
  {
    int error;
    _ASSERTE((error = SendMessage(pcommon->hwnd, SE_DEBUG, 0, 0)) == SEDIT_NO_ERROR);
  }
#endif

  // Дальнейшие действия:
  //SEInvalidateAndUpdate(pcommon, pcaret, pfirstLine, prect);
  //UpdateScrollBars(pcommon, pcaret, pfirstLine);
  //GotoCaret(pcommon, pcaret, pfirstLine);
  //CaretModified(pcommon, pcaret, pfirstLine, 0);
}

/* Функция вставляет текст из буфера в позицию каретки
 * ПАРАМЕТРЫ:
 *   pcommon - общая информация об окне
 *   pcaret - информация о каретке
 *   pfirstLine - первая строка на экране
 *   buff - буфер
 */
extern char PasteFromBuffer(common_info_t *pcommon, caret_info_t *pcaret, pselection_t psel,
                            line_info_t *pfirstLine, undo_info_t *pundo, wchar_t *buff)
{
  pline_t pnewLine = NULL;
  preal_line_t pnewRealLine = NULL;
  unsigned int count;

  if (!buff)
    return SEDIT_NO_PARAM;

  if ((count = ParseTextInMemory(buff, &pnewLine, &pnewRealLine)) > 0)
  {
    RECT rc;
    range_t range;
    HDC hdc = GetDC(pcommon->hwnd);

    SelectObject(hdc, pcommon->hfont);
    Paste(hdc, pcommon, pcaret, pfirstLine, pundo, pnewLine, pnewRealLine, count, &rc, &range);
    ReleaseDC(pcommon->hwnd, hdc);

    if (IS_SHIFT_PRESSED)
      StartSelection(psel, &pcaret->line, pcaret->index);

    SEInvalidateRect(pcommon, &rc, FALSE);

    DocModified(pcommon, SEDIT_ACTION_PASTE, &range);

    SEUpdateWindow(pcommon, pcaret, pfirstLine);
    UpdateScrollBars(pcommon, pcaret, pfirstLine);

    GotoCaret(pcommon, pcaret, pfirstLine);
    CaretModified(pcommon, pcaret, pfirstLine, CARET_RIGHT);
    DocUpdated(pcommon, SEDIT_ACTION_PASTE, &range, 1);
  }

  return SEDIT_NO_ERROR;
}

/* Функция вставляет текст из буфера обмена в позицию каретки
 * ПАРАМЕТРЫ:
 *   pcommon - общая информация об окне
 *   pcaret - информация о каретке
 *   pfirstLine - первая строка на экране
 */
extern void PasteFromClipboard(common_info_t *pcommon, caret_info_t *pcaret, pselection_t psel,
                               line_info_t *pfirstLine, undo_info_t *pundo)
{
  HGLOBAL hclipMemory;
  pline_t pnewLine = NULL;
  preal_line_t pnewRealLine = NULL;
  unsigned int count;
  wchar_t *buff;
  RECT rc;

  if (!IsClipboardFormatAvailable(CF_UNICODETEXT))
    return;

  if (!OpenClipboard(pcommon->hwnd))
    return;

  hclipMemory = GetClipboardData(CF_UNICODETEXT);
  buff = (wchar_t*)GlobalLock(hclipMemory);
  count = ParseTextInMemory(buff, &pnewLine, &pnewRealLine);
  GlobalUnlock(hclipMemory);
  CloseClipboard();

  if (count > 0)
  {
    range_t range;
    HDC hdc = GetDC(pcommon->hwnd);

    SelectObject(hdc, pcommon->hfont);
    Paste(hdc, pcommon, pcaret, pfirstLine, pundo, pnewLine, pnewRealLine, count, &rc, &range);
    ReleaseDC(pcommon->hwnd, hdc);

    if (IS_SHIFT_PRESSED)
      StartSelection(psel, &pcaret->line, pcaret->index);

    SEInvalidateRect(pcommon, &rc, FALSE);

    DocModified(pcommon, SEDIT_ACTION_PASTE, &range);

    SEUpdateWindow(pcommon, pcaret, pfirstLine);
    UpdateScrollBars(pcommon, pcaret, pfirstLine);

    GotoCaret(pcommon, pcaret, pfirstLine);
    CaretModified(pcommon, pcaret, pfirstLine, CARET_RIGHT);
    DocUpdated(pcommon, SEDIT_ACTION_PASTE, &range, 1);
  }
}

/* Функция добавляет строку после каретки и устанавливает каретку в её начало
 * ПАРАМЕТРЫ:
 *   pcommon - общая информация об окне
 *   pcaret - информация о каретке
 *   pfirstLine - первая строка на экране
 */
extern void NewLine(common_info_t *pcommon, caret_info_t *pcaret, pselection_t psel,
                    line_info_t *pfirstLine, undo_info_t *pundo)
{
  int i = 0, added = 0, deleted = 0;
  long int maxLength = pcommon->isWordWrap ? WRAP_MAX_LENGTH(pcommon) : MAX_LENGTH;
  pline_t pline;
  RECT rc;
  HDC hdc;
  range_t range;

#ifdef _DEBUG
  {
    int error;
    _ASSERTE(psel->isSelection == 0 && psel->isOldSelection == 0);
    _ASSERTE(pcaret->line.number >= pfirstLine->number);
    _ASSERTE((error = SendMessage(pcommon->hwnd, SE_DEBUG, 0, 0)) == SEDIT_NO_ERROR);
  }
#endif

  // удалим все отображаемые строки, относящиеся к реальной строке каретки (только те, которые после неё)
  while (pcaret->line.pline->next && pcaret->line.pline->next->prealLine == pcaret->line.pline->prealLine)
  {
    deleteLine(pcaret->line.pline->next);
    i++;
  }

  insertRealLine(pcaret->line.pline->prealLine, NULL);
  insertLine(pcaret->line.pline, pcaret->line.pline->prealLine->next);

  pline = pcaret->line.pline->next;
  pcommon->nLines += 1 - i;

  // скопируем остаток текущей строки в новую строку
  InsertSubString(pline, 0, 0,
               &pcaret->line.pline->prealLine->str[pcaret->index], pcaret->length - pcaret->index);

  // удалим скопированное из строки каретки
  DeleteSubString(pcaret->line.pline, pcaret->length, pcaret->index, pcaret->length - pcaret->index);

  pcaret->line.pline->end = pcaret->index;
  pline->end = pcaret->length - pcaret->index;

  rc.top = LINE_Y(pcommon, pcaret->line.number);
  rc.bottom = pcommon->clientY;
  rc.left = 0;
  rc.right = pcommon->clientX;

  hdc = GetDC(pcommon->hwnd);
  SelectObject(hdc, pcommon->hfont);

  // если строка с кареткой получилась в результате вёрстки, то теперь надо расширить предыдущую строку
  if (pcaret->line.pline->prev->prealLine == pcaret->line.pline->prealLine)
  {
    pline_t ptmpLine = pcaret->line.pline->prev;

    EnlargeLine(hdc, &pcommon->di, ptmpLine, maxLength, &deleted);
    if (deleted >= 0)
    {
      if (deleted > 0)
      {
        _ASSERTE(deleted == 1);

        if (pfirstLine->number == pcaret->line.number)
        {
          pfirstLine->number--;
          pfirstLine->pline = ptmpLine;
        }

        pcaret->line.number--;
        pcaret->line.pline = ptmpLine;
        pcaret->index = ptmpLine->end; // установим каретку в конец предыдущей строки, флаг правильных расстояний уже был снят
      }
      rc.top = rc.top > (long)pcommon->di.averageCharY ? rc.top - pcommon->di.averageCharY : rc.top;
    }
    else
      deleted = 0;
  }
  UpdateCaretSizes(hdc, pcommon, pcaret);

  pline = ReduceLine(hdc, &pcommon->di, pline, maxLength, &i, &added);
  ReleaseDC(pcommon->hwnd, hdc);

  pcommon->nLines += added - deleted;
  pcommon->nRealLines += 1;

  PutSymbol(pundo, pcaret->line.realNumber, pcaret->index, L'\n');
  NextCell(pundo, ACTION_NEWLINE_ADD, pcommon->isModified);

  if (IS_SHIFT_PRESSED)
    StartSelection(psel, &pcaret->line, pcaret->index);

  SEInvalidateRect(pcommon, &rc, FALSE);

  range.start.x = pcaret->index;
  range.start.y = pcaret->line.realNumber;
  range.end.x = 0;
  range.end.y = pcaret->line.realNumber + 1;
  DocModified(pcommon, SEDIT_ACTION_NEWLINE, &range);

  SEUpdateWindow(pcommon, pcaret, pfirstLine);
  UpdateScrollBars(pcommon, pcaret, pfirstLine);

  MoveCaret(pcommon, pcaret, pfirstLine, 1, CARET_RIGHT);

  DocUpdated(pcommon, SEDIT_ACTION_NEWLINE, &range, 1);

#ifdef _DEBUG
  {
    int error;
    _ASSERTE((error = SendMessage(pcommon->hwnd, SE_DEBUG, 0, 0)) == SEDIT_NO_ERROR);
  }
#endif
}

/* Функция добавляет символ sym в позицию каретки
 * ПАРАМЕТРЫ:
 *   pcommon - общая информация об окне
 *   pcaret - информация о каретке
 *   pfirstLine - первая строка на экране
 *   sym - символ
 */
extern void InputChar(common_info_t *pcommon, caret_info_t *pcaret, pselection_t psel,
                      line_info_t *pfirstLine, undo_info_t *pundo, wchar_t sym)
{
  unsigned int oldFirstLineNumber = pfirstLine->number;
  int enlarged = 0, i, deleted = 0, reduced = 0, added = 0;
  long int maxLength = pcommon->isWordWrap ? WRAP_MAX_LENGTH(pcommon) : MAX_LENGTH;
  char shiftPressed = IS_SHIFT_PRESSED;
  pline_t pline;
  pos_info_t caretPos, firstLinePos;
  RECT rc;
  HDC hdc;
  range_t range;

#ifdef _DEBUG
  {
    int error;
    _ASSERTE(psel->isSelection == 0 && psel->isOldSelection == 0);
    _ASSERTE((error = SendMessage(pcommon->hwnd, SE_DEBUG, 0, 0)) == SEDIT_NO_ERROR);
  }
#endif

  InsertSubString(pcaret->line.pline, pcaret->length, pcaret->index, &sym, 1);
  pcaret->length++;

  // k - сколько строк от pline до следующей исходной строки

  rc.top = LINE_Y(pcommon, pcaret->line.number);
  rc.right = pcommon->clientX;
  rc.left = pcaret->posX;
  rc.bottom = min(rc.top + pcommon->di.averageCharY, pcommon->clientY);

  hdc = GetDC(pcommon->hwnd);
  SelectObject(hdc, pcommon->hfont);

  pline = pcaret->line.pline;

  caretPos.piLine = &pcaret->line;
  caretPos.index = pcaret->index;
  SetStartLine(&caretPos);

  firstLinePos.piLine = pfirstLine;
  firstLinePos.index = pfirstLine->pline->begin;
  SetStartLine(&firstLinePos);

  // если строка каретки появилась в результате вёрстки, то часть её нужно попробовать перенести выше
  // (например при вводе пробела)
  if (pline->prev->prealLine == pline->prealLine)
  {
    pline_t ptmpLine = pline->prev;

    pline = pline->prev;
    rc.left = 0;
    rc.top = max((long)(rc.top - pcommon->di.averageCharY), 0);

    i = 0;
    while (pline && pline->prealLine == pcaret->line.pline->prealLine && i >= 0)
    {
      pline = EnlargeLine(hdc, &pcommon->di, pline, maxLength, &i);
      if (i >= 0)
      {
        ++enlarged;  // обработана очередная строка
        deleted += i;  // i строк было удалено
      }
    }
    pline = ptmpLine->next;
    _ASSERTE(pline != NULL);
  }

  if (enlarged < 2)
    pline = ReduceLine(hdc, &pcommon->di, pline, maxLength, &reduced, &added);

  UpdateLine(&caretPos);
  UpdateLine(&firstLinePos);
  UpdateCaretSizes(hdc, pcommon, pcaret);
  UpdateCaretPos(pcommon, pcaret, pfirstLine);

  if (deleted != added || oldFirstLineNumber != pfirstLine->number)  // изменилось число строк или экран прокручен
  {
    rc.left = 0;
    rc.bottom = pcommon->clientY;
    pcommon->nLines += added - deleted;
  }
  else
  {
    int count = max(enlarged - 1, reduced);

    _ASSERTE(deleted == 0 && added == 0);
    rc.left = count > 0 ? 0 : rc.left;
    rc.bottom = min(rc.bottom + count * pcommon->di.averageCharY, pcommon->clientY);
  }

  if (!pcommon->isWordWrap)
  {
    size_t sizeX = getCaretLineParameters(hdc, pcommon, pcaret);
    if (sizeX > pcommon->maxStringSizeX)
      pcommon->maxStringSizeX = sizeX;
  }
  ReleaseDC(pcommon->hwnd, hdc);

  PutSymbol(pundo, pcaret->line.realNumber, pcaret->index, sym);
  NextCell(pundo, ACTION_SYMBOL_INPUT, pcommon->isModified);

  SEInvalidateRect(pcommon, &rc, FALSE);

  range.start.x = pcaret->index;
  range.start.y = pcaret->line.realNumber;
  range.end.x = pcaret->index + 1;
  range.end.y = pcaret->line.realNumber;
  DocModified(pcommon, SEDIT_ACTION_INPUTCHAR, &range);

  SEUpdateWindow(pcommon, pcaret, pfirstLine);
  UpdateScrollBars(pcommon, pcaret, pfirstLine);

  MoveCaret(pcommon, pcaret, pfirstLine, 1, CARET_RIGHT);

  if (shiftPressed)
    StartSelection(psel, &pcaret->line, pcaret->index);

  DocUpdated(pcommon, SEDIT_ACTION_INPUTCHAR, &range, 1);

  GetActionForSmartInput(pcommon, pcaret, pundo);
  DoSmartInputAction(pcommon, pcaret, NULL);

#ifdef _DEBUG
  {
    int error;
    _ASSERTE((error = SendMessage(pcommon->hwnd, SE_DEBUG, 0, 0)) == SEDIT_NO_ERROR);
  }
#endif
}

/* Функция удаляет символ перед позицией каретки
 * ПАРАМЕТРЫ:
 *   pcommon - общая информация об окне
 *   pcaret - информация о каретке
 *   pfirstLine - первая строка на экране
 */
extern void DeleteChar(common_info_t *pcommon, caret_info_t *pcaret, pselection_t psel,
                       line_info_t *pfirstLine, undo_info_t *pundo)
{
  HDC hdc;
  RECT rc;
  long int maxLength = pcommon->isWordWrap ? WRAP_MAX_LENGTH(pcommon) : MAX_LENGTH;
  unsigned int action = SEDIT_ACTION_NONE;
  range_t range = {0};

#ifdef _DEBUG
  {
    int error;
    _ASSERTE(psel->isSelection == 0 && psel->isOldSelection == 0);
    _ASSERTE((error = SendMessage(pcommon->hwnd, SE_DEBUG, 0, 0)) == SEDIT_NO_ERROR);
  }
#endif

  hdc = GetDC(pcommon->hwnd);
  SelectObject(hdc, pcommon->hfont);

  if (pcaret->index > 0)
  {
    int width = pcaret->dx[pcaret->index - pcaret->line.pline->begin] - pcaret->dx[pcaret->index - pcaret->line.pline->begin - 1];
    int i, num, deleted = 0;
    char isProcessed = 0, isCaretGoUp = 0;
    pos_info_t caretPos, firstLinePos;
    pline_t pline = pcaret->line.pline;
    pline_t ptmpLine = pcaret->line.pline->prev;
    wchar_t deletedSym;

    MoveCaret(pcommon, pcaret, pfirstLine, 1, CARET_LEFT_WITHOUT_MODIFICATION);
    deletedSym = pline->prealLine->str[pcaret->index];
    DeleteSubString(pcaret->line.pline, pcaret->length, pcaret->index, 1);
    pcaret->length--;

    if (ptmpLine == pcaret->line.pline)
      isCaretGoUp = 1;  // каретка сместилась на одну линию вверх, изменения же коснутся и нижней линии

    rc.top = LINE_Y(pcommon, pcaret->line.number);
    rc.left = pcaret->posX;
    rc.right = pcommon->clientX;
    rc.bottom = min(rc.top + pcommon->di.averageCharY, pcommon->clientY);

    caretPos.index = pcaret->index;
    caretPos.piLine = &pcaret->line;
    SetStartLine(&caretPos);

    num = pfirstLine->number;
    firstLinePos.index = pfirstLine->pline->begin;
    firstLinePos.piLine = pfirstLine;
    SetStartLine(&firstLinePos);

    if (ptmpLine->prealLine == pline->prealLine)
    {
      if (!IsBreakSym(ptmpLine->prealLine->str[ptmpLine->end - 1]) && !IsBreakSym(pline->prealLine->str[pline->begin]))
      {
        deleted -= ProcessLine(hdc, &pcommon->di, ptmpLine, maxLength);
        isProcessed = 1;
        rc.top = max((long)(rc.top - pcommon->di.averageCharY), 0);
      }
      else
      {
        EnlargeLine(hdc, &pcommon->di, ptmpLine, maxLength, &deleted);
        if (deleted >= 0)
        {
          pline = ptmpLine->next;
          if (!isCaretGoUp) // если карека не смещалась вверх, то прямоугольник не охватывает ptmpLine
            rc.top = max((long)(rc.top - pcommon->di.averageCharY), 0);
          else  // иначе старая линия каретки не охвачена прямоугольником
            rc.bottom = min(rc.bottom + pcommon->di.averageCharY, pcommon->clientY);
        }
        else if (deleted < 0)  // это значит, что расширить не удалось
          deleted = 0;
      }
      rc.left = 0;
    }

    // если следующая после каретки строка составляет единое целое со строкой каретки (проще всё переобработать)
    if (!isProcessed && pline && pline->next && pline->next->prealLine == pline->prealLine)
    {
      if (!IsBreakSym(pline->prealLine->str[pline->end - 1]) && !IsBreakSym(pline->next->prealLine->str[pline->next->begin]))
      {
        deleted -= ProcessLine(hdc, &pcommon->di, pline, maxLength);
        isProcessed = 1;
        rc.left = 0;
      }
    }

    if (!isProcessed)
    {
      i = 0;
      while (pline && pline->prealLine == pcaret->line.pline->prealLine && i >= 0)
      {
        pline = EnlargeLine(hdc, &pcommon->di, pline, maxLength, &i);
        if (i >= 0)
        {
          deleted += i;
          rc.left = 0;
          rc.bottom += pcommon->di.averageCharY;
        }
      }
      rc.bottom = min(rc.bottom, (long int)pcommon->clientY);
    }

    UpdateLine(&firstLinePos);
    UpdateLine(&caretPos);

    if (deleted != 0)  // изменилось количество строк
    {
      pcommon->nLines -= deleted;
      rc.bottom = pcommon->clientY;
    }
    else if (isProcessed && num == pfirstLine->number)
    {
      pline = pcaret->line.pline;

      i = pcaret->line.number; // посчитаем сколько строк могло измениться
      while (pline && pline->prealLine == pcaret->line.pline->prealLine)
      {
        pline = pline->next;
        i++;
      }
      rc.bottom = min(pcommon->clientY, rc.bottom + i * pcommon->di.averageCharY);
    }
    else if (num != pfirstLine->number)
      rc.bottom = pcommon->clientY;

  //  if (!pcommon->isWordWrap)  // без переноса строк не надо обновлять окно левее каретки
  //  {
  //    if ()
  //    rc.left = pcaret->sizeX - (pcommon->hscrollPos) * HSCROLL_INCREMENT;
  //  }

    UpdateCaretSizes(hdc, pcommon, pcaret);
    UpdateCaretPos(pcommon, pcaret, pfirstLine);

    PutSymbol(pundo, pcaret->line.realNumber, pcaret->index, deletedSym); // сохраним символ в стеке undo
    NextCell(pundo, ACTION_SYMBOL_DELETE, pcommon->isModified);

    if (IS_SHIFT_PRESSED)
      StartSelection(psel, &pcaret->line, pcaret->index);
    SEInvalidateRect(pcommon, &rc, FALSE);

    action = SEDIT_ACTION_DELETECHAR;
    range.start.x = pcaret->index;
    range.start.y = pcaret->line.realNumber;
    range.end.x = pcaret->index + 1;
    range.end.y = pcaret->line.realNumber;
    DocModified(pcommon, action, &range);
  }
  else if (pcaret->line.number > 0)
  // каретка в самом начале исходной строки и это не самое начало файла
  {
    pline_t pline, pstartLine, pendLine;
    size_t length = 0;
    int i = 2, deleted = 0, count;

    MoveCaret(pcommon, pcaret, pfirstLine, 1, CARET_LEFT_WITHOUT_MODIFICATION);
    pline = pcaret->line.pline;
    pstartLine = pline->next;
    _ASSERTE(pline->prealLine != pstartLine->prealLine);

    pendLine = pstartLine;
    while (pendLine->next && pendLine->next->prealLine == pendLine->prealLine)
    {
      pendLine = pendLine->next;
      i++;      // i - количество отображаемых линий с линии каретки до последней линии сливаемой исходной строки
    }

    rc.top = LINE_Y(pcommon, pcaret->line.number);
    rc.left = 0;
    rc.right = pcommon->clientX;
    rc.bottom = min(rc.top + i * pcommon->di.averageCharY, pcommon->clientY);

    length = pendLine->end;
    pline->prealLine->str = realloc(pline->prealLine->str, sizeof(wchar_t) * (pcaret->length + length + 1));
    wcsncpy(&pline->prealLine->str[pline->end], pstartLine->prealLine->str, length + 1);
    pline->prealLine->next = pstartLine->prealLine->next;
    pline->prealLine->surplus = 0;

    free(pstartLine->prealLine->str);
    free(pstartLine->prealLine);

    while (pstartLine != pendLine->next)
    {
      pstartLine->prealLine = pline->prealLine;
      pstartLine->begin += pline->end;
      pstartLine->end += pline->end;
      pstartLine = pstartLine->next;
    }

    while (pline && pline->prealLine == pcaret->line.pline->prealLine)
    {
      pline = EnlargeLine(hdc, &pcommon->di, pline, maxLength, &count);
      if (count > 0)
        deleted += count;
    }

    if (deleted != 0)
    {
      pcommon->nLines -= deleted;
      rc.bottom = pcommon->clientY;
    }
    pcommon->nRealLines -= 1;

    if (!pcommon->isWordWrap)
    {
      size_t sizeX = GetStringSizeX(hdc, &pcommon->di, pcaret->line.pline->prealLine->str, pcaret->line.pline->begin,
                                    pcaret->line.pline->end, NULL);
      if (pcommon->maxStringSizeX < sizeX)
        pcommon->maxStringSizeX = sizeX;
    }

    UpdateCaretSizes(hdc, pcommon, pcaret);
    UpdateCaretPos(pcommon, pcaret, pfirstLine);

    PutSymbol(pundo, pcaret->line.realNumber, pcaret->index, L'\n');
    NextCell(pundo, ACTION_NEWLINE_DELETE, pcommon->isModified);

    if (IS_SHIFT_PRESSED)
      StartSelection(psel, &pcaret->line, pcaret->index);
    SEInvalidateRect(pcommon, &rc, FALSE);

    action = SEDIT_ACTION_DELETELINE;
    range.start.x = pcaret->index;
    range.start.y = pcaret->line.realNumber;
    range.end.x = 0;
    range.end.y = pcaret->line.realNumber + 1;
    DocModified(pcommon, action, &range);
  }
  ReleaseDC(pcommon->hwnd, hdc);

  SEUpdateWindow(pcommon, pcaret, pfirstLine);
  UpdateScrollBars(pcommon, pcaret, pfirstLine);

  CaretModified(pcommon, pcaret, pfirstLine, CARET_LEFT);

  DocUpdated(pcommon, action, &range, (range.start.x != range.end.x || range.start.y != range.end.y) ? 1 : pcommon->isModified);

#ifdef _DEBUG
  {
    int error;
    _ASSERTE((error = SendMessage(pcommon->hwnd, SE_DEBUG, 0, 0)) == SEDIT_NO_ERROR);
  }
#endif
}

extern void DeleteNextChar(common_info_t *pcommon, caret_info_t *pcaret, pselection_t psel,
                           line_info_t *pfirstLine, undo_info_t *pundo)
{
  if (pcaret->line.number + 1 != pcommon->nLines || pcaret->index < pcaret->line.pline->end)
  {
    // если каретка находится в конце свёрстанной строки и следующая строка относится к той же исходной, то
    // надо передвинуть каретку
    MoveCaret(pcommon, pcaret, pfirstLine, 1, CARET_RIGHT);
    DeleteChar(pcommon, pcaret, psel, pfirstLine, pundo);
  }
}

/* Функция удаляет область
 * ПАРАМЕТРЫ:
 *   pcommon - указатель на структуру с общей информацией об окне
 *   pfirstLine - первая строка на экране
 *   piStartLine - строка, содержащая верхнюю границу области
 *   si - индекс границы области в строке piStartLine
 *   piEndLine - строка, содержащая нижнюю границу области
 *   ei -индекс границы области в строке piEndLine
 *   rect - указатель на структуру, которая будет содержать прямоугольник для перерисовки
 */
extern void DeleteArea(HDC hdc, common_info_t *pcommon, line_info_t *pfirstLine,
                       line_info_t *piStartLine, unsigned int si, line_info_t *piEndLine, unsigned int ei, RECT *rect)
{
  int deleted = 0, realDeleted = 0;
  unsigned int num = pfirstLine->number;
  long int maxLength = pcommon->isWordWrap ? WRAP_MAX_LENGTH(pcommon) : MAX_LENGTH;
  char isProcessed = 0;  // признак того, что строка полностью обработана
  pline_t pline = piStartLine->pline;
  pos_info_t firstLinePos;
  RECT rc;

  _ASSERTE(pfirstLine->number <= piStartLine->number);

  deleteStrings(piStartLine->pline, si, piEndLine->pline, ei, &deleted, &realDeleted);
  rc.left = 0;
  rc.right = pcommon->clientX;

  firstLinePos.index = pfirstLine->pline->begin;
  firstLinePos.piLine = pfirstLine;
  SetStartLine(&firstLinePos);

  if (pline->prev->prealLine == pline->prealLine)
  {
    pline = pline->prev;
    piStartLine->pline = pline;
    piStartLine->number--;
    deleted -= ProcessLine(hdc, &pcommon->di, pline, maxLength);
  }
  else
    deleted -= ProcessLine(hdc, &pcommon->di, pline, maxLength);

  rc.top = max(LINE_Y(pcommon, piStartLine->number), 0);
  UpdateLine(&firstLinePos);

  if (!pcommon->isWordWrap)
  {
    pline_t pline = piStartLine->pline;
    unsigned long size = GetStringSizeX(hdc, &pcommon->di, pline->prealLine->str, pline->begin, pline->end, NULL);

    if (pcommon->maxStringSizeX < size)
      pcommon->maxStringSizeX = size;
  }

  pcommon->nLines -= deleted;
  pcommon->nRealLines -= realDeleted;
  if (pfirstLine->number != num || deleted != 0)
    rc.bottom = pcommon->clientY;
  else
  {
    unsigned int count = 1;
    pline = pline->next;

    while (pline && pline->prealLine == pline->prev->prealLine)
    {
      pline = pline->next;
      count++;
    }
    rc.bottom = min(LINE_Y(pcommon, piStartLine->number + count), pcommon->clientY);
  }

  rect->top = rc.top;
  rect->bottom = rc.bottom;
  rect->left = rc.left;
  rect->right = rc.right;

//  после выхода надо обновить скроллбары и окно:
//  SEInvalidateAndUpdate();
//  UpdateScrollBars();
}

/* Функция закрывает текущий документ
 * ПАРАМЕТРЫ:
 *   pcommon - общая информация об окне
 *   pcaret - информация о каретке
 *   pfirstLine - первая строка на экране
 *   prealHead - голова списка исходных строк в документе
 *   pfakeHead - голова списка отображаемых строк в документе
 */
extern void CloseDocument(common_info_t *pcommon,
                          caret_info_t *pcaret,
                          pselection_t pselection,
                          line_info_t *pfirstLine,
                          undo_info_t *pundo,
                          text_t *ptext)
{
  HWND hwndParent = GetParent(pcommon->hwnd);

  SetEmptyDocument(ptext, &pcommon->nLines, &pcommon->nRealLines);

  if (hwndParent)
  {
    sen_doc_closed_t sen;
    sen.header.idFrom = pcommon->id;
    sen.header.hwndFrom = pcommon->hwnd;
    sen.header.code = SEN_DOCCLOSED;

    SendMessage(hwndParent, WM_NOTIFY, (WPARAM)pcommon->id, (LPARAM)&sen);
  }

  resetDocument(pcommon, pcaret, pselection, pfirstLine, pundo, ptext->pfakeHead);
}

// функция вызывается при любом изменении текста, до того, как экран будет обновлён
extern void DocModified(common_info_t *pcommon, unsigned int action, const range_t *prange)
{
  HWND hwndParent = GetParent(pcommon->hwnd);

  if (hwndParent)
  {
    sen_doc_changed_t sen;
    sen.header.idFrom = pcommon->id;
    sen.header.hwndFrom = pcommon->hwnd;
    sen.header.code = SEN_DOCCHANGED;
    sen.action = action;
    sen.range = *prange;

    SendMessage(hwndParent, WM_NOTIFY, (WPARAM)pcommon->id, (LPARAM)&sen);
  }
}

// функция вызывается при любом изменении флага модификации, после того, как всё будет обновлено
extern void DocUpdated(common_info_t *pcommon, unsigned int action, const range_t *prange, char isModified)
{
  HWND hwndParent = GetParent(pcommon->hwnd);

  _ASSERTE(prange != NULL);

  pcommon->isModified = isModified;

  if (hwndParent)
  {
    sen_doc_updated_t sen;
    sen.header.idFrom = pcommon->id;
    sen.header.hwndFrom = pcommon->hwnd;
    sen.header.code = SEN_DOCUPDATED;
    sen.action = action;
    sen.range = *prange;
    sen.isDocModified = isModified;

    SendMessage(hwndParent, WM_NOTIFY, (WPARAM)pcommon->id, (LPARAM)&sen);
  }
}

extern void ShowContextMenu(window_t *pw, int x, int y)
{
  HMENU hMenu, hSubMenuPopup;
  POINT pt = {x, y};

  if (!pw->common.di.CreatePopupMenuFunc || !pw->common.di.DestroyPopupMenuFunc)
    return;

  if (pt.x < 0 || pt.x > (long)pw->common.clientX || pt.y < 0 || pt.y > (long)pw->common.clientY)
    return;

  if ((hMenu = pw->common.di.CreatePopupMenuFunc(pw->common.hwnd, pw->common.id)) == NULL)
    return;

  hSubMenuPopup = GetSubMenu(hMenu, 0);

  if (!hSubMenuPopup)
  {
    pw->common.di.DestroyPopupMenuFunc(pw->common.hwnd, pw->common.id, hMenu);
    return;
  }

  ClientToScreen(pw->common.hwnd, (LPPOINT) &pt);

#if !defined(_WIN32_WCE)
  TrackPopupMenu(hSubMenuPopup, TPM_LEFTALIGN | TPM_LEFTBUTTON, pt.x, pt.y, 0, pw->common.hwnd, NULL);
#else
  TrackPopupMenu(hSubMenuPopup, TPM_LEFTALIGN, pt.x, pt.y, 0, pw->common.hwnd, NULL);
#endif

  pw->common.di.DestroyPopupMenuFunc(pw->common.hwnd, pw->common.id, hMenu);
}

extern char UpdateBitmaps(HDC hdc, display_info_t *pdi, long int width, long int height)
{
  if (pdi->bmHeight < height || pdi->bmWidth < width)
  {
    HBITMAP hlineBmp = 0;

    width = max(pdi->bmWidth, width);
    height = max(pdi->bmHeight, height);

    if ((hlineBmp = CreateCompatibleBitmap(hdc, width, height)) == 0)
      return 0;
    SelectObject(pdi->hlineDC, (HGDIOBJ)hlineBmp);

    if (pdi->hlineBitmap)
      DeleteObject(pdi->hlineBitmap);

    pdi->hlineBitmap = hlineBmp;
    pdi->bmWidth = width;
    pdi->bmHeight = height;
  }
  return 1;
}


// функция перерисовывает линию piLine, если цвет каретки не совпадает с цветом текста
extern void RedrawCaretLine(common_info_t *pcommon, caret_info_t *pcaret, line_info_t *pfirstLine, line_info_t *piLine)
{
  RECT rc;
  unsigned int start = piLine->number, end = piLine->number;
  pline_t pline;

  if (pcommon->di.caretTextColor == pcommon->di.textColor && pcommon->di.caretBgColor == pcommon->di.bgColor)
    return;

  pline = piLine->pline;
  while (pline->prev->prealLine == pline->prealLine)
  {
    pline = pline->prev;
    start--;
  }
  if (start > pcommon->nscreenLines + pfirstLine->number)
    return;

  pline = piLine->pline;
  while (pline->next && pline->next->prealLine == pline->prealLine)
  {
    pline = pline->next;
    end++;
  }
  if (end < pfirstLine->number)
    return;

  rc.left = 0;
  rc.right = pcommon->clientX;
  rc.top = (start > pfirstLine->number) ? LINE_Y(pcommon, start) : 0;
  rc.bottom = min(LINE_Y(pcommon, end + 1), pcommon->clientY);
  SEInvalidateRect(pcommon, &rc, FALSE);
  SEUpdateWindow(pcommon, pcaret, pfirstLine);
}

/* Функция выводит кусок строки, который попадает в зону рисования
 * ПАРАМЕТРЫ:
 *   hdc - контекст устройства
 *   pcommon - общая информация об окне
 *   pcaret - информация о каретке
 *   piline - информация о строке для вывода
 *   psel - информация о выделении
 *   startX - позиция по X начала вывода
 *   endX - позиция по X конца вывода
 *   y - позиция по Y для вывода

 * ВОЗВРАЩАЕТ: код ошибки
 */
extern char DrawLine(HDC hdc,
                     common_info_t *pcommon,
                     caret_info_t *pcaret,
                     line_info_t *piLine,
                     pselection_t psel,
                     long int _startX,
                     long int _endX,
                     long int y
                     /*const HBRUSH *hbrushes*/)
{
  pline_t pline = piLine->pline;
  int i, *dx;
  char inSelection;
  int nLinePoints = 0, nBkLinePoints = 0;
  COLORREF color, savedColor, originalColor;
  position_t strRange, bkRange;  // области для вывода строки и фона в индексах строки
  pix_position_t strRangeX, bkRangeX; // области для вывода строки и фона в пикселях

  // end - позиция до которой строка будет выведена
  // outBegin, outEnd - текущие позиции вывода(строка выводится по частям)
  unsigned int outBegin, outEnd;

  long int outX, startX = _startX, endX = _endX, surplusX = 0;
  RECT rc;

  rc.top = y;
  rc.bottom = y + pcommon->di.averageCharY;

  if (piLine->pline != pcaret->line.pline)  // выводится строка без каретки
  {
    if (reallocDX(&pcommon->di.dx, &pcommon->di.dxSize, pline->end - pline->begin) != SEDIT_NO_ERROR)
      return SEDIT_MEMORY_ERROR;
    getLinePlacement(hdc, &pcommon->di, &pline->prealLine->str[pline->begin], pline->end - pline->begin, pcommon->di.dx);
    dx = pcommon->di.dx;
  }
  else
    dx = pcaret->dx;

  rc.left = 0;
  rc.right = getLineDrawParams(hdc, pcommon, piLine, startX, endX, &strRange, &bkRange, &strRangeX, &bkRangeX, dx);

  // Fill left indent area with background color
  if (rc.left != rc.right)
  {
    HBRUSH hbr = CreateSolidBrush(pcommon->di.bgColor);
    if (0 == hbr)
      return SEDIT_MEMORY_ERROR;
    FillRect(hdc, &rc, hbr);
    DeleteObject(hbr);
  }

  // Prepare data for drawing string
  if (getStringSpecialPoints(pcommon, piLine, psel, &strRange, &bkRange, &nLinePoints, &nBkLinePoints) != SEDIT_NO_ERROR)
    return SEDIT_MEMORY_ERROR;

  // 1. draw background for string
  outX = bkRangeX.x;
  outBegin = bkRange.x;
  outEnd = (nBkLinePoints == 0) ? outBegin : pcommon->bkLinePoints[0].i;
  originalColor = pline->prealLine == pcaret->line.pline->prealLine ? pcommon->di.caretBgColor : pcommon->di.bgColor;
  color = originalColor;
  inSelection = 0;

  for (i = 0; i < nBkLinePoints; ++i)
  {
    if (outBegin < outEnd)
    {
      HBRUSH hbr = CreateSolidBrush(color);
      if (0 == hbr)
        return SEDIT_MEMORY_ERROR;
      rc.left = outX;
      outX += GetStringSizeX(hdc, &pcommon->di, pline->prealLine->str, outBegin, outEnd, &dx[outBegin - pline->begin]);
      rc.right = outX;
      FillRect(hdc, &rc, hbr);
      DeleteObject(hbr);
    }

    switch (pcommon->bkLinePoints[i].data[0])
    {
    case SELECTION_BEGIN:
      outBegin = pcommon->bkLinePoints[i].i;
      savedColor = color;
      color = pcommon->di.selectionColor;
      inSelection = 1;
      break;

    case SELECTION_END:
      outBegin = pcommon->bkLinePoints[i].i;
      color = savedColor;
      inSelection = 0;

      // if the string is already on the screen and selection contains strings below,
      // we should add one space in order to make selection's passage visible
      if (pcommon->bkLinePoints[i].i == pline->end &&
          (psel->pos.endLine.number > piLine->number || psel->pos.startLine.number > piLine->number) &&
          pline->next->prealLine != pline->prealLine)
      {
        surplusX = pcommon->di.specSymWidths[SPACE];
      }
      break;

    case COLOR_CHANGE:
      outBegin = pcommon->bkLinePoints[i].i;
      if (!inSelection)
        color = RGB(pcommon->bkLinePoints[i].data[1], pcommon->bkLinePoints[i].data[2], pcommon->bkLinePoints[i].data[3]);
      else
        savedColor = RGB(pcommon->bkLinePoints[i].data[1], pcommon->bkLinePoints[i].data[2], pcommon->bkLinePoints[i].data[3]);
      break;
    }
    outEnd = (i == nBkLinePoints - 1) ? bkRange.y : pcommon->bkLinePoints[i + 1].i;
  }

  // surplusX means that we should extend current background
  if (surplusX)
  {
    HBRUSH hbr = CreateSolidBrush(pcommon->di.selectionColor);
    if (0 == hbr)
      return SEDIT_MEMORY_ERROR;
    rc.left = outX;
    outX += surplusX;
    rc.right = outX;
    FillRect(hdc, &rc, hbr);
    DeleteObject(hbr);
  }

  // fill remaining space with background color
  if (outX < _endX)
  {
    HBRUSH hbr = CreateSolidBrush(originalColor);
    if (0 == hbr)
      return SEDIT_MEMORY_ERROR;
    rc.left = outX;
    rc.right = _endX/* + pcommon->di.maxOverhang*/;
    FillRect(hdc, &rc, hbr);
    DeleteObject(hbr);
  }

  // 2. draw string
  outX = strRangeX.x;
  outBegin = strRange.x;
  outEnd = (nLinePoints == 0) ? outBegin : pcommon->linePoints[0].i;
  color = pline->prealLine == pcaret->line.pline->prealLine ? pcommon->di.caretTextColor : pcommon->di.textColor;
  inSelection = 0;

  SetTextColor(hdc, color);
  SetBkMode(hdc, TRANSPARENT);

  for (i = 0; i < nLinePoints; i++)
  {
    if (outEnd - outBegin > 0)
    {
      ExtTextOut(hdc, outX, y, 0, NULL, &pline->prealLine->str[outBegin], outEnd - outBegin, NULL);
      outX += GetStringSizeX(hdc, &pcommon->di, pline->prealLine->str, outBegin, outEnd, &dx[outBegin - pline->begin]);
    }

    switch (pcommon->linePoints[i].data[0])
    {
    case FORMAT_SYM:
      outBegin = pcommon->linePoints[i].i + 1;
      drawFormatSymbol(hdc, pline, dx, pcommon->linePoints[i].i, &outX, y);
      break;

    case SELECTION_BEGIN:
      outBegin = pcommon->linePoints[i].i;
      savedColor = color;
      SetTextColor(hdc, pcommon->di.selectedTextColor);
      inSelection = 1;
      break;

    case SELECTION_END:
      outBegin = pcommon->linePoints[i].i;
      color = savedColor;
      SetTextColor(hdc, color);
      inSelection = 0;
      break;

    case COLOR_CHANGE:
      outBegin = pcommon->linePoints[i].i;
      if (!inSelection)
      {
        color = RGB(pcommon->linePoints[i].data[1], pcommon->linePoints[i].data[2], pcommon->linePoints[i].data[3]);
        SetTextColor(hdc, color);
      }
      else
        savedColor = RGB(pcommon->linePoints[i].data[1], pcommon->linePoints[i].data[2], pcommon->linePoints[i].data[3]);
      break;
    }
    outEnd = (i == nLinePoints - 1) ? strRange.y : pcommon->linePoints[i + 1].i;
  }

  if (outBegin < strRange.y)
  {
    ExtTextOut(hdc, outX, y, 0, NULL, &pline->prealLine->str[outBegin], strRange.y - outBegin, NULL);
    //outX += GetStringSizeX(hdc, &pcommon->di, pline->prealLine->str, outBegin, end, &dx[outBegin - pline->begin]);
  }

  return 0;
}

/* Функция обрабатывает текст при изменении размеров окна
 * ПАРАМЕТРЫ:
 *   pcommon - общая информация об окне
 *   pcaret - информация о каретке
 *   pselection - информация о выделении
 *   pfirstLine - первая строка на экране
 *   pfakeHead - голова списка отображаемых строк в документе
 *   clientX - ширина рабочей области окна
 *   clientY - высота рабочей области окна
 */
extern void ResizeDocument(common_info_t *pcommon,
                           caret_info_t *pcaret,
                           pselection_t pselection,
                           line_info_t *pfirstLine,
                           pline_t pfakeHead,
                           long clientX,
                           long clientY)
{
  long oldClientX = pcommon->clientX;
  long oldClientY = pcommon->clientY;
  char needValidate = 0;
  HDC hdc = GetDC(pcommon->hwnd);

  SelectObject(hdc, pcommon->hfont);

  pcommon->clientX = clientX;
  pcommon->clientY = clientY;
  pcommon->nscreenLines = pcommon->clientY / pcommon->di.averageCharY;

  if (pcommon->forValidateRect == SEDIT_REPAINTED)
  {
    needValidate = 1;
    pcommon->forValidateRect = SEDIT_NOT_REPAINTED;
  }

  if (pcommon->isWordWrap)
  {
    pos_info_t caretPos, flinePos;  // для сохранения позиций верхней линии, каретки
    pos_info_t selStartPos, selEndPos; // для сохранения выделенной области
    unsigned int numbers[4];
    pline_t plines[4];
    int size;
    char shiftPressed = IS_SHIFT_PRESSED;

    caretPos.index = pcaret->index;
    caretPos.piLine = &pcaret->line;
    flinePos.index = pfirstLine->pline->begin;
    flinePos.piLine = pfirstLine;

    // при расширении строк часть из них будет удалена, поэтому для верхней линии и каретки перейдём в строку
    // с которой начинается соответствующая реальная строка
    // при уменьшении строк надо запомнить позицию, в которой стояла каретка (в начале или в конце строки)

    SetStartLine(&flinePos);      // установим верхнюю линию в самое начало реальной строки
    SetStartLine(&caretPos);   // установим каретку в самое начало реальной строки

    if (pselection->isSelection || shiftPressed)
    {
      selStartPos.index = pselection->pos.startIndex;
      selStartPos.piLine = &pselection->pos.startLine;
      selEndPos.index = pselection->pos.endIndex;
      selEndPos.piLine = &pselection->pos.endLine;
      SetStartLine(&selStartPos);
      SetStartLine(&selEndPos);
    }

    if (pcommon->clientX < oldClientX)
      pcommon->nLines = PrepareLines(hdc, &pcommon->di, pfakeHead, REDUCE, pcommon->clientX - pcommon->di.leftIndent);
    else if (pcommon->clientX > oldClientX)
      pcommon->nLines = PrepareLines(hdc, &pcommon->di, pfakeHead, ENLARGE, pcommon->clientX - pcommon->di.leftIndent);

    UpdateLine(&flinePos);  // найдём по сохранённому индексу новую правильную верхнюю строку
    UpdateLine(&caretPos);

    // изменились все строки, а SetStartLine/UpdateLine учли изменения только в соответствующей реальной строке
    plines[0] = pfirstLine->pline;
    plines[1] = pcaret->line.pline;
    size = 2;

    if (pselection->isSelection || shiftPressed)
    {
      UpdateLine(&selStartPos);
      UpdateLine(&selEndPos);
      plines[2] = pselection->pos.startLine.pline;
      plines[3] = pselection->pos.endLine.pline;
      size += 2;
    }

    GetNumbersFromLines(pfakeHead->next, 0, size, plines, numbers);

    pfirstLine->number = numbers[0];
    pcaret->line.number = numbers[1];
    if (pselection->isSelection || shiftPressed)
    {
      pselection->pos.startLine.number = numbers[2];
      pselection->pos.endLine.number = numbers[3];
    }
    if (pselection->isOldSelection && pselection->isSelection)
      memcpy(&pselection->oldPos, &pselection->pos, sizeof(selection_pos_t));
    else if (pselection->isOldSelection)
      ClearSavedSel(pselection);   // никогда не должно выполняться!
  }

  UpdateCaretSizes(hdc, pcommon, pcaret);
  UpdateCaretPos(pcommon, pcaret, pfirstLine); // вычисление новой X/Y позиции каретки

  UpdateBitmaps(hdc, &pcommon->di, pcommon->clientX, pcommon->di.averageCharY + 2);

  // если либо выключен вордврап, либо размер окна по оси X не менялся,
  // то можно помечать часть окна как неизменившуюся
  if (needValidate && (!pcommon->isWordWrap || clientX == oldClientX))
  {
    RECT rc;

    rc.left = 0;
    rc.right = min(clientX, oldClientX);
    rc.top = 0;
    rc.bottom = min(clientY, oldClientY);

    ValidateRect(pcommon->hwnd, &rc);
  }
  ReleaseDC(pcommon->hwnd, hdc);

  if (updateVScrollPos(pcommon, pfirstLine))
  {
    SEInvalidateRect(pcommon, NULL, FALSE);
    SEUpdateWindow(pcommon, pcaret, pfirstLine);
  }
  UpdateScrollBars(pcommon, pcaret, pfirstLine);

  CaretModified(pcommon, pcaret, pfirstLine, 0);
}

extern void ChangeWordWrap(common_info_t *pcommon,
                           caret_info_t *pcaret,
                           pselection_t pselection,
                           text_t *plists,
                           line_info_t *pfirstLine)
{
  HDC hdc = GetDC(pcommon->hwnd);
  SelectObject(hdc, pcommon->hfont);

  pcommon->isWordWrap = !pcommon->isWordWrap;
  RewrapText(hdc, pcommon, pcaret, pselection, pfirstLine, plists->pfakeHead);

  ReleaseDC(pcommon->hwnd, hdc);

/*  {
    LPARAM lParam;

    pcommon->isWordWrap = 1;
    pcommon->hscrollPos = 0;
    pcommon->hscrollRange = 0;
    LW(lParam) = (WORD)(pcommon->clientX++);  // увеличим clientX, чтобы запустить PrepareLines с REDUCE
    HW(lParam) = (WORD)pcommon->clientY;
    SendMessage(pcommon->hwnd, WM_SIZE, 0, lParam);
  }*/
  SEInvalidateRect(pcommon, NULL, FALSE);
  SEUpdateWindow(pcommon, pcaret, pfirstLine);
  UpdateScrollBars(pcommon, pcaret, pfirstLine);
}

// функция вычисляет размер буфера для хранения текста
extern size_t GetTextSize(pline_t psLine, unsigned int si, pline_t peLine, unsigned int ei)
{
  pline_t pline;
  size_t size = 0;

  // получим размер, необходимый для хранения текста
  if (psLine != peLine)
  {
    size += psLine->end - si;
    pline = psLine->next;
    _ASSERTE(pline);

    while (pline != peLine)
    {
      if (pline->prealLine != pline->prev->prealLine)
        size += CRLF_SIZE;
      size += pline->end - pline->begin;
      pline = pline->next;
      _ASSERTE(pline);
    }

    if (pline->prealLine != pline->prev->prealLine)
    {
      size += CRLF_SIZE;
      //if (pline->begin == pline->end) // строка пустая, надо добавить ещё один переход строки
        //size += CRLF_SIZE;
    }
    size += ei - peLine->begin;
  }
  else
    size += ei - si;

  return size;
}

// функция копирует текст в буфер
extern char CopyTextToBuffer(pline_t psLine, unsigned int si, pline_t peLine, unsigned int ei, wchar_t *buff)
{
  pline_t pline;

  _ASSERTE(psLine && peLine);

  // скопируем текст в буфер
  if (psLine != peLine)
  {
    unsigned int i = 0;

    wcsncpy(&buff[i], &psLine->prealLine->str[si], psLine->end - si);
    i += psLine->end - si;
    pline = psLine->next;

    while (pline != peLine)
    {
      if (pline->prealLine != pline->prev->prealLine)
      {
        wcscpy(&buff[i], CRLF_SEQUENCE);
        i += CRLF_SIZE;
      }
      wcsncpy(&buff[i], &pline->prealLine->str[pline->begin], pline->end - pline->begin);
      i += pline->end - pline->begin;
      pline = pline->next;
    }

    if (pline->prealLine != pline->prev->prealLine)
    {
      wcscpy(&buff[i], CRLF_SEQUENCE);
      i += CRLF_SIZE;
      //if (pline->begin == pline->end) // строка пустая, надо добавить ещё один переход строки
      //{
      //  wcscpy(&buff[i], CRLF_SEQUENCE);
      //  i += CRLF_SIZE;
      //}
    }
    wcsncpy(&buff[i], &pline->prealLine->str[pline->begin], ei - pline->begin);
    i += ei - pline->begin;
    buff[i] = L'\0';
  }
  else
  {
    wcsncpy(buff, &psLine->prealLine->str[si], ei - si);
    buff[ei - si] = L'\0';
  }

  return 1;
}

// Функция объединяет все строки с pline до конца реальной строки
// Возвращает количество удалённых строк
extern int UniteLine(pline_t psLine)
{
  int i = 0;
  pline_t pline = psLine->next, ptmpLine;

  while (pline && pline->prealLine == psLine->prealLine)
  {
    ptmpLine = pline->next;
    psLine->end = pline->end;
    deleteLine(pline);
    pline = ptmpLine;
    i++;
  }

  return i;
}

// Функция восстанавливает все линии
extern unsigned int UniteLines(pline_t head)
{
  pline_t pline = head->next;
  unsigned int nLines = 1;

  while (pline->next)
  {
    if (pline->next->prealLine == pline->prealLine)
      (void)UniteLine(pline);
    else
    {
      nLines++;
      pline = pline->next;
    }
  }

  return nLines;
}

extern void RewrapText(HDC hdc,
                       common_info_t *pcommon,
                       caret_info_t *pcaret,
                       pselection_t pselection,
                       line_info_t *pfirstLine,
                       pline_t pfakeHead)
{
  long int maxLength = pcommon->isWordWrap ? WRAP_MAX_LENGTH(pcommon) : MAX_LENGTH;
  pos_info_t caretPos, flinePos;
  pos_info_t selStartPos, selEndPos; // для сохранения выделенной области
  pline_t plines[4];
  unsigned int numbers[4];
  int size;
  char shiftPressed = IS_SHIFT_PRESSED;

  caretPos.index = pcaret->index;
  caretPos.piLine = &pcaret->line;
  flinePos.index = pfirstLine->pline->begin;
  flinePos.piLine = pfirstLine;
  SetStartLine(&flinePos);      // установим верхнюю линию в самое начало реальной строки
  SetStartLine(&caretPos);   // установим каретку в самое начало реальной строки

  if (pselection->isSelection || shiftPressed) // если есть выделенная область, то её тоже надо сохрнить
  {
    selStartPos.index = pselection->pos.startIndex;
    selStartPos.piLine = &pselection->pos.startLine;
    selEndPos.index = pselection->pos.endIndex;
    selEndPos.piLine = &pselection->pos.endLine;
    SetStartLine(&selStartPos);
    SetStartLine(&selEndPos);
  }

  pcommon->nLines = UniteLines(pfakeHead);
  pcommon->nLines = PrepareLines(hdc, &pcommon->di, pfakeHead, REDUCE, maxLength);

  if (!pcommon->isWordWrap)
    pcommon->maxStringSizeX = GetMaxStringSizeX(hdc, &pcommon->di, pfakeHead->next);
  else
    pcommon->maxStringSizeX = 0;

  UpdateLine(&flinePos);  // найдём по сохранённому индексу новую правильную верхнюю строку
  UpdateLine(&caretPos);

  // изменились все строки, а SetStartLine/UpdateLine учли изменения только в соответствующей реальной строке
  plines[0] = pfirstLine->pline;
  plines[1] = pcaret->line.pline;
  size = 2;

  if (pselection->isSelection || shiftPressed)
  {
    UpdateLine(&selStartPos);
    UpdateLine(&selEndPos);
    plines[2] = pselection->pos.startLine.pline;
    plines[3] = pselection->pos.endLine.pline;
    size += 2;
  }

  GetNumbersFromLines(pfakeHead->next, 0, size, plines, numbers);

  pfirstLine->number = numbers[0];
  pcaret->line.number = numbers[1];
  if (pselection->isSelection || shiftPressed)
  {
    pselection->pos.startLine.number = numbers[2];
    pselection->pos.endLine.number = numbers[3];
  }
  if (pselection->isOldSelection && pselection->isSelection)
    memcpy(&pselection->oldPos, &pselection->pos, sizeof(selection_pos_t));
  else if (pselection->isOldSelection)
    ClearSavedSel(pselection);   // никогда не должно выполняться!

  UpdateCaretSizes(hdc, pcommon, pcaret);
  UpdateCaretPos(pcommon, pcaret, pfirstLine);

  // InvalidateRect(hwnd, NULL, FALSE);
  // UpdateWindow(hwnd);
  // UpdateScrollBars(pcommon, pcaret, pfirstLine);
}


extern char SetOptions(common_info_t *pcommon, options_t *popt)
{
  char needUpdate = UPDATE_NOTHING;

  if (popt->type & SEDIT_SETSELCOLOR)
  {
    if (pcommon->di.selectionColor != popt->selectionColor)
    {
      pcommon->di.selectionColor = popt->selectionColor;
      needUpdate |= UPDATE_REPAINT;
    }
  }

  if (popt->type & SEDIT_SETSELTEXTCOLOR)
  {
    if (pcommon->di.selectedTextColor != popt->selectedTextColor)
    {
      pcommon->di.selectedTextColor = popt->selectedTextColor;
      needUpdate |= UPDATE_REPAINT;
    }
  }

  if (popt->type & SEDIT_SETSELTEXTTRANSPARENCY)
  {
    if (pcommon->di.selectionTransparency != popt->selectionTransparency)
    {
      pcommon->di.selectionTransparency = popt->selectionTransparency;
      needUpdate |= UPDATE_REPAINT;
    }
  }

  if (popt->type & SEDIT_SETBGCOLOR)
  {
    if (pcommon->di.bgColor != popt->bgColor)
    {
      pcommon->di.bgColor = popt->bgColor;
      needUpdate |= UPDATE_REPAINT;
    }
  }

  if (popt->type & SEDIT_SETTEXTCOLOR)
  {
    if (pcommon->di.textColor != popt->textColor)
    {
      pcommon->di.textColor = popt->textColor;
      needUpdate |= UPDATE_REPAINT;
    }
  }

  if (popt->type & SEDIT_SETCARETTEXTCOLOR)
  {
    if (pcommon->di.caretTextColor != popt->caretTextColor)
    {
      pcommon->di.caretTextColor = popt->caretTextColor;
      needUpdate |= UPDATE_REPAINT;
    }
  }

  if (popt->type & SEDIT_SETCARETBGCOLOR)
  {
    if (pcommon->di.caretBgColor != popt->caretBgColor)
    {
      pcommon->di.caretBgColor = popt->caretBgColor;
      needUpdate |= UPDATE_REPAINT;
    }
  }

  if (popt->type & SEDIT_SETTAB)
  {
    popt->tabSize = max(MIN_TAB_SIZE, popt->tabSize);
    popt->tabSize = min(MAX_TAB_SIZE, popt->tabSize);
    if (popt->tabSize != pcommon->di.tabSize)
    {
      pcommon->di.tabSize = popt->tabSize;
      needUpdate |= (UPDATE_TEXT | UPDATE_REPAINT);
    }
  }

  if (popt->type & SEDIT_SETLEFTINDENT)
  {
    //popt->leftIndent = min(popt->leftIndent, pcommon->clientX / 2);
    popt->leftIndent = max(popt->leftIndent, 0);
    if (pcommon->di.leftIndent != popt->leftIndent)
    {
      pcommon->di.leftIndent = popt->leftIndent;
      needUpdate |= (UPDATE_TEXT | UPDATE_REPAINT);
    }
  }

  if (popt->type & SEDIT_SETFONT)
  {
    if (popt->hfont && popt->hfont != pcommon->hfont)
    {
      pcommon->hfont = popt->hfont;
      needUpdate |= (UPDATE_TEXT | UPDATE_CARET | UPDATE_REPAINT);
    }
  }

  if (popt->type & SEDIT_SETCARETWIDTH)
  {
    if (pcommon->di.caretWidth != popt->caretWidth)
    {
      pcommon->di.caretWidth = popt->caretWidth;
      needUpdate |= UPDATE_CARET;
    }
  }

  if (popt->type & SEDIT_SETWRAPTYPE)
  {
    if (pcommon->di.wordWrapType != popt->wrapType)
    {
      pcommon->di.wordWrapType = popt->wrapType;
      needUpdate |= (UPDATE_TEXT | UPDATE_REPAINT);
    }
  }

  if (popt->type & SEDIT_SETHIDESPACES)
  {
    if (pcommon->di.hideSpaceSyms != popt->hideSpaceSyms)
    {
      pcommon->di.wordWrapType = popt->hideSpaceSyms;
      needUpdate |= (UPDATE_TEXT | UPDATE_REPAINT);
    }
  }

  if (popt->type & SEDIT_SETSCROLLBARS)
  {
    // PERMANENT_SCROLLBARS = 1, поэтому можно просто сравнить значения
    if ((pcommon->SBFlags & PERMANENT_SCROLLBARS) != popt->permanentScrollBars)
    {
      pcommon->SBFlags ^= PERMANENT_SCROLLBARS;
      pcommon->SBFlags |= RESET_SCROLLBARS;
      needUpdate |= UPDATE_REPAINT;
    }
  }

  if (popt->type & SEDIT_SETUSEREGEXP)
    pcommon->useRegexp = popt->useRegexp ? 1 : 0;

  if (popt->type & SEDIT_SETKINETICSCROLL)
    pcommon->di.isKineticScrollTurnedOn = popt->kineticScroll ? 1 : 0;

  SendMessage(pcommon->hwnd, MM_UPDATE_PARAMS, needUpdate, 0);
  return 1;
}


// Функция инициализирует основные переменные редактора для отображения текста и устанавливает пустой документ
extern char InitEditor(HWND hwnd, common_info_t *pcommon, caret_info_t *pcaret, line_info_t *pfirstLine, undo_info_t *pundo,
                       pselection_t pselection, text_t *ptext, pos_info_t *psearchPos)
{
  char res;
  HDC hdc;

  ZeroMemory(pcommon, sizeof(common_info_t));
  ZeroMemory(pcaret, sizeof(caret_info_t));
  ZeroMemory(pselection, sizeof(selection_t));
  ptext->pfakeHead = NULL;
  ptext->prealHead = NULL;

  if (!InitUndo(pundo, UNDO_HISTORY_SIZE) || !InitRealLineList(&ptext->prealHead) || !InitLineList(&ptext->pfakeHead))
  {
    DestroyUndo(pundo);
    if (ptext->prealHead)
      free(ptext->prealHead);
    if (ptext->pfakeHead)
      free(ptext->pfakeHead);
    ptext->pfakeHead = NULL;
    ptext->prealHead = NULL;
    return SEDIT_INITIALIZE_ERROR;
  }

  pcommon->hwnd = hwnd;
  pcommon->hfont = (HFONT)GetStockObject(SYSTEM_FONT);
  pcommon->SBFlags = UPDATE_SCROLLBARS;
  pcommon->di.leftIndent = DEFAULT_LEFTINDENT;
  pcommon->isWordWrap = DEFAULT_WORDWRAP;
  pcommon->action = DEFAULT_SMARTINPUT;
  pcommon->forValidateRect = SEDIT_RESTRICT_VALIDATE;
  pcommon->di.hideSpaceSyms = 1;
  pcommon->di.wordWrapType = DEFAULT_WORDWRAP_TYPE;
  pcommon->di.tabSize = TAB_SIZE;
  pcommon->di.caretWidth = CARET_WIDTH;
  pcommon->di.selectionColor = SELECTION_COLOR;
  pcommon->di.selectedTextColor = SELECTED_TEXT_COLOR;
  pcommon->di.caretTextColor = TEXT_COLOR;
  pcommon->di.textColor = TEXT_COLOR;
  pcommon->di.caretBgColor = BACKGROUND_COLOR;
  pcommon->di.bgColor = BACKGROUND_COLOR;
  //pcommon->di.isInvalidatedRectEmpty = 1;

  pcommon->di.isKineticScrollTurnedOn = 1;

  hdc = GetDC(hwnd);
  SelectObject(hdc, pcommon->hfont);
  RefreshFontParams(hdc, &pcommon->di);

  if (!createOutBuffer(hdc, &pcommon->di, GetSystemMetrics(SM_CXSCREEN), pcommon->di.averageCharY + 2))
  {
    DestroyUndo(pundo);
    free(ptext->prealHead);
    free(ptext->pfakeHead);
    ptext->pfakeHead = NULL;
    ptext->prealHead = NULL;
    return SEDIT_INITIALIZE_ERROR;
  }
  ReleaseDC(hwnd, hdc);

  if ((res = SetEmptyDocument(ptext, &pcommon->nLines, &pcommon->nRealLines)) < 0)
  {
    DestroyUndo(pundo);
    if (ptext->prealHead)
      free(ptext->prealHead);
    if (ptext->pfakeHead)
      free(ptext->pfakeHead);
    ptext->pfakeHead = NULL;
    ptext->prealHead = NULL;
    return SEDIT_INITIALIZE_ERROR;
  }

  pfirstLine->pline = ptext->pfakeHead->next;
  pfirstLine->realNumber = pfirstLine->number = 0;
  pcaret->line = *pfirstLine;
  ptext->textFirstLine = *pfirstLine;

  StartSelection(pselection, pfirstLine, 0);

  (*psearchPos->piLine) = *pfirstLine;
  psearchPos->index = 0;
  psearchPos->pos = POS_BEGIN;

  return SEDIT_NO_ERROR;
}

extern char DestroyEditor(common_info_t *pcommon, caret_info_t *pcaret, undo_info_t *pundo,
                          pline_t pfakeHead, preal_line_t prealHead)
{
  DestroyUndo(pundo);
  DestroyRealLineList(prealHead);
  DestroyLineList(pfakeHead);
  destroyOutBuffer(&pcommon->di);
  if (pcommon->linePoints)
    free(pcommon->linePoints);
  if (pcommon->bkLinePoints)
    free(pcommon->bkLinePoints);
  if (pcommon->di.dx)
    free(pcommon->di.dx);
  if (pcaret->dx)
    free(pcaret->dx);

  return SEDIT_NO_ERROR;
}

#if defined(_WIN32_WCE)
static unsigned int identifyLongTap(HWND hwnd, long int x, long int y)
{
  SHRGINFO shrg;

  shrg.cbSize = sizeof(shrg);
  shrg.hwndClient = hwnd;
  shrg.ptDown.x = x;
  shrg.ptDown.y = y;
  shrg.dwFlags = SHRG_RETURNCMD;

  return SHRecognizeGesture(&shrg);
}
#endif

extern void MouseLButtonDown(window_t *pw, short int xCoord, short int yCoord)
{
  if (!(pw->caret.type & CARET_EXIST))
    SetFocus(pw->common.hwnd);
  SetCapture(pw->common.hwnd);

  TrackMousePosition(pw, xCoord, yCoord);
  StopKinecticScroll(pw);

#if defined(_WIN32_WCE)
  if (identifyLongTap(pw->common.hwnd, xCoord, yCoord) == GN_CONTEXTMENU)
  {
    ShowContextMenu(pw, xCoord, yCoord);
    return;
  }
#endif

  if (!pw->common.isDragMode)
  {
    char shiftPressed = IS_SHIFT_PRESSED;

    SetCaretPosFromCoords(&pw->common, &pw->caret, &pw->firstLine, xCoord, yCoord);

    if (shiftPressed)
      ChangeSelection(&pw->selection, &pw->caret.line, pw->caret.index);
    else
      pw->selection.isSelection = 0;

    if (pw->caret.line.pline == pw->selection.pos.startLine.pline)
      UpdateSelection(&pw->common, &pw->caret, &pw->firstLine, &pw->selection, pw->caret.dx); // перерисуем часть окна с выделением
    else
      UpdateSelection(&pw->common, &pw->caret, &pw->firstLine, &pw->selection, NULL);

    if (!shiftPressed)  // если шифт не нажат выделение должно начаться с позиции курсора
      StartSelection(&pw->selection, &pw->caret.line, pw->caret.index);
  }

  pw->mouse.lButtonDown = MOUSE_SINGLE_CLICK;


#ifdef _DEBUG
    {
      int error;
      _ASSERTE((error = SendMessage(pw->common.hwnd, SE_DEBUG, 0, 0)) == SEDIT_NO_ERROR);
    }
#endif
}

extern void MouseMove(window_t *pw, short int xCoord, short int yCoord)
{
  if (!pw->mouse.lButtonDown)
    return;

  if (!pw->common.isDragMode)
  {
    // если мышка за пределами окна по X, то надо прокрутить экран
    if (xCoord >= pw->common.clientX)
      HScrollWindow(&pw->common, &pw->caret, &pw->firstLine, pw->common.hscrollPos + AUTO_SCROLL);
    else if (xCoord <= 0)
      HScrollWindow(&pw->common, &pw->caret, &pw->firstLine, pw->common.hscrollPos - AUTO_SCROLL);

    // если мышка за пределами окна по Y, то надо прокрутить экран
    if (pw->common.isVScrollByLine) // включена полинейная прокрутка
    {
      if (yCoord >= pw->common.clientY)
        VScrollWindow(&pw->common, &pw->caret, &pw->firstLine, vScrollPosLineOnTop(&pw->common, pw->firstLine.number + AUTO_SCROLL));
      else if (yCoord <= 0)
      {
        if (pw->firstLine.number > AUTO_SCROLL)
          VScrollWindow(&pw->common, &pw->caret, &pw->firstLine, vScrollPosLineOnTop(&pw->common, pw->firstLine.number - AUTO_SCROLL));
        else
          VScrollWindow(&pw->common, &pw->caret, &pw->firstLine, vScrollPosLineOnTop(&pw->common, 0));
      }
    }
    else // попиксельная прокрутка
    {
      if (yCoord >= pw->common.clientY)
        VScrollWindow(&pw->common, &pw->caret, &pw->firstLine, pw->common.vscrollPos + AUTO_SCROLL);
      else if (yCoord <= 0)
        VScrollWindow(&pw->common, &pw->caret, &pw->firstLine, pw->common.vscrollPos - AUTO_SCROLL);
    }

    xCoord = min(xCoord, (short)pw->common.clientX);
    xCoord = max(xCoord, 0);
    yCoord = min(yCoord, (short)pw->common.clientY);
    yCoord = max(yCoord, 0);

    switch (pw->mouse.lButtonDown)
    {
    case MOUSE_SINGLE_CLICK:
      SetCaretPosFromCoords(&pw->common, &pw->caret, &pw->firstLine, xCoord, yCoord);
      break;

    case MOUSE_DOUBLE_CLICK:
      SetCaretPosFromCoords(&pw->common, &pw->caret, &pw->firstLine, xCoord, yCoord);
      if (pw->caret.line.number > pw->selection.pos.startLine.number ||
          (pw->caret.line.number == pw->selection.pos.startLine.number && pw->caret.index >= pw->selection.pos.startIndex))
      {
        // каретка справа от начала выделения, теперь проверим ориентацию выделения и, если что, поменяем
        if (pw->selection.pos.startLine.number > pw->selection.pos.endLine.number ||
            (pw->selection.pos.startLine.number == pw->selection.pos.endLine.number &&
             pw->selection.pos.startIndex > pw->selection.pos.endIndex))
        {
          pos_info_t pos;

          if (pw->selection.isSelection)
          {
            pw->selection.isSelection = 0;
            UpdateSelection(&pw->common, &pw->caret, &pw->firstLine, &pw->selection, NULL);
          }

          // выделение ориентировано справа налево, поменяем начало с концом
          pw->selection.pos.endLine = pw->selection.pos.startLine;
          pw->selection.pos.endIndex = pw->selection.pos.startIndex;
          pos.piLine = &pw->selection.pos.startLine;
          pw->selection.pos.startIndex = pos.index = getWordPos(pw->selection.pos.startLine.pline->prealLine->str,
                                                                pw->selection.pos.startIndex, POS_WORDBEGIN);
          UpdateLine(&pos);
        }
        MoveCaretToPosition(&pw->common, &pw->caret, &pw->firstLine, POS_WORDEND);
      }
      else
      {
        // каретка слева от начала выделения
        if (pw->selection.pos.startLine.number < pw->selection.pos.endLine.number ||
            (pw->selection.pos.startLine.number == pw->selection.pos.endLine.number &&
             pw->selection.pos.startIndex < pw->selection.pos.endIndex))
        {
          pos_info_t pos;

          if (pw->selection.isSelection)
          {
            pw->selection.isSelection = 0;
            UpdateSelection(&pw->common, &pw->caret, &pw->firstLine, &pw->selection, NULL);
          }

          // выделение ориентировано слева направо, поменяем начало с концом
          pw->selection.pos.endLine = pw->selection.pos.startLine;
          pw->selection.pos.endIndex = pw->selection.pos.startIndex;
          pos.piLine = &pw->selection.pos.startLine;
          pw->selection.pos.startIndex = pos.index = getWordPos(pw->selection.pos.startLine.pline->prealLine->str,
                                                            pw->selection.pos.startIndex, POS_WORDEND);
          UpdateLine(&pos);
        }
        MoveCaretToPosition(&pw->common, &pw->caret, &pw->firstLine, POS_WORDBEGIN);
      }
      break;
    }

    ChangeSelection(&pw->selection, &pw->caret.line, pw->caret.index);
    UpdateSelection(&pw->common, &pw->caret, &pw->firstLine, &pw->selection, pw->caret.dx);

    TrackMousePosition(pw, xCoord, yCoord);
  }
  else  // drag mode
  {
    const mouse_point_t *plastPoint = GetMouseLastPoint(pw);

    if (plastPoint)
    {
      int adjustedXCoord = max(min(xCoord, (short)pw->common.clientX), 0);
      int adjustedYCoord = max(min(yCoord, (short)pw->common.clientY), 0);
      int diffPosX = adjustedXCoord - plastPoint->x;
      int diffPosY = adjustedYCoord - plastPoint->y;
      int newXPos = pw->common.hscrollPos - diffPosX;
      int newYPos = pw->common.vscrollPos - diffPosY;
      char isDrawingRestricted = 0;

      newXPos = max(newXPos, 0);
      newXPos = min(newXPos, pw->common.hscrollRange);
      newYPos = max(newYPos, 0);
      newYPos = min(newYPos, pw->common.vscrollRange);

      // if there are both shifts, we should restrict drawing in order to draw them simultaneously
      if (!pw->common.di.isDrawingRestricted && newXPos != pw->common.hscrollPos && newYPos != pw->common.vscrollPos)
      {
        RestrictDrawing(&pw->common, &pw->caret, &pw->firstLine);
        isDrawingRestricted = 1;
      }

      HScrollWindow(&pw->common, &pw->caret, &pw->firstLine, newXPos);
      VScrollWindow(&pw->common, &pw->caret, &pw->firstLine, newYPos);

      if (isDrawingRestricted)
        AllowDrawing(&pw->common, &pw->caret, &pw->firstLine);
    }

    TrackMousePosition(pw, max(min(xCoord, (short)pw->common.clientX), 0), max(min(yCoord, (short)pw->common.clientY), 0));
  }

#ifdef _DEBUG
  {
    int error;
    _ASSERTE((error = SendMessage(pw->common.hwnd, SE_DEBUG, 0, 0)) == SEDIT_NO_ERROR);
  }
#endif
}

#if !defined(_WIN32_WCE) && defined(WHEEL_DELTA)
extern void MouseWheel(common_info_t *pcommon, caret_info_t *pcaret, line_info_t *pfirstLine, int delta)
{
  delta = (delta / WHEEL_DELTA) * WHEEL_CHANGE;

  if (pcommon->isVScrollByLine)
  {
    if (delta < 0) // колесо вниз
      VScrollWindow(pcommon, pcaret, pfirstLine, vScrollPosLineOnTop(pcommon, pfirstLine->number - delta));
    else if (delta > 0 && pfirstLine->number > (unsigned)delta)
      VScrollWindow(pcommon, pcaret, pfirstLine, vScrollPosLineOnTop(pcommon, pfirstLine->number - delta));
    else if (delta > 0)
      VScrollWindow(pcommon, pcaret, pfirstLine, vScrollPosLineOnTop(pcommon, 0));
  }
  else
    VScrollWindow(pcommon, pcaret, pfirstLine, pcommon->vscrollPos - delta * pcommon->di.averageCharY);
}
#endif
