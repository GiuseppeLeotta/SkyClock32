#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include <Arduino.h>
#include <SPI.h>

#include "gImage.h"

class display {
protected:
  SPIClass *spi;
  uint8_t pinCS;
  uint8_t pinRST;
  uint8_t pinDC;
public:
  void setCS(bool b);
  void writeCommand(uint8_t c);
  void writeData(uint8_t d);
  void writeData16(uint16_t d);
  void begin(void);
  display(SPIClass *_spi, uint8_t _pinDC, uint8_t _pinCS = -1, uint8_t _pinRST = -1);
  virtual void setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) = 0;
  virtual void setRotation(uint8_t m) = 0;
  virtual void initDisp() = 0;
  virtual void drawPixel(uint16_t x, uint16_t y, uint16_t color565) = 0;
  virtual void fillScreen(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color565) = 0;
  virtual void clearScreen(uint16_t color565) = 0;
  virtual void dumpImage(uint16_t x, uint16_t y, gImage *gIm) = 0;
  virtual uint16_t xValMax(void) = 0;
  virtual uint16_t yValMax(void) = 0;
  virtual ~display() = default;  // distruttore virtuale
};





// Macro per costruire un colore BGR565 da valori 8-bit (0–255)
#define BGR565(b, g, r) ((uint16_t)(((uint16_t)((b) >> 3) << 11) | ((uint16_t)((g) >> 2) << 5) | ((uint16_t)((r) >> 3))))

#define BGR565_BLACK 0x0000  // B=0,   G=0,   R=0
#define BGR565_WHITE 0xFFFF  // B=31,  G=63,  R=31

#define BGR565_RED 0x001F    // B=0,   G=0,   R=31
#define BGR565_GREEN 0x07E0  // B=0,   G=63,  R=0
#define BGR565_BLUE 0xF800   // B=31,  G=0,   R=0

#define BGR565_YELLOW 0x07FF   // G+R:  0x07E0 | 0x001F
#define BGR565_CYAN 0xFFE0     // B+G:  0xF800 | 0x07E0
#define BGR565_MAGENTA 0xF81F  // B+R:  0xF800 | 0x001F

// Qualche colore in più
#define BGR565_SILVER 0xBDF7               // (192,192,192)
#define BGR565_GRAY 0x8410                 // (128,128,128)
#define BGR565_MAROON 0x0010               // (0,0,128)
#define BGR565_OLIVE 0x07E4                // (0,128,128)
#define BGR565_PURPLE 0xF81F               // (128,0,128) – stesso di MAGENTA
#define BGR565_TEAL 0xFFE0                 // (0,128,128) – stesso di CYAN
#define BGR565_NAVY 0xF800                 // (128,0,0)   – stesso di BLUE
#define BGR565_ORANGE BGR565(0, 165, 255)  // (255,165,0)


// Macro per costruire un colore RGB565 da valori 8-bit (0–255)
#define RGB565(r, g, b) ((uint16_t)(((uint16_t)((r) >> 3) << 11) | ((uint16_t)((g) >> 2) << 5) | ((uint16_t)((b) >> 3))))

// Colori base
#define RGB565_BLACK 0x0000  // R=0,   G=0,   B=0
#define RGB565_WHITE 0xFFFF  // R=31,  G=63,  B=31

#define RGB565_RED 0xF800    // R=31,  G=0,   B=0
#define RGB565_GREEN 0x07E0  // R=0,   G=63,  B=0
#define RGB565_BLUE 0x001F   // R=0,   G=0,   B=31

#define RGB565_YELLOW 0xFFE0   // R+G:  0xF800 | 0x07E0
#define RGB565_CYAN 0x07FF     // G+B:  0x07E0 | 0x001F
#define RGB565_MAGENTA 0xF81F  // R+B:  0xF800 | 0x001F

// Qualche colore in più
#define RGB565_SILVER RGB565(192, 192, 192)  // ≈0xC618
#define RGB565_GRAY RGB565(128, 128, 128)    // ≈0x8410
#define RGB565_MAROON RGB565(128, 0, 0)      // ≈0x8000
#define RGB565_OLIVE RGB565(128, 128, 0)     // ≈0x8400
#define RGB565_PURPLE RGB565(128, 0, 128)    // ≈0x8010
#define RGB565_TEAL RGB565(0, 128, 128)      // ≈0x0410
#define RGB565_NAVY RGB565(0, 0, 128)        // ≈0x0010
#define RGB565_ORANGE RGB565(255, 165, 0)    // ≈0xFD20

// Colori tenui predefiniti
#define RGB565_LIGHT_BLUE 0xAEDC     // (173, 216, 230)
#define RGB565_LIGHT_RED 0xFBB2      // (255, 182, 193)
#define RGB565_LIGHT_CYAN 0xDFFF     // (224, 255, 255)
#define RGB565_LIGHT_GREEN 0x9772    // (144, 238, 144)
#define RGB565_LIGHT_YELLOW 0xFFF9   // (255, 255, 224)
#define RGB565_LIGHT_MAGENTA 0xF81F  // (255, 182, 193)
#define RGB565_LIGHT_GRAY 0xC618     // (192, 192, 192)
#define RGB565_LIGHT_ORANGE 0xFD20   // (255, 165, 0)


#endif
