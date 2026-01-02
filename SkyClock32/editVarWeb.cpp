#include <arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ArduinoJson.h>
#include <time.h>
#include "drawDisp.h"
#include "editVarWeb.h"
#include "utility.h"
#include "ds3231.h"

//#include "Fonts/FreeMonoBold12pt8b.h"

WebServer server(80);
DNSServer dnsServer;

const byte DNS_PORT = 53;
const char* hostname = "skyclock.local";


void handleRoot();
void handleUpdate();
void handleGetValues();
void handleReload();

time_t attTime = TIME_2010_01_01;
time_t nDtime = 0;
time_t nHtime = 0;

bool EndConfig = false;

long pMenuSSID=0; 

ConfigField configFields[] = {
  { ConfigField::DATE, "Set date (local time)", &nDtime, 0, 0, 0, NULL, 0 },
  { ConfigField::TIME, "Set hour (local time)", &nHtime, 0, 0, 0, NULL, 0 },
  { ConfigField::BOOL, "Enable Wi-Fi", &DataPrg.OnWifi, 0, 0, 0, NULL, 0 },
  { ConfigField::STRING, "SSID", &DataPrg.ssid, MAX_CHR_SSID, 0, 0, NULL, 0 },
  { ConfigField::STRING, "password", &DataPrg.password, MAX_CHR_SSID, 0, 0, NULL, 0 },
  { ConfigField::DOUBLE, "Latitude", &DataPrg.lat, 0, -90, 90, NULL, 0 },
  { ConfigField::DOUBLE, "Longitude", &DataPrg.log, 0, -180, 180, NULL, 0 },
  { ConfigField::DOUBLE, "height (m)", &DataPrg.height, 0, 0, 4000, NULL, 0 },
  { ConfigField::LONG, "OffSet UTC time (sec)", &DataPrg.utcOffset, 0, -43200, 43200, NULL, 0 },
  { ConfigField::MENU, "Daylight", &DataPrg.TyDaylight, 0, 0, 0, strDayLigh, 3 },
  { ConfigField::DOUBLE, "Set speaker volume", &DataPrg.Vol, 0, 0, 200, NULL, 0 },
  { ConfigField::BOOL, "Enables the bells to ring at noon", &DataPrg.onSoundBell, 0, 0, 0, NULL, 0 },
  { ConfigField::BOOL, "Enable the cuckoo sound every hour", &DataPrg.onSoundCucu, 0, 0, 0, NULL, 0 },
  { ConfigField::TIME, "Turn on the speaker at", &DataPrg.hourStartSound, 0, 0, 0, NULL, 0 },
  { ConfigField::TIME, "Turn off the speaker at", &DataPrg.hourEndSound, 0, 0, 0, NULL, 0 },
  { ConfigField::BOOL, "Enable alarm clock", &DataPrg.AlarmOn, 0, 0, 0, NULL, 0 },
  { ConfigField::BOOL, "The alarm is off for the weekend", &DataPrg.OffAlarmWeek, 0, 0, 0, NULL, 0 },
  { ConfigField::TIME, "set the alarm for", &DataPrg.hourAlarm, 0, 0, 0, NULL, 0 },
  { ConfigField::LONG, "The alarm rings for (sec)", &DataPrg.longRingSec, 0, 10, 150, NULL, 0 },
  { ConfigField::LONG, "Alarm snooze", &DataPrg.nCycleAlarm, 0, 1, 5, NULL, 0 },
  { ConfigField::LONG, "Time interval for the alarm to snooze (sec)", &DataPrg.longRipAlarmSec, 0, 60, 600, NULL, 0 }
};

const size_t numFields = sizeof(configFields) / sizeof(ConfigField);

// uint32_t bssidToHash(const String& bssid_str) {
//     uint32_t hash = 0;
//     int values[6];
    
//     // Parsing del MAC address
//     sscanf(bssid_str.c_str(), "%02X:%02X:%02X:%02X:%02X:%02X", 
//            &values[0], &values[1], &values[2], 
//            &values[3], &values[4], &values[5]);
    
