#include "globals.h"

extern HINSTANCE g_hinst = NULL;

extern unsigned int g_unblocked[LOCK_TYPES][MAX_UNBLOCKED] = {{2, SE_APPEND, SE_ENDAPPEND},
                                                              {2, SE_GETNEXTLINE, SE_ENDRECEIVE}};

