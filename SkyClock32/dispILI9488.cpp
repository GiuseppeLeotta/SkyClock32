
#include "dispILI9488.h"
#include <pgmspace.h>
#include "gImage.h"

#define pgm_read_pointer(addr) ((void *)pgm_read_dword(addr))

inline uint16_t *pgm_read_image_ptr(const gImage *gIm) {
  return (uint16_t *)pgm_read_pointer(&gIm->image);
}

dispILI9488::dispILI9488(SPIClass *_spi, uint16_t _xMax, uint16_t _yMax, uint8_t _pinDC,
                         uint8_t _pinCS, uint8_t _pinRST)
  : display(_spi, _pinDC, _pinCS, _pinRST) {
  xMax = _xMax;
  yMax = _yMax;
  xV[0] = xMax;
  xV[1] = yMax;
  xV[2] = xMax;
  xV[3] = yMax;
  yV[0] = yMax;
  yV[1] = xMax;
  yV[2] = yMax;
  yV[3] = xMax;
}

// imposta l’area di disegno (colonna e riga)
void dispILI9488::setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
  static uint16_t Oldx0 = 0xFFFF, Oldx1 = 0xFFFF, Oldy0 = 0xFFFF, Oldy1 = 0xFFFF;
  setCS(LOW);
  if (x0 != Oldx0 || x1 != Oldx1) {
    writeCommand(0x2A);  // COLADDRSET
    writeData16(x0);
    writeData16(x1);
    Oldx0 = x0;
    Oldx1 = x1;
  }
  if (y0 != Oldy0 || y1 != Oldy1) {
    writeCommand(0x2B);  // PAGEADDRSET
    writeData16(y0);
    writeData16(y1);
    Oldy0 = x0;
    Oldy1 = x1;
  }
  writeCommand(0x2C);  // MEMWR
  setCS(HIGH);
}

#define ILI9488_MADCTL_BGR 0x08
#define ILI9488_MADCTL_MV 0x20
#define ILI9488_MADCTL_MY 0x80
#define ILI9488_MADCTL_MX 0x40

void dispILI9488::setRotation(uint8_t m) {
  const uint8_t madctl[] = {
    ILI9488_MADCTL_BGR | ILI9488_MADCTL_MX,                                         // 0: portrait          (MY=0, MX=1, MV=0, ML=0, RGB=1, MH=0)
    ILI9488_MADCTL_BGR | ILI9488_MADCTL_MV,                                         // 1: landscape         (MY=0, MX=0, MV=1, ML=0, RGB=1, MH=0) :contentReference[oaicite:1]{index=1}
    ILI9488_MADCTL_BGR | ILI9488_MADCTL_MY,                                         // 2: portrait inverted (MY=1, MX=0, MV=0, ML=0, RGB=1, MH=0)
    ILI9488_MADCTL_BGR | ILI9488_MADCTL_MV | ILI9488_MADCTL_MX | ILI9488_MADCTL_MY  // 3: landscape inv.    (MY=1, MX=1, MV=1, ML=0, RGB=1, MH=0) :contentReference[oaicite:2]{index=2}
  };

  //const uint16_t xV[] = {320, 480, 320, 480};
  //const uint16_t yV[] = {480, 320, 480, 320};

  if (m > 3 || m < 0) m = 0;
  xMax = xV[m];
  yMax = yV[m];
  // invia comando MADCTL
  setCS(LOW);
  digitalWrite(pinDC, LOW);
  spi->write(0x36);
  // invia parametro
  digitalWrite(pinDC, HIGH);
  spi->write(madctl[m]);
  setCS(HIGH);
}