//     // Creazione hash (FNV-1a semplificato)
//     for(int i = 0; i < 6; i++) {
//         hash = (hash * 16777619) ^ values[i];
//     }
    
//     return hash;
// }

char** scanNetworkNames(int& count) {
  // Inizializza il contatore
  count = 0;

  // Scansiona le reti
  int networkCount = WiFi.scanNetworks();

  if (networkCount <= 0) {
    WiFi.scanDelete();
    return NULL;
  }

  // Alloca l'array di puntatori a char
  char** networks = (char**)malloc(networkCount * sizeof(char*));
  if (networks == NULL) {
    WiFi.scanDelete();
    return NULL;
  }

  int validNetworks = 0;

  // Copia i nomi delle reti
  for (int i = 0; i < networkCount; i++) {
    String ssid = WiFi.SSID(i);

    // Salta SSID vuoti
    if (ssid.length() == 0) continue;
    
    // Canali 1-14: 2.4GHz
    // Canali 36-165: 5GHz
    int channel = WiFi.channel(i);
    if (channel >= 36 && channel <= 165) continue;
    
    bool ctloop=false;
    for (int j=0;j<validNetworks && !ctloop;j++) if (strcmp(networks[j],ssid.c_str())==0) ctloop=true;
    if (ctloop) continue;

    //Serial.printf("%d %d %s %s %d\n",j, i, networks[j],ssid.c_str(), strcmp(networks[j],ssid.c_str()));
    //Serial.printf("%d %s %s\n",channel,ssid.c_str(),WiFi.BSSIDstr(i).c_str());
    // Alloca memoria per il nome della rete
    networks[validNetworks] = (char*)malloc((ssid.length() + 1) * sizeof(char));
    if (networks[validNetworks] == NULL) {
      // In caso di errore di allocazione, libera ciò che è stato allocato finora
      for (int j = 0; j < validNetworks; j++) {
        free(networks[j]);
      }
      free(networks);
      WiFi.scanDelete();
      return NULL;
    }

    // Copia la stringa
    strcpy(networks[validNetworks], ssid.c_str());
    validNetworks++;
  }

  // Aggiorna il contatore con il numero effettivo di reti valide
  count = validNetworks;

  // Se non ci sono reti valide, libera la memoria e restituisce NULL
  if (validNetworks == 0) {
    free(networks);
    networks = NULL;
  }

  WiFi.scanDelete();
  return networks;
}

/**
 * Libera la memoria allocata da scanNetworkNames()
 * @param networks Array di stringhe da liberare
 * @param count Numero di elementi nell'array
 */
void freeNetworkNames(char** networks, int count) {
  if (networks != NULL) {
    for (int i = 0; i < count; i++) {
      if (networks[i] != NULL) {
        free(networks[i]);
      }
    }
    free(networks);
  }
}

String timeToString(time_t t) {
  struct tm* timeinfo;
  timeinfo = gmtime(&t);
  char buffer[6];
  strftime(buffer, sizeof(buffer), "%H:%M", timeinfo);
  return String(buffer);
}


// Funzione per convertire stringa HH:MM in time_t
time_t stringToTime(const String& timeStr) {
  int hours, minutes;
  time_t t;
  if (sscanf(timeStr.c_str(), "%d:%d", &hours, &minutes) == 2) {
    // Usiamo una data fissa (oggi) e impostiamo solo ore e minuti
    t = (long)hours * 3600 + minutes * 60;
    return t;
  }
  return 0;
}

// Funzione per convertire time_t in stringa YYYY-MM-DD
String dateToString(time_t t) {
  struct tm* timeinfo;
  timeinfo = gmtime(&t);
  char buffer[11];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d", timeinfo);
  return String(buffer);
}

// Funzione per convertire stringa YYYY-MM-DD in time_t
time_t stringToDate(const String& dateStr) {
  int year, month, day;
  if (sscanf(dateStr.c_str(), "%d-%d-%d", &year, &month, &day) == 3) {
    struct tm timeinfo = { 0 };
    timeinfo.tm_mday = day;
    timeinfo.tm_mon = month - 1;     // tm_mon va da 0 a 11
    timeinfo.tm_year = year - 1900;  // tm_year è anni dal 1900
    timeinfo.tm_hour = 0;
    timeinfo.tm_min = 0;
    timeinfo.tm_sec = 0;
    return mktime(&timeinfo);
  }
  return 0;
}

