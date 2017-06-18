#ifndef __EDITOR_
#define __EDITOR_

#include "types.h"
#include "globals.h"
#include "smarttext.h"

/*************************************************************/
/*                   Functions for text processing           */
/*************************************************************/

/* Inserts substring 'str' into pline in position 'index'
 * PARAMETERS:
 *   pline - destination visual line
 *   length - length of the real line from pline
 *   index - position to for inserting (must be inside pline)
 *   str - substring
 *   n - number of symbols to insert from str
 *
 * RETURN: Number of visual lines after pline which belong to the similar real line
 */
extern unsigned int InsertSubString(pline_t pline, unsigned int length, unsigned int index, wchar_t *str, unsigned int n);

/* Deletes substring
 * PARAMETERS:
 *   pline - destination line
 *   length - length of the real line on which pline is based
 *   index - position for deleting (must be inside pline)
 *   n - number of symbols to delete
 *
 * RETURN: Number of visual lines after pline which belong to the similar real line
 */
extern unsigned int DeleteSubString(pline_t pline, unsigned int length, unsigned int index, unsigned int n);

/* Creates list of real lines
 * PARAMETERS:
 *   head - container for the list
 */ 
extern char InitRealLineList(preal_line_t *head);

/* Deletes list of real lines
 * PARAMETERS:
 *   head - list
 */ 
extern void DestroyRealLineList(preal_line_t head);

/* Creates list of visual lines
 * PARAMETERS:
 *   head - container for the list
 */ 
extern char InitLineList(pline_t *head);

/* Deletes list of visual lines
 * PARAMETERS:
 *   head - list
 */ 
extern void DestroyLineList(pline_t head);

/* Unites all visual lines after pline
 * PARAMETERS:
 *   psLine - line from which unite should be started
 *
 * RETURN: number of removed visual lines;
 */ 
extern int UniteLine(pline_t psLine);

/* Function reduces (wraps) visual lines after the given line (including it) if wrap is on.
 * Affects on lines which belongs to the same real line with the given visual line
 * PARAMETERS:
 *   hdc - device context
 *   pdi - common editor's display structure
 *   pline - visual line to start reducing
 *   maxLength - max length of the visual line in pixels
 *   processed - variable which will contain number of processed lines
 *   added - variable which will contain number of added lines
 * 
 * RETURN: pointer on the first visual line after all processed.
 */
extern pline_t ReduceLine(__in HDC hdc, __inout display_info_t *pdi, __inout pline_t pline, __in long maxLength, __out int *processed, __out int *added);

/* Function enlarges (wraps) visual lines after the given line (including it) if wrap is on.
 * Affects on lines which belongs to the same real line with the given visual line
 * PARAMETERS:
 *   hdc - device context
 *   pdi - common editor's display structure
 *   pline - visual line to start enlarging
 *   maxLength - max length of the visual line in pixels
 *   count - variable which will contain number of removed visual lines, 0 if no one line has been removed or -1, if nothing has been changed.
 * 
 * RETURN: pointer on the first visual line after all processed.
 */
extern pline_t EnlargeLine(__in HDC hdc, __inout display_info_t *pdi, __inout pline_t pline, __in long maxLength, __out int *count);

/* Function combines effects functions of ReduceLine and EnlargeLine. It processes the given visual line in most proper way.
 * PARAMETERS:
 *   hdc - device context
 *   pdi - common editor's display structure
 *   psLine - visual line to start processing
 *   maxLength - max length of the visual line in pixels
 * 
 * RETURN: difference between old and new numbers of visual lines from the given one to the end of it's real line
 */
extern int ProcessLine(HDC hdc, display_info_t *pdi, pline_t psLine, long maxLength);

/* Processes all visual lines from the given to the end of text (works like ReduceLine or EnlargeLine but for all text)
 * PARAMETERS:
 *   hdc - device context
 *   pdi - common editor's display structure
 *   pfakeHead - visual line to start processing
 *   prepareType - type of processing (REDUCE or ENLARGE)
 *   maxLength - max length of the visual line in pixels
 * 
 * RETURN: number of visual lines from the given to the end of text after processing
 */
extern unsigned int PrepareLines(HDC hdc, display_info_t *pdi, pline_t pfakeHead, char prepareType, long maxLength);

/* 
 * PARAMETERS:
 *   pline - visual line
 * 
 * RETURN: length of the real line under the given visual line
 */
extern size_t RealStringLength(pline_t pline);

/* Calculates total lines number from the given visual line (without it)
 * PARAMETERS:
 *   pline - visual line
 *   piLine - pointer line_info_t. Only for optimization, can be NULL
 *   type - 1 means visual lines, 0 means real lines.
 * 
 * RETURN: number of real/visual lines after the given visual line
 */
extern unsigned int GetLinesNumber(pline_t pline, line_info_t *piLine, char type);

