
//
// Global variables
//

#include "common.h"

const uint8_t lastNextMealChangeTimer_TO = 5;      //s
const uint8_t backlightTurnOffTimer_TO = 15;      //s
const uint16_t debouncingTimer_TO = 300;           //ms
const uint16_t extButtonDebouncingTimer_TO = 1000;  //ms
const uint8_t hungryCatAlarm_TO = 2;               //s
const uint16_t screenAnimationFrameDuration = 50;  //ms

const float chargeMinimumVoltage = 2.7f;
const float chargeMaximumVoltage = 4.2f;

ScreenType currentScreen = ScreenType::MAIN;
Datime setTimeTemp;

FedStatusDef FedStatus;
SettingsDef Settings, SettingsCopy;
TimersDef Timers; 
ScreenAnimationDef ScreenAnimation;
CursorsDef Cursors;
BatteryInfoDef BatteryInfo;

bool keyHandleStatus = false;
bool backlightStatus = true;
bool usbConnectionStatus = false;
bool batteryHandleStatus = false;

bool tenMsExpired = false;
bool externalButtonClicked = false;

GyverDS3231 rtc;
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);


CursorSettings& operator++(CursorSettings& val, int) 
{
	const int i = static_cast<int>(val)+1;
  val = static_cast<CursorSettings>(i%7);
	return val;
}

CursorTimeset& operator++(CursorTimeset& val, int) 
{
	const int i = static_cast<int>(val)+1;
  val = static_cast<CursorTimeset>(i%6);
	return val;
}