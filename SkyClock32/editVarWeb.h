#ifndef _EDIT_VAR_WEB_H_
#define _EDIT_VAR_WEB_H_

#include "dispILI9488.h"
#include "structDataPrg.h"

// Struttura per gestire i campi configurabili
struct ConfigField {
  enum Type { BOOL, LONG, DOUBLE, STRING, TIME, DATE, MENU };
  Type type;
  String label;
  void* variable;
  size_t maxLen; // Solo per stringhe
  double minVal; // Valore minimo (per numeri)
  double maxVal; // Valore massimo (per numeri)
  char** options;  // Per il tipo MENU
  int numOptions;        // Per il tipo MENU
};

extern dispILI9488 myDisp;

extern const uint16_t backColor;
extern const uint16_t foreColor;
extern tagDataPrg DataPrg;

extern const uint16_t MAX_COL;
extern const uint16_t MAX_ROW;


int SetEditVarWeb(int& stato, bool onButton, uint32_t tButt, time_t ltm);



#endif
