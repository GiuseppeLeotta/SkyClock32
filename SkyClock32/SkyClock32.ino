#include <arduino.h>
#include <time.h>
#include <SPI.h>
#include <WiFi.h>
#include <WiFiUdp.h>



#include "dispILI9488.h"
#include "drawDisp.h"
#include "drawClock.h"
#include "NTPclient.h"
#include "FileData.h"
#include "Button.h"
#include "PlayMp3.h"
#include "uRandom.h"
#include "ds3231.h"
#include "BMP180.h"
#include "SHT30.h"
#include "SolarSys.h"
#include "cAverage.h"
#include "editVar.h"
#include "utility.h"
#include "structDataPrg.h"
#include "editVarWeb.h"


#include "gImage.h"
#include "dataTermoImg.h"
#include "dataUmiImg.h"
#include "dataPressImg.h"
#include "dataSunImg.h"
#include "dataMoonImg.h"
#include "dataMercuryImg.h"
#include "dataVenusImg.h"
#include "dataMarsImg.h"
#include "dataJupiterImg.h"
#include "dataSaturnImg.h"


const float Version = 1.01;  // SkyClock version

#define I2S_SCK 26
#define I2S_WS 25
#define I2S_OUT 22

#define SPI_CLK 18
#define SPI_MOSI 23
#define SPI_CS 5
#define SPI_RST 4
#define SPI_DS 19

#define I2C_SDA 21
#define I2C_SCL 32

#define PIN_DISP_LED 16

#define PIN_BUTT 14

#define PIN_PHOTO_R 34

#define PWM_FREQ 5000
#define PWM_RESOLUTION 8


const uint16_t MAX_COL = 320;
const uint16_t MAX_ROW = 480;

const char* strReduction = " .";
const GFXfont* Font09 = &FreeMonoBold9pt8b;
const GFXfont* Font12 = &FreeMonoBold12pt8b;
const GFXfont* Font18 = &FreeMonoBold18pt8b;
const GFXfont* Font24 = &FreeMonoBold24pt8b;

#define NAME_FILE_CUCU_PLAY "/as/CUCUCLK.mp3"
#define NAME_FILE_ALARM_CLOCK "/as/alarmClock1.mp3"
#define NAME_FILE_INIT_SOUND "/as/InitSound.mp3"


char* strDayLigh[] = { "Solar", "Legal", "Auto (European method)" };


// press [Ctrl] + [Shift] + [P] to open the command palette

typedef void (*CallbackPos)(double jd, Observer& obs, CelestialPosition& pos);
typedef int (*CallbackRiseSet)(double jd, Observer& obs, const double horizon, RiseSet& Twilight);

tagDataPrg DataPrg;

Observer obs;

#define TIME_SYNC_CLOCK (24 * 60 * 60UL * 1000UL)

// sync ogni 24 ore, timeout risposta 5 secondi
NTPClientNB ntp("pool.ntp.org", 123, TIME_SYNC_CLOCK, 5000);

SPIClass* vspi = new SPIClass(VSPI);

dispILI9488 myDisp(vspi, MAX_COL, MAX_ROW, SPI_DS, SPI_CS, SPI_RST);

#define BMP180_ADDR 0x77

BMP180 bm180(Wire, BMP180_ADDR, 3);
SHT30 sht(Wire);

uRandom urn;

#define NUM_POINT_ACQ 144
#define TIME_POINT_ACQ 600


MemoAcqData tempAmb(NUM_POINT_ACQ, TIME_POINT_ACQ, 10);
MemoAcqData umiAmb(NUM_POINT_ACQ, TIME_POINT_ACQ, 1);
MemoAcqData pressAmb(NUM_POINT_ACQ, TIME_POINT_ACQ, 1);

float tempExt = 0;
float umiExt = 0;
float pressExt = 0;
float tempInt = 0;
float luxExt = 0;
float tempClock = 0;
bool ErrTemp = false;
bool ErrPress = false;

const uint16_t backColor = 0x0bb4;
const uint16_t foreColor = RGB565_WHITE;

const uint16_t colorStrSet = RGB565_ORANGE;
const uint16_t colorStrRise = RGB565_YELLOW;


char** listFileMp3;    // lista per file mp3
int countFileMp3 = 0;  // numero di file mpe


bool IsDaylight = false;  // True is Daylight

#define PIXEL_TICK 4
#define NUM_POINT_GRAPH 130
#define SEC_POINT_GRAPH 720
#define AXIS_GRID_TICK 5
#define AXIS_RS_LINE 3

const uint16_t ColorLineAz = RGB565_GREEN;
const uint16_t ColorLineEl = RGB565_RED;



const uint16_t nxTick = 24;
const uint16_t nyTick = 10;

#define PIX_NEWLINE 23
#define PIX_NEWLINE01 25


#define NUM_SKY_OBJ 7

typedef struct tagCelPos {
  float el[NUM_POINT_GRAPH];
  float az[NUM_POINT_GRAPH];
  double t[NUM_POINT_GRAPH];
  time_t tm;
} CelPos;

CelPos plotCelPos[NUM_SKY_OBJ];


void PlotCelestialPosition(uint16_t x0, uint16_t y0, uint16_t lx, uint16_t ly, time_t tm, CallbackPos cb, RiseSet& TwGraph, CelPos& pC) {
  drawDisp axis(&myDisp, Font09, foreColor, backColor);
  uint16_t x1 = lx - 1, y1 = ly - 1;
  axis.axis(lx, ly, nxTick, nyTick, PIXEL_TICK, AXIS_GRID_TICK);

  CelestialPosition pos;

  if (tm - pC.tm > 360) {
    pC.tm = tm;
    for (int i = 0; i < NUM_POINT_GRAPH; i++) {
      pC.t[i] = tm - 13 * 3600 + SEC_POINT_GRAPH * i;
      double jd = time2jd(pC.t[i]);
      //SunPosition(jd, obs, pos);
      cb(jd, obs, pos);
      pC.el[i] = pos.el;
      pC.az[i] = (pos.az > 180 ? 360 - pos.az : pos.az);
      pC.t[i] = localTime(pC.t[i]);
    }
  }

  axis.plot(NUM_POINT_GRAPH, pC.t, pC.el, pC.t[0], pC.t[NUM_POINT_GRAPH - 1], -90, 90, ColorLineEl, 2);
  axis.plot(NUM_POINT_GRAPH, pC.t, pC.az, pC.t[0], pC.t[NUM_POINT_GRAPH - 1], 0, 180, ColorLineAz, 2);
  axis.refresh(x0, y0);

  drawDisp pS(&myDisp, Font09, foreColor, backColor, strReduction);
  char buff[16];
  uint16_t lw, lh, x, px, opx=0, olw=0;
  time_t tRS;
  if (TwGraph.polar == 0) {

    //Serial.printf(" rise %s\n", jd2str(localTimeJD(TwGraph.rise)));
    //Serial.printf(" transit %s\n", jd2str(localTimeJD(TwGraph.transit)));
    //Serial.printf(" set %s\n", jd2str(localTimeJD(TwGraph.set)));
    
    tRS = localTime(jd2time(TwGraph.rise));
    sprintf(buff, jd2str(localTimeJD(TwGraph.rise), true));
    pS.boxString(buff, lw, lh);
    myDisp.fillScreen(x0, y0 + ly + 1, MAX_ROW, y0 + ly + 1 + lh, backColor);

    //Serial.printf("rise %d %d\n", tRS >= pC.t[0], tRS <= pC.t[NUM_POINT_GRAPH - 1]);
    if (tRS >= pC.t[0] && tRS <= pC.t[NUM_POINT_GRAPH - 1]) {
      pS.creatCanvas(1, ly);
      pS.drawVerticalLine(0, 0, ly, AXIS_RS_LINE);
      pS.setColor(colorStrRise, backColor);
      x = pS.dScale(tRS, 0, lx - 1, pC.t[0], pC.t[NUM_POINT_GRAPH - 1]);
      //x=(x>=lx ? lx-1 : x);
      pS.refresh(x0 + x, y0);
      //sprintf(buff, jd2str(localTimeJD(TwGraph.rise), true));
      //pS.boxString(buff, lw, lh);
      //myDisp.fillScreen(0, y0 + ly + 1, MAX_ROW, y0 + ly + 1 + lh, backColor);
      //pS.drawString(buff);
      px = x0 + x;
      if (px + lw / 2 >= MAX_ROW) px = MAX_ROW - lw - 1;
      else px -= lw / 2;
      if (x0>px) px=x0+1;
      pS.drawPrint(px, y0 + ly + 1, buff);
      opx=px; olw=lw;
    }
    tRS = localTime(jd2time(TwGraph.set));
    //Serial.printf("set %d %d\n", tRS >= pC.t[0], tRS <= pC.t[NUM_POINT_GRAPH - 1]);
    if (tRS >= pC.t[0] && tRS <= pC.t[NUM_POINT_GRAPH - 1]) {
      pS.creatCanvas(1, ly);
      pS.drawVerticalLine(0, 0, ly, AXIS_RS_LINE);
      pS.setColor(colorStrSet, backColor);
      x = pS.dScale(tRS, 0, lx - 1, pC.t[0], pC.t[NUM_POINT_GRAPH - 1]);
      pS.refresh(x0 + x, y0);
      sprintf(buff, jd2str(localTimeJD(TwGraph.set), true));
      pS.boxString(buff, lw, lh);
      //pS.drawString(buff);
      px = x0 + x;
      if (px + lw / 2 >= MAX_ROW) px = MAX_ROW - lw - 1;
      else px -= lw / 2;
      if (x0>px) px=x0+1;
      if ((px>=opx || px+lw>=opx) && px<=opx+olw) pS.drawPrint(px, y0 + 4, buff);
      else pS.drawPrint(px, y0 + ly + 1, buff);
    }
  }

  pS.setColor(ColorLineAz, backColor);
  sprintf(buff, "180");
  pS.boxString(buff, lw, lh);
  //pS.drawString(buff);
  pS.drawPrint(x0 - (lw + 1), y0 + 1, buff);
  sprintf(buff, "0");
  pS.boxString(buff, lw, lh);
  //pS.drawString(buff);
  pS.drawPrint(x0 - (lw + 1), y0 + ly - (lh + 1), buff);
  pS.setColor(ColorLineEl, backColor);
  sprintf(buff, "+90");
  pS.boxString(buff, lw, lh);
  //pS.drawString(buff);
  pS.drawPrint(x0 - (lw + 1), y0 + 2 + lh, buff);
  sprintf(buff, "-90");
  pS.boxString(buff, lw, lh);
  //pS.drawString(buff);
  pS.drawPrint(x0 - (lw + 1), y0 + ly - 2 * (lh + 1), buff);
}