const String NamePageWeb = "Update data for SkyClock";

#define Y_POS 100

#define NAME_SSID "SkyClock"

uint16_t yPos = 0;

#define PIXEL_ROW 25


char ** NameFileNetWork;
int numNetWork;


#define POS_CONF_SSID 3

int FoundSSID(char * s, char **nNet, int numNet) {
   if (numNet==0) return -1;
   for (int i=0;i<numNet;i++) {
      if (strcmp(s,nNet[i])==0) return i;  
   }
  return -1;
}

int initSetEditVarWeb(void) {

  char buff[48];
  uint16_t lw, lh;
  drawDisp pS(&myDisp, Font12 , foreColor, backColor);
  myDisp.clearScreen(backColor);
  WiFi.disconnect(true);
  delay(200);
  WiFi.mode(WIFI_OFF);
  delay(200);
  WiFi.mode(WIFI_AP);
  WiFi.persistent(false);
  WiFi.setAutoReconnect(false);
  WiFi.softAP(NAME_SSID, NULL);
  

  sprintf(buff, "Network scanner...");
  pS.boxString(buff, lw, lh);
  //pS.drawString(buff);
  pS.drawPrint((MAX_ROW - lw) / 2, (MAX_COL - lh) / 2,buff);

  NameFileNetWork=scanNetworkNames(numNetWork);
  // if (numNetWork) {
  //   for (int i=0;i<numNetWork;i++) {
  //     Serial.printf("%2d) SSID: %s\n",i+1,NameFileNetWork[i]);
  //   }
  // }
  pMenuSSID=FoundSSID(DataPrg.ssid, NameFileNetWork, numNetWork);

  if (numNetWork) {
    if (pMenuSSID==-1) pMenuSSID=0;
    configFields[POS_CONF_SSID].type=ConfigField::MENU;
    configFields[POS_CONF_SSID].variable=&pMenuSSID;
    configFields[POS_CONF_SSID].options=NameFileNetWork;
    configFields[POS_CONF_SSID].numOptions=numNetWork;
  }

  // Route server web
  server.on("/", HTTP_GET, handleRoot);
  server.on("/update", HTTP_POST, handleUpdate);
  server.on("/values", HTTP_GET, handleGetValues);
  server.on("/reload", HTTP_GET, handleReload);

  server.begin();
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
  Serial.println("Server HTTP avviato");

  myDisp.clearScreen(backColor);
  Serial.print("AP IP address: ");
  String ip = WiFi.softAPIP().toString();
  Serial.println(ip);
  yPos = Y_POS;
  sprintf(buff, "Network connect: %s", NAME_SSID);
  pS.boxString(buff, lw, lh);
  //pS.drawString(buff);
  pS.drawPrint((MAX_ROW - lw) / 2, yPos,buff);
  yPos += PIXEL_ROW;
  sprintf(buff, "http://%s", hostname);
  pS.boxString(buff, lw, lh);
  //pS.drawString(buff);
  pS.drawPrint((MAX_ROW - lw) / 2, yPos,buff);
  yPos += PIXEL_ROW;
  sprintf(buff, "IP address: %s", ip.c_str());
  pS.boxString(buff, lw, lh);
  //pS.drawString(buff);
  pS.drawPrint((MAX_ROW - lw) / 2, yPos,buff);
  yPos += PIXEL_ROW;
  yPos += PIXEL_ROW;

  return 1;
}



void CloseSetEditVarWeb(void) {
  
  freeNetworkNames(NameFileNetWork, numNetWork);

  myDisp.clearScreen(backColor);
  server.stop();
  dnsServer.stop();
  server.close();
  WiFi.softAPdisconnect(true);
  delay(200);
  WiFi.disconnect(true);
  delay(200);
  WiFi.mode(WIFI_OFF);
  delay(200);
  WiFi.mode(WIFI_STA);
  delay(200);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
}

