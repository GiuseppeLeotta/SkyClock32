#include "PlayMp3.h"

#include <string.h>

#include <AudioFileSourceLittleFS.h>
//#include <AudioFileSourceSPIFFS.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputI2S.h>


AudioOutputI2S* out;
AudioFileSourceLittleFS* file[2];
//AudioFileSourceSPIFFS  * file;
AudioGeneratorMP3* mp3;

enum { ST_MP3_WAIT,
       ST_MP3_START,
       ST_MP3_RUN,
       ST_MP3_CLOSE } Mp3Stato = ST_MP3_WAIT;

float mp3Vol = 1;
int IsRepeatMP3 = 0;
char nameFileMp3[48];
int cPlayer = 0;


int SetMp3Vol(float v) {
  int ret = 0;
  if (v >= 0.0 && v <= 2.0) {
    ret = 1;
    mp3Vol = v;
    if (Mp3Stato == ST_MP3_RUN) out->SetGain(mp3Vol);
  }
  return ret;
}

int OpenMp3File(char* NameFile, int cyclePlay) {
  int ret = 0;
  if (Mp3Stato == ST_MP3_WAIT) {
    char buff[48];
    nameFileMp3[0] = 0;
    IsRepeatMP3 = 0;
    if (NameFile[0] != '/') {
      buff[0] = '/';
      buff[1] = 0;
      strcat(buff, NameFile);
    } else strcpy(buff, NameFile);
    file[0] = new AudioFileSourceLittleFS(buff);
    file[1] = NULL;
    //file = new AudioFileSourceSPIFFS(buff);
    if (!file[0]->isOpen()) {
      Serial.printf("File mp3 %s non aperto\n", buff);
    } else {
      IsRepeatMP3 = cyclePlay;
      if (IsRepeatMP3) {
        file[1] = new AudioFileSourceLittleFS(buff);
        strcpy(nameFileMp3, buff);
      }
      mp3 = new AudioGeneratorMP3();
      out = new AudioOutputI2S(0, 0, 32);
      out->SetGain(mp3Vol);
      ret = 1;
      Mp3Stato = ST_MP3_START;
      cPlayer = 0;
      //Serial.printf("File mp3 %s in run\n", buff);
    }
  }
  return ret;
}


int CloseMp3File(void) {
  int ret = 0;
  if (Mp3Stato == ST_MP3_RUN) {
    ret = 1;
    mp3->stop();
    Mp3Stato = ST_MP3_CLOSE;
  }
  IsRepeatMP3 = 0;
  return ret;
}

bool InRunMp3(void) {
  return (Mp3Stato != ST_MP3_WAIT);
}


int loopMp3(void) {
  int ret = 0;
  if (mp3->isRunning()) {
    if (!mp3->loop()) {
      mp3->stop();
    } else ret = 1;
  }
  return ret;
}



int DoMp3Start(void) {
  int ret = 1;
  int e = mp3->begin(file[cPlayer], out);
  if (e) {
    Mp3Stato = ST_MP3_RUN;
    for (int i = 1; i < 16; i++)
      if (!loopMp3()) break;
      else delay(1);
    if (IsRepeatMP3) {
      for (int i = 0; i < 2; i++) {
        if (!file[i]) {
          Serial.printf("load file %d aPlayer %d\n", i, cPlayer);
          file[i] = new AudioFileSourceLittleFS(nameFileMp3);
          //if (!mp3[i]) mp3[i]= new AudioGeneratorMP3();
        }
      }
    }
  } else {
    Mp3Stato = ST_MP3_CLOSE;
    IsRepeatMP3 = false;
    ret = 0;
    Serial.printf("Error play mp3. Mem %ld\n", ESP.getMaxAllocHeap());
  }
  return ret;
}

int DoMp3Close(void) {
  //if (file[cPlayer]->isOpen()) file->close();  // chiude lo stream SPIFFS
  delete file[cPlayer];
  file[cPlayer] = NULL;
  //delete mp3;
  cPlayer = (cPlayer + 1) % 2;
  if (IsRepeatMP3) {
    //Mp3Stato = ST_MP3_START;
    IsRepeatMP3--;
    DoMp3Start();
  } else {
    Mp3Stato = ST_MP3_WAIT;
    for (int i = 0; i < 2; i++) {
      if (file[i]) delete file[i];
    }
    delete out;
    delete mp3;
  }
  delay(1);
  return 0;
}

int DoMp3Run(void) {
  int ret;
  if ((ret = loopMp3()) == 0) {
    Mp3Stato = ST_MP3_CLOSE;
    if (IsRepeatMP3) DoMp3Close();
  }
  return ret;
}

int RunMP3(void) {
  int ret = 0;
  switch (Mp3Stato) {
    case ST_MP3_START:
      ret = DoMp3Start();
      break;
    case ST_MP3_RUN:
      ret = DoMp3Run();
      break;
    case ST_MP3_CLOSE:
      ret = DoMp3Close();
      break;
    default:
    case ST_MP3_WAIT:
      break;
  }
  return ret;
}