void GraphClock(time_t tm, uint32_t ms) {  // localtime --> ora GMT0
  static uint32_t do40 = 0;
  static time_t tm40 = 0;
  drawClock pClock(&myDisp, Font12, foreColor, backColor);
  const uint16_t cx = 125, cy = 125, radius = 124, long_tick = 8, short_tick = 4, posx = 3, posy = 33;

  if (tm != tm40) {
    do40 = 0;
    tm40 = tm;
  }
  int nCanvas = pClock.drawCircleClock(cx, cy, radius, long_tick, short_tick);
  pClock.drawHour(tm, RGB565_LIGHT_RED);
  pClock.drawMinute(tm, RGB565_LIGHT_RED);
  pClock.setCanvas(nCanvas);
  pClock.drawSecond(tm, do40);
  pClock.setPriority(nCanvas, 2);

  pClock.refresh(posx, posy);

  do40 += millis() - ms;
  do40 = (do40 > 1000 ? 0 : do40);
}


void playBell(void) {
  if (InRunMp3() == 0 && countFileMp3) {
    int16_t num;
    if (urn.hasNext()) {
      num = urn.next();
      Serial.printf("Estratto: %d (restanti: %u)\n", num, urn.remaining());
    } else {
      Serial.println("Tutti i numeri sono stati estratti.");
      Serial.println("Reset estrazioni...");
      urn.reset();
      num = urn.next();
    }
    Serial.printf("%d) name=%s\n", num, listFileMp3[num]);
    OpenMp3File(listFileMp3[num]);
  }
}

#define INTER_ACQ_DATA 15000  // millisec

bool AcqData(double rValue) {
  bool ret = false;
  static waitTimer tmAcqData(INTER_ACQ_DATA);
  if (!tmAcqData.run()) {
    tmAcqData.start();
    float p;
    auto r = sht.read();
    if (r.valid) {
      tempExt = r.temperature;
      umiExt = r.humidity;
      ErrTemp = false;
      //Serial.printf("%f %f\n",t,u);
    } else {
      Serial.printf("STH30 Error read\n");
      ErrTemp = true;
    }
    if (bm180.isConnected()) {
      tempInt = bm180.readTemperature();
      p = bm180.readPressure();
      pressExt = bm180.seaLevelPressure(p, DataPrg.height, tempExt);
      //Serial.printf("%f %f\n",tp,p);
      ErrPress = false;
    } else {
      Serial.printf("BMP180 Error read\n");
      ErrPress = true;
    }
    tempClock = readTemperatureDs3231();
    //Serial.printf("Acq data\n");
    ret = true;
  }
  luxExt = (float)valueLux(rValue);
  return ret;
}


void dpDigTime(time_t ltm) {
  drawDisp ph(&myDisp, Font18, foreColor, backColor, strReduction);
  ph.drawPrint(9, 0, strTimeAll(ltm));
}

void gPrintClock(time_t ltm) {  // localtime --> ora GMT0
  char buff[64];
  dpDigTime(ltm);
  drawDisp pS(&myDisp, Font24, foreColor, backColor, strReduction);
  sprintf(buff, "%3.1f\260", tempExt);
  //pS.drawString(buff);
  pS.drawPrint(330, 65, buff);
  sprintf(buff, "%3.1f%%", umiExt);
  //pS.drawString(buff);
  pS.drawPrint(330, 125, buff);
  sprintf(buff, "%4.0f", pressExt);
  //pS.drawString(buff);
  pS.drawPrint(330, 185, buff);
}


void gPrintRS(RiseSet Twilight) {
  char buff[64];
  uint16_t lw, lh;
  drawDisp ph(&myDisp, Font09, foreColor, backColor);
  if (Twilight.polar == 0) {
    char b1[24], b2[24];
    strcpy(b1, jd2str(localTimeJD(Twilight.transit), true));
    strcpy(b2, jd2str(localTimeJD(Twilight.set), true));
    sprintf(buff, "Sunrise %s  Noon %s  Sunset %s", jd2str(localTimeJD(Twilight.rise), true), b1, b2);
    ph.boxString(buff, lw, lh);
    //ph.drawString(buff);
    ph.drawPrint((MAX_ROW - lw) / 2, 295, buff);
  } else {
    if (Twilight.polar == 1) sprintf(buff, "circumpolar, above the horizon");
    else sprintf(buff, "circumpolar, bellow the horizon");
    ph.boxString(buff, lw, lh);
    //ph.drawString(buff);
    ph.drawPrint((MAX_ROW - lw) / 2, 295, buff);
  }
}

void gPrintData(void) {
  drawDisp ph(&myDisp, Font09, foreColor, backColor);
  myDisp.dumpImage(268, 40, &thermoImage);
  myDisp.dumpImage(273, 40 + 64 + 5, &umiImage);
  myDisp.dumpImage(259 + 5, 40 + (64 + 5) * 2, &pressImage);
  //drawDisp pSb(&myDisp, &FreeMonoBold9pt7b, foreColor, backColor);
  //ph.drawString("mbar");
  ph.drawPrint(330 + 35, 185 + 40, "mbar");
}

const time_t TIME_2010_01_01 = 1262304000;

void PrintLineRS(drawDisp* pS, char* s) {
  Serial.printf("%s", s);
  pS->printLine(s);
}



void DispBrightness(int v) {  // v = 0..100
  float vl = (255.0 * v) / 100.0;
  vl = (vl > 255 ? 255 : vl);
  vl = (vl < 0 ? 0 : vl);
  ledcWrite(PIN_DISP_LED, (int)vl);
}

void initData(void) {
  uint8_t* s = (uint8_t*)&DataPrg;
  for (int i = 0; i < sizeof(DataPrg); i++, s++) *s = 0;
  strcpy(DataPrg.ssid, "");
  strcpy(DataPrg.password, "");
  DataPrg.lat = 41.8;
  DataPrg.log = 12.5;
  DataPrg.height = 0;
  DataPrg.utcOffset = 3600;
  DataPrg.TyDaylight = 2;
  DataPrg.StartWork = 0;
  DataPrg.OnWifi = true;
  DataPrg.Vol = 100;  // 0..200
  DataPrg.onSoundBell = true;
  DataPrg.onSoundCucu = true;
  DataPrg.hourStartSound = 8.50 * 3600;
  DataPrg.hourEndSound = 20.0 * 3600;
  DataPrg.hourAlarm = 9.5 * 3600;
  DataPrg.AlarmOn = true;
  DataPrg.OffAlarmWeek = false;   // Se false sveglia off del weekend
  DataPrg.nCycleAlarm = 2;        // numero di ripetizione allarme; se è 0 alarm off
  DataPrg.longRingSec = 30;       // durata della sveglia
  DataPrg.longRipAlarmSec = 120;  // l'allarma è ripetuto dopo longRipAlarmSec se non stop
}

static waitTimer tmDisp(180 * 1000);  // display al massimo della luminosita per x sec dopo press button

void dataInitialize(void) {
  static int setEditV = 1;
  static int OffMenu = 1;
  uint32_t tButt;
  while (OffMenu) {
    bool onButton = onButtonQueque(tButt);
    OffMenu = SetEditVarWeb(setEditV, onButton, tButt, 0);
    delay(100);
  }
}

void offWiFi(void) {
  WiFi.disconnect(true);
  delay(100);
  WiFi.mode(WIFI_OFF);
}

char* ReadIp(void) {
  static char buff[64];
  if (WiFi.status() == WL_CONNECTED) sprintf(buff, "WiFi IP: %s  RSSI %d dBm", WiFi.localIP().toString().c_str(), WiFi.RSSI());
  return buff;
}

void stopPlaySound(void) {
  if (InRunMp3()) {
    while (!CloseMp3File()) RunMP3();
    while (InRunMp3()) RunMP3();
  }
}

void startPlaySoundAlr(void) {
  stopPlaySound();
  OpenMp3File(NAME_FILE_ALARM_CLOCK, 10);  // si ripete 10+1 volte circa 135 secondi
}

void setup() {
  char buff[64];
  Serial.begin(115200);
  delay(10);
  pinMode(I2S_SCK, OUTPUT);
  digitalWrite(I2S_SCK, HIGH);
  pinMode(I2S_WS, OUTPUT);
  digitalWrite(I2S_WS, HIGH);
  pinMode(I2S_OUT, OUTPUT);
  digitalWrite(I2S_OUT, HIGH);
  ledcAttach(PIN_DISP_LED, PWM_FREQ, PWM_RESOLUTION);
  DispBrightness(100);
  pinMode(PIN_PHOTO_R, INPUT);
  Serial.printf("\n\n\nSTART....\n");

  vspi->begin();
  //vspi->setFrequency(40000000);
  vspi->beginTransaction(SPISettings(80000000, MSBFIRST, SPI_MODE0));
  initButton(PIN_BUTT);
  Wire.begin(I2C_SDA, I2C_SCL);
  bm180.begin();
  sht.begin();
  myDisp.initDisp();
  myDisp.clearScreen(RGB565_BLACK);
  drawDisp pS(&myDisp, Font09, RGB565_WHITE, RGB565_BLACK, strReduction);
  pS.creatScreen(0, 0, 480, 320);
  sprintf(buff, "Start @gl610524 ver %4.2f - 2025\n", Version);
  PrintLineRS(&pS, buff);
  if (InitFileData(true) == false) {
    PrintLineRS(&pS, "No open file system... stop cpu. Need reset!\n");
    while (true) delay(100);
  }
  PrintLineRS(&pS, "Open file system!\n");
  initData();
  if (LoadData()) {
    PrintLineRS(&pS, "Read file data\n");
  } else {
    PrintLineRS(&pS, "File data empty\n");
    dataInitialize();
    //SaveData();
  }
  if (DataPrg.OnWifi) {
    waitTimer tmWiFi(60 * 1000);
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
    WiFi.begin(DataPrg.ssid, DataPrg.password);
    PrintLineRS(&pS, "Connessione WiFi...");
    delay(500);
    tmWiFi.start();
    while (WiFi.status() != WL_CONNECTED && tmWiFi.run()) {
      delay(200);
      PrintLineRS(&pS, ".");
    }
    if (tmWiFi.run()) {
      sprintf(buff, "\nConnect! %s\n", ReadIp());
      PrintLineRS(&pS, buff);
    } else {
      DataPrg.OnWifi = false;
      sprintf(buff, "connection failed, WIFI OFF!\n");
      PrintLineRS(&pS, buff);
    }
  } else PrintLineRS(&pS, "WiFI off\n");
  float tf = readTemperatureDs3231();
  sprintf(buff, "Temperature Ds3231 %3.1f\260\n", tf);
  PrintLineRS(&pS, buff);
  auto r = sht.read();
  float t = r.temperature;
  if (r.valid) {
    float u = r.humidity;
    sprintf(buff, "SHT30 Serial Number %lX\n", sht.readSerialNumber());
    PrintLineRS(&pS, buff);
    //sprintf(buff, "SHT30 status %X\n", sht.readStatusRegister());
    //PrintLineRS(&pS, buff);
    sprintf(buff, "T.Amp %3.1f\260 - Hum %3.1f%%\n", t, u);
  } else sprintf(buff, "STH30 Error read\n");
  PrintLineRS(&pS, buff);
  if (bm180.isConnected()) {
    float tp = bm180.readTemperature();
    float p = bm180.readPressure();
    p = bm180.seaLevelPressure(p, 160, t);
    sprintf(buff, "Temp BMP180 %3.1f\260 Press Atm %4.0f mbar\n", tp, p);
  } else sprintf(buff, "BMP180 Error read\n");
  PrintLineRS(&pS, buff);
  time_t tm = readDateTimeDs3231();
  if (tm > TIME_2010_01_01) {
    sprintf(buff, "%s\n", strTimeAll(tm));
    PrintLineRS(&pS, buff);
  } else {
    sprintf(buff, "No set Data in DS3231\n");
    PrintLineRS(&pS, buff);
    tm = TIME_2010_01_01;
    setDataSystem(tm);
    if (!DataPrg.OnWifi) {
      waitTimer tmWait(10000);
      tmWait.start();
      PrintLineRS(&pS, "You need to set the clock...");
      while (tmWait.run()) delay(100);
      dataInitialize();
    }
  }
  ntp.begin(!DataPrg.OnWifi);
  if (DataPrg.OnWifi) {
    PrintLineRS(&pS, "Sincro NTP wait...\n");
    while (ntp.update() != NTP_OK) delay(10);
    PrintLineRS(&pS, "Sincro NTP done!\n");
    tm = nowTime();
    setDateTimeDs3231(tm);
    offWiFi();
  }
  setTimeClock();
  listFileMp3 = listFilesWithExtension("/", ".mp3", &countFileMp3);
  if (countFileMp3) {
    urn.setMax(countFileMp3 - 1);
    // for (int i = 0; i < countFileMp3; i++) {
    //   sprintf(buff, "%2.2d) %s\n", i + 1, listFileMp3[i]);
    //   PrintLineRS(&pS, buff);
    //   delay(10);
    // }
  }
  sprintf(buff, "Found %d file mp3\n", countFileMp3);
  PrintLineRS(&pS, buff);
  //delay(1500);

  obs = { DataPrg.lat, DataPrg.log, DataPrg.height, 24.0, 1024.25 };

  //obs = {-65.61, 12.16, 0, 24.0, 1024.25};
  //
  //obs.lat = 67;
  SetMp3Vol(DataPrg.Vol / 100.);
  sprintf(buff, "Set volume: %3.0f\n", DataPrg.Vol);
  PrintLineRS(&pS, buff);

  if (DataPrg.StartWork == 0 && tm > TIME_2010_01_01 + 86400) {
    DataPrg.StartWork = tm;
    SaveData();
  }

  PrintData();

  if (checkokSound(localTime(tm))) OpenMp3File(NAME_FILE_INIT_SOUND);
  waitTimer tmWait(15000);
  tmWait.start();
  uint32_t tButt;
  bool onB;
  while (!(onB = onButtonQueque(tButt)) && tmWait.run()) {
    RunMP3();
    delay(10);
  }
  if (onB) stopPlaySound();
  tmDisp.start();
  myDisp.clearScreen(backColor);
}