void SetDataVarWeb(void) {
  time_t t = nDtime + nHtime;
  t -= calcOffSetTime();
  setDateTimeDs3231(t);
  if (numNetWork) strcpy(DataPrg.ssid,NameFileNetWork[pMenuSSID]);    
  SaveData();
  CloseSetEditVarWeb();
  PrintData();
  EndConfig = false;
  ESP.restart();
}

int SetEditVarWeb(int& stato, bool onButton, uint32_t tButt, time_t ltm) {
  if (stato == 0) return 0;
  if (tButt > 400) onButton = false;

  if (stato == 1) {
    if (initSetEditVarWeb()) stato = 2;
    else return 0;
  }
  if (ltm != attTime && ltm) {
    attTime = ltm;
    drawDisp pS(&myDisp, Font12, foreColor, backColor);
    char buff[48];
    uint16_t lw, lh;
    sprintf(buff, "%s", strTimeAll(ltm));
    pS.boxString(buff, lw, lh);
    //pS.drawString(buff);
    pS.drawPrint((MAX_ROW - lw) / 2, yPos,buff);
  }
  server.handleClient();
  dnsServer.processNextRequest();
  if (EndConfig) SetDataVarWeb();
  if (onButton) {
    CloseSetEditVarWeb();
    return 0;
  }
  return 1;
}

