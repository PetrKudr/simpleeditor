#ifndef __FIND_
#define __FIND_

#include "types.h"

#define FIND_MATCH_CASE 1
#define FIND_WHOLE_WORD 2

extern char Find(LPFINDREPLACE pfr, common_info_t *pcommon, caret_info_t *pcaret, pos_info_t *psp, 
                 pselection_t psel, line_info_t *pfirstLine);
extern char Replace(LPFINDREPLACE pfr, common_info_t *pcommon, caret_info_t *pcaret, undo_info_t *pundo,
                    pos_info_t *psp, pselection_t psel, line_info_t *pfirstLine);
extern int ReplaceAll(LPFINDREPLACE pfr, common_info_t *pcommon, caret_info_t *pcaret, pselection_t psel,
                      line_info_t *pfirstLine, undo_info_t *pundo, pline_t head);

#endif /* __FIND */
