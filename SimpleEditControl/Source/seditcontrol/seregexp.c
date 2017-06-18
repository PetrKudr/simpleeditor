#include "..\debug.h"
#include "editor.h"
#include "seregexp.h"

#include "..\regexp\regexp.h"

typedef struct tag_regexp_t
{
  regexp *compiled;
  regmatch *matches;
} regexp_t;

// size of the reserved place in structure sedit_regexp_t should be enough for storing structure regexp_t
C_ASSERT(sizeof(regexp_t) == REGEXP_OPAQUE);


extern int RegexpCompile(sedit_regexp_t *regexp, const wchar_t *pattern)
{
  int retval;
  regmatch *matches;

  retval = re_comp_w(&((regexp_t*)(regexp->opaque))->compiled, pattern);
  if (retval < 0)
    return SEDIT_SEARCH_ERROR;

  regexp->nSubExpr = re_nsubexp(((regexp_t*)(regexp->opaque))->compiled);

  matches = malloc(sizeof(regmatch) * regexp->nSubExpr);
  regexp->subExpr = malloc(sizeof(position_t) * regexp->nSubExpr);
  if (matches == NULL || regexp->subExpr == NULL)
  {
    if (matches)
      free(matches);
    re_free(((regexp_t*)(regexp->opaque))->compiled);
    return SEDIT_MEMORY_ERROR;
  }
  ((regexp_t*)(regexp->opaque))->matches = matches;

  return SEDIT_NO_ERROR;
}

extern int RegexpGetMatch(sedit_regexp_t *regexp, const wchar_t *str, unsigned int index)
{
  regexp_t *re = (regexp_t*)regexp->opaque;
  int retval;

  _ASSERTE(re != NULL);

  if (str[0] == L'\0') // движок глючит на пустых строках
    return 0;

  retval = re_exec_w(re->compiled, &str[index], regexp->nSubExpr, re->matches);
  if (retval == 1)
  {
    int i;

    for (i = 0; i < regexp->nSubExpr; i++)
    {
      regexp->subExpr[i].x = index + (unsigned int)re->matches[i].begin;
      regexp->subExpr[i].y = index + (unsigned int)re->matches[i].end;
    }
  }
  else if (retval < 0)
    return SEDIT_SEARCH_ERROR;

  return retval;
}

extern void RegexpFree(sedit_regexp_t *regexp)
{
  re_free(((regexp_t*)(regexp->opaque))->compiled);
  free(((regexp_t*)(regexp->opaque))->matches);
  free(regexp->subExpr);
}


// Function looks down for the next match.
// Returns 1 if success, 0 if nothing was found, <0 for error
extern int RegexpFindDown(sedit_regexp_t *regexp, line_info_t *piLine, unsigned int index, wchar_t *pattern)
{
  int retval;
  pline_t pline = piLine->pline;
  unsigned int number = piLine->number, realNumber = piLine->realNumber;

  while ((retval = RegexpGetMatch(regexp, pline->prealLine->str, index)) == 0)
  {
    pline = pline->next;
    ++number;
    while (pline && pline->prev->prealLine == pline->prealLine)
    {
      pline = pline->next;
      ++number;
    }
    if (!pline)
      break;
    ++realNumber;
    index = 0;
  }
  if (retval == 1)
  {
    piLine->pline = pline;
    piLine->number = number;
    piLine->realNumber = realNumber;
  }

  return retval;
}


// Function returns start position of the subStr in pres structure.
// Return value is 0 if there are no matches, length of the substring if match has found and <0 if error occures
extern int RegexpFindMatchDown(const line_info_t *piStartLine, unsigned int index, wchar_t *pattern, pos_info_t *pres)
{
  sedit_regexp_t regexp;
  int retval;

  if( (retval = RegexpCompile(&regexp, pattern)) != SEDIT_NO_ERROR)
    return retval;

  *pres->piLine = *piStartLine;
  if ((retval = RegexpFindDown(&regexp, pres->piLine, index, pattern)) == 1)
  {
    pres->index = regexp.subExpr[0].x;
    pres->pos = POS_BEGIN;
    UpdateLine(pres);    // Find appropriate line

    retval = regexp.subExpr[0].y - regexp.subExpr[0].x;
  }
  RegexpFree(&regexp);

  return retval;
}

