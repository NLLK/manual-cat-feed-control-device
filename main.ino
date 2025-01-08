#include <LiquidCrystal.h>
#include <Time.h>
#include <EEPROM.h>
#include <GyverDS3231.h>
#include "timer-api.h"

#define LEDlcdPin 10

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
//
// Global variables
//

const uint8_t lastNextMealChangeTimer_TO = 5;      //s
const uint8_t backlightTurnOffTimer_TO = 120;      //s
const uint16_t debouncingTimer_TO = 300;           //ms
const uint8_t hungryCatAlarm_TO = 2;               //s
const uint16_t screenAnimationFrameDuration = 50;  //ms

ScreenType currentScreen = ScreenType::MAIN;
Datime setTimeTemp;
struct {
  Datime firstMeal;
  Datime secondMeal;
  Datime thirdMeal;
} Settings, SettingsCopy;

struct {
  bool isFed;
  Datime nextMeal;
  Datime lastMeal;
  uint8_t hoursHungry;
} FedStatus;

struct {
  uint8_t lastNextMealChange;
  bool lastNextMealChange_FF_status;

  uint16_t debouncing;
  bool debouncingStatus;

  uint8_t backlightTurnOff;

  uint8_t hungryCatAlarmChange;
  uint8_t hungryCatAlarmChange_FF_status;

} Timers;

struct {
  uint8_t counter;
  bool status;
  uint16_t timer;
  bool directionToRight;
  bool offset;
} ScreenAnimation;

struct {
  CursorSettings settings;
  bool settingsSaveState;

  CursorTimeset timeset;
  bool timesetSaveState;
} Cursors;

bool backlightStatus = true;
bool hungryCatAlarmStatus = false;
bool keyHandleStatus = false;

bool tenMsExpired = false;
uint16_t currentYearDay = 0;
//
// setup
//

GyverDS3231 rtc;
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

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

