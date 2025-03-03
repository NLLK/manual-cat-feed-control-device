#include <EEPROM.h>

#include "timer-api.h"
#include "presentation.h"
#include "interaction.h"
#include "common.h"

#define LEDlcdPin 10
#define RTC_POWER_PIN 12
#define EXTERNAL_BUTTON_PIN 1
#define KEYBOARD_ANALOG_PIN 0
#define BATTERY_VOLTAGE_ADC_PIN 1
#define BATTERY_CHARGING_PIN 11
#define BATTERY_DONE_CHG_PIN 13
void setup() {

  timer_init_ISR_100Hz(TIMER_DEFAULT);

  pinMode(LEDlcdPin, OUTPUT);
  digitalWrite(LEDlcdPin, LOW);

  pinMode(RTC_POWER_PIN, OUTPUT);
  digitalWrite(RTC_POWER_PIN, HIGH);

  Wire.begin();
  Serial.begin(9600);
  Serial.setTimeout(10);
  lcd.begin(16, 2);

  lcd.setCursor(0,0);
  lcd.print("INIT");

  bool status = rtc.begin() && rtc.isOK();
  if (!status){
    showRtcErrorMessage();
  }

  if (!rtc.updateNow()){
    rtc.setBuildTime();
  }

  //load settings from eeprom
  loadSettings();
  digitalWrite(LEDlcdPin, HIGH);

  attachInterrupt(digitalPinToInterrupt(EXTERNAL_BUTTON_PIN), handleExternalButtonInterrupt, RISING);
  pinMode(EXTERNAL_BUTTON_PIN, INPUT_PULLUP);

  pinMode(BATTERY_CHARGING_PIN, INPUT);
  pinMode(BATTERY_DONE_CHG_PIN, INPUT);

  while(!rtc.tick()){
    //wait for one seconds tick from rtc
  }

  usbConnectionStatus = true;
  lcd.clear();
  UC_CatIsFed();
}



//
// logic
//

void loop() {
  if (rtc.tick()) {
    secondsInt();
  }

  if (!rtc.isOK()){
    showRtcErrorMessage();
  }

  screenAnimationHandle();
  powerHandle();
  keysHandle();
}

void secondsInt(void) {
  if (!usbConnectionStatus && Serial.available() > 0){
    Serial.readString();
    usbConnectionStatus = true;
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

  hungryAndTimeManagement();

  batteryHandle();

  if (!ScreenAnimation.status)
    updateScreen();
}

void hungryAndTimeManagement(){

  if (rtc.getTime() > FedStatus.nextMeal) {
    FedStatus.isFed = false;
  }

  if (!FedStatus.isFed) {
    int secondsHungry = rtc.getTime().daySeconds() - FedStatus.nextMeal.daySeconds();
    FedStatus.hoursHungry = secondsHungry / 3600;
  }
}

//10 ms timer
void timer_handle_interrupts(int timer) {
  tenMsExpired = true;

  static uint8_t counter = 0;
  counter++;
  if (counter == 5) {
    counter = 0;
    keyHandleStatus = true;

    static uint8_t cnt2 = 0;
    cnt2++;
    if (cnt2 == 5){
      cnt2 = 0;
      batteryHandleStatus = true;
    }
  }

  if (Timers.debouncingStatus) {
    Timers.debouncing += 10;
  }

  if (Timers.extDebouncingStatus) {
    Timers.extDebouncing += 10;
  }
}

void powerHandle() {

  static bool lastBacklightStatus = true;
  if (lastBacklightStatus != backlightStatus) {
    lastBacklightStatus = backlightStatus;
    lcd.display();
    digitalWrite(LEDlcdPin, backlightStatus ? HIGH : LOW);
    
    if (!backlightStatus){  
      digitalWrite(RTC_POWER_PIN, LOW);
      lcd.noDisplay();
      power.hardwareDisable(PWR_ALL);
      power.setSleepMode(POWERDOWN_SLEEP);
      power.sleep(SLEEP_FOREVER);
      power.hardwareEnable(PWR_ALL);
      digitalWrite(RTC_POWER_PIN, HIGH);  
      usbConnectionStatus = false;
    }
  }
}

void keysHandle() {

  if (externalButtonClicked){
    externalButtonClicked = false;

    if (!Timers.extDebouncingStatus) {
      Timers.extDebouncingStatus = true;
    } else {
      if (Timers.extDebouncing >= extButtonDebouncingTimer_TO) {
        Timers.extDebouncingStatus = false;
        Timers.extDebouncing = 0;

        interact(Key::ENTER);
        updateScreen();
      }
    }  
  }
  
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

void handleExternalButtonInterrupt(){
  externalButtonClicked = true;
}

void batteryHandle(){

  if (!batteryHandleStatus) return;
  batteryHandleStatus = false;

  BatteryInfo.isCharging = digitalRead(BATTERY_CHARGING_PIN) == 1;
  BatteryInfo.isDoneCharging = digitalRead(BATTERY_DONE_CHG_PIN) == 1; 

  if (usbConnectionStatus){
    Serial.print("BAT CHG: ");
    Serial.println(digitalRead(BATTERY_CHARGING_PIN));
    Serial.print("BAT RDY: ");
    Serial.println(digitalRead(BATTERY_DONE_CHG_PIN));
  }

  setBatteryPercentage();
}

void setBatteryPercentage(){

  uint16_t batteryAdcData = analogRead(BATTERY_VOLTAGE_ADC_PIN);
  if (usbConnectionStatus){
    Serial.print("BAT ADC: ");
    Serial.println(batteryAdcData);
  }

  float batteryVoltage = 5.0f * (batteryAdcData/1024.0f);

  if (usbConnectionStatus){
    Serial.print("BAT VOLTAGE: ");
    Serial.println(batteryVoltage);
  }

  float minBasedBatteryVoltage = batteryVoltage - chargeMinimumVoltage;

  if (minBasedBatteryVoltage < 0){
    BatteryInfo.chargePercent = 0;
    return;
  }

  float minBasedMaxValue = chargeMaximumVoltage - chargeMinimumVoltage;

  float percentageFloat = minBasedMaxValue / minBasedBatteryVoltage;
  if (percentageFloat > 1)
    BatteryInfo.chargePercent = 100;
  else
    BatteryInfo.chargePercent = percentageFloat * 100;
}
