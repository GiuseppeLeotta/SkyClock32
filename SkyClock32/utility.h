#ifndef H_UTILITY_H
#define H_UTILITY_H

#include <arduino.h>
#include <time.h>
#include "structDataPrg.h"


#include "Fonts/FreeMonoBold9pt8b.h"
#include "Fonts/FreeMonoBold12pt8b.h"
#include "Fonts/FreeMonoBold24pt8b.h"
#include "Fonts/FreeMonoBold18pt8b.h"

extern tagDataPrg DataPrg;
extern bool IsDaylight;  // True is Daylight
extern char* strDayLigh[];
extern const time_t TIME_2010_01_01;

extern const char * strReduction;

extern const GFXfont* Font09;
extern const GFXfont* Font12;
extern const GFXfont* Font18;
extern const GFXfont* Font24;


#define MAXSCALE_ANALOG 3.3
#define VALUE_PART_R2 47        // in kohm
#define VALUE_RPHOTO_10LUX 300  // in kohm
#define VALUE_GAMMA 0.5
#define RESOL_ANALOG 4096
#define MAX_INTENSITY 100  // valore massimo intensità disp
#define MIN_INTENSITY 5    // valore minimointensità disp
#define MAX_VALUE_LUX 500  // Valore per la massima luminosita
#define MIN_VALUE_LUX 10   // Valore per la minima intesita




double dig2Volt(uint16_t value);
double valueBrightness(double rValue);
double valueLux(double rValue);

uint16_t decBrightness(uint16_t colorRGB565, float brightness);

bool CheckDaylight(time_t t);


time_t CalcDayEaster(time_t ltm);


char* strHourMin(time_t t, bool bSec = false);
char* strDay(time_t t);
time_t nowTime(void);
int julianDay(time_t ltm);
long calcOffSetTime(void);
char* strTimeAll(time_t t, bool OnlyDay = false);
void setDataSystem(time_t tm);
void setTimeClock(void);
bool SaveData(void);
bool LoadData(void);

inline time_t localTime(time_t tm) {
  return tm + calcOffSetTime();
}

inline double localTimeJD(double jd) {
  return jd + (double)calcOffSetTime() / 86400.0;
}

inline int attHour(time_t tm) {
  return (tm % 86400) / 3600;
}

inline int attMin(time_t tm) {
  return (tm % 3600) / 60;
}

inline bool isNoon(time_t tm) {
  return (tm % 86400 == 43200);
}

void PrintData();

/* ===== Class waitTimer ==== */
class waitTimer {
private:
  uint32_t memSec;
  uint32_t contSec;
  bool inRun;
public:
  waitTimer(uint32_t periodTime = 0)
    : memSec(periodTime), inRun(false) {}
  ~waitTimer() {}
  void start(uint32_t periodTime = 0) {
    if (periodTime) memSec = periodTime;
    if (memSec) {
      inRun = true;
      contSec = millis();
    }
  }
  bool run() {
    bool ret = false;
    if (inRun) {
      if (millis() - contSec >= memSec) inRun = false;
      else ret = true;
    }
    return ret;
  }
};

class MemoAcqData {
private:
  uint16_t maxData;  // numero max array
  uint16_t mData;    // num acq data
  uint16_t pData;    // pos attuale data in array
  uint16_t molt;     // moltiplicatore. deve andare a step di potenza del 10: 1,10,100...ect
  int16_t *data;     // array
  uint16_t secAcq;   // acquisizione ogni secAcq secondi
  time_t lastAcq;    // ultima acq;
  double addValue;   // valore di accumulo
  uint16_t nAdd;     // numero di valori accumulati
  // float vMin;        // valore minimo
  // float vMax;        // valore massimo
  // uint16_t pMax;     // posizione v Max
  // uint16_t pMin;     // posizione v Min
public:
  MemoAcqData(uint16_t _maxData, uint16_t _secAcq, uint16_t molt);
  ~MemoAcqData();
  void clearData(void);
  bool putAcqData(float value);
  bool getAcqData(uint16_t p, float &value);
  inline uint16_t numPoint(void) const {
    return mData;
  };
  long secNextAcq(void);
  bool getArrayValue(float *vArray, float & valueMin, uint16_t &posMin, float & valueMax, uint16_t &posMax);
  inline time_t timeLastAcq(void) const {
    return (lastAcq > 0 ? lastAcq - secAcq : 0);
  };
  inline int16_t nDecimal(void) const {
    return molt / 10;
  };
};

#endif