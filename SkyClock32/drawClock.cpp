

#include "drawClock.h"


drawClock::drawClock( display * _disp, const GFXfont * gfx_Font, uint16_t _color, uint16_t _bkcolor)
  : drawDisp(_disp, gfx_Font, _color, _bkcolor) { }

int drawClock::drawCircleClock(uint16_t _cx, uint16_t _cy, uint16_t _radius, uint16_t _long_tick, uint16_t _short_tick) {

  cx = _cx;  cy = _cy; radius = _radius; long_tick = _long_tick; short_tick = _short_tick;
  int nCanvas = creatCanvas((radius + 1)*2, (radius + 1)*2);
  if (nCanvas >= 0) {
    // 2) Cerchio esterno
    drawCircle(cx, cy, radius);

    // 3) Ticks per minuti e ore
    drawCircleTick(cx, cy, radius, long_tick, short_tick);

    // 4) Numeri 3,6,9,12
    char *labels[12] = { "12", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11"};
    drawCircleLabel(cx, cy, radius, long_tick, labels, 12);
  }
  return nCanvas;
}

int drawClock::drawHour(time_t tm, int32_t color) {
  int hour = (tm % 86400) / 3600;
  int minute = (tm % 3600) / 60;
  double angle = ((hour % 12) + (minute / 60.0)) * (2 * M_PI / 12.0) - M_PI / 2;
  int16_t length = radius * 3 / 5;
  int16_t base_width = radius / 6;
  int16_t offset = radius / 20;
  return drawTriangleHand(cx, cy, angle, length, base_width, offset, color);
}

int drawClock::drawMinute( time_t tm, int32_t color) {
  double minute = (tm % 3600) / 60. ;
  double angle = (minute * (2 * M_PI / 60.0)) - M_PI / 2;
  int16_t length = radius * 4 / 5;
  int16_t base_width = radius / 10;
  int16_t offset = radius / 30;
  return drawTriangleHand(cx, cy, angle, length, base_width, offset, color);
}

void drawClock::drawSecond( time_t tm, uint32_t ms) {
  double second = (tm % 60) + ms / 1000.;
  double angle = (second * (2 * M_PI / 60.0)) - M_PI / 2;
  int length = radius - 2;
  // linea sottile
  int x1 = cx + (int)round(length * cos(angle));
  int y1 = cy + (int)round(length * sin(angle));
  drawLine(cx, cy, x1, y1);
}
