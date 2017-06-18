#ifndef mouse_h__
#define mouse_h__

#include "types.h"


extern void TrackMousePosition(window_t *pw, short int x, short int y);

extern void GetMouseMoveVelocity(const window_t *pw, float *pxVelocity, float *pyVelocity);

extern const mouse_point_t* GetMouseLastPoint(const window_t *pw);

extern int GetMouseHistorySize(const window_t *pw);

extern const mouse_point_t* GetMouseHistoryPoint(const window_t *pw, int prev);

#endif // mouse_h__