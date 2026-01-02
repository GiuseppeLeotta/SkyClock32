#include <Arduino.h>
#include <queue>
#include <driver/timer.h>
#include "button.h"

#define DEBOUNCE_US 10  // in millisec
#define MAX_QUEUE 20

std::queue<unsigned long> eventQueue;

// Variabili di stato - ORA NON PIÙ VOLATILE perché protette dal timer

bool buttonPressed = false;
bool lastStableState = HIGH;
unsigned long pressStartTime = 0;
uint16_t buttonPin=0;

// Timer handle
hw_timer_t *debounceTimer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;



// ISR leggera - solo per rilevare i fronti
void IRAM_ATTR buttonEdgeISR() {
  // Ferma il timer e lo riavvia (reset del debounce)
  timerStop(debounceTimer);
  timerStart(debounceTimer);
}

// Timer ISR - qui avviene il debounce e la logica di stato
void IRAM_ATTR debounceTimerISR() {
  portENTER_CRITICAL_ISR(&timerMux);

  bool currentState = digitalRead(buttonPin);

  // Se lo stato è stabile dopo il periodo di debounce
  if (currentState != lastStableState) {
    lastStableState = currentState;

    if (currentState == LOW) {
      // Pulsante premuto stabilmente
      if (!buttonPressed) {
        buttonPressed = true;
        pressStartTime = millis();
      }
    } else {
      // Pulsante rilasciato stabilmente
      if (buttonPressed) {
        buttonPressed = false;
        unsigned long duration = millis() - pressStartTime;
        if (duration >= DEBOUNCE_US && eventQueue.size()<MAX_QUEUE) {
        // Aggiungi alla coda (in ISR dobbiamo fare attenzione)
        eventQueue.push(duration);
      }
    }
  }
}

portEXIT_CRITICAL_ISR(&timerMux);
}

int initButton(uint16_t nPin) {
  int ret=0;
  buttonPin=nPin;
  pinMode(buttonPin, INPUT_PULLUP);

  // Configura interrupt sul pin
  attachInterrupt(digitalPinToInterrupt(buttonPin), buttonEdgeISR, CHANGE);

  // Configura timer hardware per debounce
  //debounceTimer = timerBegin(0, 80, true); // Timer 0, prescaler 80 (1MHz), count up
  debounceTimer = timerBegin(1000000);
  if (debounceTimer) {
    timerAttachInterrupt(debounceTimer, &debounceTimerISR);
  //timerAlarmWrite(debounceTimer, DEBOUNCE_US); // 50ms, auto-reload
  //timerAlarmEnable(debounceTimer);
    timerAlarm(debounceTimer, DEBOUNCE_US*1000, false, 0);
    ret=1;
  }
  return ret;
}

bool onButtonQueque(uint32_t & tButt) {
  bool ret=false;
  tButt=0;
  if (!eventQueue.empty()) {
     tButt=eventQueue.front();
     eventQueue.pop();
     ret=true;
  } else if (buttonPressed) {
    tButt=millis()-pressStartTime; 
  }
  return ret;
}

void clearQuequeButton(void) {
  while (!eventQueue.empty())  eventQueue.pop();
}

int numInQuequeButton(void) {
  return eventQueue.size();
}