void dispILI9488::initDisp(void) {
  // hardware reset se disponibile…
  begin();
  if (pinRST != -1) {
    digitalWrite(pinRST, HIGH);
    delay(5);
    digitalWrite(pinRST, LOW);
    delay(20);
    digitalWrite(pinRST, HIGH);
    delay(150);
  }
  setCS(LOW);
  writeCommand(0x01);
  delay(250);          // Software reset
  writeCommand(0xE0);  // Positive Gamma Control
  writeData(0x00);
  writeData(0x03);
  writeData(0x09);
  writeData(0x08);
  writeData(0x16);
  writeData(0x0A);
  writeData(0x3F);
  writeData(0x78);
  writeData(0x4C);
  writeData(0x09);
  writeData(0x0A);
  writeData(0x08);
  writeData(0x16);
  writeData(0x1A);
  writeData(0x0F);
  writeCommand(0xE1);  // Negative Gamma Control
  writeData(0x00);
  writeData(0x16);
  writeData(0x19);
  writeData(0x03);
  writeData(0x0F);
  writeData(0x05);
  writeData(0x32);
  writeData(0x45);
  writeData(0x46);
  writeData(0x04);
  writeData(0x0E);
  writeData(0x0D);
  writeData(0x35);
  writeData(0x37);
  writeData(0x0F);
  writeCommand(0xC0);  // Power Control 1
  writeData(0x17);
  writeData(0x15);
  writeCommand(0xC1);  // Power Control 2
  writeData(0x41);
  writeCommand(0xC5);  // VCOM Control
  writeData(0x00);
  writeData(0x12);
  writeData(0x80);
  writeCommand(0x36);  // MADCTL
  writeData(0x48);     // MX, BGR
  writeCommand(0x3A);  // Pixel Format
  //writeData(0x66);    // 18‑bit colour for SPI :contentReference[oaicite:1]{index=1}
  writeData(0x55);
  writeCommand(0xB0);
  writeData(0x00);  // Interface Mode Control
  writeCommand(0xB1);
  writeData(0xA0);  // Frame Rate Control
  writeCommand(0xB4);
  writeData(0x02);     // Display Inversion Control
  writeCommand(0xB6);  // Display Function Control
  writeData(0x02);
  writeData(0x02);
  writeData(0x3B);
  writeCommand(0xB7);
  writeData(0xC6);     // Entry Mode Set
  writeCommand(0xF7);  // Adjust Control 3
  writeData(0xA9);
  writeData(0x51);
  writeData(0x2C);
  writeData(0x82);
  writeCommand(0x11);
  delay(120);  // Sleep Out
  writeCommand(0x29);
  delay(25);  // Display ON
  setRotation(1);
  setCS(HIGH);
}


void dispILI9488::drawPixel(uint16_t x, uint16_t y, uint16_t color565) {

  setAddrWindow(x, y, x, y);  // CASET + PASET + MEMWR
  setCS(LOW);
  digitalWrite(pinDC, HIGH);
  spi->write16(color565);
  setCS(HIGH);
}

static inline uint16_t swap(uint16_t u) {
  return (u << 8) | (u >> 8);
}

void dispILI9488::fillScreen(uint16_t x0,uint16_t y0, uint16_t x1, uint16_t y1,uint16_t color565) {

  setAddrWindow(x0, y0, x1-1, y1-1);
  // invio ripetuto di pixel
  color565 = swap(color565);
  setCS(LOW);
  digitalWrite(pinDC, HIGH);
  spi->writePattern((const uint8_t *)&color565, 2, (uint32_t)(x1-x0)*(uint32_t)(y1-y0) );
  setCS(HIGH);
}

void dispILI9488::clearScreen(uint16_t color565) {
  fillScreen(0, 0, xMax, yMax, color565);
}

void dispILI9488::dumpImage(uint16_t x, uint16_t y, gImage *gIm) {
  setAddrWindow(x, y, x + gIm->xLen - 1, y + gIm->yLen - 1);
  uint16_t *bf = pgm_read_image_ptr(gIm);
  setCS(LOW);
  digitalWrite(pinDC, HIGH);
  for (uint32_t i = 0; i < (uint32_t)gIm->yLen * (uint32_t)gIm->xLen; i++) {
    uint16_t cl = pgm_read_word(&bf[i]);
    spi->write16(cl);
    if (i%1000==0) yield();
  }
  setCS(HIGH);
}