void dispTitle(char* s, time_t ltm) {
  char buff[80];
  snprintf(buff, 80, "%s - %s", s, strTimeAll(ltm));
  drawDisp pS(&myDisp, Font09, foreColor, backColor, strReduction);
  uint16_t lw, lh;
  pS.boxString(buff, lw, lh);
  //pS.drawString(buff);
  pS.drawPrint((MAX_ROW - lw) / 2, 0, buff);
}


void dispPosLocal(uint16_t x0, uint16_t y0) {
  uint16_t lw, lh;
  char buff[64];
  drawDisp pS(&myDisp, Font09, foreColor, backColor, strReduction);
  snprintf(buff, 64, "local: lat %4.2f\260  lon %4.2f\260  alt %4.0f m", DataPrg.lat, DataPrg.log, DataPrg.height);
  pS.boxString(buff, lw, lh);
  //pS.drawString(buff);
  pS.drawPrint((MAX_ROW - lw) / 2, y0, buff);
}

void dispPosObj(uint16_t x0, uint16_t y0, CelestialPosition& pos) {
  char buff[64], buff1[64];
  uint16_t lw, lh, lw1, lw2;
  drawDisp pS(&myDisp, Font09, foreColor, backColor, strReduction);
  y0 += PIX_NEWLINE;
  snprintf(buff, 64, "Azimut %4.2f\260", pos.az);
  snprintf(buff1, 64, "  Elev %4.2f\260", pos.el);
  pS.setColor(ColorLineAz, backColor);
  pS.boxString(buff, lw, lh);
  //strcat(buff, "  ");  // per pulire lo schermo
  //pS.drawString(buff);
  pS.drawPrint(x0, y0, buff);
  pS.setColor(ColorLineEl, backColor);
  pS.boxString(buff1, lw1, lh);
  //strcat(buff1, "  ");  // per pulire lo schermo
  //pS.drawString(buff1);
  pS.drawPrint(x0 + lw, y0, buff1);
  pS.setColor(foreColor, backColor);
  snprintf(buff, 64, "  Magnitude %4.2f", pos.magnitude);
  //pS.drawString(buff);
  pS.boxString(buff, lw2, lh);
  pS.drawPrint(x0 + lw + lw1, y0, buff);
  myDisp.fillScreen(x0 + lw + lw1 + lw2, y0, MAX_ROW, y0 + PIX_NEWLINE, backColor);

  y0 += PIX_NEWLINE;
  snprintf(buff, 64, "RA %s  Decl %s", StrhourToHMS(pos.RA), StrdegToDMS(pos.dec));
  //strcat(buff, "  ");  // per pulire lo schermo
  //pS.drawString(buff);
  pS.boxString(buff, lw, lh);
  pS.drawPrint(x0, y0, buff);
  snprintf(buff, 64, "  Diam %2.1f\250", pos.diam);
  pS.boxString(buff, lw1, lh);
  //pS.boxString(buff, lw, lh);
  //pS.drawString(buff);
  pS.drawPrint(x0 + lw, y0, buff);
  myDisp.fillScreen(x0 + lw + lw1, y0, MAX_ROW, y0 + PIX_NEWLINE, backColor);
  y0 += PIX_NEWLINE;
  if (pos.dist < 100) snprintf(buff, 64, "Dist %.2f AU", pos.dist);
  else snprintf(buff, 64, "Dist %.0f km", pos.dist);
  //pS.drawString(buff);
  pS.drawPrint(x0 + 275, y0, buff);
}



void foundRiseSet(time_t tm, CallbackRiseSet cbs, const double horizon, RiseSet& Twilight, RiseSet& Tw) {
  double jd = time2jd(tm);
  RiseSet Twn;
  cbs(jd, obs, horizon, Twilight);
  // Serial.printf(" rise %s\n",jd2str(localTimeJD(Twilight.rise)));
  // Serial.printf(" transit %s\n",jd2str(localTimeJD(Twilight.transit)));
  // Serial.printf(" set %s\n",jd2str(localTimeJD(Twilight.set)));
  // Serial.printf(" diff %f\n--------\n",jd - Twilight.transit);
  //if (Twilight.set < Twilight.rise || jd - Twilight.transit >= 0.5) {
  if (Twilight.set < Twilight.rise) {
    jd += 1;
    cbs(jd, obs, horizon, Twilight);
  }
  if (Twilight.polar == 0) {
    double sTm = jd - 0.5 - 1. / 24.;
    double eTm = jd + 0.5 + 1. / 24.;
    Twn = Tw = Twilight;
    //Serial.printf("%f %f %f %f\n", sTm, eTm, Twilight.rise, Twilight.set);
    if (sTm < Twilight.rise && Twilight.rise < eTm) Tw.rise = Twilight.rise;
    else {
      if (Twilight.rise < sTm) cbs(jd + 1, obs, horizon, Tw);
      else cbs(jd - 1, obs, horizon, Tw);
    }
    if (sTm < Twilight.set && Twilight.set < eTm) Tw.set = Twilight.set;
    else {
      if (Twilight.set < sTm) cbs(jd + 1, obs, horizon, Twn);
      else cbs(jd - 1, obs, horizon, Twn);
      Tw.set = Twn.set;
    }
  }
  Tw.polar = Twilight.polar;
}


void dispRSobj(int16_t x0, int16_t y0, RiseSet Twilight) {
  char buff[64];
  drawDisp pS(&myDisp, Font09, foreColor, backColor, strReduction);
  snprintf(buff, 64, "Transit %s", jd2str(localTimeJD(Twilight.transit)));
  //pS.drawString(buff);
  pS.drawPrint(x0, y0 - PIX_NEWLINE, buff);
  myDisp.fillScreen(0, y0, MAX_ROW, y0 + PIX_NEWLINE, backColor);
  if (Twilight.polar == 0) {
    pS.setColor(colorStrRise, backColor);
    snprintf(buff, 64, "Rise %s", jd2str(localTimeJD(Twilight.rise)));
    //pS.drawString(buff);
    pS.drawPrint(x0, y0, buff);
    snprintf(buff, 64, "Set %s", jd2str(localTimeJD(Twilight.set)));
    pS.setColor(colorStrSet, backColor);
    //pS.drawString(buff);
    pS.drawPrint(x0 + 245, y0, buff);
  } else {
    if (Twilight.polar == 1) sprintf(buff, "circumpolar, above the horizon");
    else sprintf(buff, "circumpolar, bellow the horizon");
    //pS.drawString(buff);
    pS.drawPrint(x0, y0, buff);
  }
}

#define MENU_CLOCK 0
#define MENU_RS_SUN 1
#define MENU_SUN 2
#define MENU_SUN_P 3
#define MENU_MOON 4
#define MENU_MOON_P 5
#define MENU_MERCURY 6
#define MENU_MERCURY_P 7
#define MENU_VENUS 8
#define MENU_VENUS_P 9
#define MENU_MARS 10
#define MENU_MARS_P 11
#define MENU_JUPITER 12
#define MENU_JUPITER_P 13
#define MENU_SATURN 14
#define MENU_SATURN_P 15
#define MENU_TEMP 16
#define MENU_UMI 17
#define MENU_PRESS 18
#define MENU_DISP_SET 19
#define MENU_END 20


