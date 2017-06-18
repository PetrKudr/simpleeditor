#include "..\debug.h"
#include <windows.h>

#include "editor.h"
#include "undo.h"
#include "seregexp.h"
#include "selection.h"


extern void StartSelection(pselection_t pselection, line_info_t *piLine, unsigned int index)
{
  pselection->pos.startLine = pselection->pos.endLine = *piLine;
  pselection->pos.startIndex = pselection->pos.endIndex = index;
  pselection->isSelection = 0;
}

extern void ChangeSelection(pselection_t pselection, line_info_t *piLine, unsigned int index)
{
  _ASSERTE(pselection->pos.startLine.pline->begin <= pselection->pos.startIndex || pselection->pos.startLine.pline->end >= pselection->pos.startIndex);
  pselection->pos.endLine = *piLine;
  pselection->pos.endIndex = index;
  pselection->isSelection = 1;
  if (pselection->pos.startLine.realNumber == pselection->pos.endLine.realNumber && 
      pselection->pos.startIndex == pselection->pos.endIndex)
    pselection->isSelection = 0;
}

static void saveSelection(pselection_t pselection)
{
  memcpy(&pselection->oldPos, &pselection->pos, sizeof(selection_pos_t));
  pselection->isOldSelection = pselection->isSelection;
}


static void getSortedSelection(selection_pos_t *pselPos, 
                               line_info_t *sl,  line_info_t *el, unsigned int *si, unsigned int *ei)
{
  *sl = (pselPos->startLine.number <= pselPos->endLine.number) ? pselPos->startLine : pselPos->endLine;
  *el = (pselPos->startLine.number <= pselPos->endLine.number) ? pselPos->endLine : pselPos->startLine;
  *si = (sl->number == pselPos->startLine.number) ? pselPos->startIndex : pselPos->endIndex;
  *ei = (el->number == pselPos->endLine.number) ? pselPos->endIndex : pselPos->startIndex;

  if (sl->number == el->number && *si > *ei)
  {
    unsigned int tmp = *ei;
    *ei = *si;
    *si = tmp;
  }
}


