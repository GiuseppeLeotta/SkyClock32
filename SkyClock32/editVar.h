#ifndef _EDIT_VAR_H_
#define _EDIT_VAR_H_

#include "dispILI9488.h"
#include "structDataPrg.h"

extern dispILI9488 myDisp;

extern const uint16_t backColor;
extern const uint16_t foreColor;
extern tagDataPrg DataPrg;


extern const uint16_t MAX_COL;
extern const uint16_t MAX_ROW;

typedef void (*CallbackEdit)(bool RorW,double & value);

struct EditVal {
  uint16_t tyVal;  //  0->double, 1->string 
  double value;
  double vMin;
  double vMax;
  double step;
  char **str;
  CallbackEdit editV;
  uint16_t reslh;
};

int SetEditVar(int& stato, bool onButton, uint32_t tButt);

#endif 