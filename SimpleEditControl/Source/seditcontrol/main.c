#include "..\debug.h"
#if defined(_WIN32_WCE)
#include <aygshell.h>
#pragma comment(lib, "aygshell.lib") 
#endif
#include <limits.h>   // USHRT_MAX
#include <string.h>
  
#include "stringmas.h"
#include "selection.h"
#include "Find.h"
#include "undo.h"
#include "mouse.h"
#include "kinetic.h"
#include "editor.h"
#include "seditcontrol.h"
#include "seditfunctions.h"


#define QUICK_COPY               3L         // Ctrl + C
#define QUICK_PASTE              22L        // Ctrl + V
#define QUICK_CUT                24L        // Ctrl + X
#define QUICK_UNDO               26L        // Ctrl + Z
#define QUICK_REDO               25L        // Ctrl + Y
#define QUICK_SELECT_ALL         1L         // Ctrl + A
#define QUICK_DELETE_PREV_WORD   127L       // Ctrl + Backspace
//#define QUICK_DELETE_NEXT_WORD   127L       // Ctrl + Delete


// прототип оконной функции
static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


//static unsigned int clickTime[CLICKS] = {0, CLICK_DIFF, 2 * CLICK_DIFF};
//static char currentClick = 0;

#ifndef GetWindowLongPtr
#define GetWindowLongPtr(hwnd, nIndex) GetWindowLongW(hwnd, nIndex)
#endif
#ifndef SetWindowLongPtr
#define SetWindowLongPtr(hwnd, nIndex, dwNewLong) SetWindowLongW(hwnd, nIndex, dwNewLong)
#endif

#ifdef _DEBUG
static char testFakeLines(pline_t pfakeHead)
{
  pline_t pline = pfakeHead->next;

  while (pline)
  {
    if (pline->prev->prealLine == pline->prealLine && pline->begin == pline->end)
      return 0;
    if (pline->begin > pline->end)
      return 0;
    pline = pline->next;
  }

  return 1;
}
#endif

static void showSEditError(HWND hwnd, long int error)
{
  switch (error)
  {
  case SEDIT_MEMORY_ERROR:
    MessageBox(hwnd, SEDIT_ALLOCATE_MEMORY_ERRMSG, SEDIT_ERROR_CAPTION, MB_OK);
    break;

//  case SEDIT_COPY_ERROR:
//    MessageBox(hwnd, SEDIT_COPY_ERRMSG, SEDIT_ERROR_CAPTION, MB_OK);
//    break;

  case SEDIT_CREATE_BRUSH_ERROR:
    MessageBox(hwnd, SEDIT_CREATE_BRUSH_ERRMSG, SEDIT_ERROR_CAPTION, MB_OK);
    break;

  case SEDIT_CREATE_PEN_ERROR:
    MessageBox(hwnd, SEDIT_CREATE_PEN_ERRMSG, SEDIT_ERROR_CAPTION, MB_OK);
    break;
  }
}

static window_t* createDocument(HWND hwnd, char *error)
{
  window_t *pwindow;

  _ASSERTE(error);
  *error = SEDIT_NO_ERROR;

  if ((pwindow = calloc(1, sizeof(window_t))) == NULL)
  {
    *error = SEDIT_MEMORY_ERROR;
    return NULL;
  }
  pwindow->searchPos.piLine = &pwindow->searchLine;

  *error = InitEditor(hwnd,
                      &pwindow->common,
                      &pwindow->caret,
                      &pwindow->firstLine,
                      &pwindow->undo,
                      &pwindow->selection,
                      &pwindow->lists,
                      &pwindow->searchPos);
  if (*error != SEDIT_NO_ERROR)
  {
    free(pwindow);
    return NULL;
  }

  return pwindow;
}

static char deleteDocument(window_t *pwindow)
{
  DestroyEditor(&pwindow->common, &pwindow->caret, &pwindow->undo,
                 pwindow->lists.pfakeHead, pwindow->lists.prealHead);
  //if (pwindow->common.hfont)
  //  DeleteObject((HGDIOBJ)pwindow->common.hfont);
  free(pwindow);

  return 1;
}

// функция проверяет заблокировано ли данное сообщение (только для SE_...)
static char isBlocked(char lockType, unsigned int msg)
{
  int nUnblocked;

  _ASSERTE(lockType <= LOCK_TYPES);

  if (!lockType)
    return 0;
  --lockType;

  nUnblocked = g_unblocked[lockType][0];
  while (nUnblocked > 0)
    if (g_unblocked[lockType][nUnblocked--] == msg)
      return 0;
  return 1;
}



// оконная процедура
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  window_t *pw = (window_t*)GetWindowLongPtr(hwnd, 0);

  // msg > SE_FIRST && msg < SE_LAST <=> this is editor's message
  if (msg > SE_FIRST && msg < SE_LAST)
  {
    if (!pw)
      return SEDIT_WINDOW_INFO_UNAVAILABLE;  // critical error.
    else if (isBlocked(pw->isLocked, msg))
      return SEDIT_WINDOW_IS_BLOCKED;        // message is blocked
  }

#ifdef _DEBUG
  if (pw != NULL)
  {
    _ASSERTE(pw->selection.isSelection == 0 ||
             (pw->selection.isSelection == 1 &&
             ((pw->selection.pos.startLine.pline != pw->selection.pos.endLine.pline) || 
             (pw->selection.pos.startIndex != pw->selection.pos.endIndex))));
  }