static char getInvalidateRectForSelection(HDC hdc, pselection_t pselection, common_info_t *pcommon, 
                                          line_info_t *pfirstLine, RECT *rc, int *dx)
{
  selection_pos_t *pselPos;
  unsigned int slineN, elineN;

  rc->left = 0;
  rc->right = pcommon->clientX;

  // 1. выделенной области не было и нет
  if (!pselection->isSelection && !pselection->isOldSelection) 
    return 0;

  // 2. какая-то из областей есть
  else if ((!pselection->isOldSelection && pselection->isSelection) || 
           (pselection->isOldSelection && !pselection->isSelection)) 
  {    

    if (!pselection->isOldSelection && pselection->isSelection)
      pselPos = &pselection->pos; // выделенной области не было, а теперь есть
    else
      pselPos = &pselection->oldPos; // выделенная область была, а теперь нет

    slineN = (pselPos->startLine.number <= pselPos->endLine.number) ? pselPos->startLine.number : pselPos->endLine.number;
    elineN = (pselPos->startLine.number <= pselPos->endLine.number) ? pselPos->endLine.number : pselPos->startLine.number;

    if (slineN == elineN)
    {
      unsigned int si = pselPos->startIndex > pselPos->endIndex ? pselPos->endIndex : pselPos->startIndex;
      unsigned int ei = pselPos->startIndex > pselPos->endIndex ? pselPos->startIndex : pselPos->endIndex;

      rc->left = (int)GetStringSizeX(hdc, &pcommon->di, pselPos->startLine.pline->prealLine->str, 
        pselPos->startLine.pline->begin, si, dx) - 1 - pcommon->hscrollPos + pcommon->di.leftIndent;
      rc->left = max(rc->left, 0);
      rc->right = (int)GetStringSizeX(hdc, &pcommon->di, pselPos->startLine.pline->prealLine->str, 
        pselPos->startLine.pline->begin, ei, dx) + 1 - pcommon->hscrollPos + pcommon->di.leftIndent;
      rc->right = min(rc->right, (int)pcommon->clientX);
    }

    if (slineN > pfirstLine->number + pcommon->nscreenLines)
      return 0;
    
    rc->top = LINE_Y(pcommon, slineN);
    rc->bottom = LINE_Y(pcommon, elineN) + pcommon->di.averageCharY;
  }

  // 3. выделенная область была и сейчас есть
  else  
  {
    unsigned int oldsl, oldsi, newsl, newsi, oldel, oldei, newel, newei;
    unsigned int crosssl, crossel, crosssi, crossei;
    line_info_t slInfo, elInfo;
    pline_t pline;

    pselPos = &pselection->oldPos;
    getSortedSelection(pselPos, &slInfo, &elInfo, &oldsi, &oldei);
    oldsl = slInfo.number;
    oldel = elInfo.number;

    pselPos = &pselection->pos;
    getSortedSelection(pselPos, &slInfo, &elInfo, &newsi, &newei);
    newsl = slInfo.number;
    newel = elInfo.number;

    // если старое и новое выделения совпадают
    if (oldsl == newsl && oldsi == newsi && oldel == newel && oldei == newei)
      return 0;

    // различные случаи взаимного расположения выделенных областей
    if (oldsl == newsl && oldsi == newsi)
    {
      crosssl = oldel;
      crosssi = oldei;
      crossel = newel;
      crossei = newei;
      pline = elInfo.pline;
    }
    else if (oldsl == newel && oldsi == newei)
    {
      crosssl = oldel;
      crosssi = oldei;
      crossel = newsl;
      crossei = newsi;
      pline = slInfo.pline;
    }
    else if (oldel == newsl && oldei == newsi)
    {
      crosssl = oldsl;
      crosssi = oldsi;
      crossel = newel;
      crossei = newei;
      pline = elInfo.pline;
    }
    else if (oldel == newel && oldei == newei)
    {
      crosssl = oldsl;
      crosssi = oldsi;
      crossel = newsl;
      crossei = newsi;
      pline = slInfo.pline;
    }
    else
      MessageBox(pcommon->hwnd, L"Selection borders change error", SEDIT_ERROR_CAPTION, MB_OK);

    if (crosssl > crossel)
    {
      unsigned int tmp = crossel;
      crossel = crosssl;
      crosssl = tmp;
    }
    else if (crosssl == crossel)  // выделение поменялось только в строке
    {
      if (crosssi > crossei)
      {
        unsigned int tmp = crossei;
        crossei = crosssi;
        crosssi = tmp; 
      }

      rc->left = (int)GetStringSizeX(hdc, &pcommon->di, pline->prealLine->str, pline->begin, crosssi, dx) 
                  - pcommon->hscrollPos + pcommon->di.leftIndent;
      if (pline->begin < crosssi)
        rc->left -= pcommon->di.averageCharX;
      rc->left = max(rc->left, 0);

      rc->right = (int)GetStringSizeX(hdc, &pcommon->di, pline->prealLine->str, pline->begin, crossei, dx) 
                  - pcommon->hscrollPos + pcommon->di.leftIndent;
      if (pline->end > crossei + 1)
        rc->right += pcommon->di.averageCharY;
      rc->right = min(rc->right, (int)pcommon->clientX);
    }
     
    rc->top = LINE_Y(pcommon, crosssl);
    rc->bottom = LINE_Y(pcommon, crossel) + pcommon->di.averageCharY;
  }

  if (rc->top > pcommon->clientY)
    return 0;
  if (rc->bottom < 0)
    return 0;

  _ASSERTE(rc->top < rc->bottom);

  if (rc->top < 0)
    rc->top = 0;
  if (rc->bottom > pcommon->clientY)
    rc->bottom = pcommon->clientY;

  return 1;
}

