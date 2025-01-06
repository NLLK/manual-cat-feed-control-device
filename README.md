# Manual cat feed control device

The goal of this device is to contol how often cat is feeded by a family of many people. It have to show the last time cat was fed and next time to feed a cat according to schedule.

# Hardware
I used an Arduino Leonardo board (as it was accessible and already had all periphery devices) and some board with 16*2 LCD display and buttons. DS3231 controller was used for RTC.

# Software
It's meant to be easy to port to any other Arduino device as it's doesn't contain strict dependencies beside widly used GyverDS3231 lib as RTC provider.

The program itself has 3 screens.
To change the screen one needs to press the "LEFT" key once. Nice smooth animation will be played and will show a new screen. This works for all following screens.

## Main
Shows current date and time; time when a cat was last fed; and when it will be fed next time according to the 3-meals per day schedule configured with screens described later on.

**Screen 1: Last fed**

| | | | | | | | | | | | | | | | |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 0 | 6 | . | 0 | 1 |   | 1 | 2 | : | 3 | 5 |   |   |   |   |   |
| L | A | S | T | : |   | 1 | 2 | : | 1 | 4 |   |   | F | E | D |

**Screen 2: Next meal**

| | | | | | | | | | | | | | | | |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 0 | 6 | . | 0 | 1 |   | 1 | 2 | : | 3 | 5 |   |   |   |   |   |
| N | E | X | T | : |   | 1 | 8 | : | 0 | 0 |   |   | F | E | D |

When it's time to feed the cat, cat is considered to be hungry. The level of "hungrieness" is shown by '*' symbols in the upper-right corner. 
If a cat is hungry more than 3 hours, the display will starts flashing showing that it needs a feeder attention.

**Screen 3: Cat is hungry**

| | | | | | | | | | | | | | | | |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 0 | 6 | . | 0 | 1 |   | 1 | 8 | : | 0 | 1 |   |   |   |   |   |
| N | E | X | T | : |   | 1 | 8 | : | 0 | 0 |   | H | N | G | R |

**Screen 3: Cat is super hungry**

| | | | | | | | | | | | | | | | |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 0 | 6 | . | 0 | 1 |   | 2 | 2 | : | 0 | 1 |   | * | * | * | * |
| N | E | X | T | : |   | 1 | 8 | : | 0 | 0 |   | H | N | G | R |

## Schedule settings

This screen allows user to change the schedule settings stored in EEPROM. Navigation principle is follows: one changes cursor, that points to value, by pressing the "ENTER" key; and "UP" and "DOWN" keys allow to change the value. Once values are selected, one will be promted to save the settings. Press the "UP" key to save settings, or "DOWN" (or skip by pressing "ENTER" or change screen) to cancel.

**Screen 4: Schedule settings**

| | | | | | | | | | | | | | | | |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | . | 0 | 8 | : | 0 | 0 |   | S | E | T | T | I | N | G | S |
| 2 | . | 1 | 8 | : | 0 | 0 |   | 3 | . | 2 | 3 | : | 0 | 0 |   |

**Screen 4: Schedule settings: Save prompt**

| | | | | | | | | | | | | | | | |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | . | 1 | 0 | : | 0 | 0 |   | S | A | V | E | ? |   |   | <ins>V</ins> |
| 2 | . | 1 | 8 | : | 0 | 0 |   | 3 | . | 2 | 3 | : | 0 | 0 |   |

## Timeset

The same navigation principle is used for this screen. It allows a user to change the RTC time.

| | | | | | | | | | | | | | | | |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
|   |   |   |   |   |   |   |   |   | T | I | M | E | S | E | T |
| 2 | 0 | 2 | 5 | / | 0 | 1 | / | 0 | 6 |   | 1 | 8 | : | 5 | 7 |