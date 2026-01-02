#include <Arduino.h>
#include <math.h>
#include "editVar.h"
#include "drawDisp.h"
#include "utility.h"


//#include "Fonts/FreeMonoBold12pt8b.h"
//#include "Fonts/FreeMonoBold9pt8b.h"

#define TIME_SET_VALUE 1000
#define TIME_SET_BACK 2000
#define TIME_SET_RETURN 3500

int SelectVarButton(unsigned long t, int& act) {
  static int oldAct = 0;
  if (t == 0) oldAct = act = 0;
  if (t >= TIME_SET_VALUE && act == 0) act++;
  if (t >= TIME_SET_BACK && act == 1) act++;
  if (t >= TIME_SET_RETURN && act == 2) act++;
  if (oldAct != act) {
    oldAct = act;
    return act;
  }
  return 0;
}

int16_t calcLenX(drawDisp& pS, char* sForm, struct EditVal& eVal) {
  uint16_t lw, lh;
  char buff[64];
  for (double v = eVal.vMin; v < eVal.vMax; v += eVal.step) {
    if (eVal.tyVal == 0) snprintf(buff, 64, sForm, v);
    else snprintf(buff, 64, "%s", eVal.str[(int)v]);
    pS.boxString(buff, lw, lh);
    eVal.reslh = (eVal.reslh < lw ? lw : eVal.reslh);
  }
  return eVal.reslh;
}

int dispEditNum(int16_t x0, int16_t y0, bool nButt, int16_t eSet, char* label, char* sForm, struct EditVal& eVal, int16_t& xRowLin) {
  drawDisp pS(&myDisp, Font09, foreColor, backColor);
  uint16_t lw, lh;
  char buff[64];
  if (eVal.editV != NULL && eSet == -1) eVal.editV(false, eVal.value);
  if (nButt) eVal.value += eVal.step;
  eVal.value = (eVal.value < eVal.vMin ? eVal.vMin : eVal.value);
  eVal.value = (eVal.value >= eVal.vMax ? eVal.vMin : eVal.value);
  if (eVal.tyVal == 0) snprintf(buff, 64, sForm, eVal.value);
  else snprintf(buff, 64, "%s", eVal.str[(int)eVal.value]);
  pS.boxString(label, lw, lh);
  //pS.drawString(label);
  pS.drawPrint(x0, y0,label);
  x0 += lw;
  //pS.boxString(buff, lw, lh);
  if (eSet == -1) calcLenX(pS, sForm, eVal);
  xRowLin = x0 + eVal.reslh;
  //eVal.reslh = (eVal.reslh < lw ? lw : eVal.reslh);
  if (!eSet) pS.setColor(RGB565_LIGHT_GREEN, backColor);
  if (nButt) myDisp.fillScreen(x0, y0, x0 + eVal.reslh, y0 + lh, backColor);
  if (eVal.editV != NULL && eSet == 1) eVal.editV(true, eVal.value);
  //pS.drawString(buff);
  pS.drawPrint(x0, y0,buff);
  return eSet;
}




#define NUM_EDIT 13


void ednCycleAlarm(bool RorW, double& value) {
  if (RorW) DataPrg.nCycleAlarm = (int) value;
  else value = DataPrg.nCycleAlarm;
}

void edOffAlarmWeek(bool RorW, double& value) {
  if (RorW) DataPrg.OffAlarmWeek = (value ? true : false);
  else value = (DataPrg.OffAlarmWeek ? 1 : 0);
}

void edAlarmOn(bool RorW, double& value) {
  if (RorW) DataPrg.AlarmOn = (value ? true : false);
  else value = (DataPrg.AlarmOn ? 1 : 0);
}

void edHourAlarmH(bool RorW, double& value) {
  //if (RorW) DataPrg.hourAlarm = value + fmod(DataPrg.hourAlarm, (int)DataPrg.hourAlarm);
  //else value = (int)DataPrg.hourAlarm;
  if (RorW) DataPrg.hourAlarm = value*3600+DataPrg.hourAlarm%3600;
  else value = DataPrg.hourAlarm/3600;
}

