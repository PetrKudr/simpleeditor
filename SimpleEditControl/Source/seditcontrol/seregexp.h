/* This module provides interface with the regexp engine        */
/* Simple Edit Control uses the Henry Spencer's Regexp Engine.  */
/* Read the readme file for more information                    */

#ifndef __SEDIT_REGEXP_
#define __SEDIT_REGEXP_

#include "types.h"

extern int RegexpCompile(sedit_regexp_t *regexp, const wchar_t *pattern);
extern int RegexpGetMatch(sedit_regexp_t *regexp, const wchar_t *str, unsigned int index);
extern void RegexpFree(sedit_regexp_t *regexp);
extern int RegexpFindDown(sedit_regexp_t *regexp, line_info_t *piLine, unsigned int index, wchar_t *pattern);

extern int RegexpFindMatchDown(const line_info_t *piStartLine, unsigned int index, wchar_t *pattern, pos_info_t *pres);
extern int RegexpFindMatchUp(line_info_t *piStartLine, unsigned int index, wchar_t *pattern, pos_info_t *pres);
extern int RegexpGetReplaceString(const wchar_t *pattern, const wchar_t *str, wchar_t **result,
                                  const position_t *subExpr, int nSubExpr);

#endif