// перерисовывает выделенную область. dx - массив расстояний для строки, в которой поменялась граница выдлеления
// dx игнорируется, если выделение занимает несколько строк
extern void UpdateSelection(common_info_t *pcommon, caret_info_t *pcaret, line_info_t *pfirstLine, 
                            pselection_t pselection, int *dx)
{
  RECT rc;
  HDC hdc = GetDC(pcommon->hwnd);
  SelectObject(hdc, pcommon->hfont);
  if (getInvalidateRectForSelection(hdc, pselection, pcommon, pfirstLine, &rc, dx))
  {
    SEInvalidateRect(pcommon, &rc, FALSE);
    SEUpdateWindow(pcommon, pcaret, pfirstLine);
  }
  saveSelection(pselection);
  ReleaseDC(pcommon->hwnd, hdc);
}

extern char GetSelectionBorders(pselection_t psel, 
                                line_info_t *piline, 
                                unsigned int begin, 
                                unsigned int end, 
                                unsigned int *b, 
                                unsigned int *e)
{
  unsigned int sl, el, si, ei;
  line_info_t slInfo, elInfo;

  *b = begin;
  *e = end;
  getSortedSelection(&psel->pos, &slInfo, &elInfo, &si, &ei);
  sl = slInfo.number;
  el = elInfo.number;

  if (psel->isSelection && sl <= piline->number && el >= piline->number)
  // эта строка попадает в границы выделенной области
  {
    if (sl == piline->number)
    {
      if (si >= begin && si <= end)
        *b = si;
      else if (si < begin)
        *b = begin;
      else
        return 0;
    }

    if (el == piline->number)
    {
      if (ei >= begin && ei <= end)
        *e = ei;
      else if (ei > end)
        *e = end;
      else
        return 0;
    }

    return 1;
  }
  return 0;
}

extern void SelectAll(common_info_t *pcommon, caret_info_t *pcaret, pline_t pfakeHead, 
                      pselection_t psel, line_info_t *pfirstLine)
{
  line_info_t piLine;
  unsigned int num = 0, realNum = 0;
  pline_t pline = pfakeHead->next;

  ClearSavedSel(psel);
  psel->pos.startLine.pline = pline;
  psel->pos.startLine.realNumber = psel->pos.startLine.number = 0;
  psel->pos.startIndex = pline->begin;

  while (pline->next)
  {
    pline = pline->next;
    ++num;
    if (pline->prev->prealLine != pline->prealLine)
      ++realNum;
  }

  piLine.pline = pline;
  piLine.number = num;
  piLine.realNumber = realNum;
  ChangeSelection(psel, &piLine, pline->end);
  saveSelection(psel);

  SEInvalidateRect(pcommon, NULL, FALSE);
  SEUpdateWindow(pcommon, pcaret, pfirstLine);
}

extern int CopySelectionToBuffer(pselection_t psel, wchar_t *buff, int size)
{
  line_info_t sl, el;
  unsigned int si, ei;
  int selSize;

  if (!psel->isSelection)
    return SEDIT_NO_SELECTION;

  getSortedSelection(&psel->pos, &sl, &el, &si, &ei);
  if ((selSize = (int)GetTextSize(sl.pline, si, el.pline, ei) + 1) > size)
    return selSize;

  if (!buff)
    return SEDIT_NO_PARAM;

  CopyTextToBuffer(sl.pline, si, el.pline, ei, buff);

  return SEDIT_NO_ERROR;
}

extern char CopySelectionToClipboard(HWND hwnd, pselection_t psel)
{
  line_info_t sl, el;
  unsigned int si, ei;
  wchar_t *buff;
  HGLOBAL hgBuff;
  size_t size;

  getSortedSelection(&psel->pos, &sl, &el, &si, &ei);
  size = GetTextSize(sl.pline, si, el.pline, ei);

  if ((hgBuff = GlobalAlloc(GMEM_MOVEABLE, (size + 1) * sizeof(wchar_t))) == NULL)
    return FALSE;

  buff = (wchar_t*)GlobalLock(hgBuff);
  CopyTextToBuffer(sl.pline, si, el.pline, ei, buff);
  GlobalUnlock(hgBuff);

  if (!OpenClipboard(hwnd))
  {
    GlobalFree(hgBuff);
    return FALSE;
  }
  EmptyClipboard();
  SetClipboardData(CF_UNICODETEXT, hgBuff);
  CloseClipboard();

  return TRUE;
}