void handleRoot() {
  //   time_t attTime = 0;
  // time_t nDtime=0;
  // time_t nHtime=0;

  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <title>)rawliteral";
  html += NamePageWeb + R"rawliteral(</title>
  <style>
    * {
      box-sizing: border-box;
      font-family: Arial, sans-serif;
    }
    body {
      margin: 0;
      padding: 15px;
      background: #f5f5f5;
      font-size: 16px;
    }
    .container {
      max-width: 500px;
      margin: 0 auto;
      background: white;
      padding: 20px;
      border-radius: 10px;
      box-shadow: 0 2px 10px rgba(0,0,0,0.1);
    }
    h1 {
      text-align: center;
      color: #333;
      margin-bottom: 25px;
      font-size: 1.5em;
    }
    .field {
      margin-bottom: 20px;
    }
    label {
      display: block;
      margin-bottom: 8px;
      color: #333;
      font-weight: bold;
      font-size: 16px;
    }
    input, select {
      width: 100%;
      padding: 12px;
      border: 2px solid #ddd;
      border-radius: 5px;
      font-size: 16px;
      background: white;
    }
    input:focus {
      border-color: #4CAF50;
      outline: none;
    }
    .range-info {
      display: block;
      margin-top: 5px;
      color: #666;
      font-size: 14px;
    }
    button {
      width: 100%;
      padding: 15px;
      background: #4CAF50;
      color: white;
      border: none;
      border-radius: 5px;
      font-size: 18px;
      font-weight: bold;
      cursor: pointer;
      margin-top: 10px;
    }
    .btn-update {
      background: #4CAF50;
      color: white;
    }
    .btn-update:active {
      background: #45a049;
    }
    .btn-reload {
      background: #2196F3;
      color: white;
    }
    .btn-reload:active {
      background: #0b7dda;
    }
    .checkbox-field {
      display: flex;
      align-items: center;
    }
    .checkbox-field input {
      width: auto;
      margin-right: 10px;
      transform: scale(1.3);
    }
    .message {
      position: fixed;
      top: 20px;
      left: 50%;
      transform: translateX(-50%);
      width: 90%;
      max-width: 400px;
      padding: 20px;
      border-radius: 10px;
      text-align: center;
      font-size: 18px;
      font-weight: bold;
      z-index: 1000;
      box-shadow: 0 4px 15px rgba(0,0,0,0.2);
      display: none;
    }
    .success {
      background: #4CAF50;
      color: white;
    }
    .info {
      background: #2196F3;
      color: white;
    }
    .error {
      background: #f44336;
      color: white;
    }
    @media (max-width: 480px) {
      body {
        padding: 10px;
      }
      .container {
        padding: 15px;
      }
      .message {
        top: 10px;
        width: 95%;
        font-size: 16px;
        padding: 15px;
      }
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>)rawliteral";
  html += NamePageWeb + R"rawliteral(</h1>
    <div id="message" class="message"></div>
    <form action="/update" method="post">
)rawliteral";

  for (size_t i = 0; i < numFields; i++) {
    html += "      <div class=\"field\">\n";

    if (configFields[i].type == ConfigField::BOOL) {
      html += "        <div class=\"checkbox-field\">\n";
      html += "          <input type=\"checkbox\" id=\"field" + String(i) + "\" name=\"field" + String(i) + "\">\n";
      html += "          <label for=\"field" + String(i) + "\">" + configFields[i].label + "</label>\n";
      html += "        </div>\n";
    } else {
      html += "        <label for=\"field" + String(i) + "\">" + configFields[i].label + "</label>\n";

      switch (configFields[i].type) {
        case ConfigField::LONG:
          html += "        <input type=\"number\" id=\"field" + String(i) + "\" name=\"field" + String(i) + "\" step=\"1\" min=\"" + String((long)configFields[i].minVal) + "\" max=\"" + String((long)configFields[i].maxVal) + "\">\n";
          html += "        <span class=\"range-info\">Range: " + String((long)configFields[i].minVal) + " - " + String((long)configFields[i].maxVal) + "</span>\n";
          break;
        case ConfigField::DOUBLE:
          html += "        <input type=\"number\" id=\"field" + String(i) + "\" name=\"field" + String(i) + "\" step=\"0.001\" min=\"" + String(configFields[i].minVal) + "\" max=\"" + String(configFields[i].maxVal) + "\">\n";
          html += "        <span class=\"range-info\">Range: " + String(configFields[i].minVal) + " - " + String(configFields[i].maxVal) + "</span>\n";
          break;
        case ConfigField::STRING:
          html += "        <input type=\"text\" id=\"field" + String(i) + "\" name=\"field" + String(i) + "\" maxlength=\"" + String(configFields[i].maxLen) + "\">\n";
          break;
        case ConfigField::TIME:
          html += "        <input type=\"time\" id=\"field" + String(i) + "\" name=\"field" + String(i) + "\">\n";
          break;
        case ConfigField::DATE:
          html += "        <input type=\"date\" id=\"field" + String(i) + "\" name=\"field" + String(i) + "\">\n";
          break;
        case ConfigField::MENU:
          html += "        <select id=\"field" + String(i) + "\" name=\"field" + String(i) + "\">\n";
          for (int j = 0; j < configFields[i].numOptions; j++) {
            html += "          <option value=\"" + String(j) + "\">" + String(configFields[i].options[j]) + "</option>\n";
          }
          html += "        </select>\n";
          break;
      }
    }
    html += "      </div>\n";
  }

  html += R"rawliteral(
      <button type="submit" class="btn-update">Update data</button>
    </form>
   <button class="btn-reload" onclick="reloadValues()">Reload Values from Server</button>
  </div>
  <script>
    function showMessage(text, type) {
      const messageEl = document.getElementById('message');
      messageEl.textContent = text;
      messageEl.className = 'message ' + type;
      messageEl.style.display = 'block';
      
      // Forza un reflow per assicurare l'animazione
      messageEl.offsetHeight;

      setTimeout(() => {
        messageEl.style.display = 'none';
      }, 3000);
    }

    function loadValues() {
      fetch('/values')
        .then(response => {
          if (!response.ok) {
            throw new Error('Network response was not ok');
          }
          return response.json();
        })
        .then(values => {
          for (let i = 0; i < values.length; i++) {
            const field = document.getElementById('field' + i);
            if (!field) continue;
            
            if (field.type === 'checkbox') {
              field.checked = Boolean(values[i]);
            } else if (field.type === 'select-one') {
              field.value = values[i] || '0';
            } else {
              field.value = values[i] || '';
            }
          }
        })
        .catch(error => {
          console.error('Errore nel caricamento:', error);
          showMessage('Errore nel caricamento dei valori', 'error');
        });
    }

   function reloadValues() {
      fetch('/reload')
        .then(response => {
          if (!response.ok) {
            throw new Error('Network response was not ok');
          }
          return response.text();
        })
        .then(message => {
          showMessage(message, 'info');
          // Ricarica i valori dopo un breve delay
          setTimeout(loadValues, 100);
        })
        .catch(error => {
          console.error('Error reloading:', error);
          showMessage('Error reloading', 'error');
        });
    }

    // Gestione dell'invio del form
    document.querySelector('form').addEventListener('submit', function(e) {
      e.preventDefault();
      
      const formData = new FormData(this);
      fetch('/update', {
        method: 'POST',
        body: formData
      })
      .then(response => {
        if (!response.ok) {
          throw new Error('Network response was not ok');
        }
        return response.text();
      })
      .then(message => {
        showMessage(message, 'success');
        // Ricarica i valori dopo l'aggiornamento per conferma
        setTimeout(loadValues, 500);
      })
      .catch(error => {
        console.error('Error in update:', error);
        showMessage('Error in update', 'error');
      });
    });

    // Carica valori correnti all'avvio
    document.addEventListener('DOMContentLoaded', function() {
      setTimeout(loadValues, 100);
    });
    
    // Gestione errori non catturati
    window.addEventListener('error', function(e) {
      console.error('Global error:', e.error);
      showMessage('An error occurred', 'error');
    });

  </script>