void calcRiseSetSun(time_t tm) {
  char buff[64];
  drawDisp pS(&myDisp, Font12, foreColor, backColor, strReduction);
  CelestialPosition pos;
  RiseSet Twilight;
  uint16_t lw, lh;
  double jd = time2jd(tm);
  SunPosition(jd, obs, pos);

  uint16_t y0 = 35, x0 = 5;
  y0 += PIX_NEWLINE01;
  strcpy(buff, "Standard");
  //pS.boxString(buff, lw, lh);
  //pS.drawString(buff);
  pS.drawPrint(x0, y0, buff);
  y0 += PIX_NEWLINE01;
  strcpy(buff, "Civil");
  //pS.drawString(buff);
  pS.drawPrint(x0, y0, buff);
  y0 += PIX_NEWLINE01;
  strcpy(buff, "Nautical");
  //pS.drawString(buff);
  pS.drawPrint(x0, y0, buff);
  y0 += PIX_NEWLINE01;
  strcpy(buff, "Astronomical");
  //pS.drawString(buff);
  pS.drawPrint(x0, y0, buff);
  y0 = 35;
  strcpy(buff, "Sunrise");
  pS.boxString(buff, lw, lh);
  //pS.drawString(buff);
  pS.drawPrint((MAX_ROW / 3 - lw) / 2 + MAX_ROW / 3, y0, buff);
  strcpy(buff, "Sunset");
  pS.boxString(buff, lw, lh);
  //pS.drawString(buff);
  pS.drawPrint((MAX_ROW / 3 - lw) / 2 + 2 * MAX_ROW / 3, y0, buff);
  double lday = 0, ckd = 0, tr = 0;
  y0 += PIX_NEWLINE01;
  for (int i = 0; i < 4; i++) {
    SolarRiseSet(jd, obs, SUN_ANGLE[i], Twilight);
    if (i == 0) tr = Twilight.transit;
    myDisp.fillScreen(MAX_ROW / 3, y0 + PIX_NEWLINE01 * i, MAX_ROW, y0 + PIX_NEWLINE01 * (i + 1), backColor);
    if (Twilight.polar == 0) {
      if (i == 0) lday = Twilight.set - Twilight.rise;
      ckd = Twilight.transit - Twilight.rise;
      if (ckd > 1) snprintf(buff, 64, "(-%d) %s", (int)ckd, jd2str(localTimeJD(Twilight.rise), true));
      else snprintf(buff, 64, "%s", jd2str(localTimeJD(Twilight.rise), true));
      pS.boxString(buff, lw, lh);
      //pS.drawString(buff);
      pS.drawPrint((MAX_ROW / 3 - lw) / 2 + MAX_ROW / 3, y0 + PIX_NEWLINE01 * i, buff);

      ckd = Twilight.rise - Twilight.transit;
      if (ckd > 1) snprintf(buff, 64, "(+%d) %s", (int)ckd, jd2str(localTimeJD(Twilight.rise), true));
      else snprintf(buff, 64, "%s", jd2str(localTimeJD(Twilight.set), true));
      pS.boxString(buff, lw, lh);
      //pS.drawString(buff);
      pS.drawPrint((MAX_ROW / 3 - lw) / 2 + 2 * MAX_ROW / 3, y0 + PIX_NEWLINE01 * i, buff);
    } else {
      if (Twilight.polar == 1) {
        sprintf(buff, "above the horizon");
        if (i == 0) lday = 1;
      } else sprintf(buff, "bellow the horizon");
      pS.boxString(buff, lw, lh);
      //pS.drawString(buff);
      pS.drawPrint((2 * MAX_ROW / 3 - lw) / 2 + MAX_ROW / 3, y0 + PIX_NEWLINE01 * i, buff);
    }
  }
  y0 = 35 + PIX_NEWLINE01 * 5;
  pS.creatCanvas(0, 0, MAX_ROW, 1, RGB565_YELLOW);
  //pS.drawLine(0, 0, MAX_ROW, 0);
  pS.drawHorizontalLine(0, MAX_ROW, 0);
  pS.refresh(0, y0 + 1);
  pS.setColor(foreColor, backColor);
  y0 += 8;
  sprintf(buff, "Transit");
  //pS.drawString(buff);
  pS.drawPrint(x0, y0, buff);
  sprintf(buff, "%s", jd2str(localTimeJD(Twilight.transit), true));
  pS.boxString(buff, lw, lh);
  //pS.drawString(buff);
  myDisp.fillScreen(MAX_ROW / 3, y0, MAX_ROW, y0 + PIX_NEWLINE01, backColor);
  pS.drawPrint((2 * MAX_ROW / 3 - lw) / 2 + MAX_ROW / 3, y0, buff);

  y0 += PIX_NEWLINE01;
  sprintf(buff, "Day hours");
  //pS.drawString(buff);
  pS.drawPrint(x0, y0, buff);
  if (lday < 1) sprintf(buff, "%s", strHourMin((time_t)(lday * 86400.)));
  else sprintf(buff, "24:00");
  pS.boxString(buff, lw, lh);
  //pS.drawString(buff);
  myDisp.fillScreen(MAX_ROW / 3, y0, MAX_ROW, y0 + PIX_NEWLINE01, backColor);
  pS.drawPrint((2 * MAX_ROW / 3 - lw) / 2 + MAX_ROW / 3, y0, buff);

  y0 += PIX_NEWLINE01;
  sprintf(buff, "Julian Day");
  //pS.drawString(buff);
  pS.drawPrint(x0, y0, buff);
  sprintf(buff, "%d", julianDay(localTime(tm)));
  pS.boxString(buff, lw, lh);
  //pS.drawString(buff);
  myDisp.fillScreen(MAX_ROW / 3, y0, MAX_ROW, y0 + PIX_NEWLINE01, backColor);
  pS.drawPrint((2 * MAX_ROW / 3 - lw) / 2 + MAX_ROW / 3, y0, buff);

  y0 += PIX_NEWLINE01;
  //y0 = 35 + PIX_NEWLINE01 * 5;
  pS.creatCanvas(0, 0, MAX_ROW, 1, RGB565_YELLOW);
  //pS.drawLine(0, 0, MAX_ROW, 0);
  pS.drawHorizontalLine(0, MAX_ROW, 0);
  pS.refresh(0, y0 + 1);
  pS.setColor(foreColor, backColor);
  y0 += 8;
  sprintf(buff, "Easter on");
  //pS.drawString(buff);
  pS.drawPrint(x0, y0, buff);
  pS.setFont(Font09);
  time_t tEaster = CalcDayEaster(localTime(tm));
  long d = difftime(tEaster, localTime(tm));
  //Serial.printf("%d %f\n",d, difftime(localTime(tm), tEaster));
  if (d) sprintf(buff, "%s in %d days", strTimeAll(tEaster, true), d / 86400);
  else sprintf(buff, "%s - Today", strTimeAll(tEaster, true));
  pS.boxString(buff, lw, lh);
  //pS.drawString(buff);
  myDisp.fillScreen(MAX_ROW / 3, y0, MAX_ROW, y0 + PIX_NEWLINE01, backColor);
  pS.drawPrint((2 * MAX_ROW / 3 - lw) / 2 + MAX_ROW / 3, y0, buff);
}

void dispMoonPhase(uint16_t x0, uint16_t y0, CelestialPosition& pos) {
  char buff[64];
  snprintf(buff, 64, "Lunar Age %3.1f  Phase %s (%3.2f)", pos.age, strMoonPhase(pos.age), pos.phase);
  drawDisp pS(&myDisp, Font09, foreColor, backColor, strReduction);
  pS.drawPrint(x0, y0, buff);
  //Serial.printf("Moon limb %.2f\n", pos.moonlimb);
  //Serial.printf("Angolo di parallactic %.2f\n", pos.parallacticAngle);
}

// void dumpImgMoonLimb(uint16_t x0, uint16_t y0, double phase, double Limb, double Paral) {
//   uint16_t sImg = moonImage.xLen * moonImage.yLen;
//   uint16_t* mem = (uint16_t*)malloc(sizeof(uint16_t) * sImg);
//   Limb -= Paral;
//   //Limb -= 90;
//   //Limb=-(Limb-180);
//   Limb = 180 - Limb;
//   if (Limb > 360) Limb -= 360;
//   if (Limb < 0) Limb += 360;
//   if (mem) {
//     drawDisp pS(&myDisp, Font09, foreColor, backColor);
//     pS.creatCanvas(moonImage.xLen, moonImage.yLen);
//     memcpy((uint8_t*)mem, (uint8_t*)moonImage.image, sizeof(uint16_t) * sImg);
//     int16_t px = moonImage.xLen / 2, py = moonImage.yLen / 2 - 2, r = 72;  //r = 72;
//     //uint16_t r0 = (330 * r) / 90;     // 3.7 dal rapporto Rterra/Rluna frazione generatrice 333/90

//     double dx, dy, rd0;
//     rd0 = r + 300 * pow((1 - 2 * abs(phase - 0.5)), 2);
//     uint16_t r0 = (uint16_t)rd0;
//     if (phase <= 0.5) {
//       dx = (double)(r0 + r * (2 * phase - 1)) * sin(Limb * PI / 180);
//       dy = (double)(r0 + r * (2 * phase - 1)) * cos(Limb * PI / 180);
//       px += (int)dx;
//       py -= (int)dy;
//     } else {
//       dx = (double)(r0 - r * (2 * phase - 1)) * sin(Limb * PI / 180);
//       dy = (double)(r0 - r * (2 * phase - 1)) * cos(Limb * PI / 180);
//       px -= (int)dx;
//       py += (int)dy;
//     }
//     //Serial.printf("%d,%d,%f,%f\n",px,py,dx,dy);
//     pS.drawFilledCircle(px, py, r0);
//     for (int j = 0; j < moonImage.yLen; j++) {
//       for (int i = 0; i < moonImage.xLen; i++) {
//         uint16_t* p = (mem + i + j * moonImage.xLen);
//         if (phase <= 0.5) {
//           if (*p != backColor && pS.getPixel(i, j)) *p = decBrightness(*p, 0.5);
//         } else {
//           if (*p != backColor && !pS.getPixel(i, j)) *p = decBrightness(*p, 0.5);
//         }
//       }
//     }
//     gImage Image = { moonImage.xLen, moonImage.yLen, mem };
//     myDisp.dumpImage(x0, y0, &Image);
//     free(mem);
//   }
// }


void dumpImgMoonLimb(uint16_t x0, uint16_t y0, double phase, double Limb, double Paral) {
  uint16_t sImg = moonImage.xLen * moonImage.yLen;
  uint16_t* mem = (uint16_t*)malloc(sizeof(uint16_t) * sImg);

  static drawDisp pSmoon(&myDisp, Font09, foreColor, backColor, moonImage.xLen, moonImage.yLen);
  Limb -= Paral;
  //Limb -= 90;
  //Limb=-(Limb-180);
  Limb = 180 - Limb;
  if (Limb > 360) Limb -= 360;
  if (Limb < 0) Limb += 360;
  if (mem) {
    static uint16_t memR0 = 0;
    static int16_t memPx = 0, memPy = 0;
    // pS.creatCanvas(moonImage.xLen, moonImage.yLen);
    memcpy((uint8_t*)mem, (uint8_t*)moonImage.image, sizeof(uint16_t) * sImg);
    int16_t px = moonImage.xLen / 2, py = moonImage.yLen / 2 - 2, r = 72;  //r = 72;
    //uint16_t r0 = (330 * r) / 90;     // 3.7 dal rapporto Rterra/Rluna frazione generatrice 333/90

    double dx, dy, rd0;
    rd0 = r + 300 * pow((1 - 2 * abs(phase - 0.5)), 2);
    uint16_t r0 = (uint16_t)rd0;
    if (phase <= 0.5) {
      dx = (double)(r0 + r * (2 * phase - 1)) * sin(Limb * PI / 180);
      dy = (double)(r0 + r * (2 * phase - 1)) * cos(Limb * PI / 180);
      px += (int)dx;
      py -= (int)dy;
    } else {
      dx = (double)(r0 - r * (2 * phase - 1)) * sin(Limb * PI / 180);
      dy = (double)(r0 - r * (2 * phase - 1)) * cos(Limb * PI / 180);
      px -= (int)dx;
      py += (int)dy;
    }
    //Serial.printf("%d,%d,%f,%f\n",px,py,dx,dy);
    if (memR0 != r0 || memPx != px || memPy != py) {
      pSmoon.clearCanvas();
      pSmoon.drawFilledCircle(px, py, r0);
      memR0 = r0;
      memPx = px;
      memPy = py;
    }
    for (int j = 0; j < moonImage.yLen; j++) {
      for (int i = 0; i < moonImage.xLen; i++) {
        uint16_t* p = (mem + i + j * moonImage.xLen);
        if (phase <= 0.5) {
          if (*p != backColor && pSmoon.getPixel(i, j)) *p = decBrightness(*p, 0.5);
        } else {
          if (*p != backColor && !pSmoon.getPixel(i, j)) *p = decBrightness(*p, 0.5);
        }
      }
    }
    gImage Image = { moonImage.xLen, moonImage.yLen, mem };
    myDisp.dumpImage(x0, y0, &Image);
    free(mem);
  }
}

