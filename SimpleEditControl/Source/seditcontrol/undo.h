#ifndef __ACTION_
#define __ACTION_

#include "types.h"

extern char InitUndo(undo_info_t *pundo, size_t size);
extern void DestroyUndo(undo_info_t *pundo);
extern void ClearUndo(undo_info_t *pundo);
extern void RemoveUnmodAction(undo_info_t *pundo); 
extern char IsUndoAvailable(undo_info_t *pundo);
extern char IsRedoAvailable(undo_info_t *pundo);
extern void StartUndoGroup(undo_info_t *pundo);
extern void EndUndoGroup(undo_info_t *pundo);

extern void NextCell(undo_info_t *pundo, char actionType, char isDocModified);

extern char PutSymbol(undo_info_t *pundo, unsigned int number, unsigned int index, wchar_t sym);

extern char PutText(undo_info_t *pundo,
                    line_info_t *piStartLine, unsigned int si,
                    line_info_t *piEndLine, unsigned int ei);

extern char PutArea(undo_info_t *pundo, 
                    unsigned int sNumber, unsigned int sIndex,
                    unsigned int eNumber, unsigned int eIndex);

extern char PutFindReplaceText(undo_info_t *pundo, const wchar_t *Find, const wchar_t *Replace, char useRegexp);
extern char PutReplacePos(undo_info_t *pundo, const wchar_t *str, unsigned int len, position_t *pos);

extern char UndoOperation(undo_info_t *pundo, common_info_t *pcommon, caret_info_t *pcaret, 
                          line_info_t *pfirstLine, selection_t *psel, text_t *ptext);
extern char RedoOperation(undo_info_t *pundo, common_info_t *pcommon, caret_info_t *pcaret, 
                          line_info_t *pfirstLine, selection_t *psel, text_t *ptext);

#define UNDO_ALLOW_SAVE(pundo) (pundo)->isSaveAllowed = 1
#define UNDO_RESTRICT_SAVE(pundo) (pundo)->isSaveAllowed = 0

#endif /* __ACTION_ */