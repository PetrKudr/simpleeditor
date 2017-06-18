#ifndef seditfunctions_h__
#define seditfunctions_h__

#include "types.h"


extern int SEditGetRange(__in window_t *pw, __in char type, __in const range_t *prange, __in size_t buffSize, __out wchar_t *buff);

extern int SEditGetPositionFromOffset(__in window_t *pw, __in unsigned int offset, __out position_t *ppos);

extern int SEditGetOffsetFromPosition(__in window_t *pw, __in position_t *ppos, __out unsigned int *poffset);


#endif // seditfunctions_h__