extern char CompareSelection(pselection_t psel, wchar_t *source, int flags)
{
  line_info_t sl, el;
  unsigned int si, ei;
  size_t len = wcslen(source);

  if (!psel->isSelection)
    return 0;

  if (psel->pos.startLine.pline->prealLine != psel->pos.endLine.pline->prealLine)
    return 0;

  getSortedSelection(&psel->pos, &sl, &el, &si, &ei);

  if (flags & FR_WHOLEWORD)
  {
    if (si > 0 && !IsBreakSym(sl.pline->prealLine->str[si - 1]))
      return 0;
    if (ei != el.pline->end && !IsBreakSym(el.pline->prealLine->str[ei]))
      return 0;
  }
  
  if (flags & FR_MATCHCASE)
  {
    if ((ei - si == len) && (wcsncmp(&sl.pline->prealLine->str[si], source, len) == 0))
      return 1;
  }
  else
  {
    if ((ei - si == len) && (_wcsnicmp(&sl.pline->prealLine->str[si], source, len) == 0))
      return 1;
  }

  return 0;
}

extern char CompareSelectionRegexp(sedit_regexp_t *regexp, pselection_t psel, wchar_t *pattern)
{
  line_info_t sl, el;
  unsigned int si, ei;
  wchar_t savedSym;
  int retval;

  if (!psel->isSelection)
    return 0;

  if (psel->pos.startLine.pline->prealLine != psel->pos.endLine.pline->prealLine)
    return 0;

  getSortedSelection(&psel->pos, &sl, &el, &si, &ei);

  savedSym = el.pline->prealLine->str[ei];
  el.pline->prealLine->str[ei] = L'\0';
  retval = RegexpGetMatch(regexp, sl.pline->prealLine->str, si);
  el.pline->prealLine->str[ei] = savedSym;

  return retval;
}

/* Функция удаляет выделение.
 * и сбрасывается флаг isSelection
 *
 * ПАРАМЕТРЫ:
 *   pcommon - указатель на структуру с общей информацией об окне
 *   pcaret - информация о каретке
 *   psel - информация о выделении
 *   pfirstLine - первая строка на экране
 *   hscrollPos - указатель на переменную, содержащую текущую позицию по оси X
 *   hscrollRange - указатель на переменную, содержащую текущий диапазон скроллбара по оси X
 *   vscrollPos - указатель на переменную, содержащую текущую позицию по оси Y
 *   vscrollRange - указатель на переменную, содержащую текущий диапазон скроллбара по оси Y
 */
