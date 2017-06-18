#include "kinetic.h"
#include "globals.h"
#include "mouse.h"
#include "editor.h"

#include <Windows.h>


extern void StartKineticScroll(window_t *pw)
{
  if (pw->common.isDragMode && (pw->common.di.xKineticScrollVelocity || pw->common.di.yKineticScrollVelocity) && !pw->common.di.kineticScrollActivated)
  {
    SetTimer(pw->common.hwnd, KINETIC_TIMER_ID, KINETIC_TIMER_TIME, NULL);
    pw->common.di.kineticScrollActivated = 1;        
  }
}

extern void UpdateKineticScroll(window_t *pw)
{
  // Set kinetic scroll if necessary
  if (pw->common.di.isKineticScrollTurnedOn)
  {
    float xMouseVelocity, yMouseVelocity;
    GetMouseMoveVelocity(pw, &xMouseVelocity, &yMouseVelocity);

    pw->common.di.xKineticScrollVelocity = -xMouseVelocity;
    pw->common.di.yKineticScrollVelocity = -yMouseVelocity;
  }
}

extern void HandleKineticScrollMessage(window_t *pw)
{
  const mouse_point_t *point = GetMouseLastPoint(pw);

  // if mouse button released
  if (GetKeyState(VK_LBUTTON) >= 0)
  {
    int diffY = (int)(pw->common.di.yKineticScrollVelocity * KINETIC_TIMER_TIME);
    int diffX = (int)(pw->common.di.xKineticScrollVelocity * KINETIC_TIMER_TIME);
    int newYPos = pw->common.vscrollPos + diffY;
    int newXPos = pw->common.hscrollPos + diffX;
    char isDrawingRestricted = 0;

    newYPos = max(newYPos, 0);
    newYPos = min(newYPos, pw->common.vscrollRange);
    newXPos = max(newXPos, 0);
    newXPos = min(newXPos, pw->common.hscrollRange);

    if (!pw->common.di.isDrawingRestricted && newXPos != pw->common.hscrollPos && newYPos != pw->common.vscrollPos)
    {
      RestrictDrawing(&pw->common, &pw->caret, &pw->firstLine);
      isDrawingRestricted = 1;
    }

    HScrollWindow(&pw->common, &pw->caret, &pw->firstLine, newXPos);
    VScrollWindow(&pw->common, &pw->caret, &pw->firstLine, newYPos);

    if (isDrawingRestricted)
      AllowDrawing(&pw->common, &pw->caret, &pw->firstLine);      

    if (diffX || diffY)
    {
      int yVelocitySign = SESIGN(pw->common.di.yKineticScrollVelocity);
      int xVelocitySign = SESIGN(pw->common.di.xKineticScrollVelocity);
      int yNewVelocitySign;
      int xNewVelocitySign;

      if (yVelocitySign)
        pw->common.di.yKineticScrollVelocity -= (pw->common.di.yKineticScrollVelocity > 0 ? KINETIC_TIMER_TIME * KINETIC_SCROLL_FRICTION : -KINETIC_TIMER_TIME * KINETIC_SCROLL_FRICTION);

      if (xVelocitySign)
        pw->common.di.xKineticScrollVelocity -= (pw->common.di.xKineticScrollVelocity > 0 ? KINETIC_TIMER_TIME * KINETIC_SCROLL_FRICTION : -KINETIC_TIMER_TIME * KINETIC_SCROLL_FRICTION);

      yNewVelocitySign = SESIGN(pw->common.di.yKineticScrollVelocity);
      xNewVelocitySign = SESIGN(pw->common.di.xKineticScrollVelocity);

      if (yNewVelocitySign && yVelocitySign != yNewVelocitySign)
        pw->common.di.yKineticScrollVelocity = 0;

      if (xNewVelocitySign && xVelocitySign != xNewVelocitySign)
        pw->common.di.xKineticScrollVelocity = 0;
    }
    else
      StopKinecticScroll(pw);

    if (pw->common.vscrollPos >= pw->common.vscrollRange && diffY > 0)
      pw->common.di.yKineticScrollVelocity = 0;

    if (pw->common.vscrollPos <= 0 && diffY < 0)
      pw->common.di.yKineticScrollVelocity = 0;

    if (pw->common.hscrollPos >= pw->common.hscrollRange && diffX > 0)
      pw->common.di.xKineticScrollVelocity = 0;

    if (pw->common.hscrollPos <= 0 && diffX < 0)
      pw->common.di.xKineticScrollVelocity = 0;
  } 
}

extern void StopKinecticScroll(window_t *pw)
{
  pw->common.di.yKineticScrollVelocity = 0;
  pw->common.di.xKineticScrollVelocity = 0;
  pw->common.di.kineticScrollActivated = 0;
  KillTimer(pw->common.hwnd, KINETIC_TIMER_ID);
}