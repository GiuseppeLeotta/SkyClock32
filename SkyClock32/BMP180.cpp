#include <math.h>
#include "Arduino.h"
#include "BMP180.h"



// registri e comandi interni
#define REG_CALIB_START   0xAA
#define REG_CONTROL       0xF4
#define REG_DATA          0xF6
#define CMD_TEMP          0x2E
#define CMD_PRESSURE      0x34

BMP180::BMP180(TwoWire &wire, uint8_t address, uint8_t oss)
  : _wire(wire), _address(address), _oss(oss), isOn(false) {}

bool BMP180::begin() {
  return isConnected();
}

void BMP180::setOversampling(uint8_t oss) {
  if (oss < 4) _oss = oss;
}

bool BMP180::readCalibration() {
  // buffer di 22 byte AC1…MD
  uint8_t buf[22];
  readRegisters(REG_CALIB_START, buf, 22);
  if (_wire.available() == 0 && buf[0] == 0xFF) return false;
  // parsifica
  AC1 = (int16_t)(buf[0]<<8 | buf[1]);
  AC2 = (int16_t)(buf[2]<<8 | buf[3]);
  AC3 = (int16_t)(buf[4]<<8 | buf[5]);
  AC4 = (uint16_t)(buf[6]<<8 | buf[7]);
  AC5 = (uint16_t)(buf[8]<<8 | buf[9]);
  AC6 = (uint16_t)(buf[10]<<8| buf[11]);
  B1  = (int16_t)(buf[12]<<8| buf[13]);
  B2  = (int16_t)(buf[14]<<8| buf[15]);
  MB  = (int16_t)(buf[16]<<8| buf[17]);
  MC  = (int16_t)(buf[18]<<8| buf[19]);
  MD  = (int16_t)(buf[20]<<8| buf[21]);
  return true;
}

void BMP180::writeRegister(uint8_t reg, uint8_t val) {
  _wire.beginTransmission(_address);
  _wire.write(reg);
  _wire.write(val);
  _wire.endTransmission();
}

void BMP180::readRegisters(uint8_t reg, uint8_t *buf, uint8_t len) {
  _wire.beginTransmission(_address);
  _wire.write(reg);
  _wire.endTransmission();
  _wire.requestFrom(_address, len);
  for (uint8_t i = 0; i < len; i++) {
    buf[i] = _wire.read();
  }
}

// int16_t BMP180::readS16(uint8_t reg) {
//   uint8_t buf[2];
//   readRegisters(reg, buf, 2);
//   return (int16_t)(buf[0] << 8 | buf[1]);
// }

uint16_t BMP180::readU16(uint8_t reg) {
  uint8_t buf[2];
  readRegisters(reg, buf, 2);
  return (uint16_t)(buf[0] << 8 | buf[1]);
}

int32_t BMP180::readRawTemperature() {
  writeRegister(REG_CONTROL, CMD_TEMP);
  delay(5);
  return readU16(REG_DATA);
}

int32_t BMP180::readRawPressure() {
  writeRegister(REG_CONTROL, CMD_PRESSURE + (_oss << 6));
  switch (_oss) {
    case 0: delay(5);  break;
    case 1: delay(8);  break;
    case 2: delay(14); break;
    case 3: delay(26); break;
  }
  uint8_t buf[3];
  readRegisters(REG_DATA, buf, 3);
  int32_t up = ((int32_t)buf[0]<<16 | (int32_t)buf[1]<<8 | buf[2]) >> (8 - _oss);
  return up;
}

float BMP180::readTemperature() {
  if (!isOn) return 0.0;
  int32_t UT = readRawTemperature();
  int32_t X1 = ((UT - (int32_t)AC6) * AC5) >> 15;
  int32_t X2 = ((int32_t)MC << 11) / (X1 + MD);
  _B5 = X1 + X2;
  int32_t T = (_B5 + 8) >> 4;
  return T / 10.0;  // °C
}

float BMP180::readPressure() {
  // assicuriamoci di avere _B5 calcolato
  if (!isOn) return 0.0;
  readTemperature();
  int32_t B6 = _B5 - 4000;
  int32_t X1 = (B2 * (B6 * B6 >> 12)) >> 11;
  int32_t X2 = (AC2 * B6) >> 11;
  int32_t X3 = X1 + X2;
  int32_t B3 = ((((int32_t)AC1 * 4 + X3) << _oss) + 2) >> 2;

  X1 = (AC3 * B6) >> 13;
  X2 = (B1 * (B6 * B6 >> 12)) >> 16;
  X3 = ((X1 + X2) + 2) >> 2;
  uint32_t B4 = (AC4 * (uint32_t)(X3 + 32768)) >> 15;
  uint32_t B7 = ((uint32_t)readRawPressure() - B3) * (50000 >> _oss);

  int32_t p = (B7 < 0x80000000)
            ? (B7 << 1) / B4
            : (B7 / B4) << 1;

  X1 = (p >> 8) * (p >> 8);
  X1 = (X1 * 3038) >> 16;
  X2 = (-7357 * p) >> 16;
  p = p + ((X1 + X2 + 3791) >> 4);

  return p / 100.0;  // hPa
}

float BMP180::seaLevelPressure(float P,  float h, float T) {

   return P*pow(1.0 - 0.0065*h/(T+273.15),-5.257);
   //return P/pow(1.0 - h/44330,5.255);   // calcolata a 15 gradi
}

uint8_t BMP180::readChipID() {
  _wire.beginTransmission(_address);
  Wire.write(0xD0);
  if (_wire.endTransmission() != 0) return 0;
  _wire.requestFrom(_address, (uint8_t)1);
  return _wire.available() ? _wire.read() : 0;
}

bool BMP180::isConnected() {
  bool c=readChipID() == 0x55;
  if (c && isOn==false) {
    isOn=true; 
    readCalibration();
  } else isOn=c; 
  return c;
}
