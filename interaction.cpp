#include "interaction.h"

void interact(Key key) {

  if (usbConnectionStatus){
    Serial.print("Key pressed: ");
    Serial.println((int)key);
  }

  Timers.backlightTurnOff = 0;

  if (!backlightStatus) {
    backlightStatus = true;
    return;  //ignore the first tap if backlight is off
  }

  switch (currentScreen) {
    case ScreenType::MAIN:      interact_MAIN(key);     break;
    case ScreenType::SETTINGS:  interact_SETTINGS(key); break;
    case ScreenType::TIMESET:   interact_TIMESET(key);  break;
    default: break;
  }
}

void interact_MAIN(Key key) {
  switch (key) {
    case Key::ENTER:
    {
      UC_CatIsFed();
      updateScreen();
      break;
    }
    case Key::LEFT:
    {
      break;
    }
    case Key::RIGHT:
    {
      changeScreen(ScreenType::SETTINGS);
      ScreenAnimation.directionToRight = true;
      break;
    }
    case Key::UP:
    {
      UC_SetLastNextFFState(false);
      updateScreen();
      break;
    }
    case Key::DOWN:
    {
      UC_SetLastNextFFState(true);
      updateScreen();
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
        Cursors.settingsSaveState = false;
        Settings = SettingsCopy;
        UC_SetMealsForToday();
        UC_saveSettings();
        UC_SetNextMeal();
      }
      break;
    }
    case Key::RIGHT:
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
        Cursors.timesetSaveState = false;
        rtc.setTime(setTimeTemp);
        UC_SetMealsForToday();
        UC_SetNextMeal();
      }
      break;
    }
    case Key::RIGHT:
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
  UC_SetLastNextFFState(false);

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
    if (usbConnectionStatus){
      Serial.println(periods[i]->toString() + " " + currentTime.toString());
    }
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
      if (usbConnectionStatus) Serial.println("next "+meals[i]->toString());
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
    if (usbConnectionStatus) Serial.println("today "+meals[i]->toString());
  }
}

void UC_saveSettings() {
  Datime* periods[3] = { &Settings.firstMeal, &Settings.secondMeal, &Settings.thirdMeal };

  for (int i = 0; i < 3; i++) {
    byte data[2] = { periods[i]->hour, periods[i]->minute };

    if (usbConnectionStatus){

      Serial.println("Save settings to EEPROM: ");
      Serial.print(data[0]);
      Serial.print(" ");
      Serial.println(data[1]);
    }
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

void UC_SetLastNextFFState(bool isNext){
  Timers.lastNextMealChange = 0;
  Timers.lastNextMealChange_FF_status = isNext;
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

void screenAnimationHandle(){

  if (!tenMsExpired) return;

  tenMsExpired = false;

  if (!ScreenAnimation.status) return;
  
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