#define LEN_GRAPH_X 410
#define LEN_GRAPH_Y 254
#define POS_GRAPH_Y 28

void dispGraphTime(uint16_t x0, uint16_t y0, time_t tm, bool bEnd = false) {
  char buff[24];
  uint16_t lw, lh, lh1;
  drawDisp pS(&myDisp, Font09, foreColor, backColor, strReduction);
  strcpy(buff, strHourMin(localTime(tm), true));
  //pS.drawString(buff);
  pS.boxString(buff, lw, lh);
  if (bEnd) pS.drawPrint(MAX_ROW - lw - 1, y0, buff);
  else pS.drawPrint(x0 - lw / 2, y0, buff);
  strcpy(buff, strDay(localTime(tm)));
  //pS.drawString(buff);
  pS.boxString(buff, lw, lh1);
  if (bEnd) pS.drawPrint(MAX_ROW - lw - 1, y0 + lh, buff);
  else pS.drawPrint(x0 - lw / 2, y0 + lh, buff);
}

// void plotAcqData(char* tit, MemoAcqData& mq, char* nScale, uint16_t graphColor, float vScale) {
//   char buff[64];
//   uint16_t lw, lh;
//   static bool clsc = false;
//   drawDisp pS(&myDisp, Font12, graphColor, backColor, strReduction);
//   snprintf(buff, 64, tit);
//   //pS.drawString(buff);
//   pS.boxString(buff, lw, lh);
//   pS.drawPrint((MAX_ROW - lw) / 2, 0, buff);
//   uint16_t np = mq.numPoint();
//   if (np > 1) {
//     uint16_t pMin = 0, pMax = 0;
//     int16_t xp, yp;
//     float aMin, aMax, vMin, vMax;
//     float v[NUM_POINT_ACQ];
//     double t[NUM_POINT_ACQ];
//     double tl = mq.timeLastAcq() - TIME_POINT_ACQ * (np - 1);
//     pS.setFont(Font09);
//     pS.setColor(foreColor, backColor);
//     for (int i = 0; i < np; i++) t[i] = tl + TIME_POINT_ACQ * i;
//     mq.getArrayValue(v, aMin, pMin, aMax, pMax);
//     if (clsc) myDisp.clearScreen(backColor);
//     clsc = false;
//     uint16_t nxTick = (np < 48 ? np : 48);
//     uint16_t nyTick = 10;
//     uint16_t px = MAX_ROW - LEN_GRAPH_X;

//     vMin=floor(aMin-vScale);
//     vMax=ceil(aMax+vScale);

//     // vMin = aMin - abs(aMin * 0.05);
//     // vMax = aMax + abs(aMax * 0.05);
//     // if (vMin >= 0) vMin = floor(vMin);
//     // else vMin = ceil(vMin);
//     // vMax = ceil(vMax);
//     // if (vScale != NULL) {
//     //   vMin = (vScale[0] < aMin ? vScale[0] : vMin);
//     //   vMax = (vScale[1] > aMax ? vScale[1] : vMax);
//     // }
//     char fbuff[16];
//     pS.axis(LEN_GRAPH_X, LEN_GRAPH_Y, nxTick, nyTick, PIXEL_TICK);
//     sprintf(fbuff, "%%.%df", mq.nDecimal());
//     //Serial.printf("%f %f %f %f\n", t[0], t[np - 1], vMin, vMax);
//     sprintf(buff, fbuff, aMin);
//     pS.boxString(buff, lw, lh);
//     //xp = (LEN_GRAPH_X * pMin) / nxTick + 2;
//     xp = pS.dScale(t[pMin], 0, LEN_GRAPH_X, t[0], t[np - 1]);
//     if (xp + lw > LEN_GRAPH_X) xp = LEN_GRAPH_X - lw - 1;
//     else xp -= lw / 2;
//     if (xp < 0) xp = 2;
//     yp = LEN_GRAPH_Y - pS.dScale(aMin, 0, LEN_GRAPH_Y, vMin, vMax);
//     if (yp > (LEN_GRAPH_Y - lh)) yp = LEN_GRAPH_Y - lh - 1;
//     pS.drawString(xp, yp + 1, buff);
//     //pS.refresh(pw,LEN_GRAPH_Y yp+1);
//     sprintf(buff, fbuff, aMax);
//     pS.boxString(buff, lw, lh);
//     xp = pS.dScale(t[pMax], 0, LEN_GRAPH_X, t[0], t[np - 1]);
//     if (xp + lw > LEN_GRAPH_X) xp = LEN_GRAPH_X - lw - 1;
//     else xp -= lw / 2;
//     if (xp < 0) xp = 2;
//     yp = LEN_GRAPH_Y - pS.dScale(aMax, 0, LEN_GRAPH_Y, vMin, vMax);
//     if (yp < lh) yp = lh + 1;
//     pS.drawString(xp, yp - lh - 1, buff);
//     pS.plot(np, t, v, t[0], t[np - 1], vMin, vMax, graphColor, 4);
//     pS.refresh(px, POS_GRAPH_Y);

//     sprintf(buff, fbuff, vMin);
//     //pS.drawString(buff);
//     pS.boxString(buff, lw, lh);
//     pS.drawPrint(MAX_ROW - LEN_GRAPH_X - lw - 1, LEN_GRAPH_Y + POS_GRAPH_Y - lh, buff);
//     sprintf(buff, fbuff, vMax);
//     //pS.drawString(buff);
//     pS.boxString(buff, lw, lh);
//     pS.drawPrint(MAX_ROW - LEN_GRAPH_X - lw - 1, POS_GRAPH_Y + 1, buff);
//     sprintf(buff, nScale);
//     //pS.drawString(buff);
//     pS.boxString(buff, lw, lh);
//     pS.drawPrint(px - lw - 1, (LEN_GRAPH_Y + lh) / 2 + POS_GRAPH_Y, buff);
//     dispGraphTime(px, POS_GRAPH_Y + LEN_GRAPH_Y + 2, t[0]);
//     dispGraphTime(px, POS_GRAPH_Y + LEN_GRAPH_Y + 2, t[np - 1], true);
//     sprintf(buff, "Time");
//     //pS.drawString(buff);
//     pS.boxString(buff, lw, lh);
//     pS.drawPrint(px + (LEN_GRAPH_X - lw) / 2, POS_GRAPH_Y + LEN_GRAPH_Y + 2, buff);
//   } else {
//     strcpy(buff, "Acquiring data, please wait...");
//     pS.boxString(buff, lw, lh);
//     //pS.drawString(buff);
//     pS.drawPrint((MAX_ROW - lw) / 2, (MAX_COL - lh) / 2, buff);
//     clsc = true;
//   }
// }

void plotAcqData(char* tit, MemoAcqData& mq, char* nScale, uint16_t graphColor, float vScale) {
  char buff[64];
  uint16_t lw, lh;
  //static bool clsc = false;
  drawDisp pS(&myDisp, Font12, graphColor, backColor, strReduction);
  snprintf(buff, 64, tit);
  //pS.drawString(buff);
  pS.boxString(buff, lw, lh);
  pS.drawPrint((MAX_ROW - lw) / 2, 0, buff);
  uint16_t np = mq.numPoint();
  if (np > 0) {
    uint16_t pMin = 0, pMax = 0;
    int16_t xp, yp;
    float aMin, aMax, vMin, vMax;
    float v[NUM_POINT_ACQ];
    double t[NUM_POINT_ACQ];
    double tl = mq.timeLastAcq() - TIME_POINT_ACQ * (np - 1);
    pS.setFont(Font09);
    pS.setColor(foreColor, backColor);
    for (int i = 0; i < np; i++) t[i] = tl + TIME_POINT_ACQ * i;
    mq.getArrayValue(v, aMin, pMin, aMax, pMax);
    //if (clsc) myDisp.clearScreen(backColor);
    //clsc = false;
    uint16_t nxTick = (np < 48 ? np : 48);
    uint16_t nyTick = 10;
    uint16_t px = MAX_ROW - LEN_GRAPH_X;

    vMin = floor(aMin - vScale);
    vMax = ceil(aMax + vScale);

    // vMin = aMin - abs(aMin * 0.05);
    // vMax = aMax + abs(aMax * 0.05);
    // if (vMin >= 0) vMin = floor(vMin);
    // else vMin = ceil(vMin);
    // vMax = ceil(vMax);
    // if (vScale != NULL) {
    //   vMin = (vScale[0] < aMin ? vScale[0] : vMin);
    //   vMax = (vScale[1] > aMax ? vScale[1] : vMax);
    // }
    char fbuff[16];
    pS.axis(LEN_GRAPH_X, LEN_GRAPH_Y, nxTick, nyTick, PIXEL_TICK);
    sprintf(fbuff, "%%.%df", mq.nDecimal());
    //Serial.printf("%f %f %f %f\n", t[0], t[np - 1], vMin, vMax);
    sprintf(buff, fbuff, aMin);
    pS.boxString(buff, lw, lh);
    //xp = (LEN_GRAPH_X * pMin) / nxTick + 2;
    xp = pS.dScale(t[pMin], 0, LEN_GRAPH_X, t[0], t[np - 1]);
    if (xp + lw > LEN_GRAPH_X) xp = LEN_GRAPH_X - lw - 1;
    else xp -= lw / 2;
    if (xp < 0) xp = 2;
    yp = LEN_GRAPH_Y - pS.dScale(aMin, 0, LEN_GRAPH_Y, vMin, vMax);
    if (yp > (LEN_GRAPH_Y - lh)) yp = LEN_GRAPH_Y - lh - 1;
    pS.drawString(xp, yp + 1, buff);
    //pS.refresh(pw,LEN_GRAPH_Y yp+1);
    sprintf(buff, fbuff, aMax);
    pS.boxString(buff, lw, lh);
    xp = pS.dScale(t[pMax], 0, LEN_GRAPH_X, t[0], t[np - 1]);
    if (xp + lw > LEN_GRAPH_X) xp = LEN_GRAPH_X - lw - 1;
    else xp -= lw / 2;
    if (xp < 0) xp = 2;
    yp = LEN_GRAPH_Y - pS.dScale(aMax, 0, LEN_GRAPH_Y, vMin, vMax);
    if (yp < lh) yp = lh + 1;
    pS.drawString(xp, yp - lh - 1, buff);
    pS.plot(np, t, v, t[0], t[np - 1], vMin, vMax, graphColor, 3);
    pS.refresh(px, POS_GRAPH_Y);

    sprintf(buff, fbuff, vMin);
    //pS.drawString(buff);
    pS.boxString(buff, lw, lh);
    pS.drawPrint(MAX_ROW - LEN_GRAPH_X - lw - 1, LEN_GRAPH_Y + POS_GRAPH_Y - lh, buff);
    sprintf(buff, fbuff, vMax);
    //pS.drawString(buff);
    pS.boxString(buff, lw, lh);
    pS.drawPrint(MAX_ROW - LEN_GRAPH_X - lw - 1, POS_GRAPH_Y + 1, buff);
    sprintf(buff, nScale);
    //pS.drawString(buff);
    pS.boxString(buff, lw, lh);
    pS.drawPrint(px - lw - 1, (LEN_GRAPH_Y + lh) / 2 + POS_GRAPH_Y, buff);
    dispGraphTime(px, POS_GRAPH_Y + LEN_GRAPH_Y + 2, t[0]);
    dispGraphTime(px, POS_GRAPH_Y + LEN_GRAPH_Y + 2, t[np - 1], true);
    sprintf(buff, "Time");
    //pS.drawString(buff);
    pS.boxString(buff, lw, lh);
    pS.drawPrint(px + (LEN_GRAPH_X - lw) / 2, POS_GRAPH_Y + LEN_GRAPH_Y + 2, buff);
  } else {
    strcpy(buff, "Acquiring data");
    pS.boxString(buff, lw, lh);
    //pS.drawString(buff);
    pS.drawPrint((MAX_ROW - lw) / 2, (MAX_COL - lh) / 2 - lh / 2, buff);
    sprintf(buff, "Please wait... (%d sec)", mq.secNextAcq());
    pS.boxString(buff, lw, lh);
    //pS.drawString(buff);
    pS.drawPrint((MAX_ROW - lw) / 2, (MAX_COL - lh) / 2 + lh / 2, buff);
    myDisp.fillScreen((MAX_ROW - lw) / 2 + lw, (MAX_COL - lh) / 2 + lh / 2, MAX_ROW, (MAX_COL - lh) / 2 + lh / 2 + lh, backColor);
  }
}

