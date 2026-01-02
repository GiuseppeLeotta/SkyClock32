#ifndef FILE_DATA_H
#define FILE_DATA_H

#include "Arduino.h"
//#include "FS.h"
//#include <SPIFFS.h>

bool InitFileData(bool IsFormat);
bool ReadData(const char * NameFile, uint8_t * data, int lenData);
bool WriteData(const char * NameFile, uint8_t * data, int lenData);
char** listFilesWithExtension(const char* path, const char* extension, int* outCount);

#endif
