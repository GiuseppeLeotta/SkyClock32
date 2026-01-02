#ifndef _DISP_ILI9488_H_
#define _DISP_ILI9488_H_

#include "display.h"

class dispILI9488 : public display {
private:
  uint16_t xMax;
  uint16_t yMax;
  uint16_t xV[4];
  uint16_t yV[4];
public:
  dispILI9488(SPIClass *_spi, uint16_t _xMax, uint16_t _yMax, uint8_t _pinDC, uint8_t _pinCS = -1, uint8_t _pinRST = -1);
  void setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) override;
  void setRotation(uint8_t m) override;
  void initDisp(void) override;
  void drawPixel(uint16_t x, uint16_t y, uint16_t color565) override;
  void fillScreen(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color565) override;
  void clearScreen(uint16_t color565) override;
  void dumpImage(uint16_t x, uint16_t y, gImage *gIm) override;
  uint16_t xValMax(void) override {
    return xMax;
  }
  uint16_t yValMax(void) override {
    return yMax;
  }
};

#endif
