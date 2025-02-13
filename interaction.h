#ifndef INTERACTION_H
#define INTERACTION_H

#include <EEPROM.h>

#include "common.h"
#include "presentation.h"

void interact(Key key);
void interact_MAIN(Key key);
void interact_SETTINGS(Key key);
void interact_TIMESET(Key key);

void UC_CatIsFed();
void UC_SetNextMeal();
void UC_SetMealsToNextDay();
void UC_SetMealsForToday();
void UC_saveSettings();
void UC_finishAnimation();

void loadSettings();
void screenAnimationHandle();

#endif