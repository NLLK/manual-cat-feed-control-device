#include "presentation.h"

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
    drewStars = false;
  }

  lcd.setCursor(x_offset + 0, 1);

  if (Timers.lastNextMealChange_FF_status) {
    lcd.print("Next: ");
    printHoursAndMinutes(FedStatus.nextMeal.hour, FedStatus.nextMeal.minute);
  } else {

    lcd.print("Last: ");
    printHoursAndMinutes(FedStatus.lastMeal.hour, FedStatus.lastMeal.minute);
  }

  lcd.setCursor(x_offset + 12, 1);
  lcd.print(FedStatus.isFed ? " FED" : "HNGR");
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

void showRtcErrorMessage(){
  lcd.setCursor(0,0);
  lcd.print("NO RTC DETECTED");
  lcd.setCursor(0,1);
  lcd.print("OR OTHER RTC ERR");
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