void loadSettings() {

  bool isOk = true;

  Datime* periods[3] = { &Settings.firstMeal, &Settings.secondMeal, &Settings.thirdMeal };

  for (int i = 0; i < 3; i++) {
    byte dataRead[2] = {0,};
    EEPROM.get(0 + i * 2, dataRead[0]);
    EEPROM.get(1 + i * 2, dataRead[1]);

    Serial.print("Got EEPROM Data at: ");
    Serial.print(i * 2);
    Serial.print(": ");
    Serial.print(dataRead[0]);
    Serial.print(" ");
    Serial.println(dataRead[1]);

    if (dataRead[0] == 0xFF || dataRead[1] == 0xFF) {
      isOk = false;
      break;
    }
    *periods[i] = Datime(0, 0, 0, dataRead[0], dataRead[1], 0);
  }

  //default settings
  if (!isOk) {
    Serial.println("Use default settings");
    Settings.firstMeal = Datime(0, 0, 0, 8, 0, 0);
    Settings.secondMeal = Datime(0, 0, 0, 18, 0, 0);
    Settings.thirdMeal = Datime(0, 0, 0, 23, 0, 0);
    UC_saveSettings();
  }
  UC_SetMealsForToday();
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

  if (key != Key::NONE) {
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
}

Key keys(int xKeyVal) {
  if (xKeyVal < 60) return Key::RIGHT;
  else if (xKeyVal < 200) return Key::UP;
  else if (xKeyVal < 400) return Key::DOWN;
  else if (xKeyVal < 600) return Key::LEFT;
  else if (xKeyVal < 800) return Key::ENTER;
  else return Key::NONE;
}

void screenAnimationHandle(){
  if (tenMsExpired){
    tenMsExpired = false;

    if (ScreenAnimation.status) {
      ScreenAnimation.timer += 10;
      if (ScreenAnimation.timer >= screenAnimationFrameDuration) {
        ScreenAnimation.timer = 0;
        ScreenAnimation.counter++;

        if (ScreenAnimation.counter >= 16) {
          UC_finishAnimation();
        } else {
          if (ScreenAnimation.directionToRight){
            lcd.scrollDisplayLeft();
          }else{
            //not implented
            //lcd.scrollDisplayRight();
          }
        }
      }
    }
  }
}

void interact(Key key) {

  Serial.print("Key pressed: ");
  Serial.println((int)key);

  Timers.backlightTurnOff = 0;

  if (!backlightStatus) {
    backlightStatus = true;
    return;  //ignore the first tap if backlight is off
  }

  switch (currentScreen) {
    case ScreenType::MAIN: interact_MAIN(key); break;
    case ScreenType::SETTINGS: interact_SETTINGS(key); break;
    case ScreenType::TIMESET: interact_TIMESET(key); break;
    default: break;
  }
}

void interact_MAIN(Key key) {
  switch (key) {
    case Key::ENTER:
    {
      UC_CatIsFed();
      break;
    }
    case Key::LEFT:
    {
      changeScreen(ScreenType::SETTINGS);
      ScreenAnimation.directionToRight = true;
      break;
    }
    default: break;
  }
}

void interact_SETTINGS(Key key) {
  switch (key) {
    case Key::ENTER:
    {
      Cursors.settings++;
      if (Cursors.settingsSaveState){
        Settings = SettingsCopy;
        UC_SetMealsForToday();
        UC_saveSettings();
        UC_SetNextMeal();
      }
      break;
    }
    case Key::LEFT:
    {
      changeScreen(ScreenType::TIMESET);
      ScreenAnimation.directionToRight = true;
      break;
    }
    default: break;
  }

  if (key != Key::UP && key != Key::DOWN) return;

  switch (Cursors.settings){
    case CursorSettings::FirstHours:{
      SettingsCopy.firstMeal.hour += (key == Key::UP ? 1 : -1);
      break;
    }
    case CursorSettings::FirstMinutes:{
      SettingsCopy.firstMeal.minute += (key == Key::UP ? 5 : -5);
      break;
    }
    case CursorSettings::SecondHours:{
      SettingsCopy.secondMeal.hour += (key == Key::UP ? 1 : -1);
      break;
    }
    case CursorSettings::SecondMinutes:{
      SettingsCopy.secondMeal.minute += (key == Key::UP ? 5 : -5);
      break;
    }
    case CursorSettings::ThirdHours:{
      SettingsCopy.thirdMeal.hour += (key == Key::UP ? 1 : -1);
      break;
    }
    case CursorSettings::ThirdMinutes:{
      SettingsCopy.thirdMeal.minute += (key == Key::UP ? 5 : -5);
      break;
    }
    case CursorSettings::Save:{
      Cursors.settingsSaveState = key == Key::UP;
      break;
    }
  }

  Datime* meals[3] = { &SettingsCopy.firstMeal, &SettingsCopy.secondMeal, &SettingsCopy.thirdMeal };
  for (int i = 0; i < 3; i++){
    if (meals[i]->hour > 23)
      meals[i]->hour = 0;

    if (meals[i]->minute > 59)
      meals[i]->minute = 0;
  }
}

void interact_TIMESET(Key key) {
  switch (key) {
    case Key::ENTER:
    {
      Cursors.timeset++;
      if (Cursors.timesetSaveState){
        rtc.setTime(setTimeTemp);
        //UC_SetMealsForToday();
        //UC_SetNextMeal(); //TODO: return it back
      }
      break;
    }
    case Key::LEFT:
    {
      changeScreen(ScreenType::MAIN);
      ScreenAnimation.directionToRight = true;
      break;
    }
    default: break;
  }
  
  if (key != Key::UP && key != Key::DOWN) return;

  switch (Cursors.timeset){
    case CursorTimeset::Year:{
      setTimeTemp.year += key == Key::UP ? 1 : -1;
      break;
    }
    case CursorTimeset::Month:{
      setTimeTemp.month += key == Key::UP ? 1 : -1;
      break;
    }
    case CursorTimeset::Day:{
      setTimeTemp.day += key == Key::UP ? 1 : -1;
      break;
    }
    case CursorTimeset::Hours:{
      setTimeTemp.hour += key == Key::UP ? 1 : -1;
      break;
    }
    case CursorTimeset::Minutes:{
      setTimeTemp.minute += key == Key::UP ? 1 : -1;
      break;
    }
    case CursorTimeset::Save:{
      Cursors.timesetSaveState = key == Key::UP;
      break;
    }
    default: break;
  }

  if (setTimeTemp.year > 2038) setTimeTemp.year = 2038;
  if (setTimeTemp.year < 2024) setTimeTemp.year = 2024;

  if (setTimeTemp.month >= 12) setTimeTemp.month = 1;
  if (setTimeTemp.month == 0) setTimeTemp.month = 12; 

  if (setTimeTemp.day > StampUtils::daysInMonth(setTimeTemp.month, setTimeTemp.year)) setTimeTemp.day = 1;
  if (setTimeTemp.day == 0) setTimeTemp.day = StampUtils::daysInMonth(setTimeTemp.month, setTimeTemp.year); 

  if (setTimeTemp.hour > 23) setTimeTemp.hour = 0;
  if (setTimeTemp.minute > 59) setTimeTemp.minute = 0;
}

//
// Use cases
//

void UC_CatIsFed() {

  hungryCatAlarmStatus = false;
  Timers.hungryCatAlarmChange = 0;
  Timers.hungryCatAlarmChange_FF_status = false;

  FedStatus.isFed = true;
  FedStatus.lastMeal = rtc.getTime();
  FedStatus.hoursHungry = 0;

  UC_SetNextMeal();
}

void UC_SetNextMeal() {

  Datime nextDay = Settings.firstMeal;
  nextDay.addDays(1);

  Datime* periods[4] = { &Settings.firstMeal, &Settings.secondMeal, &Settings.thirdMeal, &nextDay }; 
  Datime currentTime = rtc.getTime();

  for (int i = 0; i < 4; i++) {
    Serial.println(periods[i]->toString() + " " + currentTime.toString());
    if (*periods[i] > currentTime){
      // it was the last meal for today - prepare for the next day
      if (i == 3) {
        UC_SetMealsToNextDay();
      }

      FedStatus.nextMeal = *periods[i];
      break;
    }
  }

  Serial.println("Next meal set to: " + FedStatus.nextMeal.toString());
}

void UC_SetMealsToNextDay() {
  Datime* meals[3] = { &Settings.firstMeal, &Settings.secondMeal, &Settings.thirdMeal };
  for (int i = 0; i < 3; i++) {
    if (meals[i]->dayIndex() == rtc.getTime().dayIndex())
      meals[i]->addDays(1);
      Serial.println("next "+meals[i]->toString());
  }
}

void UC_SetMealsForToday() {
  Datime* meals[3] = { &Settings.firstMeal, &Settings.secondMeal, &Settings.thirdMeal };

  Datime currentTime = rtc.getTime();

  for (int i = 0; i < 3; i++) {
    Datime copy = *meals[i];
    meals[i]->year = currentTime.year;
    meals[i]->month = currentTime.month;
    meals[i]->day = currentTime.day;
    Serial.println("today "+meals[i]->toString());
  }
}

void UC_saveSettings() {
  Datime* periods[3] = { &Settings.firstMeal, &Settings.secondMeal, &Settings.thirdMeal };

  for (int i = 0; i < 3; i++) {
    byte data[2] = { periods[i]->hour, periods[i]->minute };
    Serial.println("Save settings to EEPROM: ");
    Serial.print(data[0]);
    Serial.print(" ");
    Serial.println(data[1]);
    EEPROM.put(0 + i * 2, data);
  }
}

void UC_finishAnimation() {
  ScreenAnimation.counter = 0;
  ScreenAnimation.status = false;
  ScreenAnimation.offset = false;
  lcd.clear();
  lcd.home();
  updateScreen();
}

//
// Presentation
//

void changeScreen(ScreenType newScreen) {
  if (newScreen == currentScreen) return;

  Serial.print("Change screen to ");
  Serial.println((int)newScreen);

  ScreenAnimation.status = true;
  currentScreen = newScreen;
  ScreenAnimation.offset = true;

  SettingsCopy = Settings;
  Cursors.settings = CursorSettings::FirstHours;
  Cursors.settingsSaveState = false;
  
  setTimeTemp = rtc.getTime();
  Cursors.timeset = CursorTimeset::Year;
  Cursors.timesetSaveState = false;

  updateScreen();
}

void updateScreen() {
  lcd.noBlink();
  lcd.noCursor();
  switch (currentScreen) {
    case ScreenType::MAIN: updateScreen_MAIN(); break;
    case ScreenType::SETTINGS: updateScreen_SETTINGS(); break;
    case ScreenType::TIMESET: updateScreen_TIMESET(); break;
    default: break;
  }
}

void updateScreen_MAIN() {
  uint8_t x_offset = ScreenAnimation.offset * 16;
  lcd.setCursor(x_offset + 0, 0);

  Datime time = rtc.getTime();

  lcd.print(time.day < 10 ? "0" : "");
  lcd.print(time.day);
  lcd.print(".");
  lcd.print(time.month < 10 ? "0" : "");
  lcd.print(time.month);
  lcd.print(" ");
  printHoursAndMinutes(time.hour, time.minute);

  static bool drewStars = false;

  if (!FedStatus.isFed && FedStatus.hoursHungry != 0) {
    uint8_t limited = FedStatus.hoursHungry > 3 ? 4 : FedStatus.hoursHungry;
    lcd.setCursor(x_offset + 16 - limited, 0);

    String stars = "";
    for (int i = 0; i < limited; i++) {
      stars += '*';
    }
    lcd.print(stars);
    drewStars = true;
  } else if (drewStars) {
    lcd.setCursor(x_offset + 12, 0);
    lcd.print("    ");
  }

  lcd.setCursor(x_offset + 0, 1);

  if (Timers.lastNextMealChange_FF_status) {
    lcd.print("Next: ");
    printHoursAndMinutes(FedStatus.nextMeal.hour, FedStatus.nextMeal.minute);
  } else {

    lcd.print("Last: ");
    printHoursAndMinutes(FedStatus.lastMeal.hour, FedStatus.lastMeal.minute);
  }

  if (FedStatus.isFed) {
    lcd.setCursor(x_offset + 12, 1);
    lcd.print(" FED");
  } else {
    lcd.setCursor(x_offset + 12, 1);
    lcd.print("HNGR");
  }
}

void updateScreen_SETTINGS() {
  uint8_t x_offset = ScreenAnimation.offset * 16;
  lcd.setCursor(x_offset, 0);

  lcd.print("1.");
  printHoursAndMinutes(SettingsCopy.firstMeal.hour, SettingsCopy.firstMeal.minute);

  lcd.setCursor(x_offset+8, 0);
  if (Cursors.settings == CursorSettings::Save){
    lcd.print("Save?  ");
    lcd.print(Cursors.settingsSaveState ? "V" : "X");
  }else{
    lcd.print("Settings");
  }

  lcd.setCursor(x_offset, 1);
  lcd.print("2.");
  printHoursAndMinutes(SettingsCopy.secondMeal.hour, SettingsCopy.secondMeal.minute);

  lcd.setCursor(x_offset+8, 1);
  lcd.print("3.");
  printHoursAndMinutes(SettingsCopy.thirdMeal.hour, SettingsCopy.thirdMeal.minute);

  if (ScreenAnimation.offset) return;

  lcd.blink();
  lcd.cursor();
  switch(Cursors.settings){
    case CursorSettings::FirstHours: {
      lcd.setCursor(3,0);
      break;
    }
    case CursorSettings::FirstMinutes: {
      lcd.setCursor(6,0);
      break;
    }
    case CursorSettings::SecondHours: {
      lcd.setCursor(3,1);
      break;
    }
    case CursorSettings::SecondMinutes: {
      lcd.setCursor(6,1);
      break;
    }
    case CursorSettings::ThirdHours: {
      lcd.setCursor(11,1);
      break;
    }
    case CursorSettings::ThirdMinutes: {
      lcd.setCursor(14,1);
      break;
    }
    case CursorSettings::Save: {
      lcd.setCursor(15,0);
      break;
    }
    default:
      lcd.noBlink();
      lcd.noCursor();
      break;
  }
}

void updateScreen_TIMESET() {
  uint8_t x_offset = ScreenAnimation.offset * 16;

  lcd.setCursor(x_offset+8, 0);
  if (Cursors.timeset == CursorTimeset::Save){
    lcd.print("Save?  ");
    lcd.print(Cursors.timesetSaveState ? "V" : "X");
  }else{
    lcd.print("Time set");
  }

  lcd.setCursor(x_offset, 1);
  lcd.print(setTimeTemp.year);
  lcd.print("/");
  lcd.print(setTimeTemp.month < 10 ? "0" : "");
  lcd.print(setTimeTemp.month);
  lcd.print("/");
  lcd.print(setTimeTemp.day < 10 ? "0" : "");
  lcd.print(setTimeTemp.day);
  lcd.print(" ");
  printHoursAndMinutes(setTimeTemp.hour,setTimeTemp.minute);

  if (ScreenAnimation.offset) return;

  lcd.blink();
  lcd.cursor();
  switch(Cursors.timeset){
    case CursorTimeset::Year: {
      lcd.setCursor(3,1);
      break;
    }
    case CursorTimeset::Month: {
      lcd.setCursor(6,1);
      break;
    }
    case CursorTimeset::Day: {
      lcd.setCursor(9,1);
      break;
    }
    case CursorTimeset::Hours: {
      lcd.setCursor(12,1);
      break;
    }
    case CursorTimeset::Minutes: {
      lcd.setCursor(15,1);
      break;
    }
    case CursorTimeset::Save: {
      lcd.setCursor(15,0);
      break;
    }
    default:
      lcd.noBlink();
      lcd.noCursor();
      break;
  }
}

//
// Utils
//

void printHoursAndMinutes(int hours, int minutes) {
  lcd.print(hours < 10 ? "0" : "");
  lcd.print(hours);
  lcd.print(":");
  lcd.print(minutes < 10 ? "0" : "");
  lcd.print(minutes);
}