#endif

  // Notify about received message
  if (pw && pw->common.ReceiveMessageFunc)
  {
    LRESULT result;
    if (pw->common.ReceiveMessageFunc(pw->common.hwnd, pw->common.id, msg, wParam, lParam, &result))
      return result;
  }

  switch (msg)
  {

  case SE_CLEARTEXT:
    CloseDocument(&pw->common, &pw->caret, &pw->selection, &pw->firstLine, &pw->undo, &pw->lists);
    break;

  case SE_SETID:
    pw->common.id = wParam;
    break;

  case SE_SETOPTIONS:
    if (!wParam)
      return SEDIT_NO_PARAM;
    SetOptions(&pw->common, (options_t*)wParam);
    break;

  case SE_SETUNDOSIZE:
    DestroyUndo(&pw->undo);
    if (lParam != 0 && !InitUndo(&pw->undo, (size_t)lParam))
      return SEDIT_INITIALIZE_UNDO_ERROR;
    break;

  case SE_SETSCROLLBYLINE:
    if (wParam == pw->common.isVScrollByLine)
      break;
    pw->common.isVScrollByLine = !pw->common.isVScrollByLine;
    VScrollWindow(&pw->common, &pw->caret, &pw->firstLine, vScrollPosLineOnTop(&pw->common, pw->firstLine.number));
    break;

  case SE_SETWORDWRAP:
    if (wParam == pw->common.isWordWrap)
      break;
    ChangeWordWrap(&pw->common, &pw->caret, &pw->selection, &pw->lists, &pw->firstLine);
    break;

  case SE_SETSMARTINPUT:
    pw->common.action = (wParam == 0) ? 0 : ACTION_TURN_ON;
    break;

  case SE_SETDRAGMODE:
    pw->common.isDragMode = (wParam == 0) ? 0 : 1;
    // если мы были в режиме прокрутки, и вышли из него с нажатой левой кнопкой мыши, 
    // то выделение должно начатся(с позиции каретки). Если у нас было выделение или был нажат шифт,
    // то выделение находится в актуальном состоянии (начинать его заново с позиции каретки не надо)
    if (!pw->common.isDragMode && pw->mouse.lButtonDown && !pw->selection.isSelection && !IS_SHIFT_PRESSED)
      StartSelection(&pw->selection, &pw->caret.line, pw->caret.index);
    break;

  case SE_SETPOSITIONS:
    if (!wParam)
      return SEDIT_NO_PARAM;
    SetMainPositions(&pw->common, &pw->caret, &pw->firstLine, &pw->selection, (doc_info_t*)wParam);
    break;

  case SE_SETMODIFY:
    {
      range_t range = {0};

      if (!wParam)
        RemoveUnmodAction(&pw->undo); 

      DocUpdated(&pw->common, SEDIT_ACTION_NONE, &range, wParam ? 1 : 0);
    }
    break;

  case SE_COPY:
    if (pw->selection.isSelection)
      if (!CopySelectionToClipboard(hwnd, &pw->selection))
        return SEDIT_COPY_ERROR;
    break;

  case SE_CUT:
    if (pw->selection.isSelection)
    {
      if (CopySelectionToClipboard(hwnd, &pw->selection))
        DeleteSelection(&pw->common, &pw->caret, &pw->selection, &pw->firstLine, &pw->undo);
      else
        return SEDIT_COPY_ERROR;
    }
    break;

  case SE_PASTE:
    if (pw->mouse.lButtonDown)
      break;

    if (pw->selection.isSelection)
      DeleteSelection(&pw->common, &pw->caret, &pw->selection, &pw->firstLine, &pw->undo);

    PasteFromClipboard(&pw->common, &pw->caret, &pw->selection, &pw->firstLine, &pw->undo);
    _ASSERTE(GetLinesNumber(pw->lists.pfakeHead, NULL, 0) == pw->common.nLines);
    break;

  case SE_DELETE:
    if (pw->selection.isSelection)
      DeleteSelection(&pw->common, &pw->caret, &pw->selection, &pw->firstLine, &pw->undo);
    else
      DeleteChar(&pw->common, &pw->caret, &pw->selection, &pw->firstLine, &pw->undo);
    break;

  case SE_SELECTALL:
    SelectAll(&pw->common, &pw->caret, pw->lists.pfakeHead, &pw->selection, &pw->firstLine);
    break;

  case SE_UNDO:
    if (!pw->mouse.lButtonDown) // если пользователь выделяет текст, то активных действий не призводим
      UndoOperation(&pw->undo, &pw->common, &pw->caret, &pw->firstLine, &pw->selection, &pw->lists);
    break;

  case SE_REDO:
    if (!pw->mouse.lButtonDown) // если пользователь выделяет текст, то активных действий не призводим
      RedoOperation(&pw->undo, &pw->common, &pw->caret, &pw->firstLine, &pw->selection, &pw->lists);
    break;

  case SE_ISEMPTY:
    return (pw->common.nLines <= 1 && pw->lists.pfakeHead->next->begin == pw->lists.pfakeHead->next->end);

  case SE_ISSCROLLBYLINE:
    return pw->common.isVScrollByLine;

  case SE_ISWORDWRAP:
    return pw->common.isWordWrap;

  case SE_ISSMARTINPUT:
    return (pw->common.action & ACTION_TURN_ON) ? 1 : 0;

  case SE_ISDRAGMODE:
    return pw->common.isDragMode;

  case SE_GETOPTIONS:
    if (!wParam)
      return SEDIT_NO_PARAM;
    else
    {
      options_t *popt = (options_t*)wParam;
      popt->hfont = pw->common.hfont;
      popt->bgColor = pw->common.di.bgColor;
      popt->textColor = pw->common.di.textColor;
      popt->caretTextColor = pw->common.di.caretTextColor;
      popt->caretBgColor = pw->common.di.caretBgColor;
      popt->selectedTextColor = pw->common.di.selectedTextColor;
      popt->selectionColor = pw->common.di.selectionColor;
      popt->tabSize = pw->common.di.tabSize;
      popt->caretWidth = pw->common.di.caretWidth;
      popt->leftIndent = pw->common.di.leftIndent;
      popt->selectionTransparency = pw->common.di.selectionTransparency;
      popt->wrapType = pw->common.di.wordWrapType;
      popt->hideSpaceSyms = pw->common.di.hideSpaceSyms;
      popt->permanentScrollBars = (pw->common.SBFlags & PERMANENT_SCROLLBARS) ? 1 : 0;
      popt->useRegexp = pw->common.useRegexp;
      popt->lineBreakType = SEDIT_VISUAL_LINEBREAK_CRLF; // others are unsupported yet
      popt->kineticScroll = pw->common.di.isKineticScrollTurnedOn;
    }
    break;

  case SE_GETLINELENGTH:
    if (lParam == SEDIT_VISUAL_LINE)
    {
      pline_t pline = GetLineFromVisualPos(GetNearestVisualLine(&pw->caret, &pw->firstLine, &pw->lists, wParam), wParam, NULL);
      return pline->end - pline->begin;
    }
    else
    {
      pline_t pline = GetLineFromRealPos(GetNearestRealLine(&pw->caret, &pw->firstLine, &pw->lists, wParam), wParam, 0, NULL);
      return RealStringLength(pline);
    }
    break;

  case SE_ISCARETVISIBLE:
    return (pw->caret.type & CARET_VISIBLE) ? 1 : 0; 

  case SE_ISMODIFIED:
    return pw->common.isModified;

  case SE_ISSELECTION:
    return pw->selection.isSelection;

  case SE_CANUNDO:
    return IsUndoAvailable(&pw->undo);

  case SE_CANREDO:
    return IsRedoAvailable(&pw->undo);

  case SE_GETID:
    return pw->common.id;

  case SE_GETDOCINFO:
    if (!wParam)
      return SEDIT_NO_PARAM;
    else
    {
      doc_info_t *pdoc = (doc_info_t*)wParam;

      pdoc->nLines = pw->common.nLines;
      pdoc->nRealLines = pw->common.nRealLines;

      if (pdoc->flags & SEDIT_GETWINDOWPOS)
      {
        pdoc->windowPos.x = pw->common.hscrollPos;
        pdoc->windowPos.y = pw->common.vscrollPos;
      }

      if (pdoc->flags & SEDIT_GETCARETPOS)
      {
        pdoc->caretPos.x = pw->caret.index - pw->caret.line.pline->begin;
        pdoc->caretPos.y = pw->caret.line.number;
      }
      else if (pdoc->flags & SEDIT_GETCARETREALPOS)
      {
        pdoc->caretPos.x = pw->caret.index;
        pdoc->caretPos.y = pw->caret.line.realNumber;
      }
    }
    break;

  case SE_FINDREPLACE:
    {
      LPFINDREPLACE lpfr = (LPFINDREPLACE)wParam;

      if (lpfr->Flags & FR_FINDNEXT) // была нажата кнопка Find Next
        return Find(lpfr, &pw->common, &pw->caret, &pw->searchPos, &pw->selection, &pw->firstLine); 
      else if (lpfr->Flags & FR_REPLACE)  // была нажата кнопка Replace
        return Replace(lpfr, &pw->common, &pw->caret, &pw->undo, &pw->searchPos, &pw->selection, &pw->firstLine);
      else if (lpfr->Flags & FR_REPLACEALL)  // была нажата кнопка Replace All
      {
        int replaced; // число замен

        replaced = ReplaceAll(lpfr, &pw->common, &pw->caret, &pw->selection, &pw->firstLine, &pw->undo, pw->lists.pfakeHead);
        if (replaced < 0)
          return replaced;
        if (lParam)
          *((int*)lParam) = replaced;
      }
    }
    break;

  case SE_SETSEARCHPOS:
    if (lParam)
    {
      position_t *ppos = (position_t*)lParam;

      if (wParam == SEDIT_VISUAL_LINE)
      {
        pw->searchLine.pline = GetLineFromVisualPos(GetNearestVisualLine(&pw->caret, &pw->firstLine, &pw->lists, ppos->y), ppos->y, &pw->searchLine.realNumber);
        pw->searchLine.number = ppos->y;
        pw->searchPos.index = pw->searchLine.pline->begin + ppos->x;
      }
      else
      {
        pw->searchLine.pline = GetLineFromRealPos(GetNearestRealLine(&pw->caret, &pw->firstLine, &pw->lists, ppos->y), ppos->y, ppos->x, &pw->searchLine.number);
        pw->searchLine.realNumber = ppos->y;
        pw->searchPos.index = ppos->x;
      }
    }
    else 
      return SEDIT_NO_PARAM;
    break;

  case SE_GETPOSFROMOFFSET:
    return SEditGetPositionFromOffset(pw, wParam, (position_t*)lParam);

  case SE_GETOFFSETFROMPOS:
    return SEditGetOffsetFromPosition(pw, (position_t*)wParam, (unsigned int*)lParam);

  case SE_PASTEFROMBUFFER:
    if (pw->mouse.lButtonDown)
      break; 
    if (pw->selection.isSelection)
    {
      if (!lParam)
      {
        pw->selection.isSelection = 0;
        UpdateSelection(&pw->common, &pw->caret, &pw->firstLine, &pw->selection, NULL);
      }
      else
        DeleteSelection(&pw->common, &pw->caret, &pw->selection, &pw->firstLine, &pw->undo);
    }
    return PasteFromBuffer(&pw->common, &pw->caret, &pw->selection, &pw->firstLine, &pw->undo, (wchar_t*)wParam);

  case SE_MOVECARET:
    MoveCaret(&pw->common, &pw->caret, &pw->firstLine, wParam, (char)lParam);
    break;

  case SE_GOTOCARET:
    GotoCaret(&pw->common, &pw->caret, &pw->firstLine);
    break;

  case SE_SETSEL:
    if (wParam)
    {
      range_t *pselPos = (range_t*)wParam;
      line_info_t iStartLine, iEndLine;
      unsigned int start, end;

      if (lParam == SEDIT_VISUAL_LINE)
      {
        iStartLine.number = pselPos->start.y;
        iStartLine.pline = GetLineFromVisualPos(&pw->firstLine, iStartLine.number, &iStartLine.realNumber);
        start = iStartLine.pline->begin + pselPos->start.x;

        iEndLine.number = pselPos->end.y;
        iEndLine.pline = GetLineFromVisualPos(&iStartLine, iEndLine.number, &iEndLine.realNumber);
        end = iEndLine.pline->begin + pselPos->end.x;
      }
      else if (lParam == SEDIT_REALLINE)
      {
        iStartLine.realNumber = pselPos->start.y;
        start = pselPos->start.x;
        iStartLine.pline = GetLineFromRealPos(GetNearestRealLine(&pw->caret, &pw->firstLine, &pw->lists, iStartLine.realNumber), 
                                              iStartLine.realNumber, start, &iStartLine.number);

        iEndLine.realNumber = pselPos->end.y;
        end = pselPos->end.x;
        iEndLine.pline = GetLineFromRealPos(GetNearestRealLine(&pw->caret, &pw->firstLine, &pw->lists, iEndLine.realNumber), 
                                            iEndLine.realNumber, end, &iEndLine.number);
      }
      else
        return SEDIT_INVALID_PARAM;

      SetSelection(pw, &iStartLine, &iEndLine, start, end, 1);
    }
    else
      return SEDIT_NO_PARAM;
    break;

  case SE_GETSEL:
    if (wParam)
    {
      range_t *pselPos = (range_t*)wParam;

      if (lParam == SEDIT_VISUAL_LINE)
      {
        pselPos->start.y = pw->selection.pos.startLine.number;
        pselPos->start.x = pw->selection.pos.startIndex - pw->selection.pos.startLine.pline->begin;
        pselPos->end.y = pw->selection.pos.endLine.number;
        pselPos->end.x = pw->selection.pos.endIndex - pw->selection.pos.endLine.pline->begin;
      }
      else if (lParam == SEDIT_REALLINE)
      {
        pselPos->start.y = pw->selection.pos.startLine.realNumber;
        pselPos->start.x = pw->selection.pos.startIndex;
        pselPos->end.y = pw->selection.pos.endLine.realNumber;
        pselPos->end.x = pw->selection.pos.endIndex;
      }
      else
        return SEDIT_INVALID_PARAM;
    }
    else
      return SEDIT_NO_PARAM;
    break;

  case SE_GETTEXT:
    if (wParam) 
    {
      text_block_t *ptextBlock = (text_block_t*)wParam;  
      return SEditGetRange(pw, (char)lParam, &ptextBlock->position, ptextBlock->size, ptextBlock->text);
    }
    return SEDIT_NO_PARAM;

  case SE_GETSELECTEDTEXT:
    return CopySelectionToBuffer(&pw->selection, (wchar_t*)wParam, (int)lParam);
     
  case SE_VALIDATE:
    pw->common.forValidateRect = wParam ? SEDIT_REPAINTED : SEDIT_RESTRICT_VALIDATE;
    break;

  case SE_UPDATERANGE:
    SEInvalidateRange(pw, (range_t*)lParam, (char)LOWORD(wParam)/*, (char)HIWORD(wParam)*/);
    SEUpdateWindow(&pw->common, &pw->caret, &pw->firstLine);
    break;

  case SE_BEGINAPPEND:
    {
      pline_t pline = pw->lists.pfakeHead->next;

      while (pline->next)
        pline = pline->next;
      while (pline->prev->prealLine == pline->prealLine)
        pline = pline->prev;
      pw->isLocked = LOCKED_BY_APPEND;
      pw->reserved = (void*)pline; // используем зарезервированный указатель
    }
    break;

  case SE_APPEND:
    if (pw->isLocked != LOCKED_BY_APPEND)
      return SEDIT_APPEND_NOT_INITIALIZED;
    else if (!wParam)
      return SEDIT_NO_PARAM;
    else
    {
      //pw->common.isModified = 1;
      if ((pw->reserved = (void*)AddRealLine(pw, (pline_t)pw->reserved, (wchar_t*)wParam, (char)lParam)) == NULL)
        return SEDIT_MEMORY_ERROR;
    }
    break;

  case SE_ENDAPPEND:
    pw->isLocked = 0;
    pw->reserved = NULL;
    SEInvalidateRect(&pw->common, NULL, FALSE);
    SEUpdateWindow(&pw->common, &pw->caret, &pw->firstLine);
    UpdateScrollBars(&pw->common, &pw->caret, &pw->firstLine);
    //DocModified(&pw->common, pw->common.isModified);
    CaretModified(&pw->common, &pw->caret, &pw->firstLine, CARET_LEFT);
    break;

  case SE_BEGINRECEIVE:
    pw->isLocked = LOCKED_BY_RECEIVE;
    pw->reserved = (void*)pw->lists.pfakeHead->next; // используем зарезервированный указатель
    break;

  case SE_GETNEXTLINE:
    if (pw->isLocked != LOCKED_BY_RECEIVE)
      return SEDIT_RECEIVE_NOT_INITIALIZED;
    else if (!wParam && lParam != 0)
      return SEDIT_NO_PARAM;
    else if (!pw->reserved)
      return SEDIT_TEXT_END_REACHED;
    else
    {
      pline_t pline = (pline_t)pw->reserved;
      size_t length = RealStringLength(pline);

      if (!wParam || lParam < (int)(length + 1))
        return length + 1;
      wcscpy((wchar_t*)wParam, pline->prealLine->str);

      while (pline->next && pline->next->prealLine == pline->prealLine)
        pline = pline->next;
      pline = pline->next;
      pw->reserved = (void*)pline;
    }
    break;

  case SE_ENDRECEIVE:
    pw->isLocked = 0;
    pw->reserved = NULL;
    break;

  case SE_STARTUNDOGROUP:
    StartUndoGroup(&pw->undo);
    break;

  case SE_ENDUNDOGROUP:
    EndUndoGroup(&pw->undo);
    break;

  case SE_SETCOLORSFUNC:
    pw->common.di.GetColorsForString = (get_colors_function_t)lParam;
    break;

  case SE_SETBKCOLORSFUNC:
    pw->common.di.GetBkColorsForString = (get_colors_function_t)lParam;
    break;

  case SE_GETCOLORSFUNC:
    return (LRESULT)pw->common.di.GetColorsForString;

  case SE_GETBKCOLORSFUNC:
    return (LRESULT)pw->common.di.GetBkColorsForString;

  case SE_SETPOPUPMENUFUNCS:
    if (!lParam || (((void**)lParam)[0] == 0 && ((void**)lParam)[1] == 0))
    {
      pw->common.di.CreatePopupMenuFunc = NULL;
      pw->common.di.DestroyPopupMenuFunc = NULL;
    }
    else if (((void**)lParam)[0] != 0 && ((void**)lParam)[1] != 0)
    {
      pw->common.di.CreatePopupMenuFunc = (create_popup_menu_function_t)((void**)lParam)[0];
      pw->common.di.DestroyPopupMenuFunc = (destroy_popup_menu_function_t)((void**)lParam)[1];
    }
    else
      return SEDIT_INVALID_PARAM;  // It is error if one function is null and another is not null
    break;

  case SE_GETPOPUPMENUFUNCS:
    if (lParam)
    {
      void** arr = (void**)lParam;
      arr[0] = pw->common.di.CreatePopupMenuFunc;
      arr[1] = pw->common.di.DestroyPopupMenuFunc;
    }
    else
      return SEDIT_NO_PARAM;
    break;

  case SE_SETRECEIVEMSGFUNC:
    pw->common.ReceiveMessageFunc = (receive_message_function_t)lParam;
    break;

  case SE_GETRECEIVEMSGFUNC:
    return (LRESULT)pw->common.ReceiveMessageFunc;

  case SE_GETPOSFROMCOORDS: 
    if (lParam)
    {
      line_info_t resultLine;
      int resultIndex;
      position_t *pposition = (position_t*)lParam;

      GetCaretPosFromCoords(&pw->common, &pw->firstLine, &resultLine, &resultIndex, LOWORD(wParam), HIWORD(wParam));
      
      pposition->y = resultLine.realNumber;
      pposition->x = resultIndex;
    }
    else
      return SEDIT_NO_PARAM;
    break;

  case SE_RESTRICTDRAWING:
    RestrictDrawing(&pw->common, &pw->caret, &pw->firstLine);
    break;

  case SE_ALLOWDRAWING:
    AllowDrawing(&pw->common, &pw->caret, &pw->firstLine);
    break;

  case SE_DEBUG:
#ifdef _DEBUG
    {
      common_info_t *pcommon = &pw->common;
      caret_info_t *pcaret = &pw->caret;
      pselection_t pselection = &pw->selection;
      line_info_t *pfirstLine = &pw->firstLine;
      text_t *plists = &pw->lists;
      unsigned int number;

      if (!plists->pfakeHead->next || !plists->prealHead->next)
        return 1;
      if (!testFakeLines(plists->pfakeHead))
        return 2;
      if (pcommon->nLines != GetLinesNumber(plists->pfakeHead, NULL, 0))
        return 3;
      if (pcommon->nRealLines != GetLinesNumber(pw->lists.pfakeHead, NULL, 1))
        return 4;
      if (pselection->isSelection || IS_SHIFT_PRESSED)
      {
        if (pselection->pos.startLine.pline->begin > pselection->pos.startIndex || pselection->pos.startLine.pline->end < pselection->pos.startIndex)
          return 5;
        if (pselection->pos.endLine.pline->begin > pselection->pos.endIndex || pselection->pos.endLine.pline->end < pselection->pos.endIndex)
          return 5;
      }
      if (pcaret->length != RealStringLength(pcaret->line.pline))
        return 6;
      if (pcaret->line.pline->begin > pcaret->index || pcaret->line.pline->end < pcaret->index)
        return 7;
       
      GetNumbersFromLines(plists->pfakeHead->next, 0, 1, &pcaret->line.pline, &number);
      if (pcaret->line.number != number)
        return 8;

      GetNumbersFromLines(plists->pfakeHead->next, 1, 1, &pcaret->line.pline, &number);
      if (pcaret->line.realNumber != number)
        return 9;

      GetNumbersFromLines(plists->pfakeHead->next, 0, 1, &pfirstLine->pline, &number);
      if (pfirstLine->number != number)
        return 10;

      GetNumbersFromLines(plists->pfakeHead->next, 1, 1, &pfirstLine->pline, &number);
      if (pfirstLine->realNumber != number)
        return 11;
    }
#endif
    break;

  case SE_REPAINT:
    wParam = UPDATE_REPAINT;

  case MM_UPDATE_PARAMS:  
    if (!pw)
      break;

    if (wParam & UPDATE_TEXT)
    {
      HDC hdc;

      hdc = GetDC(hwnd);
      SelectObject(hdc, pw->common.hfont);
      RefreshFontParams(hdc, &pw->common.di);
      UpdateBitmaps(hdc, &pw->common.di, pw->common.clientX, pw->common.di.averageCharY + 2);
      pw->common.nscreenLines = pw->common.clientY / pw->common.di.averageCharY;
      RewrapText(hdc, &pw->common, &pw->caret, &pw->selection, &pw->firstLine, pw->lists.pfakeHead);
      ReleaseDC(hwnd, hdc);
    }

    if (wParam & UPDATE_CARET && pw->caret.type & CARET_EXIST)
    {
      HideCaret(hwnd);
      DestroyCaret();
      CreateCaret(hwnd, (HBITMAP)0, pw->common.di.caretWidth, pw->common.di.averageCharY);
      ShowCaret(hwnd);
      UpdateCaretPos(&pw->common, &pw->caret, &pw->firstLine);
    }

    if (wParam & UPDATE_REPAINT)
    {
      SEInvalidateRect(&pw->common, NULL, FALSE);
      SEUpdateWindow(&pw->common, &pw->caret, &pw->firstLine);
      UpdateScrollBars(&pw->common, &pw->caret, &pw->firstLine);
      if (wParam & UPDATE_TEXT)
        CaretModified(&pw->common, &pw->caret, &pw->firstLine, CARET_RIGHT);
    }
    break;


  case WM_CREATE:
    {
      char error;

      pw = createDocument(hwnd, &error);

      if (error != SEDIT_NO_ERROR)
        return -1;
      SetWindowLongPtr(hwnd, 0, (LONG)pw);
    }
    break;


  case WM_SIZE:
    if (!pw)
      break;
#ifdef _DEBUG
    {
      int error;
      _ASSERTE((error = SendMessage(hwnd, SE_DEBUG, 0, 0)) == SEDIT_NO_ERROR);
    }
#endif
    ResizeDocument(&pw->common, 
                   &pw->caret, 
                   &pw->selection,
                   &pw->firstLine, 
                   pw->lists.pfakeHead,
                   LOWORD(lParam), HIWORD(lParam));
#ifdef _DEBUG
    {
      int error;
      _ASSERTE((error = SendMessage(hwnd, SE_DEBUG, 0, 0)) == SEDIT_NO_ERROR);
    }
#endif
    break;


  case WM_SETFOCUS:
    if (pw != NULL)
    {
      HWND hparentWnd = GetParent(hwnd);

      CreateCaret(hwnd, (HBITMAP)0, pw->common.di.caretWidth, pw->common.di.averageCharY);
      ShowCaret(hwnd);
      pw->caret.type = pw->caret.type | CARET_EXIST;
      UpdateCaretPos(&pw->common, &pw->caret, &pw->firstLine);
      if (hparentWnd)
      {
        sen_focus_changed_t sen;
        sen.header.code = SEN_FOCUSCHANGED;
        sen.header.idFrom = pw->common.id;
        sen.header.hwndFrom = pw->common.hwnd;
        sen.focus = 1;
        SendMessage(hparentWnd, WM_NOTIFY, (WPARAM)pw->common.id, (LPARAM)&sen);
      }
    }    
    break;


  case WM_KILLFOCUS:
    if (pw != NULL)
    {
      HWND hparentWnd = GetParent(hwnd);

      HideCaret(hwnd);
      DestroyCaret();
      pw->caret.type = pw->caret.type & (~CARET_EXIST);
      pw->caret.type &= (~(CARET_VISIBLE | CARET_PARTIALLY_VISIBLE));
      if (hparentWnd)
      {
        sen_focus_changed_t sen;
        sen.header.code = SEN_FOCUSCHANGED;
        sen.header.idFrom = pw->common.id;
        sen.header.hwndFrom = pw->common.hwnd;
        sen.focus = 0;
        SendMessage(hparentWnd, WM_NOTIFY, (WPARAM)pw->common.id, (LPARAM)&sen);
      }
    }
    break;

  
  case WM_KEYDOWN: 
    if (!pw || pw->isLocked)
      break;

    if (pw->mouse.lButtonDown)
      break;  // если пользователь выделяет текст, то никаких действий с клавиатурой не производим

#ifdef _DEBUG
    {
      int error;
      _ASSERTE((error = SendMessage(hwnd, SE_DEBUG, 0, 0)) == SEDIT_NO_ERROR);
    }
#endif

    switch (wParam)
    {
    case VK_SHIFT:
      //pw->selection.shiftPressed = TRUE;
      if (!pw->selection.isSelection)
        StartSelection(&pw->selection, &pw->caret.line, pw->caret.index);
      break;

    case VK_LEFT:
    case VK_RIGHT:
    case VK_UP:
    case VK_DOWN:
    case VK_HOME:
    case VK_END:
    case VK_PRIOR:
    case VK_NEXT:
      GotoCaret(&pw->common, &pw->caret, &pw->firstLine);
      if (!IS_SHIFT_PRESSED && pw->selection.isSelection)
      {
        pw->selection.isSelection = 0;
        UpdateSelection(&pw->common, &pw->caret, &pw->firstLine, &pw->selection, pw->caret.dx);
      }

      switch (wParam)
      {
      case VK_LEFT:
        // нажата клавиша Ctrl и это не самое начало строки
        if (GetKeyState(VK_CONTROL) < 0 && pw->caret.index > 0)
          MoveCaretToPosition(&pw->common, &pw->caret, &pw->firstLine, POS_WORDBEGIN);
        else
          MoveCaretLeft(&pw->common, &pw->caret, &pw->firstLine);
        break;

      case VK_RIGHT:
        // нажата клавиша Ctrl и это не самый конец строки
        if (GetKeyState(VK_CONTROL) < 0 && pw->caret.index < pw->caret.length)
          MoveCaretToPosition(&pw->common, &pw->caret, &pw->firstLine, POS_WORDEND);
        else
          MoveCaretRight(&pw->common, &pw->caret, &pw->firstLine);
        break;

      case VK_UP:
        // нажата клавиша Ctrl
        if (GetKeyState(VK_CONTROL) < 0)
        {
          if (pw->caret.index == 0)
            MoveCaretLeft(&pw->common, &pw->caret, &pw->firstLine);
          MoveCaretToPosition(&pw->common, &pw->caret, &pw->firstLine, POS_REALLINEBEGIN);  
        }
        else
          MoveCaretUp(&pw->common, &pw->caret, &pw->firstLine);
        break;

      case VK_DOWN:
        // нажата клавиша Ctrl
        if (GetKeyState(VK_CONTROL) < 0)
        {
          if (pw->caret.index == pw->caret.length)
            MoveCaretRight(&pw->common, &pw->caret, &pw->firstLine);
          MoveCaretToPosition(&pw->common, &pw->caret, &pw->firstLine, POS_REALLINEEND);
        }
        else
          MoveCaretDown(&pw->common, &pw->caret, &pw->firstLine);
        break;

      case VK_HOME:
        MoveCaretToPosition(&pw->common, &pw->caret, &pw->firstLine, POS_STRINGBEGIN);
        break;

      case VK_END:
        MoveCaretToPosition(&pw->common, &pw->caret, &pw->firstLine, POS_STRINGEND);
        break;

      case VK_PRIOR:  // Page Up
        PageUp(&pw->common, &pw->caret, &pw->firstLine);
        break;

      case VK_NEXT:  // Page Down
        PageDown(&pw->common, &pw->caret, &pw->firstLine);
        break;
      }

      if (IS_SHIFT_PRESSED)
      {
        ChangeSelection(&pw->selection, &pw->caret.line, pw->caret.index);
        UpdateSelection(&pw->common, &pw->caret, &pw->firstLine, &pw->selection, pw->caret.dx);
      }
      break;

    case VK_DELETE:
      if (!pw->selection.isSelection)
      {
        // нажата клавиша Ctrl и каретка не в самом конце абзаца
        if (GetKeyState(VK_CONTROL) < 0 && pw->caret.index < pw->caret.length) 
        {
          line_info_t startLine = pw->caret.line;
          unsigned int si = pw->caret.index;

          MoveCaretToPosition(&pw->common, &pw->caret, &pw->firstLine, POS_WORDEND);
          if (SetSelection(pw, &startLine, &pw->caret.line, si, pw->caret.index, 0))
            DeleteSelection(&pw->common, &pw->caret, &pw->selection, &pw->firstLine, &pw->undo);          
        }
        else
          DeleteNextChar(&pw->common, &pw->caret,  &pw->selection, &pw->firstLine, &pw->undo);
      }
      else
        DeleteSelection(&pw->common, &pw->caret, &pw->selection, &pw->firstLine, &pw->undo);
      break;
    }
#ifdef _DEBUG
    {
      int error;
      _ASSERTE((error = SendMessage(hwnd, SE_DEBUG, 0, 0)) == SEDIT_NO_ERROR);
    }
#endif
    break;


  /*case WM_KEYUP:
    if (pw && !pw->isLocked)
    {
      switch (wParam)
      {
      case VK_SHIFT:
        //pw->selection.shiftPressed = FALSE;
        break;
      }
    }
    break;*/


  case WM_CHAR:
    if (!pw || pw->isLocked || pw->mouse.lButtonDown)
      break;

#ifdef _DEBUG
    {
      int error;
      _ASSERTE((error = SendMessage(hwnd, SE_DEBUG, 0, 0)) == SEDIT_NO_ERROR);
    }
#endif
    
    GotoCaret(&pw->common, &pw->caret, &pw->firstLine);
    DoSmartInputAction(&pw->common, &pw->caret, (wchar_t*)&wParam);
    if (GetKeyState(VK_CONTROL) < 0) // нажата клавиша Ctrl
    {
      int error;

      switch (wParam)
      {
      case QUICK_COPY:
        if ((error = SendMessage(hwnd, SE_COPY, 0, 0)) != SEDIT_NO_ERROR)
          showSEditError(hwnd, error);
        break;

      case QUICK_PASTE:
        if ((error = SendMessage(hwnd, SE_PASTE, 0, 0)) != SEDIT_NO_ERROR)
          showSEditError(hwnd, error);
        break;

      case QUICK_CUT:
        if ((error = SendMessage(hwnd, SE_CUT, 0, 0)) != SEDIT_NO_ERROR)
          showSEditError(hwnd, error);
        break;

      case QUICK_SELECT_ALL:
        if ((error = SendMessage(hwnd, SE_SELECTALL, 0, 0)) != SEDIT_NO_ERROR)
          showSEditError(hwnd, error);
        break;

      case QUICK_UNDO:
        SendMessage(hwnd, SE_UNDO, 0, 0);
        break;

      case QUICK_REDO:
        SendMessage(hwnd, SE_REDO, 0, 0);
        break;

      case QUICK_DELETE_PREV_WORD:
        if (!pw->selection.isSelection)
        {
          if (pw->caret.index > 0)
          {
            line_info_t endLine = pw->caret.line;
            unsigned int ei = pw->caret.index;

            MoveCaretToPosition(&pw->common, &pw->caret, &pw->firstLine, POS_WORDBEGIN);
            if (SetSelection(pw, &pw->caret.line, &endLine, pw->caret.index, ei, 0))
              DeleteSelection(&pw->common, &pw->caret, &pw->selection, &pw->firstLine, &pw->undo);
          }
          else
            DeleteChar(&pw->common, &pw->caret,  &pw->selection, &pw->firstLine, &pw->undo);
        }
        else
          DeleteSelection(&pw->common, &pw->caret, &pw->selection, &pw->firstLine, &pw->undo);
        break;
      }
    }
    else
    {
      switch (wParam)
      {
      case VK_BACK:   // Backspace
        if (!pw->selection.isSelection)
          DeleteChar(&pw->common, &pw->caret,  &pw->selection, &pw->firstLine, &pw->undo);
        else
          DeleteSelection(&pw->common, &pw->caret, &pw->selection, &pw->firstLine, &pw->undo);
        break;

      case VK_RETURN:  // Enter
        if (pw->selection.isSelection)
          DeleteSelection(&pw->common, &pw->caret, &pw->selection, &pw->firstLine, &pw->undo);
        NewLine(&pw->common, &pw->caret, &pw->selection, &pw->firstLine, &pw->undo);
        break;

      default:
        if (pw->selection.isSelection)
          DeleteSelection(&pw->common, &pw->caret, &pw->selection, &pw->firstLine, &pw->undo);
        InputChar(&pw->common, &pw->caret, &pw->selection, &pw->firstLine, &pw->undo, (wchar_t)wParam);
      }
    }

#ifdef _DEBUG
    {
      int error;
      _ASSERTE((error = SendMessage(hwnd, SE_DEBUG, 0, 0)) == SEDIT_NO_ERROR);
    }
#endif
    break;


  case WM_LBUTTONDOWN:
    if (pw && !pw->isLocked)
      MouseLButtonDown(pw, (short)(LOWORD(lParam)), (short)(HIWORD(lParam)));
    break;


  case WM_LBUTTONDBLCLK:
    if (!pw || pw->isLocked)
      break;
    SetCapture(hwnd);
    if (!pw->common.isDragMode && pw->caret.type & CARET_EXIST)
      SelectWordUnderCaret(&pw->common, &pw->caret, &pw->selection, &pw->firstLine);
    pw->mouse.lButtonDown = MOUSE_DOUBLE_CLICK;
    TrackMousePosition(pw, (short)(LOWORD(lParam)), (short)(HIWORD(lParam)));
    break;


  case WM_MOUSEMOVE:
    if (pw && !pw->isLocked)
      MouseMove(pw, (short)(LOWORD(lParam)), (short)(HIWORD(lParam)));
    break;


  case WM_LBUTTONUP:
    if (pw && !pw->isLocked)
    {
      unsigned int currentTime = GetTickCount();
      const mouse_point_t *pmousePoint = GetMouseLastPoint(pw);

      ReleaseCapture();
      pw->mouse.lButtonDown = 0;

      if (pmousePoint && currentTime >= pmousePoint->time && currentTime - pmousePoint->time < MOUSE_EVENT_ROW_TIME)
      {
        UpdateKineticScroll(pw);
        StartKineticScroll(pw);
      }
    }
    break;


#if !defined(_WIN32_WCE)
  case WM_CONTEXTMENU:
  case WM_RBUTTONUP:
    if (!pw || pw->isLocked)
      break;
    if (!(pw->caret.type & CARET_EXIST))
      SetFocus(hwnd);
    ShowContextMenu(pw, LOWORD(lParam), HIWORD(lParam));
    break;


  case WM_MOUSEWHEEL:
    if (!pw || pw->isLocked)
      break;
    MouseWheel(&pw->common, &pw->caret, &pw->firstLine, GET_WHEEL_DELTA_WPARAM(wParam));
    break;
#endif


  case WM_VSCROLL:
    if (pw && !pw->isLocked)
    {
      int newVScrollPos = pw->common.vscrollPos;

      switch(LOWORD(wParam))
      {
      case SB_LINEUP:
        newVScrollPos -= (int)(pw->common.di.averageCharY * VSCROLL_LINE_UPDOWN);
        break;
  
      case SB_LINEDOWN:
        newVScrollPos += (int)(pw->common.di.averageCharY * VSCROLL_LINE_UPDOWN);
        break;

      case SB_PAGEUP:
        newVScrollPos -= (pw->common.clientY > pw->common.di.averageCharY) ? 
          pw->common.clientY - (pw->common.clientY % pw->common.di.averageCharY) - 1 : 
          pw->common.clientY;

        /*RestrictDrawing(&pw->common, &pw->caret, &pw->firstLine);
        HScrollWindow(&pw->common, &pw->caret, &pw->firstLine, pw->common.hscrollPos - pw->common.di.averageCharY * 6);
        VScrollWindow(&pw->common, &pw->caret, &pw->firstLine, pw->common.vscrollPos - pw->common.di.averageCharY * 6);
        AllowDrawing(&pw->common, &pw->caret, &pw->firstLine);
        return 0;*/
        break;

      case SB_PAGEDOWN:
        newVScrollPos += (pw->common.clientY > pw->common.di.averageCharY) ? 
          pw->common.clientY - (pw->common.clientY % pw->common.di.averageCharY) - 1 : 
          pw->common.clientY;

        /*RestrictDrawing(&pw->common, &pw->caret, &pw->firstLine);
        HScrollWindow(&pw->common, &pw->caret, &pw->firstLine, pw->common.hscrollPos + pw->common.di.averageCharY * 6);
        VScrollWindow(&pw->common, &pw->caret, &pw->firstLine, pw->common.vscrollPos + pw->common.di.averageCharY * 6);
        AllowDrawing(&pw->common, &pw->caret, &pw->firstLine);
        return 0;*/
        break;

        case SB_THUMBTRACK:
          newVScrollPos = ((short)HIWORD(wParam)) * pw->common.vscrollCoef;
          break;
      } // end of switch

      if (pw->common.isVScrollByLine)
      {
        int count = GetVScrollCount(&pw->common, &pw->firstLine, newVScrollPos - pw->common.vscrollPos);

        if (LOWORD(wParam) != SB_THUMBTRACK)
        {
          if (newVScrollPos > pw->common.vscrollPos)
            newVScrollPos = vScrollPosLineOnTop(&pw->common, pw->firstLine.number + count + 1);
          else if (newVScrollPos < pw->common.vscrollPos)
            newVScrollPos = vScrollPosLineOnTop(&pw->common, pw->firstLine.number + count);
        }
        else if (newVScrollPos < pw->common.vscrollRange && pw->firstLine.number + pw->common.nscreenLines + 1 < pw->common.nLines)
          newVScrollPos = vScrollPosLineOnTop(&pw->common, pw->firstLine.number + count);
      }

      VScrollWindow(&pw->common, &pw->caret, &pw->firstLine, newVScrollPos);
    }
    break;


  case WM_HSCROLL:
    if (pw && !pw->isLocked)
    {
      int newHScrollPos = pw->common.hscrollPos;

      switch(LOWORD(wParam))
      {
      case SB_LINEUP:
        newHScrollPos -= (int)(pw->common.di.averageCharX * HSCROLL_LINE_UPDOWN);
        break;

      case SB_LINEDOWN:
        newHScrollPos += (int)(pw->common.di.averageCharX * HSCROLL_LINE_UPDOWN);
        break;

      case SB_PAGELEFT:
        newHScrollPos -= pw->common.clientX;
        break;

      case SB_PAGERIGHT:
        newHScrollPos += pw->common.clientX;
        break;

      case SB_THUMBTRACK:
        newHScrollPos = pw->common.hscrollCoef * ((short)HIWORD(wParam));
      } // end of switch

      HScrollWindow(&pw->common, &pw->caret, &pw->firstLine, newHScrollPos);
    }
    break;


  case WM_ERASEBKGND:
    if (pw)
    {
      HBRUSH hbrush = CreateSolidBrush(pw->common.di.bgColor);

      if (hbrush)
      {
        RECT rc;

        GetUpdateRect(hwnd, &rc, 0);
        if (rc.left | rc.right | rc.top | rc.bottom)
        {
          HDC hdc = GetDC(hwnd);
          FillRect(hdc, &rc, hbrush);
          ReleaseDC(hwnd, hdc);
        }
        DeleteObject(hbrush);

        return TRUE;
      }
    }
    break;


  case WM_TIMER:
    if (wParam == KINETIC_TIMER_ID)
      HandleKineticScrollMessage(pw);
    break;

  case WM_PAINT:
    if (pw && !pw->isLocked && !pw->common.di.isDrawingRestricted)
    {
      HDC hdc;
      PAINTSTRUCT ps;   
      HBRUSH hbrush;

      HRGN hregion;
      int retval, numberOfRectangles = 0;
      RGNDATA *pregionData = 0;
      RECT *prect = 0;

      line_info_t currentLine;
      int begin, end, count = 0;
      long int screenDistance = pw->common.vscrollPos;
      char err;

      // prepare region data
      //regionData.rdh.dwSize = sizeof(regionData.rdh);
      //regionData.rdh.iType = RDH_RECTANGLES;
      //regionData.rdh.nRgnSize = 2 * sizeof(RECT);

      hregion = CreateRectRgn(0,0,0,0);
      retval = GetUpdateRgn(hwnd, hregion, 0);      
      if (retval == COMPLEXREGION)
      {
        retval = GetRegionData(hregion, 0, NULL);
        pregionData = (RGNDATA*)calloc(1, retval);
        pregionData->rdh.dwSize = sizeof(RGNDATAHEADER);
        pregionData->rdh.iType = RDH_RECTANGLES;
        GetRegionData(hregion, retval, pregionData);

        numberOfRectangles = pregionData->rdh.nCount;
        prect = (RECT*)pregionData->Buffer;
      }
      DeleteObject(hregion);

      if (pw->common.forValidateRect != SEDIT_RESTRICT_VALIDATE)
        pw->common.forValidateRect = SEDIT_REPAINTED;

      SelectObject(pw->common.di.hlineDC, pw->common.hfont);

      hdc = BeginPaint(hwnd, &ps); 

      if (!numberOfRectangles) 
      {
        numberOfRectangles = 1;
        prect = &ps.rcPaint;
      }

      while (numberOfRectangles-- > 0) 
      {
        begin = GetVScrollCount(&pw->common, &pw->firstLine, prect->top);
        end = GetVScrollCount(&pw->common, &pw->firstLine, prect->bottom) + 1;

        currentLine = pw->firstLine;
        count = 0;

        if (pw->common.nLines - (currentLine.number + 1) >= (unsigned int)begin)
        {
          while (count++ < begin)
            MoveLineDown(currentLine)
          count = end - begin;
        }        

        prect->top = (pw->firstLine.number + begin) * pw->common.di.averageCharY - screenDistance;

        while (count > 0)
        {
          if ((err = DrawLine(pw->common.di.hlineDC, &pw->common, &pw->caret, &currentLine, &pw->selection, prect->left, prect->right, 0)) != SEDIT_NO_ERROR)
          {
            showSEditError(hwnd, err);
            CloseDocument(&pw->common, &pw->caret, &pw->selection, &pw->firstLine, &pw->undo, &pw->lists);            
            EndPaint(hwnd, &ps);
            if (pregionData)
              free(pregionData);
            break;
          }
          BitBlt(hdc, 
            prect->left, prect->top, 
            prect->right - prect->left, pw->common.di.averageCharY,
            pw->common.di.hlineDC, prect->left, 0, SRCCOPY);

          prect->top += pw->common.di.averageCharY;
          --count;

          if (currentLine.pline->next)
            MoveLineDown(currentLine)
          else
            break;
        }

        if ((hbrush = CreateSolidBrush(pw->common.di.bgColor)) == NULL)
        {
          showSEditError(hwnd, SEDIT_CREATE_BRUSH_ERROR);
          EndPaint(hwnd, &ps);
          if (pregionData)
            free(pregionData);
          break;
        }
        ++prect->right;
        ++prect->bottom;
        FillRect(hdc, prect, hbrush);

        DeleteObject(hbrush);

        prect++;
      }
      
      if (pregionData)
        free(pregionData);
      EndPaint(hwnd, &ps);
    }
    else
    {
      PAINTSTRUCT ps;
      HBRUSH hbrush;
      HDC hdc = BeginPaint(hwnd, &ps);

      if ((hbrush = CreateSolidBrush(pw->common.di.bgColor)) == NULL)
      {
        showSEditError(hwnd, SEDIT_CREATE_BRUSH_ERROR);
        EndPaint(hwnd, &ps);
        break;
      }
      FillRect(hdc, &ps.rcPaint, hbrush);

      DeleteObject(hbrush);
      EndPaint(hwnd, &ps);
    }
    break; 


  case WM_DESTROY:
    if (pw)
    {
      deleteDocument(pw);
      SetWindowLongPtr(hwnd, 0, 0);
      pw = 0;
    }
    break;

  default:
	  return DefWindowProc(hwnd, msg, wParam, lParam);  
  }

