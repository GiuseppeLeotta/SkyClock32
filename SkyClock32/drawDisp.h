#ifndef _DRAW_DISP_H_
#define _DRAW_DISP_H_

#include <Arduino.h>
#include <pgmspace.h>
#include "display.h"
#include "gfxfont.h"


#define MAXCANVAS 4

#define WRITE_HOR 0
#define WRITE_VER 1

class drawDisp {
private:
  const GFXfont *gfxFont;
  uint16_t ox[MAXCANVAS];
  uint16_t oy[MAXCANVAS];
  uint16_t lx[MAXCANVAS];
  uint16_t ly[MAXCANVAS];
  uint32_t stride[MAXCANVAS];
  uint16_t color[MAXCANVAS];
  uint16_t bkcolor;
  uint16_t lyfont;
  uint8_t *buff[MAXCANVAS];
  int8_t priority[MAXCANVAS];
  int8_t pBuff;
  int8_t nBuff;
  uint16_t nChar;     // numero di caratteri disponibili per il font
  uint8_t yAdvance;   //  Newline distance (y axis)
  int8_t baseline;    // linea di base del char
  uint16_t xposLine;  // usato per printLine
  uint16_t yposLine;  // usato per printLine
  uint16_t xStart;    // usato per printLine
  uint16_t yStart;    // usato per printLine
  const char * charReduction;  // dimezza lo spazio del char
  display *disp;
  void clrbuff(void);
  int16_t nextline(void);
  int rowUp(uint16_t nPixels);
  int refreshMem(int16_t x0, int16_t y0, int16_t xb, int16_t yb, int16_t lxb, int16_t lyb, bool onXor);
  bool isCharReduction(char c);
public:
  drawDisp(display *_disp, const GFXfont *gfx_Font, uint16_t _color, uint16_t _bkcolor, const char * _charReduction=NULL);
  drawDisp(display *_disp, const GFXfont *gfx_Font, uint16_t _color, uint16_t _bkcolor, uint16_t _lx, uint16_t _ly, const char * _charReduction=NULL);
  ~drawDisp();
  void setPixel(uint16_t x, uint16_t y, bool v = true);
  bool getPixel(uint16_t _x, uint16_t _y);
  void setFont(const GFXfont *gfx_Font);
  int drawString(int16_t x0, int16_t y0, char *s, bool onScreen = false, uint16_t Direction = WRITE_HOR);
  int drawString(char *s);
  int drawPrint(int16_t x0, int16_t y0, char *s);
  void boxString(char *s, uint16_t &lw, uint16_t &lh, uint16_t Direction = WRITE_HOR);
  int refresh(int16_t x0, int16_t y0, int16_t xb, int16_t yb, int16_t lxb, int16_t lyb, bool onXor = false);
  int refresh(int16_t x0, int16_t y0, bool onXor = false);
  int creatCanvas(uint16_t _ox, uint16_t _oy, uint16_t _lx, uint16_t _ly, uint16_t _color);
  int creatCanvas(uint16_t _lx, uint16_t _ly);
  int creatScreen(uint16_t _xStart, uint16_t _yStart, int16_t _lx, uint16_t _ly);
  void setColor(uint16_t _color, uint16_t _bkcolor);
  void axis(uint16_t lx, uint16_t ly, uint16_t nxTick, uint16_t nyTick, uint16_t lenTick = 3, uint16_t gridDash = 0);
  int16_t dScale(double x, double minScale, double maxScale, double minValue, double maxValue);
  void plot(uint16_t nPoint, double *xVal, float *yVal, double xMinScale, double xMaxScale, float yMinScale, float yMaxScale, uint16_t color, int16_t thickness=1);
  void drawHorizontalLine(int16_t x1, int16_t x2, int16_t y, int16_t lenDash = 0);
  void drawVerticalLine(int16_t x, int16_t y1, int16_t y2, int16_t lenDash = 0);
  void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t thickness=1, int16_t lenDash = 0);  
  void drawCircle(int16_t cx, int16_t cy, int16_t r);
  void drawFilledCircle(int16_t x0, int16_t y0, int16_t r);
  void drawArcCircle(int cx, int cy, int radius, double a1_deg, double a2_deg);
  void drawCircleTick(uint16_t cx, uint16_t cy, uint16_t radius, uint16_t long_tick, uint16_t short_tick, uint16_t startTick = 0);
  void drawCircleLabel(uint16_t cx, uint16_t cy, uint16_t radius, uint16_t dist, char *Label[], uint16_t nLabel, uint16_t startLabel = 0);
  void drawCircleLine(uint16_t cx, uint16_t cy, uint16_t radius, double angdeg, uint16_t lenLine);
  void drawTriangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
  void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2);
  int setPriority(int16_t nCanvas, int16_t nPriority);
  int setCanvas(int16_t nCanvas);
  int clearCanvas(void);
  int drawTriangleHand(int16_t cx, int16_t cy, double angle,
                       int16_t length, int16_t base_width, int16_t offset, int32_t color = -1);
  void printLine(char *s);
};


#endif