extern void DeleteSelection(common_info_t *pcommon, caret_info_t *pcaret, pselection_t psel, 
                            line_info_t *pfirstLine, undo_info_t *pundo)
{
  HDC hdc;
  RECT rc;
  pos_info_t ppos;
  range_t range;

  range.start.x = psel->pos.startIndex;
  range.start.y = psel->pos.startLine.realNumber;
  range.end.x = psel->pos.endIndex;
  range.end.y = psel->pos.endLine.realNumber;
  SortRange(&range);

#ifdef _DEBUG
  {
    int error;
    _ASSERTE(psel->isSelection);
    _ASSERTE((psel->pos.startLine.pline->prealLine != psel->pos.endLine.pline->prealLine) || 
             (psel->pos.startIndex != psel->pos.endIndex));
    _ASSERTE((error = SendMessage(pcommon->hwnd, SE_DEBUG, 0, 0)) == SEDIT_NO_ERROR);
  }
#endif

  hdc = GetDC(pcommon->hwnd);
  SelectObject(hdc, pcommon->hfont);
  if (psel->pos.startLine.number < psel->pos.endLine.number || 
      (psel->pos.startLine.number == psel->pos.endLine.number && psel->pos.startIndex < psel->pos.endIndex))
  {
    pcaret->line = psel->pos.startLine;  // первая линия выделения не могла быть удалена
    pcaret->index = psel->pos.startIndex;  // поэтому каретку поставим в начало выделенной области
    UpdateCaretSizes(hdc, pcommon, pcaret);
    UpdateCaretPos(pcommon, pcaret, pfirstLine);
    GotoCaret(pcommon, pcaret, pfirstLine);    

    ppos.piLine = &pcaret->line;
    ppos.index = pcaret->index;
    SetStartLine(&ppos);

    PutText(pundo, &psel->pos.startLine, psel->pos.startIndex, &psel->pos.endLine, psel->pos.endIndex);
    NextCell(pundo, ACTION_TEXT_DELETE, pcommon->isModified);

    DeleteArea(hdc, pcommon, pfirstLine, &psel->pos.startLine, psel->pos.startIndex, 
               &psel->pos.endLine, psel->pos.endIndex, &rc);
    UpdateLine(&ppos);
  }
  else if (psel->pos.startLine.number > psel->pos.endLine.number ||
           (psel->pos.startLine.number == psel->pos.endLine.number && psel->pos.startIndex > psel->pos.endIndex))
  {
    pcaret->line = psel->pos.endLine;    // начало выделенной области - endLine / endIndex
    pcaret->index = psel->pos.endIndex; 
    UpdateCaretSizes(hdc, pcommon, pcaret);
    UpdateCaretPos(pcommon, pcaret, pfirstLine);
    GotoCaret(pcommon, pcaret, pfirstLine); 

    ppos.piLine = &pcaret->line;
    ppos.index = pcaret->index;
    SetStartLine(&ppos);

    PutText(pundo, &psel->pos.endLine, psel->pos.endIndex, &psel->pos.startLine, psel->pos.startIndex);
    NextCell(pundo, ACTION_TEXT_DELETE, pcommon->isModified);

    DeleteArea(hdc, pcommon, pfirstLine, &psel->pos.endLine, psel->pos.endIndex, 
               &psel->pos.startLine, psel->pos.startIndex, &rc);
    UpdateLine(&ppos);
  }
 
  UpdateCaretSizes(hdc, pcommon, pcaret);
  UpdateCaretPos(pcommon, pcaret, pfirstLine);
  ClearSavedSel(psel);
  StartSelection(psel, &pcaret->line, pcaret->index);
  ReleaseDC(pcommon->hwnd, hdc);

  SEInvalidateRect(pcommon, &rc, FALSE);

  DocModified(pcommon, SEDIT_ACTION_DELETEAREA, &range);

  SEUpdateWindow(pcommon, pcaret, pfirstLine);
  UpdateScrollBars(pcommon, pcaret, pfirstLine);

  CaretModified(pcommon, pcaret, pfirstLine, CARET_RIGHT);

  DocUpdated(pcommon, SEDIT_ACTION_DELETEAREA, &range, 1);

#ifdef _DEBUG
  {
    int error;
    _ASSERTE((error = SendMessage(pcommon->hwnd, SE_DEBUG, 0, 0)) == SEDIT_NO_ERROR);
  }
#endif
}