#ifdef _DEBUG
  if (pw)
  {
    _ASSERTE(pw->selection.isSelection == 0 ||
             (pw->selection.isSelection == 1 &&
             ((pw->selection.pos.startLine.pline != pw->selection.pos.endLine.pline) || 
             (pw->selection.pos.startIndex != pw->selection.pos.endIndex))));
  }
#endif


  return 0;
}


/************************************************************************/
/*                  Simple Edit Control API                             */
/************************************************************************/


extern char SEAPIENTRY SEAPIRegister(HINSTANCE hInstance, COLORREF bgColor)
{
  static HBRUSH hbrush;
  static LPWSTR className = SEDIT_CLASS;
#if !defined(_WIN32_WCE)
  WNDCLASSEX wc;
#else
  WNDCLASS wc;
#endif

  //if ((hbrush = CreateSolidBrush(bgColor)) == NULL)
  //  return SEDIT_CREATE_BRUSH_ERROR;

	ZeroMemory(&wc, sizeof(wc));
  wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
  wc.lpfnWndProc = (WNDPROC)WndProc;
  wc.hInstance = hInstance; 
  wc.cbWndExtra = sizeof(window_t*);
  wc.hCursor = LoadCursor(NULL, IDC_IBEAM);
  //wc.hbrBackground = (HBRUSH)hbrush;
  wc.hbrBackground = NULL;
  wc.lpszClassName = (LPWSTR)className;
#if !defined(_WIN32_WCE)
  wc.cbSize = sizeof(wc);

  if (!RegisterClassEx(&wc))
    return 0;
#else
  if (!RegisterClass(&wc))
    return SEDIT_REGISTER_CLASS_ERROR;
#endif

  g_hinst = hInstance;
  MasSetMemoryFunctions(MemorySystemAllocate, MemorySystemCallocate, MemorySystemFree);

  return SEDIT_NO_ERROR;
}

extern char SEAPIENTRY SEAPIAddBreakSyms(const wchar_t *str, unsigned short length)
{
  if (str)
  {
    char retval = SEDIT_NO_ERROR;
    unsigned short i;
    for (i = 0; i < length; ++i)
      if ((retval = AddBreakSym(str[i])) != SEDIT_NO_ERROR)
        break;
    return retval;
  }
  return SEDIT_NO_PARAM;
}