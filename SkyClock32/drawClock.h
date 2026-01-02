#ifndef _DRAW_CLOCK_H_
#define _DRAW_CLOCK_H_

#include <Arduino.h>

#include "drawDisp.h"
#include "gfxfont.h"


class drawClock : public drawDisp {
  private:
    uint16_t cx;
    uint16_t cy;
    uint16_t radius;
    uint16_t long_tick;
    uint16_t short_tick;
  public:
    drawClock(display * _disp, const GFXfont * gfx_Font, uint16_t _color, uint16_t _bkcolor);
    int drawCircleClock(uint16_t cx, uint16_t cy, uint16_t radius, uint16_t long_tick, uint16_t short_tick);
    int drawHour(time_t tm, int32_t color = -1);
    int drawMinute(time_t tm, int32_t color = -1);
    void drawSecond(time_t tm, uint32_t ms);
};

#endif