char* lbCompass[] = { "N", "E", "S", "O" };
char* lbAng[] = { "+90\260", "", "-90\260", "0\260" };
char* lbHour[] = { "24", "6", "12", "18" };

#define D_RADIUS 5

double grHourDay(time_t ltm) {
  double g = ltm % 86400;
  return g * 360.0 / 86400;
}

#define ARC_GRADI 30


void GraphObjAzEl(uint16_t y0, time_t ltm, CelestialPosition& pos, RiseSet Twilight) {
  char buff[64];
  uint16_t lw, lh;
  uint16_t y = (MAX_COL - y0);
  uint16_t r = (y - D_RADIUS) / 2;
  uint16_t r1 = r - 4;
  uint16_t x = MAX_ROW / 3;

  drawDisp pS(&myDisp, Font09, foreColor, backColor, strReduction);
  pS.creatCanvas(y, y);
  pS.drawCircle(y / 2, y / 2, r);
  pS.drawCircleTick(y / 2, y / 2, r, 6, 3);
  pS.drawCircleLabel(y / 2, y / 2, r, 1, lbCompass, 4);
  //pS.drawTriangleHand(y/2, y/2, pos.az*PI/180., r-4, 5, 0 , RGB565_RED);
  pS.drawTriangleHand(y / 2, y / 2, (pos.az - 90) * PI / 180., r - 4, 8, 2, ColorLineAz);
  pS.creatCanvas(0, 0, y, y, ColorLineAz);
  sprintf(buff, "%.2f\260", pos.az);
  pS.boxString(buff, lw, lh);
  pS.drawString((y - lw) / 2, (pos.az > 90 && pos.az < 270 ? y / 2 - lh - 8 : y / 2 + 8), buff);
  //Serial.printf("%d %d %d %d %d %d %d %s %d\n", (y - lw) / 2, D_RADIUS+(r-lh)/2, D_RADIUS+(r-lh)/2)+r, lw, lh, r, y, buff, (pos.az > 90 && pos.az < 270 ? y / 4 - lh / 2 : y / 4 - lh / 2 + r));
  pS.refresh((x - y) / 2, y0);

  sprintf(buff, "Azimut");
  pS.boxString(buff, lw, lh, WRITE_VER);
  pS.setColor(ColorLineAz, backColor);
  pS.drawString(0, 0, buff, false, WRITE_VER);
  pS.refresh((x + y) / 2 + 1, y0 + (y - lh) / 2);

  uint16_t yf = (r * (cos(ARC_GRADI * PI / 180.) * 100)) / 100;
  uint16_t xf = (r * (sin(ARC_GRADI * PI / 180.) * 100)) / 100;
  uint16_t yEl = y / 2 + r - xf;
  pS.creatCanvas(yEl, y);
  pS.drawArcCircle(y / 2, y / 2, r, -210, ARC_GRADI);
  pS.drawCircleTick(y / 2, y / 2, r, 6, 3, ARC_GRADI);
  pS.drawCircleLabel(y / 2, y / 2, r, 1, lbAng, 4, 2);

  pS.drawVerticalLine(y / 2 + xf, r - yf, r + yf);
  pS.drawTriangleHand(y / 2, y / 2, (pos.el + 180.) * PI / 180., r - 4, 8, 2, ColorLineEl);

  pS.creatCanvas(0, 0, y, y, ColorLineEl);
  pS.drawTriangleHand(y / 2, y / 2, (Twilight.minEl + 180.) * M_PI / 180., r - 1, 8, r - 15);
  pS.drawTriangleHand(y / 2, y / 2, (Twilight.maxEl + 180.) * M_PI / 180., r - 1, 8, r - 15);


  sprintf(buff, "%.2f\260", pos.el);
  pS.boxString(buff, lw, lh);
  pS.drawString((y - lw) / 2 - 8, (pos.el < 0 ? y / 2 - lh - 8 : y / 2 + 8), buff);
  pS.refresh((x - y) / 2 + x + 18, y0, true);
  sprintf(buff, "Elev");
  pS.boxString(buff, lw, lh, WRITE_VER);
  pS.setColor(ColorLineEl, backColor);
  pS.drawString(0, 0, buff, false, WRITE_VER);
  pS.refresh((x - y) / 2 + x + yEl + 18 + 2, y0 + (y - lh) / 2);

  //Serial.printf("%f %f\n",Twilight.minEl,Twilight.maxEl );
  double aR, aS, aT;
  pS.creatCanvas(y, y);
  pS.setColor(foreColor, backColor);
  pS.drawCircle(y / 2, y / 2, r1);
  pS.drawCircleTick(y / 2, y / 2, r1, 6, 3);
  pS.drawCircleLabel(y / 2, y / 2, r1, 1, lbHour, 4);
  pS.drawTriangleHand(y / 2, y / 2, (grHourDay(ltm) - 90.0) * PI / 180., r1 - 4, 8, 2, RGB565_YELLOW);

  pS.creatCanvas(0, 0, y, y, RGB565_YELLOW);
  sprintf(buff, "Time");
  pS.boxString(buff, lw, lh);
  pS.drawString((y - lw) / 2, (grHourDay(ltm) > 90 && grHourDay(ltm) < 270 ? y / 2 - lh - 8 : y / 2 + 8), buff);

  if (Twilight.polar == 0) {
    aR = grHourDay(jd2time(localTimeJD(Twilight.rise)));
    aS = grHourDay(jd2time(localTimeJD(Twilight.set)));
  } else if (Twilight.polar == 1) {
    aR = 0;
    aS = 359;
  } else aR = aS = 0;

  pS.creatCanvas(y, y);
  pS.setColor(RGB565_LIGHT_MAGENTA, backColor);
  aT = grHourDay(jd2time(localTimeJD(Twilight.transit)));
  pS.drawTriangleHand(y / 2, y / 2, (aT - 90) * M_PI / 180., r1, 8, r1 - 15);
  pS.drawArcCircle(y / 2, y / 2, r1 + 1, aR, aS);
  pS.drawArcCircle(y / 2, y / 2, r1 + 2, aR, aS);
  pS.drawArcCircle(y / 2, y / 2, r1 + 3, aR, aS);
  pS.refresh((x - y) / 2 + 2 * x, y0, true);

  
}


uint16_t clearRow(uint16_t rLen, uint16_t oLen, uint16_t y0) {
  if (rLen < oLen) myDisp.fillScreen(0, y0, MAX_ROW, y0 + PIX_NEWLINE, backColor);
  return rLen;
}

