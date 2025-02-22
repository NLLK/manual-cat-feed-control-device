#ifndef COMMON_H
#define COMMON_H

#include <LiquidCrystal.h>
#include <GyverDS3231.h>
#include <GyverPower.h>

//
// definitions
//

enum class ScreenType {
  MAIN,
  SETTINGS,
  TIMESET
};

enum class Key {
  NONE,
  ENTER,
  LEFT,
  RIGHT,
  UP,
  DOWN
};

enum class CursorSettings{
  FirstHours,
  FirstMinutes,
  SecondHours,
  SecondMinutes,
  ThirdHours,
  ThirdMinutes,
  Save
};

enum class CursorTimeset{
  Year,
  Month,
  Day,
    
  Hours,
  Minutes,
  Save
};

CursorSettings& operator++(CursorSettings& val, int);
CursorTimeset& operator++(CursorTimeset& val, int);

//
// Global variables
//

extern const uint8_t lastNextMealChangeTimer_TO;      //s
extern const uint8_t backlightTurnOffTimer_TO;        //s
extern const uint16_t debouncingTimer_TO;             //ms
extern const uint16_t extButtonDebouncingTimer_TO;    //ms
extern const uint8_t hungryCatAlarm_TO;               //s
extern const uint16_t screenAnimationFrameDuration;   //ms

extern ScreenType currentScreen;
extern Datime setTimeTemp;

struct SettingsDef{
  Datime firstMeal;
  Datime secondMeal;
  Datime thirdMeal;
};
extern SettingsDef Settings, SettingsCopy;

struct FedStatusDef{
  bool isFed;
  Datime nextMeal;
  Datime lastMeal;
  uint8_t hoursHungry;
};
extern FedStatusDef FedStatus;

struct TimersDef {
  uint8_t lastNextMealChange;
  bool lastNextMealChange_FF_status;

  uint16_t debouncing;
  bool debouncingStatus;
  
  uint16_t extDebouncing;
  bool extDebouncingStatus;

  uint8_t backlightTurnOff;
};
extern TimersDef Timers;

struct ScreenAnimationDef{
  uint8_t counter;
  bool status;
  uint16_t timer;
  bool directionToRight;
  bool offset;
};
extern ScreenAnimationDef ScreenAnimation;

struct CursorsDef{
  CursorSettings settings;
  bool settingsSaveState;

  CursorTimeset timeset;
  bool timesetSaveState;
};
extern CursorsDef Cursors;

extern bool backlightStatus;
extern bool keyHandleStatus;
extern bool usbConnectionStatus;

extern bool tenMsExpired;
extern bool externalButtonClicked;

extern GyverDS3231 rtc;
extern LiquidCrystal lcd;

#endif