// Function returns start position of the subStr in pres structure.
// Return value is 0 if there are no matches, length of the substring if match has found and <0 if error occures
extern int RegexpFindMatchUp(line_info_t *piStartLine, unsigned int index, wchar_t *pattern, pos_info_t *pres)
{
  regexp* compiled;
  regmatch* matches, lastMatch;
  pline_t pline = piStartLine->pline;
  unsigned int number = piStartLine->number, realNumber = piStartLine->realNumber, i;
  int retval, nSubExpr;

  if ((retval = re_comp_w(&compiled, pattern)) < 0)
    return SEDIT_SEARCH_ERROR;

  _ASSERTE(re_nsubexp(compiled) > 0);
  nSubExpr = re_nsubexp(compiled);
  matches = malloc(sizeof(regmatch) * nSubExpr);
  if (matches == NULL)
  {
    re_free(compiled);
    return SEDIT_MEMORY_ERROR;
  }

  while (pline->prev)
  {
    i = 0;
    lastMatch.begin = lastMatch.end = -1;
    retval = re_exec_w(compiled, pline->prealLine->str, nSubExpr, matches);
    while (retval == 1 && (int)index > matches[0].end)
    {
      _ASSERTE(matches[0].begin < (int)RealStringLength(pline));
      i = matches[0].begin + 1;  
      lastMatch.begin = matches[0].begin;  
      lastMatch.end = matches[0].end;
      retval = re_exec_w(compiled, &pline->prealLine->str[i], nSubExpr, matches);
      if (retval == 1)
      {
        matches[0].begin += i;
        matches[0].end += i;
      }
    } 
    if (retval < 0)
    {
      free(matches);
      re_free(compiled);
      return SEDIT_SEARCH_ERROR;
    }
    else if (retval == 1 && index == matches[0].end)
    {
      lastMatch.begin = matches[0].begin;
      lastMatch.end = matches[0].end;
      break;
    }
    else if (lastMatch.begin > -1) // at least one match has found
      break;
 
    // no matches in the current line
    pline = pline->prev;
    --number;
    while (pline->next->prealLine == pline->prealLine)
    {
      pline = pline->prev;
      --number;
    }
    --realNumber;
    index = pline->end;
  }
  re_free(compiled);

  if (lastMatch.begin > -1)  // we have an appropriate match
  {
    pres->piLine->pline = pline;
    pres->piLine->number = number;
    pres->piLine->realNumber = realNumber;
    pres->index = lastMatch.begin;
    pres->pos = POS_BEGIN;
    UpdateLine(pres);    // Find appropriate line

    retval = lastMatch.end - lastMatch.begin;
  }
  else
    retval = 0;  // no matches
  free(matches);

  return retval;
}

extern int RegexpGetReplaceString(const wchar_t *pattern, const wchar_t *str, wchar_t **result,
                                  const position_t *subExpr, int nSubExpr)
{
  const wchar_t *sym = pattern;
  wchar_t *Replace;
  int size = 0, k, num;

  // We run through the pattern two times:
  // At first time we just count the size of the Replace string;
  // Finally, we copy data to the Replace string
  for (k = 0; k < 2; k++)
  {
    while (*sym)
    {
      if (*sym == L'\\')
      {
        ++sym;
        if (iswdigit(*sym)) // '\X', where X is digit
        {
          num = (int)(*sym - L'0');
          if (num >= nSubExpr)
            return SEDIT_WRONG_REPLACE_PATTERN;

          if (k > 0)
            wcsncpy(&Replace[size], &str[subExpr[num].x], subExpr[num].y - subExpr[num].x);
          size += subExpr[num].y - subExpr[num].x;
        }
        else if (*sym == L'\\')  // '\\' 
        {
          if (k > 0)
            Replace[size] = *sym;
          ++size;
        }
        else  // '\X', where X is neither digit, nor slash
          return SEDIT_WRONG_REPLACE_PATTERN;
      }
      else
      {
        if (k > 0)
          Replace[size] = *sym;
        ++size;
      }
      ++sym;
    }

    if (!result)  // we should just count size of the string
      return size;

    if (k == 0)
    {
      if ((Replace = malloc(sizeof(wchar_t) * (size + 1))) == NULL)
        return SEDIT_MEMORY_ERROR;
      size = 0;
      sym = pattern;
      *result = Replace;
    }
    Replace[size] = L'\0';
  }

  return size;
}
