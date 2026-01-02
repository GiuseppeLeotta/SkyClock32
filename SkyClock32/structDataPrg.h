#ifndef _STRUCT_DATAPRG_H
#define _STRUCT_DATAPRG_H


#define MAX_CHR_SSID 64

typedef struct  {
  char ssid[MAX_CHR_SSID];
  char password[MAX_CHR_SSID];
  double lat = 37.61;
  double log = 15.16;
  double height = 161;  // in meters
  long utcOffset = 3600;
  time_t StartWork = 0;     
  long TyDaylight = 0;  // [0->off,1->on,2->auto]
  bool OnWifi = true;     // if false wifi off
  double Vol = 100;         // 0..200
  bool onSoundBell = true;
  bool onSoundCucu = true;
  time_t hourStartSound = 3600*8.5;
  time_t hourEndSound = 3600*21.0;
  time_t hourAlarm = 3600*9.5;
  bool AlarmOn = true;        // Sveglia attivata
  bool OffAlarmWeek = false;  // Se true sveglia off del weekend 
  long nCycleAlarm = 1;        // numero di ripetizione allarme; se è 0 alarm off
  long longRingSec = 10;      // durata della sveglia
  long longRipAlarmSec = 30;  // l'allarma è ripetuto dopo longRipAlarmSec se non stop
} tagDataPrg;


#endif 