#include <wchar.h>

#include "undo.h"
#include "globals.h"
#include "smarttext.h"

#define BREAKSYMS_SIZE (SEDIT_BREAKSYMSTR_SIZE)
static wchar_t breakSyms[BREAKSYMS_SIZE] = L"\0";
static int nBreakSyms = 0;

// функция анализирует позицию состояние каретки, а также последние действия.
// По результатам устанавливает новое действие
extern void GetActionForSmartInput(common_info_t *pcommon, caret_info_t *pcaret, undo_info_t *pundo)
{
  wchar_t ch;

  if (!(pcommon->action & ACTION_TURN_ON) || pcommon->action & ACTION_IN_PROGRESS)
    return;

  ch = pcaret->index > 0 ? pcaret->line.pline->prealLine->str[pcaret->index - 1] : L'\0';

  switch (ch)
  {
  case L'?':
    // "! ?" заменяется на "!? ", "? ?" на "?? "
    if (pcaret->index > pcaret->line.pline->begin + 2 && pcaret->line.pline->prealLine->str[pcaret->index - 2] == L' ' &&
        (pcaret->line.pline->prealLine->str[pcaret->index - 3] == L'!' || 
         pcaret->line.pline->prealLine->str[pcaret->index - 3] == L'?'))
    {
      wcscpy(pcommon->actString, L"? ");
      pcommon->action |= ACTION_DOUBLE_BACKSPACE | ACTION_ADD_STRING | ACTION_UPCASE;
    }
    else
    {
      wcscpy(pcommon->actString, L" ");
      pcommon->action |= ACTION_ADD_STRING | ACTION_UPCASE;        
    }
    break;

  case L'!':
    // "! !" заменяется на "!! ", "? !" на "?! "
    if (pcaret->index > pcaret->line.pline->begin + 2 && pcaret->line.pline->prealLine->str[pcaret->index - 2] == L' ' &&
        (pcaret->line.pline->prealLine->str[pcaret->index - 3] == L'?' ||
         pcaret->line.pline->prealLine->str[pcaret->index - 3] == L'!'))
    {
      wcscpy(pcommon->actString, L"! ");
      pcommon->action |= ACTION_DOUBLE_BACKSPACE | ACTION_ADD_STRING | ACTION_UPCASE;
    }
    else
    {
      wcscpy(pcommon->actString, L" ");
      pcommon->action |= ACTION_ADD_STRING | ACTION_UPCASE;        
    }
    break;

  case L':':
  case L';':
  case L',':
    pcommon->action |= ACTION_ADD_STRING;
    wcscpy(pcommon->actString, L" ");
    break;

  case L'.':
    // .. заменяется на "... "
    if (pcaret->index > pcaret->line.pline->begin + 2 && pcaret->line.pline->prealLine->str[pcaret->index - 3] == L'.' && 
        pcaret->line.pline->prealLine->str[pcaret->index - 2] == L' ')
    {
      wcscpy(pcommon->actString, L".. ");
      pcommon->action |= ACTION_DOUBLE_BACKSPACE | ACTION_ADD_STRING | ACTION_UPCASE;
    }
    else
    {
      wcscpy(pcommon->actString, L" ");
      pcommon->action |= ACTION_ADD_STRING | ACTION_UPCASE;        
    }
    break;

  case L'-':
    if (pcaret->index > pcaret->line.pline->begin + 1 && pcaret->line.pline->prealLine->str[pcaret->index - 2] == L' ')
    {
      wcscpy(pcommon->actString, L" ");
      pcommon->action |= ACTION_ADD_STRING;
    }
    break;

  case L'&':
    // "& &" заменяется на "&& "
    if (pcaret->index > pcaret->line.pline->begin + 2 && 
        pcaret->line.pline->prealLine->str[pcaret->index - 2] == L' ' &&
        pcaret->line.pline->prealLine->str[pcaret->index - 3] == L'&')
    {
      wcscpy(pcommon->actString, L"& ");
      pcommon->action |= ACTION_DOUBLE_BACKSPACE | ACTION_ADD_STRING;
    }
    else
    {
      wcscpy(pcommon->actString, L" ");
      pcommon->action |= ACTION_ADD_STRING;        
    }
    break;

  case L'|':
    // "| |" заменяется на "|| "
    if (pcaret->index > pcaret->line.pline->begin + 2 && 
        pcaret->line.pline->prealLine->str[pcaret->index - 2] == L' ' &&
        pcaret->line.pline->prealLine->str[pcaret->index - 3] == L'|')
    {
      wcscpy(pcommon->actString, L"| ");
      pcommon->action |= ACTION_DOUBLE_BACKSPACE | ACTION_ADD_STRING;
    }
    else
    {
      wcscpy(pcommon->actString, L" ");
      pcommon->action |= ACTION_ADD_STRING;        
    }
    break;

  case L'(':
    wcscpy(pcommon->actString, L")");
    pcommon->action |= ACTION_ADD_STRING | ACTION_MOVE_CARET_LEFT;
    break;

  case L'[':
    wcscpy(pcommon->actString, L"]");
    pcommon->action |= ACTION_ADD_STRING | ACTION_MOVE_CARET_LEFT;
    break;

  case L'{':
    wcscpy(pcommon->actString, L"}");
    pcommon->action |= ACTION_ADD_STRING | ACTION_MOVE_CARET_LEFT;
    break;

  case L'\'':
    wcscpy(pcommon->actString, L"\'");
    pcommon->action |= ACTION_ADD_STRING | ACTION_MOVE_CARET_LEFT;
    break;

  case L'\"':
    wcscpy(pcommon->actString, L"\"");
    pcommon->action |= ACTION_ADD_STRING | ACTION_MOVE_CARET_LEFT;
    break;

  /*case L'\0': // новая строка
    if (!pundo->undoAvailable)
      break;
    i = (pundo->stackIndex + UNDO_STACK_SIZE - 1) % UNDO_STACK_SIZE;

    if (pundo->stack[i].dataType == ACTION_NEWLINE_ADD && pcommon->action & ACTION_UPCASE)
    {
      pcommon->action &= (~ACTION_UPCASE);
      pcommon->action |= ACTION_DOUBLE_UPCASE;
    }
    break;*/
  }
}