/* Finds number of the given lines since the start visual line.
 * PARAMETERS:
 *   pstartLine - visual line from which search will be started
 *   type - 0 means visual lines, 1 means real lines
 *   size - number of array with visual lines
 *   plines - array of visual lines
 *   numbers - array for results
 * 
 * RETURN: 1, if all specified lines have been found, 0 otherwise
 */
extern char GetNumbersFromLines(__in pline_t pstartLine, __in char type, __in int size, __in pline_t *plines, __out unsigned int *numbers);

/* Finds visual line from the number of real line and index in it.
 * PARAMETERS:
 *   piStartLine - line from which search will be started
 *   realNumber - number of real line
 *   index - index in real line
 *   number - variable for number of found visual line. Can be NULL
 * 
 * RETURN: visual line
 */
extern pline_t GetLineFromRealPos(__in const line_info_t *piStartLine, 
                                  __in unsigned int realNumber, 
                                  __in unsigned int index, 
                                  __out unsigned int *number);

/* Finds visual line from its number.
 * PARAMETERS:
 *   piStartLine - line from which search will be started
 *   number - number of visual line
 *   prealNumber - variable for number of real line under found visual line. Can be NULL
 * 
 * RETURN: visual line
 */
extern pline_t GetLineFromVisualPos(__in const line_info_t *piStartLine, __in unsigned int number, __out unsigned int *prealNumber);

/* Converts real position to visual
 * PARAMETERS:
 *   piStartLine - line from which search will be started
 *   ppos - real position
 * 
 * RETURN: visual line which corresponds to the given real position
 */
extern pline_t ConvertRealPosToVisual(__in const line_info_t *piStartLine, __inout position_t *ppos);

/* Finds nearest line to the specified number of visual line.
 * PARAMETERS:
 *   pcaret - caret
 *   pfirstLine - first line
 *   ptext - text
 *   number - number of visual line
 * 
 * RETURN: nearest line
 */
extern line_info_t* GetNearestVisualLine(caret_info_t *pcaret, line_info_t *pfirstLine, text_t *ptext, unsigned int number);

/* Finds nearest line to the specified number of real line.
 * PARAMETERS:
 *   pcaret - caret
 *   pfirstLine - first line
 *   ptext - text
 *   number - number of real line
 * 
 * RETURN: nearest line
 */
extern line_info_t* GetNearestRealLine(caret_info_t *pcaret, line_info_t *pfirstLine, text_t *ptext, unsigned int number);

/* Sets ppos->piLine to the first visual line which corresponds to the current real line for ppos->piLine.
 * PARAMETERS:
 *   ppos - position. Must be filled

 * RETURN: number of visual lines from new position to old position. NOTE: function does not change index in ppos.
 */
extern unsigned int SetStartLine(pos_info_t *ppos);

/* Updates ppos->piLine using the ppos->index (and ppos->pos).
 * PARAMETERS:
 *   ppos - position. Must be filled

 * RETURN: 1 if ppos->index exists in real line under the ppos->piLine, 0 otherwise
 */
extern char UpdateLine(pos_info_t *ppos);

/* (Inserts new string after / Replaces with the new string) real line under the given visual line.
 * NOTE: First line and caret line must not be bigger than real line of the given visual line. (firstLine.realNumber <= realNumber && caret.line.realNumber <= realNumber)
 * PARAMETERS:
 *   pw - window information
 *   pline - pointer to the visual line
 *   str - new string
 *   type - SEDIT_APPEND/SEDIT_REPLACE
 *
 * RETURN: first visual line of the inserted real line.
 */ 
extern pline_t AddRealLine(window_t *pw, pline_t pline, const wchar_t *str, char type);

/* Iterates visual line up.
 * PARAMETERS:
 *   infoLine - line_info_t
 */ 
#define MoveLineUp(infoLine)                                               \
{                                                                          \
  if ((infoLine).pline->prev)                                              \
  {                                                                        \
    (infoLine).pline = (infoLine).pline->prev;                             \
    (infoLine).number--;                                                   \
    if ((infoLine).pline->next->prealLine != (infoLine).pline->prealLine)  \
     (infoLine).realNumber--;                                              \
  }                                                                        \
}

/* Iterates visual line down.
 * PARAMETERS:
 *   infoLine - line_info_t
 */ 
#define MoveLineDown(infoLine)                                              \
{                                                                           \
  if ((infoLine).pline->next)                                               \
  {                                                                         \
    (infoLine).pline = (infoLine).pline->next;                              \
    (infoLine).number++;                                                    \
    if ((infoLine).pline->prev->prealLine != (infoLine).pline->prealLine)   \
      (infoLine).realNumber++;                                              \
  }                                                                         \
}



/*************************************************************/
/*                   Работа с окном                          */
/*************************************************************/

