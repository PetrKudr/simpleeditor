#ifndef __SMART_TEXT_
#define __SMART_TEXT_

#include "types.h"

#define RESTRICT_SMART_INPUT(pcommon) pcommon->action |= ACTION_IN_PROGRESS
#define ALLOW_SMART_INPUT(pcommon)    pcommon->action &= (~ACTION_IN_PROGRESS)

#define IS_SHIFT_PRESSED (GetKeyState(VK_SHIFT) < 0 ? 1 : 0)

extern void GetActionForSmartInput(common_info_t *pcommon, caret_info_t *pcaret, undo_info_t *pundo);
extern void DoSmartInputAction(common_info_t *pcommon, caret_info_t *pcaret, wchar_t *ch);
extern void ActionOnCaretModified(common_info_t *pcommon);

extern char AddBreakSym(wchar_t sym);
extern char IsBreakSym(wchar_t sym);

#endif