void dispSetData(void) {
  char buff[64], buff1[32];
  uint16_t lw, lh;
  uint16_t y0 = 1;
  uint16_t x0 = 5;
  static uint16_t oLen[10];

  drawDisp pS(&myDisp, Font09, foreColor, backColor, strReduction);
  sprintf(buff, "System variables (ver %4.2f)", Version);
  pS.boxString(buff, lw, lh);
  // pS.drawString(buff);
  pS.drawPrint((MAX_ROW - lw) / 2, y0, buff);
  y0 += PIX_NEWLINE;

  if (!ErrTemp) sprintf(buff, "Temperature %4.1f\260 humidity %3.0f [%%]", tempExt, umiExt);
  else sprintf(buff, "temperature sensor error");
  pS.boxString(buff, lw, lh);
  //pS.drawString(buff);
  oLen[0] = clearRow(lw, oLen[0], y0);
  pS.drawPrint(x0, y0, buff);
  y0 += PIX_NEWLINE;

  if (!ErrPress) sprintf(buff, "Atm press %4.0f [mbar] Temp press sensor %4.1f\260", pressExt, tempInt);
  else sprintf(buff, "atmospheric sensor error");
  pS.boxString(buff, lw, lh);
  //pS.drawString(buff);
  oLen[1] = clearRow(lw, oLen[1], y0);
  pS.drawPrint(x0, y0, buff);
  y0 += PIX_NEWLINE;

  sprintf(buff, "brightness %5.0f [lux] Temp inter clock %4.1f\260", luxExt, tempClock);
  // pS.drawString(buff);
  pS.boxString(buff, lw, lh);
  oLen[2] = clearRow(lw, oLen[2], y0);
  pS.drawPrint(x0, y0, buff);
  y0 += PIX_NEWLINE;

  if (DataPrg.OnWifi) {
    // if (WiFi.status() == WL_CONNECTED) {
    //   sprintf(buff, "WiFi IP: %s  RSSI %d dBm", WiFi.localIP().toString().c_str(), WiFi.RSSI());
    // } else strcpy(buff, "WiFi: connection lost");
    sprintf(buff, "%s", ReadIp());
  } else strcpy(buff, "WiFi: off");
  pS.boxString(buff, lw, lh);
  oLen[3] = clearRow(lw, oLen[3], y0);
  //pS.drawString(buff);
  pS.drawPrint(x0, y0, buff);
  y0 += PIX_NEWLINE;

  sprintf(buff1, "to %s", strHourMin(DataPrg.hourEndSound));
  sprintf(buff, "Speaker on from %s %s (Vol %3.0f)", strHourMin(DataPrg.hourStartSound), buff1, DataPrg.Vol);
  //pS.drawString(buff);
  pS.drawPrint(x0, y0, buff);
  y0 += PIX_NEWLINE;

  if (DataPrg.AlarmOn) {
    sprintf(buff, "Alarm On at %s   Weekend alarm %s", strHourMin(DataPrg.hourAlarm), (DataPrg.OffAlarmWeek ? "On" : "Off"));
  } else strcpy(buff, "Alarm Off");
  pS.boxString(buff, lw, lh);
  oLen[4] = clearRow(lw, oLen[4], y0);
  //pS.drawString(buff);
  pS.drawPrint(x0, y0, buff);
  y0 += PIX_NEWLINE;

  sprintf(buff, "The bell sound: %s   The cuckoo sound: %s", (DataPrg.onSoundBell ? "On" : "Off"), (DataPrg.onSoundCucu ? "On" : "Off"));
  pS.boxString(buff, lw, lh);
  oLen[5] = clearRow(lw, oLen[5], y0);
  //pS.drawString(buff);
  pS.drawPrint(x0, y0, buff);
  y0 += PIX_NEWLINE;


  snprintf(buff, 64, "UTC offset %d (sec)", DataPrg.utcOffset);
  pS.boxString(buff, lw, lh);
  oLen[6] = clearRow(lw, oLen[6], y0);
  //pS.drawString(buff);
  pS.drawPrint(x0, y0, buff);
  y0 += PIX_NEWLINE;

  //char* strDayLigh[] = { "Solar", "Legal", "Auto (European)" };
  snprintf(buff, 64, "Daylight %s", strDayLigh[DataPrg.TyDaylight]);
  pS.boxString(buff, lw, lh);
  oLen[7] = clearRow(lw, oLen[7], y0);
  //pS.drawString(buff);
  pS.drawPrint(x0, y0, buff);
  y0 += PIX_NEWLINE;

  time_t lsync = localTime(ntp.lastSync());
  if (DataPrg.OnWifi) snprintf(buff, 64, "Last sync with NTP at %s - %s", strDay(lsync), strHourMin(lsync, true));
  else snprintf(buff, 64, "No sync with NTP, wifi off");
  pS.boxString(buff, lw, lh);
  oLen[8] = clearRow(lw, oLen[8], y0);
  //pS.drawString(buff);
  pS.drawPrint(x0, y0, buff);
  y0 += PIX_NEWLINE;

  uint32_t t = nowTime() - DataPrg.StartWork;
  snprintf(buff, 64, "it has been running for %d days %s", t / 86400, strHourMin(t, true));
  pS.boxString(buff, lw, lh);
  oLen[9] = clearRow(lw, oLen[9], y0);
  //pS.drawString(buff);
  pS.drawPrint(x0, y0, buff);
  y0 += PIX_NEWLINE;
}

void DispMenu(time_t tm, int aMenu, bool onSec, bool onMin, bool onHour) {
  time_t ltm = localTime(tm);
  static int oldMenu = MENU_CLOCK;
  CelestialPosition pos;
  static RiseSet TwGraph, Twilight;


  double jd = time2jd(tm);

  if (oldMenu != aMenu) {
    oldMenu = aMenu;
    myDisp.clearScreen(backColor);
  }
  switch (aMenu) {
    case MENU_CLOCK:
      GraphClock(ltm, millis());
      if (onSec) gPrintClock(ltm);
      if (onMin) {
        foundRiseSet(tm, SolarRiseSet, SUN_ANGLE[0], Twilight, TwGraph);
        gPrintRS(Twilight);
      }
      if (onHour) gPrintData();
      break;
    case MENU_RS_SUN:
      if (onSec) dpDigTime(ltm);
      if (onMin) calcRiseSetSun(tm);
      break;
    case MENU_SUN:
    case MENU_SUN_P:
      if (onMin) {
        foundRiseSet(tm, SolarRiseSet, SUN_ANGLE[0], Twilight, TwGraph);
        if (aMenu == MENU_SUN_P) dispRSobj(5, 185 + PIX_NEWLINE * 4, Twilight);
        PlotCelestialPosition(216, 18, 264, 150, tm, SunPosition, TwGraph, plotCelPos[0]);
      }
      if (onSec) {
        dispTitle("Sun", ltm);
        SunPosition(jd, obs, pos);
        if (aMenu == MENU_SUN_P) dispPosObj(5, 185, pos);
        else GraphObjAzEl(185, ltm, pos, Twilight);
      }
      if (onHour) {
        myDisp.dumpImage(20, 20, &sunImage);
        if (aMenu == MENU_SUN_P) dispPosLocal(5, 185);
      }
      break;
    case MENU_MOON:
    case MENU_MOON_P:
      if (onMin) {
        //dispRSobj(5, 185 + PIX_NEWLINE * 4, tm, MoonRiseSet, STAR_HORIZON, TwGraph);
        foundRiseSet(tm, MoonRiseSet, 0, Twilight, TwGraph);
        if (aMenu == MENU_MOON_P) dispRSobj(5, 185 + PIX_NEWLINE * 4, Twilight);
        PlotCelestialPosition(216, 18, 264, 150, tm, MoonPosition, TwGraph, plotCelPos[1]);
        MoonPosition(jd, obs, pos);
        dumpImgMoonLimb(20, 20, pos.phase, pos.moonlimb, pos.parallacticAngle);
        if (aMenu == MENU_MOON_P) dispMoonPhase(5, 185 + PIX_NEWLINE * 5, pos);
      }
      if (onSec) {
        dispTitle("Moon", ltm);
        MoonPosition(jd, obs, pos);
        if (aMenu == MENU_MOON_P) dispPosObj(5, 185, pos);
        else GraphObjAzEl(185, ltm, pos, Twilight);
      }
      if (onHour)
        if (aMenu == MENU_MOON_P) dispPosLocal(5, 185);
      break;
    case MENU_MERCURY:
    case MENU_MERCURY_P:
      if (onMin) {
        foundRiseSet(tm, MercuryRiseSet, STAR_HORIZON, Twilight, TwGraph);
        if (aMenu == MENU_MERCURY_P) dispRSobj(5, 185 + PIX_NEWLINE * 4, Twilight);
        PlotCelestialPosition(216, 18, 264, 150, tm, MercuryPosition, TwGraph, plotCelPos[2]);
      }
      if (onSec) {
        dispTitle("Mercury", ltm);
        MercuryPosition(jd, obs, pos);
        if (aMenu == MENU_MERCURY_P) dispPosObj(5, 185, pos);
        else GraphObjAzEl(185, ltm, pos, Twilight);
      }
      if (onHour) {
        if (aMenu == MENU_MERCURY_P) dispPosLocal(5, 185);
        myDisp.dumpImage(20, 20, &mercuryImage);
      }
      break;
    case MENU_VENUS:
    case MENU_VENUS_P:
      if (onMin) {
        foundRiseSet(tm, VenusRiseSet, STAR_HORIZON, Twilight, TwGraph);
        if (aMenu == MENU_VENUS_P) dispRSobj(5, 185 + PIX_NEWLINE * 4, Twilight);
        PlotCelestialPosition(216, 18, 264, 150, tm, VenusPosition, TwGraph, plotCelPos[3]);
      }
      if (onSec) {
        dispTitle("Venus", ltm);
        VenusPosition(jd, obs, pos);
        if (aMenu == MENU_VENUS_P) dispPosObj(5, 185, pos);
        else GraphObjAzEl(185, ltm, pos, Twilight);
      }
      if (onHour) {
        if (aMenu == MENU_VENUS_P) dispPosLocal(5, 185);
        myDisp.dumpImage(20, 20, &venusImage);
      }
      break;
    case MENU_MARS:
    case MENU_MARS_P:
      if (onMin) {
        foundRiseSet(tm, MarsRiseSet, STAR_HORIZON, Twilight, TwGraph);
        if (aMenu == MENU_MARS_P) dispRSobj(5, 185 + PIX_NEWLINE * 4, Twilight);
        PlotCelestialPosition(216, 18, 264, 150, tm, MarsPosition, TwGraph, plotCelPos[4]);
      }
      if (onSec) {
        dispTitle("Mars", ltm);
        MarsPosition(jd, obs, pos);
        if (aMenu == MENU_MARS_P) dispPosObj(5, 185, pos);
        else GraphObjAzEl(185, ltm, pos, Twilight);
      }
      if (onHour) {
        if (aMenu == MENU_MARS_P) dispPosLocal(5, 185);
        myDisp.dumpImage(20, 20, &marsImage);
      }
      break;
    case MENU_JUPITER:
    case MENU_JUPITER_P:
      if (onMin) {
        foundRiseSet(tm, JupiterRiseSet, STAR_HORIZON, Twilight, TwGraph);
        if (aMenu == MENU_JUPITER_P) dispRSobj(5, 185 + PIX_NEWLINE * 4, Twilight);
        PlotCelestialPosition(216, 18, 264, 150, tm, JupiterPosition, TwGraph, plotCelPos[5]);
      }
      if (onSec) {
        dispTitle("Jupiter", ltm);
        JupiterPosition(jd, obs, pos);
        if (aMenu == MENU_JUPITER_P) dispPosObj(5, 185, pos);
        else GraphObjAzEl(185, ltm, pos, Twilight);
      }
      if (onHour) {
        if (aMenu == MENU_JUPITER_P) dispPosLocal(5, 185);
        myDisp.dumpImage(20, 20, &jupiterImage);
      }
      break;
    case MENU_SATURN:
    case MENU_SATURN_P:
      if (onMin) {
        foundRiseSet(tm, SaturnRiseSet, STAR_HORIZON, Twilight, TwGraph);
        if (aMenu == MENU_SATURN_P) dispRSobj(5, 185 + PIX_NEWLINE * 4, Twilight);
        PlotCelestialPosition(216, 18, 264, 150, tm, SaturnPosition, TwGraph, plotCelPos[6]);
      }
      if (onSec) {
        dispTitle("Saturn", ltm);
        SaturnPosition(jd, obs, pos);
        if (aMenu == MENU_SATURN_P) dispPosObj(5, 185, pos);
        else GraphObjAzEl(185, ltm, pos, Twilight);
      }
      if (onHour) {
        if (aMenu == MENU_SATURN_P) dispPosLocal(5, 185);
        myDisp.dumpImage(20, 20, &saturnImage);
      }
      break;
    case MENU_TEMP:
      if (onMin || (onSec && !tempAmb.numPoint())) plotAcqData("Room temperature", tempAmb, "[C\260]", RGB565_LIGHT_RED, 2);
      break;
    case MENU_UMI:
      if (onMin || (onSec && !umiAmb.numPoint())) plotAcqData("Room humidity", umiAmb, "[%%]", RGB565_LIGHT_GREEN, 2.5);
      break;
    case MENU_PRESS:
      if (onMin || (onSec && !pressAmb.numPoint())) plotAcqData("Atmospheric pressure", pressAmb, "[mbar]", RGB565_LIGHT_CYAN, 5);
      break;
    case MENU_DISP_SET:
      if (onSec) dispSetData();
      break;
  }
}