// функция устанавливает экран так, чтобы было видно выделение
extern void GotoSelection(common_info_t *pcommon, caret_info_t *pcaret, pselection_t psel, line_info_t *pfirstLine)
{
  line_info_t sLine, eLine;
  unsigned int sIndex, eIndex;
  int sHScrollPos, eHScrollPos, sVScrollPos, eVScrollPos;
  int newHScrollPos, newVScrollPos;
  HDC hdc;

  if (!psel->isSelection)
    return;

  getSortedSelection(&psel->pos, &sLine, &eLine, &sIndex, &eIndex);

  hdc  = GetDC(pcommon->hwnd);
  SelectObject(hdc, pcommon->hfont);
  sHScrollPos = GetHorzScrollPos(hdc, pcommon, sLine.pline, sIndex, NULL);
  eHScrollPos = GetHorzScrollPos(hdc, pcommon, eLine.pline, eIndex, NULL);
  ReleaseDC(pcommon->hwnd, hdc);

  sVScrollPos = GetVertScrollPos(pcommon, pfirstLine, sLine.number);
  eVScrollPos = GetVertScrollPos(pcommon, pfirstLine, eLine.number);

  newHScrollPos = sHScrollPos;
  newVScrollPos = sVScrollPos;
  if (sHScrollPos >= pcommon->hscrollPos && eHScrollPos > pcommon->hscrollPos)
  {
    newHScrollPos = eHScrollPos;
    newVScrollPos = eVScrollPos;
  }
  else if (sVScrollPos >= pcommon->vscrollPos && eVScrollPos > sVScrollPos)
  {
    newHScrollPos = eHScrollPos;
    newVScrollPos = eVScrollPos;
  }

  HScrollWindow(pcommon, pcaret, pfirstLine, newHScrollPos);
  VScrollWindow(pcommon, pcaret, pfirstLine, newVScrollPos);
}

// функция выделяет слово под кареткой
extern void SelectWordUnderCaret(common_info_t *pcommon, caret_info_t *pcaret, pselection_t psel, line_info_t *pfirstLine)
{
  unsigned int si = pcaret->index, ei = pcaret->index;
  wchar_t *str = pcaret->line.pline->prealLine->str;
  pos_info_t pos;

  _ASSERTE(pcaret->length == RealStringLength(pcaret->line.pline));

  if (!IsBreakSym(str[pcaret->index]))
  {
    while (si && !IsBreakSym(str[si]))
      si--;
    if (IsBreakSym(str[si]))
      si++;

    while (ei < pcaret->length && !IsBreakSym(str[ei]))
      ei++;;
  }

  if (si == ei && ei < pcaret->length)
    ei++;
  else if (si == ei && si > 0)
    si--;

  // уберём выделение
  if (psel->isSelection)
  {
    psel->isSelection = 0;
    UpdateSelection(pcommon, pcaret, pfirstLine, psel, NULL);
  }
   
  psel->pos.startLine = psel->pos.endLine = pcaret->line;
  pos.piLine = &psel->pos.startLine;
  psel->pos.startIndex = pos.index = si;
  SetStartLine(&pos);
  UpdateLine(&pos);  // теперь в выделении настроена первая граница

  pos.piLine = &psel->pos.endLine;
  psel->pos.endIndex = pos.index = ei;
  SetStartLine(&pos);
  UpdateLine(&pos);  // теперь в выделении настроена вторая граница
  ChangeSelection(psel, &psel->pos.endLine, psel->pos.endIndex);  // чтобы обновить поле isSelection
  UpdateSelection(pcommon, pcaret, pfirstLine, psel, pcaret->dx);

  MoveCaret(pcommon, pcaret, pfirstLine, ei - pcaret->index, 0); // передвинем каретку в конец выделения
}

/* Функция убирает старое выделение и устанавливает новое
 * ПАРАМЕТРЫ:
 *   pw - информация об окне
 *   piStart - информация о линии начала выделения
 *   piEnd - информация о линии конца выделения
 *   si - индекс в строке piStart
 *   ei - индекс в строке piEnd
 *   updateScreen - 0 - экран не будет обновлён, 1 - будет
 *
 * ВОЗВРАЩАЕТ:
 *   0 - выделение есть, 1 - нет (pw->selection.isSelection)
 */
extern char SetSelection(window_t *pw, line_info_t *piStart, line_info_t *piEnd, 
                         unsigned int si, unsigned int ei, char updateScreen)
{
  if (pw->selection.isSelection)
  {
    pw->selection.isSelection = 0;
    if (updateScreen)
      UpdateSelection(&pw->common, &pw->caret, &pw->firstLine, &pw->selection, NULL);
  }

  StartSelection(&pw->selection, piStart, si);
  ChangeSelection(&pw->selection, piEnd, ei);

  if (updateScreen)
    UpdateSelection(&pw->common, &pw->caret, &pw->firstLine, &pw->selection, NULL);

  return pw->selection.isSelection;
}