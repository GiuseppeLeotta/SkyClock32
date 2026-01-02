#include <Wire.h>
#include <time.h>



// Indirizzo I2C del DS3231
#define DS3231_ADDRESS 0x68

// Registri del DS3231
#define REG_SECONDS   0x00
#define REG_MINUTES   0x01
#define REG_HOURS     0x02
#define REG_DAY       0x03
#define REG_DATE      0x04
#define REG_MONTH     0x05
#define REG_YEAR      0x06
#define REG_TEMP_MSB  0x11
#define REG_TEMP_LSB  0x12

typedef uint8_t byte;

// Converte BCD in decimale
byte bcdToDec(byte val) {
  return (val >> 4) * 10 + (val & 0x0F);
}

// Converte decimale in BCD
byte decToBcd(byte val) {
  return ((val / 10) << 4) | (val % 10);
}

// Legge un singolo registro dal DS3231
byte readRegister(byte reg) {
  Wire.beginTransmission(DS3231_ADDRESS);
  Wire.write(reg);
  Wire.endTransmission();
  Wire.requestFrom(DS3231_ADDRESS, (byte)1);
  return Wire.available() ? Wire.read() : 0;
}

// Scrive un singolo registro sul DS3231
void writeRegister(byte reg, byte value) {
  Wire.beginTransmission(DS3231_ADDRESS);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

// Imposta data e ora sul DS3231 usando time_t (UTC)
void setDateTimeDs3231(time_t t) {
  struct tm *tm = gmtime(&t);
  byte second      = tm->tm_sec;
  byte minute      = tm->tm_min;
  byte hour        = tm->tm_hour;
  byte dayOfWeek   = (tm->tm_wday + 1); // 1=Sunday
  byte dayOfMonth  = tm->tm_mday;
  byte month       = tm->tm_mon + 1;
  uint16_t year    = tm->tm_year + 1900;

  Wire.beginTransmission(DS3231_ADDRESS);
  Wire.write(REG_SECONDS);
  Wire.write(decToBcd(second));
  Wire.write(decToBcd(minute));
  // 24-hour mode, bit6=0
  Wire.write(decToBcd(hour));
  Wire.write(decToBcd(dayOfWeek));
  Wire.write(decToBcd(dayOfMonth));
  // Century bit se anno >= 2100
  byte m = decToBcd(month);
  if (year >= 2100) m |= 0x80;
  Wire.write(m);
  Wire.write(decToBcd(year % 100));
  Wire.endTransmission();
}

// Legge data e ora dal DS3231 e restituisce time_t (UTC)
time_t readDateTimeDs3231(void) {
  byte rawSec   = readRegister(REG_SECONDS);
  byte rawMin   = readRegister(REG_MINUTES);
  byte rawHour  = readRegister(REG_HOURS);
  byte rawDate  = readRegister(REG_DATE);
  byte rawMonth = readRegister(REG_MONTH);
  byte rawYear  = readRegister(REG_YEAR);

  byte second = bcdToDec(rawSec);
  byte minute = bcdToDec(rawMin);

  byte hour;
  if (rawHour & 0x40) {
    // 12-hour mode
    hour = bcdToDec(rawHour & 0x1F);
    bool isPM = rawHour & 0x20;
    if (isPM) hour = (hour % 12) + 12;
  } else {
    // 24-hour mode
    hour = bcdToDec(rawHour & 0x3F);
  }

  byte dayOfMonth = bcdToDec(rawDate);
  byte month = bcdToDec(rawMonth & 0x1F);
  int year = 2000 + bcdToDec(rawYear);

  struct tm tm;
  tm.tm_sec  = second;
  tm.tm_min  = minute;
  tm.tm_hour = hour;
  tm.tm_mday = dayOfMonth;
  tm.tm_mon  = month - 1;
  tm.tm_year = year - 1900;
  tm.tm_isdst = 0;
  // tm_wday e tm_yday sono calcolati da mktime

  return mktime(&tm);
}

// Legge temperatura dal DS3231 (in °C)
float readTemperatureDs3231(void) {
  byte msb = readRegister(REG_TEMP_MSB);
  byte lsb = readRegister(REG_TEMP_LSB);
  int8_t signedMsb = (int8_t)msb;
  float fraction = (lsb >> 6) * 0.25;
  return signedMsb + fraction;
}
