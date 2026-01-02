
#include "FileData.h"


//#include <SPIFFS.h>
#include <LittleFS.h>

#define TYPEFAT LittleFS 

#define MAX_NAME_LEN 32  // lunghezza massima nome file in SPIFFS (incluso terminatore '\0')

/**
 * Ottiene la lista di file con estensione specifica in una directory SPIFFS.
 *
 * @param path       Percorso della directory (es. "/music")
 * @param extension  Estensione da filtrare (es. ".mp3")
 * @param outCount   Puntatore a intero in cui viene scritto il numero di file trovati
 * @return           Array di char* allocato in un unico blocco; nullptr se nessun file
 *                   Ogni fileArray[i] punta a un buffer di MAX_NAME_LEN byte.
 */
 
char** listFilesWithExtension(const char* path, const char* extension, int* outCount) {
    
    *outCount = 0;
    // Conta i file con estensione
    int count = 0;
    File root = TYPEFAT.open(path);
    if (!root.isDirectory()) return nullptr;
    File file = root.openNextFile();
    while (file) {
        if (String(file.name()).endsWith(extension)) count++;
        file = root.openNextFile();
    }
    *outCount = count;
    if (count == 0) return nullptr;

    // Calcola dimensioni: puntatori + buffer nomi
    size_t ptrsSize = count * sizeof(char*);
    size_t namesSize = count * MAX_NAME_LEN;
    void* block = malloc(ptrsSize + namesSize);
    if (!block) {
        Serial.println("Errore: malloc block");
        *outCount = 0;
        return nullptr;
    }
    root.close(); file.close(); 
    char** fileArray = (char**)block;
    char* namesBuf = (char*)block + ptrsSize;

    // Seconda passata: assegna puntatori e copia nomi
    root = TYPEFAT.open(path);
    file = root.openNextFile();
    int idx = 0;
    while (file) {
        String name = file.name();
        if (name.endsWith(extension) && idx < count) {
            fileArray[idx] = namesBuf + idx * MAX_NAME_LEN;
            name.toCharArray(fileArray[idx], MAX_NAME_LEN);
            idx++;
        }
        file = root.openNextFile();
    }
    root.close(); file.close(); 
    return fileArray;
}

bool InitFileData(bool IsFormat)
{
  bool ret = true;
  if (!TYPEFAT.begin(IsFormat)) {
    ret = false;
    Serial.println("\nUnable to begin(), formating...");
    // if (IsFormat) {
    //   if (!SPIFFS.format()) {
    //     Serial.println("Unable to format(), aborting...");
    //     ret = false;
    //   }
    // } else ret = false;
  }
  return ret;
}

bool ReadData(const char * NameFile, uint8_t * data, int lenData)
{
  bool ret = false;
  File f = TYPEFAT.open(NameFile, "r");
  if (f) {
    int r = f.read(data, lenData);
    delay(1);
    //Serial.printf("File len %d %d %d\n",f.size(),r,lenData);
    if (f.size() == lenData) ret = true;
    f.close();
  }
  return ret;
}

bool WriteData(const char * NameFile, uint8_t * data, int lenData)
{
  bool ret = false;
  File f = TYPEFAT.open(NameFile, "w");
  if (f) {
    int r = f.write(data, lenData);
    delay(1);
    f.close();
    if (r == lenData) ret = true;
  }
  return ret;
}