extern char SetEmptyDocument(text_t *ptext, __out int *pnWrappedLines, __out int *pnRealLines);
extern unsigned int UniteLines(pline_t head);
extern int GetVScrollCount(common_info_t *pcommon, line_info_t *pfirstLine, long int diff);
extern int GetVertScrollPos(common_info_t *pcommon, line_info_t *pfirstLine, unsigned int number);
extern int GetHorzScrollPos(HDC hdc, common_info_t *pcommon, pline_t pline, unsigned int index, int *dx);
extern void VScrollWindow(common_info_t *pcommon, caret_info_t *pcaret, line_info_t *pfirstLine, int newPos);
extern void HScrollWindow(common_info_t *pcommon, caret_info_t *pcaret, line_info_t *pfirstLine, int newPos);
//extern void VHScrollWindow(common_info_t *pcommon, caret_info_t *pcaret, line_info_t *pfirstLine, 
//                          int hScrollPos, int vScrollPos);
extern void UpdateScrollBars(common_info_t *pcommon, caret_info_t *pcaret, line_info_t *pfirstLine);

extern void SetMainPositions(common_info_t *pcommon, 
                             caret_info_t *pcaret, 
                             line_info_t *pfirstLine, 
                             pselection_t pselection, 
                             doc_info_t *pdoc);

extern void ResizeDocument(common_info_t *pcommon, 
                           caret_info_t *pcaret, 
                           pselection_t pselection,
                           line_info_t *pfirstLine,
                           pline_t pfakeHead,
                           long clientX,
                           long clientY);

extern void RewrapText(HDC hdc,
                       common_info_t *pcommon, 
                       caret_info_t *pcaret, 
                       pselection_t pselection,
                       line_info_t *pfirstLine,
                       pline_t pfakeHead);

extern void ChangeWordWrap(common_info_t *pcommon, 
                           caret_info_t *pcaret,
                           pselection_t pselection,
                           text_t *plists,
                           line_info_t *pfirstLine);

extern void CloseDocument(common_info_t *pcommon, 
                          caret_info_t *pcaret, 
                          pselection_t pselection,
                          line_info_t *pfirstLine,
                          undo_info_t *pundo,
                          text_t *ptext);

extern void DocModified(common_info_t *pcommon, unsigned int action, const range_t *prange);
extern void DocUpdated(common_info_t *pcommon, unsigned int action, const range_t *prange, char isModified);

extern void MouseLButtonDown(window_t *pw, short int xCoord, short int yCoord);
extern void MouseMove(window_t *pw, short int xCoord, short int yCoord);

#if !defined(_WIN32_WCE) && defined(WHEEL_DELTA)
extern void MouseWheel(common_info_t *pcommon, caret_info_t *pcaret, line_info_t *pfirstLine, int delta);
#endif

// определяет позицию по Y, чтобы сверху экрана была строка number
#define vScrollPosLineOnTop(pcommon, number) (LINE_DISTANCE(pcommon, number))


/*************************************************************/
/*                   Работа с кареткой                       */
/*************************************************************/

extern void GotoCaret(common_info_t *pcommon, caret_info_t *pcaret, line_info_t *pfirstLine);

extern void GetCaretPosFromCoords(common_info_t *pcommon, 
                                  line_info_t *pfirstLine,
                                  line_info_t *presultPosition,
                                  int *pindex,
                                  long x, 
                                  long y);

extern void SetCaretPosFromCoords(common_info_t *pcommon, 
                                  caret_info_t *pcaret,
                                  line_info_t *pfirstLine, 
                                  long x, 
                                  long y);

extern void MoveCaret(common_info_t *pcommon, caret_info_t *pcaret, line_info_t *pfirstLine, unsigned int n, char dir);
extern void MoveCaretLeft(common_info_t *pcommon, caret_info_t *pcaret, line_info_t *pfirstLine);
extern void MoveCaretRight(common_info_t *pcommon, caret_info_t *pcaret, line_info_t *pfirstLine);
extern void MoveCaretUp(common_info_t *pcommon, caret_info_t *pcaret, line_info_t  *pfirstLine);
extern void MoveCaretDown(common_info_t *pcommon, caret_info_t *pcaret, line_info_t  *pfirstLine);
extern void MoveCaretToPosition(common_info_t *pcommon, caret_info_t *pcaret, line_info_t *pfirstLine, char position);
extern void PageUp(common_info_t *pcommon, caret_info_t *pcaret, line_info_t *pfirstLine);
extern void PageDown(common_info_t *pcommon, caret_info_t *pcaret, line_info_t *pfirstLine);

extern void UpdateCaretPos(common_info_t *pcommon, caret_info_t *pcaret, line_info_t *pfirstLine);
extern void UpdateCaretSizes(HDC hdc, common_info_t *pcommon, caret_info_t *pcaret);
extern void CaretModified(common_info_t *pcommon, caret_info_t *pcaret, line_info_t *pfirstLine, char direction);


