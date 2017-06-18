#include "mouse.h"

#include <Windows.h>


#define MOUSE_MOVE_VELOCITY_START_MARGIN   0
#define MOUSE_MOVE_VELOCITY_END_MARGIN     1


#define PREV_MOUSE_POINT(current, prev) (((current) + MOUSE_HISTORY_SIZE - (prev)) % MOUSE_HISTORY_SIZE)

#define NEXT_MOUSE_POINT(current, next) (((current) + (next)) % MOUSE_HISTORY_SIZE)


static int getRangeSize(int startRangePoint, int endRangePoint)
{
  return (startRangePoint > endRangePoint ? 
    (endRangePoint + MOUSE_HISTORY_SIZE - startRangePoint + 1) :
    (endRangePoint - startRangePoint + 1));
}

//static char isPointInRange(int targetPoint, int startRangePoint, int endRangePoint)
//{
//  return (startRangePoint > endRangePoint ? 
//    (targetPoint >= startRangePoint || targetPoint <= endRangePoint) : 
//    (targetPoint >= startRangePoint && targetPoint <= endRangePoint)) ;
//}

// Searches first point in a connected row
static int findRowStartPoint(const window_t *pw, int endPoint)
{
  int startPoint = PREV_MOUSE_POINT(endPoint, 1);
  int tempPoint = endPoint;

  if (pw->mouse.historySize >= MOUSE_HISTORY_SIZE)
  {
    while (startPoint != endPoint && (pw->mouse.history[tempPoint].time - pw->mouse.history[startPoint].time) < MOUSE_EVENT_ROW_TIME)
    {
      tempPoint = startPoint;
      startPoint = PREV_MOUSE_POINT(startPoint, 1);
    }
  }
  else
  {
    while (startPoint >= 0 && (pw->mouse.history[tempPoint].time - pw->mouse.history[startPoint].time) < MOUSE_EVENT_ROW_TIME)
    {
      tempPoint = startPoint;
      startPoint = PREV_MOUSE_POINT(startPoint, 1);   
    }
  }

  return tempPoint;
}


extern void TrackMousePosition(window_t *pw, short int x, short int y)
{
  int current = pw->mouse.currentPoint;

  pw->mouse.history[current].time = GetTickCount();
  pw->mouse.history[current].x = x;
  pw->mouse.history[current].y = y;
  pw->mouse.historySize = min(pw->mouse.historySize + 1, MOUSE_HISTORY_SIZE);

  pw->mouse.currentPoint = NEXT_MOUSE_POINT(current, 1);
}

extern void GetMouseMoveVelocity(const window_t *pw, float *pxVelocity, float *pyVelocity)
{
  *pxVelocity = 0;
  *pyVelocity = 0;

  if (pw->mouse.historySize > MOUSE_MOVE_VELOCITY_START_MARGIN + MOUSE_MOVE_VELOCITY_END_MARGIN)
  {
    // determine number of connected points

    int endPoint = PREV_MOUSE_POINT(pw->mouse.currentPoint, 1);
    int startPoint = findRowStartPoint(pw, endPoint);

    // if there are connected events, analyze them
    if (getRangeSize(startPoint, endPoint) > MOUSE_MOVE_VELOCITY_START_MARGIN + MOUSE_MOVE_VELOCITY_END_MARGIN + 1)
    { 
      const mouse_point_t *history = pw->mouse.history;
      int tempPoint;
      float weightSum = 0, weight = 1;
      float xVelocity = 0, yVelocity = 0;

      startPoint = NEXT_MOUSE_POINT(startPoint, MOUSE_MOVE_VELOCITY_START_MARGIN);
      endPoint = PREV_MOUSE_POINT(endPoint, MOUSE_MOVE_VELOCITY_START_MARGIN);
      tempPoint = PREV_MOUSE_POINT(endPoint, 1);

      while (endPoint != startPoint)
      {
        int timeDiff = history[endPoint].time - history[tempPoint].time;

        if (timeDiff > 0)
        {
          xVelocity += (((float)(history[endPoint].x - history[tempPoint].x)) / timeDiff) * weight;
          yVelocity += (((float)(history[endPoint].y - history[tempPoint].y)) / timeDiff) * weight;

          weightSum += weight;
          //weight *= 1f;
        }

        endPoint = tempPoint;
        tempPoint = PREV_MOUSE_POINT(tempPoint, 1);
      }

      if (weightSum > 0)
      {
        xVelocity /= weightSum;
        yVelocity /= weightSum;

        *pxVelocity = xVelocity;
        *pyVelocity = yVelocity;
      }
    }
  }
}

extern const mouse_point_t* GetMouseLastPoint(const window_t *pw)
{
  return GetMouseHistoryPoint(pw, 1);
}

extern int GetMouseHistorySize(const window_t *pw)
{
  return pw->mouse.historySize;
}

extern const mouse_point_t* GetMouseHistoryPoint(const window_t *pw, int prev)
{
  return prev < pw->mouse.historySize ? &pw->mouse.history[PREV_MOUSE_POINT(pw->mouse.currentPoint, prev)] : NULL;
}