#define TIME_BACK_MENU 1200
#define TIME_RETURN_MENU 3500
#define TIME_SETVAR_MENU 15000
#define TIME_SETVAR_WEB 30000


int SelectTmButton(unsigned long t, int& act) {
  static int oldAct = 0;
  if (t == 0) oldAct = act = 0;
  if (t >= TIME_BACK_MENU && act == 0) act++;
  if (t >= TIME_RETURN_MENU && act == 1) act++;
  if (t >= TIME_SETVAR_MENU && act == 2) act++;
  if (t >= TIME_SETVAR_WEB && act == 3) act++;
  if (oldAct != act) {
    oldAct = act;
    return act;
  }
  return 0;
}

bool isAlarmClock(time_t ltm) {
  struct tm* t;
  t = gmtime(&ltm);
  ltm %= 86400;
  if (DataPrg.hourAlarm % 86400 == ltm && DataPrg.AlarmOn) {
    if (!DataPrg.OffAlarmWeek && (t->tm_wday == 0 || t->tm_wday == 6)) return false;
    else return true;
  }
  return false;
}


bool checkokSound(time_t ltm) {
  ltm %= 86400;
  if (DataPrg.hourEndSound >= DataPrg.hourStartSound) {
    if (ltm >= DataPrg.hourStartSound && ltm <= DataPrg.hourEndSound) return true;
  } else {
    if (DataPrg.hourStartSound <= ltm || ltm <= DataPrg.hourEndSound) return true;
  }

  return false;
}


bool attemptReconnection() {
  static waitTimer tmConnWifi(10000);
  bool ret = false;
  if (!tmConnWifi.run()) {
    WiFi.mode(WIFI_STA);
    tmConnWifi.start();
    Serial.println("Tentativo di riconnessione...");
    WiFi.disconnect();
    delay(100);
    WiFi.begin(DataPrg.ssid, DataPrg.password);
    ret = true;
  }
  return ret;
}


bool checkWiFiStatus(void) {
  static waitTimer tmCheckWifi(60000);
  static bool wifiConnected = false;
  static uint16_t nAttempt = 0;
  bool ret = false;
  wl_status_t status = WiFi.status();
  if (nAttempt > 20) {
    tmCheckWifi.start();
    nAttempt = 0;
  }
  if (!tmCheckWifi.run()) {
    if (status == WL_CONNECTED) {
      ret = true;
      if (!wifiConnected) {
        wifiConnected = true;
        Serial.println("Wi-Fi riconnesso!");
        Serial.printf("%s\n", ReadIp());
        nAttempt = 0;
      }
    } else {
      wifiConnected = false;
      if (attemptReconnection()) nAttempt++;
    }
  }
  return ret;
}

#define NUM_PRESS_BUTT_ON_BELL 24

void loop() {
  //static char buff[48];
  static waitTimer tmPlayBell(800);
  static waitTimer tmMinPlay(2000);
  static waitTimer tmRunAllarm(DataPrg.longRingSec * 1000);      // durata della sveglia
  static waitTimer tmRipAllarm(DataPrg.longRipAlarmSec * 1000);  // l'allarma è ripetuto dopo longRipAlarmSec se non stop
  static waitTimer tmRetFrMenu(600 * 1000);                      // ritorna al menu principale
  static cAverage mdLux(10);
  static time_t tEverySec = 0;
  static int tEveryHour = -1;
  static int tEveryMin = -1;
  static int attMenu = MENU_CLOCK;
  static int memAttMenu = MENU_CLOCK;
  static bool clearButtOnDisp = false;
  static int act = 0;
  static int cNButt = 0;     // conta il numero di volte che butt e' pressed, si azzera con tmPlayBell
  static int bRunAlarm = 0;  // se true allarme in run
  static int nCyAlarm = 0;   // conta il numero di ripetizione delle allarme
  bool onSec, onMin, onHour;
  time_t tm, ltm;
  static int setEditV = 0;
  static int OffMenu = 0;
  static int setEditWeb = 0;


  tm = nowTime();
  ltm = localTime(tm);

  RunMP3();

  mdLux.pushValue(dig2Volt(analogRead(PIN_PHOTO_R)));

  uint32_t tButt;
  bool onButton = onButtonQueque(tButt);

  // static int stato = 0;
  // if (stato) {
  //   stopPlaySound();
  //   OffMenu = SetEditVarWeb(stato, onButton, tButt, ltm);
  //   //OffMenu = SetEditVar(stato, onButton, tButt);
  //   if (!OffMenu) {
  //     stato = 0;
  //     //ForceCheckWiFi = true;
  //   }
  // }


  if (setEditV) {
    uint32_t otButt = tButt;
    if (InRunMp3()) stopPlaySound();
    if (tButt >= TIME_SETVAR_MENU) {
      onButton = false;
      otButt = 0;
    }
    if (setEditWeb) OffMenu = SetEditVarWeb(setEditV, onButton, otButt, ltm);
    else OffMenu = SetEditVar(setEditV, onButton, otButt);
    tmDisp.start();
    clearButtOnDisp = true;
    if (!OffMenu) {
      setEditWeb = setEditV = 0;
      tEverySec = 0;
      tEveryHour = tEveryMin = -1;
      SetMp3Vol(DataPrg.Vol / 100.);
      //myDisp.clearScreen(backColor);
      setTimeClock();
      //SaveData();
      //ForceCheckWiFi = true;
    }
  }

  if (onButton && setEditV == 0 && setEditWeb == 1) setEditWeb = 0;

  if (tButt > 0) {
    if (!tmDisp.run() && valueBrightness(mdLux.retMd()) < 50) clearButtOnDisp = true;
    tmDisp.start();
    DispBrightness(100);
    //if (onButton) Serial.printf("Press Button %ld\n", tButt);
  } else act = 0;

  if (tmMinPlay.run()) clearQuequeButton();

  if (onButton && (bRunAlarm || (!tmMinPlay.run() && InRunMp3()))) {
    bRunAlarm = 0;
    Serial.println("Press button: stop Alarm/play");
    if (InRunMp3()) {
      clearButtOnDisp = true;
      stopPlaySound();
    }
  }


  int a = SelectTmButton(tButt, act);

  if (a) {
    clearButtOnDisp = true;
    if (attMenu != MENU_CLOCK) {
      tEverySec = 0;
      tEveryHour = tEveryMin = -1;
      if (a == 1) {
        attMenu--;
        if (attMenu < 0) attMenu = MENU_CLOCK;
      }
      if (a == 2) attMenu = MENU_CLOCK;
    }
    if (a == 3) setEditV = 1;
    if (a == 4) setEditWeb = setEditV = 1;
  }

  if (clearButtOnDisp && onButton) {
    onButton = clearButtOnDisp = false;
    tButt = 0;
  }


  if (!tmRetFrMenu.run()) {
    if (attMenu != MENU_CLOCK) {
      memAttMenu = attMenu;
      attMenu = MENU_CLOCK;
      tEverySec = 0;
      tEveryHour = tEveryMin = -1;
    }
  }

  if (onButton && !OffMenu) {
    //setEditWeb = 0;
    if (tmPlayBell.run()) cNButt += numInQuequeButton() + 1;
    else cNButt = 0;
    tmPlayBell.start();
    tEverySec = 0;
    tEveryHour = tEveryMin = -1;

    attMenu++;
    if (attMenu >= MENU_END) attMenu = MENU_CLOCK;

    if (memAttMenu != MENU_CLOCK) {
      attMenu = memAttMenu;
      memAttMenu = MENU_CLOCK;
    }
  }


  if (onButton && tButt > 0) tmRetFrMenu.start();


  if (!OffMenu && DataPrg.OnWifi) {
    int st = ntp.update();
    static bool eSt = false;
    if (st == NTP_ERR_WIFI_ERR) eSt = true;
    if (eSt) checkWiFiStatus();
    if (st == NTP_OK) {
      Serial.println("Sincronizzazione NTP avvenuta e time interno aggiornato!");
      tm = nowTime();
      ltm = localTime(tm);
      setDateTimeDs3231(tm);
      setTimeClock();
      offWiFi();
      eSt = false;
    }
  }

  if (attHour(ltm) != tEveryHour) {
    tEveryHour = attHour(ltm);
    setTimeClock();
    onHour = true;
  } else onHour = false;

  if (attMin(ltm) != tEveryMin) {
    tEveryMin = attMin(ltm);
    onMin = true;
  } else onMin = false;

  double v;
  if (ltm - tEverySec >= 1) {
    tEverySec = ltm;
    v = mdLux.retMd();
    //Serial.printf("%f %f\n",valueBrightness(v),valueLux(v));
    if (tmDisp.run()) DispBrightness(100);
    else DispBrightness((int)valueBrightness(v));
    onSec = true;
  } else onSec = false;


  if (AcqData(v)) {
    if (!ErrTemp) {
      tempAmb.putAcqData(tempExt);
      umiAmb.putAcqData(umiExt);
    }
    if (!ErrPress) pressAmb.putAcqData(pressExt);
  }

  if (!OffMenu) DispMenu(tm, attMenu, onSec, onMin, onHour);


  if (isAlarmClock(ltm) && DataPrg.nCycleAlarm && bRunAlarm == 0 && !OffMenu) {
    startPlaySoundAlr();
    //tmMinPlay.start();
    tmRunAllarm.start(DataPrg.longRingSec * 1000);
    bRunAlarm = 1;
    nCyAlarm = 0;
    Serial.printf("Start alarm\n");
  }
  //if (bRunAlarm) Serial.printf("%d %d %d %d\n",tmRunAllarm.run(),tmRipAllarm.run(),bRunAlarm, nCyAlarm);

  if (bRunAlarm == 1 && !tmRunAllarm.run()) {
    stopPlaySound();
    tmRipAllarm.start(DataPrg.longRipAlarmSec * 1000);
    nCyAlarm++;
    if (nCyAlarm >= DataPrg.nCycleAlarm) bRunAlarm = 0;
    else bRunAlarm = 2;
    Serial.printf("Stop alarm\n");
  }

  if (bRunAlarm == 2 && !tmRipAllarm.run() && !tmRunAllarm.run()) {
    startPlaySoundAlr();
    //tmMinPlay.start();
    tmRunAllarm.start(DataPrg.longRingSec * 1000);
    bRunAlarm = 1;
    Serial.printf("Cycle alarm\n");
  }

  if ((isNoon(ltm) && checkokSound(ltm) && DataPrg.onSoundBell && !InRunMp3() && !OffMenu) || cNButt > NUM_PRESS_BUTT_ON_BELL) {
    playBell();
    if (cNButt) clearQuequeButton();
    cNButt = 0;
    tmMinPlay.start();
  }

  if (DataPrg.onSoundCucu && !isNoon(ltm) && checkokSound(ltm) && !InRunMp3() && ltm % 3600 == 0 && !OffMenu) {
    OpenMp3File(NAME_FILE_CUCU_PLAY);
    //tmMinPlay.start();
  }
}