void edHourAlarmM(bool RorW, double& value) {
  //if (RorW) DataPrg.hourAlarm = ((int)(DataPrg.hourAlarm)) + value / 60.0;
  //else value = (int)(fmod(DataPrg.hourAlarm, (int)DataPrg.hourAlarm) * 60.0);
  if (RorW) DataPrg.hourAlarm = 3600*(DataPrg.hourAlarm/3600) + value * 60;
  else value = (DataPrg.hourAlarm%3600)/60;
}

void edOnSoundBell(bool RorW, double& value) {
  if (RorW) DataPrg.onSoundBell = (value ? true : false);
  else value = (DataPrg.onSoundBell ? 1 : 0);
}

void edOnSoundCucu(bool RorW, double& value) {
  if (RorW) DataPrg.onSoundCucu = (value ? true : false);
  else value = (DataPrg.onSoundCucu ? 1 : 0);
}

void edDayLight(bool RorW, double& value) {  // bool RorW -> false=read, true=write
  if (RorW) DataPrg.TyDaylight = (uint8_t)value;
  else value = DataPrg.TyDaylight;
}

void edStartSoundH(bool RorW, double& value) {
  //if (RorW) DataPrg.hourStartSound = value + fmod(DataPrg.hourStartSound, (int)DataPrg.hourStartSound);
  //else value = (int)DataPrg.hourStartSound;
  if (RorW) DataPrg.hourStartSound = value*3600+DataPrg.hourStartSound%3600;
  else value = DataPrg.hourStartSound/3600;
}

void edStartSoundM(bool RorW, double& value) {
  //if (RorW) DataPrg.hourStartSound = ((int)(DataPrg.hourStartSound)) + value / 60.0;
  //else value = (int)(fmod(DataPrg.hourStartSound, (int)DataPrg.hourStartSound) * 60.0);
  if (RorW) DataPrg.hourStartSound = 3600*(DataPrg.hourStartSound/3600) + value * 60;
  else value = (DataPrg.hourStartSound%3600)/60;
}

void edEndSoundH(bool RorW, double& value) {
  //if (RorW) DataPrg.hourEndSound = value + fmod(DataPrg.hourEndSound, (int)DataPrg.hourEndSound);
  //else value = (int)DataPrg.hourEndSound;
  if (RorW) DataPrg.hourEndSound = value*3600+DataPrg.hourEndSound%3600;
  else value = DataPrg.hourEndSound/3600;
}

void edEndSoundM(bool RorW, double& value) {
  //if (RorW) DataPrg.hourEndSound = ((int)(DataPrg.hourEndSound)) + value / 60.0;
  //else value = (int)(fmod(DataPrg.hourEndSound, (int)DataPrg.hourEndSound) * 60.0);
  if (RorW) DataPrg.hourEndSound = 3600*(DataPrg.hourEndSound/3600) + value * 60;
  else value = (DataPrg.hourEndSound%3600)/60;
}

void edVolume(bool RorW, double& value) {
  //if (RorW) DataPrg.hourEndSound = ((int)(DataPrg.hourEndSound)) + value / 60.0;
  //else value = (int)(fmod(DataPrg.hourEndSound, (int)DataPrg.hourEndSound) * 60.0);
  if (RorW) DataPrg.Vol = value;
  else value =  DataPrg.Vol;
}

//char* strDayLigh[] = { "Solar", "Legal", "Auto (European method)" };

char* strOnOff[] = { "Off", "On" };

