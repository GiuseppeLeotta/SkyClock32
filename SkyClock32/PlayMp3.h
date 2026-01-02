#ifndef PLAY_MP3_H
#define PLAY_MP3_H
#include <Arduino.h>

int SetMp3Vol(float v);
int OpenMp3File(char * NameFile,int cyclePlay = 0);
int CloseMp3File(void);
bool InRunMp3(void);
int RunMP3(void);

#endif
