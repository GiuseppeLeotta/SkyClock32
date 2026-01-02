#include <Arduino.h>
#include "FileData.h"
#include "utility.h"
#include "ds3231.h"

const char* NameFileData = "/filedata.bin";

#define MAX_LEN_FILE_DATA 1024

const char* monthsOfTheYear[] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};


char* daysOfTheWeek[] = {
  "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

double Lux2Volt(double Lux, double VoltAlm, double rPhoto10Lux, double R2, double Gamma) {
  return (VoltAlm * R2) / (pow(10, -Gamma * (log10(Lux) - 1) + log10(rPhoto10Lux)) + R2);
}

double dig2Volt(uint16_t value) {
  return (double)value * MAXSCALE_ANALOG / (double)RESOL_ANALOG;
}

double ReadLux(double rValue, double VoltAlm, double rPhoto10Lux, double R2, double Gamma) {
  rValue = (rValue == 0 ? 0.001 : rValue);
  double R1 = VoltAlm * R2 / rValue - R2;
  //Serial.printf("%f %f %f\n",rValue,R1,-(log10(R1)-1)/Gamma+1);
  double r = pow(10, -(log10(R1) - log10(rPhoto10Lux)) / Gamma + 1);
  return (r > 100000 ? 100000 : r);
}

double IntesityValue(double rValue, double VoltAlm, double rPhoto10Lux, double R2, double Gamma, double MaxLux, double MinLux, double MaxIntesity, double MinIntesity) {
  double vMinLux = Lux2Volt(MinLux, VoltAlm, rPhoto10Lux, R2, Gamma);
  double vMaxLux = Lux2Volt(MaxLux, VoltAlm, rPhoto10Lux, R2, Gamma);
  double v = (pow(10, rValue) - pow(10, vMinLux)) / (pow(10, vMaxLux) - pow(10, vMinLux)) * (MaxIntesity - MinIntesity) + MinIntesity;
  v = (v < MinIntesity ? MinIntesity : v);
  v = (v > MaxIntesity ? MaxIntesity : v);
  return v;
}

double valueBrightness(double rValue) {
  return IntesityValue(rValue, MAXSCALE_ANALOG, VALUE_RPHOTO_10LUX, VALUE_PART_R2, VALUE_GAMMA, MAX_VALUE_LUX, MIN_VALUE_LUX, MAX_INTENSITY, MIN_INTENSITY);
}

double valueLux(double rValue) {

  return ReadLux(rValue, MAXSCALE_ANALOG, VALUE_RPHOTO_10LUX, VALUE_PART_R2, VALUE_GAMMA);
}


int daySun(int nMonth, int nYear, bool firstSun = false) {
  int nDay = 0;
  struct tm timeinfo, *ti;
  timeinfo.tm_hour = timeinfo.tm_min = timeinfo.tm_sec = 0;
  timeinfo.tm_mon = nMonth - 1;
  timeinfo.tm_mday = 1;
  timeinfo.tm_year = (nYear > 1900 ? nYear - 1900 : nYear);
  time_t t = mktime(&timeinfo);
  //printf("Start day  %.24s\n",ctime(&t));
  for (long i = 0; i < 31; i++) {
    t += 86400;
    //printf("check day  %.24s\n",ctime(&t));
    ti = gmtime(&t);
    if (ti->tm_wday == 0 && ti->tm_mon == nMonth - 1) {
      nDay = ti->tm_mday;
      if (firstSun) break;
    }
  }
  return nDay;
}

bool CheckDaylight(time_t t) {
  struct tm* ti;
  bool ret = false;
  ti = gmtime(&t);
  int mon = ti->tm_mon + 1;
  int year = ti->tm_year;
  int hour = ti->tm_hour;
  int mday = ti->tm_mday;
  if (mon > 3 && mon < 10) ret = true;
  else if (mon == 3) {
    int d = daySun(mon, year);
    ret = true;
    if (mday < d) ret = false;
    else if (hour < 2) ret = false;
  } else if (mon == 10) {
    int d = daySun(mon, year);
    ret = false;
    if (mday < d) ret = true;
    else if (hour < 2) ret = true;
  } else ret = false;
  return ret;
}

uint16_t decBrightness(uint16_t colorRGB565, float brightness) {

  uint16_t r5 = (colorRGB565 >> 11) & 0x1F;
  uint16_t g6 = (colorRGB565 >> 5) & 0x3F;
  uint16_t b5 = colorRGB565 & 0x1F;

  // Converte a 8 bit per maggiore precisione
  uint16_t r8 = (r5 * 527 + 23) >> 6;  // 5 bit -> 8 bit
  uint16_t g8 = (g6 * 259 + 33) >> 6;  // 6 bit -> 8 bit
  uint16_t b8 = (b5 * 527 + 23) >> 6;  // 5 bit -> 8 bit

  // Applica la luminosità
  r8 = (uint16_t)(r8 * brightness);
  g8 = (uint16_t)(g8 * brightness);
  b8 = (uint16_t)(b8 * brightness);

  // Limita e riconverte al formato originale
  if (r8 > 255) r8 = 255;
  if (g8 > 255) g8 = 255;
  if (b8 > 255) b8 = 255;

  r5 = (r8 * 249 + 1014) >> 11;  // 8 bit -> 5 bit
  g6 = (g8 * 253 + 505) >> 10;   // 8 bit -> 6 bit
  b5 = (b8 * 249 + 1014) >> 11;  // 8 bit -> 5 bit

  // Ricombina
  return ((r5 << 11) | (g6 << 5) | b5);
}



MemoAcqData::MemoAcqData(uint16_t _maxData, uint16_t _secAcq, uint16_t _molt) {
  maxData = 0;
  secAcq = _secAcq;
  molt = _molt;
  clearData();
  if ((data = (int16_t*)malloc(sizeof(int16_t) * _maxData)) != NULL) maxData = _maxData;
}

MemoAcqData::~MemoAcqData() {
  if (data != NULL) free(data);
}

void MemoAcqData::clearData(void) {
  addValue = 0.0;
  lastAcq = nAdd = mData = pData = 0;
}

bool MemoAcqData::putAcqData(float value) {
  if (data == NULL) return false;
  time_t tm;
  time(&tm);
  if (lastAcq == 0) lastAcq = tm + secAcq;
  if (lastAcq > tm) {
    addValue += value;
    nAdd++;
  } else {
    lastAcq += secAcq;
    if (nAdd) {
      addValue /= (double)nAdd;
      int16_t v = (int16_t)(molt * addValue);
      addValue = nAdd = 0;
      *(data + pData) = v;
      (++pData) %= maxData;
      if (mData < maxData) mData++;
    }
  }
  return true;
}

bool MemoAcqData::getAcqData(uint16_t p, float& value) {
  value = 0;
  if (data == NULL || !mData || p >= mData) return false;
  int16_t v;
  if (mData < maxData) v = *(data + p);
  else {
    p += pData;
    p %= maxData;
    v = *(data + p);
  }
  value = (float)v / (float)molt;
  return true;
}

bool MemoAcqData::getArrayValue(float* vArray, float& valueMin, uint16_t& posMin, float& valueMax, uint16_t& posMax) {
  uint16_t p, sp;
  float v;
  valueMax = valueMin = posMin = posMax = 0;
  if (data == NULL || !mData) return false;
  valueMin = 1e99;
  valueMax = -1e99;
  if (mData < maxData) sp = 0;
  else sp = pData;
  for (int i = 0; i < mData; i++) {
    p = sp + i;
    p %= maxData;
    v = (float)*(data + p) / (float)molt;
    *(vArray + i) = v;
    if (valueMin > v) {
      valueMin = v;
      posMin = i;
    }
    if (valueMax < v) {
      valueMax = v;
      posMax = i;
    }
  }
  return true;
}

long MemoAcqData::secNextAcq(void) {
  time_t tm;
  time(&tm);
  long r = (lastAcq > 0 ? lastAcq - tm : 0);
  return (r > 0 ? r : 0);
}


time_t doCalcDayEaster(int yyyy) {
  struct tm t;
  short a = yyyy % 19;
  short b = yyyy % 4;
  short c = yyyy % 7;
  short M, N, dd, mm;
  switch (yyyy / 100) {
    case 16:
      M = 22;
      N = 2;
      break;
    case 17:
      M = 23;
      N = 3;
      break;
    case 18:
      M = 23;
      N = 4;
      break;
    case 19:
    case 20:
      M = 24;
      N = 5;
      break;
    case 21:
      M = 24;
      N = 6;
      break;
  }
  short d = (19 * a + M) % 30;
  short e = (2 * b + 4 * c + 6 * d + N) % 7;
  if ((d + e) < 10) {
    dd = d + e + 22;
    mm = 3;
  } else {
    dd = d + e - 9;
    if (dd == 26) dd = 19;
    else if (dd == 25 && d == 28 && e == 6 && a > 10) dd = 18;
    mm = 4;
  }
  t.tm_mday = dd;
  t.tm_mon = mm - 1;
  t.tm_year = yyyy - 1900;
  t.tm_sec = t.tm_min = t.tm_hour = 0;
  return mktime(&t);
}


time_t CalcDayEaster(time_t ltm) {
  struct tm* t;
  t = gmtime(&ltm);
  int yy = t->tm_year + 1900;
  int a = -1;
  long d;
  time_t tEaster;
  do {
    a++;
    tEaster = doCalcDayEaster(yy + a);
  } while ((d = (int)difftime(tEaster, ltm)) < 0);
  return tEaster;
}

char* strHourMin(time_t t, bool bSec) {
  static char buff[16];
  struct tm* ptm;
  ptm = gmtime(&t);
  if (bSec) sprintf(buff, "%2.2d:%2.2d:%2.2d", ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
  else sprintf(buff, "%2.2d:%2.2d", ptm->tm_hour, ptm->tm_min);
  return buff;
}

char* strDay(time_t t) {
  static char buff[16];
  struct tm* ptm;
  ptm = gmtime(&t);
  //sprintf(buff, "%2.2d/%2.2d/%2.2d", ptm->tm_mday, ptm->tm_mon + 1, ptm->tm_year - 100);
  sprintf(buff, "%2.2d/%2.2d/%2.2d", ptm->tm_year - 100, ptm->tm_mon + 1, ptm->tm_mday);
  return buff;
}

time_t nowTime(void) {
  time_t t;
  time(&t);
  return t;
}

int julianDay(time_t ltm) {
  struct tm* attD;
  attD = gmtime(&ltm);
  // struct tm initD = *attD;
  // initD.tm_mday = 1;
  // initD.tm_mon = initD.tm_hour = initD.tm_min = initD.tm_sec = 0;
  // time_t ti = mktime(&initD);
  // return (difftime(ltm, ti) / 86400 + 1);
  return attD->tm_yday + 1;
}

long calcOffSetTime(void) {
  long r = DataPrg.utcOffset;
  if (DataPrg.TyDaylight) r += (IsDaylight == true ? 1 : 0) * 3600;
  return r;
}

char* strTimeAll(time_t t, bool OnlyDay) {
  static char buff[48];
  struct tm* ptm;
  ptm = gmtime(&t);
  if (OnlyDay) snprintf(buff, 48, "%s %2.2d %s %4.4d", daysOfTheWeek[ptm->tm_wday], ptm->tm_mday, monthsOfTheYear[ptm->tm_mon], ptm->tm_year + 1900);
  else snprintf(buff, 48, "%s %2.2d %s %4.4d %2.2d:%2.2d:%2.2d", daysOfTheWeek[ptm->tm_wday], ptm->tm_mday, monthsOfTheYear[ptm->tm_mon],
                ptm->tm_year + 1900, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
  //Serial.printf("%s\n",buff);
  return buff;
}

void setDataSystem(time_t tm) {
  struct timeval tv;
  tv.tv_sec = tm;
  tv.tv_usec = 0;
  settimeofday(&tv, nullptr);
}

void setTimeClock(void) {
  time_t tm = readDateTimeDs3231();
  time_t ltm = localTime(tm);
  if (DataPrg.TyDaylight == 0) IsDaylight = false;
  else if (DataPrg.TyDaylight == 1) IsDaylight = true;
  else IsDaylight = CheckDaylight(ltm);
  setDataSystem(tm);
}

bool SaveData(void) {
  uint8_t buff[MAX_LEN_FILE_DATA];
  uint8_t* s = (uint8_t*)&DataPrg;
  int i;
  for (i = 0; i < MAX_LEN_FILE_DATA; i++) buff[i] = 0;
  for (i = 0; i < sizeof(DataPrg); i++, s++) buff[i] = *s;
  return WriteData(NameFileData, buff, MAX_LEN_FILE_DATA);
}

bool LoadData(void) {
  bool ret = false;
  uint8_t buff[MAX_LEN_FILE_DATA];
  uint8_t* s = (uint8_t*)&DataPrg;
  if (ReadData(NameFileData, buff, MAX_LEN_FILE_DATA)) {
    for (int i = 0; i < sizeof(DataPrg); i++, s++) *s = buff[i];
    ret = true;
  }
  return ret;
}


void PrintData(void) {
  Serial.printf("DataPrg.ssid %s\n", DataPrg.ssid);
  Serial.printf("DataPrg.password %s\n", DataPrg.password);
  Serial.printf("DataPrg.lat %.2f\n", DataPrg.lat);
  Serial.printf("DataPrg.log %.2f\n", DataPrg.log);
  Serial.printf("DataPrg.height %.2f\n", DataPrg.height);
  Serial.printf("DataPrg.utcOffset %d\n", DataPrg.utcOffset);
  Serial.printf("DataPrg.TyDaylight %d\n", DataPrg.TyDaylight);
  Serial.printf("DataPrg.OnWifi %d\n", DataPrg.OnWifi);
  Serial.printf("DataPrg.Vol %.0f\n", DataPrg.Vol);
  Serial.printf("DataPrg.onSoundBell %d\n", DataPrg.onSoundBell);
  Serial.printf("DataPrg.onSoundCucu %d\n", DataPrg.onSoundCucu);
  Serial.printf("DataPrg hourStartSound %2.2lld:%2.2lld:%2.2lld\n", DataPrg.hourStartSound / 3600, (DataPrg.hourStartSound % 3600) / 60, DataPrg.hourStartSound % 60);
  Serial.printf("DataPrg hourEndSound %2.2lld:%2.2lld:%2.2lld\n", DataPrg.hourEndSound / 3600, (DataPrg.hourEndSound % 3600) / 60, DataPrg.hourEndSound % 60);
  Serial.printf("DataPrg hourAlarm %2.2lld:%2.2lld:%2.2lld\n", DataPrg.hourAlarm / 3600, (DataPrg.hourAlarm % 3600) / 60, DataPrg.hourAlarm % 60);
  Serial.printf("DataPrg.AlarmOn  %d\n", DataPrg.AlarmOn);
  Serial.printf("DataPrg.OffAlarmWeek  %d\n", DataPrg.OffAlarmWeek);
  Serial.printf("DataPrg.nCycleAlarm  %d\n", DataPrg.nCycleAlarm);
  Serial.printf("DataPrg.longRingSec  %d\n", DataPrg.longRingSec);
  Serial.printf("DataPrg.longRipAlarmSec  %d\n", DataPrg.longRipAlarmSec);
}