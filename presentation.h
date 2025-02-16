#ifndef PRESENTATION_H
#define PRESENTATION_H

#include "common.h"

void updateScreen();
void updateScreen_MAIN();
void updateScreen_SETTINGS();
void updateScreen_TIMESET();
void printHoursAndMinutes(int hours, int minutes);
void changeScreen(ScreenType newScreen);

#endif