</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
}

void handleUpdate() {
  for (size_t i = 0; i < numFields; i++) {
    // Gestione speciale per i campi booleani
    if (configFields[i].type == ConfigField::BOOL) {
      // Per le checkbox, se esistono nell'argomento sono "on", altrimenti sono false
      *static_cast<bool*>(configFields[i].variable) = server.hasArg("field" + String(i));
    } else if (server.hasArg("field" + String(i))) {
      String argValue = server.arg("field" + String(i));
      switch (configFields[i].type) {
        case ConfigField::LONG:
          *static_cast<long*>(configFields[i].variable) = argValue.toInt();
          break;
        case ConfigField::DOUBLE:
          *static_cast<double*>(configFields[i].variable) = argValue.toDouble();
          break;
        case ConfigField::STRING:
          argValue.toCharArray(static_cast<char*>(configFields[i].variable), configFields[i].maxLen);
          break;
        case ConfigField::TIME:
          *static_cast<time_t*>(configFields[i].variable) = stringToTime(argValue);
          break;
        case ConfigField::DATE:
          *static_cast<time_t*>(configFields[i].variable) = stringToDate(argValue);
          break;
        case ConfigField::MENU:
          *static_cast<long*>(configFields[i].variable) = argValue.toInt();
          break;
      }
    }
  }
  server.send(200, "text/plain", "Configuration updated!");
  EndConfig = true;
}

void handleGetValues() {
  StaticJsonDocument<512> doc;
  JsonArray array = doc.to<JsonArray>();
  nHtime = attTime % 86400;
  nDtime = attTime - nHtime;
  for (size_t i = 0; i < numFields; i++) {
    switch (configFields[i].type) {
      case ConfigField::BOOL:
        array.add(*static_cast<bool*>(configFields[i].variable));
        break;
      case ConfigField::LONG:
        array.add(*static_cast<long*>(configFields[i].variable));
        break;
      case ConfigField::DOUBLE:
        array.add(*static_cast<double*>(configFields[i].variable));
        break;
      case ConfigField::STRING:
        array.add(static_cast<char*>(configFields[i].variable));
        break;
      case ConfigField::TIME:
        array.add(timeToString(*static_cast<time_t*>(configFields[i].variable)));
        break;
      case ConfigField::DATE:
        array.add(dateToString(*static_cast<time_t*>(configFields[i].variable)));
        break;
      case ConfigField::MENU:
        array.add(*static_cast<long*>(configFields[i].variable));
        break;
    }
  }

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleReload() {
  server.send(200, "text/plain", "Values ​​reloaded from the server");
}