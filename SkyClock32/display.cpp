
#include "display.h"


display::display(SPIClass *_spi, uint8_t _pinDC, uint8_t _pinCS , uint8_t _pinRST) {

  pinCS = _pinCS; pinRST = _pinRST; pinDC = _pinDC; spi = _spi;

}

void display::begin(void) {
  pinMode(pinDC, OUTPUT);
  digitalWrite(pinDC, LOW);

  if (pinCS != -1) {
    pinMode(pinCS, OUTPUT);
    digitalWrite(pinCS, HIGH);
  }
  if (pinRST != -1) {
    pinMode(pinRST, OUTPUT);
    digitalWrite(pinRST, HIGH);
  }
}

void display::setCS(bool b) {
  if (pinCS != -1) digitalWrite(pinCS, b);
}

void display::writeCommand(uint8_t c) {
  digitalWrite(pinDC, LOW);
  spi->write(c);
  digitalWrite(pinDC, HIGH);
}

void display::writeData(uint8_t d) {
  //digitalWrite(pinDC, HIGH);
  spi->write(d);
}

void display::writeData16(uint16_t d) {
  //digitalWrite(pinDC, HIGH);
  spi->write16(d);
}
