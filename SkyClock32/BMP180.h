#ifndef BMP180_H
#define BMP180_H

#include <Wire.h>

class BMP180 {
public:
  // oversampling: 0..3 (default 0 = ultra-low power)
  BMP180(TwoWire &wire = Wire, uint8_t address = 0x77, uint8_t oss = 0);
  
  // inizializza il bus I2C e legge i parametri di calibrazione
  // restituisce true se ok
  bool begin();

  // imposta oversampling (0..3)
  void setOversampling(uint8_t oss);

  // legge la temperatura in °C
  float readTemperature();

  // legge la pressione in hPa
  float readPressure();
  
  // Calcola la pressione al livello del mare (Pa) dalla pressione misurata in quota P->press, h->alt, T=temperatura
  float seaLevelPressure(float P,  float h=0, float T=25);
  
  bool isConnected();
private:
  TwoWire &_wire;
  uint8_t _address;
  uint8_t _oss;
  bool isOn;

  // parametri di calibrazione
  int16_t  AC1, AC2, AC3, B1, B2, MB, MC, MD;
  uint16_t AC4, AC5, AC6;
  int32_t  _B5;  // variabile di lavoro per calcoli

  // basi I2C
  void writeRegister(uint8_t reg, uint8_t val);
  void readRegisters(uint8_t reg, uint8_t *buf, uint8_t len);
  int16_t readS16(uint8_t reg);
  uint16_t readU16(uint8_t reg);

  // misure raw
  int32_t readRawTemperature();
  int32_t readRawPressure();

  // carica i dati di calibrazione dal sensore
  bool readCalibration();
  uint8_t readChipID();

};

#endif // BMP180_H