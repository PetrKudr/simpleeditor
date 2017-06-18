#ifndef __SELECTION_
#define __SELECTION_

#include "types.h"

extern void RestrictSelection();
extern void AllowSelection();

extern void StartSelection(pselection_t pselection, line_info_t *piLine, unsigned int index);
extern void ChangeSelection(pselection_t pselection, line_info_t *piLine, unsigned int index);
extern void UpdateSelection(common_info_t *pcommon, caret_info_t *pcaret, line_info_t *pfirstLine, 
                            pselection_t pselection, int *dx);
extern char GetSelectionBorders(pselection_t psel, 
                                line_info_t *piline, 
                                unsigned int begin, 
                                unsigned int end, 
                                unsigned int *b, 
                                unsigned int *e);

extern void GotoSelection(common_info_t *pcommon, caret_info_t *pcaret, pselection_t psel, line_info_t *pfirstLine); 
extern void SelectWordUnderCaret(common_info_t *pcommon, caret_info_t *pcaret, pselection_t psel, line_info_t *pfirstLine);
extern void SelectAll(common_info_t *pcommon, caret_info_t *pcaret, pline_t pfakeHead, 
                      pselection_t psel, line_info_t *pfirstLine);

extern int CopySelectionToBuffer(pselection_t psel, wchar_t *buff, int size);
extern char CopySelectionToClipboard(HWND hwnd, pselection_t psel);
extern char CompareSelection(pselection_t psel, wchar_t *source, int flags);
extern char CompareSelectionRegexp(sedit_regexp_t *regexp, pselection_t psel, wchar_t *pattern);
extern void DeleteSelection(common_info_t *pcommon, caret_info_t *pcaret, pselection_t psel, 
                            line_info_t *pfirstLine, undo_info_t *pundo);

extern char SetSelection(window_t *pw, line_info_t *piStart, line_info_t *piEnd, 
                         unsigned int si, unsigned int ei, char updateScreen);

#define ClearSavedSel(pselection) (pselection)->isOldSelection = 0

#endif /* __SELECTION_ */