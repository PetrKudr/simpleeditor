#include "seditfunctions.h"
#include "editor.h"


extern int SEditGetRange(__in window_t *pw, __in char type, __in const range_t *prange, __in size_t buffSize, __out wchar_t *buff)
{
  if (prange && pw != NULL && !pw->isLocked)
  {
    pline_t pstartLine = 0, pendLine = 0;
    unsigned int firstLineNumber, lastLineNumber;
    size_t size = 0;
    range_t range = *prange;

    SortRange(&range);
    firstLineNumber = range.start.y;
    lastLineNumber = range.end.y;

    if (lastLineNumber < firstLineNumber)
      return SEDIT_INVALID_PARAM;

    if (type == SEDIT_VISUAL_LINE)
    {
      if (lastLineNumber < pw->common.nLines)
      {
        pline_t pstartLine = GetLineFromVisualPos(GetNearestVisualLine(&pw->caret, &pw->firstLine, &pw->lists, firstLineNumber), firstLineNumber, NULL);
        pline_t pendLine = GetLineFromVisualPos(GetNearestVisualLine(&pw->caret, &pw->firstLine, &pw->lists, lastLineNumber), lastLineNumber, NULL);

        if (range.start.x < pstartLine->begin || range.start.x > pstartLine->end) 
          return SEDIT_INVALID_PARAM;
        if (range.end.x < pendLine->begin || range.end.x > pendLine->end) 
          return SEDIT_INVALID_PARAM;
      }
      else
        return SEDIT_INVALID_PARAM;
    }
    else
    {
      if (lastLineNumber < pw->common.nRealLines)
      {
        pstartLine = GetLineFromRealPos(GetNearestRealLine(&pw->caret, &pw->firstLine, &pw->lists, firstLineNumber), firstLineNumber, range.start.x, NULL);
        pendLine = GetLineFromRealPos(GetNearestRealLine(&pw->caret, &pw->firstLine, &pw->lists, lastLineNumber), lastLineNumber, range.end.x, NULL);

        if (range.start.x > RealStringLength(pstartLine)) 
          return SEDIT_INVALID_PARAM;
        if (range.end.x > RealStringLength(pendLine)) 
          return SEDIT_INVALID_PARAM;
      }
      else
        return SEDIT_INVALID_PARAM;
    }

    size = GetTextSize(pstartLine, range.start.x, pendLine, range.end.x) + 1;
    if (buffSize < size) 
      return size;

    return CopyTextToBuffer(pstartLine, range.start.x, pendLine, range.end.x, buff);
  } 

  if (NULL != pw && pw->isLocked)
    return SEDIT_WINDOW_IS_BLOCKED;

  return SEDIT_NO_PARAM;
}

extern int SEditGetPositionFromOffset(__in window_t *pw, __in unsigned int offset, __out position_t *ppos)
{
  if (!pw->isLocked && ppos)
  {
    line_info_t lineInfo = pw->lists.textFirstLine;
    unsigned int currentOffset = 0;

    while (lineInfo.pline->next && currentOffset + (lineInfo.pline->end - lineInfo.pline->begin) + lineInfo.realNumber * CRLF_SIZE < offset)
    {
      currentOffset += lineInfo.pline->end - lineInfo.pline->begin;
      MoveLineDown(lineInfo);
    }

    currentOffset += CRLF_SIZE * lineInfo.realNumber;

    if (currentOffset <= offset)
    {
      ppos->y = lineInfo.realNumber;
      ppos->x = min(lineInfo.pline->begin + (offset - currentOffset), lineInfo.pline->end);
    }
    else 
    {
      // this could happen if for some reason offset is inside CRLF
      ppos->y = lineInfo.realNumber - 1;
      ppos->x = lineInfo.pline->prev->end;
    }
  }
  else
  {
    if (pw->isLocked)
      return SEDIT_WINDOW_IS_BLOCKED;

    if (!ppos)
      return SEDIT_NO_PARAM;
  }

  return SEDIT_NO_ERROR;
}

extern int SEditGetOffsetFromPosition(__in window_t *pw, __in position_t *ppos, __out unsigned int *poffset)
{
  if (!pw->isLocked && ppos && poffset)
  {
    pline_t pline = GetLineFromRealPos(GetNearestRealLine(&pw->caret, &pw->firstLine, &pw->lists, ppos->y), ppos->y, ppos->x, NULL);
    *poffset = GetTextSize(pw->lists.pfakeHead->next, 0, pline, min(ppos->x, pline->end));
  }
  else
  {
    if (pw->isLocked)
      return SEDIT_WINDOW_IS_BLOCKED;

    if (!ppos || !poffset)
      return SEDIT_NO_PARAM;
  }

  return SEDIT_NO_ERROR;
}