/*************************************************************/
/*                   Отображение текста                      */
/*************************************************************/

#define SCREEN_DISTANCE(pcommon) ((long int)((pcommon)->vscrollPos))
#define LINE_DISTANCE(pcommon, number) ((long int)((number) * (pcommon)->di.averageCharY))
#define LINE_Y(pcommon, number) ((long int)(LINE_DISTANCE(pcommon, number) - SCREEN_DISTANCE(pcommon)))

extern void RefreshFontParams(HDC hdc, display_info_t *pdi);
extern char UpdateBitmaps(HDC hdc, display_info_t *pdi, long int width, long int height);
extern unsigned long GetStringSizeX(HDC hdc, display_info_t *pdi, wchar_t *str, 
                                    unsigned int begin, unsigned int end, int *dx);
extern unsigned long GetMaxStringSizeX(HDC hdc, display_info_t *pdi, pline_t pline);
extern void RedrawCaretLine(common_info_t *pcommon, caret_info_t *pcaret, line_info_t *pfirstLine, line_info_t *piLine);
extern char DrawLine(HDC hdc, 
                     common_info_t *pcommon,
                     caret_info_t *pcaret, 
                     line_info_t *piLine,
                     pselection_t psel,
                     long int _startX,
                     long int _endX,
                     long int y
                     /*const HBRUSH *hbrushes*/);


/*************************************************************/
/*                   Редактирование                          */
/*************************************************************/

extern size_t GetTextSize(pline_t psLine, unsigned int si, pline_t peLine, unsigned int ei);
extern char CopyTextToBuffer(pline_t psLine, unsigned int si, pline_t peLine, unsigned int ei, wchar_t *buff);
extern unsigned int ParseTextInMemory(wchar_t *text, pline_t *pstartLine, preal_line_t *pstartRealLine);
extern void Paste(HDC hdc, common_info_t *pcommon, caret_info_t *pcaret, line_info_t *pfirstLine,  undo_info_t *pundo,
                  pline_t pnewLine, preal_line_t pnewRealLine, unsigned int count, RECT *prect, range_t *prange);
extern void DeleteArea(HDC hdc, common_info_t *pcommon, line_info_t *pfirstLine,
                      line_info_t *piStartLine, unsigned int si, line_info_t *piEndLine, unsigned int ei, RECT *rect);

extern void NewLine(common_info_t *pcommon, caret_info_t *pcaret, pselection_t psel,
                    line_info_t *pfirstLine, undo_info_t *pundo);
extern void InputChar(common_info_t *pcommon, caret_info_t *pcaret, pselection_t psel,
                      line_info_t *pfirstLine, undo_info_t *pundo, wchar_t sym);
extern void DeleteChar(common_info_t *pcommon, caret_info_t *pcaret, pselection_t psel,
                       line_info_t *pfirstLine, undo_info_t *pundo);
extern void DeleteNextChar(common_info_t *pcommon, caret_info_t *pcaret, pselection_t psel, 
                           line_info_t *pfirstLine, undo_info_t *pundo);
extern char PasteFromBuffer(common_info_t *pcommon, caret_info_t *pcaret, pselection_t psel,
                            line_info_t *pfirstLine, undo_info_t *pundo, wchar_t *buff);
extern void PasteFromClipboard(common_info_t *pcommon, caret_info_t *pcaret, pselection_t psel,
                               line_info_t *pfirstLine, undo_info_t *pundo);


/*************************************************************/
/*                       Разное                              */
/*************************************************************/

extern void SortRange(range_t *prange);

extern void RestrictDrawing(common_info_t *pcommon, caret_info_t *pcaret, line_info_t *pfirstLine);
extern void AllowDrawing(common_info_t *pcommon, caret_info_t *pcaret, line_info_t *pfirstLine);

extern void SEInvalidateRect(common_info_t *pcommon, const RECT *prect, int bErase);
extern char SEInvalidateRange(window_t *pwindow, const range_t *prange, char posType);
extern void SEUpdateWindow(common_info_t *pcommon, caret_info_t *pcaret, line_info_t *pfirstLine);
extern char SetOptions(common_info_t *pcommon, options_t *popt);
extern void ShowContextMenu(window_t *pw, int x, int y);
extern char InitEditor(HWND hwnd, common_info_t *pcommon, caret_info_t *pcaret, line_info_t *firstLine, undo_info_t *pundo,
                       pselection_t pselection, text_t *ptext, pos_info_t *psearchPos);
extern char DestroyEditor(common_info_t *pcommon, caret_info_t *pcaret, undo_info_t *pundo,
                          pline_t pfakeHead, preal_line_t prealHead);

#endif /* __EDITOR_ */