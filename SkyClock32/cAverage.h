#ifndef C_AVERAGE_H
#define C_AVERAGE_H

#include <Arduino.h>

class cAverage {
private:
  float *value;
  uint16_t piz;
  uint16_t ap;
  uint16_t maxV;
public:
  cAverage(uint16_t maxValue)
    : value(nullptr), piz(0), ap(0), maxV(0) { setVlMd(maxValue); }
  ~cAverage() {
    if (value) free(value);
  };
  void setVlMd(uint16_t maxValue) {
    if (value) free(value);
    maxV = maxValue;
    value = (float *)malloc(maxV * sizeof(float));
    clear();
  }
  void clear(void) {
    if (value) {
      piz = ap = 0;
      for (int i = 0; i < maxV; i++) *(value + i) = 0;
    }
  }
  void pushValue(float v) {
    if (value) {
      *(value + ap) = v;
      if (piz < maxV) piz++;
      ap++;
      ap %= maxV;
    }
  }
  float retMd(void) {
    float ret = 0;
    if (value && piz) {
      float v = 0;
      for (int i = 0; i < piz; i++) v += *(value + i);
      ret = v / (double)piz;
    }
    return ret;
  }
  float retStd(void) {
    float ret = 0;
    if (value && piz) {
      float v = 0;
      float md = retMd();
      for (int i = 0; i < piz; i++) v += (*(value+i) - md) * (*(value+i) - md);
      ret= sqrt(v / (double)piz);
    }
    return ret;
  }
};

#endif
