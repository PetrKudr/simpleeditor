#ifndef kinetic_h__
#define kinetic_h__

#include "types.h"


extern void StartKineticScroll(window_t *pw);

extern void UpdateKineticScroll(window_t *pw);

extern void HandleKineticScrollMessage(window_t *pw);

extern void StopKinecticScroll(window_t *pw);


#endif // kinetic_h__