EditVal ev[NUM_EDIT] = { 1, 0, 0, 3, 1, strDayLigh, edDayLight, 0,
                         0, 0, 0, 24, 1, NULL, edStartSoundH, 0,
                         0, 0, 0, 60, 10, NULL, edStartSoundM, 0,
                         0, 0, 0, 24, 1, NULL, edEndSoundH, 0,
                         0, 0, 0, 60, 10, NULL, edEndSoundM, 0,
                         1, 0, 0, 2, 1, strOnOff, edOnSoundBell, 0,
                         1, 0, 0, 2, 1, strOnOff, edOnSoundCucu, 0,
                         1, 0, 0, 2, 1, strOnOff, edAlarmOn, 0, 
                         0, 0, 0, 24, 1, NULL, edHourAlarmH, 0,
                         0, 0, 0, 60, 5, NULL, edHourAlarmM, 0,
                         1, 0, 0, 2, 1, strOnOff, edOffAlarmWeek, 0, 
                         0, 1, 1, 4, 1, NULL, ednCycleAlarm, 0,
                         0, 100, 0, 200, 10, NULL, edVolume, 0};





#define sX0 15
#define sY0 35
#define yLine 28

int16_t x0Arr[NUM_EDIT] = { sX0,
                            sX0, 0, 0, 0,
                            sX0, 0,
                            sX0, 0, 0,
                            sX0, 0,
                            sX0};

int16_t y0Arr[NUM_EDIT] = { sY0,
                            sY0 + yLine * 1, sY0 + yLine * 1, sY0 + yLine * 1, sY0 + yLine * 1,
                            sY0 + yLine * 2, sY0 + yLine * 2,
                            sY0 + yLine * 3, sY0 + yLine * 3, sY0 + yLine * 3,
                            sY0 + yLine * 4, sY0 + yLine * 4,
                            sY0 + yLine * 5 };


char* sLabArr[] = { "Set DayLigh:",
                    "Enable sound from: ", ":", " to ", ":",
                    "Enable bell sound: ", "  CuCu sound: ",
                    "Alarm: ", "  Set the alarm for: ", ":",
                    "Weekends alarm: ", "  Alarm snooze:",
                    "Set Volume: "};

char* sFormatArr[] = { "",
                       "%02.0f", "%02.0f", "%02.0f", "%02.0f",
                       "", "",
                       "", "%02.0f", "%02.0f",
                       "", "%2.0f",
                       "%3.0f" };


int16_t setXlen(int i, int16_t xLen[]) {
  int16_t x;
  x = x0Arr[i];
  if (i == 2) x = xLen[1];
  if (i == 3) x = xLen[2];
  if (i == 4) x = xLen[3];
  if (i == 6) x = xLen[5];
  if (i == 8) x = xLen[7];
  if (i == 9) x = xLen[8];
  if (i == 11) x = xLen[10];
  return x;
}

int SetEditVar(int& stato, bool onButton, uint32_t tButt) {
  if (stato==0) return 0;
  drawDisp pS(&myDisp, Font12, foreColor, backColor);
  uint16_t lw, lh;
  char buff[64];
  strcpy(buff, "Set Values");
  //pS.drawString(buff);
  pS.boxString(buff, lw, lh);
  pS.drawPrint((MAX_ROW - lw) / 2, 2,buff);

  static int act;
  static int p = 0;
  static int16_t xLen[NUM_EDIT];
  int16_t x;
  if (tButt > 400) onButton = false;
  if (stato == 1) {
    myDisp.clearScreen(backColor);
    stato = 2; 
    p = 0;
    for (int i = 0; i < NUM_EDIT; i++) {
      x = setXlen(i, xLen);
      dispEditNum(x, y0Arr[i], false, -1 , sLabArr[i], sFormatArr[i], ev[i], xLen[i]);
    }
  } else if (stato==2) {
    int a = SelectVarButton(tButt, act);
    if (a == 3) {
      //Serial.printf("%f %f\n", DataPrg.hourStartSound, DataPrg.hourEndSound);
      myDisp.clearScreen(backColor);
      SaveData();
      PrintData();
      return 0;
    }
    x = setXlen(p, xLen);
    if (dispEditNum(x, y0Arr[p], onButton, a, sLabArr[p], sFormatArr[p], ev[p], xLen[p])) {
      if (a==1) p++;
      else p-=2;
      p=(p<0 ? NUM_EDIT-1 : p);
      p %= NUM_EDIT;
    }
  }
  return 1;
}