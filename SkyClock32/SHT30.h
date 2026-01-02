#pragma once
#include <Wire.h>

class SHT30 {
public:

  // Commands
  static constexpr uint8_t CMD_MEASURE[2] = { 0x24, 0x00 };    // High repeatability, no clock stretch
  static constexpr uint8_t CMD_SOFTRESET[2] = { 0x30, 0xA2 };  // Soft reset command
  static constexpr uint8_t CMD_READSTATUS[2] = { 0xF3, 0x2D };
  static constexpr uint8_t CMD_CLEARSTATUS[2] = { 0x30, 0x41 };
  static constexpr uint8_t CMD_READSERIAL[2] = { 0x37, 0x80 };
  static constexpr uint8_t CMD_HEATER_ON[2] = { 0x30, 0x6D };
  static constexpr uint8_t CMD_HEATER_OFF[2] = { 0x30, 0x66 };

  struct Reading {
    float temperature;  // °C
    float humidity;     // %RH
    bool valid;         // CRC check
  };

  // Constructor: specify SDA/SCL pins for Wire
  SHT30(TwoWire &_wire = Wire, uint8_t _address = 0x44)
    : wire(_wire), address(_address) {}

  // Initialize I2C and reset sensor
  void begin() {
    softReset();
    clearAll();
    heaterOnOff(false);
  }

  // Perform a soft reset
  void softReset() {
    writeCmd(CMD_SOFTRESET);
    // Wait for reset to complete (~1ms)
    delay(5);
  }

  uint32_t readSerialNumber() {
    uint8_t buf[6];
    writeCmd(CMD_READSERIAL);
    wire.requestFrom(address, (uint8_t) 6);
    if (wire.available() < 6) return 0;
    for (int i = 0; i < 6; i++) {
      buf[i] = wire.read();
    }
    if (crc8(buf, 2) != buf[2] || crc8(buf + 3, 2) != buf[5]) {
      return 0;  // CRC failed
    }
    return ((buf[0] << 24) | (buf[1] << 16) | (buf[3] << 8) | buf[4]);
  }

  void clearAll() {
    writeCmd(CMD_CLEARSTATUS);
  }

  uint16_t readStatusRegister() {
    uint8_t buf[3];
    writeCmd(CMD_READSTATUS);
    wire.requestFrom(address, (uint8_t) 3);
    if (wire.available() < 3) return 0;
    for (int i = 0; i < 3; i++) {
      buf[i] = wire.read();
    }
    if (crc8(buf, 2) != buf[2]) return 0;
    return (buf[0] << 8) | buf[1];
  }

  void heaterOnOff(bool OnOff) {
    if (OnOff) writeCmd(CMD_HEATER_ON); else writeCmd(CMD_HEATER_OFF);
  }

  // Trigger measurement and read data
  Reading read() {
    Reading result{ 0.0f, 0.0f, false };
    // heaterOnOff(true);
    // delay(100);
    // heaterOnOff(false);
    // delay(100);
    // Send measure command
    writeCmd(CMD_MEASURE);
    // Wait conversion (max ~15ms)
    delay(20);

    // Request 6 bytes
    wire.requestFrom(address, (uint8_t)6);
    if (wire.available() < 6) {
      return result;  // valid=false
    }

    uint8_t buf[6];
    for (int i = 0; i < 6; i++) {
      buf[i] = wire.read();
    }



    // CRC checks
    if (crc8(buf, 2) != buf[2] || crc8(buf + 3, 2) != buf[5]) {
      return result;  // CRC failed
    }

    // Raw values
    uint16_t rawT = (uint16_t(buf[0]) << 8) | buf[1];
    uint16_t rawH = (uint16_t(buf[3]) << 8) | buf[4];

    // Convert to human units
    result.temperature = -45.0f + 175.0f * ((double) rawT / 65535.0f);
    result.humidity = 100.0f * ((double)rawH / 65535.0f);
    result.valid = true;

    return result;
  }

private:
  // I2C address
  TwoWire &wire;
  uint8_t address;

  // CRC8 calculation (polynomial 0x31)
  static uint8_t crc8(const uint8_t *data, int len) {
    uint8_t crc = 0xFF;
    for (int i = 0; i < len; i++) {
      crc ^= data[i];
      for (int b = 0; b < 8; b++) {
        if (crc & 0x80) crc = (crc << 1) ^ 0x31;
        else crc <<= 1;
      }
    }
    return crc;
  }
  void writeCmd(const uint8_t *cmd) {
    wire.beginTransmission(address);
    wire.write(cmd, 2);
    wire.endTransmission();
  }
};
