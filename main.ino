#include <EEPROM.h>

#include "timer-api.h"
#include "presentation.h"
#include "interaction.h"
#include "common.h"

#define LEDlcdPin 10

void setup() {

  timer_init_ISR_100Hz(TIMER_DEFAULT);

  pinMode(LEDlcdPin, OUTPUT);
  digitalWrite(LEDlcdPin, LOW);

  Wire.begin();
  Serial.begin(9600);
  lcd.begin(16, 2);
  rtc.begin();

  if (!rtc.updateNow()){
    rtc.setBuildTime();
  }

  //load settings from eeprom
  loadSettings();
  digitalWrite(LEDlcdPin, HIGH);
}

//
// logic
//

void loop() {
  if (rtc.tick()) {
    static bool inited = false;
    if (!inited) {
      inited = true;

      UC_CatIsFed();
    }

    secondsInt();
  }

  screenAnimationHandle();
  backlightHandle();
  keysHandle();
}

void secondsInt(void) {

  if (currentYearDay != rtc.yearDay()){
    currentYearDay = rtc.yearDay();
    UC_SetMealsForToday();
  }

  if (backlightStatus) {
    Timers.backlightTurnOff++;

    if (Timers.backlightTurnOff > backlightTurnOffTimer_TO) {
      Timers.backlightTurnOff = 0;
      backlightStatus = false;
    }
  } else {
    Timers.backlightTurnOff = 0;
  }

  Timers.lastNextMealChange++;
  if (Timers.lastNextMealChange >= lastNextMealChangeTimer_TO) {
    Timers.lastNextMealChange = 0;
    Timers.lastNextMealChange_FF_status = !Timers.lastNextMealChange_FF_status;
  }

  if (hungryCatAlarmStatus) {
    Timers.hungryCatAlarmChange++;
    if (Timers.hungryCatAlarmChange >= hungryCatAlarm_TO) {
      Timers.hungryCatAlarmChange = 0;
      Timers.hungryCatAlarmChange_FF_status = !Timers.hungryCatAlarmChange_FF_status;
    }
  }

  if (rtc.getTime() > FedStatus.nextMeal) {
    FedStatus.isFed = false;
  }

  if (!FedStatus.isFed) {
    int secondsHungry = rtc.getTime().daySeconds() - FedStatus.nextMeal.daySeconds();

    if (secondsHungry > 0) {
      FedStatus.hoursHungry = secondsHungry / 3600;

      if (FedStatus.hoursHungry >= 3) {
        hungryCatAlarmStatus = true;
      }
    }
  }

  if (!ScreenAnimation.status)
    updateScreen();
}

//10 ms timer
void timer_handle_interrupts(int timer) {
  tenMsExpired = true;

  static uint8_t counter = 0;
  counter++;
  if (counter == 5) {
    counter = 0;
    keyHandleStatus = true;
  }

  if (Timers.debouncingStatus) {
    Timers.debouncing += 10;
  }
}

void backlightHandle() {

  bool wantedStatus = backlightStatus || Timers.hungryCatAlarmChange_FF_status;

  static bool lastBacklightStatus = true;
  if (lastBacklightStatus != wantedStatus) {
    lastBacklightStatus = wantedStatus;
    digitalWrite(LEDlcdPin, wantedStatus ? HIGH : LOW);
  }
}

void keysHandle() {
  if (!keyHandleStatus) return;

  keyHandleStatus = false;
  Key key = keys(analogRead(0));

  if (key == Key::NONE) return;

  if (!Timers.debouncingStatus) {
    Timers.debouncingStatus = true;
  } else {
    if (Timers.debouncing >= debouncingTimer_TO) {
      Timers.debouncingStatus = false;
      Timers.debouncing = 0;

      interact(key);
      updateScreen();
    }
  }  
}

Key keys(int xKeyVal) {
  if (xKeyVal < 60) return Key::RIGHT;
  else if (xKeyVal < 200) return Key::UP;
  else if (xKeyVal < 400) return Key::DOWN;
  else if (xKeyVal < 600) return Key::LEFT;
  else if (xKeyVal < 800) return Key::ENTER;
  else return Key::NONE;
}