extern void DoSmartInputAction(common_info_t *pcommon, caret_info_t *pcaret, wchar_t *ch)
{
  if (!(pcommon->action & ACTION_TURN_ON) || (pcommon->action & ACTION_IN_PROGRESS))
    return;

  pcommon->action |= ACTION_IN_PROGRESS;

  if (pcommon->action & ACTION_BACKSPACE)
  {
    pcommon->action = pcommon->action & (~ACTION_BACKSPACE);
    SendMessage(pcommon->hwnd, WM_CHAR, VK_BACK, 0);
  }

  if (pcommon->action & ACTION_DOUBLE_BACKSPACE)
  {
    pcommon->action = pcommon->action & (~ACTION_DOUBLE_BACKSPACE);
    SendMessage(pcommon->hwnd, WM_CHAR, VK_BACK, 0);
    SendMessage(pcommon->hwnd, WM_CHAR, VK_BACK, 0);
  }

  if (pcommon->action & ACTION_ADD_STRING)
  {
    int i = 0;

    pcommon->action &= (~ACTION_ADD_STRING);
    while (pcommon->actString[i])
      SendMessage(pcommon->hwnd, WM_CHAR, (WPARAM)pcommon->actString[i++], 0);
  }

  if ((pcommon->action & (ACTION_UPCASE | ACTION_DOUBLE_UPCASE)) && ch && !isCRLFSym(*ch))
  {
    *ch = towupper(*ch);
    pcommon->action &= (~ACTION_UPCASE);
 
    if (pcommon->action & ACTION_DOUBLE_UPCASE)
    {
      pcommon->action &= (~ACTION_DOUBLE_UPCASE);
      pcommon->action |= ACTION_UPCASE;
    }
  }
  else if (pcommon->action & ACTION_UPCASE && ch && isCRLFSym(*ch))  // для перевода строки с сохранением UPCASE
  {
    pcommon->action &= (~ACTION_UPCASE);
    pcommon->action |= ACTION_DOUBLE_UPCASE;
  }

  if (pcommon->action & ACTION_MOVE_CARET_LEFT)
  {
    range_t pselPos;

    pcommon->action &= (~ACTION_MOVE_CARET_LEFT);
    SendMessage(pcommon->hwnd, WM_KEYDOWN, VK_LEFT, 0);

    memset(&pselPos, 0, sizeof(pselPos));
    SendMessage(pcommon->hwnd, SE_SETSEL, (WPARAM)&pselPos, SEDIT_VISUAL_LINE);
  }

  pcommon->action &= (~ACTION_IN_PROGRESS);
}

extern void ActionOnCaretModified(common_info_t *pcommon)
{
  if (pcommon->action & ACTION_TURN_ON && !(pcommon->action & ACTION_IN_PROGRESS))
  {
    pcommon->action &= (~ACTION_UPCASE);
    if (pcommon->action & ACTION_DOUBLE_UPCASE)
    {
      pcommon->action &= (~ACTION_DOUBLE_UPCASE);
      pcommon->action |= ACTION_UPCASE;
    }
  }
}

extern char AddBreakSym(wchar_t sym)
{
  if (nBreakSyms < SEDIT_BREAKSYMSTR_SIZE)
  {
    if (!IsBreakSym(sym))
    {
      int i = nBreakSyms++;

      breakSyms[i] = sym;
      while (i > 0 && breakSyms[--i] > sym)
      {
        breakSyms[i+1] = breakSyms[i];
        breakSyms[i] = sym;
      }
    }
    return SEDIT_NO_ERROR;
  }
  return SEDIT_BREAKSYMS_LIMIT_REACHED;
}

extern char IsBreakSym(wchar_t sym)
{
  if (nBreakSyms > 0)
  {
    int l = 0, r = nBreakSyms - 1, i;

    while (l + 1 < r)
    {
      i = (l + r) / 2;
      if (breakSyms[i] < sym)
        l = i;
      else if (breakSyms[i] > sym)
        r = i;
      else
        return 1;
    }
    if (breakSyms[l] == sym || breakSyms[r] == sym)
      return 1;
  }
  return 0;
}